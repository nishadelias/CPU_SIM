#!/usr/bin/env bash
# Build RV32 example ELFs for CPU_SIM (64 KiB RAM @ 0).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
mkdir -p "${BUILD}"

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
# rv32im (no C): avoids known compressed-JAL issues in the simulator.
ARCH_FLAGS=(-march=rv32im -mabi=ilp32 -O0 -ffreestanding -fno-builtin -fno-pie -mno-relax)
LDFLAGS=(-nostdlib -T "${ROOT}/examples/linker.ld" -Wl,-no-pie)

build_one() {
  local src="$1"
  local out="$2"
  echo "  ${src} -> ${out}"
  "${GCC}" "${ARCH_FLAGS[@]}" "${LDFLAGS[@]}" "${src}" -o "${out}"
}

echo "Using ${GCC}"
echo "Building examples:"

build_one "${ROOT}/examples/hello.c" "${BUILD}/hello.elf"
build_one "${ROOT}/examples/fib_print.c" "${BUILD}/fib_print.elf"
build_one "${ROOT}/examples/count_primes.c" "${BUILD}/count_primes.elf"

echo "  call_ret (crt0 + main) -> ${BUILD}/call_ret.elf"
"${GCC}" "${ARCH_FLAGS[@]}" "${LDFLAGS[@]}" \
  "${ROOT}/examples/crt0.S" "${ROOT}/examples/call_ret_main.c" \
  -I "${ROOT}/examples" -o "${BUILD}/call_ret.elf"

ARCH_AFC=(-march=rv32imafc -mabi=ilp32 -O0 -ffreestanding -fno-builtin -fno-pie -mno-relax)
if [[ -f "${ROOT}/examples/fp_demo.c" ]]; then
  echo "  fp_demo -> ${BUILD}/fp_demo.elf"
  "${GCC}" "${ARCH_AFC[@]}" "${LDFLAGS[@]}" "${ROOT}/examples/fp_demo.c" \
    -I "${ROOT}/examples" -o "${BUILD}/fp_demo.elf"
fi

echo "Done:"
file "${BUILD}/hello.elf" "${BUILD}/fib_print.elf" "${BUILD}/count_primes.elf" "${BUILD}/call_ret.elf" 2>/dev/null || true
