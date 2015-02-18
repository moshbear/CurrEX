//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Container for structured rates.
//
#ifndef RATES_HH
#define RATES_HH

#include <vector>
#include <string>
#include <tuple>
#include <ostream>
#include <ext/prettyprint.hpp>

struct Rate {
	// instrument name
	std::string instrument;
	// selling rate
	double bid;
	// asking rate
	double ask;
	Rate(std::string const& instr, double b, double a)
	: instrument(instr), bid(b), ask(a)
	{ }
	// comparator
	bool operator< (Rate const& r) {
		return (instrument < r.instrument);
	}
};

template <typename CT, typename TT>
std::basic_ostream<CT,TT>& operator<< (std::basic_ostream<CT,TT>& os, Rate const& r) {
	os << std::tie(r.instrument, r.bid, r.ask);
	return os;
}


#endif
