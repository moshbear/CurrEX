//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// HTTP common functions.
//
// This is basically a blending of the common code in fetch-instr and rates, in
// the spirit of minimizing code duplication.

#ifndef HTTP_HH
#define HTTP_HH

#include <array>
#include <string>

namespace http {

// Make a HTTP GET request, given protocol uri[0], domain uri[1], query_string uri[2]
// Throws std::invalid_argument if non-200 status returned by remote side
std::string query(std::array<std::string, 3> uri);

}

#endif
