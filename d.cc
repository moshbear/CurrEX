//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Implementation detail for debug level states.

#include <mutex>
#include <deque>
#include <string>
#include <utility>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <type_traits>
#include <memory>
#include <cstring>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/tokenizer.hpp>


#include <d.hh>

namespace {

class Scope_node {
public:

	std::string const id;

	Scope_node ()
	: id("") {
		init_mutexes();
	}

	Scope_node (std::string const& _id)
	: id(_id), _d_level(D_silent) {
		init_mutexes();
	}

	Scope_node (std::string const& _id, D_level const d)
	: id(_id), _d_level(d) {
		init_mutexes();
	}

	// Getter/setters for d_level
	// We wrap accesses so specific value is thread-safe by rw-lock mutex.
	class _gs_dlevel {
		D_level* _d;
		mutable std::mutex* _d_mtx;
		_gs_dlevel() = delete;
	public:
		_gs_dlevel(D_level& d, std::mutex& d_mtx)
		: _d(&d), _d_mtx(&d_mtx)
		{ }
		operator D_level () const {
			std::lock_guard<std::mutex> lock(*_d_mtx);
			D_level d(*_d);
			return d;
		}
		void operator=(D_level const d) {
			std::lock_guard<std::mutex> lock(*_d_mtx);
			*_d = d;
		}
	};
	class _g_dlevel {
		D_level const* _d;
		mutable std::mutex* _d_mtx;
		_g_dlevel() = delete;
		void operator=(D_level) = delete;
	public:
		_g_dlevel (D_level const& d, std::mutex& d_mtx)
		: _d(&d), _d_mtx(&d_mtx)
		{ }
		operator D_level () const {
			std::lock_guard<std::mutex> lock(*_d_mtx);
			D_level d(*_d);
			return d;
		}
	};
	
	_gs_dlevel d_level () {
		return _gs_dlevel(_d_level, *_dlevel_mtx);
	}
	_g_dlevel d_level () const {
		return _g_dlevel(_d_level, *_dlevel_mtx);
	}

	Scope_node& get_child(char const* const child_id) const {
		std::lock_guard<std::mutex> lock(*_child_mtx);
		if (!_child)
			throw std::out_of_range("no children");
		return _child->at(std::string(child_id));
	}
	void add_child(char const* const child_id, D_level const dl) {
		std::lock_guard<std::mutex> lock(*_child_mtx);
		if (!_child)
			_child.reset(new typename std::remove_reference<decltype(*_child)>::type);
		_child->insert(std::make_pair(std::string(child_id), Scope_node(child_id, dl)));
	}
private:
	D_level _d_level;
	mutable std::shared_ptr<std::mutex> _dlevel_mtx;
	std::shared_ptr<std::unordered_map<std::string, Scope_node>> _child;
	mutable std::shared_ptr<std::mutex> _child_mtx;

	void init_mutexes() {
		_dlevel_mtx.reset(new std::mutex);
		_child_mtx.reset(new std::mutex);
	}
};

Scope_node root_scope;

/// D_level -> name lookup

// The degenerate case we don't have a match
const char* dn_unk = "??";

// Lookup table for stringizations of D_level.
// No need for hash or rb-table because small set.
constexpr std::pair<D_level, const char*> d_names[] = {
	{ D_silent, "XX" },
	{ D_err, "EE" },
	{ D_warn, "WW" },
	{ D_info, "II" },
	{ D_trace, "TT" }
};

char const* d_name(D_level d) {
	for (auto const& n : d_names)
		if (n.first == d)
			return n.second;
	return dn_unk;
}

// Simple D_level parser
D_level dl_from_string(std::string const& str) {
	D_level d;
	switch (tolower(str[0])) {
	case 'x': d = D_silent; break;
	case 'e': d = D_err; break;
	case 'w': d = D_warn; break;
	case 'i': d = D_info; break;
	case 't': d = D_trace; break;
	default: throw std::invalid_argument(str.substr(0,1) + " (expected one of [xewit])");
	}
	return d;
}
	

// ID expansion and parsing
//
char const* id_sep = ":";
char const* param_sep = ",";
char const* kv_sep = "=";

std::deque<char const *> reverse_view(D_id_list const& idl) {
	std::deque<char const *> ids;
	D_id_list const* cur_id = &idl;
	while (cur_id != nullptr) {
		ids.push_front(cur_id->id);
		auto old_cur = cur_id;
		cur_id = cur_id->up;
		if (cur_id == old_cur)
			throw std::logic_error("Infinite loop detected; is something wrong"
						" with symbol shadowing resolution?");
	}
	return ids;
}

// Expand the supplied D_id_list into a qualified id-string in the form of
// scope_1:...:scope_N
// TODO: possible caching
std::string expand_id(D_id_list const& idl) {
	auto ids = reverse_view(idl);
	std::string expand;
	// this happens iff `&idl == nullptr`
	if (ids.empty())
		throw std::invalid_argument("Invalid data in idl");
	// invalidly-terminated chain
	if ((**ids.begin()) != '\0')
		throw std::invalid_argument("Root scope needs to be nameless");
	for (auto const& id : ids) {
		if (id == nullptr)
			throw std::invalid_argument("Invalid data in idl");
		if (*id == '\0')
			continue;
		expand.append(id);
		expand.append(id_sep);
	}
	return expand.substr(0, expand.size() - strlen(id_sep));
}

// Refactor of internals of D_set.
// Templated so we can pass generic suitable containers instead of relying on
// std::vector or std::deque.
template <template <typename...> class C>
void do_set(C<char const *> const& ids, char const* tailp, D_level d) {
	Scope_node* scope = &root_scope;
	for (auto const& id : ids) {
		try {
			Scope_node* new_scope = &scope->get_child(id);
			scope = new_scope;
			// don't pollute tree: set only leaf level
			if (id == tailp)
				scope->d_level() = d;
		} catch (std::out_of_range&) {
			scope->add_child(id, d);
			scope = &scope->get_child(id);
		}
	}

}

// Streambuf emulating /dev/null.
template <typename CT, typename TT = std::char_traits<CT>>
class devnull_streambuf : public std::basic_streambuf<CT, TT> {
protected:
	typename std::basic_streambuf<CT,TT>::int_type
	overflow(typename std::basic_streambuf<CT,TT>::int_type c = TT::eof());
};

template <typename CT, typename TT>
typename std::basic_streambuf<CT,TT>::int_type
devnull_streambuf<CT,TT>::overflow(typename std::basic_streambuf<CT,TT>::int_type)
{
	this->setp(this->pbase(), this->epptr());
	return TT::to_int_type(0);
}

std::unique_ptr<std::ofstream> f_ptr;
std::unique_ptr<devnull_streambuf<char>> ign_sb;
std::unique_ptr<std::ostream> ign_fp;
std::ostream* cur_ofp { nullptr };
D_flag_type flags { D_flag_type() };
int ofp_delaybit { 0 };

inline bool ofp_is_file() {
	return cur_ofp == f_ptr.get();
}
}

void D_set(D_context const& d) {
	do_set(reverse_view(*d.id), d.id->id, d.level);
}

D_level D_get(D_id_list const& idx) {
	auto ids = reverse_view(idx);
	D_level d (D_silent);
	Scope_node const* scope = &root_scope;
	for (auto const& id : ids) {
		try {
			Scope_node const* new_scope = &scope->get_child(id);
			scope = new_scope;
			d = static_cast<D_level>(std::max(static_cast<int>(d), static_cast<int>(scope->d_level())));
			// Early return due to value saturation : we want the max along the path and got it early
			if (d == D_trace)
				return d;
		} catch (std::out_of_range&) {
			break;
		}
	}
	return d;
}

void D_set_from_string(std::string const& str) {
	D_set(D_add_context(D_silent));
	boost::char_separator<char> p_sep(param_sep);
	boost::tokenizer<decltype(p_sep)> params(str, p_sep);
	for (auto const& param : params) {
		std::vector<std::string> scopes;
		std::string::size_type kv_pos;
		scopes.push_back("");
		if ((kv_pos = param.find(kv_sep)) != std::string::npos) {
			boost::char_separator<char> i_sep(id_sep);
			// scope of key part needs to be pulled up so iterators used by boost::tokenizer are valid
			auto key = param.substr(0, kv_pos);
			boost::tokenizer<decltype(i_sep)> ids(key, i_sep);
			for (auto const& id : ids)
				scopes.push_back(std::string(id));
		}
		std::vector<char const*> sp;
		for (auto const& s : scopes)
			sp.push_back(s.c_str());
		do_set(sp, sp.back(), dl_from_string(param.substr(kv_pos + 1)));
	}
}

void D_set_from_args(int argc, char const* const* argv, char const* d_str) {
	if (argc < 1) {
		D_set(D_add_context(D_silent));
		return;
	}
attempt:
	if (!strncmp(argv[0], d_str, strlen(d_str))) {
		if (argc < 2)
			throw std::invalid_argument("Missing -d arg");
		D_set_from_string(argv[1]);
	} else if (argc > 1) {
		++argv;
		--argc;
		goto attempt;
	}
}

std::istream& operator>> (std::istream& in, D_scan_tag&) {
	std::string token;
	in >> token;
	D_set_from_string(token);
	return in;
}

std::ostream& operator<< (std::ostream& out, D_id_list const& id) {
	return out << expand_id(id);
}
std::ostream& operator<< (std::ostream& out, D_level const lv) {
	return out << '(' << d_name(lv) << ')';
}
std::ostream& operator<< (std::ostream& out, D_context const& d) {
	return out << d.level << ' ' << *d.id;
}

void D_set_file(std::string const& f) {
	if ((ofp_delaybit > 0) && ofp_is_file() && (flags & D_flag_ofp_throw))
		throw std::invalid_argument("File change while delay bit set and file is current ofp");
	if (f_ptr)
		f_ptr->close();
	f_ptr.reset(new std::ofstream(f));
}

void D_unset_file() {
	if ((ofp_delaybit > 0) && ofp_is_file() && (flags & D_flag_ofp_throw))
		throw std::invalid_argument("File change while delay bit set and file is current ofp");
	if (f_ptr)
		f_ptr->close();
	f_ptr.reset(nullptr);
}

void D_set_xparam(D_flag_type f) {
	flags = f;
}

std::ostream* D_ofp() {
	if (ofp_delaybit > 0)
		return cur_ofp;
	std::ostream* next_fp (nullptr);
	if (flags & D_flag_lib) {
		if (f_ptr) {
			next_fp = f_ptr.get();
		} else {
			if (flags & D_flag_lib_throw)
				throw std::invalid_argument("Lib mode and missing file");
			else
				next_fp = D_ofp_ignore();
		}
	} else {
		if (f_ptr)
			next_fp = f_ptr.get();
		else
			next_fp = &std::cerr;
	}

	return (cur_ofp = next_fp);
}

std::ostream* D_ofp_ignore() {
	if (!ign_fp) {
		ign_sb.reset(new devnull_streambuf<char>);
		ign_fp.reset(new std::ostream(ign_sb.get()));
	}
	return ign_fp.get();
}


void D_delay_ofp() {
	++ofp_delaybit;
}

void D_undelay_ofp() {
	--ofp_delaybit;
}

