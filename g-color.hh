//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Graph coloring namespaces and functions.
//
#ifndef G_COLOR_HH
#define G_COLOR_HH

#include <cstddef>
#include <iterator>
#include <set>

#include <algo.hh>
#include <g-common.hh>

namespace g_color {

	// std::set<Vertex>
	// 	unvisited_neighbors
	// 	<Graph, Vertex_colors, Color, Vertex>
	// 	(Graph const&, Vertex, Vertex_colors const&, Color, bool).
	// Fetch the set of unvisited vertices in G adjacent to a vertex.
	// 	Visitation state is kept track by Color_map and coloring is checked against
	//	Color.
	//	Inequality checking can be done by setting `eq` to false, allowing for
	//	whitelisting visitation colors.
	// Note: std::set is used in place of std::vertex due to use of std::set_intersection
	// 	 following.
	//
	// (TArg): Graph - Graph type
	// (TArg): Vertex_colors - BGL color property map type
	// (TArg): Color - Color type
	// [TArg]: Vertex - Vertex type; defaults to g_common::VE<Graph>::Vertex
	// Arg: Graph const& g - the graph to probe
	// Arg: Vertex u - the vertex whose adjacencies to search
	// Arg: Vertex_colors const& c - the BGL color property map describing visitation coloring
	// Arg: Color cv - the Color value to check against
	// [Arg]: bool eq - equality vs inequality check for removing unvisited neighbors; defaults to true
	// Ret: std::set<Vertex> of unvisited neighbors
	// Pre: each edge (u,v) in g has a corresponding edge (v,u) in g
	template <typename Graph, typename Vertex_colors, typename Color,
		  typename Vertex = typename g_common::VE<Graph>::Vertex>
	auto unvisited_neighbors(Graph const& g, Vertex u,
				 Vertex_colors const& c, Color cv, bool eq = true)
	-> std::set<Vertex>
	{
		g_common::check<Graph>::v(Vertex());

		auto out (g_common::out_vertices(g, u, true));

		// Filter out @cv-colored vertices
		algo::erase_if(out,
			[&c, cv, eq] (Vertex vtx) {
				return (bgl::get(c, vtx) == cv) ^ (!eq);
			});
		return out;
	}

	// std::set<Vertex>
	// 	intersecting_vertices
	// 	<Graph, Vertex_colors, Color, Vertex>
	// 	(Graph const&, Vertex, Vertex, Vertex_colors const&, Color, bool).
	// Fetch the set of unvisited vertices in G which are adjacent to both specified vertices.
	//	The return value of this function is the intersection of the sets returned by
	// 	unvisited_neighbors<...>(...) with the given arguments for each vertex.
	//
	// (TArg): Graph - Graph type
	// (TArg): Vertex_colors - BGL color property map type
	// (TArg): Color - Color type
	// [TArg]: Vertex - Vertex type; defaults to g_common::VE<Graph>::Vertex
	// Arg: Graph const& g - the graph to probe
	// Arg: Vertex u - the first vertex whose adjacencies to search
	// Arg: Vertex v - the second vertex whose adjacencies to search
	// Arg: Vertex_colors const& c - the BGL color property map describing visitation coloring
	// Arg: Color cv - the Color value to check against
	// [Arg]: bool eq - equality vs inequality check for removing unvisited neighbors; defaults to true
	// Ret: std::set<Vertex> of intersecting neighbors of `u` and `v` in `g`
	// Pre: each edge (u,v) in g has a corresponding edge (v,u) in g
	template <typename Graph, typename Vertex_colors, typename Color,
		  typename Vertex = typename g_common::VE<Graph>::Vertex>
	auto intersecting_vertices(Graph const& g, Vertex u, Vertex v,
				Vertex_colors const& c, Color cv, bool eq = true)
	-> std::set<Vertex>
	{
		g_common::check<Graph>::v(Vertex());

		auto out_u (unvisited_neighbors(g, u, c, cv, eq));
		auto out_v (unvisited_neighbors(g, v, c, cv, eq));

		// Get intersecting vertices
		std::set<decltype(u)> intersection;
		std::set_intersection(out_u.begin(), out_u.end(),
					out_v.begin(), out_v.end(),
					std::inserter(intersection, intersection.begin()));
		return intersection;
	}
}

#endif

