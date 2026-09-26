#!/usr/bin/env bash
# Pre-demo validation: build, run canned cache/predictor comparisons, print pass/fail.
# Covers RISC-V (required for full benches) and AArch32 when arm-none-eabi-gcc is present.
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
echo "[1/5] Configure and build CLI..."
CMAKE_ARGS=(-S "${ROOT}" -B "${BUILD}" -DBUILD_GUI=OFF)
if [[ "$(uname -s)" == "Darwin" ]] && command -v xcrun >/dev/null 2>&1; then
  # Prefer a stable SDK when Command Line Tools ship a too-new default (link failures).
  SDK=""
  for candidate in \
      /Library/Developer/CommandLineTools/SDKs/MacOSX15.sdk \
      /Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk \
      /Library/Developer/CommandLineTools/SDKs/MacOSX14.sdk \
      "$(xcrun --show-sdk-path 2>/dev/null || true)"; do
    if [[ -n "${candidate}" && -d "${candidate}" ]]; then
      SDK="${candidate}"
      break
    fi
  done
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

# --- Build RISC-V example ELFs ---
echo "[2/5] Build RISC-V example ELFs..."
RV_OK=0
if bash "${ROOT}/scripts/build_example_elf.sh"; then
  pass "RISC-V example ELFs built"
  RV_OK=1
else
  skip "RISC-V cross-compiler not available — skipping RV ELF benchmarks"
fi
echo

# --- Build AArch32 example ELFs (optional) ---
echo "[3/5] Build AArch32 example ELFs..."
ARM_OK=0
if bash "${ROOT}/scripts/build_example_elf_arm.sh"; then
  pass "AArch32 example ELFs built"
  ARM_OK=1
else
  skip "ARM cross-compiler not available — skipping ARM ELF benchmarks"
fi
echo

if [[ "${RV_OK}" -eq 0 && "${ARM_OK}" -eq 0 ]]; then
  echo "[4/5] Hex fallback benchmark..."
  HEX="${ROOT}/instruction_memory/instMem-forward.txt"
  if [[ -f "${HEX}" ]]; then
    OUT="$(run_bench "${HEX}" --predictor ant || true)"
    if echo "$OUT" | grep -q '"cycles"'; then
      pass "RV hex program runs with --bench --json"
    else
      fail "hex benchmark produced no JSON output"
    fi
  else
    fail "hex fallback file not found: ${HEX}"
  fi
  ARM_HEX="${ROOT}/instruction_memory/instMem-forward-arm.txt"
  if [[ -f "${ARM_HEX}" ]]; then
    OUT="$(run_bench "${ARM_HEX}" --predictor ant || true)"
    if echo "$OUT" | grep -q '"cycles"'; then
      pass "ARM hex program runs with --bench --json (auto ISA from filename)"
    else
      fail "ARM hex benchmark produced no JSON output"
    fi
  fi
  echo
  echo "Summary: ${PASS} passed, ${FAIL} failed, ${SKIP} skipped"
  exit $(( FAIL > 0 ? 1 : 0 ))
fi

# --- Branch predictor comparison ---
echo "[4/5] Branch predictor comparison..."
if [[ "${RV_OK}" -eq 1 ]]; then
  COUNT_PRIMES="${BUILD}/count_primes.elf"
  ANT_JSON="$(run_bench "${COUNT_PRIMES}" --predictor ant)"
  GS_JSON="$(run_bench "${COUNT_PRIMES}" --predictor gshare)"
  ANT_ACC="$(extract_json_field "$ANT_JSON" branch_predictor_accuracy)"
  GS_ACC="$(extract_json_field "$GS_JSON" branch_predictor_accuracy)"
  if [[ -z "$ANT_ACC" || -z "$GS_ACC" ]]; then
    fail "RV: could not parse branch_predictor_accuracy from bench output"
  else
    echo "  [RV] Always Not Taken accuracy: ${ANT_ACC}%"
    echo "  [RV] GShare accuracy:           ${GS_ACC}%"
    if awk -v a="$ANT_ACC" -v g="$GS_ACC" 'BEGIN { exit (g > a) ? 0 : 1 }'; then
      pass "RV: GShare accuracy (${GS_ACC}%) > Always Not Taken (${ANT_ACC}%)"
    else
      fail "RV: expected GShare accuracy > Always Not Taken (got ${GS_ACC} vs ${ANT_ACC})"
    fi
  fi
fi
if [[ "${ARM_OK}" -eq 1 ]]; then
  COUNT_PRIMES_ARM="${BUILD}/count_primes_arm.elf"
  ANT_JSON="$(run_bench "${COUNT_PRIMES_ARM}" --predictor ant --max-cycles 1500000)"
  GS_JSON="$(run_bench "${COUNT_PRIMES_ARM}" --predictor gshare --max-cycles 1500000)"
  ANT_ACC="$(extract_json_field "$ANT_JSON" branch_predictor_accuracy)"
  GS_ACC="$(extract_json_field "$GS_JSON" branch_predictor_accuracy)"
  if [[ -z "$ANT_ACC" || -z "$GS_ACC" ]]; then
    fail "ARM: could not parse branch_predictor_accuracy from bench output"
  else
    echo "  [ARM] Always Not Taken accuracy: ${ANT_ACC}%"
    echo "  [ARM] GShare accuracy:           ${GS_ACC}%"
    if awk -v a="$ANT_ACC" -v g="$GS_ACC" 'BEGIN { exit (g > a) ? 0 : 1 }'; then
      pass "ARM: GShare accuracy (${GS_ACC}%) > Always Not Taken (${ANT_ACC}%)"
    else
      fail "ARM: expected GShare accuracy > Always Not Taken (got ${GS_ACC} vs ${ANT_ACC})"
    fi
  fi
fi
echo

# --- Cache comparison ---
echo "[5/5] Cache comparison..."
if [[ "${RV_OK}" -eq 1 ]]; then
  FIB_PRINT="${BUILD}/fib_print.elf"
  DIRECT_JSON="$(run_bench "${FIB_PRINT}" --cache direct)"
  WAY4_JSON="$(run_bench "${FIB_PRINT}" --cache 4way)"
  DIRECT_HR="$(extract_json_field "$DIRECT_JSON" cache_hit_rate)"
  WAY4_HR="$(extract_json_field "$WAY4_JSON" cache_hit_rate)"
  if [[ -z "$DIRECT_HR" || -z "$WAY4_HR" ]]; then
    fail "RV: could not parse cache_hit_rate from bench output"
  else
    echo "  [RV] Direct-mapped hit rate: ${DIRECT_HR}%"
    echo "  [RV] 4-way hit rate:         ${WAY4_HR}%"
    if awk -v d="$DIRECT_HR" -v w="$WAY4_HR" 'BEGIN { exit (w >= d) ? 0 : 1 }'; then
      pass "RV: 4-way hit rate (${WAY4_HR}%) >= direct-mapped (${DIRECT_HR}%)"
    else
      fail "RV: expected 4-way hit rate >= direct-mapped (got ${WAY4_HR} vs ${DIRECT_HR})"
    fi
  fi
fi
if [[ "${ARM_OK}" -eq 1 ]]; then
  FIB_PRINT_ARM="${BUILD}/fib_print_arm.elf"
  DIRECT_JSON="$(run_bench "${FIB_PRINT_ARM}" --cache direct)"
  WAY4_JSON="$(run_bench "${FIB_PRINT_ARM}" --cache 4way)"
  DIRECT_HR="$(extract_json_field "$DIRECT_JSON" cache_hit_rate)"
  WAY4_HR="$(extract_json_field "$WAY4_JSON" cache_hit_rate)"
  if [[ -z "$DIRECT_HR" || -z "$WAY4_HR" ]]; then
    fail "ARM: could not parse cache_hit_rate from bench output"
  else
    echo "  [ARM] Direct-mapped hit rate: ${DIRECT_HR}%"
    echo "  [ARM] 4-way hit rate:         ${WAY4_HR}%"
    if awk -v d="$DIRECT_HR" -v w="$WAY4_HR" 'BEGIN { exit (w >= d) ? 0 : 1 }'; then
      pass "ARM: 4-way hit rate (${WAY4_HR}%) >= direct-mapped (${DIRECT_HR}%)"
    else
      fail "ARM: expected 4-way hit rate >= direct-mapped (got ${WAY4_HR} vs ${DIRECT_HR})"
    fi
  fi
fi
echo

echo "=== Summary: ${PASS} passed, ${FAIL} failed, ${SKIP} skipped ==="
exit $(( FAIL > 0 ? 1 : 0 ))
