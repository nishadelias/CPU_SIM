#!/usr/bin/env bash
# Build examples/hello.elf for CPU_SIM (RV32, 64 KiB RAM @ 0).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/build/hello.elf"
mkdir -p "${ROOT}/build"

# Homebrew RISC-V toolchains are often outside non-interactive PATH.
export PATH="/opt/homebrew/bin:/usr/local/bin:${PATH}"

pick_gcc() {
  local candidates=(
    riscv32-unknown-elf-gcc
    riscv64-unknown-elf-gcc
    riscv64-elf-gcc
    /opt/homebrew/bin/riscv64-elf-gcc
    /usr/local/bin/riscv64-elf-gcc
  )
  local c
  for c in "${candidates[@]}"; do
    if command -v "${c}" >/dev/null 2>&1; then
      command -v "${c}"
      return
    fi
    if [[ -x "${c}" ]]; then
      echo "${c}"
      return
    fi
  done
  echo "No RISC-V embedded GCC found. Install one of:" >&2
  echo "  brew install riscv64-elf-gcc" >&2
  exit 1
}

GCC="$(pick_gcc)"
# rv32im (no C): C.JAL expansion in the simulator is wrong for crt0's call main.
ARCH_FLAGS=(-march=rv32im -mabi=ilp32)

"${GCC}" "${ARCH_FLAGS[@]}" -nostdlib -T "${ROOT}/examples/linker.ld" \
  "${ROOT}/examples/hello.c" -o "${OUT}"

echo "Built ${OUT}"
file "${OUT}" 2>/dev/null || true
