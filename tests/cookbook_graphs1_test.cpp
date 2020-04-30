// The MIT License (MIT)
//
// Copyright (c) Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "daw/json/daw_json_iterator.h"
#include "daw/json/daw_json_link.h"

#include <daw/daw_graph.h>
#include <daw/daw_memory_mapped_file.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

namespace daw::cookbook_graphs1 {
	struct Metadata {
		int member0;
		std::string member1;
		bool member2;
	};

	struct GraphNode {
		size_t id;
		Metadata metadata;
	};

	struct GraphEdge {
		size_t source;
		size_t target;
	};
} // namespace daw::cookbook_graphs1

namespace daw::json {
	template<>
	struct json_data_contract<daw::cookbook_graphs1::Metadata> {
#ifdef __cpp_nontype_template_parameter_class
		using type = json_member_list<json_number<"member0", int>,
		                              json_string<"member1">, json_bool<"member2">>;
#else
		static inline constexpr char const member0[] = "member0";
		static inline constexpr char const member1[] = "member1";
		static inline constexpr char const member2[] = "member2";
		using type = json_member_list<json_number<member0, int>,
		                              json_string<member1>, json_bool<member2>>;
#endif
	};

	template<>
	struct json_data_contract<daw::cookbook_graphs1::GraphNode> {
#ifdef __cpp_nontype_template_parameter_class
		using type =
		  json_member_list<json_number<"id", size_t, LiteralAsStringOpt::Always>,
		                   json_class<"metadata", daw::cookbook_graphs1::Metadata>>;
#else
		static inline constexpr char const id[] = "id";
		static inline constexpr char const metadata[] = "metadata";
		using type =
		  json_member_list<json_number<id, size_t, LiteralAsStringOpt::Always>,
		                   json_class<metadata, daw::cookbook_graphs1::Metadata>>;
#endif
	};

	template<>
	struct json_data_contract<daw::cookbook_graphs1::GraphEdge> {
#ifdef __cpp_nontype_template_parameter_class
		using type = json_member_list<
		  json_number<"source", size_t, LiteralAsStringOpt::Always>,
		  json_number<"target", size_t, LiteralAsStringOpt::Always>>;
#else
		static inline constexpr char const source[] = "source";
		static inline constexpr char const target[] = "target";
		using type =
		  json_member_list<json_number<source, size_t, LiteralAsStringOpt::Always>,
		                   json_number<target, size_t, LiteralAsStringOpt::Always>>;
#endif
	};
} // namespace daw::json

struct Node {
	size_t id;
	int member0;
	std::string member1;
	bool member2;
};

int main( int argc, char **argv ) try {
	if( argc <= 1 ) {
		puts( "Must supply path to cookbook_graphs1.json file\n" );
		exit( EXIT_FAILURE );
	}
	auto data = daw::filesystem::memory_mapped_file_t<>( argv[1] );
	std::string_view json_sv = std::string_view( data.data( ), data.size( ) );

	daw::graph_t<Node> g{ };

	using node_range_t =
	  daw::json::json_array_range<daw::cookbook_graphs1::GraphNode>;
	for( auto node : node_range_t( json_sv, "nodes" ) ) {
		g.add_node( node.id, node.metadata.member0, node.metadata.member1,
		            node.metadata.member2 );
	}

	auto const find_node_id = [&g]( size_t id ) -> std::optional<daw::node_id_t> {
		auto result = g.find( [id]( auto const &node ) {
			daw_json_assert( node, "Expected a node" );
			return node.value( ).id == id;
		} );
		if( result.empty( ) ) {
			return { };
		}
		daw_json_assert( result.size( ) == 1, "Unexpected size" );
		return result.front( );
	};

	auto const find_node = [&]( size_t id ) {
		auto result = find_node_id( id );
		daw_json_assert( result, "Expected a result" );
		return g.get_node( *result );
	};

	using edge_range_t =
	  daw::json::json_array_range<daw::cookbook_graphs1::GraphEdge>;
	for( auto edge : edge_range_t( json_sv, "edges" ) ) {
		auto source_id = *find_node_id( edge.source );
		auto target_id = *find_node_id( edge.target );

		g.add_directed_edge( source_id, target_id );
	}
	(void)g;

	daw_json_assert( g.size( ) == 3U, "Expected graph to have 3 nodes" );
	auto nid_0 = find_node_id( 0 );
	daw_json_assert( nid_0, "Expecting a node with id 0" );
	auto nid_1 = find_node_id( 1 );
	daw_json_assert( nid_1, "Expecting a node with id 1" );
	auto nid_2 = find_node_id( 2 );
	daw_json_assert( nid_2, "Expecting a node with id 2" );

	auto const &n0 = find_node( 0 );

	daw_json_assert( n0.outgoing_edges( ).size( ) == 1U,
	                 "Node id 0 should only have 1 outgoing edge" );
	daw_json_assert(
	  n0.outgoing_edges( ).count( *nid_1 ) == 1U,
	  "Node id 0 should only have 2 incoming edge from node id 1" );

	daw_json_assert( n0.incoming_edges( ).size( ) == 2U,
	                 "Node id 0 should 2 incoming edges" );
	daw_json_assert( n0.incoming_edges( ).count( *nid_1 ) == 1U,
	                 "Node id 0 should have 1 incoming edge from node id 1" );

	daw_json_assert( n0.incoming_edges( ).count( *nid_2 ) == 1U,
	                 "Node id 0 should have 1 incoming edge from node id 2" );

	return 0;
} catch( daw::json::json_exception const &jex ) {
	std::cerr << "Exception thrown by parser: " << jex.reason( ) << std::endl;
	exit( 1 );
}
