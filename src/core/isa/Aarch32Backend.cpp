#include "isa/Aarch32Backend.h"
#include "CPU.h"

#include <cstdio>
#include <sstream>

namespace {

const char* kArmAbiNames[16] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
};

const char* kCondSuffix[16] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "", "nv"
};

const char* kDpMnemonic[16] = {
    "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
    "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"
};

const char* kShiftName[4] = { "lsl", "lsr", "asr", "ror" };

inline uint32_t bits(uint32_t v, int hi, int lo) {
  return (v >> lo) & ((1u << (hi - lo + 1)) - 1u);
}

inline int32_t sign_extend(uint32_t v, int width) {
  const uint32_t sign = 1u << (width - 1);
  return static_cast<int32_t>((v ^ sign) - sign);
}

bool is_test_op(uint8_t opc) {
  return opc >= 8 && opc <= 11; // TST, TEQ, CMP, CMN
}

void reset_control(ArmDecoded& d) {
  d.regWrite = false;
  d.memRe = false;
  d.memWr = false;
  d.memToReg = false;
  d.branch = false;
  d.aluSrc = false;
  d.aluOp = 0;
  d.memReadType = 0;
  d.memWriteType = 0;
  d.immediate = 0;
  d.rs1 = 0;
  d.rs2 = 0;
  d.rd_pipe = 0;
  d.ecall = false;
}

void fill_dataproc_control(ArmDecoded& d) {
  reset_control(d);
  d.rs1 = d.rn;
  d.rs2 = d.rm;
  d.rd_pipe = d.rd;
  d.aluSrc = d.use_imm;
  d.immediate = static_cast<int32_t>(d.imm_operand);
  d.aluOp = d.opc;
  d.regWrite = !is_test_op(d.opc);
}

void fill_mul_control(ArmDecoded& d) {
  reset_control(d);
  d.regWrite = true;
  d.rd_pipe = d.rd;
  // Rm * Rs [+ Rn]; expose Rm as rs1, Rs as rs2, accumulate Rn via rn field
  d.rs1 = d.rm;
  d.rs2 = d.rs;
  d.aluOp = d.accumulate ? 1 : 0;
}

void fill_ls_control(ArmDecoded& d) {
  reset_control(d);
  d.rs1 = d.rn;
  d.rd_pipe = d.rd;
  d.aluSrc = true;
  // Signed offset from U bit
  int32_t off = static_cast<int32_t>(d.imm12);
  if (!d.add) off = -off;
  d.immediate = off;

  if (d.load) {
    d.memRe = true;
    d.memToReg = true;
    d.regWrite = true;
    if (d.byte) {
      d.memReadType = 2; // LBU
    } else if (d.half) {
      d.memReadType = 4; // LHU
    } else {
      d.memReadType = 5; // LW
    }
  } else {
    d.memWr = true;
    d.rs2 = d.rd; // store data in Rd
    if (d.byte) {
      d.memWriteType = 1; // SB
    } else if (d.half) {
      d.memWriteType = 2; // SH
    } else {
      d.memWriteType = 3; // SW
    }
  }
}

void fill_branch_control(ArmDecoded& d, bool link) {
  reset_control(d);
  d.branch = true;
  d.link = link;
  d.immediate = d.imm24;
  if (link) {
    d.regWrite = true;
    d.rd_pipe = 14; // LR
  }
}

void fill_bx_control(ArmDecoded& d) {
  reset_control(d);
  d.branch = true;
  d.rs1 = d.rm;
  d.rs2 = d.rm;
}

void fill_svc_control(ArmDecoded& d) {
  reset_control(d);
  d.ecall = true;
  d.immediate = static_cast<int32_t>(d.svc_imm);
}

std::string cond_sfx(uint8_t cond) {
  if (cond >= 16) return "";
  if (cond == 0xE) return "";
  return kCondSuffix[cond];
}

std::string reg_name(unsigned r) {
  if (r < 16) return kArmAbiNames[r];
  return "r?";
}

std::string hex_imm(uint32_t v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "#0x%x", v);
  return buf;
}

std::string format_shifter(const ArmDecoded& d) {
  if (d.use_imm) {
    return hex_imm(d.imm_operand);
  }
  std::string s = reg_name(d.rm);
  if (d.shift_by_reg) {
    s += ", ";
    s += kShiftName[d.shift_type & 3];
    s += " ";
    s += reg_name(d.rs);
  } else if (d.shift_imm != 0 || d.shift_type != 0) {
    // ROR #0 is RRX
    if (d.shift_type == 3 && d.shift_imm == 0) {
      s += ", rrx";
    } else {
      uint32_t amt = d.shift_imm;
      if ((d.shift_type == 1 || d.shift_type == 2) && amt == 0) amt = 32;
      s += ", ";
      s += kShiftName[d.shift_type & 3];
      s += " #";
      s += std::to_string(amt);
    }
  }
  return s;
}

} // namespace

// ---- Aarch32Backend ----

const char* Aarch32Backend::abi_name(int i) const {
  if (i < 0 || i >= 16) return "?";
  return kArmAbiNames[i];
}

// ---- CPSR ----

uint32_t ArmCpsr::to_bits() const {
  uint32_t b = 0;
  if (n) b |= (1u << 31);
  if (z) b |= (1u << 30);
  if (c) b |= (1u << 29);
  if (v) b |= (1u << 28);
  return b;
}

ArmCpsr ArmCpsr::from_bits(uint32_t bits_in) {
  ArmCpsr f;
  f.n = (bits_in >> 31) & 1;
  f.z = (bits_in >> 30) & 1;
  f.c = (bits_in >> 29) & 1;
  f.v = (bits_in >> 28) & 1;
  return f;
}

bool arm_condition_passed(uint8_t cond, const ArmCpsr& cpsr) {
  switch (cond & 0xF) {
    case 0x0: return cpsr.z;                         // EQ
    case 0x1: return !cpsr.z;                        // NE
    case 0x2: return cpsr.c;                         // CS/HS
    case 0x3: return !cpsr.c;                        // CC/LO
    case 0x4: return cpsr.n;                         // MI
    case 0x5: return !cpsr.n;                        // PL
    case 0x6: return cpsr.v;                         // VS
    case 0x7: return !cpsr.v;                        // VC
    case 0x8: return cpsr.c && !cpsr.z;              // HI
    case 0x9: return !cpsr.c || cpsr.z;              // LS
    case 0xA: return cpsr.n == cpsr.v;               // GE
    case 0xB: return cpsr.n != cpsr.v;               // LT
    case 0xC: return !cpsr.z && (cpsr.n == cpsr.v);  // GT
    case 0xD: return cpsr.z || (cpsr.n != cpsr.v);   // LE
    case 0xE: return true;                           // AL
    case 0xF: return false;                          // NV (educational: never)
    default:  return false;
  }
}

bool aarch32_should_take_branch(uint8_t cond, const ArmCpsr& cpsr) {
  return arm_condition_passed(cond, cpsr);
}

// ---- Barrel shifter ----

uint32_t aarch32_shift_operand(uint32_t val, int shift_type, uint32_t shift_amount,
                               bool& carry_out, bool carry_in) {
  shift_type &= 3;
  carry_out = carry_in;

  // Immediate-encoding convention: ROR #0 means RRX.
  if (shift_type == 3 && shift_amount == 0) {
    carry_out = val & 1u;
    return (val >> 1) | (carry_in ? 0x80000000u : 0u);
  }

  if (shift_amount == 0) {
    carry_out = carry_in;
    return val;
  }

  switch (shift_type) {
    case 0: { // LSL
      if (shift_amount >= 32) {
        // LSL #32: carry = Rm[0]; LSL >32: carry = 0
        carry_out = (shift_amount == 32) ? (val & 1u) : false;
        return 0;
      }
      carry_out = (val >> (32 - shift_amount)) & 1u;
      return val << shift_amount;
    }
    case 1: { // LSR
      if (shift_amount >= 32) {
        carry_out = (shift_amount == 32) ? ((val >> 31) & 1u) : false;
        return 0;
      }
      carry_out = (val >> (shift_amount - 1)) & 1u;
      return val >> shift_amount;
    }
    case 2: { // ASR
      int32_t sval = static_cast<int32_t>(val);
      if (shift_amount >= 32) {
        carry_out = (val >> 31) & 1u;
        return static_cast<uint32_t>(sval >> 31);
      }
      carry_out = (val >> (shift_amount - 1)) & 1u;
      return static_cast<uint32_t>(sval >> shift_amount);
    }
    case 3: { // ROR
      uint32_t amt = shift_amount & 31u;
      if (amt == 0) {
        // ROR by multiple of 32: result = val, carry = bit 31
        carry_out = (val >> 31) & 1u;
        return val;
      }
      carry_out = (val >> (amt - 1)) & 1u;
      return (val >> amt) | (val << (32 - amt));
    }
    default:
      return val;
  }
}

// ---- Data processing ----

int32_t aarch32_data_proc(uint8_t opc, uint32_t rn, uint32_t op2, bool& write_rd,
                          ArmCpsr& flags_inout, bool set_flags, bool carry_in) {
  opc &= 0xF;
  write_rd = !is_test_op(opc);

  uint64_t result64 = 0;
  uint32_t result = 0;
  bool shifter_carry = carry_in; // for logical ops C comes from shifter (passed as carry_in here as shifter C)
  // Note: caller passes the shifter carry-out as carry_in for flag updates on logical ops.
  // For ADC/SBC/RSC, carry_in is the CPSR.C bit.

  auto set_nz = [&](uint32_t r) {
    flags_inout.n = (r >> 31) & 1;
    flags_inout.z = (r == 0);
  };

  auto add_with_carry = [&](uint32_t a, uint32_t b, uint32_t cin) -> uint32_t {
    result64 = static_cast<uint64_t>(a) + static_cast<uint64_t>(b) + cin;
    result = static_cast<uint32_t>(result64);
    if (set_flags) {
      set_nz(result);
      flags_inout.c = (result64 >> 32) & 1;
      // V: signed overflow
      bool sa = (a >> 31) & 1;
      bool sb = (b >> 31) & 1;
      bool sr = (result >> 31) & 1;
      flags_inout.v = (sa == sb) && (sr != sa);
    }
    return result;
  };

  switch (opc) {
    case 0x0: // AND
      result = rn & op2;
      if (set_flags) { set_nz(result); flags_inout.c = shifter_carry; }
      break;
    case 0x1: // EOR
      result = rn ^ op2;
      if (set_flags) { set_nz(result); flags_inout.c = shifter_carry; }
      break;
    case 0x2: // SUB
      result = add_with_carry(rn, ~op2, 1);
      break;
    case 0x3: // RSB
      result = add_with_carry(op2, ~rn, 1);
      break;
    case 0x4: // ADD
      result = add_with_carry(rn, op2, 0);
      break;
    case 0x5: // ADC
      result = add_with_carry(rn, op2, carry_in ? 1u : 0u);
      break;
    case 0x6: // SBC
      result = add_with_carry(rn, ~op2, carry_in ? 1u : 0u);
      break;
    case 0x7: // RSC
      result = add_with_carry(op2, ~rn, carry_in ? 1u : 0u);
      break;
    case 0x8: // TST
      result = rn & op2;
      if (set_flags) { set_nz(result); flags_inout.c = shifter_carry; }
      break;
    case 0x9: // TEQ
      result = rn ^ op2;
      if (set_flags) { set_nz(result); flags_inout.c = shifter_carry; }
      break;
    case 0xA: // CMP
      result = add_with_carry(rn, ~op2, 1);
      break;
    case 0xB: // CMN
      result = add_with_carry(rn, op2, 0);
      break;
    case 0xC: // ORR
      result = rn | op2;
      if (set_flags) { set_nz(result); flags_inout.c = shifter_carry; }
      break;
    case 0xD: // MOV
      result = op2;
      if (set_flags) { set_nz(result); flags_inout.c = shifter_carry; }
      break;
    case 0xE: // BIC
      result = rn & ~op2;
      if (set_flags) { set_nz(result); flags_inout.c = shifter_carry; }
      break;
    case 0xF: // MVN
      result = ~op2;
      if (set_flags) { set_nz(result); flags_inout.c = shifter_carry; }
      break;
    default:
      break;
  }

  return static_cast<int32_t>(result);
}

// ---- Decode ----

bool aarch32_decode(uint32_t inst, ArmDecoded& out) {
  out = ArmDecoded{};
  const uint8_t cond = static_cast<uint8_t>(bits(inst, 31, 28));
  out.cond = cond;

  // Educational mode: cond==0xF (NV) → invalid / NOP
  if (cond == 0xF) {
    out.valid = false;
    out.kind = ArmOpKind::Undefined;
    return false;
  }

  const uint32_t op = bits(inst, 27, 25);

  // ---- Hint / NOP space: bits[27:20]=0x32, Rn=0, Rd=0xF ----
  if (bits(inst, 27, 20) == 0x32 && bits(inst, 19, 16) == 0 && bits(inst, 15, 12) == 0xF) {
    out.valid = true;
    out.kind = ArmOpKind::DataProc;
    out.opc = 0xD; // treat as MOV with no write
    out.set_flags = false;
    reset_control(out);
    out.regWrite = false;
    return true;
  }

  // ---- SVC: bits[27:24] == 1111 ----
  if (bits(inst, 27, 24) == 0xF) {
    out.valid = true;
    out.kind = ArmOpKind::Svc;
    out.svc_imm = bits(inst, 23, 0);
    fill_svc_control(out);
    return true;
  }

  // ---- Branch: bits[27:25] == 101 ----
  if (op == 0x5) {
    const bool link = bits(inst, 24, 24) != 0;
    out.valid = true;
    out.kind = link ? ArmOpKind::BranchLink : ArmOpKind::Branch;
    out.link = link;
    out.imm24 = sign_extend(bits(inst, 23, 0) << 2, 26);
    fill_branch_control(out, link);
    return true;
  }

  // ---- LDM/STM: bits[27:25] == 100 ----
  if (op == 0x4) {
    out.valid = true;
    out.kind = ArmOpKind::BlockTransfer;
    out.preindex = bits(inst, 24, 24) != 0;  // P
    out.add = bits(inst, 23, 23) != 0;       // U
    out.writeback = bits(inst, 21, 21) != 0; // W
    out.load = bits(inst, 20, 20) != 0;      // L
    out.rn = static_cast<uint8_t>(bits(inst, 19, 16));
    out.reglist = static_cast<uint16_t>(bits(inst, 15, 0));
    reset_control(out);
    out.rs1 = out.rn;
    out.rd_pipe = out.rn; // writeback base
    out.regWrite = out.writeback;
    out.memRe = out.load;
    out.memWr = !out.load;
    out.immediate = static_cast<int32_t>(out.reglist);
    out.aluSrc = true;
    return true;
  }

  // ---- Load/store word/byte: bits[27:26] == 01 (imm or reg offset) ----
  if (bits(inst, 27, 26) == 0x1) {
    const bool reg_off = bits(inst, 25, 25) != 0; // I bit
    out.valid = true;
    out.kind = ArmOpKind::LoadStore;
    out.preindex = bits(inst, 24, 24) != 0;
    out.add = bits(inst, 23, 23) != 0;
    out.byte = bits(inst, 22, 22) != 0;
    out.half = false;
    out.writeback = bits(inst, 21, 21) != 0;
    out.load = bits(inst, 20, 20) != 0;
    out.rn = static_cast<uint8_t>(bits(inst, 19, 16));
    out.rd = static_cast<uint8_t>(bits(inst, 15, 12));
    if (reg_off) {
      // Register offset, shift by imm only (educational)
      if (bits(inst, 4, 4) != 0) {
        out.valid = false;
        out.kind = ArmOpKind::Undefined;
        return false;
      }
      out.use_imm = false;
      out.rm = static_cast<uint8_t>(bits(inst, 3, 0));
      out.shift_type = static_cast<uint8_t>(bits(inst, 6, 5));
      out.shift_imm = static_cast<uint8_t>(bits(inst, 11, 7));
      out.imm12 = 0;
    } else {
      out.use_imm = true;
      out.imm12 = bits(inst, 11, 0);
      out.rm = 0;
    }
    if (!out.preindex) out.writeback = true;
    fill_ls_control(out);
    if (reg_off) {
      out.aluSrc = false; // offset from rm (shifted)
      out.rs2 = out.rm;
      out.immediate = 0;
    }
    return true;
  }

  // ---- BX: cond + 000100101111111111110001 + Rm ----
  if ((inst & 0x0FFFFFF0u) == 0x012FFF10u) {
    out.valid = true;
    out.kind = ArmOpKind::Bx;
    out.rm = static_cast<uint8_t>(bits(inst, 3, 0));
    fill_bx_control(out);
    return true;
  }

  // ---- Halfword load/store immediate: bits[27:25]=000, bit22=1, bit7=1, bit4=1, bits[6:5]=01 ----
  if (bits(inst, 27, 25) == 0 &&
      bits(inst, 22, 22) == 1 &&
      bits(inst, 7, 7) == 1 &&
      bits(inst, 4, 4) == 1 &&
      bits(inst, 6, 5) == 0x1) {
    out.valid = true;
    out.kind = ArmOpKind::LoadStore;
    out.preindex = bits(inst, 24, 24) != 0;
    out.add = bits(inst, 23, 23) != 0;
    out.writeback = bits(inst, 21, 21) != 0;
    out.load = bits(inst, 20, 20) != 0;
    out.rn = static_cast<uint8_t>(bits(inst, 19, 16));
    out.rd = static_cast<uint8_t>(bits(inst, 15, 12));
    out.imm12 = (bits(inst, 11, 8) << 4) | bits(inst, 3, 0);
    out.byte = false;
    out.half = true;
    if (!out.preindex) out.writeback = true;
    fill_ls_control(out);
    return true;
  }

  // ---- Multiply MUL/MLA: bits[27:22]=000000, bits[7:4]=1001 ----
  if (bits(inst, 27, 22) == 0 && bits(inst, 7, 4) == 0x9) {
    out.valid = true;
    out.kind = ArmOpKind::Multiply;
    out.accumulate = bits(inst, 21, 21) != 0;
    out.set_flags = bits(inst, 20, 20) != 0;
    // Rd bits[19:16], Rn bits[15:12] (acc), Rs bits[11:8], Rm bits[3:0]
    out.rd = static_cast<uint8_t>(bits(inst, 19, 16));
    out.rn = static_cast<uint8_t>(bits(inst, 15, 12));
    out.rs = static_cast<uint8_t>(bits(inst, 11, 8));
    out.rm = static_cast<uint8_t>(bits(inst, 3, 0));
    fill_mul_control(out);
    return true;
  }

  // ---- MOVW / MOVT (ARMv7): bits[27:20] == 0x30 / 0x34 ----
  if (bits(inst, 27, 20) == 0x30 || bits(inst, 27, 20) == 0x34) {
    const bool top = bits(inst, 27, 20) == 0x34;
    out.valid = true;
    out.kind = ArmOpKind::DataProc;
    out.opc = top ? 0xE /*BIC placeholder*/ : 0xD; // mark via set_flags unused + use funct
    out.set_flags = false;
    out.rd = static_cast<uint8_t>(bits(inst, 15, 12));
    out.rn = out.rd; // MOVT reads Rd
    const uint32_t imm16 = (bits(inst, 19, 16) << 12) | bits(inst, 11, 0);
    out.use_imm = true;
    out.imm_operand = top ? (imm16 << 16) : imm16;
    out.shift_type = top ? 1 : 0; // reuse: 0=MOVW, 1=MOVT
    reset_control(out);
    out.regWrite = true;
    out.rd_pipe = out.rd;
    out.rs1 = out.rd;
    out.aluSrc = true;
    out.immediate = static_cast<int32_t>(out.imm_operand);
    out.aluOp = top ? 0x101 : 0x100; // wide-move markers for EX
    return true;
  }

  // ---- Data-processing: bits[27:26] == 00 (not multiply / not extra LS) ----
  if (bits(inst, 27, 26) == 0) {
    // Exclude remaining extra load/store (bit4=1, bit7=1)
    if (bits(inst, 25, 25) == 0 && bits(inst, 7, 7) == 1 && bits(inst, 4, 4) == 1) {
      out.valid = false;
      out.kind = ArmOpKind::Undefined;
      return false;
    }

    out.valid = true;
    out.kind = ArmOpKind::DataProc;
    out.use_imm = bits(inst, 25, 25) != 0;
    out.opc = static_cast<uint8_t>(bits(inst, 24, 21));
    out.set_flags = bits(inst, 20, 20) != 0;
    out.rn = static_cast<uint8_t>(bits(inst, 19, 16));
    out.rd = static_cast<uint8_t>(bits(inst, 15, 12));

    if (out.use_imm) {
      const uint32_t imm8 = bits(inst, 7, 0);
      const uint32_t rot = bits(inst, 11, 8);
      // Rotate right by 2*rot
      if (rot == 0) {
        out.imm_operand = imm8;
      } else {
        const uint32_t r = rot * 2;
        out.imm_operand = (imm8 >> r) | (imm8 << (32 - r));
      }
      out.shift_type = 0;
      out.shift_imm = 0;
      out.shift_by_reg = false;
      out.rm = 0;
    } else {
      out.rm = static_cast<uint8_t>(bits(inst, 3, 0));
      out.shift_type = static_cast<uint8_t>(bits(inst, 6, 5));
      out.shift_by_reg = bits(inst, 4, 4) != 0;
      if (out.shift_by_reg) {
        out.rs = static_cast<uint8_t>(bits(inst, 11, 8));
        out.shift_imm = 0;
      } else {
        out.shift_imm = static_cast<uint8_t>(bits(inst, 11, 7));
        out.rs = 0;
      }
      out.imm_operand = 0;
    }

    fill_dataproc_control(out);
    return true;
  }

  out.valid = false;
  out.kind = ArmOpKind::Undefined;
  return false;
}

// ---- Disassemble ----

std::string aarch32_disassemble(uint32_t inst) {
  ArmDecoded d;
  if (!aarch32_decode(inst, d)) {
    char buf[40];
    std::snprintf(buf, sizeof(buf), ".word 0x%08x", inst);
    return buf;
  }

  const std::string cs = cond_sfx(d.cond);
  std::ostringstream oss;

  switch (d.kind) {
    case ArmOpKind::DataProc: {
      if (d.aluOp == 0x100) {
        oss << "movw" << cs << " " << reg_name(d.rd) << ", " << hex_imm(d.imm_operand & 0xFFFFu);
        break;
      }
      if (d.aluOp == 0x101) {
        oss << "movt" << cs << " " << reg_name(d.rd) << ", " << hex_imm((d.imm_operand >> 16) & 0xFFFFu);
        break;
      }
      const char* mnem = kDpMnemonic[d.opc & 0xF];
      oss << mnem << cs;
      if (d.set_flags && !is_test_op(d.opc)) oss << "s";
      oss << " ";
      if (is_test_op(d.opc)) {
        oss << reg_name(d.rn) << ", " << format_shifter(d);
      } else if (d.opc == 0xD || d.opc == 0xF) { // MOV / MVN
        oss << reg_name(d.rd) << ", " << format_shifter(d);
      } else {
        oss << reg_name(d.rd) << ", " << reg_name(d.rn) << ", " << format_shifter(d);
      }
      break;
    }
    case ArmOpKind::Multiply: {
      if (d.accumulate) {
        oss << "mla" << cs;
        if (d.set_flags) oss << "s";
        oss << " " << reg_name(d.rd) << ", " << reg_name(d.rm)
            << ", " << reg_name(d.rs) << ", " << reg_name(d.rn);
      } else {
        oss << "mul" << cs;
        if (d.set_flags) oss << "s";
        oss << " " << reg_name(d.rd) << ", " << reg_name(d.rm)
            << ", " << reg_name(d.rs);
      }
      break;
    }
    case ArmOpKind::BlockTransfer: {
      oss << (d.load ? "ldm" : "stm") << cs;
      // Simplified addressing hint
      if (d.preindex && !d.add) oss << "db";
      else if (!d.preindex && d.add) oss << "ia";
      else if (d.preindex && d.add) oss << "ib";
      else oss << "da";
      oss << " " << reg_name(d.rn);
      if (d.writeback) oss << "!";
      oss << ", {";
      bool first = true;
      for (int i = 0; i < 16; ++i) {
        if (d.reglist & (1u << i)) {
          if (!first) oss << ", ";
          oss << reg_name(static_cast<unsigned>(i));
          first = false;
        }
      }
      oss << "}";
      break;
    }
    case ArmOpKind::LoadStore: {
      const char* mnem = d.load ? "ldr" : "str";
      oss << mnem << cs;
      if (d.byte) oss << "b";
      else if (d.half) oss << "h";
      oss << " " << reg_name(d.rd) << ", ";
      char immbuf[24];
      if (d.add)
        std::snprintf(immbuf, sizeof(immbuf), "#0x%x", d.imm12);
      else
        std::snprintf(immbuf, sizeof(immbuf), "#-0x%x", d.imm12);
      if (d.preindex) {
        oss << "[" << reg_name(d.rn);
        if (d.imm12 != 0 || !d.add) {
          oss << ", " << immbuf;
        }
        oss << "]";
        if (d.writeback) oss << "!";
      } else {
        oss << "[" << reg_name(d.rn) << "], " << immbuf;
      }
      break;
    }
    case ArmOpKind::Branch:
    case ArmOpKind::BranchLink: {
      oss << (d.link ? "bl" : "b") << cs << " ";
      char buf[24];
      if (d.imm24 < 0)
        std::snprintf(buf, sizeof(buf), "-0x%x", static_cast<uint32_t>(-d.imm24));
      else
        std::snprintf(buf, sizeof(buf), "0x%x", static_cast<uint32_t>(d.imm24));
      oss << buf;
      break;
    }
    case ArmOpKind::Bx:
      oss << "bx" << cs << " " << reg_name(d.rm);
      break;
    case ArmOpKind::Svc: {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "svc%s #0x%x", cs.c_str(), d.svc_imm);
      return buf;
    }
    default: {
      char buf[40];
      std::snprintf(buf, sizeof(buf), ".word 0x%08x", inst);
      return buf;
    }
  }

  return oss.str();
}

// ---- ID/EX mapping ----

void aarch32_fill_id_ex(const ArmDecoded& d, ID_EX_Register& id_ex) {
  id_ex.regWrite = d.regWrite;
  id_ex.aluSrc = d.aluSrc;
  id_ex.branch = d.branch;
  id_ex.memRe = d.memRe;
  id_ex.memWr = d.memWr;
  id_ex.memToReg = d.memToReg;
  id_ex.upperIm = d.writeback; // reuse: writeback for LS / block
  id_ex.aluOp = d.aluOp;
  id_ex.memReadType = d.memReadType;
  id_ex.memWriteType = d.memWriteType;
  id_ex.fpRegWrite = false;
  id_ex.fpRegRead1 = false;
  id_ex.fpRegRead2 = false;
  id_ex.fpRegRead3 = false;
  id_ex.fpOp = 0;
  id_ex.csr_access = false;
  id_ex.opcode = static_cast<unsigned int>(d.kind);
  id_ex.rd = d.rd_pipe;
  id_ex.rs1 = d.rs1;
  id_ex.rs2 = d.rs2;
  id_ex.rs3 = 0;
  id_ex.funct3 = d.cond;
  id_ex.funct7 = d.opc;
  id_ex.immediate = d.immediate;
  id_ex.ebreak = false;
  id_ex.ecall = d.ecall;
  id_ex.mret = false;
  id_ex.valid = d.valid;

  id_ex.arm_cond = d.cond;
  id_ex.arm_set_flags = d.set_flags;
  id_ex.arm_dp_opc = d.opc;
  id_ex.arm_link = d.link || (d.kind == ArmOpKind::BranchLink);
  id_ex.arm_bx = (d.kind == ArmOpKind::Bx);
  id_ex.arm_is_arm = d.valid;
  id_ex.arm_shift_type = d.shift_type;
  id_ex.arm_shift_imm = d.shift_imm;
  id_ex.arm_op2_imm = d.use_imm;

  if (d.kind == ArmOpKind::Multiply) {
    id_ex.funct7 = d.accumulate ? 1u : 0u;
    id_ex.rs3 = d.rn;
  } else if (d.kind == ArmOpKind::LoadStore) {
    id_ex.funct7 = (d.add ? 1u : 0u)
                 | (d.preindex ? 2u : 0u)
                 | (d.writeback ? 4u : 0u)
                 | (d.byte ? 8u : 0u)
                 | (d.half ? 16u : 0u)
                 | (d.use_imm ? 0u : 32u); // bit5: reg offset
    if (!d.use_imm) {
      id_ex.rs2 = d.rm;
    }
  } else if (d.kind == ArmOpKind::BlockTransfer) {
    id_ex.funct7 = (d.add ? 1u : 0u)
                 | (d.preindex ? 2u : 0u)
                 | (d.writeback ? 4u : 0u)
                 | (d.load ? 8u : 0u);
    id_ex.immediate = static_cast<int32_t>(d.reglist);
    id_ex.rs1 = d.rn;
    id_ex.rd = d.rn;
  } else if (d.kind == ArmOpKind::DataProc) {
    id_ex.funct7 = d.opc;
    id_ex.arm_dp_opc = d.opc;
    if (d.aluOp == 0x100 || d.aluOp == 0x101) {
      id_ex.aluOp = d.aluOp; // preserve MOVW/MOVT markers
      id_ex.arm_dp_opc = static_cast<uint8_t>(d.aluOp & 0xFF);
    }
    if (d.shift_by_reg) {
      id_ex.rs3 = d.rs;
    }
  }
}

void aarch32_apply_decode(uint32_t inst, ID_EX_Register& id_ex, bool& ok) {
  ArmDecoded d;
  ok = aarch32_decode(inst, d);
  if (!ok) {
    // Leave a benign NOP-like ID/EX
    ID_EX_Register cleared;
    cleared.instruction = inst;
    cleared.arm_is_arm = false;
    cleared.valid = false;
    // Preserve pc if already set by caller
    const uint32_t pc = id_ex.pc;
    const bool is_c = id_ex.is_compressed;
    const uint16_t cinst = id_ex.compressed_inst;
    id_ex = cleared;
    id_ex.pc = pc;
    id_ex.is_compressed = is_c;
    id_ex.compressed_inst = cinst;
    id_ex.instruction = inst;
    return;
  }
  aarch32_fill_id_ex(d, id_ex);
  id_ex.instruction = inst;
}
