#pragma once
#include "isa/IsaBackend.h"

class Riscv32Backend : public IsaBackend {
public:
  IsaKind kind() const override { return IsaKind::Riscv32; }
  uint16_t elf_machine() const override { return EM_RISCV; }
  int gpr_count() const override { return 32; }
  int sp_index() const override { return 2; }
  int zero_reg_index() const override { return 0; }
  int syscall_nr_reg() const override { return 17; }  // a7
  int syscall_arg0_reg() const override { return 10; } // a0
  int syscall_arg1_reg() const override { return 11; } // a1
  int syscall_arg2_reg() const override { return 12; } // a2
  int syscall_ret_reg() const override { return 10; }  // a0
  const char* abi_name(int i) const override;
  const char* name() const override { return "RV32"; }
};
