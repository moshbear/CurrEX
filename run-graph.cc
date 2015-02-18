//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
//
// Unit test/block for graph module.

// Input: A list of rates
// Output: The best path. An observation is made if this path is hamiltonian.
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cctype>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <d.hh>
#include <g-common.hh>
#include <g-color.hh>
#include <g-rategraph.hh>
#include <rates.hh>
#include <labeled.hh>
#include <graph.hh>

// <iostream>, <string>
using std::getline;
// <iostream>
using std::cout;
using std::cerr;
using std::string;
using std::vector;
// <stdexcept>
using std::invalid_argument;
using std::out_of_range;

using std::array;

int main(int argc, char** argv) {
	D_push_id(run_graph);
	D_set_from_args(argc - 1, argv + 1, "-d");

//	int nperm = 10;

	// graph construction
	vector<string> nodes;

	labeled::Graph<g_rategraph::Graph> lg;
	graph::Input_description vrates;

	while (std::cin.good()) {
		string line;
		getline(std::cin, line);
		if (!line.size())
			continue;
		boost::char_separator<char> sep(" ");
		boost::tokenizer<decltype(sep)> line_tokens(line, sep);
		vector<string> toks;
		for (auto const& t : line_tokens)
			toks.push_back(t);
		
		if (toks.size() != 3)
			throw invalid_argument("Bad input: `" + line + "'");
		// line ~ /u_v bid ask/
		string uv = toks[0];
		double bid = boost::lexical_cast<double>(toks[1]);
		double ask = boost::lexical_cast<double>(toks[2]);
		vrates.push_back(rates::Rate(uv, bid, ask));
	}
	graph::load_graph_from_rates(lg, vrates);

	auto op = graph::best_path(lg);
	
	auto const& P (op.path);
	if (P.size() == bgl::num_vertices(lg.graph))
		D_print(D_info, std::cerr, "Hamiltonian.");
	bool p_once = true;
	for (auto const& p : P)
		std::cout << lg.labels[p] << ((p != P[0] || p_once) ? ((p_once = false), ";") : "");
	std::cout << ' ' << op.lrate << std::endl;
}

