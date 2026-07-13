#!/usr/bin/env bash
#
# mayhem/build.sh — build the daw_json_link fuzz harness and the upstream test suite.
#
# daw_json_link is a header-only C++17 JSON library. Its header-only dependencies
# (beached/header_libraries, beached/utf_range) and the test corpus
# (beached/daw_json_link_test_data) are pre-cloned into /opt/daw-deps by the
# Dockerfile so this script is fully air-gapped and re-runnable offline:
#   /opt/daw-deps/header_libraries        (FetchContent daw_header_libraries)
#   /opt/daw-deps/utf_range               (FetchContent daw_utf_range)
#   /opt/daw-deps/daw_json_link_test_data (FetchContent daw_json_link_test_data)
#
# Outputs:
#   /mayhem/fuzz_serializer             sanitized libFuzzer target (the Mayhem target)
#   /mayhem/fuzz_serializer-standalone  run-once non-fuzzer reproducer
#   build-tests/                        the full upstream CMake/ctest suite (normal flags)
set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — it must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "${SRC:-/mayhem}"
SRC="${SRC:-/mayhem}"

DEPS=/opt/daw-deps
HDR_INC="$DEPS/header_libraries/include"
UTF_INC="$DEPS/utf_range/include"

# Upstream's CMake fetches test_data only when $SRC/test_data is absent; stage the
# pre-cloned copy so the configure step never touches the network.
if [ ! -d "$SRC/test_data" ]; then
  cp -r "$DEPS/daw_json_link_test_data" "$SRC/test_data"
fi

INCLUDES="-I$SRC/include -I$HDR_INC -I$UTF_INC"

# 1) Sanitized libFuzzer harness — header-only library, so instrumenting the harness TU
#    instruments the whole parsing/serialization code path. -gdwarf-3: DWARF must be < 4.
# shellcheck disable=SC2086
$CXX -std=c++17 -O1 $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE $INCLUDES \
  "$SRC/mayhem/fuzz_serializer.cpp" -o /mayhem/fuzz_serializer

# 2) Standalone run-once reproducer (no libFuzzer runtime). Compile the C driver
#    separately so its LLVMFuzzerTestOneInput reference keeps C linkage.
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o /tmp/standalone_main.o
# shellcheck disable=SC2086
$CXX -std=c++17 -O1 $SANITIZER_FLAGS $DEBUG_FLAGS $INCLUDES \
  "$SRC/mayhem/fuzz_serializer.cpp" /tmp/standalone_main.o -o /mayhem/fuzz_serializer-standalone

# 3) Full upstream test suite, NORMAL flags (clean, unsanitized) — mayhem/test.sh only RUNS it.
#    FetchContent is pinned to the pre-cloned trees and fully disconnected (air-gapped).
cmake -S "$SRC" -B "$SRC/build-tests" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS" \
  -DDAW_ENABLE_TESTING=ON \
  -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
  -DFETCHCONTENT_SOURCE_DIR_DAW_HEADER_LIBRARIES="$DEPS/header_libraries" \
  -DFETCHCONTENT_SOURCE_DIR_DAW_UTF_RANGE="$DEPS/utf_range"
cmake --build "$SRC/build-tests" -j"$MAYHEM_JOBS"

echo "build.sh: built /mayhem/fuzz_serializer(-standalone) and the upstream test suite in build-tests/"
