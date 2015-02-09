//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// eval.cc - REPL
#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <bitset>
#include <stdexcept>
#include <array>
#include <functional>
#include <mutex>

#include <d.hh>
#include <instr-ls.hh>
#include <pruner.hh>
#include <rates.hh>
#include <labeled.hh>
#include <g-rategraph.hh>
#include <graph.hh>
#include <c-print.hh>

namespace {
	// inter-pipeline variables
	std::vector<std::string> instrument_list;
	std::vector<std::string> pruned_instruments;
	std::vector<rates::Rate> rate_list;
	labeled::Graph<g_rategraph::Graph> labeled_graph;
	g_rategraph::Rated_path<g_rategraph::Graph> best_path;

	// checkpointing of pipeline flow
	enum class IS_SET : unsigned { instr, pruned, rates, graph, best_path, _last };
	std::bitset<static_cast<unsigned>(IS_SET::_last)> is_set;

	// globals for REPL, including getvar
	std::map<std::string, std::function<void()>> command_handler;
	std::map<std::string, std::function<void()>> getvar_handler;
	std::once_flag getvar_init;
	std::string out;

	// checkpointing functions
	void need(IS_SET i, std::string const& msg)
	{
		if (!is_set[static_cast<unsigned>(i)])
			throw std::invalid_argument("need " + msg);
	}
	void provide(IS_SET i)
	{
		is_set[static_cast<unsigned>(i)] = true;
	}
	bool check(IS_SET i)
	{
		return is_set[static_cast<unsigned>(i)];
	}

	// I/O utilities
	
	// Set output from scalar.
	template <typename T>
	void set_output(T const& val)
	{
		std::stringstream s;
		s << val;
		out = s.str();
	}
	// Set output from vector, with optionally specified fill char
	template <typename C>
	void set_output_V(C const& container, char fill_c = ' ')
	{
		std::stringstream s;
		for (auto const& elem : container)
			s << elem << fill_c;
		out = s.str().substr(0, s.str().size() - 1);
	}

	// Token scanner.
	// Recursively variadic case
	template <typename IStream, typename T, typename... Ts>
	ptrdiff_t tokenization_helper(IStream& is, T& t, Ts&... ts)
	{

		if (!(is >> t))
			return -1;
		// -1 * { recursion depth at failure } or 0
		ptrdiff_t fail_idx = tokenization_helper(is, ts...);
		return (fail_idx != 0 ? fail_idx - 1 : 0);
	}
	// Vector greedy-line consumption case
	// Undefined behavior if further values remain in input stream.
	template <typename IStream, typename T>
	ptrdiff_t tokenization_helper(IStream& is, std::vector<T>& tv)
	{
		T t;
		while (is >> t)
			tv.push_back(t);
		return 0;
	}
	// Base case
	template <typename IStream, typename T>
	ptrdiff_t tokenization_helper(IStream& is, T& t)
	{
		return (is >> t) ? 0 : -1;
	}

	// Tokenize a string based on types of arguments passed by lvalue reference
	template <typename... String_spec, typename... Ts>
	ptrdiff_t tokenize_line(std::basic_string<String_spec...> const& str, Ts&... ts)
	{
		std::basic_stringstream<String_spec...> ss;
		ss.str(str);
		
		return tokenization_helper(ss, ts...);
	}

	// Scan a line from an istream and return it.
	template <typename C, typename T, typename A>
	std::basic_string<C,T,A> read_line(std::basic_string<C,T,A>, std::basic_istream<C,T>& in)
	{
		std::basic_string<C,T,A> str;
		std::getline(in, str);
		return str;
	}
	// Scan a line from an istream to prepare it for the next command,
	// discarding the output.
	template<typename C, typename T>
	void discard_line(std::basic_istream<C,T>& is)
	{
		std::basic_string<C,T> str;
		std::getline(is, str);
	}

	// Commands.

	void set_dlevel()
	{
		auto str = read_line(std::string(), std::cin);
		D_scan_tag tag;
		if (tokenize_line(str, tag) < 0)
			throw std::invalid_argument("tokenization error");
	}
	void update_instruments()
	{
		discard_line(std::cin);
		instrument_list = std::move(instruments::list());
		provide(IS_SET::instr);
	}
	void update_pruned()
	{
		discard_line(std::cin);
		need(IS_SET::instr, "instruments");
		pruned_instruments = std::move(pruner(instrument_list));
		provide(IS_SET::pruned);
	}
	void update_rates() 
	{
		discard_line(std::cin);
		need(IS_SET::pruned, "pruned");
		rate_list = std::move(rates::get(pruned_instruments));
		provide(IS_SET::rates);
	}
	void load_graph()
	{
		discard_line(std::cin);
		need(IS_SET::rates, "rates");
		auto M_ = graph::load_graph_from_rates(labeled_graph, rate_list);
		provide(IS_SET::graph);
	}
	void search_graph()
	{
		need(IS_SET::graph, "graph");
		long ilim;
		auto line = read_line(std::string(), std::cin);
		if (tokenize_line(line, ilim) < 0)
			ilim = -1;
		best_path = graph::best_path(labeled_graph, static_cast<size_t>(ilim));
		provide(IS_SET::best_path);
	}
	void eval_rates()
	{
		need(IS_SET::best_path, "path");

		double lrate = best_path.lrate;
		auto line = read_line(std::string(), std::cin);
		std::vector<double> IV;
		if (tokenize_line(line, IV) < 0)
			throw std::invalid_argument("tokenization error");
		std::vector<std::array<typename decltype(IV)::value_type,2>> OV;
		for (auto const& val : IV) {
			auto lv = log(val);
			auto revenue = exp(lv - lrate);
			auto profit = revenue - val;
			OV.push_back({{ revenue, profit }});
		}
		set_output_V(OV);
	}	
	void init_getvar_handlers();
	void get_var()
	{
		std::call_once(getvar_init, init_getvar_handlers);

		auto line = read_line(std::string(), std::cin);
		std::string cmd;
		if (tokenize_line(line, cmd) < 0)
			throw std::invalid_argument("tokenization error");
		auto cmdlet = getvar_handler.find(cmd);
		if (cmdlet == getvar_handler.end()) {
	bad_cmd:
			throw std::invalid_argument("bad var: " + cmd);
		}
		auto f = cmdlet->second;
		if (!f)
			goto bad_cmd;
	}

	// Command initialization.

	void init_getvar_handlers()
	{
		getvar_handler["instr"] = [] { set_output(instrument_list); };
		getvar_handler["pruned"] = [] { set_output(pruned_instruments); };
		getvar_handler["ratelist"] = [] { set_output(rate_list); };
		getvar_handler["graph"] = [] { set_output(labeled_graph); };
		getvar_handler["path"] = [] { set_output(best_path.path); };
		getvar_handler["lrate"] = [] { set_output(best_path.lrate); };
		// I_ is for internals
		getvar_handler["I_isset"] = [] {
						std::vector<char> OV;
						OV.push_back(check(IS_SET::instr) ? 'I' : '-');
						OV.push_back(check(IS_SET::pruned) ? 'P' : '-');
						OV.push_back(check(IS_SET::rates) ? 'R' : '-');
						OV.push_back(check(IS_SET::graph) ? 'G' : '-');
						OV.push_back(check(IS_SET::best_path) ? 'X' : '-');
						set_output_V(OV);
					};
	}
	void init_command_handlers()
	{
		command_handler["setd"] = set_dlevel;
		command_handler["instr"] = update_instruments;
		command_handler["prune"] = update_pruned;
		command_handler["rates"] = update_rates;
		command_handler["gload"] = load_graph;
		command_handler["gsearch"] = search_graph;
		command_handler["eval"] = eval_rates;
		command_handler["getvar"] = get_var;
	}
}

int main(int argc, char** argv) {
	D_set_from_args(argc - 1, argv + 1, "-d");
	init_command_handlers();
	std::string cmd;
	while (std::cin >> cmd) {
		out.clear();
		try {
			auto cmdlet = command_handler.find(cmd);
			if (cmdlet == command_handler.end()) {
bad_cmd:			throw std::invalid_argument("bad cmd: " + cmd);
			}
			auto f = cmdlet->second;
			if (!f)
				goto bad_cmd;
			if (out.size() > 0)
				std::cout << out << std::endl;
		} catch (std::invalid_argument const& ia) {
			std::cout << "Argument error: " << ia.what() << std::endl;
		}
	}
}
