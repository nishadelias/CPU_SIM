#include "MMU.h"
#include "MemoryIf.h"

MMU::MMU() : ram_(nullptr), satp_(0) {}

MMU::MMU(SimpleRAM* ram) : ram_(ram), satp_(0) {}

bool MMU::translate(uint32_t vaddr, uint32_t& paddr, bool for_fetch, bool for_store, bool& page_fault) {
    page_fault = false;
    if (!paging_enabled()) {
        paddr = vaddr;
        return true;
    }
    if (!walk(vaddr, paddr, for_fetch, for_store)) {
        page_fault = true;
        return false;
    }
    return true;
}

bool MMU::walk(uint32_t vaddr, uint32_t& paddr, bool for_fetch, bool for_store) {
    if (!ram_) {
        return false;
    }

    const uint32_t vpn1 = (vaddr >> 22) & 0x3FFu;
    const uint32_t vpn0 = (vaddr >> 12) & 0x3FFu;
    const uint32_t ppnn = satp_ & 0xFFFFFu;
    const uint32_t root = ppnn << 12;

    auto read_pte = [&](uint32_t addr, uint32_t& pte) -> bool {
        MemResp r = ram_->load(addr, AccessSize::Word);
        if (!r.ok) {
            return false;
        }
        pte = r.data;
        return true;
    };

    uint32_t pte1 = 0;
    if (!read_pte(root + vpn1 * 4, pte1)) {
        return false;
    }
    if ((pte1 & 1u) == 0) {
        return false;
    }
    if ((pte1 & 0xE) == 0) {  // mega/super at level1 not supported
        return false;
    }

    const uint32_t ppn1 = pte1 >> 10;
    uint32_t pte0 = 0;
    if (!read_pte((ppn1 << 12) + vpn0 * 4, pte0)) {
        return false;
    }
    if ((pte0 & 1u) == 0) {
        return false;
    }

    const uint32_t ppn0 = pte0 >> 10;
    paddr = (ppn0 << 12) | (vaddr & 0xFFFu);

    if (for_fetch && (pte0 & 0x4u) == 0) {
        return false;
    }
    if (for_store && (pte0 & 0x8u) == 0) {
        return false;
    }
    if (!for_fetch && !for_store && (pte0 & 0x2u) == 0) {
        return false;
    }
    (void)for_fetch;
    return true;
}
