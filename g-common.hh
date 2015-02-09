//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Common graph namespaces and functions.
//
#ifndef G_COMMON_HH
#define G_COMMON_HH

#include <cstddef>
#include <utility>
#include <set>
#include <vector>
#include <array>
#include <iterator>
#include <fstream>
#include <type_traits>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graphviz.hpp>

#include <util.hh>

// Namespace aliasing of parts of BGL that are used.
namespace bgl {
	// adjacency_list
	using boost::listS;
	using boost::vecS;
	using boost::undirectedS;
	using boost::directedS;
	using boost::adjacency_list;
	// graph concepts
	using boost::add_edge;
	using boost::remove_edge;
	using boost::source;
	using boost::target;
	using boost::edge;
	using boost::edges;
	using boost::vertices;
	using boost::num_vertices;
	using boost::clear_vertex;
	using boost::remove_vertex;
	using boost::adjacent_vertices;
	using boost::in_edges;
	using boost::out_edges;
	using boost::num_edges;
	using boost::in_degree;
	using boost::out_degree;
	// visitor
	using boost::visitor;
	using boost::dfs_visitor;
	using boost::depth_first_search;
	// color
	using boost::default_color_type;
	// property
	using boost::graph_traits;
	using boost::property_traits;
	using boost::color_traits;
	using boost::vertex_index;
	using boost::get;
	using boost::put;
	using boost::make_iterator_property_map;
	// property tags
	using boost::edge_property_tag;
	using boost::property;
	using boost::no_property;
}

namespace g_common {

	// VE<Graph>
	// Alias for BGL Edge and Vertex types.
	// Contains casting helpers as deemed necessary.
	//
	// TArg: Graph - Graph type
	template <typename Graph>
	struct VE {
		// Vertex [descriptor type]
		typedef typename bgl::graph_traits<Graph>::vertex_descriptor Vertex;
		// Edge [descriptor type]
		typedef typename bgl::graph_traits<Graph>::edge_descriptor Edge;
		// Vertex V<T>(T)
		// Cast a value to a Vertex if the following two conditions hold:
		// 	(1) T is convertible to Vertex; and
		// 	(2) t is representable by Vertex (e.g. no negatives, etc).
		// 	Violation of (1) yields a compile-time error; and
		// 	Violation of (2) yields a run-time std::out_of_range.
		// This is used mostly for signed/unsigned conversions.
		//
		// (TArg): T - input type
		// Arg: t - input value
		// Ret: t cast to Vertex, if such a conversion is well-formed
		// Throw: std::out_of_range if a range check error is detected in util::checked_cast
		template <typename T>
		static auto V(T t) 
		-> typename std::enable_if<std::is_convertible<T, Vertex>::value, Vertex>::type
		{
			return util::checked_cast<Vertex>(t);
		}
	};

	// Type compatibility checker.
	// As Vertex,Edge,etc are moved into template parameters, the risk of
	// over-accepting incompatible type combinations increases.
	// This puts a stop to that.
	//
	// TArg: G - Graph type
	template <typename G>
	struct check {
		// General notes:
		//
		// All functions do compile-time checking in two ways:
		// (1) static_assert
		// (2) constexpr bool (...)
		//
		// C++11 is strict about allowed statements in constexpr functions:
		// 	- aliases (typedef, using, etc)
		// 	- static_assert
		// 	- precisely one return-statement
		// Use preprocessor to hack around this with G_COMMON_CHECK

		// static bool v<V>(V).
		// Check that V is the same as VE<G>::Vertex.
		//
		// (TArg): V - candidate vertex type
		// Arg: V - candidate vertex type (used only for type capture)
		// Ret: std::is_same<V, VE<G>::Vertex>::value
		template <typename V>
		constexpr static bool v(V) {
		#define G_COMMON_CHECK std::is_same<V, typename VE<G>::Vertex>::value
			static_assert(G_COMMON_CHECK, "Supplied Vertex differs in type with Vertex derived from Graph");
			return G_COMMON_CHECK;
		#undef G_COMMON_CHECK
		}

		// static bool e<E>(E).
		// Check that E is the same as VE<G>::Edge.
		//
		// (TArg): E - candidate edge type
		// Arg: E - candidate edge typee (used only for type capture)
		// Ret: std::is_same<E, VE<G>::Edge>::value
		template <typename E>
		constexpr static bool e(E) {
		#define G_COMMON_CHECK std::is_same<E, typename VE<G>::Edge>::value
			static_assert(G_COMMON_CHECK, "Supplied Edge differs in type with Edge derived from Graph");
			return G_COMMON_CHECK;
		#undef G_COMMON_CHECK
		}

		// static bool ve<V,E>(V,E)
		// Check that {V,E} are the same as VE<G>::{Vertex,Edge}
		//
		// (TArg): V - candidate vertex type
		// (TArg): E - candidate edge type
		// Arg: V - candidate vertex type (used only for type capture)
		// Arg: E - candidate edge typee (used only for type capture)
		// Ret: v<V>(V) && e<E>()
		template <typename V, typename E>
		constexpr static bool ve(V, E) {
			return v<V>() && e<E>();
		}
	};

	// Path<Graph>
	// A typeholder for a path of vectors.
	//
	// TArg: Graph - Graph type
	template <typename Graph>
	struct Path {
		typedef std::vector<typename VE<Graph>::Vertex> type;
	};

	// Path<G>::type close_path(Path<G>).
	// Close the open path denoted by input.
	// Specifically, push_back(*begin()) is called to close it.
	// Pass-by-value to induce copying is intended.
	//
	// (TArg): Graph - Graph type
	// Arg: Path<Graph>::type open_path - an open path to close
	// Ret: closed form of open_path
	template <typename Graph>
	auto close_path(typename Path<Graph>::type open_path)
	-> typename Path<Graph>::type
	{
		open_path.push_back(*(open_path.begin()));
		return open_path;
	}

	// std::array<size_t, 2> degree<G>(G const&, Vertex).
	// Fetch the number of in and out edges of a Vertex in G.
	// That is, the in and out degrees of a Vertex in G.
	//
	// (TArg): Graph - Graph type
	// (TArg): Vertex - Vertex type
	// Arg: Graph const& g - Graph
	// Arg: Vertex v - a vertex in `g`
	// Ret: [ # in degree, # out degree ]
	template <typename Graph, typename Vertex = typename VE<Graph>::Vertex>
	auto degree(Graph const& g, Vertex v)
	-> std::array<size_t, 2>
	{
		check<Graph>::v(Vertex());
		std::array<size_t, 2> deg {{ bgl::in_degree(v, g), bgl::out_degree(v, g) }};
	#if 0
		auto inp = bgl::in_edges(v, g);
		auto outp = bgl::out_edges(v, g);
		deg[0] = std::distance(inp.first, inp.second);
		deg[1] = std::distance(outp.first, outp.second);
	#endif
		return deg;
	}

	// std::set<Vertex> out_vertices<G>(G const&, Vertex, bool filter_loops = false).
	// Fetch the set of adjacent output vertices of a vertex in G.
	//
	// (TArg): Graph - Graph type
	// (TArg): Vertex - Vertex type
	// Arg: Graph const& g - input graph
	// Arg: Vertex vtx - vertex in `g`
	// [Arg]: bool filter_loops - whether to filter out loops so
	// 			that `v` will never occur in output; default is false
	// Ret: std::set<Vertex> consisting of out vertices of `vtx` in `g`.
	template <typename Graph, typename Vertex = typename VE<Graph>::Vertex>
	auto out_vertices(Graph const& g, Vertex vtx, bool filter_loops = false)
	-> std::set<Vertex>
	{
		check<Graph>::v(Vertex());
		std::set<Vertex> out_vertices;
		for (auto tgt : util::pair_to_range(bgl::adjacent_vertices(vtx, g))) 
			if (!filter_loops || tgt != vtx) 
				out_vertices.insert(tgt);
		return out_vertices;
	}

	// void to_gv_dotfile<G>(G const&, const char*).
	// Write a graph to a file specified by path, formatted as a GraphViz Dot file.
	//
	// (TArg): Graph - Graph type
	// Arg: Graph const& g - input graph
	// Arg: char const* fname - file name
	template <typename Graph>
	void to_gv_dotfile(Graph const& g, char const* fname) {
		std::ofstream f(fname);
		boost::write_graphviz(f, g);
		f.close();
	}

}

#endif
