//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Graph labeling.
//
#ifndef LABELED_HH
#define LABELED_HH

#include <ostream>
#include <initializer_list>
#include <array>
#include <vector>
#include <string>
#include <utility>

#include <algo.hh>
#include <c-print.hh>
#include <util.hh>
#include <g-common.hh>

namespace labeled {

	// Labeling proxy objects.

	// Base class: cref-capture the string vector enumerating the labels
	struct Node
	{
	private:
		static const std::vector<std::string> _l0 ;
	public:
		std::reference_wrapper<const std::vector<std::string>> label;
		Node() : label(_l0) {}
		template <typename L>
		Node(L&& l) : label(std::forward<L>(l)) { }
	};
	// Labeling proxy type for vertices.
	// Printed output: vertex."(".label[vertex].")"
	//
	// TArg: Vertex - Vertex type
	template <typename Vertex>
	struct Vertex_t : public Node
	{
		Vertex vertex;
	};
	// Labeling proxy type for edges represented as pairs of vertices.
	// Printed output: edge[0]."->".edge[1]."(".label[edge[0]]."->".label[edge[1]].")"
	//
	// TArg: Vertex - Vertex type
	template <typename Vertex>
	struct Edge_t : public Node
	{
		std::array<Vertex, 2> edge;
	};
	template <typename VT>
	std::ostream& operator<< (std::ostream& os, Vertex_t<VT> const& v) 
	{
		os << v.vertex << "(" << v.label.get().at(v.vertex) << ")";
		return os;
	}
	template <typename VT>
	std::ostream& operator<< (std::ostream& os, Edge_t<VT> const& e)
	{
		os << e.edge[0] << "->" << e.edge[1] << "("
		   << e.label.get().at(e.edge[0]) << "->" << e.label.get().at(e.edge[1]) << ")";
		   return os;
	}
	// Labeling wrapper for a Vertex.
	//
	// (TArg): VT - Vertex type
	// Arg: Node const& node - the base Node which captured the label vector
	// Arg: VT const v - the vertex to label
	// Ret: a Vertex_T<VT> containing the deferred labeling of this vertex
	template <typename VT>
	Vertex_t<VT> vertex(Node const& node, VT const v)
	{
		Vertex_t<VT> lv;
		static_cast<Node&>(lv) = node;
		lv.vertex = v;
		return lv;
	}
	// Labeling wrapper for an Edge.
	//
	// (TArg): VT - Vertex type
	// Arg: Node const& node - the base Node which captured the label vector
	// Arg: std::array<VT, 2> const& e - the edge to label
	// Ret: a Vertex_T<VT> containing the deferred labeling of this edge
	template <typename VT>
	Edge_t<VT> edge(Node const& node, std::array<VT, 2> const& e)
	{
		Edge_t<VT> le;
		static_cast<Node&>(le) = node;
		le.edge = e;
		return le;
	}

	// std::vector<Vertex_t<T>>
	// 	labelify_vertices<T>
	// 	(std::vector<T> const&, std::vector<std::string> const&).
	// Labelify a set of vertices given a label vector by capturing the label vector and performing
	// a foreach over v with function vertex().
	//
	// (TArg): T - Vertex type
	// Arg: std::vector<T> const& v - a vector of vertices to label
	// Arg: std::vector<std::string> const& l - a vector of labels to cref-capture
	// Ret: a vector of Vertex_t<T>'s, with lazily evaluated labeling.
	template <typename T>
	std::vector<Vertex_t<T>>
	labelify_vertices(std::vector<T> const& v, std::vector<std::string> const& l)
	{
		Node ln{ std::cref(l) };
		std::vector<Vertex_t<T>> out;
		for (auto const& x : v)
			out.push_back(vertex(ln, x));
		return out;
	}
	// std::vector<Edge_t<T>>
	// 	labelify_edges<T>
	// 	(std::vector<std::array<T, 2>> const&, std::vector<std::string> const&).
	// Labelify a set of vertices given a label vector by capturing the label vector and performing 
	// a foreach over v with function vertex().
	//
	// (TArg): T - Vertex type
	// Arg: std::vector<T> const& e - a vector of edges to label
	// Arg: std::vector<std::string> const& l - a vector of labels to cref-capture
	// Ret: a vector of Edge_t<T>'s, with lazily evaluated labeling.
	template <typename T>
	std::vector<Edge_t<T>>
	labelify_edges(std::vector<std::array<T, 2>> const& e, std::vector<std::string> const& l)
	{
		Node ln{ std::cref(l) };
		std::vector<Edge_t<T>> out;
		for (auto const& x : e)
			out.push_back(edge(ln, x));
		return out;
	}

	// Graph<G>.
	// A labeled graph, where a graph is coupled with a vector of labels describing the vertices.
	//
	// Printing via ostream yields a tuple print of c_print printers, with "vertices" and "edges"
	// prefixes occurring before the enumeration of the labeled vertices and edges.
	//
	// TArg: G - Graph type
	template <typename G>
	struct Graph
	{
		// the graph
		G graph;
		// the label vector
		std::vector<std::string> labels;

		Graph() = default;

		template <typename G_, typename L_>
		Graph(G_&& g, L_&& l)
		: graph(std::forward<G_>(g)), labels(std::forward<L_>(l))
		{ }
	};
	template <typename CT, typename TT, typename G>
	std::basic_ostream<CT,TT>& operator<< (std::basic_ostream<CT,TT>& os, Graph<G> const& lg)
	{
		typedef typename g_common::VE<G>::Vertex Vertex;
		typedef std::array<Vertex,2> Edge;
		std::vector<Vertex> vertices;
		std::vector<Edge> edges;
		G const& graph (lg.graph);
		std::vector<std::string> const& labels (lg.labels);
		for (Vertex u = 0; u < bgl::num_vertices(graph); ++u)
			vertices.push_back(u);
		for (auto const& e : util::pair_to_range(bgl::edges(graph)))
			edges.push_back(std::array<Vertex,2>{{ bgl::source(e, graph), bgl::target(e, graph) }});
		os << std::make_tuple(c_print::printer(labelify_vertices(vertices, labels), "vertices"),
					c_print::printer(labelify_edges(edges, labels), "edges"));
		return os;

	}

}

#endif
