//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Non-templated code in graph.hh

#include <vector>
#include <string>
#include <set>
#include <sstream>
#include <map>
#include <array>
#include <utility>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <d.hh>
#include <algo.hh>
#include <g-common.hh>
#include <g-rategraph.hh>
#include <rates.hh>
#include <labeled.hh>
#include <util.hh>
#include <c-print.hh>
#include <graph.hh>

namespace {
	typedef std::vector<std::string> VecT;
	typedef VecT::difference_type DiffT;

}

namespace graph {

std::vector<DiffT> remap(VecT const& xold, VecT const& xnew)
{
	D_push_id(remap);

	std::map<DiffT,DiffT> remaps;
	std::vector<DiffT> out;
	VecT xdel;

	DiffT oidx = 0;
	for (auto const& x : xold) {
		out.push_back(remaps[oidx] = algo::index_of(xnew, x));
		++oidx;
	}
	for (auto const& x : xnew)
		if (algo::index_of(xold, x) == algo::npos<VecT>::value)
			xdel.push_back(x);

	std::ostream& _d_cout = D_out;
	D_DELAY;
	D_eval(D_trace, D_out << c_print::printer(remaps, "remaps") << '\n');
	D_eval(D_trace, D_out << c_print::printer(xdel, "new") << '\n');

	return out;
}

Reload load_graph_from_rates(Labeled_graph& lg, std::vector<Rate> const& rates)
{
	using VE = g_common::VE<Graph>;
	typedef typename VE::Vertex Vertex;

	D_push_id(load_graph_from_rates);
	Graph& graph (lg.graph);
	std::vector<std::string>& labels (lg.labels);
	typedef std::vector<Vertex> Edge;

	// Copy the old vertices and edges.
	//
	// vertices is a std::set<> since it's crucial (*begin) < ... < *(end-1) holds
	// edges is a std::set<> since find-and-erase has O(|E|^2) cost if std::vector<>
	//
	// We fetch the values now, since g_rategraph::load_edge_pair may add edges (and vertices)
	std::set<Vertex> old_vertices;
	std::set<Edge> old_edges;

	for (Vertex u = 0; u < bgl::num_vertices(graph); ++u)
		old_vertices.insert(u);
	for (auto const& e : util::pair_to_range(bgl::edges(graph)))
		old_edges.insert({{{ bgl::source(e, graph), bgl::target(e, graph) }}});

	std::set<Vertex> visited_vertices;
	std::set<Edge> visited_edges;

	std::ostream& _d_cout = D_out;
	D_DELAY;
	for (auto const& rate : rates) {
		std::vector<std::string> toks;
		boost::split(toks, rate.instrument, [](char ch) { return ch == '_' ; });
		std::string u (toks[0]);
		std::string v (toks[1]);
		typename std::remove_reference<decltype(labels)>::type::difference_type upos, vpos, npos;
		npos = algo::npos<typename std::remove_reference<decltype(labels)>::type>::value;
		if ((upos = algo::index_of(labels, u)) == npos) {
			upos = util::checked_cast<decltype(upos)>(labels.size());
			labels.push_back(u);
		}
		if ((vpos = algo::index_of(labels, v)) == npos) {
			vpos = util::checked_cast<decltype(vpos)>(labels.size());
			labels.push_back(v);
		}
		
		g_rategraph::load_edge_pair(graph, VE::V(upos), VE::V(vpos), rate.ask, rate.bid);

		visited_vertices.insert(VE::V(upos));
		visited_vertices.insert(VE::V(vpos));
		visited_edges.insert({{{ VE::V(upos), VE::V(vpos) }}});
		visited_edges.insert({{{ VE::V(vpos), VE::V(upos) }}});
	}

	// New = Vis \ Old
	std::vector<Vertex> new_vertices;
	std::vector<Edge> new_edges;
	std::set_difference(visited_vertices.begin(), visited_vertices.end(),
			    old_vertices.begin(), old_vertices.end(),
			    std::back_inserter(new_vertices));
	std::set_difference(visited_edges.begin(), visited_edges.end(),
			    old_edges.begin(), old_edges.end(),
			    std::back_inserter(new_edges));
	// Del = Old \ Vis
	std::vector<Vertex> deleted_vertices;
	std::vector<Edge> deleted_edges;
	std::set_difference(old_vertices.begin(), old_vertices.end(),
			    visited_vertices.begin(), visited_vertices.end(),
			    std::back_inserter(deleted_vertices));
	std::set_difference(old_edges.begin(), old_edges.end(),
			    visited_edges.begin(), visited_edges.end(),
			    std::back_inserter(deleted_edges));
	
	old_vertices.clear();
	old_edges.clear();
	visited_vertices.clear();
	visited_edges.clear();

	D_print(D_info, _d_cout, deleted_vertices.size() > 0 ? "Removed vertices" : "No removed vertices");
	D_eval(D_trace,
		if (deleted_vertices.size())
			_d_cout << D_add_context(D_trace) << ": "
				  << c_print::printer(labeled::labelify_vertices(deleted_vertices, labels),
							"Deleted vertices")
				  << '\n');
	D_print(D_info, _d_cout, deleted_edges.size() > 0 ? "Removed edges" : "No removed edges");
	D_eval(D_trace,
		if (deleted_edges.size())
			_d_cout << D_add_context(D_trace) << ": "
				  << c_print::printer(labeled::labelify_edges(deleted_edges, labels),
							"Deleted edges")
				  << '\n');
	D_print(D_info, _d_cout, new_vertices.size() > 0 ? "Added vertices" : "No added vertices");
	D_eval(D_trace,
		if (new_vertices.size())
			_d_cout << D_add_context(D_trace) << ": "
				  << c_print::printer(labeled::labelify_vertices(new_vertices, labels),
							"<UNADJ> Added vertices")
				  << '\n');
	D_print(D_info, _d_cout, new_edges.size() > 0 ? "Added edges" : "No added edges");
	D_eval(D_trace,
		if (new_edges.size())
			_d_cout << D_add_context(D_trace) << ": "
				  << c_print::printer(labeled::labelify_edges(new_edges, labels),
							"<UNADJ> Added edges")
				  << '\n');

	for (auto const& del_e : deleted_edges)
		bgl::remove_edge(del_e[0], del_e[1], graph);

	// update indices of new_{vertices,edges} to preserve contiguity of indices
	// Note: we iterate in reverse direction to simplify adjustment
	for (auto const& del_v : deleted_vertices | boost::adaptors::reversed) {
		algo::erase_at(labels, util::checked_cast<typename std::remove_reference<decltype(labels)>::type::difference_type>
							(del_v));
		bgl::clear_vertex(del_v, graph);
		bgl::remove_vertex(del_v, graph);			
		// reindex new vertices
		auto where_v = algo::where(new_vertices,
					[&](Vertex const& u)
					{ return u > del_v; }
				);
		D_print(D_trace, D_out,
			[&]() { std::stringstream s;
				s << "Delete vertex " << del_v << ": adjust " << where_v.size() << " vertices";
				return std::string(s.str());
			}());
		for (auto& wv : where_v) 
			--*wv;
		// reindex new edges
		auto where_e = algo::where(new_edges,
					[&](Edge const& e)
					{ return (e[0] > del_v) || (e[1] > del_v); }
				);
		D_print(D_trace, D_out,
			[&]() { std::stringstream s;
				s << "Delete vertex " << del_v << ": adjust " << where_v.size() << " edges";
				return std::string(s.str());
			}());
		for (auto& we : where_e) {
			auto& ee = *we;
			if (ee[0] > del_v)
				--ee[0];
			if (ee[1] > del_v)
				--ee[1];
		}
	}
	D_eval(D_trace,
		if (new_vertices.size())
			_d_cout << D_add_context(D_trace) << ": "
				  << c_print::printer(labeled::labelify_vertices(new_vertices, labels),
						"Corrected new vertices")
				  << '\n');
	D_eval(D_trace,
		if (new_edges.size())
			_d_cout << D_add_context(D_trace) << ": "
				  << c_print::printer(labeled::labelify_edges(new_edges, labels),
							"Corrected new edges")
				  << '\n');

	return Reload {		std::move(deleted_vertices), std::move(deleted_edges),
				std::move(new_vertices), std::move(new_edges)
			};			
}

g_rategraph::Rated_path best_path(Labeled_graph const& lg_in, long max_iterations)
{
	D_push_id(best_path);

	// G is a composition of the two; typedef the base classes to allow
	// this-shifting by static_cast<Base ...&>((Derived)) over (const) reference
	g_rategraph::Rated_path rp_out;
	long c_iter = 0;
	std::ostream& _d_cout = D_out;
	D_DELAY;
	rp_out = g_rategraph::find_initial_simplex(lg_in.graph);
	D_print(D_info, _d_cout, [&] {
		std::stringstream s;
		s << "Iteration " << c_iter;
		if (max_iterations != -1)
			s << " of " << max_iterations;
		s << ": path=[";
		auto const& p (rp_out.path);
		for (auto const& n : p) 
			s << lg_in.labels[n] << "->";
		s << lg_in.labels[p[0]] << "] lrate=";
		s << rp_out.lrate;
		return std::string(s.str());
	}());
	while ((max_iterations < 0) || ++c_iter < max_iterations)  {
		auto last_size = rp_out.path.size();
		rp_out = g_rategraph::do_iteration(lg_in.graph, rp_out);
		D_print(D_info, _d_cout, [&] {
			std::stringstream s;
			s << "Iteration " << c_iter;
			if (util::checked_cast<long>(max_iterations) != -1)
				s << " of " << max_iterations;
			s << ": path=[";
			auto const& p (rp_out.path);
			for (auto const& n : p) 
				s << lg_in.labels[n] << "->";
			s << lg_in.labels[p[0]] << "] lrate=";
			s << rp_out.lrate;
			return std::string(s.str());
		}());
		// Convergence
		if (rp_out.path.size() == last_size) {
			D_print(D_info, _d_cout, "Convergence");
			break;
		}
	}
	rp_out.path = g_common::close_path<Graph>(rp_out.path);
	return rp_out;
}

}
