//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Unit test/block for rates module.

// Args: optional; -d enables printing of header; -s suppresses it
// Input: each line of stdin specifies an instrument to query
// Output: each line of output contains { Instrument Bid Ask } for each input Instrument, space-delimited

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include <rates.hh>

#define ARGCHK(x,a) !strncmp(x, a, sizeof a)

int main (int argc, char** argv) {
	bool print_hdr = false;
	if (argc >= 2) {
		if (ARGCHK(argv[1], "-d"))
			print_hdr = true;
		else if(ARGCHK(argv[1], "-s"))
			print_hdr = false;
	}

	std::vector<std::string> instruments;
	while (std::cin.good()) {
		std::string line;
		std::getline(std::cin, line);
		if (!line.size())
			continue;
		instruments.push_back(line);
	}

	auto data = rates::get(instruments);
	if (print_hdr)
		std::cout << "Instrument Bid Ask\n";
	for (const auto& price : data) 
		std::cout << price.instrument << ' ' << price.bid << ' ' << price.ask << '\n';
	std::cout.flush();

	return 0;
}
