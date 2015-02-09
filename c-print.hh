//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Miscellaneous template utilities for containers.
//
// In namespace `c_print`:
//	printer<C,CT,T>(C const&[, (std::basic_string<CT,T> const&|CT const*)])
//		Capture C into a helper class to allow stream I/O of the form
//			(`pfx: ({c.size()}): ` << c)
//		where `pfx` is the optional second argument
//
#ifndef C_PRINT_HH
#define C_PRINT_HH

#include <cstddef>
#include <utility>
#include <string>
#include <ostream>
#include <functional>

#include <ext/prettyprint.hpp>

namespace c_print {

	// printer_impl<Container, Char_t, Traits>.
	// Proxy object for container printing.
	//
	// This proxies printing of `prefix (.size()): obj`, which reduces to 
	// (.size()): obj` when prefix is empty
	//
	// TArg: Container - Container type
	// TArg: Char_t - character type for output strings
	// TArg: Traits - char_traits type for output strings
	template <typename Container, typename Char_t = char, typename Traits = std::char_traits<Char_t>>
	struct printer_impl
	{
		using xstring = std::basic_string<Char_t, Traits>;
		using xostream = std::basic_ostream<Char_t, Traits>;
		using cp_type = printer_impl<Container, Char_t, Traits>;
		
		// Proxied object
		std::reference_wrapper<const Container> object;
		// Printing prefix
		xstring prefix;
	};

	// printer<Container, Char_t, Traits>(Container const& cp).
	// Wrap a printable container for printing.
	//
	// (TArg): Container - container type for printing
	// [TArg]: Char_t - character type for output; defaults to `char`
	// [Targ]: Traits - char_traits type for output; defaults to `std::char_traits<Char_t>`
	// Arg: Container const& cp - Container to print
	// Ret: printer_impl<Container, Char_t, Traits> proxying `cp`
	template <typename Container, typename Char_t = char, typename Traits = std::char_traits<Char_t>>
	printer_impl<Container, Char_t, Traits>
	printer(Container const& cp)
	{
		using Impl = printer_impl<Container, Char_t, Traits>;
		Impl ret { cp, typename Impl::xstring() };
		return ret;
	}

	// printer<Container, Char_t, Traits>(Container const& cp, std::basic_string<Char_t, Traits> const& pfx).
	// Wrap a printable container for printing, with prefix string.
	//
	// (TArg): Container - container type for printing
	// (TArg): Char_t - character type for output
	// (Targ): Traits - char_traits type for output
	// Arg: Container const& cp - Container to print
	// Arg: std::basic_string<Char_t, Traits> const& pfx - prefix string
	// Ret: printer_impl<Container, Char_t, Traits> proxying `cp` and `pfx`
	template <typename Container, typename Char_t, typename Traits>
	printer_impl<Container, Char_t, Traits>
	printer(Container const& cp, std::basic_string<Char_t, Traits> const& pfx)
	{
		printer_impl<Container, Char_t, Traits> ret { cp, pfx };
		return ret;
	}

	// printer<Container, Char_t, Traits>(Container const& cp, std::basic_string<Char_t, Traits> const& pfx).
	// Wrap a printable container for printing, with prefix string.
	//
	// (TArg): Container - container type for printing
	// (TArg): Char_t - character type for output
	// [Targ]: Traits - char_traits type for output; defaults to `std::char_traits<Char_t>`
	// Arg: Container const& cp - Container to print
	// Arg: Char_t const* pfx - null-terminated prefix string
	// Ret: printer_impl<Container, Char_t, Traits> proxying `cp` and `pfx`
	template <typename Container, typename Char_t = char, typename Traits = std::char_traits<Char_t>>
	printer_impl<Container, Char_t, Traits>
	printer(Container const& cp, Char_t const* pfx)
	{
		printer_impl<Container, Char_t, Traits> ret { cp, pfx };
		return ret;
	}

	// printer_delims_vals<Char_t>
	// Delimiter values structure for printer.
	// Each print has the form of:
	// 	(if prefix is empty): `pre_np.obj_size.`post`.obj
	// 	(if prefix is non-empty): prefix.`pre`.obj_size.`post`.obj
	//
	// TArg: Char_t - character type
	template <typename Char_t>
	struct printer_delims_vals
	{
		// pre-size delimiter in the presence of a prefix
		Char_t const* pre;
		// pre-size delimiter in the absence of a prefix
		Char_t const* pre_np;
		// post-size delimiter
		Char_t const* post;
		// appropriate newline (currently unused)
		Char_t const* newline;
	};
	// printer_delims<Char_t>
	// Delimiters for Char_t printer.
	// 
	// TArg: Char_t - character type
	//
	// Provided specializations:
	// 	<Char_t = char>
	// 	<Char_t = wchar_t>
	template <typename Char_t>
	struct printer_delims
	{
		static const printer_delims_vals<Char_t> values;
	};

	// The deferred printer.
	//
	// Arguments undocumented since it precisely models the ostream-output pattern.
	template <typename Container, typename Char_t, typename Traits>
	std::basic_ostream<Char_t, Traits>&
	operator<< (std::basic_ostream<Char_t, Traits>& out, printer_impl<Container, Char_t, Traits> const& cp)
	{
		auto const delims = printer_delims<Char_t>::values;
		auto const& obj = cp.object.get();
		out << cp.prefix << (cp.prefix.size() ? delims.pre : delims.pre_np)
		    << obj.size() << delims.post << obj;
		return out;
	}

}

	
#endif

