#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "MemoryMap.h"
#include "MMU.h"
#include "test_common.h"

#include <cstdio>

int main() {
    SimpleRAM dram(MemoryMap::RAM_SIZE);
    MMU mmu(&dram);
    mmu.set_satp(0);  // bare

    uint32_t pa = 0;
    bool pf = false;
    if (!mmu.translate(0x1000, pa, false, false, pf) || pa != 0x1000) {
        std::fprintf(stderr, "bare translate failed\n");
        return 1;
    }

    // Identity map one page: root PTE at 0x8000
    const uint32_t root = 0x8000;
    const uint32_t leaf = 0x9000;
    // PTE for VPN1: points to leaf table
    dram.store(root, (0x9u << 10) | 0xF, AccessSize::Word);  // ppn=0x9 -> table at 0x9000
    // PTE for VPN0: maps VA 0x00000000 -> PA 0x00000000
    dram.store(leaf, (0x00u << 10) | 0xF, AccessSize::Word);

    const uint32_t satp = (1u << 31) | (root >> 12);
    mmu.set_satp(satp);

    if (!mmu.translate(0x40, pa, false, false, pf) || pa != 0x40) {
        std::fprintf(stderr, "paged translate failed pa=%x pf=%d\n", pa, pf);
        return 2;
    }

    std::printf("mmu_test ok\n");
    return 0;
}
