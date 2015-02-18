//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Graph code. Templated since concept match gives less expressive constraints than type match.
//
#ifndef GRAPH_HH
#define GRAPH_HH

#include <iostream>
#include <sstream>
#include <array>
#include <vector>
#include <string>
#include <utility>


#include <d.hh>
#include <rates.hh>
#include <g-common.hh>
#include <g-rategraph.hh>
#include <labeled.hh>

namespace graph {
	D_push_id(graph);

	// Type-alias output of rate::get for load_graph_from_rates
	typedef std::vector<Rate> Input_description;

	typedef g_rategraph::Graph Graph;

	// Reload
	// Hold the modifications that occurred when reloading a Graph with a new description.
	struct Reload
	{
		typedef g_common::VE<Graph>::Vertex Vertex;
		typedef std::vector<Vertex> Edge;
		
		// indices of removed vertices
		std::vector<Vertex> removed_vertices;
		// vertex-pair indices of removed edges
		std::vector<Edge> removed_edges;
		// indices of added vertices, corrected for removed ones
		std::vector<Vertex> added_vertices;
		// vertex-pair indices of added edges, corrected for removed vertices
		std::vector<Edge> added_edges;
	};

	// std::vector<std::vector<std::string>::difference_type>
	// remap(std::vector<std::string> const&, std::vector<std::string> const&).
	// Translate old offsets into new ones, given offset vectors _old and _new.
	// 	If a translation is impossible for a given offset slot, that slot has value -1.
	//
	// Arg: std::vector<std::string> const& old_labels - the vector of old labels
	// Arg: std::vector<std::string> const& new_labels - the vector of new labels
	// Ret: A remapping vector such that (Ret[x] != 0 => Old[X] == New[Ret[X]])
	std::vector<std::vector<std::string>::difference_type>
	remap(std::vector<std::string> const& old_labels, std::vector<std::string> const& new_labels);

	// Reload load_graph_from_rates(Labeled_graph&, Input_description const&).
	// (Re)build a graph from a vector of rates.
	//
	// Arg: Labeled_graph& lg - output/updated labeled graph
	// Arg: Input_description const& rates - list of rates describing the graph
	// Ret: modifications done to the graph, contained in a Reload
	Reload load_graph_from_rates(Labeled_graph& lg, Input_description const& rates);

	// Rated_path best_path(Labeled_graph const&, size_t max_iterations = -1)
	// Compute the best path, subject to an optional specified iteration limit.
	//	0th iteration searches for initial 3-cycle and successive iterations build iteratively from that.
	//
	// Arg: Labeled_graph const& lg_in - input graph
	// Arg: size_t max_iterations - maximum iteration count
	// Ret: best path subject to the iteration count constraint
	g_rategraph::Rated_path best_path(Labeled_graph const& lg_in, long max_iterations = -1);

}

#endif
