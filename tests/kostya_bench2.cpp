// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#include <daw/json/daw_json_link.h>
#include <daw/json/daw_json_value_state.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <unistd.h>

std::string read_file( std::string const &filename ) {
	std::ifstream f( filename );
	if( !f ) {
		return {};
	}
	return std::string( std::istreambuf_iterator<char>( f ),
	                    std::istreambuf_iterator<char>( ) );
}

int main( int argc, char *argv[] ) {
	std::string const json_text = read_file( "/tmp/1.json" );

	double x = 0;
	double y = 0;
	double z = 0;
	int len = 0;

	using namespace daw::json;
	auto rng =
	  from_json_unchecked<json_delayed<no_name>>( json_text, "coordinates" );

	auto val = json_value_state( );
	for( json_pair item : rng ) {
		val.reset( item.value );
		x += from_json_unchecked<double>( val["x"] );
		y += from_json_unchecked<double>( val["y"] );
		z += from_json_unchecked<double>( val["z"] );
		++len;
	}
	std::cout << x / len << '\n';
	std::cout << y / len << '\n';
	std::cout << z / len << '\n';

	return EXIT_SUCCESS;
}
