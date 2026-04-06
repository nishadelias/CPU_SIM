#pragma once

#include "MemoryIf.h"
#include <cstdint>
#include <string>

// Load legacy hex text (two hex chars per byte, whitespace-separated words) into RAM at `base`.
// Returns number of bytes written in `out_bytes`.
bool load_hex_text_file(const std::string& path, SimpleRAM& ram, uint32_t base, uint32_t& out_bytes);
