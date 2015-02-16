//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Global debug level: structures, prototypes and macros.
//
#ifndef D_HH
#define D_HH

#include <string>
#include <iosfwd>

// Debug level enumeration.
enum D_level {
	// print nothing (errors which may break the algorithm greviously should printed);
	// output prefix is "XX"
	D_silent,
	// print errors (and the above);
	// output prefix is "EE"
	D_err,
	// print warnings (and the above);
	// output prefix is "WW"
	D_warn,
	// print informatives (and the above);
	// output prefix is "II"
	D_info,
	// print tracing information (and the above);
	// output prefix is "TT"
	D_trace,
};

// D_id_list.
// A refinement-based identifier list that permits for fuzzy matching of D_level by scope inheritance.
// Use D_push_id(ID) for instantiation to make chaining painless.
// Note: explicit construction without use of D_push_id is unsupported.
//
// To allow refinable scoping of D_level, we introduce the concept of an identifier list.
// The most recent identifier is up first, and `this->up` is called until the parent with
// `this->up == nullptr` is found to construct our chain.
// When D_get is called, the list is traversed to parent to get the scope identifiers and
// matching is done by recursive-best-match.
struct D_id_list {
	// scope string
	char const* id;
	// parent pointer
	D_id_list const* up;

	// default root node
	constexpr D_id_list()
	: id(""), up(nullptr)
	{ }
	// Construct a D_id_list.
	// D_push_id takes care of keeping `_up` valid.
	// Use of invalid values will cause UB.
	constexpr D_id_list(char const* _id, D_id_list const* _up)
	: id(_id), up(_up)
	{ }
};

// Root D_id_list.
// `static` for linking visibility reasons.
static D_id_list D_identifier;

// D_context.
// A coupling of a D_id_list with a D_level for use in D_* functions.
// Use D_add_context to transform a D_level into this.
struct D_context {
	D_id_list const* id;
	D_level level;
	// Default context: global scope, silent level
	constexpr D_context()
	: id(&::D_identifier), level(D_silent)
	{ }
	// General context: most-local scope, custom level
	//
	// Arg: _id - cref to D_identifier used
	// Arg: _lv - D_level used
	constexpr D_context(D_id_list const& _id, D_level const _lv)
	: id(&_id), level(_lv)
	{ }
};
// Dummy tag type used for invoking istream::operator>>(D_...&)
struct D_scan_tag { };

// Tag-based trigger to D_set_from_string for scanned argument
std::istream& operator>> (std::istream&, D_scan_tag&);
// Print a D_id_list to ostream
std::ostream& operator<< (std::ostream&, D_id_list const&);
// Print a D_level to ostream
std::ostream& operator<< (std::ostream&, D_level const);
// Print a D_context to ostream
std::ostream& operator<< (std::ostream&, D_context const&);

// void D_set(D_context const&).
// Set the debug level for some D_context.
//
// Arg: D_context providing qualified scope and level.
void D_set(D_context const&);
// D_level D_get(D_id_list const&)
// Get the current debug level for a specified D_id_list qualified scope.
//
// Arg: D_id_list providing scope.
// Ret: Associated D_level.
D_level D_get(D_id_list const&);
// bool D_ok(D_context const&).
// Query whether the passed context is appropriate.
//
// Arg: D_context const& d - passed context to check
// Ret: suitability of this context
//
// Inlined because one-liner.
inline bool D_ok(D_context const& d) {
	return static_cast<int>(d.level) <= static_cast<int>(D_get(*d.id));
}
// void D_xprint(D_context&, std::ostream& o, std::string const& s)
// Print message `s` to `o` with some specified D_context.
//
// Arg: D_context const& d - specified context
// Arg: std::ostream& o - output std::ostream
// Arg: std::string const& s - message to print
inline void D_xprint(D_context const& d, std::ostream& o, std::string const& s) {
	o << d << ": " << s << '\n';
}
// void D_xcprint(D_context&, std::ostream& o, std::string const s)
// Print message `s` to `o` with some specified D_context, copying `s`.
//
// Arg: D_context const& d - specified context
// Arg: std::ostream& o - output std::ostream
// Arg: std::string const s - message to copy then print
inline void D_xcprint(D_context const& d, std::ostream& o, std::string const s) {
	D_xprint(d, o, s);
}
// void D_set_from_string(std::string const &).
// Set D_level scope tree based on input string.
// Format:
// 	Let s_I be a scope token and T=S_1(:S_i)* be a scope list
//	Then T=D(,T=D)* denotes a list of D_levels D to set for scope-lists T
//	A missing T= implies global D.
//	In the event of duplicates, the last value holds.
//
// Arg: std::string const - formatted input string
void D_set_from_string(std::string const&);
// void D_set_from_args(int argc, char const* const* argv, char* const d_str)
// Scan [argv[0], ... argv[argc-1]] for d_str and forward to D_set_from_string.
//
// Arg: int argc - argument count (length of argv)
// Arg: char const* const* argv - argument vector (vector of strings)
// Arg: char const* d_str - option such that next element in argv is the string to parse
// Throw: std::invalid_argument if d_str is the last (non-null) element in argv
void D_set_from_args(int argc, char const* const* argv, char const* d_str);
// void D_set_file(std::string const&).
// Enable logging output to a file.
// If existing file is set, it is closed.
//
// Arg: std::string const& - file name
void D_set_file(std::string const&);
// void D_unset_file().
// Disable logging output to a file.
void D_unset_file();
// D_flag_* - Flags for setting extended params
// Flag type.
typedef unsigned D_flag_type;
// Set when in library mode - logging to console becomes disabled
static const D_flag_type D_flag_lib = 1<<0;
// Set when to throw instead of ignore attempts to log to console (i.e. no set file) when in lib mode
static const D_flag_type D_flag_lib_throw = 1<<1;
// Set when to throw if delaybit is set, and active ofp is file, and D_{un,}set_file is called
static const D_flag_type D_flag_ofp_throw = 1<<2;
// void D_set_xparam(D_flag_type)
// Set extended params as needed.
//
// D_flag_type - Parameter flagset
void D_set_xparam(D_flag_type);

// std::ostream* D_ofp()
// Get current output file stream ptr.
std::ostream* D_ofp();
// std::ostream* D_ofp_ignore()
// Get the specified ignore device.
std::ostream* D_ofp_ignore();
// void D_delay_ofp().
// void D_undelay_ofp().
//
// Mark or clear delaying of modification of output file stream ptr,
// eg if the value is cached in a function.
void D_delay_ofp();
void D_undelay_ofp();

// Delay proxy type. By placing delay and undelay into ctor/dtor,
// delay is now exception-safe.
struct D_delay {
	D_delay() {
		D_delay_ofp();
	}
	~D_delay() {
		D_undelay_ofp();
	}
};

// Macros.
// The following are defined for external code:
// 	D_eval(L, ...)    - conditionally evaluate <...> for D_level L and currently-scoped D_identifier
// 	D_print(L, o, s)  - conditionally print `s` to `o` for D_level L and currently-scoped D_identifier
// 	D_cprint(L, o, s) - as above, but a copy is made of the string argument for scoping reasons
// 	D_push_id(ID) - bring the most-local D_identifier with local part ID into scope
// 	D_add_context(D_l) - add an identifier context to D_l; uses most local D_identifier
// 	D_out - current output stream
// Defining NDEBUG converts these into empty statements.
// Anything in D_X_* is unsupported when used externally.
#define D_X_CATX(X,Y) X##Y
#define D_X_CAT(X,Y) D_X_CATX(X,Y)
#define D_X_CAT3(X,Y,Z) D_X_CAT(X,D_X_CATX(Y,Z))
#ifndef NDEBUG
#define D_eval(L, ...) if (D_ok(D_add_context(L))) do { __VA_ARGS__; } while(0)
#define D_print(L, o, s) if (D_ok(D_add_context(L))) do { D_xprint(D_add_context(L), o, s); } while(0)
#define D_cprint(L, o, s) if (D_ok(D_add_context(L))) do { D_xcprint(D_add_context(L), o, s); } while(0)
// Warning: this macro can't be quasi-hygeinic by means of do{...}while(0) as the { } will effect scope loss
#define D_push_id(ID) \
	static constexpr D_id_list const* D_X_CAT3(SAVE,ID,__LINE__) = &D_identifier; \
	static constexpr D_id_list D_identifier(#ID, D_X_CAT3(SAVE,ID,__LINE__))
#define D_add_context(D_level) D_context{ D_identifier, D_level }
#define D_out (*D_ofp())
#define D_DELAY D_delay D_X_CAT(DELAY,__LINE__)
#else
#define D_eval(...)
#define D_print(...)
#define D_cprint(...)
#define D_push_id(...)
#define D_add_context(...)
#define D_out (*D_ofp_ignore())
#define D_DELAY
#endif

#endif
