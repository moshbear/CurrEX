//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// instr-ls implementation: json parsing
//
// Linking dependencies: -ljson-c -lboost_system

// parts from http://www.boost.org/doc/html/boost_asio/example/cpp03/http/client/sync_client.cpp
// parts from http://stackoverflow.com/a/16976703/982617

#include <array>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

extern "C" {
#include <json-c/json.h>
}

#include <http.hh>
#include <instr-ls.hh>

namespace {

	std::vector<std::string> parse_json(const std::string& data) {
		typedef json_object* J_obj; // non-refcounted json_object
		std::vector<std::string> result;

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

		auto j_instruments = json_try_get(j_rootobj.get(), "instruments");
		auto ninstruments = json_object_array_length(j_instruments);
		for (auto i = 0; i < ninstruments; ++i) {
			auto j_instr = json_object_array_get_idx(j_instruments, i);
			result.push_back(json_object_get_string(json_try_get(j_instr, "instrument")));
		}
		return result;
	}

	std::array<std::string, 3> make_query_url() {
		std::string prot("http");
		std::string domain("api-sandbox.oanda.com");
		std::string q("/v1/instruments");
		return {{ prot, domain, q }};
	}
}

std::vector<std::string> instruments::list() {
	return parse_json(http::query(make_query_url()));
}

