//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Unit test/block for pruner module

// Input: Each line of stdin specifies a ${SRC_LABEL}_${DST_LABEL} edge
// Output: New edges printed to stdout
// For debugging, the numerical form of the graph is printed a GraphViz DOT file `pre.dot`
// After pruning, ditto `post.dot`

#include <iostream>
#include <vector>
#include <cstring>
#include <cctype>
#include <string>
#include <stdexcept>

#include <pruner.hh>
#include <d.hh>

using std::vector;
using std::cerr;
using std::string;
// <stdexcept>
using std::invalid_argument;
using std::out_of_range;

int main (int argc, char** argv) {
	D_set_from_args(argc - 1, argv + 1, "-d");

	std::vector<std::string> input;
	while (std::cin.good()) {
		std::string line;
		std::getline(std::cin, line);
		if (!line.size())
			continue;
		input.push_back(line);
	}
	auto res = pruner(input);
	for (auto const& e : res)
		std::cout << e << '\n';
	std::cout.flush();
}

