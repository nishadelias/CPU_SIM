#pragma once

#include <cstdint>

class SimpleRAM;

// Sv32 subset: bare mode or page-table walk for RV32.
class MMU {
public:
  MMU();
  explicit MMU(SimpleRAM* ram);

  void set_ram(SimpleRAM* ram) { ram_ = ram; }
  void set_satp(uint32_t satp) { satp_ = satp; }
  uint32_t satp() const { return satp_; }
  bool paging_enabled() const { return (satp_ >> 31) != 0; }

  bool translate(uint32_t vaddr, uint32_t& paddr, bool for_fetch, bool for_store, bool& page_fault);

private:
  SimpleRAM* ram_;
  uint32_t satp_;

  bool walk(uint32_t vaddr, uint32_t& paddr, bool for_fetch, bool for_store);
};
