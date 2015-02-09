//          Copyright Andrey Moshbear 2014 - 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// <algorithm>-like utility functions for containers and iterators
//
#ifndef ALGO_HH
#define ALGO_HH

#include <cstddef>
#include <climits>
#include <algorithm>
#include <utility>
#include <iterator>
#include <vector>
#include <ostream>
#include <type_traits>
#include <functional>
#include <boost/range.hpp>
#include <boost/range/adaptor/reversed.hpp>

namespace algo {

	// constant returned by `index_of(c,v)` when it can't find `v` in `c`
	// TArg: C - container type needed to find appropriate C::difference_type
	template <typename C>
	struct npos
	{
		typedef typename C::difference_type type;
		static const type value = static_cast<type>(-1);
	};

	// index_of<C,T>(C const& c, T const& v).
	// Return the index of `v` in `c`; precisely, it is the distance between `find(c.begin(), c.end(), v)`
	// 	and `c.begin`, unless `v` isn't found, in which case `npos` is returned
	//
	// (TArg): C - container type
	// (TArg): T - value type
	// Arg: C const& c - container to query
	// Arg: T const& v - value to find index of
	// Ret: C::difference_type containing distance from c.begin(), or npos<C>::value (type-safe -1) if not found
	// Throw: std::out_of_range if `c` contains enough elements to warrant an overflow risk
	// Pre: C::iterator is a RandomAccessIterator
	// Pre: C is a container of T
	template <typename C, typename T>
	auto index_of(C const& c, T const& v)
	-> typename C::difference_type
	{
		auto begin = std::begin(c);
		auto end = std::end(c);

		if (c.size() > PTRDIFF_MAX)
			throw std::out_of_range("potential overflow in C::difference_type");

		auto ipos = std::find(begin, end, v);
		if (ipos == end) 
			return npos<C>::value;
		else
			return ipos - begin;
	}

	// where<C,UPred>(C& c, UPred u).
	// Return a vector of iterators referring to elements in `c` for which `u(*it)` holds true.
	//
	// (TArg): C - container type
	// (TArg): Unary_predicate - function type
	// Arg: C& c - container to query
	// Arg: Unary_predicate p - predicate function satisfying STL UnaryPredicate
	// Ret: std::vector<C::iterator> containing iterators for which p(*it) holds true 
	template <typename C, typename Unary_predicate>
	auto where(C& c, Unary_predicate p)
	-> std::vector<typename C::iterator>
	{
		typedef typename C::iterator Iter;
		std::vector<Iter> it;
		auto begin = std::begin(c);
		auto end = std::end(c);
		while (begin != end) {
			begin = std::find_if(begin, end, p);
			if (begin != end) {
				it.push_back(begin);
				++begin;
			}
		}
		return it;
	}
	
	// where<C,UPred>(C const& c, UPred u).
	// Return a vector of iterators referring to elements in `c` for which `u(*it)` holds true.
	// Const overload of above.
	//
	// (TArg): C - container type
	// (TArg): Unary_predicate - function type
	// Arg: C const& c - container to query
	// Arg: Unary_predicate p - predicate function satisfying STL UnaryPredicate
	// Ret: std::vector<C::const_iterator> containing iterators for which p(*it) holds true 
	template <typename C, typename Unary_predicate>
	auto where(C const& c, Unary_predicate p)
	-> std::vector<typename C::const_iterator>
	{
		typedef typename C::const_iterator Iter;
		std::vector<Iter> it;
		auto begin = std::begin(c);
		auto end = std::end(c);
		while (begin != end) {
			begin = std::find_if(begin, end, p);
			if (begin != end) {
				it.push_back(begin);
				++begin;
			}
		}
		return it;
	}
					
	// erase_if<C,P>(C& c, P p).
	// Erase elements from `C` `c` where predicate `p` holds; it is essentially a find-erase loop
	// 	which relaxes std::remove_if<C::iterator,P> for non-movable containers (e.g. std::set);
	// 	of course, this relaxation places a constraint that the container must
	// 	now be passed instead of the iterator pair.
	// Note: works in reverse order to make things like std::vector O(N);
	// 	 ordering has no effect on std::list or std::map, though.
	//
	// (TArg): C - container type
	// (TArg): Unary_predicate - function type
	// Arg: C& c - container to query
	// Arg: Unary_predicate p - predicate function satisfying STL UnaryPredicate
	template<typename C, typename Unary_predicate>
	void erase_if(C& c, Unary_predicate p)
	{
		// std::vector<T,A> is forward-fragile: erasure of iterators invalidates any following
		// Preceding is valid, though, so do reverse iteration
		auto to_erase = where(c, p);
		for (auto e : to_erase | boost::adaptors::reversed)
			c.erase(e);
	}
	
	// fragile_erase_if<C,P>(C& c, P p).
	// Variant for fragile containers, where erase() invalidates preceding *and* following iterators,
	// 	e.g. std::deque.
	// This version is O(N^2), but is necessarily slow; use erase_if when possibld
	//
	// (TArg): C - container type
	// (TArg): Unary_predicate - function type
	// Arg: C& c - container to query
	// Arg: Unary_predicate p - predicate function satisfying STL UnaryPredicate
	template<typename C, typename Unary_predicate>
	void fragile_erase_if(C& c, Unary_predicate p)
	{
		for (;;) {
			auto end = std::end(c);
			auto pos = std::find_if(std::begin(c), end, p);
			if (pos == end)
				break;
			c.erase(pos);
		}
	}

	// erase_at<C>(C& c, size_t n).
	// `c`.erase requires an iterator argument; this is a generalization that relaxes that constraint
	// 	by fetching `c`.begin(), incrementing it by `n`, and passing that to `c`.erase
	//
	// (TArg): C - container type
	// Arg: C& c - container to erase from
	// Arg: C::difference_type n - offset of element to erase
	// Ret: iterator returned from `c`.erase
	// Pre: C::iterator is a RandomAccessIterator
	template <typename C>
	auto erase_at(C& c, typename C::difference_type n)
	-> typename C::iterator
	{
		if (n < 0)
			throw std::invalid_argument("negative ptrdiff_t n");
		typename C::iterator i = c.begin();
		return c.erase(i + n);
	}

}

	
#endif

