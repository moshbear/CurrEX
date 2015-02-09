//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Unit test/block for final output.
//
// Input: first line contains output of graph:
// 		a semicolon-delimited list of vertices followed by the log-rate;
// 	  subsequent lines are evaluator inputs for which the log-rate evaluates
// 	  revenue and profit.
// Output: revenue."(".profit")." for each evaluator input
#include <cmath>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

int main() {
	std::string path;
	double lrate;
first:
	if (!std::cin.good())
		throw std::invalid_argument("stdin unvailable");
	std::string line;
	std::getline(std::cin, line);
	if (!line.size())
		goto first;
	boost::char_separator<char> sep(" ");
	boost::tokenizer<decltype(sep)> line_tokens(line, sep);
	std::vector<std::string> toks;
	for (auto const& t: line_tokens)
		toks.push_back(t);
	path = toks[0];
	lrate = boost::lexical_cast<double>(toks[1]);
	double x;
	while (std::cin >> x) {
		std::cout << exp(log(x) - lrate)
			  << " (" << (exp(log(x) - lrate) - x)
			  << ")"  << std::endl;
	}
}


