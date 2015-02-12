//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Mock test for HTTP bits.

#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <array>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string/join.hpp>

#include <http.hh>

namespace {
	// Refactorable into separate .hh file
	template <typename Throwable>
	void throw_if(bool condition, Throwable&& t)
	{
		if (condition)
			throw std::forward<Throwable>(t);
	}
	
	std::string const fixpath ("mock/");

	// Unused now, but may be useful for some stateful purpose.
	std::unordered_map<std::string, int> call_count;

	std::vector<std::string> read_text_file(std::string const& filename, char delim = '\n')
	{
		std::vector<std::string> text;
		std::string line;
		std::ifstream in((fixpath + filename).c_str());
		while (std::getline(in, line, delim))
			text.push_back(line);
		in.close();
		return text;
	}

	// Read the JSON contained in instruments filename (a json dump) to a string
	// Used for mock-testing the http input in instrs-ls
	std::string do_instruments()
	{
		static std::string const filename ("INSTRUMENTS.json");
		++call_count["do_instruments"];
		std::stringstream ss;
		for (auto&& line : read_text_file(filename)) {
			ss << std::move(line) << '\n';
		}
		return ss.str();
	}

	// Validate the query instruments list to ensure they're valid by comparing against the instruments file.
	//
	// Throws std::invalid_argument if instruments contains an invalid instrument. This models actual http
	//                              throwing std::invalid_argument if remote end returns HTTP 400 Bad request.
	void validate_instruments(std::vector<std::string> const& instruments)
	{
		// Note: Given it's not intended for it to be called repeatedly,
		//       there is no caching logic whatsoever here.
		std::vector<std::string> valid (read_text_file("INSTRUMENTS.valid"));
		for (auto const& instrument : instruments) {
			if (!std::binary_search(valid.begin(), valid.end(), instrument))
				throw std::invalid_argument("Invalid instrument <" + instrument + ">");
		}

	}
	std::string do_rates(std::vector<std::string> const& instruments)
	{
		++call_count["do_rates"];
		static std::string const rates_json_header ("{\n"
							    "\t\"prices\" : [\n");
		static std::string const rates_json_footer ("\t]\n"
							    "}");
		validate_instruments(instruments);
		std::vector<std::string> rates_x (read_text_file("RATES.hx"));
		std::stringstream out;
		std::vector<std::string> prices;

		for (auto const& instrument : instruments) {
			// find entry by comparing the instrument part of the quasi-json
			auto entry = std::find_if(rates_x.begin(), rates_x.end(),
						[&instrument] (std::string const& e) {
							if (e.find('"', 16) != (16 + instrument.size()))
								return false;
							return e.compare(16, instrument.size(), instrument) == 0;
						});
			if (entry == rates_x.end())
				throw std::logic_error("Unexpected invalid entry: <" + instrument + ">");
			std::stringstream price_entry;
			decltype(instrument.find(char())) index1 = 0;
			auto index2 = entry->find('@', index1 + 1);
			price_entry << "\t\t{""\n\t\t\t";
			while (index2 != std::string::npos) {
				price_entry << entry->substr(index1, index2 - index1) << "\n\t\t\t";
				index1 = index2 + 1;
				index2 = entry->find('@', index1 + 1);
			}
			price_entry << entry->substr(index1);
			price_entry << "\n\t\t}";
			prices.push_back(price_entry.str());
		}
		out << rates_json_header
		    << boost::algorithm::join(prices, ",")
		    << rates_json_footer;
		return out.str();
	}
}


std::string http::query(std::array<std::string, 3> uri) {
	static std::string const instruments_query ("/v1/instruments");
	static std::string const price_query_base  ("/v1/prices?instruments=");

	auto is_instruments = [&uri] { return uri[2] == instruments_query; };
	auto is_rates = [&uri] { return 0 == uri[2].compare(0, price_query_base.size(), price_query_base); };
	throw_if(uri[0] != "http",
		std::invalid_argument("Bad protocol \"" + uri[0] + "\""));
	throw_if(uri[1] != "api-sandbox.oanda.com", 
		std::invalid_argument("Bad comain \"" + uri[1] + "\""));
	if (is_instruments())
		return do_instruments();
	else if (is_rates()) {
		std::vector<std::string> rates;
		{
			auto index = price_query_base.size();
			auto index2 = uri[2].find("%2C", index);
			while (index2 != std::string::npos) {
				rates.push_back(uri[2].substr(index, index2 - index));
				index = index2 + 3;
				index2 = uri[2].find("%2C", index);
			}
			auto last = uri[2].substr(index);
			if (last.size() > 0)
				rates.push_back(std::move(last));
		}
		return do_rates(rates);
	} else
		throw std::invalid_argument("Bad URL \"" + uri[2] + "\"");
}
