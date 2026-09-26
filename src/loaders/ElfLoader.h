#pragma once

#include "MemoryIf.h"
#include <cstdint>
#include <string>

struct ElfLoadResult {
    bool ok = false;
    std::string error;
    uint32_t entry = 0;
    uint32_t heap_brk = 0;  // program break after loaded segments
    uint16_t machine = 0;   // ELF e_machine (EM_RISCV or EM_ARM)
};

// Load a static ELF32 (little-endian) with PT_LOAD segments that fit in RAM [0, size).
// Accepts EM_RISCV or EM_ARM. Expects linked for low RAM (examples/linker*.ld).
ElfLoadResult load_elf32_into_ram(const std::string& path, SimpleRAM& ram);
