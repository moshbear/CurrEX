//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Rate graph namespaces and functions
//
#ifndef G_RATEGRAPH_HH
#define G_RATEGRAPH_HH

#include <cstddef>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

#include <d.hh>
#include <algo.hh>
#include <c-print.hh>
#include <util.hh>
#include <g-common.hh>
#include <g-color.hh>

namespace g_rategraph {

	// Construct a rategraph bgl graph

	// Edge property: rate
	struct Edge_property {
		double rate;
	};
	struct Rate_tag {
		typedef bgl::edge_property_tag kind;
	};
	typedef bgl::property<Rate_tag, Edge_property> Rate_property;

	typedef bgl::adjacency_list<bgl::vecS, bgl::vecS, bgl::directedS, bgl::no_property, Rate_property> Graph;

	using VE = g_common::VE<Graph>;

	// Rated_path.
	// A rated path container.
	// Couples a path and its log-rate together.
	struct Rated_path {
		// A vector of vertices denoting an open path.
		typedef g_common::Path<Graph>::type path_type;
		path_type path;
		// The log-rate of the path
		typedef double lrate_type;
		lrate_type lrate;

		Rated_path() = default;
		template <typename P_>
		Rated_path(P_&& path_, lrate_type lrate_)
		: path(std::forward<P_>(path_)), lrate(lrate_)
		{ }

		// Getter for Path. Needed since SWIG marshals native to interpreted structures
		// only when arguments of functions.
		path_type get_path() const;
	};
	// Print a Rated_path as a std::tuple
	template <typename CT, typename TT>
	std::basic_ostream<CT,TT>& operator<< (std::basic_ostream<CT,TT>& os, Rated_path const& p)
	{
		os << std::tie(p.path, p.lrate);
		return os;
	}

	// void load_edge_pair(Graph&, Vertex, Vertex, double ask_rate, double bid_rate)
	// Add an edge pair between two vertices to a graph, with forward weight -log ask_rate and backward weight bid_rate.
	//
	// Arg: G& g - Graph to update
	// Arg: Vertex u - edge source
	// Arg: Vertex v - edge target
	// Arg: double ask_rate - Forward cost
	// Arg: double bid_rate - Reverse cost
	void load_edge_pair(Graph& g, typename VE::Vertex u, typename VE::Vertex v, double ask_rate, double bid_rate);

	// Rated_path find_initial_simplex(Graph const&)
	// Find the greedily best starting simplex from which to start incremental path expansion.
	//
	// Arg - Graph const& g - Graph to search
	// Ret: Rated_path containing best input path
	Rated_path find_initial_simplex(Graph const& g);

	// Rated_path do_iteration(Graph const&, Rated_path& const)
	// Do an iteration of expansion. The rated path is used to carry the path.
	//
	// Arg: Graph const& g - Input rate graph
	// Arg: Rated_path const& iter - Input Rated_path whose path is carried as input into expansion algorithm.
	// Ret: Rated_path containing best one-iteration expansion of input path
	Rated_path do_iteration(Graph const& g, Rated_path const& iter);

}

#endif
