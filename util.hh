//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// <utility>-like utility functions for containers and iterators
//
#ifndef UTIL_HH
#define UTIL_HH

#include <limits>
#include <stdexcept>
#include <utility>
#include <boost/range.hpp>

namespace util {

	// boost::iterator_range<Iter> pair_to_range<Iter>(std::pair<Iter, Iter> const& p).
	// Convert an iterator pair into a range suitable for range-based for-loops.
	// This is required for, among other things, iteration over bgl vertices and edges.
	//
	// (TArg): Iter - Iterator type
	// Arg: p - Iterator pair to convert into a range
	// Ret: An iterator range such that p.first == Ret.begin() and p.second == Ret.end()
	template <typename Iter>
	auto pair_to_range(std::pair<Iter, Iter> const& p)
	-> boost::iterator_range<Iter>
	{ return boost::make_iterator_range(p.first, p.second); }

	// checked_cast<T>(U).
	// Check that a value of type U fits into type T without overflow or underflow.
	// Throws std::out_of_range if the condition doesn't hold.
	// Retuns static_cast<T>(std::forward<U>(...)) otherwise.
	//
	// Pretty much the only place in the code with compiler warnings due to sign mixing left.

	// SFINAE helper for checked_cast
	template <typename T, typename U>
	class cc_detail
	{
	private:
		static constexpr const auto min = std::numeric_limits<T>::min();
		static constexpr const auto max = std::numeric_limits<T>::max();
	public:
		static bool ok_lo(U const&);
		static bool ok_lo(U const&, std::true_type); // overload for unsigned U
		static bool ok_lo(U const&, std::false_type);
		static bool ok_hi(U const&);
		static bool ok_hi(U const&, std::true_type); // overload for integral T,U
		static bool ok_hi(U const&, std::false_type);
	};
	template <typename T, typename U>
	bool cc_detail<T,U>::ok_lo(U const& u)
	{	return cc_detail<T,U>::ok_lo(u, std::is_unsigned<typename std::remove_reference<U>::type>{});	}
	template <typename T, typename U>
	bool cc_detail<T,U>::ok_lo(U const&, std::true_type)
	{	return true;	};
	template <typename T, typename U>
	bool cc_detail<T,U>::ok_lo(U const& u, std::false_type)
	{	return !(u < cc_detail<T,U>::min);	}
	template <typename T, typename U>
	bool cc_detail<T,U>::ok_hi(U const& u)
	{
		return cc_detail<T,U>::ok_hi(u, std::integral_constant<bool,
								std::is_integral<typename std::remove_reference<T>::type>::value
								&& std::is_integral<typename std::remove_reference<U>::type>::value>{});
	}
	template <typename T, typename U>
	bool cc_detail<T,U>::ok_hi(U const& u, std::true_type)
	{
		using Common = typename std::common_type<T,U>::type;
		return !(static_cast<Common>(u) > static_cast<Common>(cc_detail<T,U>::max));
	}
	template <typename T, typename U>
	bool cc_detail<T,U>::ok_hi(U const& u, std::false_type)
	{	return !(u > cc_detail<T,U>::max);	}
	template <typename T, typename U>
	auto checked_cast(U&& u) -> decltype(static_cast<T>(std::forward<U>(u)))
	{
		using detail = cc_detail<T,U>;
		if (detail::ok_lo(u) && detail::ok_hi(u))
			return static_cast<T>(std::forward<U>(u));
		throw std::out_of_range("checked_cast");
	}
}


#endif

