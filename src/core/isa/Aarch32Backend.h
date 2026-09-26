#pragma once
#include "isa/IsaBackend.h"
#include <cstdint>
#include <string>

struct ID_EX_Register; // defined in CPU.h; mapping implemented in .cpp

// ---- CPSR NZCV helpers ----
struct ArmCpsr {
  bool n = false, z = false, c = false, v = false;
  uint32_t to_bits() const;
  static ArmCpsr from_bits(uint32_t bits);
};

bool arm_condition_passed(uint8_t cond, const ArmCpsr& cpsr);

enum class ArmOpKind : uint8_t {
  DataProc,
  Multiply,
  LoadStore,
  BlockTransfer,  // LDM/STM (push/pop)
  Branch,
  BranchLink,
  Bx,
  Svc,
  Undefined
};

struct ArmDecoded {
  bool valid = false;
  uint8_t cond = 0xE;
  bool set_flags = false;
  ArmOpKind kind = ArmOpKind::Undefined;

  // Data-processing
  uint8_t opc = 0;       // 0-15
  uint8_t rn = 0;
  uint8_t rd = 0;
  uint8_t rm = 0;
  uint32_t imm_operand = 0;
  bool use_imm = false;
  uint8_t shift_type = 0;  // 0=LSL,1=LSR,2=ASR,3=ROR
  uint8_t shift_imm = 0;
  bool shift_by_reg = false;
  uint8_t rs = 0;

  // Multiply
  bool accumulate = false;

  // Load/store
  uint32_t imm12 = 0;
  uint8_t offset_reg = 0;
  bool add = true;       // U bit
  bool preindex = true;  // P bit
  bool writeback = false;
  bool load = false;
  bool byte = false;
  bool half = false;

  // Branch
  int32_t imm24 = 0; // already sign-extended byte displacement (imm24<<2)
  bool link = false;

  // SVC
  uint32_t svc_imm = 0;

  // LDM/STM
  uint16_t reglist = 0;

  // Pipeline-style control (mirrors ID_EX)
  bool regWrite = false;
  bool memRe = false;
  bool memWr = false;
  bool memToReg = false;
  bool branch = false;
  bool aluSrc = false;
  int aluOp = 0;
  int memReadType = 0;
  int memWriteType = 0;
  int32_t immediate = 0;
  unsigned int rs1 = 0;
  unsigned int rs2 = 0;
  unsigned int rd_pipe = 0;
  bool ecall = false;
};

class Aarch32Backend : public IsaBackend {
public:
  IsaKind kind() const override { return IsaKind::Aarch32; }
  uint16_t elf_machine() const override { return EM_ARM; }
  int gpr_count() const override { return 16; }
  int sp_index() const override { return 13; }
  int zero_reg_index() const override { return -1; }
  int syscall_nr_reg() const override { return 7; }   // r7
  int syscall_arg0_reg() const override { return 0; } // r0
  int syscall_arg1_reg() const override { return 1; } // r1
  int syscall_arg2_reg() const override { return 2; } // r2
  int syscall_ret_reg() const override { return 0; }  // r0
  const char* abi_name(int i) const override;
  const char* name() const override { return "AArch32"; }
};

bool aarch32_decode(uint32_t inst, ArmDecoded& out);
std::string aarch32_disassemble(uint32_t inst);

uint32_t aarch32_shift_operand(uint32_t val, int shift_type, uint32_t shift_amount,
                               bool& carry_out, bool carry_in);

int32_t aarch32_data_proc(uint8_t opc, uint32_t rn, uint32_t op2, bool& write_rd,
                          ArmCpsr& flags_inout, bool set_flags, bool carry_in);

bool aarch32_should_take_branch(uint8_t cond, const ArmCpsr& cpsr);

void aarch32_fill_id_ex(const ArmDecoded& d, ID_EX_Register& id_ex);
void aarch32_apply_decode(uint32_t inst, ID_EX_Register& id_ex, bool& ok);
