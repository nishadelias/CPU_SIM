#include "HexLoader.h"
#include "MemoryMap.h"

#include <fstream>
#include <sstream>
#include <string>

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

bool load_hex_text_file(const std::string& path, SimpleRAM& ram, uint32_t base, uint32_t& out_bytes) {
    out_bytes = 0;
    std::ifstream infile(path);
    if (!infile) {
        return false;
    }

    std::string line;
    uint32_t addr = 0;
    while (infile >> line) {
        if (line.size() < 2) {
            continue;
        }
        int hi = hex_digit(line[0]);
        int lo = hex_digit(line[1]);
        if (hi < 0 || lo < 0) {
            continue;
        }
        uint8_t b = static_cast<uint8_t>((hi << 4) | lo);
        if (base + addr >= MemoryMap::RAM_SIZE) {
            return false;
        }
        uint8_t buf[1] = {b};
        if (!ram.poke_bytes(base + addr, buf, 1)) {
            return false;
        }
        addr++;
    }
    out_bytes = addr;
    return true;
}
