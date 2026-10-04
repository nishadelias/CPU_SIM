#!/usr/bin/env bash
# Build cpusim, run unit tests, and smoke-test AArch32 hex + ELF programs.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
mkdir -p "${BUILD}"
cmake -S "${ROOT}" -B "${BUILD}" >/dev/null
cmake --build "${BUILD}" -j
ctest --test-dir "${BUILD}" --output-on-failure

OUT="$("${BUILD}/cpusim" "${ROOT}/instruction_memory/instMem-svc-exit-arm.txt" 2>&1)" || true
echo "${OUT}"
echo "${OUT}" | grep -q "Syscall exit code: 42" || {
  echo "Expected 'Syscall exit code: 42' from instMem-svc-exit-arm.txt" >&2
  exit 1
}
echo "AArch32 SVC hex integration smoke OK."

OUT="$("${BUILD}/cpusim" "${ROOT}/instruction_memory/instMem-mul-svc-arm.txt" 2>&1)" || true
echo "${OUT}"
echo "${OUT}" | grep -q "Syscall exit code: 42" || {
  echo "Expected 'Syscall exit code: 42' from instMem-mul-svc-arm.txt" >&2
  exit 1
}
echo "AArch32 MUL+SVC hex integration smoke OK."

export PATH="/opt/homebrew/bin:/usr/local/bin:${PATH}"
if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  echo "arm-none-eabi-gcc: $(arm-none-eabi-gcc --version | head -1)"
  if [[ -x "${ROOT}/scripts/build_example_elf_arm.sh" ]]; then
    "${ROOT}/scripts/build_example_elf_arm.sh"
    HELLO_OUT="$("${BUILD}/cpusim" "${BUILD}/hello_arm.elf" --max-cycles 50000 2>&1)" || true
    echo "${HELLO_OUT}"
    echo "${HELLO_OUT}" | grep -q "Syscall exit code: 42" || {
      echo "Expected 'Syscall exit code: 42' from build/hello_arm.elf" >&2
      exit 1
    }
    echo "ELF hello_arm.elf integration smoke OK."
  fi
else
  echo "No ARM embedded GCC in PATH; install arm-none-eabi-gcc to compile C programs."
fi
