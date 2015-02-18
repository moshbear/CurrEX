//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Static constants and non-templatables in labeled

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include <g-rategraph.hh>
#include <labeled.hh>

const std::vector<std::string> labeled::Node::_l0 {};

Labeled_graph* new_Labeled_graph()
{
        return new Labeled_graph;
}

void delete_Labeled_graph(Labeled_graph* lg)
{
        delete lg;
}

void Labeled_graph_filedump(Labeled_graph const& lg, std::string const& filename)
{
        std::ofstream out(filename.c_str());
        out << lg;
        out.close();
}

std::vector<std::string> Labeled_graph_labels(Labeled_graph const& lg)
{
        return (lg.labels);
}
