
add_executable(fuzz_daw fuzz_serializer.cpp)

target_compile_options(fuzz_daw PRIVATE -fsanitize=undefined,fuzzer)
target_link_options(fuzz_daw PRIVATE -fsanitize=undefined,fuzzer)

target_link_libraries(fuzz_daw daw-json-link)