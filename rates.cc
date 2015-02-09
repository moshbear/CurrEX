//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Linking dependencies: -ljson-c -lboost_system

// Implementation detail of rates

// parts from http://www.boost.org/doc/html/boost_asio/example/cpp03/http/client/sync_client.cpp
// parts from http://stackoverflow.com/a/16976703/982617

#include <array>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

#include <http.hh>
#include <rates.hh>

extern "C" {
#include <json-c/json.h>
}

namespace {

	std::vector<rates::Rate> parse_json(const std::string& data) {
		using rates::Rate;
		typedef json_object* J_obj; // non-refcounted json_object
		std::vector<Rate> result;

		struct JsonC_deleter { // json_object refcount decrementer
			void operator() (json_object* o) const
			{ json_object_put(o); }
		};

		// json bits adapted from json-c array answer on SO and json-c docs
		std::shared_ptr<json_object> j_rootobj(json_tokener_parse(data.c_str()), JsonC_deleter());
		
		auto json_try_get = [] (J_obj root, const char* name) -> J_obj {
			J_obj val;
			if (!json_object_object_get_ex(root, name, &val))
				throw std::invalid_argument(std::string("JSON error: \"") + name + "\" not found");
			return val;
		};

		auto j_prices = json_try_get(j_rootobj.get(), "prices");
		auto nprices = json_object_array_length(j_prices);
		for (auto i = 0; i < nprices; ++i) {
			auto j_price = json_object_array_get_idx(j_prices, i);
			result.push_back(Rate(
				json_object_get_string(json_try_get(j_price, "instrument")),
				json_object_get_double(json_try_get(j_price, "bid")),
				json_object_get_double(json_try_get(j_price, "ask"))
			));
		}
		return result;
	}
	
	std::array<std::string, 3> make_query_url(std::vector<std::string> const& instrs) {
		if (!instrs.size())
			throw std::invalid_argument("empty vector");
		std::string prot("http");
		std::string domain("api-sandbox.oanda.com");
		std::string q("/v1/prices?instruments=");
		for (auto const& instr : instrs) {
			q += instr;
			q += "%2C";
		}
		q.resize(q.size() - 3);
		return {{ prot, domain, q }};
	}
}

std::vector<rates::Rate> rates::get(std::vector<std::string> const& instruments) {
	return parse_json(http::query(make_query_url(instruments)));
}

