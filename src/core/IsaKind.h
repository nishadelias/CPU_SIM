#pragma once
#include <cstdint>
enum class IsaKind : uint8_t { Riscv32 = 0, Aarch32 = 1 };
constexpr uint16_t EM_RISCV = 0xF3;
constexpr uint16_t EM_ARM = 40;
