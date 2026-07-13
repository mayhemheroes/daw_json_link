#!/usr/bin/env bash
#
# mayhem/test.sh — RUN the full upstream ctest suite built by mayhem/build.sh.
# The suite is upstream's own assertion-based unit/functional tests (daw_json_assert /
# test_assert golden checks), so it is behavioral: a neutered exit(0) binary fails it.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"
cd "${SRC:-/mayhem}"
SRC="${SRC:-/mayhem}"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

BUILD="$SRC/build-tests"
[ -f "$BUILD/CTestTestfile.cmake" ] || { echo "test.sh: $BUILD missing — mayhem/build.sh must run first" >&2; emit_ctrf cmake-ctest 0 1; exit 1; }

LOG=/tmp/ctest.log
( cd "$BUILD" && ctest -j"$MAYHEM_JOBS" --output-on-failure ) | tee "$LOG"

# ctest summary: "100% tests passed, 0 tests failed out of 268" (+ optional "N tests skipped")
TOTAL=$(sed -n 's/.*tests failed out of \([0-9]\+\).*/\1/p' "$LOG" | tail -1)
FAILED=$(sed -n 's/.*, \([0-9]\+\) tests failed out of.*/\1/p' "$LOG" | tail -1)
SKIPPED=$(sed -n 's/^\([0-9]\+\) tests skipped.*/\1/p' "$LOG" | tail -1)
[ -n "${TOTAL:-}" ] && [ -n "${FAILED:-}" ] || { echo "test.sh: could not parse ctest summary" >&2; emit_ctrf cmake-ctest 0 1; exit 1; }
SKIPPED=${SKIPPED:-0}
PASSED=$(( TOTAL - FAILED - SKIPPED ))

# Known-answer output check (ctest's default pass criterion is exit-0, which alone is not
# behavioral): simple_test must actually parse cities.json and print the matched record.
KA_OUT=$( cd "$SRC/test_data" && "$BUILD/tests/simple_test" ./cities.json 2>/dev/null )
if printf '%s' "$KA_OUT" | grep -q 'Chitungwiza was found.' && \
   printf '%s' "$KA_OUT" | grep -q '{"country":"ZW","name":"Chitungwiza","lat":"-18.01274","lng":"31.07555"}'; then
  PASSED=$(( PASSED + 1 ))
else
  echo "test.sh: known-answer check FAILED (simple_test output mismatch)" >&2
  FAILED=$(( FAILED + 1 ))
fi

emit_ctrf cmake-ctest "$PASSED" "$FAILED" "$SKIPPED"
