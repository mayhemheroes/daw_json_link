# JSON Link v3

## Note: this requires C++20 which isn't released yet and is still early testing.
## Note2: Just backported to allow C++17.  Names must be static constexpr string literals with names, instead of bare string literals  e.g.
```cpp
static constexpr char const name_a[] = "name_a";

...json_number<name_a>
```

For now v3 is focussed on deserialization.  This can be accomplished by writing a function called describe_json_class with a single arugment that is your type.  The library is only concerned with it's return value. For example:

```C++
#include <daw/json/daw_json_link.h>

struct TestClass {
	int i = 0;
	double d = 0.0;
	bool b = false;
	daw::string_view s{};
	std::vector<int> y{};

	TestClass( int Int, double Double, bool Bool, daw::string_view S,
	            std::vector<int> Y ) noexcept
	  : i( Int )
	  , d( Double )
	  , b( Bool )
	  , s( S )
	  , y( Y ) {}
};

auto describe_json_class( TestClass ) {
	using namespace daw::json;
	return class_description_t<
		json_number<"i", int>,
		json_number<"d">,
		json_bool<"b">,
		json_string<"s", daw::string_view>,
		json_array<"y", std::vector<int>, json_number<"", int>>
 	>{};
}

std::string json_data = ...;
std::string json_array_data = ...;

TestClass test_class = daw::json::from_json<TestClass>( json_data );
std::vector<TestClass> arry_of_test_class = daw::json::from_json_array<json_class<"", TestClass>>( json_data );
```
Both aggregate and normal construction is supported.  The description provides the values needed to construct your type and the order.  The order specified is the order they are placed into the constructor.  There are customization points to provide a way of constructing your type too(TODO discuss customization points)  A class like:

```cpp
#include <daw/json/daw_json_link.h>

struct AggClass {
	int a{};
	double b{};
};
auto describe_json_class( AggClass ) {
	using namespace daw::json;
	return class_description_t<
		json_number<"a", int>,
		json_number<"b">
	>{};
}
```
Works too.

Iterating over JSON arrays.  The input iterator ```daw::json::json_array_iterator<JsonElement>``` allows one to iterator over the array of JSON elements.  It is technically an input iterator but can be stored and reused like a forward iterator.  It does not return a reference but a value.
```cpp

#include <daw/json/daw_json_link.h>

struct AggClass {
	int a{};
	double b{};
};
auto describe_json_class( AggClass ) {
	using namespace daw::json;
	return class_description_t<
		json_number<"a", int>,
		json_number<"b">
	>{};

int main( ) {
	std::string json_array_data = ...;
	using iterator_t = daw::json::json_array_iterator<JsonElement<"", AggClass>>;
	auto pos = std::find_if( iterator_t( json_array_data ), iterator_t( ),
	    []( AggData const & element ) { return element.b > 1000.0; } );
	if( pos == iterator_t( ) ) {
		std::cout << "Not found\n";
	} else {
		std::cout << "Found\n";
	}
}
```

## Serialization
To enable serialization on must create an additional free function called ```to_json_data( T );``` It will provide a mapping from your type to the arguments provided in the class description.  To serialize to a json string, one calls ```to_json( value );``` where value is a registered type. Using the exmaple above lets add that

```cpp
#include <daw/json/daw_json_link.h>
#include <tuple>

struct AggClass {
	int a{};
	double b{};
};
auto describe_json_class( AggClass ) {
	using namespace daw::json;
	return class_description_t<
		json_number<"a", int>,
		json_number<"b">
	>{};
}
auto to_json_data( AggClass const & ) {
    return std::forward_as_tuple( c.a, c.b );
}
//...
AggData value = //...;
std::string json_data = to_json( value );

// or
std::vector<AggData> values = //...;
std::string json_array_data = to_json_array( values );
```
	
