//Copyright (c) 2018 Emil Dotchevski
//Copyright (c) 2018 Second Spectrum, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//This is a simple program that demonstrates the use of LEAF to transport error info between threads,
//when using exception handling to report failures. See transport_ec.cpp for the variant that doesn't
//use exception handling.

#include <boost/leaf/all.hpp>
#include <vector>
#include <string>
#include <future>
#include <iterator>
#include <iostream>
#include <algorithm>

namespace leaf = boost::leaf;

//Define several error info types.
struct failed_thread_id { std::thread::id value; };
struct failure_info1 { std::string value; };
struct failure_info2 { int value; };
struct failure_info3 { long value; };
struct failure_info4 { float value; };

//A type that represents a successfully returned result from a task.
struct task_result { };

//An exception type thrown in case of task failure.
struct failure : virtual std::exception { };

//This is a test task which succeeds or fails depending on its argument. In case of success, it returns
//task_result, in case of error it throws failure.
task_result task( bool succeed )
{
	if( succeed )
		return task_result(); //Simulate successful result.
	else
		leaf::throw_exception( failure(),
			failed_thread_id{std::this_thread::get_id()},
			failure_info1{"info"},
			failure_info2{42},
			failure_info4{42} );
}

//Launch the specified number of asynchronous tasks. In case an asynchronous task throws, its error info
//(of the type list used to instantiate leaf::capture_exception) is captured and wrapped in a different exception,
//which transports it to the main thread. The original exception is then recovered by leaf::get.
template <class... E>
std::vector<std::future<task_result>> launch_async_tasks( int thread_count )
{
	std::vector<std::future<task_result>> fut;
	std::generate_n( std::inserter(fut,fut.end()), thread_count, [ ]
	{
		return std::async( std::launch::async,
			leaf::capture_exception<E...>( [ ] //returns a wrapper function for the lambda...
				{
					return task((rand()%4)!=0); //...which transports the E... objects.
				} ) );
	} );
	return fut;
}

int main()
{
	//Launch tasks, transport the specified types of error info. For demonstration, note that the task provides
	//failure_info4 which we don't care about, and that we say we could use failure_info3, but which the
	//task doesn't provide. So, we'll only get failed_thread_id, failure_info1 and failure_info2.
	auto fut = launch_async_tasks<failed_thread_id, failure_info1, failure_info2, failure_info3>(42);

	//Collect results or deal with failures.
	for( auto & f : fut )
	{
		f.wait();

		//Storage for error info.
		leaf::expect<failed_thread_id, failure_info1, failure_info2, failure_info3> exp;

		try
		{
			//Instead of calling future::get we pass the future object to leaf::get. In case the future finished with an exception,
			//this will rethrow that exception, after dropping captured error info into exp.
			task_result r = leaf::get(f);

			//Success!
			std::cout << "Success!" << std::endl;
		}
		catch( failure const & e )
		{
			//Failure! Handle the error, print failure info.
			handle_exception( exp, e,
				leaf::match<failure_info1, failure_info2, failed_thread_id>( [ ] ( std::string const & v1, int v2, std::thread::id tid )
					{
					std::cerr << "Error in thread " << tid << "! failure_info1: " << v1 << ", failure_info2: " << v2 << std::endl;
					} ) );
		}
	}
}
