#pragma once
#include "IsaKind.h"
#include <cstdint>
#include <string>

class IsaBackend {
public:
  virtual ~IsaBackend() = default;
  virtual IsaKind kind() const = 0;
  virtual uint16_t elf_machine() const = 0;
  virtual int gpr_count() const = 0;
  virtual int sp_index() const = 0;
  virtual int zero_reg_index() const = 0; // -1 if none (ARM), 0 for RV
  virtual int syscall_nr_reg() const = 0;
  virtual int syscall_arg0_reg() const = 0;
  virtual int syscall_arg1_reg() const = 0;
  virtual int syscall_arg2_reg() const = 0;
  virtual int syscall_ret_reg() const = 0;
  virtual const char* abi_name(int i) const = 0;
  virtual const char* name() const = 0;
};
