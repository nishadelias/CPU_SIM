#pragma once

#include "MemoryIf.h"
#include <cstdint>
#include <string>

struct ElfLoadResult {
    bool ok = false;
    std::string error;
    uint32_t entry = 0;
    uint32_t heap_brk = 0;  // program break after loaded segments
};

// Load a static RV32 ELF (little-endian) with PT_LOAD segments that fit in RAM [0, size).
// Expects linked for low RAM (linker script in scripts/ or examples/).
ElfLoadResult load_elf32_into_ram(const std::string& path, SimpleRAM& ram);
