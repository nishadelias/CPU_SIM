#!/usr/bin/env bash
# Build cpusim, run unit tests and M-extension + ECALL smoke program.
# If riscv32-unknown-elf-gcc is installed, print its version (optional full program link is out of scope).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
mkdir -p "${BUILD}"
cmake -S "${ROOT}" -B "${BUILD}" >/dev/null
cmake --build "${BUILD}" -j
ctest --test-dir "${BUILD}" --output-on-failure
OUT="$("${BUILD}/cpusim" "${ROOT}/instruction_memory/instMem-m-ext.txt" 2>&1)" || true
echo "${OUT}"
echo "${OUT}" | grep -q "Syscall exit code: 42" || {
  echo "Expected 'Syscall exit code: 42' in cpusim output" >&2
  exit 1
}
echo "M-extension + ECALL integration smoke OK."
if command -v riscv32-unknown-elf-gcc >/dev/null 2>&1; then
  echo "riscv32-unknown-elf-gcc: $(riscv32-unknown-elf-gcc --version | head -1)"
elif command -v riscv64-unknown-elf-gcc >/dev/null 2>&1; then
  echo "riscv64-unknown-elf-gcc (try: -march=rv32imcf -mabi=ilp32 for RV32): $(riscv64-unknown-elf-gcc --version | head -1)"
else
  echo "No RISC-V embedded GCC in PATH; install riscv32-unknown-elf-gcc to compile C programs for this ISA."
fi
