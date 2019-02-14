// The MIT License (MIT)
//
// Copyright (c) 2018-2019 Darrell Wright
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

#pragma once

#include <cstdint>
//#include <iostream>
#include <limits>
#include <string>

#include <daw/daw_algorithm.h>
#include <daw/daw_bounded_string.h>
#include <daw/daw_exception.h>
#include <daw/daw_parser_helper_sv.h>
#include <daw/daw_sort_n.h>
#include <daw/daw_string_view.h>
#include <daw/daw_traits.h>
#include <daw/daw_utility.h>

namespace daw {
	namespace json {
		template<typename T>
		constexpr T from_json_t( daw::string_view sv );

		namespace impl {
			namespace {
				constexpr char to_lower( char c ) noexcept {
					return c | ' ';
				}

				enum class JsonParseTypes : uint_fast8_t {
					Number,
					Bool,
					String,
					Date,
					Class
				};

				template<typename T>
				using json_parser_description_t =
				  decltype( describe_json_parser( std::declval<T>( ) ) );

				template<typename T>
				inline constexpr bool has_json_parser_description_v =
				  is_detected_v<json_parser_description_t, T>;

				template<JsonParseTypes v>
				using ParseTag = std::integral_constant<JsonParseTypes, v>;

				template<typename string_t>
				struct kv_t {
					string_t name;
					JsonParseTypes expected_type;
					bool nullable;
					size_t pos;

					constexpr kv_t( string_t Name, JsonParseTypes Expected, bool Nullable,
					                size_t Pos )
					  : name( std::move( Name ) )
					  , expected_type( Expected )
					  , nullable( Nullable )
					  , pos( Pos ) {}
				};

				struct value_pos {
					string_view value_str{};
					bool is_nullable;

					explicit constexpr value_pos( bool Nullable ) noexcept
					  : is_nullable( Nullable ) {}

					constexpr explicit operator bool( ) const noexcept {
						return is_nullable or !value_str.empty( );
					}
				};

				template<typename string_t>
				kv_t( string_t )->kv_t<string_t>;

				template<typename JsonType>
				using json_parse_to = typename JsonType::parse_to_t;

				template<typename JsonType>
				inline constexpr bool is_json_nullable_v = JsonType::nullable;

				constexpr daw::string_view parse_name( daw::string_view &sv ) {
					if( sv.front( ) != '"' ) {
						sv = parser::trim_left( sv );
					}
					if( sv.front( ) != '"' ) {
						if( sv.front( ) >= 'a' and sv.front( ) <= 'z' ) {
							throw "lower-case";
						}
						if( sv.front( ) >= '0' and sv.front( ) <= '9' ) {
							throw "number";
						}
						switch( sv.front( ) ) {
						case '"':
							break;
						case '\n':
							throw "newline";
						case ' ':
							throw "space";
						case ':':
							throw ":";
						case '	':
							throw "tab";
						case ',':
							throw ",";
						default:
							throw "no quote";
						}
					}
					sv.remove_prefix( );
					auto name = sv.pop_front( "\"" );
					sv = parser::trim_left( sv );
					sv.pop_front( ":" );
					sv = parser::trim_left( sv );
					return name;
				}

				constexpr daw::string_view
				skip_string( daw::string_view &sv ) noexcept {
					// TODO: handle escaped quotes
					// Assumes front()  == '"'
					auto tmp = sv.substr( 1 );
					auto result = tmp.pop_front( "\"" );
					sv = tmp;
					auto pos = sv.find_first_of( ",}]" );
					exception::precondition_check( pos != daw::string_view::npos,
					                               "Invalid class" );
					sv.remove_prefix( pos + 1 );
					sv = parser::trim_left( sv );
					return result;
				}

				constexpr daw::string_view skip_other( daw::string_view &sv ) noexcept {
					auto pos = sv.find_first_of( ",}]" );
					exception::precondition_check( pos != sv.npos, "Invalid class" );
					auto result = sv.pop_front( pos );
					sv.remove_prefix( );
					sv = parser::trim_left( sv );
					return result;
				}

				constexpr daw::string_view skip_class( daw::string_view &sv ) noexcept {
					size_t bracket_count = 1;
					bool is_escaped = false;
					bool in_quotes = false;
					auto tmp_sv = sv;
					sv.remove_prefix( );
					while( !sv.empty( ) and bracket_count > 0 ) {
						switch( sv.front( ) ) {
						case '\\':
							if( !in_quotes and !is_escaped ) {
								is_escaped = true;
								sv.remove_prefix( );
								continue;
							}
							break;
						case '"':
							if( !is_escaped ) {
								in_quotes = !in_quotes;
								sv.remove_prefix( );
								continue;
							}
							break;
						case '{':
							if( !in_quotes and !is_escaped ) {
								++bracket_count;
							}
							break;
						case '}':
							if( !in_quotes and !is_escaped ) {
								--bracket_count;
							}
						}
						is_escaped = false;
						sv.remove_prefix( );
					}
					tmp_sv = tmp_sv.pop_front( tmp_sv.size( ) - sv.size( ) );
					auto pos = sv.find_first_of( ",}]" );
					exception::precondition_check( pos != sv.npos, "Invalid class" );
					sv.remove_prefix( );
					sv = parser::trim_left( sv );
					return tmp_sv;
				}

				constexpr daw::string_view skip_array( daw::string_view &sv ) noexcept {
					size_t bracket_count = 1;
					bool is_escaped = false;
					bool in_quotes = false;
					auto tmp_sv = sv;
					sv.remove_prefix( );
					while( !sv.empty( ) and bracket_count > 0 ) {
						switch( sv.front( ) ) {
						case '\\':
							if( !in_quotes and !is_escaped ) {
								is_escaped = true;
								sv.remove_prefix( );
								continue;
							}
							break;
						case '"':
							if( !is_escaped ) {
								in_quotes = !in_quotes;
								sv.remove_prefix( );
								continue;
							}
							break;
						case '[':
							if( !in_quotes and !is_escaped ) {
								++bracket_count;
							}
							break;
						case ']':
							if( !in_quotes and !is_escaped ) {
								--bracket_count;
							}
						}
						is_escaped = false;
						sv.remove_prefix( );
					}
					tmp_sv = tmp_sv.pop_front( tmp_sv.size( ) - sv.size( ) );
					auto pos = sv.find_first_of( ",}]" );
					exception::precondition_check( pos != sv.npos, "Invalid array" );
					sv.remove_prefix( );
					sv = parser::trim_left( sv );
					return tmp_sv;
				}

				constexpr daw::string_view skip_value( daw::string_view &sv ) {
					sv = parser::trim_left( sv );
					switch( sv.front( ) ) {
					case '"':
						return skip_string( sv );
					case '[':
						return skip_array( sv );
					case '{':
						return skip_class( sv );
					default:
						return skip_other( sv );
					}
				}

				template<typename ParseInfo>
				constexpr auto parse_value( ParseTag<JsonParseTypes::Number>,
				                            value_pos pos ) {
					using result_t = typename ParseInfo::parse_to_t;
					if constexpr( is_floating_point_v<result_t> ) {
						// TODO
						return static_cast<result_t>( 0.12345 );
					} else {
						return parser::parse_int<result_t>( pos.value_str );
					}
				}

				template<typename ParseInfo>
				constexpr auto parse_value( ParseTag<JsonParseTypes::Bool>,
				                            value_pos pos ) {
					using result_t = typename ParseInfo::parse_to_t;

					return construct_a<result_t>{}( to_lower( pos.value_str.front( ) ) ==
					                                't' );
				}

				template<typename ParseInfo>
				constexpr auto parse_value( ParseTag<JsonParseTypes::String>,
				                            value_pos pos ) {

					using result_t = typename ParseInfo::parse_to_t;
					return construct_a<result_t>{}( pos.value_str.data( ),
					                                pos.value_str.size( ) );
				}

				template<typename ParseInfo>
				constexpr auto parse_value( ParseTag<JsonParseTypes::Date>,
				                            value_pos pos ) {
					using result_t = typename ParseInfo::parse_to_t;
					return result_t{};
				}

				template<typename ParseInfo>
				constexpr auto parse_value( ParseTag<JsonParseTypes::Class>,
				                            value_pos pos ) {

					using result_t = typename ParseInfo::parse_to_t;
					return from_json_t<result_t>( pos.value_str );
				}
			} // namespace
		}   // namespace impl

		template<typename... JsonMembers>
		class json_parser_t {
			static constexpr size_t find_string_capacity( ) noexcept {
				return ( JsonMembers::name.extent + ... );
			}
			using string_t = basic_bounded_string<char, find_string_capacity( )>;

			template<size_t N>
			static constexpr impl::kv_t<string_t> get_item( ) noexcept {
				using type_t = traits::nth_type<N, JsonMembers...>;
				return {type_t::name, type_t::expected_type, type_t::nullable, N};
			}

			template<size_t... Is>
			static constexpr auto make_map( std::index_sequence<Is...> ) noexcept {

				return std::array{get_item<Is>( )...};
			}

			static constexpr auto name_map =
			  make_map( std::index_sequence_for<JsonMembers...>{} );

			static constexpr bool has_name( daw::string_view key ) noexcept {
				auto result = algorithm::find_if(
				  begin( name_map ), end( name_map ),
				  [key]( auto const &kv ) { return kv.name == key; } );
				return result != std::end( name_map );
			}

			static constexpr size_t find_name( daw::string_view key ) noexcept {
				auto result = algorithm::find_if(
				  begin( name_map ), end( name_map ),
				  [key]( auto const &kv ) { return kv.name == key; } );
				if( result == std::end( name_map ) ) {
					std::terminate( );
				}
				return static_cast<size_t>(
				  std::distance( begin( name_map ), result ) );
			}

			template<size_t N>
			static constexpr decltype( auto ) parse_item(
			  std::array<impl::value_pos, sizeof...( JsonMembers )> const &locations,
			  daw::string_view sv ) {
				using type_t = traits::nth_type<N, JsonMembers...>;
				return impl::parse_value<type_t>(
				  impl::ParseTag<type_t::expected_type>{}, locations[N] );
			}

			static constexpr auto get_locations( daw::string_view & sv ) {
				std::array result = {
				  impl::value_pos( impl::is_json_nullable_v<JsonMembers> )...};

				while( !sv.empty( ) and sv.front( ) != '}' ) {
					auto name = impl::parse_name( sv );
					if( !has_name( name ) ) {
						impl::skip_value( sv );
						continue;
					}
					size_t const pos = find_name( name );
					result[pos].value_str = impl::skip_value( sv );
					sv = parser::trim_left( sv );
				}
				return result;
			}

			template<typename Result, size_t... Is>
			static constexpr Result parse_json_class( daw::string_view sv,
			                                          std::index_sequence<Is...> ) {
				static_assert(
				  can_construct_a_v<Result, typename JsonMembers::parse_to_t...>,
				  "Supplied types cannot be used for construction of this type" );

				auto sv_orig = sv;

				sv = parser::trim_left( sv );
				exception::precondition_check( sv.front( ) == '{' );
				sv.remove_prefix( );
				sv = parser::trim_left( sv );

				auto const locations = get_locations( sv );

				// Ensure locations of all parts there
				exception::precondition_check( algorithm::all_of(
				  begin( locations ), end( locations ), []( auto const &loc ) -> bool {
					  return static_cast<bool>( loc );
				  } ) );

				return construct_a<Result>{}( parse_item<Is>( locations, sv_orig )... );
			}

		public:
			template<typename Result>
			static constexpr decltype( auto ) parse( daw::string_view sv ) {
				return parse_json_class<Result>(
				  sv, std::index_sequence_for<JsonMembers...>{} );
			}
		};

		template<basic_bounded_string Name, typename T = double,
		         bool Nullable = false>
		struct json_number {
			static_assert( std::is_arithmetic_v<T>,
			               "Number must be an arithmentic type" );
			static constexpr auto const name = Name;
			static constexpr impl::JsonParseTypes expected_type =
			  impl::JsonParseTypes::Number;
			static constexpr bool nullable = Nullable;
			using parse_to_t = T;
		};

		template<basic_bounded_string Name, typename T = bool,
		         bool Nullable = false>
		struct json_bool {
			static_assert( std::is_convertible_v<bool, T>,
			               "Supplied result type must be convertable from bool" );
			static constexpr auto const name = Name;
			static constexpr impl::JsonParseTypes expected_type =
			  impl::JsonParseTypes::Bool;
			static constexpr bool nullable = Nullable;
			using parse_to_t = T;
		};

		template<basic_bounded_string Name, typename T = void, // std::string,
		         bool Nullable = false>
		struct json_string {
			static constexpr auto const name = Name;
			static constexpr impl::JsonParseTypes expected_type =
			  impl::JsonParseTypes::String;
			static constexpr bool nullable = Nullable;
			using parse_to_t = T;
		};

		template<basic_bounded_string Name,
		         typename T = std::chrono::system_clock::time_point,
		         bool Nullable = false>
		struct json_date {
			static constexpr auto const name = Name;
			static constexpr impl::JsonParseTypes expected_type =
			  impl::JsonParseTypes::Date;
			static constexpr bool nullable = Nullable;
			using parse_to_t = T;
		};

		template<basic_bounded_string Name, typename T, bool Nullable = false>
		struct json_class {
			static constexpr auto const name = Name;
			static constexpr impl::JsonParseTypes expected_type =
			  impl::JsonParseTypes::Class;
			static constexpr bool nullable = Nullable;
			using parse_to_t = T;
		};

		template<typename T>
		constexpr T from_json_t( daw::string_view sv ) {
			static_assert(
			  impl::has_json_parser_description_v<T>,
			  "A function call describe_json_parser must exist for type." );

			return impl::json_parser_description_t<T>::template parse<T>( sv );
		}

	} // namespace json
} // namespace daw
