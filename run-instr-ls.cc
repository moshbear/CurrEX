//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Unit test/block for instruments module

// Output: each line of output contains an Instrument; graph is not guaranteed cyclic

#include <iostream>
#include <string>
#include <vector>

#include <instr-ls.hh>

int main() {

	// parse and print
	auto data = instruments::list();
	for (const auto& instr : data)
		std::cout << instr << '\n';
	std::cout.flush();

	return 0;
}
