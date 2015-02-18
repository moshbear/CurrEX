//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Pruner implementation.

// For debugging, the numerical form of the graph is printed a GraphViz DOT file `pre.dot`
// After pruning, ditto `post.dot`
// If NDEBUG not #define'd, debug prints are sent to stderr

#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <array>
#include <functional>
#include <stdexcept>
#include <type_traits>

#include <algo.hh>
#include <c-print.hh>
#include <g-common.hh>
#include <d.hh>
#include <pruner.hh>


namespace {
// Directedness-invariant DFS visitor for cycle detection
	class cycle_detector_t : public bgl::dfs_visitor<> {
		typedef std::vector<bool> bitvec;
	public:
		cycle_detector_t(bitvec& ev_)
		: ev(ev_) { }

		template <typename Edge, typename Graph>
		void back_edge(Edge e, const Graph& g) {
			auto v = bgl::target(e, g);
			ev.get()[v] = true;
		}
	private:
		std::reference_wrapper<bitvec> ev;
	};
}

using std::vector;
using std::stringstream;
using std::string;
// <stdexcept>
using std::invalid_argument;
using std::out_of_range;
// <array>
using std::array;
// <functional>
using std::ref;
// <boost/graph/*>

typedef bgl::adjacency_list<bgl::vecS, bgl::vecS, bgl::undirectedS> graph;

static const char node_sep = '_';


vector<string> prune_vertices(vector<string> const& input) {
	D_push_id(pruner);

	// graph construction
	vector<string> nodes;
	vector<string> output;

	vector<array<string, 2>> unparsed;
	vector<array<g_common::VE<graph>::Vertex, 2>> edges;

	std::ostream& _d_cout = D_out;
	D_DELAY;
	D_print(D_info, _d_cout, "load graph");

	for (auto const& line : input) {
		auto pos = line.find(node_sep);
		if (pos == std::remove_reference<decltype(line)>::type::npos || pos == 0 || pos == (line.size() - 1))
			throw invalid_argument("Bad input");
		string u = line.substr(0, pos);
		string v = line.substr(pos + 1);
		unparsed.push_back({{u, v}});
		// find the corresponding node numbers
		decltype(nodes)::difference_type upos, vpos, npos;
		npos = algo::npos<decltype(nodes)>::value;
		if ((upos = algo::index_of(nodes, u)) == npos) {
			upos = util::checked_cast<decltype(upos)>(nodes.size());
			nodes.push_back(u);
		}
		if ((vpos = algo::index_of(nodes, v)) == npos) {
			vpos = util::checked_cast<decltype(vpos)>(nodes.size());
			nodes.push_back(v);
		}
		D_print(D_trace, _d_cout, [&] { stringstream s;
					s << "Load edge: " << u << node_sep << v << " -> [" << upos << "]->[" << vpos << "]";
					return string(s.str()); }());
		// load edge into graph
		edges.push_back({{g_common::VE<graph>::V(upos), g_common::VE<graph>::V(vpos)}});
	}


	D_eval(D_trace, _d_cout << D_add_context(D_trace) << ' '
				  << c_print::printer(nodes, "Nodes") << '\n');

	graph g;
	for (const auto& e: edges) 
		bgl::add_edge(e[0], e[1], g);

	D_eval(D_trace, g_common::to_gv_dotfile(g, "pre.dot"));

	vector<string> removed;
	D_print(D_info, _d_cout, "pre-prune lone vertices");
	for (auto v_end = bgl::num_vertices(g), v = v_end - 1; v < v_end; --v) {
		auto d = g_common::degree(g, v);
		if (d[0] < 2 && d[1] < 2) {
			bgl::clear_vertex(v, g);
			bgl::remove_vertex(v, g);
			removed.push_back(nodes[util::checked_cast<decltype(nodes)::size_type>(v)]);
			algo::erase_at(nodes, util::checked_cast<decltype(nodes)::difference_type>(v));
		}
	}
	D_eval(D_trace, _d_cout << D_add_context(D_trace) << ' '
				  << c_print::printer(removed, "Removed vertices") << '\n');
	removed.clear();

	// cycle detection
	D_print(D_info, _d_cout, "find cycles");

	vector<bool> cyclic(bgl::num_vertices(g), false);
	cycle_detector_t cycle_detector(cyclic);

	bgl::depth_first_search(g, visitor(cycle_detector));

	D_print(D_info, _d_cout, "prune acyclics");


	// prune in reverse direction since bgl invalidates vertex descriptors following on removal
	for (auto v_end = cyclic.size(), v = v_end - 1; v < v_end; --v) 
		if (!cyclic[v]) {
			bgl::clear_vertex(v, g);
			bgl::remove_vertex(v, g);
			removed.push_back(nodes[util::checked_cast<decltype(nodes)::size_type>(v)]);
			algo::erase_at(nodes, util::checked_cast<decltype(nodes)::difference_type>(v));
		}

	// print output
	D_eval(D_trace, _d_cout << D_add_context(D_trace) << ' '
				  << c_print::printer(removed, "Removed vertices") << '\n');

	D_eval(D_trace, _d_cout << D_add_context(D_trace) << ' '
				  << c_print::printer(nodes, "New vertices") << '\n');
	D_eval(D_trace, g_common::to_gv_dotfile(g, "post.dot"));
	vector<array<std::string, 2>> new_edges;
	auto es = bgl::edges(g);
	for (auto eit = es.first; eit != es.second; ++eit) {
		auto u_id = bgl::source(*eit, g);
		auto v_id = bgl::target(*eit, g);
		try { // UB Paranoia.
			const auto& u = nodes.at(u_id);
			const auto& v = nodes.at(v_id);
			output.push_back(u + node_sep + v);
			new_edges.push_back({{u, v}});
		} catch (out_of_range&) {
			D_print(D_err, _d_cout, [&] { stringstream s;
						s << "**UB** edge [" << u_id << "]->[" << v_id << "]";
						return string(s.str()); }());
		}
	}
	D_eval(D_trace, _d_cout << D_add_context(D_trace) << ' '
				  << c_print::printer(new_edges, "New edges") << '\n');
	D_eval(D_err, _d_cout << std::flush);
	return output;
}

