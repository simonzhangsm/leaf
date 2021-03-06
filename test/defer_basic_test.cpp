// Copyright (c) 2018 Emil Dotchevski
// Copyright (c) 2018 Second Spectrum, Inc.

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/leaf/preload.hpp>
#include <boost/leaf/handle.hpp>
#include "boost/core/lightweight_test.hpp"

namespace leaf = boost::leaf;

int global;

int get_global() noexcept
{
	return global;
}

struct info
{
	int value;
};

leaf::error_id g()
{
	global = 0;
	auto propagate = leaf::defer( [ ] { return info{get_global()}; } );
	global = 42;
	return leaf::new_error();
}

leaf::error_id f()
{
	return g();
}

int main()
{
	int r = leaf::handle_all(
		[ ]() -> leaf::result<int>
		{
			return f();
		},
		[ ]( info const & i42 )
		{
			BOOST_TEST(i42.value==42);
			return 1;
		},
		[ ]
		{
			return 2;
		} );
	BOOST_TEST(r==1);
	return boost::report_errors();
}
