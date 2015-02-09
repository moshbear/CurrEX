//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Fetches the currently available list of instruments in vector form.
//
#ifndef INSTR_LIST_HH
#define INSTR_LIST_HH

#include <vector>
#include <string>

namespace instruments {

	// std::vector<std::string> list().
	// Returns a list of available 	instruments. There is no guarantee that the graph
	// representation is strongly cyclic, (that is, every vertex is part of a cycle).
	// Postprocessing with pruner strongly recommended.
	std::vector<std::string> list();

}

#endif
