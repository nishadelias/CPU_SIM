#include "isa/Riscv32Backend.h"

namespace {
// Matches CPU::REGISTER_NAMES
const char* kRvAbiNames[32] = {
    "Zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};
} // namespace

const char* Riscv32Backend::abi_name(int i) const {
  if (i < 0 || i >= 32) return "?";
  return kRvAbiNames[i];
}
