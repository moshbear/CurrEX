//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// A pruner that eliminates acyclic vertices from the edge list defined by
// the input.
// The new edge list is returned as a vector.

#ifndef PRUNER_HH
#define PRUNER_HH

#include <vector>
#include <string>

// Prune a graph described by an edge list in input and return the
// pruned graph's edge list as output.
// Graph is assumed to be undirected.
std::vector<std::string> prune_vertices(std::vector<std::string> const& in);

#endif
