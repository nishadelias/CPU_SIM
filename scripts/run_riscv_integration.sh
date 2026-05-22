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
export PATH="/opt/homebrew/bin:/usr/local/bin:${PATH}"
if command -v riscv32-unknown-elf-gcc >/dev/null 2>&1; then
  echo "riscv32-unknown-elf-gcc: $(riscv32-unknown-elf-gcc --version | head -1)"
elif command -v riscv64-elf-gcc >/dev/null 2>&1; then
  echo "riscv64-elf-gcc: $(riscv64-elf-gcc --version | head -1)"
  if [[ -x "${ROOT}/scripts/build_example_elf.sh" ]]; then
    "${ROOT}/scripts/build_example_elf.sh"
    HELLO_OUT="$("${BUILD}/cpusim" "${BUILD}/hello.elf" --max-cycles 50000 2>&1)" || true
    echo "${HELLO_OUT}"
    echo "${HELLO_OUT}" | grep -q "Syscall exit code: 42" || {
      echo "Expected 'Syscall exit code: 42' from build/hello.elf" >&2
      exit 1
    }
    echo "ELF hello.elf integration smoke OK."
  fi
else
  echo "No RISC-V embedded GCC in PATH; install riscv64-elf-gcc (brew install riscv64-elf-gcc) to compile C programs."
fi
