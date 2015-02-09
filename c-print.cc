//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// c_print constants

#include <cstddef>
#include <c-print.hh>

#include <ext/prettyprint.hpp>

namespace c_print {

	template <>
	const printer_delims_vals<char> printer_delims<char>::values = { " (", "(", "): ", "\n" };
	template <>
	const printer_delims_vals<wchar_t> printer_delims<wchar_t>::values = { L" (", L"(", L"): ", L"\n" };

}

