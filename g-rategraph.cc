//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Rate graph namespaces and functions
//

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
#include <g-rategraph.hh>

namespace {
	
	// double evaluate_path(Graph const&, Path const&).
	// Evaluate the rate along an open path in a graph.
	//
	// Arg: Graph const& g - input rate graph
	// Arg: Path const& path - open path
	// Ret: sum of [log-]rates

	double evaluate_path(g_rategraph::Graph const& g, g_rategraph::Rated_path::path_type const& path)
	{
		auto path_len (0);
		using Const_iterator = decltype(path.cbegin());
		auto path_begin = path.cbegin();
		auto path_end = path.cend();
		auto next = [path_begin, path_end, &path_len] (Const_iterator ci) {
			std::advance(ci, 1);
			++path_len;
			return ci != path_end ? ci : path_begin;
		};
		double acc (0);
		std::ostream& _d_cout = D_out;
		D_DELAY;
		// can't do range-based for since we need access to both the current iterate and its successor
		for (auto p = path_begin; p != path_end; ++p) {
			auto u (*p);
			auto v (*next(p));
			auto edge (bgl::edge(u, v, g));
			if (!edge.second) {
				std::stringstream emsg;
				emsg << "invalid edge (" << u << "," << v << ") in path";
				D_print(D_err, _d_cout, emsg.str());
				throw std::invalid_argument("g_rategraph::evaluate_path: see D log");
			}
			auto rate_info (bgl::get(g_rategraph::Rate_tag(), g, edge.first));
			acc += rate_info.rate;
		}
		if (path_len < 2)
			throw std::invalid_argument("insufficient vertex count spanned by path");
		return acc;
	}

	// Rated_path try_expand(Graph const&, Path const&)
	// Walk the path defined by the iterator pair and see if a better rate is
	// 	possible by splitting edges by connecting the ends to a new vertex.
	//	Vertex coloring used to avoid re-evaluating accepted additions.
	//
	// Arg: Graph const& g - Input rate graph
	// Arg: Path const& path - open path
	// Ret: Rated_path containing best one-iteration expansion of input path
	g_rategraph::Rated_path try_expand(g_rategraph::Graph const& g, g_rategraph::Rated_path::path_type const& path)
	{
		using Const_iterator = decltype(path.cbegin());
		auto path_begin = path.cbegin();
		auto path_end = path.cend();
		auto next = [path_begin, path_end] (Const_iterator ci) {
			std::advance(ci, 1);
			return ci != path_end ? ci : path_begin;
		};
		std::vector<bgl::default_color_type> color_vec(bgl::num_vertices(g));
		bgl::default_color_type c = boost::white_color; 
		auto vertex_colors = bgl::make_iterator_property_map
						(color_vec.begin(), bgl::get(bgl::vertex_index, g), c);
		typedef bgl::color_traits<decltype(c)> Color;
		for (auto const& p : path)
			put(vertex_colors, p, Color::black());
		typename g_common::Path<g_rategraph::Graph>::type vertices;
		double new_rate (0);
		std::ostream& _d_cout = D_out;
		D_DELAY;
		for (Const_iterator p = path_begin; p != path_end; ++p) {
			auto u(*p);
			auto v(*next(p));
			auto rate (bgl::get(g_rategraph::Rate_tag(), g, bgl::edge(u, v, g).first).rate);
			typedef bgl::color_traits<decltype(c)> Color;
			vertices.push_back(u);
			// Choose best unvisited `w`
			// Initialize to bgl::num_vertices so that, if we don't find new vertices, the
			// edge checker code returns false and loops aren't added.
			decltype(u) candidate (bgl::num_vertices(g));
			double c_rate (0);
			D_print(D_trace, _d_cout, 
				[&] { std::stringstream s;
					s << "Existing: [" << u << "->" << v << "] = " << rate;
					return std::string(s.str());
				}());
			for (auto const& w : g_color::intersecting_vertices(g, u, v, vertex_colors, Color::black())) {
				auto xrate (bgl::get(g_rategraph::Rate_tag(), g, bgl::edge(u, w, g).first).rate
						+ bgl::get(g_rategraph::Rate_tag(), g, bgl::edge(w, v, g).first).rate);
				D_print(D_trace, _d_cout,
					[&] { std::stringstream s;
						s << "Evaluating [" << u << "->" << w << "->" << v <<  "]: d = " << (xrate - c_rate);
						return std::string(s.str());
					}());
				if (xrate < c_rate) {
					c_rate = xrate;
					candidate = w;
				}

			}
			if (c_rate < rate && bgl::edge(u, candidate, g).second && bgl::edge(candidate, v, g).second) {
				new_rate += c_rate;
				vertices.push_back(candidate);
				bgl::put(vertex_colors, candidate, Color::black());
				D_print(D_info, _d_cout,
					[&] { std::stringstream s;
						s << "growth: adding node " << candidate
						  << " raised profits by " << -1 * (c_rate - rate);
						return std::string(s.str());
					}());
			} else

				new_rate += rate;
		}
		return g_rategraph::Rated_path(std::move(vertices), new_rate);
	}



}

namespace g_rategraph {

Rated_path::path_type Rated_path::get_path() const
{
	return this->path;
}

void load_edge_pair(Graph& g, VE::Vertex u, VE::Vertex v, double ask_rate, double bid_rate)
{
	// Safety check. bgl::edge(u, v, g) returns
	// std::pair<>(g.__edge_iterator(u, v), g.__vertex[u].is_adjacent(v))
	// Unsafe u or v -> UB in decltype(G::__vertex)::operator[] (Vertex)
	if (bgl::num_vertices(g) <= std::max(u,v)) {
new_edge:
		Edge_property p;
		p.rate = -1 * log(ask_rate);
		bgl::add_edge(u, v, p, g);
		p.rate = log(bid_rate);
		bgl::add_edge(v, u, p, g);
		return;
	}

	auto uv = bgl::edge(u, v, g);
	auto vu = bgl::edge(v, u, g);

	// Existing unidirectional edge
	if (uv.second ^ vu.second) {
		std::stringstream s;
		s << "Edge between " << u << " and " << v << "exists in only one direction";
		D_print(D_err, D_out, s.str());
		throw std::invalid_argument("g_rategraph::load_edge_pair: see D log");
	}
	// Existing edge: set new rates
	if (uv.second) {
		Edge_property p;
		p.rate = -1 * log(ask_rate);
		bgl::put(Rate_tag(), g, uv.first, p);
		p.rate = log(bid_rate);
		bgl::put(Rate_tag(), g, vu.first, p);
	} else {
		goto new_edge;
	}
}


Rated_path find_initial_simplex(Graph const& g)
{
	std::vector<bgl::default_color_type> color_vec(bgl::num_vertices(g));
	typedef bgl::color_traits<bgl::default_color_type> Color;
	bgl::default_color_type c = Color::white();
	auto vertex_colors = bgl::make_iterator_property_map
					(color_vec.begin(),
					 bgl::get(bgl::vertex_index, g), c);

	
	Rated_path::path_type best_simplex;
	Rated_path::lrate_type best (0);

	std::vector<Rated_path> candidates;

	// Guide to vertex colors:
	// black = visited primary; do not treat as neighboring vertex to avoid recalculation
	// white = unvisited
	auto vertex_range = util::pair_to_range(bgl::vertices(g));

	for (auto u : vertex_range) {
		typedef bgl::color_traits<decltype(c)> Color;
		bgl::put(vertex_colors, u, Color::black());
		auto neighbors =  g_color::unvisited_neighbors(g, u, vertex_colors, Color::black());
		for (auto const& v : neighbors) {
			bgl::put(vertex_colors, v, Color::gray());
			auto vc = bgl::get(vertex_colors, v);
			if (vc == Color::white())
				bgl::put(vertex_colors, v, Color::gray());
			auto triangles = g_color::intersecting_vertices(g, u, v, vertex_colors, Color::white(), false);
			for (auto const& w : triangles) {
				// evaluate u->v->w->u
				decltype(best_simplex) cur_simplex { u, v, w };
				auto rate (evaluate_path(g, cur_simplex));
				D_eval(D_trace, if (rate < 0) candidates.push_back(Rated_path(g_common::close_path<Graph>(cur_simplex), rate)));
				if (rate < best) {
					best = rate;
					best_simplex = cur_simplex;
				} 
				// evaluate u->w->v->u
				cur_simplex = { u, w, v };
				rate = evaluate_path(g, cur_simplex);
				D_eval(D_trace, if (rate < 0) candidates.push_back(Rated_path(g_common::close_path<Graph>(cur_simplex), rate)));
				if (rate < best) {
					best = rate;
					best_simplex = cur_simplex;
				}
			}
		}

	}
	D_eval(D_trace, D_DELAY; D_out << D_add_context(D_trace) << ": " << c_print::printer(candidates) << '\n');
	return Rated_path(best_simplex, best);
}

Rated_path do_iteration(Graph const& g, Rated_path const& iter)
{
	return try_expand(g, iter.path);
}

}
