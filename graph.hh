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

#include <boost/tokenizer.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <d.hh>
#include <algo.hh>
#include <c-print.hh>
#include <util.hh>
#include <rates.hh>
#include <g-common.hh>
#include <g-rategraph.hh>
#include <labeled.hh>

namespace graph {
	D_push_id(graph);

	// Type-alias output of rate::get for load_graph_from_rates
	typedef std::vector<rates::Rate> Input_description;

	// Modified<G>
	// Hold the modifications that occurred when reloading a Graph with a new description.
	//
	// TArg: G - Graph type
	template <typename G>
	struct Modified
	{
		typedef typename g_common::VE<G>::Vertex Vertex;
		typedef std::array<Vertex,2> Edge;
		
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

	// Modified<G> load_graph_from_rates<G>(labeled::Graph<G>&, Input_description const&).
	// (Re)build a graph from a vector of rates.
	//
	// (TArg): G - Graph type
	// Arg: labeled::Graph<G>& lg - output/updated labeled graph
	// Arg: Input_description const& rates - list of rates describing the graph
	// Ret: modifications done to the graph, contained in a Modified<G>
	template <typename G>
	auto load_graph_from_rates(labeled::Graph<G>& lg, Input_description const& rates)
	-> Modified<G>
	{
		typedef typename g_common::VE<G>::Vertex Vertex;

		D_push_id(load_graph_from_rates);
		G& graph (lg.graph);
		std::vector<std::string>& labels (lg.labels);
		typedef std::array<Vertex,2> Edge;

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

		for (auto const& rate : rates) {
			boost::char_separator<char> sep("_");
			boost::tokenizer<decltype(sep)> names(rate.instrument, sep);
			std::vector<std::string> toks;
			for (auto const& t: names)
				toks.push_back(t);
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
			
			typedef typename g_common::VE<G> VE;
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

		D_print(D_info, std::cerr, deleted_vertices.size() > 0 ? "Removed vertices" : "No removed vertices");
		D_eval(D_trace,
			if (deleted_vertices.size())
				std::cerr << D_add_context(D_trace) << ": "
					  << c_print::printer(labeled::labelify_vertices(deleted_vertices, labels),
								"Deleted vertices")
					  << '\n');
		D_print(D_info, std::cerr, deleted_edges.size() > 0 ? "Removed edges" : "No removed edges");
		D_eval(D_trace,
			if (deleted_edges.size())
				std::cerr << D_add_context(D_trace) << ": "
					  << c_print::printer(labeled::labelify_edges(deleted_edges, labels),
								"Deleted edges")
					  << '\n');
		D_print(D_info, std::cerr, new_vertices.size() > 0 ? "Added vertices" : "No added vertices");
		D_eval(D_trace,
			if (new_vertices.size())
				std::cerr << D_add_context(D_trace) << ": "
					  << c_print::printer(labeled::labelify_vertices(new_vertices, labels),
								"<UNADJ> Added vertices")
					  << '\n');
		D_print(D_info, std::cerr, new_edges.size() > 0 ? "Added edges" : "No added edges");
		D_eval(D_trace,
			if (new_edges.size())
				std::cerr << D_add_context(D_trace) << ": "
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
			D_print(D_trace, std::cerr,
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
			D_print(D_trace, std::cerr,
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
				std::cerr << D_add_context(D_trace) << ": "
					  << c_print::printer(labeled::labelify_vertices(new_vertices, labels),
								"Corrected new vertices")
					  << '\n');
		D_eval(D_trace,
			if (new_edges.size())
				std::cerr << D_add_context(D_trace) << ": "
					  << c_print::printer(labeled::labelify_edges(new_edges, labels),
								"Corrected new edges")
					  << '\n');

		return Modified<G> {	std::move(deleted_vertices), std::move(deleted_edges),
					std::move(new_vertices), std::move(new_edges)
				   };			
	}

	// Rated_path<G> best_path<G>(labeled::Graph<G> const&, size_t max_iterations = -1)
	// Compute the best path, subject to an optional specified iteration limit.
	//	0th iteration searches for initial 3-cycle and successive iterations build iteratively from that.
	//
	// (TArg): G - Graph type
	// Arg: labeled::Graph<G> const& lg_in - input graph
	// Arg: size_t max_iterations - maximum iteration count
	// Ret: best path subject to the iteration count constraint
	template <typename G>
	auto best_path(labeled::Graph<G> const& lg_in, size_t max_iterations = static_cast<size_t>(-1))
	-> g_rategraph::Rated_path<G>
	{
		D_push_id(best_path);

		// G is a composition of the two; typedef the base classes to allow
		// this-shifting by static_cast<Base ...&>((Derived)) over (const) reference
		g_rategraph::Rated_path<G> rp_out;
		size_t c_iter = 0;
		rp_out = g_rategraph::find_initial_simplex(lg_in.graph);
		D_print(D_info, std::cerr, [&] {
			std::stringstream s;
			s << "Iteration " << c_iter;
			if (max_iterations != static_cast<size_t>(-1))
				s << " of " << max_iterations;
			s << ": path=[";
			auto const& p (rp_out.path);
			for (auto const& n : p) 
				s << lg_in.labels[n] << "->";
			s << lg_in.labels[p[0]] << "] lrate=";
			s << rp_out.lrate;
			return std::string(s.str());
		}());
		while (++c_iter < max_iterations)  {
			size_t last_size = rp_out.path.size();
			rp_out = g_rategraph::do_iteration(lg_in.graph, rp_out);
			D_print(D_info, std::cerr, [&] {
				std::stringstream s;
				s << "Iteration " << c_iter;
				if (max_iterations != static_cast<size_t>(-1))
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
				D_print(D_info, std::cerr, "Convergence");
				break;
			}
		}
		return rp_out;
	}

}

#endif
