//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Non-templated code in graph.hh

#include <vector>
#include <string>
#include <sstream>
#include <map>

#include <d.hh>
#include <algo.hh>
#include <c-print.hh>
#include <graph.hh>

namespace {
	typedef std::vector<std::string> VecT;
	typedef VecT::difference_type DiffT;

}

std::vector<DiffT> graph::remap(VecT const& xold, VecT const& xnew)
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
	D_eval(D_trace, std::cerr << c_print::printer(remaps, "remaps") << '\n');
	D_eval(D_trace, std::cerr << c_print::printer(xdel, "new") << '\n');

	return out;
}
