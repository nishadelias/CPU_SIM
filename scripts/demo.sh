#!/usr/bin/env bash
# Pre-demo validation: build, run canned cache/predictor comparisons, print pass/fail.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
CPUSIM="${BUILD}/cpusim"
PASS=0
FAIL=0
SKIP=0

pass() { echo "  PASS: $*"; PASS=$((PASS + 1)); }
fail() { echo "  FAIL: $*" >&2; FAIL=$((FAIL + 1)); }
skip() { echo "  SKIP: $*"; SKIP=$((SKIP + 1)); }

extract_json_field() {
  local json="$1" field="$2"
  echo "$json" | sed -n "s/.*\"${field}\":\([0-9.eE+-]*\).*/\1/p" | head -1
}

run_bench() {
  local program="$1"
  shift
  "${CPUSIM}" "$program" --bench --json "$@" 2>/dev/null
}

echo "=== CPU_SIM Demo Script ==="
echo "Project root: ${ROOT}"
echo

# --- Build CLI ---
echo "[1/4] Configure and build CLI..."
CMAKE_ARGS=(-S "${ROOT}" -B "${BUILD}" -DBUILD_GUI=OFF)
if [[ "$(uname -s)" == "Darwin" ]] && command -v xcrun >/dev/null 2>&1; then
  SDK="$(xcrun --show-sdk-path 2>/dev/null || true)"
  if [[ -n "${SDK}" ]]; then
    CMAKE_ARGS+=(-DCMAKE_OSX_SYSROOT="${SDK}")
  fi
fi
cmake "${CMAKE_ARGS[@]}" -G Ninja 2>/dev/null \
  || cmake "${CMAKE_ARGS[@]}"
cmake --build "${BUILD}" --target cpusim -j
if [[ -x "${CPUSIM}" ]]; then
  pass "cpusim built at ${CPUSIM}"
else
  fail "cpusim not found after build"
  echo
  echo "Summary: ${PASS} passed, ${FAIL} failed, ${SKIP} skipped"
  exit 1
fi
echo

# --- Build example ELFs ---
echo "[2/4] Build example ELFs..."
if bash "${ROOT}/scripts/build_example_elf.sh"; then
  pass "example ELFs built"
else
  skip "RISC-V cross-compiler not available — skipping ELF benchmarks"
  echo
  echo "[3/4] Hex fallback benchmark..."
  HEX="${ROOT}/instruction_memory/instMem-forward.txt"
  if [[ -f "${HEX}" ]]; then
    OUT="$(run_bench "${HEX}" --predictor ant || true)"
    if echo "$OUT" | grep -q '"cycles"'; then
      pass "hex program runs with --bench --json"
    else
      fail "hex benchmark produced no JSON output"
    fi
  else
    fail "hex fallback file not found: ${HEX}"
  fi
  echo
  echo "Summary: ${PASS} passed, ${FAIL} failed, ${SKIP} skipped"
  exit $(( FAIL > 0 ? 1 : 0 ))
fi
echo

COUNT_PRIMES="${BUILD}/count_primes.elf"
FIB_PRINT="${BUILD}/fib_print.elf"

# --- Branch predictor comparison ---
echo "[3/4] Branch predictor comparison (count_primes.elf)..."
ANT_JSON="$(run_bench "${COUNT_PRIMES}" --predictor ant)"
GS_JSON="$(run_bench "${COUNT_PRIMES}" --predictor gshare)"

ANT_ACC="$(extract_json_field "$ANT_JSON" branch_predictor_accuracy)"
GS_ACC="$(extract_json_field "$GS_JSON" branch_predictor_accuracy)"

if [[ -z "$ANT_ACC" || -z "$GS_ACC" ]]; then
  fail "could not parse branch_predictor_accuracy from bench output"
else
  echo "  Always Not Taken accuracy: ${ANT_ACC}%"
  echo "  GShare accuracy:           ${GS_ACC}%"
  if awk -v a="$ANT_ACC" -v g="$GS_ACC" 'BEGIN { exit (g > a) ? 0 : 1 }'; then
    pass "GShare accuracy (${GS_ACC}%) > Always Not Taken (${ANT_ACC}%)"
  else
    fail "expected GShare accuracy > Always Not Taken (got ${GS_ACC} vs ${ANT_ACC})"
  fi
fi
echo

# --- Cache comparison ---
echo "[4/4] Cache comparison (fib_print.elf)..."
DIRECT_JSON="$(run_bench "${FIB_PRINT}" --cache direct)"
WAY4_JSON="$(run_bench "${FIB_PRINT}" --cache 4way)"

DIRECT_HR="$(extract_json_field "$DIRECT_JSON" cache_hit_rate)"
WAY4_HR="$(extract_json_field "$WAY4_JSON" cache_hit_rate)"

if [[ -z "$DIRECT_HR" || -z "$WAY4_HR" ]]; then
  fail "could not parse cache_hit_rate from bench output"
else
  echo "  Direct-mapped hit rate: ${DIRECT_HR}%"
  echo "  4-way hit rate:         ${WAY4_HR}%"
  if awk -v d="$DIRECT_HR" -v w="$WAY4_HR" 'BEGIN { exit (w >= d) ? 0 : 1 }'; then
    pass "4-way hit rate (${WAY4_HR}%) >= direct-mapped (${DIRECT_HR}%)"
  else
    fail "expected 4-way hit rate >= direct-mapped (got ${WAY4_HR} vs ${DIRECT_HR})"
  fi
fi
echo

echo "=== Summary: ${PASS} passed, ${FAIL} failed, ${SKIP} skipped ==="
exit $(( FAIL > 0 ? 1 : 0 ))
