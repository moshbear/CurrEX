//          Copyright Andrey Moshbear 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// Common HTTP bits.
// Linking depdendencies: -lboost_system


// parts from http://www.boost.org/doc/html/boost_asio/example/cpp03/http/client/sync_client.cpp

#include <iostream>
#include <istream>
#include <ostream>
#include <array>
#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <boost/asio.hpp>

#include <http.hh>

using boost::asio::ip::tcp;

std::string http::query(std::array<std::string, 3> uri) {

	// things following from here are more or less copy paste from
	// linked boost tutorial, excepting exceptions and json parsing

	boost::asio::io_service io_service;

	// Get a list of endpoints corresponding to the server name.
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(uri[1], uri[0]);
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

	// Try each endpoint until we successfully establish a connection.
	tcp::socket socket(io_service);
	boost::asio::connect(socket, endpoint_iterator);

	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "GET " << uri[2] << " HTTP/1.0\r\n";
	request_stream << "Host: " << uri[1] << "\r\n";
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n\r\n";

	// Send the request.
	boost::asio::write(socket, request);

	// Read the response status line. The response streambuf will automatically
	// grow to accommodate the entire line. The growth may be limited by passing
	// a maximum size to the streambuf constructor.
	boost::asio::streambuf response;
	boost::asio::read_until(socket, response, "\r\n");

	// Check that response is OK.
	std::istream response_stream(&response);
	std::string http_version;
	response_stream >> http_version;
	unsigned int status_code;
	response_stream >> status_code;
	std::string status_message;
	std::getline(response_stream, status_message);
	if (!response_stream || http_version.substr(0, 5) != "HTTP/")
		throw std::logic_error("ill-formed HTTP response");
	if (status_code != 200)
		throw std::invalid_argument(
			([status_code] {
				std::stringstream s;
				s << "HTTP status code: "; s << status_code; s << "\n"; 
				std::string ss(s.str());
				return ss;
			})());

	// Read the response headers, which are terminated by a blank line.
	boost::asio::read_until(socket, response, "\r\n\r\n");

	// Process the response headers.
	std::string header;
	std::string headers;
	while (std::getline(response_stream, header) && header != "\r") {
		headers += header;
		headers += "\n";
	}

	std::stringstream data;
	// Write whatever content we already have to output.
	if (response.size() > 0)
		data << &response;

	// Read until EOF, writing data to output as we go.
	boost::system::error_code error;
	while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
		data << &response;
	if (error != boost::asio::error::eof)
		throw boost::system::system_error(error);

	return data.str();
}
