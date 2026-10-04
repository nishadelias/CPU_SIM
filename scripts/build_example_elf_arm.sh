#!/usr/bin/env bash
# Build AArch32 (ARMv7-A A32) example ELFs for CPU_SIM (64 KiB RAM @ 0).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
mkdir -p "${BUILD}"

export PATH="/opt/homebrew/bin:/usr/local/bin:${PATH}"

pick_gcc() {
  local candidates=(
    arm-none-eabi-gcc
    /opt/homebrew/bin/arm-none-eabi-gcc
    /usr/local/bin/arm-none-eabi-gcc
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
  echo "No ARM embedded GCC found. Install one of:" >&2
  echo "  brew install arm-none-eabi-gcc" >&2
  echo "  apt install gcc-arm-none-eabi" >&2
  exit 1
}

GCC="$(pick_gcc)"
# Force ARM state (A32), soft-float, freestanding — matches educational AArch32 backend.
ARCH_FLAGS=(-march=armv7-a -mfloat-abi=soft -marm -O0 -ffreestanding -fno-builtin -fno-pie)
LDFLAGS=(-nostdlib -T "${ROOT}/examples/linker_arm.ld" -Wl,-no-pie -lgcc)

build_one() {
  local src="$1"
  local out="$2"
  echo "  ${src} -> ${out}"
  "${GCC}" "${ARCH_FLAGS[@]}" -I "${ROOT}/examples" "${src}" \
    -nostdlib -T "${ROOT}/examples/linker_arm.ld" -Wl,-no-pie -o "${out}" -lgcc
}

echo "Using ${GCC}"
echo "Building ARM examples:"

build_one "${ROOT}/examples/hello_arm.c" "${BUILD}/hello_arm.elf"
build_one "${ROOT}/examples/fib_print_arm.c" "${BUILD}/fib_print_arm.elf"
build_one "${ROOT}/examples/count_primes_arm.c" "${BUILD}/count_primes_arm.elf"

echo "  call_ret (crt0_arm + main) -> ${BUILD}/call_ret_arm.elf"
"${GCC}" "${ARCH_FLAGS[@]}" -I "${ROOT}/examples" \
  "${ROOT}/examples/crt0_arm.S" "${ROOT}/examples/call_ret_main_arm.c" \
  -nostdlib -T "${ROOT}/examples/linker_arm.ld" -Wl,-no-pie \
  -o "${BUILD}/call_ret_arm.elf" -lgcc

if [[ -f "${ROOT}/examples/fp_demo_arm.c" ]]; then
  echo "  fp_demo_arm (soft-float stub) -> ${BUILD}/fp_demo_arm.elf"
  build_one "${ROOT}/examples/fp_demo_arm.c" "${BUILD}/fp_demo_arm.elf"
fi

echo "Done:"
file "${BUILD}/hello_arm.elf" "${BUILD}/fib_print_arm.elf" "${BUILD}/count_primes_arm.elf" "${BUILD}/call_ret_arm.elf" 2>/dev/null || true
