// Fixed execution environment for RV32IMC user-mode simulation.
#pragma once

#include <cstdint>

namespace MemoryMap {

constexpr uint32_t RAM_BASE = 0;
constexpr uint32_t RAM_SIZE = 64 * 1024;  // Must match SimpleRAM in cpusim / GUI

// Hex teaching programs load at byte 0 (legacy behavior).
constexpr uint32_t HEX_PROGRAM_BASE = 0;

// Initial stack pointer (16-byte aligned, grows down).
constexpr uint32_t STACK_TOP = RAM_BASE + RAM_SIZE;

// Default heap break after ELF load is computed from segments; initial brk before _start.
constexpr uint32_t INITIAL_BRK = 0;

}  // namespace MemoryMap
