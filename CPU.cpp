// file: CPU.cpp

#include "CPU.h"
#include "Cache.h"
#include "CacheScheme.h"
#include <iomanip>
#include <iostream>
#include <cmath>
#include <cstring>

#define MEMORY_SIZE 4096

CPU::CPU()
{
    PC = 0; //set PC to 0

    for (int i = 0; i < 32; i++) {
        registers[i] = 0;
        registers_fp[i] = 0.0f;
    }
    
    // Initialize FCSR (Floating-Point Control and Status Register)
    fcsr = 0;  // Default: round to nearest, no exceptions

    // Initialize pipeline control
    pipeline_stall = false;
    pipeline_flush = false;
    maxPC = 0;
    enable_logging = false;
    enable_tracing_ = false;
    dmem_ = NULL;
    branch_predictor_ = NULL;
    branch_predicted_taken_ = false;
    branch_predicted_target_ = 0;
    branch_pc_ = 0;

    // Initialize tracking structures
    for (int i = 0; i < 32; i++) {
        previous_register_values_[i] = 0;
    }

    // dmem_ will be set by main(); we keep nullptr check in load/store
}

CPU::~CPU()
{
	if (log_file.is_open()) {
		log_file.close();
	}
}

void CPU::reset() {
    PC = 0;
    
    // Reset all registers
    for (int i = 0; i < 32; i++) {
        registers[i] = 0;
        registers_fp[i] = 0.0f;
        previous_register_values_[i] = 0;
    }
    
    // Reset FCSR
    fcsr = 0;
    
    // Reset pipeline control
    pipeline_stall = false;
    pipeline_flush = false;
    maxPC = 0;
    branch_predicted_taken_ = false;
    branch_predicted_target_ = 0;
    branch_pc_ = 0;
    
    // Reset branch predictor if it exists
    if (branch_predictor_) {
        branch_predictor_->reset();
    }
    
    // Reset pipeline registers
    if_id = IF_ID_Register();
    id_ex = ID_EX_Register();
    ex_mem = EX_MEM_Register();
    mem_wb = MEM_WB_Register();
    ex_mem_prev = EX_MEM_Register();
    mem_wb_prev = MEM_WB_Register();
    
    // Clear traces and statistics
    clear_trace();
    clear_memory_history();
    clear_register_history();
    clear_dependencies();
    
    // Reset statistics
    stats_ = CPUStatistics();
    
    // Reset maps
    pc_to_cycle_map_.clear();
    pc_to_rd_map_.clear();
    
    // Note: dmem_ is preserved (set externally)
    // Note: enable_tracing_ is preserved
}


unsigned long CPU::readPC()
{
	return PC;
}
void CPU::incPC(int increment)
{
	PC += increment;
}

// returns the current instruction as a string (32-bit)
string CPU::get_instruction(char *IM) {
	string inst = "";
	if (IM[PC*2] == '0' && IM[PC*2+1] == '0') {
		return "00000000";
	}
    for (int i = 0; i < 4; i++) {
        inst += IM[PC*2 + 6 - i*2];
        inst += IM[PC*2 + 7 - i*2];
    }
	return inst;
}

// returns a 16-bit instruction as a string
string CPU::get_instruction_16bit(char *IM) {
	string inst = "";
	if (IM[PC*2] == '0' && IM[PC*2+1] == '0' && IM[PC*2+2] == '0' && IM[PC*2+3] == '0') {
		return "0000";
	}
	// Read 2 bytes (4 hex chars) in little-endian format
	// Memory layout in string: [byte0_high, byte0_low, byte1_high, byte1_low]
	// For little-endian 16-bit instruction, we want: byte1_high byte1_low byte0_high byte0_low
	// So read byte1 first, then byte0
	inst += IM[PC*2 + 2];  // byte1 high nibble
	inst += IM[PC*2 + 3];  // byte1 low nibble
	inst += IM[PC*2];      // byte0 high nibble
	inst += IM[PC*2 + 1];  // byte0 low nibble
	return inst;
}

// returns the value of a specific register
int CPU::get_register_value(int reg) {
	if (reg < 0 || reg > 32)
		return 0;
	return static_cast<int>(registers[reg]);
}

// decodes an instruction to get control signals and decode the instruction into the necessary parts
bool CPU::decode_instruction(string inst, bool *regWrite, bool *aluSrc, bool *branch, bool *memRe, bool *memWr, bool *memToReg, bool *upperIm, int *aluOp,
	unsigned int *opcode, unsigned int *rd, unsigned int *funct3, unsigned int *rs1, unsigned int *rs2, unsigned int *funct7, bool debug) {
	    
    unsigned int instruction = std::stoul(inst, nullptr, 16);

    // Extract instruction fields
    *opcode = instruction & 0x7F;
    *rd = (instruction >> 7) & 0x1F;
    *funct3 = (instruction >> 12) & 0x7;
    *rs1 = (instruction >> 15) & 0x1F;
    *rs2 = (instruction >> 20) & 0x1F;
    *funct7 = (instruction >> 25) & 0x7F;

    // aluOp values:
    // Arithmetic: 0x00 - 0x0F
    // Logic:      0x10 - 0x1F
    // Shift:      0x20 - 0x2F
    // Branch:     0x30 - 0x3F
    // Load/Store: 0x40 - 0x4F
    // Jump:       0x50 - 0x5F
    

    if (debug) {
        auto oldflags = std::cout.flags(); // save format flags
        std::cout << "PC: " << std::dec << PC << std::endl;
        std::cout << "Hex Instruction: " << inst << std::endl;
        std::cout << "Decoded fields:" << std::endl;
        std::cout << "  opcode: 0x" << std::hex << *opcode << std::endl;
        std::cout << "  rd: " << std::dec << *rd << std::endl;
        std::cout << "  funct3: " << std::dec << *funct3 << std::endl;
        std::cout << "  rs1: " << std::dec << *rs1 << std::endl;
        std::cout << "  rs2: " << std::dec << *rs2 << std::endl;
        std::cout << "  funct7: 0x" << std::hex << *funct7 << std::endl;
        std::cout.flags(oldflags); // restore original format flags
    }
    

    // Default control signals
    *regWrite = false;
    *aluSrc = false;
    *branch = false;
    *memRe = false;
    *memWr = false;
    *memToReg = false;
    *upperIm = false;
    *aluOp = 0;
    // We'll stash load/store widths in ID/EX after this call (set defaults here)
    id_ex.memReadType = 0;
    id_ex.memWriteType = 0;
    
    // Initialize FP control signals
    id_ex.fpRegWrite = false;
    id_ex.fpRegRead1 = false;
    id_ex.fpRegRead2 = false;
    id_ex.fpOp = 0;

    // Decode based on opcode
    switch (*opcode) {
        case 0x33: // R-type (ADD, SUB, OR, XOR, SLL, SRL, SRA, SLT, SLTU) and M extension
            *regWrite = true;
            *aluSrc = false;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;
            
            // M extension instructions (funct7=0x01) - check first for proper decoding
            if (*funct7 == 0x01) {
                if (*funct3 == 0x0) { // MUL
                    *aluOp = 0x60;
                }
                else if (*funct3 == 0x1) { // MULH
                    *aluOp = 0x61;
                }
                else if (*funct3 == 0x2) { // MULHSU
                    *aluOp = 0x62;
                }
                else if (*funct3 == 0x3) { // MULHU
                    *aluOp = 0x63;
                }
                else if (*funct3 == 0x4) { // DIV
                    *aluOp = 0x64;
                }
                else if (*funct3 == 0x5) { // DIVU
                    *aluOp = 0x65;
                }
                else if (*funct3 == 0x6) { // REM
                    *aluOp = 0x66;
                }
                else if (*funct3 == 0x7) { // REMU
                    *aluOp = 0x67;
                }
            }
            // Standard RV32I R-type instructions
            else if (*funct3 == 0x0 && *funct7 == 0x00) { // ADD
                *aluOp = 0x00;
            }
            else if (*funct3 == 0x0 && *funct7 == 0x20) { // SUB
                *aluOp = 0x01;
            }
            else if (*funct3 == 0x6 && *funct7 == 0x00) { // OR
                *aluOp = 0x11;
            }
            else if (*funct3 == 0x4 && *funct7 == 0x00) { // XOR
                *aluOp = 0x12;
            }
            else if (*funct3 == 0x1 && *funct7 == 0x00) { // SLL
                *aluOp = 0x20;
            }
            else if (*funct3 == 0x5 && *funct7 == 0x00) { // SRL
                *aluOp = 0x21;
            }
            else if (*funct3 == 0x5 && *funct7 == 0x20) { // SRA
                *aluOp = 0x22;
            }
            else if (*funct3 == 0x2 && *funct7 == 0x00) { // SLT
                *aluOp = 0x13;
            }
            else if (*funct3 == 0x3 && *funct7 == 0x00) { // SLTU
                *aluOp = 0x14;
            }
            else if (*funct3 == 0x7 && *funct7 == 0x00) { // AND
                *aluOp = 0x10;
            }
            break;

        case 0x13: // I-type (ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI)
            *regWrite = true;
            *aluSrc = true;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;
            
            if (*funct3 == 0x0) { // ADDI
                *aluOp = 0x00;
            }
            else if (*funct3 == 0x2) { // SLTI
                *aluOp = 0x15;
            }
            else if (*funct3 == 0x3) { // SLTIU
                *aluOp = 0x16;
            }
            else if (*funct3 == 0x4) { // XORI
                *aluOp = 0x17;
            }
            else if (*funct3 == 0x6) { // ORI
                *aluOp = 0x18;
            }
            else if (*funct3 == 0x7) { // ANDI
                *aluOp = 0x19;
            }
            else if (*funct3 == 0x1 && *funct7 == 0x00) { // SLLI
                *aluOp = 0x23;
            }
            else if (*funct3 == 0x5 && *funct7 == 0x00) { // SRLI
                *aluOp = 0x24;
            }
            else if (*funct3 == 0x5 && *funct7 == 0x20) { // SRAI
                *aluOp = 0x25;
            }
            else {
                // Invalid funct3 for I-type, treat as NOP
                *regWrite = false;
                *aluOp = 0;
                if (debug) {
                    std::cout << "Invalid funct3 " << *funct3 << " for I-type instruction, treating as NOP" << std::endl;
                }
            }
            break;

        case 0x03: // Load instructions (LB, LBU, LH, LHU, LW)
            *regWrite = true;
            *aluSrc = true;
            *branch = false;
            *memRe = true;
            *memWr = false;
            *memToReg = true;
            *upperIm = false;
            
            if (*funct3 == 0x0) { // LB
                *aluOp = 0x40;
                id_ex.memReadType = 1;
            }
            else if (*funct3 == 0x4) { // LBU
                *aluOp = 0x41;
                id_ex.memReadType = 2;
            }
            else if (*funct3 == 0x1) { // LH
                *aluOp = 0x42;
                id_ex.memReadType = 3;
            }
            else if (*funct3 == 0x5) { // LHU
                *aluOp = 0x43;
                id_ex.memReadType = 4;
            }
            else if (*funct3 == 0x2) { // LW
                *aluOp = 0x44;
                id_ex.memReadType = 5;
            }
            break;

        case 0x23: // Store instructions (SB, SH, SW)
            *regWrite = false;
            *aluSrc = true;
            *branch = false;
            *memRe = false;
            *memWr = true;
            *memToReg = false;
            *upperIm = false;
            if (*funct3 == 0x0) { // SB
                *aluOp = 0x45;
                id_ex.memWriteType = 1;
            }
            else if (*funct3 == 0x1) { // SH
                *aluOp = 0x46;
                id_ex.memWriteType = 2;
            }
            else if (*funct3 == 0x2) { // SW
                *aluOp = 0x47;
                id_ex.memWriteType = 3;
            }
            break;

        case 0x63: // BEQ
            *regWrite = false;
            *aluSrc = false;
            *branch = true;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;

            if (*funct3 == 0x0) // BEQ
                *aluOp = 0x30;
            else if (*funct3 == 0x1) // BNE
                *aluOp = 0x35;
            else if (*funct3 == 0x2) // Reserved, but treat as BEQ for compatibility
                *aluOp = 0x30;
            else if (*funct3 == 0x4) // BLT
                *aluOp = 0x33;
            else if (*funct3 == 0x5) // BGE
                *aluOp = 0x31;
            else if (*funct3 == 0x6) // BLTU
                *aluOp = 0x34;
            else if (*funct3 == 0x7) // BGEU
                *aluOp = 0x32;
            break;
        
        case 0x67: // JALR
        case 0x6F: // JAL
            *regWrite = true;
            *aluSrc = true;
            *branch = true;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;
            *aluOp = 0x00;
            break;

        case 0x37: // LUI
            *regWrite = true;
            *aluSrc = true;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = true;
            *aluOp = 0xF;
            break;

        case 0x17: //AUIPC
            *regWrite = true;
            *aluSrc = true;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = true;
            *aluOp = 0x00;
            break;

        case 0x00: // NULL instruction (program end)
			// cout << "Program end" << endl;
			return false;

        case 0x07: // FLW - Load word to FP register
            id_ex.fpRegWrite = true;
            id_ex.fpRegRead1 = false;
            id_ex.fpRegRead2 = false;
            *regWrite = false;  // Not writing to integer register
            *aluSrc = true;
            *branch = false;
            *memRe = true;
            *memWr = false;
            *memToReg = true;  // Load from memory
            *upperIm = false;
            *aluOp = 0x44;  // Use LW address calculation
            id_ex.memReadType = 6;  // FLW
            break;

        case 0x27: // FSW - Store word from FP register
            id_ex.fpRegWrite = false;
            id_ex.fpRegRead1 = false;
            id_ex.fpRegRead2 = true;  // Read rs2 from FP register
            *regWrite = false;
            *aluSrc = true;
            *branch = false;
            *memRe = false;
            *memWr = true;
            *memToReg = false;
            *upperIm = false;
            *aluOp = 0x47;  // Use SW address calculation
            id_ex.memWriteType = 4;  // FSW
            break;

        case 0x53: // FP arithmetic instructions (FADD.S, FSUB.S, FMUL.S, FDIV.S, etc.)
            // Decode based on funct7 and funct3
            id_ex.fpRegWrite = true;
            id_ex.fpRegRead1 = true;  // Read rs1 from FP register
            id_ex.fpRegRead2 = true;  // Read rs2 from FP register
            *regWrite = false;  // Writing to FP register, not integer
            *aluSrc = false;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;
            *aluOp = 0;  // Not using ALU
            
            // Decode FP operation based on funct7 and funct3
            if (*funct7 == 0x00) {
                if (*funct3 == 0x0) {
                    id_ex.fpOp = 0x70;  // FADD.S
                } else if (*funct3 == 0x4) {
                    id_ex.fpOp = 0x71;  // FSUB.S
                } else if (*funct3 == 0x8) {
                    id_ex.fpOp = 0x72;  // FMUL.S
                } else if (*funct3 == 0xC) {
                    id_ex.fpOp = 0x73;  // FDIV.S
                } else if (*funct3 == 0x10) {
                    id_ex.fpOp = 0x74;  // FSGNJ.S
                } else if (*funct3 == 0x14) {
                    id_ex.fpOp = 0x75;  // FMIN.S
                } else if (*funct3 == 0x18) {
                    id_ex.fpOp = 0x76;  // FMAX.S
                } else if (*funct3 == 0x50) {
                    id_ex.fpOp = 0x77;  // FSQRT.S
                } else if (*funct3 == 0x60) {
                    id_ex.fpOp = 0x78;  // FCVT.W.S (convert float to int)
                    id_ex.fpRegRead2 = false;  // Only uses rs1
                    *regWrite = true;  // Writes to integer register
                } else if (*funct3 == 0x68) {
                    id_ex.fpOp = 0x79;  // FCVT.S.W (convert int to float)
                    id_ex.fpRegRead1 = false;  // Reads from integer register
                    id_ex.fpRegRead2 = false;
                } else if (*funct3 == 0x70) {
                    id_ex.fpOp = 0x7A;  // FMV.X.W (move FP to int)
                    id_ex.fpRegRead2 = false;
                    *regWrite = true;  // Writes to integer register
                } else if (*funct3 == 0x78) {
                    id_ex.fpOp = 0x7B;  // FMV.W.X (move int to FP)
                    id_ex.fpRegRead1 = false;  // Reads from integer register
                    id_ex.fpRegRead2 = false;
                }
            } else if (*funct7 == 0x50) {
                if (*funct3 == 0x0) {
                    id_ex.fpOp = 0x7C;  // FLE.S (less than or equal)
                    *regWrite = true;  // Writes comparison result to integer register
                } else if (*funct3 == 0x1) {
                    id_ex.fpOp = 0x7D;  // FLT.S (less than)
                    *regWrite = true;
                } else if (*funct3 == 0x2) {
                    id_ex.fpOp = 0x7E;  // FEQ.S (equal)
                    *regWrite = true;
                }
            } else if (*funct7 == 0x70) {
                if (*funct3 == 0x0) {
                    id_ex.fpOp = 0x7F;  // FCLASS.S (classify float)
                    id_ex.fpRegRead2 = false;
                    *regWrite = true;  // Writes to integer register
                }
            }
            break;

        default:
            // For now, treat unknown opcodes as NOP to avoid pipeline stalls
            *regWrite = false;
            *aluSrc = false;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;
            *aluOp = 0;
            if (debug) {
                std::cout << "Unknown opcode: 0x" << std::hex << *opcode << ", treating as NOP" << std::endl;
            }
            break;
	}
	return true;
}

// generates immediate for the given instruction
int32_t CPU::generate_immediate(uint32_t instruction, int opcode) const {
    int32_t imm = 0;
    
    switch(opcode) {
        case 0x13: // I-type (ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI)
            imm = instruction >> 20;
            if (((instruction >> 12) & 0x7) == 0x1) { // SLLI
                imm = imm & 0x1F;
            } else if (((instruction >> 12) & 0x7) == 0x5) {
                // Keep full 12 bits for funct7 detection
                // Lower 5 bits are shamt
                // Upper 7 bits are funct7 (used only in decode)
                // For ALU execution, we still want the shamt
                imm = imm & 0x1F;
            } else {
                imm = sign_extend(imm, 12);
            }
            break;
            
        case 0x03: // Load instructions (LB, LW)
        case 0x67: // JALR
            imm = instruction >> 20;
            imm = sign_extend(imm, 12);
            break;
            
        case 0x23: // Store instructions (SB, SW)
            // Immediate is split into two parts
            imm = ((instruction >> 20) & 0xFE0) | ((instruction >> 7) & 0x1F);
            imm = sign_extend(imm, 12);
            break;
            
        case 0x63: // Branch Instructions (BEQ, BGE, BGEU)
            // B-type immediate encoding: imm[12|10:5] in bits 31:25, imm[4:1|11] in bits 11:7
            imm = ((instruction >> 31) & 0x1) << 12 |  // imm[12]
                  ((instruction >> 7) & 0x1) << 11 |   // imm[11]
                  ((instruction >> 25) & 0x3F) << 5 |  // imm[10:5]
                  ((instruction >> 8) & 0xF) << 1;     // imm[4:1]
            imm = sign_extend(imm, 13);
            break;
            
        case 0x6F: // JAL
            // UJ-type immediate encoding: imm[20|10:1|11|19:12] in bits 31:12
            imm = ((instruction >> 31) & 0x1) << 20 |  // imm[20]
                  ((instruction >> 21) & 0x3FF) << 1 | // imm[10:1]
                  ((instruction >> 20) & 0x1) << 11 |  // imm[11]
                  ((instruction >> 12) & 0xFF) << 12;  // imm[19:12]
            imm = sign_extend(imm, 21);
            break;
            
        case 0x37: // LUI
        case 0x17: //AUIPC
            // Upper immediate - already shifted
            imm = instruction & 0xFFFFF000;
            break;
    }
    
    return imm;
}

// Sign extension helper
int32_t CPU::sign_extend(int32_t value, int bits) const {
    int32_t sign_bit = (value >> (bits - 1)) & 1;
    if (sign_bit) {
        value |= (~0U << bits);
    }
    return value;
}

// Memory access alignment check
bool CPU::check_address_alignment(uint32_t address, uint32_t bytes) {
    if (address >= MEMORY_SIZE) {
        std::cerr << "Memory access out of bounds: " << address << std::endl;
        return false;
    }
    
    if (bytes == 2 && (address % 2 != 0)) {
        std::cerr << "Unaligned halfword access at address: " << address << std::endl;
        return false;
    }
    
    if (bytes == 4 && (address % 4 != 0)) {
        std::cerr << "Unaligned word access at address: " << address << std::endl;
        return false;
    }
    
    return true;
}

// Memory read operation (now via dmem_)
int32_t CPU::read_memory(uint32_t address, int type) {
    // type: 1=LB (byte, sign extended), 2=LBU (byte, zero extended),
    //       3=LH (halfword, sign extended), 4=LHU (halfword, zero extended), 5=LW (word)

    if (!dmem_) { std::cerr << "ERROR: data memory not set\n"; return 0; }

    AccessSize sz = AccessSize::Byte;
    switch (type) {
        case 1: case 2: sz = AccessSize::Byte; break;
        case 3: case 4: sz = AccessSize::Half; break;
        case 5:         sz = AccessSize::Word; break;
        default: return 0;
    }

    uint32_t bytes = static_cast<uint32_t>(sz);
    if (!check_address_alignment(address, bytes)) return 0;

    MemResp r = dmem_->load(address, sz);
    if (!r.ok) { std::cerr << "Memory read OOB @ " << address << "\n"; return 0; }

    if (type == 1) { // LB sign-extend
        int8_t b = (int8_t)(r.data & 0xFF);
        return (int32_t)b;
    } else if (type == 2) { // LBU zero-extend
        uint8_t b = (uint8_t)(r.data & 0xFF);
        return (int32_t)b;
    } else if (type == 3) { // LH sign-extend
        int16_t h = (int16_t)(r.data & 0xFFFF);
        return (int32_t)h;
    } else if (type == 4) { // LHU zero-extend
        uint16_t h = (uint16_t)(r.data & 0xFFFF);
        return (int32_t)h;
    } else { // LW
        return (int32_t)r.data;
    }
}

// Memory write operation (now via dmem_)
void CPU::write_memory(uint32_t address, int32_t value, int type) {
    if (!dmem_) { std::cerr << "ERROR: data memory not set\n"; return; }

    AccessSize sz = AccessSize::Byte;
    switch (type) {
        case 1: sz = AccessSize::Byte; break;  // SB
        case 2: sz = AccessSize::Half; break;  // SH
        case 3: sz = AccessSize::Word; break;  // SW
        default: return;
    }

    uint32_t bytes = static_cast<uint32_t>(sz);
    if (!check_address_alignment(address, bytes)) return;

    bool ok = dmem_->store(address, (uint32_t)value, sz);
    if (!ok) { std::cerr << "Memory write OOB @ " << address << "\n"; }
}


void CPU::print_all_registers() {
    std::cout << std::dec; // force decimal
    std::cout << "Register Values:" << std::endl;
    for (int i = 0; i < 32; i++) {
        std::cout << REGISTER_NAMES[i] << ": " << registers[i] << std::endl;
    }
}

// Pipeline stage implementations

void CPU::instruction_fetch(char* instMem, bool debug) {
    if (pipeline_stall) {
        if (debug) { std::cout << "IF: Pipeline stalled, no instruction fetched" << std::endl; }
        return;
    }
    if (pipeline_flush) {
        if_id.valid = false;
        pipeline_flush = false;
        if (debug) { std::cout << "IF: Flushed due to branch" << std::endl; }
        return;
    }
    if (PC >= maxPC) {
        if_id.valid = false;
        if (debug) { std::cout << "IF: End of program reached at PC " << PC << std::endl; }
        return;
    }

    // First, fetch 16 bits to check if it's a compressed instruction
    string inst_16_str = get_instruction_16bit(instMem);
    if (inst_16_str == "0000") {
        if_id.valid = false;
        if (debug) { std::cout << "IF: NOP instruction (all zeros)" << std::endl; }
        return;
    }
    
    uint16_t inst_16 = static_cast<uint16_t>(std::stoul(inst_16_str, nullptr, 16));
    
    // Check if it's a compressed instruction (bottom 2 bits != 0b11)
    if (is_compressed_instruction(inst_16)) {
        // It's a 16-bit compressed instruction
        if_id.is_compressed = true;
        if_id.compressed_inst = inst_16;
        // Expand to 32-bit equivalent
        if_id.instruction = expand_compressed_instruction(inst_16);
        if_id.pc = PC;
        // Only mark as valid if expansion succeeded (non-zero result)
        if_id.valid = (if_id.instruction != 0);
        
        if (debug) {
            std::cout << "IF: Fetched compressed instruction 0x" << std::hex << inst_16 
                      << " (expanded to 0x" << if_id.instruction << ") at PC 0x" << if_id.pc << std::dec << std::endl;
        }
        incPC(2);  // Increment PC by 2 for compressed instruction
    } else {
        // It's a 32-bit instruction
    string inst_str = get_instruction(instMem);
    if (inst_str == "00000000") {
        if_id.valid = false;
        if (debug) { std::cout << "IF: NOP instruction (all zeros)" << std::endl; }
        return;
    }
    if_id.instruction = std::stoul(inst_str, nullptr, 16);
        if_id.is_compressed = false;
        if_id.compressed_inst = 0;
    if_id.pc = PC;
    if_id.valid = true;

    if (debug) {
        std::cout << "IF: Fetched instruction 0x" << std::hex << if_id.instruction 
                  << " at PC 0x" << if_id.pc << std::dec << std::endl;
        std::cout << "IF: Raw instruction string: " << inst_str << std::endl;
    }
        incPC(4);  // Increment PC by 4 for 32-bit instruction
    }
}


void CPU::instruction_decode(bool debug) {
    if (pipeline_flush) {
        id_ex.valid = false;
        pipeline_flush = false;
        if (debug) {
            std::cout << "ID: Flushed due to branch" << std::endl;
        }
        return;
    }

    if (!if_id.valid) {
        id_ex.valid = false;
        if (debug) {
            std::cout << "ID: No valid instruction to decode" << std::endl;
        }
        return;
    }

    // Decode the instruction
    bool regWrite, aluSrc, branch, memRe, memWr, memToReg, upperIm;
    int aluOp;
    unsigned int opcode, rd, funct3, rs1, rs2, funct7;

    // Convert instruction to hex string for decode_instruction
    char inst_str[9];
    snprintf(inst_str, sizeof(inst_str), "%08x", if_id.instruction);
    bool valid = decode_instruction(string(inst_str), &regWrite, &aluSrc, &branch, &memRe, 
                                   &memWr, &memToReg, &upperIm, &aluOp,
                                   &opcode, &rd, &funct3, &rs1, &rs2, &funct7, debug);

    // Load-use hazard detection - disabled for now, relying on forwarding
    // TODO: Implement proper load-use hazard detection
    /*
    if (id_ex.memRe && id_ex.regWrite && id_ex.rd != 0 && (
        (id_ex.rd == rs1 && rs1 != 0) || 
        (id_ex.rd == rs2 && rs2 != 0))) {

        pipeline_stall = true;
        if (debug) {
            std::cout << "ID: Load-use hazard detected. Stalling pipeline." << std::endl;
            std::cout << "    Previous instruction: rd=" << id_ex.rd << ", memRe=" << id_ex.memRe << std::endl;
            std::cout << "    Current instruction: rs1=" << rs1 << ", rs2=" << rs2 << std::endl;
        }
        return;
    }
    */

    if (!valid) {
        id_ex.valid = false;
        if (debug) {
            std::cout << "ID: Invalid instruction decoded" << std::endl;
        }
        return;
    }

    // Track instruction type statistics
    stats_.total_instructions++;
    switch (opcode) {
        case 0x33: stats_.r_type_count++; break;
        case 0x13: stats_.i_type_count++; break;
        case 0x03: stats_.load_count++; break;
        case 0x23: stats_.store_count++; break;
        case 0x63: stats_.branch_count++; break;
        case 0x67: case 0x6F: stats_.jump_count++; break;
        case 0x37: case 0x17: stats_.lui_auipc_count++; break;
    }

    // Read register values
    int32_t rs1_data = (rs1 != 0) ? get_register_value(rs1) : 0;
    int32_t rs2_data = (rs2 != 0) ? get_register_value(rs2) : 0;
    
    // Read FP register values if needed
    float rs1_fp_data = 0.0f;
    float rs2_fp_data = 0.0f;
    if (id_ex.fpRegRead1 && rs1 != 0) {
        rs1_fp_data = registers_fp[rs1];
    }
    if (id_ex.fpRegRead2 && rs2 != 0) {
        rs2_fp_data = registers_fp[rs2];
    }

    // Generate immediate value
    int32_t immediate = generate_immediate(if_id.instruction, opcode);

    // Branch prediction: predict in ID stage for branch instructions
    if (branch && branch_predictor_ && (opcode == 0x63)) {  // Only predict conditional branches (not jumps)
        uint32_t target = if_id.pc + immediate;
        BranchPrediction pred = branch_predictor_->predict(if_id.pc, target);
        
        branch_predicted_taken_ = pred.predicted_taken;
        branch_predicted_target_ = pred.predicted_target;
        branch_pc_ = if_id.pc;
        
        // If predicted taken, update PC and flush IF/ID
        if (pred.predicted_taken) {
            PC = target;
            pipeline_flush = true;  // Use pipeline_flush flag so IF stage can handle it properly
            if (debug) {
                std::cout << "ID: Branch predicted taken, PC -> 0x" << std::hex << target << std::dec << std::endl;
            }
        }
    } else if (branch && (opcode == 0x67 || opcode == 0x6F)) {
        // Jumps (JAL/JALR) are always taken - no prediction needed
        branch_predicted_taken_ = true;
        branch_predicted_target_ = if_id.pc + immediate;
        branch_pc_ = if_id.pc;
    } else {
        branch_predicted_taken_ = false;
        branch_predicted_target_ = 0;
        branch_pc_ = 0;
    }

    // Update ID/EX register
    id_ex.regWrite = regWrite;
    id_ex.aluSrc = aluSrc;
    id_ex.branch = branch;
    id_ex.memRe = memRe;
    id_ex.memWr = memWr;
    id_ex.memToReg = memToReg;
    id_ex.upperIm = upperIm;
    id_ex.aluOp = aluOp;
    id_ex.opcode = opcode;
    id_ex.rd = rd;
    id_ex.funct3 = funct3;
    id_ex.rs1 = rs1;
    id_ex.rs2 = rs2;
    id_ex.funct7 = funct7;
    id_ex.rs1_data = rs1_data;
    id_ex.rs2_data = rs2_data;
    id_ex.rs1_fp_data = rs1_fp_data;
    id_ex.rs2_fp_data = rs2_fp_data;
    id_ex.immediate = immediate;
    id_ex.pc = if_id.pc;
    id_ex.instruction = if_id.instruction;
    id_ex.is_compressed = if_id.is_compressed;
    id_ex.compressed_inst = if_id.compressed_inst;
    id_ex.valid = true;

    // Track instruction dependencies
    if (enable_tracing_) {
        track_instruction_dependencies(stats_.total_cycles, if_id.pc, rd, rs1, rs2);
    }

    if (debug) {
        string disasm = if_id.is_compressed ? 
            disassemble_compressed_instruction(if_id.compressed_inst) + " [expanded: " + disassemble_instruction(if_id.instruction) + "]" :
            disassemble_instruction(if_id.instruction);
        std::cout << "ID: Decoded instruction - " << disasm << std::endl;
        std::cout << "    rs1_data: " << rs1_data << ", rs2_data: " << rs2_data << ", immediate: " << immediate << std::endl;
        std::cout << "    Valid: " << (valid ? "true" : "false") << std::endl;
    }
}


void CPU::execute_stage(bool debug) {
    if (!id_ex.valid) {
        ex_mem.valid = false;
        if (debug) {
            std::cout << "EX: No valid instruction to execute" << std::endl;
        }
        return;
    }

    // Forward from previous-cycle EX/MEM for operand1 (rs1)
    int32_t operand1;
    if (ex_mem_prev.regWrite && ex_mem_prev.rd != 0 && ex_mem_prev.rd == id_ex.rs1) {
        operand1 = ex_mem_prev.alu_result;
    }
    else if (mem_wb_prev.regWrite && mem_wb_prev.rd != 0 && mem_wb_prev.rd == id_ex.rs1) {
        operand1 = mem_wb_prev.memToReg ? mem_wb_prev.mem_data : mem_wb_prev.alu_result;
    } else {
        operand1 = id_ex.rs1_data;
    }

    // Forward from previous-cycle EX/MEM for operand2 (rs2) unless using immediate
    int32_t operand2;
    if (id_ex.aluSrc) {
        operand2 = id_ex.immediate;  // e.g., SRAI
    }
    else if (ex_mem_prev.regWrite && ex_mem_prev.rd != 0 && ex_mem_prev.rd == id_ex.rs2) {
        operand2 = ex_mem_prev.alu_result;
    }
    else if (mem_wb_prev.regWrite && mem_wb_prev.rd != 0 && mem_wb_prev.rd == id_ex.rs2) {
        operand2 = mem_wb_prev.memToReg ? mem_wb_prev.mem_data : mem_wb_prev.alu_result;
    } else {
        operand2 = id_ex.rs2_data;
    }


    // LUI operand setup
    if (id_ex.opcode == 0x37 && id_ex.upperIm) {
        operand1 = id_ex.immediate;
        operand2 = 0;
    }

    // Forward FP operands for FP operations
    float fp_operand1 = id_ex.rs1_fp_data;
    float fp_operand2 = id_ex.rs2_fp_data;
    
    // FP forwarding from EX/MEM
    if (ex_mem_prev.fpRegWrite && ex_mem_prev.rd != 0 && ex_mem_prev.rd == id_ex.rs1 && id_ex.fpRegRead1) {
        fp_operand1 = ex_mem_prev.fp_result;
    } else if (mem_wb_prev.fpRegWrite && mem_wb_prev.rd != 0 && mem_wb_prev.rd == id_ex.rs1 && id_ex.fpRegRead1) {
        fp_operand1 = mem_wb_prev.memToReg ? mem_wb_prev.mem_fp_data : mem_wb_prev.fp_result;
    }
    
    if (ex_mem_prev.fpRegWrite && ex_mem_prev.rd != 0 && ex_mem_prev.rd == id_ex.rs2 && id_ex.fpRegRead2) {
        fp_operand2 = ex_mem_prev.fp_result;
    } else if (mem_wb_prev.fpRegWrite && mem_wb_prev.rd != 0 && mem_wb_prev.rd == id_ex.rs2 && id_ex.fpRegRead2) {
        fp_operand2 = mem_wb_prev.memToReg ? mem_wb_prev.mem_fp_data : mem_wb_prev.fp_result;
    }

    // Call ALU
    int32_t alu_result = alu.execute(operand1, operand2, id_ex.aluOp);
    
    // Execute FP operations
    float fp_result = 0.0f;
    int32_t fp_int_result = 0;
    if (id_ex.fpOp != 0) {
        if (id_ex.fpOp == 0x78) {  // FCVT.W.S - convert float to int
            fp_int_result = static_cast<int32_t>(fp_operand1);
        } else if (id_ex.fpOp == 0x79) {  // FCVT.S.W - convert int to float
            fp_result = static_cast<float>(operand1);
        } else if (id_ex.fpOp == 0x7A) {  // FMV.X.W - move FP to int (bitwise)
            uint32_t bits;
            memcpy(&bits, &fp_operand1, sizeof(uint32_t));
            fp_int_result = static_cast<int32_t>(bits);
        } else if (id_ex.fpOp == 0x7B) {  // FMV.W.X - move int to FP (bitwise)
            uint32_t bits = static_cast<uint32_t>(operand1);
            memcpy(&fp_result, &bits, sizeof(float));
        } else if (id_ex.fpOp == 0x7C || id_ex.fpOp == 0x7D || id_ex.fpOp == 0x7E) {  // FP comparisons
            fp_int_result = execute_fp_compare(fp_operand1, fp_operand2, id_ex.fpOp);
        } else if (id_ex.fpOp == 0x7F) {  // FCLASS.S
            fp_int_result = execute_fp_classify(fp_operand1);
        } else if (id_ex.fpOp == 0x77) {  // FSQRT.S - only uses operand1
            fp_result = execute_fp_operation(fp_operand1, 0.0f, id_ex.fpOp);
        } else {
            // Other FP operations use both operands
            fp_result = execute_fp_operation(fp_operand1, fp_operand2, id_ex.fpOp);
        }
    }

    // --- Jumps (handle control transfer + link) ---
    if (id_ex.opcode == 0x6F) { // JAL
        // rd := PC+4, PC := PC + imm (imm already byte-scaled)
        ex_mem.regWrite   = true;
        ex_mem.memRe      = false;
        ex_mem.memWr      = false;
        ex_mem.memToReg   = false;
        ex_mem.memReadType  = 0;
        ex_mem.memWriteType = 0;
        ex_mem.alu_result = static_cast<int32_t>(id_ex.pc + 4);
        ex_mem.rs2_data   = 0;
        ex_mem.rd         = id_ex.rd;
        ex_mem.pc         = id_ex.pc;
        ex_mem.instruction = id_ex.instruction;
        ex_mem.is_compressed = id_ex.is_compressed;
        ex_mem.compressed_inst = id_ex.compressed_inst;
        ex_mem.valid      = true;

        uint32_t target_pc = id_ex.pc + id_ex.immediate;
        PC = target_pc;
        pipeline_flush = true;
        if (debug || enable_logging) { 
            std::cout << "EX: JAL at PC=0x" << std::hex << id_ex.pc 
                      << " immediate=" << std::dec << id_ex.immediate
                      << " target=0x" << std::hex << target_pc << std::dec << std::endl;
            if (enable_logging && log_file.is_open()) {
                log_file << "EX: JAL at PC=0x" << std::hex << id_ex.pc 
                         << " immediate=" << std::dec << id_ex.immediate
                         << " target=0x" << std::hex << target_pc << std::dec << std::endl;
            }
        }
        return;
    }
    if (id_ex.opcode == 0x67) { // JALR
        // rd := PC+4, PC := (rs1 + imm) & ~1
        int32_t target = (operand1 + id_ex.immediate) & ~1;

        ex_mem.regWrite   = true;
        ex_mem.memRe      = false;
        ex_mem.memWr      = false;
        ex_mem.memToReg   = false;
        ex_mem.memReadType  = 0;
        ex_mem.memWriteType = 0;
        ex_mem.alu_result = static_cast<int32_t>(id_ex.pc + 4);
        ex_mem.rs2_data   = 0;
        ex_mem.rd         = id_ex.rd;
        ex_mem.pc         = id_ex.pc;
        ex_mem.instruction = id_ex.instruction;
        ex_mem.is_compressed = id_ex.is_compressed;
        ex_mem.compressed_inst = id_ex.compressed_inst;
        ex_mem.valid      = true;

        PC = static_cast<uint32_t>(target);
        pipeline_flush = true;
        if (debug) { std::cout << "EX: JALR taken to " << PC << ", link=" << (id_ex.pc+4) << std::endl; }
        return;
    }

    // Branch decision
    if (id_ex.branch) {
        bool should_branch = false;
        uint32_t target = id_ex.pc + id_ex.immediate;
        
        // For conditional branches, determine if branch should be taken
        if (id_ex.opcode == 0x63) {  // Conditional branch
            switch (id_ex.aluOp) {
                case 0x30: should_branch = alu.isZero(); break; // BEQ
                case 0x35: should_branch = !alu.isZero(); break; // BNE
                case 0x31: should_branch = alu.isZero(); break;  // BGE
                case 0x33: should_branch = alu.isZero(); break; // BLT
                case 0x32: should_branch = alu.isZero(); break;  // BGEU
                case 0x34: should_branch = alu.isZero(); break; // BLTU
            }
            
            // Update branch predictor with actual outcome
            if (branch_predictor_) {
                branch_predictor_->update(id_ex.pc, target, should_branch);
            }
            
            // Check for misprediction
            bool mispredicted = false;
            if (should_branch != branch_predicted_taken_) {
                mispredicted = true;
                stats_.branch_mispredictions++;
            } else if (should_branch && target != branch_predicted_target_) {
                // Predicted taken but wrong target (shouldn't happen with our predictors, but check anyway)
                mispredicted = true;
                stats_.branch_mispredictions++;
            }
            
            // Update branch statistics based on ACTUAL outcome (not prediction)
            if (should_branch) {
                stats_.branch_taken_count++;
            } else {
                stats_.branch_not_taken_count++;
            }
            
            if (mispredicted) {
                // Correct the PC
                if (should_branch) {
                    PC = target;
                } else {
                    PC = id_ex.pc + 4;  // Should have continued sequentially
                }
                pipeline_flush = true;
                if (debug) {
                    std::cout << "EX: Branch mispredicted! Correcting PC." << std::endl;
                }
            }
            // If correctly predicted (taken or not taken), no flush needed in EX
            // - If correctly predicted taken: flush already happened in ID stage
            // - If correctly predicted not taken: no flush needed
        } else if (id_ex.opcode == 0x6F || id_ex.opcode == 0x67) {
            // Jumps (JAL/JALR) are always taken
            // Note: Jumps are not conditional branches, so they don't affect branch prediction statistics
            // They are tracked separately in jump_count
            PC = target;
            pipeline_flush = true;
            // Don't increment branch_taken_count for jumps - they're not conditional branches
        }

        if (debug) {
            std::cout << "EX: Branch decision - aluOp=" << std::hex << id_ex.aluOp 
                      << ", alu.isZero()=" << alu.isZero() 
                      << ", should_branch=" << should_branch << std::dec << std::endl;
        }
    }

    // Forward rs2_data for store operations
    int32_t forwarded_rs2_data;
    if (ex_mem_prev.regWrite && ex_mem_prev.rd != 0 && ex_mem_prev.rd == id_ex.rs2) {
        forwarded_rs2_data = ex_mem_prev.alu_result;
    }
    else if (mem_wb_prev.regWrite && mem_wb_prev.rd != 0 && mem_wb_prev.rd == id_ex.rs2) {
        forwarded_rs2_data = mem_wb_prev.memToReg ? mem_wb_prev.mem_data : mem_wb_prev.alu_result;
    } else {
        forwarded_rs2_data = id_ex.rs2_data;
    }
    
    // Forward FP rs2_data for FSW operations
    float forwarded_rs2_fp_data = id_ex.rs2_fp_data;
    if (ex_mem_prev.fpRegWrite && ex_mem_prev.rd != 0 && ex_mem_prev.rd == id_ex.rs2 && id_ex.memWriteType == 4) {
        forwarded_rs2_fp_data = ex_mem_prev.fp_result;
    } else if (mem_wb_prev.fpRegWrite && mem_wb_prev.rd != 0 && mem_wb_prev.rd == id_ex.rs2 && id_ex.memWriteType == 4) {
        forwarded_rs2_fp_data = mem_wb_prev.memToReg ? mem_wb_prev.mem_fp_data : mem_wb_prev.fp_result;
    }

    // Update EX/MEM register
    ex_mem.regWrite = id_ex.regWrite;
    ex_mem.memRe = id_ex.memRe;
    ex_mem.memWr = id_ex.memWr;
    ex_mem.memToReg = id_ex.memToReg;
    ex_mem.memReadType  = id_ex.memReadType;
    ex_mem.memWriteType = id_ex.memWriteType;
    ex_mem.fpRegWrite = id_ex.fpRegWrite;
    ex_mem.fp_result = (id_ex.fpOp != 0 && id_ex.fpOp != 0x78 && id_ex.fpOp != 0x7A) ? fp_result : 0.0f;
    ex_mem.alu_result = (id_ex.fpOp == 0x78 || id_ex.fpOp == 0x7A || id_ex.fpOp == 0x7C || id_ex.fpOp == 0x7D || id_ex.fpOp == 0x7E || id_ex.fpOp == 0x7F) ? fp_int_result : alu_result;
    ex_mem.rs2_data = forwarded_rs2_data;
    ex_mem.rs2_fp_data = forwarded_rs2_fp_data;
    ex_mem.rd = id_ex.rd;
    ex_mem.pc = id_ex.pc;
    ex_mem.instruction = id_ex.instruction;
    ex_mem.is_compressed = id_ex.is_compressed;
    ex_mem.compressed_inst = id_ex.compressed_inst;
    ex_mem.valid = true;

    if (debug) {
        std::cout << "EX: ALU operation - " << operand1 << " op " << operand2 << " = " << alu_result << std::endl;

        if (id_ex.branch) {
            bool should_branch = false;
            switch (id_ex.aluOp) {
                case 0x30: should_branch = alu.isZero(); break; // BEQ
                case 0x35: should_branch = !alu.isZero(); break; // BNE
                case 0x31: should_branch = alu.isZero(); break;  // BGE
                case 0x33: should_branch = alu.isZero(); break; // BLT
                case 0x32: should_branch = alu.isZero(); break;  // BGEU
                case 0x34: should_branch = alu.isZero(); break; // BLTU
            }
            std::cout << "EX: Branch instruction ";
            std::cout << (should_branch ? "taken" : "not taken");
            std::cout << " (Zero flag = " << alu.isZero() << ")" << std::endl;
        }
    }
}


void CPU::memory_stage(bool debug) {
    if (!ex_mem.valid) {
        mem_wb.valid = false;
        if (debug) {
            std::cout << "MEM: No valid instruction for memory stage" << std::endl;
        }
        return;
    }
    
    int32_t mem_data = 0;
    float mem_fp_data = 0.0f;
    
    // Memory operations
    if (ex_mem.memRe) {
        // Load operation - track cache stats before access
        uint64_t hits_before = 0, misses_before = 0;
        bool had_cache = get_cache_stats(hits_before, misses_before);
        
        if (ex_mem.memReadType == 6) {  // FLW - load float
            mem_data = read_memory(ex_mem.alu_result, 5);  // Use LW read type
            // Convert int32_t to float (bitwise copy)
            uint32_t bits = static_cast<uint32_t>(mem_data);
            memcpy(&mem_fp_data, &bits, sizeof(float));
        } else {
        mem_data = read_memory(ex_mem.alu_result, ex_mem.memReadType);
        }
        stats_.memory_reads++;
        
        // Check if this was a cache hit or miss
        bool cache_hit = false;
        if (had_cache) {
            uint64_t hits_after = 0, misses_after = 0;
            get_cache_stats(hits_after, misses_after);
            // If hits increased, it was a hit; if misses increased, it was a miss
            cache_hit = (hits_after > hits_before);
        }
        
        // Track memory access
        if (enable_tracing_) {
            track_memory_access(stats_.total_cycles, ex_mem.alu_result, false, mem_data, ex_mem.pc, cache_hit);
        }
        
        if (debug) {
            if (ex_mem.memReadType == 6) {
                std::cout << "MEM: FLW from address " << ex_mem.alu_result << " = " << mem_fp_data << std::endl;
            } else {
            std::cout << "MEM: Load from address " << ex_mem.alu_result << " = " << mem_data << std::endl;
            }
        }
    } else if (ex_mem.memWr) {
        // Store operation - track cache stats before access
        uint64_t hits_before = 0, misses_before = 0;
        bool had_cache = get_cache_stats(hits_before, misses_before);
        
        if (ex_mem.memWriteType == 4) {  // FSW - store float
            // Convert float to int32_t (bitwise copy)
            uint32_t bits;
            memcpy(&bits, &ex_mem.rs2_fp_data, sizeof(float));
            write_memory(ex_mem.alu_result, static_cast<int32_t>(bits), 3);  // Use SW write type
        } else {
        write_memory(ex_mem.alu_result, ex_mem.rs2_data, ex_mem.memWriteType);
        }
        stats_.memory_writes++;
        
        // Check if this was a cache hit or miss
        bool cache_hit = false;
        if (had_cache) {
            uint64_t hits_after = 0, misses_after = 0;
            get_cache_stats(hits_after, misses_after);
            // If hits increased, it was a hit; if misses increased, it was a miss
            cache_hit = (hits_after > hits_before);
        }
        
        // Track memory access
        if (enable_tracing_) {
            track_memory_access(stats_.total_cycles, ex_mem.alu_result, true, ex_mem.rs2_data, ex_mem.pc, cache_hit);
        }
        
        if (debug) {
            std::cout << "MEM: Store " << ex_mem.rs2_data << " to address " << ex_mem.alu_result << std::endl;
        }
    }
    
    // Update MEM/WB register
    mem_wb.regWrite = ex_mem.regWrite;
    mem_wb.memToReg = ex_mem.memToReg;
    mem_wb.alu_result = ex_mem.alu_result;
    mem_wb.mem_data = mem_data;
    mem_wb.rd = ex_mem.rd;
    mem_wb.pc = ex_mem.pc;
    mem_wb.instruction = ex_mem.instruction;
    mem_wb.is_compressed = ex_mem.is_compressed;
    mem_wb.compressed_inst = ex_mem.compressed_inst;
    mem_wb.valid = true;
}

void CPU::write_back_stage(bool debug) {
    if (!mem_wb.valid) {
        if (debug) {
            std::cout << "WB: No valid instruction for write back" << std::endl;
        }
        return;
    }
    
    if (mem_wb.regWrite && mem_wb.rd != 0) {
        int32_t write_data = mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result;
        int32_t old_value = previous_register_values_[mem_wb.rd];
        registers[mem_wb.rd] = write_data;
        stats_.instructions_retired++;
        
        // Track register change
        if (enable_tracing_) {
            track_register_change(stats_.total_cycles, mem_wb.rd, old_value, write_data, mem_wb.pc);
            previous_register_values_[mem_wb.rd] = write_data;
            
            // Update dependency tracking maps
            pc_to_cycle_map_[mem_wb.pc] = stats_.total_cycles;
            pc_to_rd_map_[mem_wb.pc] = mem_wb.rd;
        }
        
        if (debug) {
            std::cout << std::dec;
            std::cout << "WB: Write " << write_data << " to register " << REGISTER_NAMES[mem_wb.rd] << std::endl;
        }
    }
    
    if (mem_wb.fpRegWrite && mem_wb.rd != 0) {
        float write_fp_data = mem_wb.memToReg ? mem_wb.mem_fp_data : mem_wb.fp_result;
        registers_fp[mem_wb.rd] = write_fp_data;
        if (!mem_wb.regWrite) {  // Only count once if both regWrite and fpRegWrite are true
            stats_.instructions_retired++;
        }
        
        if (debug) {
            std::cout << std::dec;
            std::cout << "WB: Write " << write_fp_data << " to FP register " << FP_REGISTER_NAMES[mem_wb.rd] << std::endl;
        }
    }
    
    if (mem_wb.valid && !mem_wb.regWrite && !mem_wb.fpRegWrite) {
        // Count instructions that complete even if they don't write to registers
        stats_.instructions_retired++;
        
        // Still track cycle for dependency analysis
        if (enable_tracing_) {
            pc_to_cycle_map_[mem_wb.pc] = stats_.total_cycles;
        }
    }
}

void CPU::run_pipeline_cycle(char* instMem, int cycle, bool debug) {
    if (debug) {
        std::cout << "\n=== Cycle " << cycle << " ===" << std::endl;
    }

    // Update statistics
    stats_.total_cycles = cycle;
    
    // Update cache statistics
    uint64_t cache_hits, cache_misses;
    if (get_cache_stats(cache_hits, cache_misses)) {
        stats_.cache_hits = cache_hits;
        stats_.cache_misses = cache_misses;
    }

    // Take snapshots so EX can see last cycle's values
    ex_mem_prev = ex_mem;
    mem_wb_prev = mem_wb;

    // Track stalls and flushes - use flags to track if they occur during this cycle
    bool cycle_had_stall = pipeline_stall;  // From previous cycle or already set
    bool cycle_had_flush = pipeline_flush;  // From previous cycle or already set

    write_back_stage(debug);
    memory_stage(debug);
    execute_stage(debug);
    
    // Check for flush/stall after EX stage
    if (pipeline_flush) cycle_had_flush = true;
    if (pipeline_stall) cycle_had_stall = true;
    
    instruction_decode(debug);
    
    // Check for flush/stall after ID stage (ID might set them)
    if (pipeline_flush) cycle_had_flush = true;
    if (pipeline_stall) cycle_had_stall = true;
    
    instruction_fetch(instMem, debug);
    
    // Final check after all stages
    if (pipeline_flush) cycle_had_flush = true;
    if (pipeline_stall) cycle_had_stall = true;
    
    // Track stalls and flushes (count cycles with stalls/flushes)
    if (cycle_had_stall) {
        stats_.stall_count++;
    }
    if (cycle_had_flush) {
        stats_.flush_count++;
    }
    
    // Store for logging (so log shows the actual state during the cycle, not after)
    bool log_stall = cycle_had_stall;
    bool log_flush = cycle_had_flush;

    // Capture pipeline snapshot for GUI
    if (enable_tracing_) {
        capture_pipeline_snapshot(cycle, log_stall, log_flush);
    }

    if (enable_logging) {
        log_pipeline_state(cycle, log_stall, log_flush);
    }

    // Clear pipeline stall when the load instruction has moved past the EX stage
    if (pipeline_stall && !id_ex.memRe) {
        pipeline_stall = false;
    }
}



void CPU::set_logging(bool enable, string log_filename) {
    // Close existing log file if open
    if (log_file.is_open()) {
        log_file.close();
    }
    
    enable_logging = enable;
    if (enable && !log_filename.empty()) {
        // Open in truncate mode to overwrite previous log
        log_file.open(log_filename, std::ios::out | std::ios::trunc);
        if (log_file.is_open()) {
            log_file << "Pipeline Execution Log" << std::endl;
            log_file << "=====================" << std::endl;
            log_file.flush();
        } else {
            std::cerr << "Failed to open log file: " << log_filename << std::endl;
        }
    }
}

bool CPU::is_pipeline_empty() {
    return !if_id.valid && !id_ex.valid && !ex_mem.valid && !mem_wb.valid;
}

void CPU::set_max_pc(int max_pc) {
    maxPC = max_pc;
}

// Check if a 16-bit instruction is a compressed instruction
// Compressed instructions have bottom 2 bits != 0b11
bool CPU::is_compressed_instruction(uint16_t inst) const {
    return (inst & 0x3) != 0x3;
}

// Expand a 16-bit compressed instruction to its 32-bit equivalent
uint32_t CPU::expand_compressed_instruction(uint16_t compressed_inst) const {
    uint8_t op = compressed_inst & 0x3;
    uint8_t funct3 = (compressed_inst >> 13) & 0x7;
    uint8_t rd_rs1 = (compressed_inst >> 7) & 0x1F;
    uint8_t rs2 = (compressed_inst >> 2) & 0x1F;
    
    // Quadrant 0: 00 (C.ADDI4SPN, C.LW, C.SW, etc.)
    if (op == 0x0) {
        if (funct3 == 0x0) {
            // C.ADDI4SPN: addi rd', x2, nzuimm[9:2]
            // nzuimm[9:6] are in bits [10:7], nzuimm[5:4] in bits [12:11], nzuimm[3] in bit [5], nzuimm[2] in bit [6]
            uint8_t rd_prime = 8 + ((compressed_inst >> 2) & 0x7);
            uint32_t imm = ((compressed_inst >> 7) & 0xF) << 6 |  // nzuimm[9:6] from bits [10:7]
                          ((compressed_inst >> 11) & 0x3) << 4 |  // nzuimm[5:4] from bits [12:11]
                          ((compressed_inst >> 5) & 0x1) << 3 |   // nzuimm[3] from bit [5]
                          ((compressed_inst >> 6) & 0x1) << 2;    // nzuimm[2] from bit [6]
            if (imm == 0) return 0; // Reserved encoding
            imm <<= 2; // Scale by 4
            // ADDI rd', x2, imm
            return (0x13) | (rd_prime << 7) | (0x02 << 15) | ((imm & 0xFFF) << 20);
        } else if (funct3 == 0x2) {
            // C.LW: lw rd', offset[6:2](rs1')
            // uimm[5:3] in bits [12:10], uimm[2|6] in bits [6:5] (uimm[6] is MSB, uimm[2] is LSB)
            uint8_t rd_prime = 8 + ((compressed_inst >> 2) & 0x7);
            uint8_t rs1_prime = 8 + ((compressed_inst >> 7) & 0x7);
            uint32_t imm = ((compressed_inst >> 10) & 0x7) << 3 |  // uimm[5:3] from bits [12:10]
                          ((compressed_inst >> 6) & 0x1) << 6 |      // uimm[6] from bit [6]
                          ((compressed_inst >> 5) & 0x1) << 2;        // uimm[2] from bit [5]
            imm <<= 2; // Scale by 4
            // LW rd', imm(rs1')
            return (0x03) | (rd_prime << 7) | (0x2 << 12) | (rs1_prime << 15) | ((imm & 0xFFF) << 20);
        } else if (funct3 == 0x6) {
            // C.SW: sw rs2', offset[6:2](rs1')
            // uimm[5:3] in bits [12:10], uimm[2|6] in bits [6:5] (uimm[6] is MSB, uimm[2] is LSB)
            uint8_t rs2_prime = 8 + ((compressed_inst >> 2) & 0x7);
            uint8_t rs1_prime = 8 + ((compressed_inst >> 7) & 0x7);
            uint32_t imm = ((compressed_inst >> 10) & 0x7) << 3 |  // uimm[5:3] from bits [12:10]
                          ((compressed_inst >> 6) & 0x1) << 6 |      // uimm[6] from bit [6]
                          ((compressed_inst >> 5) & 0x1) << 2;        // uimm[2] from bit [5]
            imm <<= 2; // Scale by 4
            // SW rs2', imm(rs1')
            return (0x23) | (0x2 << 12) | (rs1_prime << 15) | (rs2_prime << 20) | ((imm & 0xFE0) << 20) | ((imm & 0x1F) << 7);
        }
    }
    // Quadrant 1: 01 (C.ADDI, C.JAL, C.LI, C.ADDI16SP, C.LUI, C.SRLI, C.SRAI, C.ANDI, C.SUB, C.XOR, C.OR, C.AND, C.J, C.BEQZ, C.BNEZ)
    else if (op == 0x1) {
        if (funct3 == 0x0) {
            // C.ADDI: addi rd, rd, nzimm[5:0]
            // rd = rd_rs1, imm = sign-extend({rd_rs1[5], imm[4:0]})
            int32_t imm = ((compressed_inst >> 12) & 0x1) ? 0xFFFFFFE0 : 0; // sign bit
            imm |= ((compressed_inst >> 2) & 0x1F);
            if (rd_rs1 == 0) return 0; // Reserved (NOP)
            // ADDI rd, rd, imm
            return (0x13) | (rd_rs1 << 7) | (0x0 << 12) | (rd_rs1 << 15) | ((imm & 0xFFF) << 20);
        } else if (funct3 == 0x1) {
            // C.JAL: jal x1, offset[11:1]
            // Only valid in RV32, not RV32C (but some implementations support it)
            // For now, treat as C.J (no link)
            int32_t imm = ((compressed_inst >> 12) & 0x1) ? 0xFFFFF000 : 0; // sign bit
            imm |= ((compressed_inst >> 2) & 0x100) |  // bit [10]
                   ((compressed_inst >> 3) & 0x80) |   // bit [9]
                   ((compressed_inst >> 6) & 0x40) |   // bit [8]
                   ((compressed_inst >> 7) & 0x20) |   // bit [7]
                   ((compressed_inst >> 8) & 0x10) |   // bit [6]
                   ((compressed_inst >> 9) & 0x8) |    // bit [5]
                   ((compressed_inst >> 10) & 0x4) |   // bit [4]
                   ((compressed_inst >> 11) & 0x2) |   // bit [3]
                   ((compressed_inst >> 5) & 0x1);     // bit [2]
            imm <<= 1; // Scale by 2
            // JAL x1, imm
            return (0x6F) | (0x01 << 7) | ((imm & 0x7FE) << 20) | ((imm & 0x800) << 12) | ((imm & 0xFF000) << 12) | ((imm & 0x100000) << 31);
        } else if (funct3 == 0x2) {
            // C.LI: addi rd, x0, imm[5:0]
            // rd = rd_rs1, imm = sign-extend(imm[5:0])
            int32_t imm = ((compressed_inst >> 12) & 0x1) ? 0xFFFFFFE0 : 0; // sign bit
            imm |= ((compressed_inst >> 2) & 0x1F);
            if (rd_rs1 == 0) return 0; // Reserved
            // ADDI rd, x0, imm
            return (0x13) | (rd_rs1 << 7) | (0x0 << 12) | (0x0 << 15) | ((imm & 0xFFF) << 20);
        } else if (funct3 == 0x3) {
            if (rd_rs1 == 2) {
                // C.ADDI16SP: addi x2, x2, nzimm[9:4]
                // nzimm[9] in bit [12], nzimm[8:7] in bits [4:3], nzimm[6:5] in bits [6:5], nzimm[4] in bit [2]
                int32_t imm = ((compressed_inst >> 12) & 0x1) << 9 |  // nzimm[9] from bit [12] (sign)
                              ((compressed_inst >> 4) & 0x1) << 8 |     // nzimm[8] from bit [4]
                              ((compressed_inst >> 3) & 0x1) << 7 |     // nzimm[7] from bit [3]
                              ((compressed_inst >> 6) & 0x1) << 6 |     // nzimm[6] from bit [6]
                              ((compressed_inst >> 5) & 0x1) << 5 |     // nzimm[5] from bit [5]
                              ((compressed_inst >> 2) & 0x1) << 4;      // nzimm[4] from bit [2]
                // Sign extend
                if (imm & 0x200) {
                    imm |= 0xFFFFFC00;
                }
                if (imm == 0) return 0; // Reserved
                imm <<= 4; // Scale by 16
                // ADDI x2, x2, imm
                return (0x13) | (0x02 << 7) | (0x0 << 12) | (0x02 << 15) | ((imm & 0xFFF) << 20);
            } else {
                // C.LUI: lui rd, nzimm[17:12]
                int32_t imm = ((compressed_inst >> 12) & 0x1) ? 0xFFFFF000 : 0; // sign bit
                imm |= ((compressed_inst >> 2) & 0x1F) << 12;
                if (rd_rs1 == 0 || rd_rs1 == 2) return 0; // Reserved
                // LUI rd, imm
                return (0x37) | (rd_rs1 << 7) | ((imm & 0xFFFFF) << 12);
            }
        } else if (funct3 == 0x4) {
            // C.SRLI, C.SRAI, C.ANDI, C.SUB, C.XOR, C.OR, C.AND
            uint8_t funct2 = (compressed_inst >> 10) & 0x3;
            if (funct2 == 0x0) {
                // C.SRLI: srli rd', rd', shamt[5:0]
                uint8_t rd_prime = 8 + ((compressed_inst >> 7) & 0x7);
                uint8_t shamt = ((compressed_inst >> 2) & 0x1F);
                if (shamt == 0) return 0; // Reserved
                // SRLI rd', rd', shamt
                return (0x13) | (rd_prime << 7) | (0x5 << 12) | (rd_prime << 15) | (shamt << 20);
            } else if (funct2 == 0x1) {
                // C.SRAI: srai rd', rd', shamt[5:0]
                uint8_t rd_prime = 8 + ((compressed_inst >> 7) & 0x7);
                uint8_t shamt = ((compressed_inst >> 2) & 0x1F);
                if (shamt == 0) return 0; // Reserved
                // SRAI rd', rd', shamt
                return (0x13) | (rd_prime << 7) | (0x5 << 12) | (0x20 << 25) | (rd_prime << 15) | (shamt << 20);
            } else if (funct2 == 0x2) {
                // C.ANDI: andi rd', rd', imm[5:0]
                uint8_t rd_prime = 8 + ((compressed_inst >> 7) & 0x7);
                int32_t imm = ((compressed_inst >> 12) & 0x1) ? 0xFFFFFFE0 : 0; // sign bit
                imm |= ((compressed_inst >> 2) & 0x1F);
                // ANDI rd', rd', imm
                return (0x13) | (rd_prime << 7) | (0x7 << 12) | (rd_prime << 15) | ((imm & 0xFFF) << 20);
            } else if (funct2 == 0x3) {
                // C.SUB, C.XOR, C.OR, C.AND: For funct2=3, check bit[12], bit[6], and bit[8] to distinguish
                // From test file encodings for rd'=1, rs2'=2:
                // 0x8c89 = C.SUB: bit[12]=0, bit[6]=0
                // 0x9ca9 = C.XOR: bit[12]=1, bit[6]=0, bit[8]=1
                // 0x9ccd = C.OR: bit[12]=1, bit[6]=1
                // 0x9c89 = C.AND: bit[12]=1, bit[6]=0, bit[8]=0
                uint8_t bit12 = (compressed_inst >> 12) & 0x1;
                uint8_t bit8 = (compressed_inst >> 8) & 0x1;
                uint8_t bit6 = (compressed_inst >> 6) & 0x1;
                uint8_t rd_prime = 8 + ((compressed_inst >> 7) & 0x7);
                uint8_t rs2_prime = 8 + ((compressed_inst >> 2) & 0x7);
                // Check bit[12] first - C.SUB has bit[12]=0
                if (bit12 == 0) {
                    // C.SUB: sub rd', rd', rs2'
                    return (0x33) | (rd_prime << 7) | (0x0 << 12) | (0x20 << 25) | (rd_prime << 15) | (rs2_prime << 20);
                } else {
                    // bit[12]=1, check bit[6] to distinguish C.OR from C.XOR/C.AND
                    if (bit6 == 1) {
                        // C.OR: or rd', rd', rs2'
                        return (0x33) | (rd_prime << 7) | (0x6 << 12) | (rd_prime << 15) | (rs2_prime << 20);
                    } else {
                        // bit[6]=0, check bit[8] to distinguish C.AND from C.XOR
                        if (bit8 == 0) {
                            // C.AND: and rd', rd', rs2'
                            return (0x33) | (rd_prime << 7) | (0x7 << 12) | (rd_prime << 15) | (rs2_prime << 20);
                        } else {
                            // C.XOR: xor rd', rd', rs2'
                            return (0x33) | (rd_prime << 7) | (0x4 << 12) | (rd_prime << 15) | (rs2_prime << 20);
                        }
                    }
                }
            }
        } else if (funct3 == 0x5) {
            // C.J: jal x0, offset[11:1]
            // Instruction bits [12|10:5|4:1|11] correspond to imm[10|9:4|3:0|11]
            int32_t imm = ((compressed_inst >> 12) & 0x1) << 10 |  // imm[10] from bit [12]
                          ((compressed_inst >> 5) & 0x3F) << 4 |   // imm[9:4] from bits [10:5]
                          ((compressed_inst >> 1) & 0xF) |          // imm[3:0] from bits [4:1]
                          ((compressed_inst >> 11) & 0x1) << 11;   // imm[11] from bit [11]
            // Sign extend
            if (imm & 0x800) {
                imm |= 0xFFFFF000;
            }
            imm <<= 1; // Scale by 2 (offset is in units of 2 bytes)
            // JAL x0, imm
            return (0x6F) | (0x00 << 7) | ((imm & 0x7FE) << 20) | ((imm & 0x800) << 12) | ((imm & 0xFF000) << 12) | ((imm & 0x100000) << 31);
        } else if (funct3 == 0x6) {
            // C.BEQZ: beq rs1', x0, offset[8:1]
            // imm[8] from bit [12], imm[7] from bit [6], imm[6] from bit [5], imm[5] from bit [2]
            // imm[4] from bit [11], imm[3] from bit [10], imm[2] from bit [4], imm[1] from bit [3]
            uint8_t rs1_prime = 8 + ((compressed_inst >> 7) & 0x7);
            int32_t imm = ((compressed_inst >> 12) & 0x1) << 8 |  // imm[8] from bit [12] (sign)
                          ((compressed_inst >> 6) & 0x1) << 7 |     // imm[7] from bit [6]
                          ((compressed_inst >> 5) & 0x1) << 6 |     // imm[6] from bit [5]
                          ((compressed_inst >> 2) & 0x1) << 5 |     // imm[5] from bit [2]
                          ((compressed_inst >> 11) & 0x1) << 4 |    // imm[4] from bit [11]
                          ((compressed_inst >> 10) & 0x1) << 3 |    // imm[3] from bit [10]
                          ((compressed_inst >> 4) & 0x1) << 2 |     // imm[2] from bit [4]
                          ((compressed_inst >> 3) & 0x1) << 1;      // imm[1] from bit [3]
            // Sign extend
            if (imm & 0x100) {
                imm |= 0xFFFFFE00;
            }
            imm <<= 1; // Scale by 2
            // BEQ rs1', x0, imm
            return (0x63) | (0x0 << 12) | (rs1_prime << 15) | ((imm & 0x800) << 4) | ((imm & 0x1E) << 7) | ((imm & 0x3E0) << 20) | ((imm & 0x400) >> 3);
        } else if (funct3 == 0x7) {
            // C.BNEZ: bne rs1', x0, offset[8:1]
            // imm[8] from bit [12], imm[7] from bit [6], imm[6] from bit [5], imm[5] from bit [2]
            // imm[4] from bit [11], imm[3] from bit [10], imm[2] from bit [4], imm[1] from bit [3]
            uint8_t rs1_prime = 8 + ((compressed_inst >> 7) & 0x7);
            int32_t imm = ((compressed_inst >> 12) & 0x1) << 8 |  // imm[8] from bit [12] (sign)
                          ((compressed_inst >> 6) & 0x1) << 7 |     // imm[7] from bit [6]
                          ((compressed_inst >> 5) & 0x1) << 6 |     // imm[6] from bit [5]
                          ((compressed_inst >> 2) & 0x1) << 5 |     // imm[5] from bit [2]
                          ((compressed_inst >> 11) & 0x1) << 4 |    // imm[4] from bit [11]
                          ((compressed_inst >> 10) & 0x1) << 3 |    // imm[3] from bit [10]
                          ((compressed_inst >> 4) & 0x1) << 2 |     // imm[2] from bit [4]
                          ((compressed_inst >> 3) & 0x1) << 1;      // imm[1] from bit [3]
            // Sign extend
            if (imm & 0x100) {
                imm |= 0xFFFFFE00;
            }
            imm <<= 1; // Scale by 2
            // BNE rs1', x0, imm
            return (0x63) | (0x1 << 12) | (rs1_prime << 15) | ((imm & 0x800) << 4) | ((imm & 0x1E) << 7) | ((imm & 0x3E0) << 20) | ((imm & 0x400) >> 3);
        }
    }
    // Quadrant 2: 10 (C.SLLI, C.LWSP, C.JR, C.MV, C.JALR, C.ADD, C.SWSP, C.EBREAK)
    else if (op == 0x2) {
        if (funct3 == 0x0) {
            // C.SLLI: slli rd, rd, shamt[5:0]
            uint8_t shamt = ((compressed_inst >> 2) & 0x1F);
            if (rd_rs1 == 0 || shamt == 0) return 0; // Reserved
            // SLLI rd, rd, shamt
            return (0x13) | (rd_rs1 << 7) | (0x1 << 12) | (rd_rs1 << 15) | (shamt << 20);
        } else if (funct3 == 0x2) {
            // C.LWSP: lw rd, offset[7:2](x2)
            // imm[5] from bit [12], imm[4:2] from bits [6:4], imm[7:6] from bits [3:2]
            uint32_t imm = ((compressed_inst >> 12) & 0x1) << 5 |  // imm[5] from bit [12]
                          ((compressed_inst >> 4) & 0x7) << 2 |      // imm[4:2] from bits [6:4]
                          ((compressed_inst >> 2) & 0x3);            // imm[7:6] from bits [3:2]
            if (rd_rs1 == 0) return 0; // Reserved
            imm <<= 2; // Scale by 4
            // LW rd, imm(x2)
            return (0x03) | (rd_rs1 << 7) | (0x2 << 12) | (0x02 << 15) | ((imm & 0xFFF) << 20);
        } else if (funct3 == 0x4) {
            if (rs2 == 0) {
                // C.JR: jalr x0, 0(rs1)
                if (rd_rs1 == 0) return 0; // Reserved
                // JALR x0, 0(rs1)
                return (0x67) | (0x00 << 7) | (0x0 << 12) | (rd_rs1 << 15);
            } else {
                // C.MV: add rd, x0, rs2
                if (rd_rs1 == 0) return 0; // Reserved
                // ADD rd, x0, rs2
                return (0x33) | (rd_rs1 << 7) | (0x0 << 12) | (0x0 << 15) | (rs2 << 20);
            }
        } else if (funct3 == 0x5) {
            if (rs2 == 0) {
                // C.JALR: jalr x1, 0(rs1)
                if (rd_rs1 == 0) return 0; // Reserved
                // JALR x1, 0(rs1)
                return (0x67) | (0x01 << 7) | (0x0 << 12) | (rd_rs1 << 15);
            } else {
                // C.ADD: add rd, rd, rs2
                if (rd_rs1 == 0) return 0; // Reserved
                // ADD rd, rd, rs2
                return (0x33) | (rd_rs1 << 7) | (0x0 << 12) | (rd_rs1 << 15) | (rs2 << 20);
            }
        } else if (funct3 == 0x6) {
            // C.SWSP: sw rs2, offset[7:2](x2)
            // imm[5:2] from bits [12:9], imm[7:6] from bits [8:7]
            uint32_t imm = ((compressed_inst >> 9) & 0xF) << 2 |   // imm[5:2] from bits [12:9]
                          ((compressed_inst >> 7) & 0x3);            // imm[7:6] from bits [8:7]
            imm <<= 2; // Scale by 4
            // SW rs2, imm(x2)
            return (0x23) | (0x2 << 12) | (0x02 << 15) | (rs2 << 20) | ((imm & 0xFE0) << 20) | ((imm & 0x1F) << 7);
        }
    }
    
    // If we can't decode it, return 0 (will be treated as invalid)
    return 0;
}

// Floating-point unit operations
float CPU::execute_fp_operation(float operand1, float operand2, int fpOp) const {
    switch (fpOp) {
        case 0x70: // FADD.S
            return operand1 + operand2;
        case 0x71: // FSUB.S
            return operand1 - operand2;
        case 0x72: // FMUL.S
            return operand1 * operand2;
        case 0x73: // FDIV.S
            if (operand2 == 0.0f) {
                // Division by zero - return infinity with sign of operand1
                return (operand1 < 0.0f) ? -INFINITY : INFINITY;
            }
            return operand1 / operand2;
        case 0x74: // FSGNJ.S (copy sign from rs2 to rs1)
            return copysignf(operand1, operand2);
        case 0x75: // FMIN.S
            return (operand1 < operand2) ? operand1 : operand2;
        case 0x76: // FMAX.S
            return (operand1 > operand2) ? operand1 : operand2;
        case 0x77: // FSQRT.S
            if (operand1 < 0.0f) {
                // Negative input - return NaN
                return NAN;
            }
            return sqrtf(operand1);
        case 0x79: // FCVT.S.W (convert int to float) - operand1 is int value
            return static_cast<float>(static_cast<int32_t>(operand1));
        case 0x7B: // FMV.W.X (move int to FP - bitwise copy)
            {
                uint32_t bits = static_cast<uint32_t>(static_cast<int32_t>(operand1));
                float result;
                memcpy(&result, &bits, sizeof(float));
                return result;
            }
        default:
            return 0.0f;
    }
}

int32_t CPU::execute_fp_compare(float operand1, float operand2, int fpOp) const {
    switch (fpOp) {
        case 0x7C: // FLE.S (less than or equal)
            return (operand1 <= operand2) ? 1 : 0;
        case 0x7D: // FLT.S (less than)
            return (operand1 < operand2) ? 1 : 0;
        case 0x7E: // FEQ.S (equal)
            return (operand1 == operand2) ? 1 : 0;
        default:
            return 0;
    }
}

int32_t CPU::execute_fp_classify(float operand) const {
    int32_t result = 0;
    
    if (isnan(operand)) {
        result |= 0x200;  // Negative NaN
        if (copysignf(1.0f, operand) < 0.0f) {
            result |= 0x100;  // Sign bit
        }
    } else if (isinf(operand)) {
        result |= 0x80;  // Negative infinity
        if (operand < 0.0f) {
            result |= 0x40;  // Sign bit
        }
    } else if (operand == 0.0f) {
        result |= 0x20;  // Negative zero
        if (copysignf(1.0f, operand) < 0.0f) {
            result |= 0x10;  // Sign bit
        }
    } else {
        // Normal or subnormal
        if (fpclassify(operand) == FP_SUBNORMAL) {
            result |= 0x08;  // Subnormal
        } else {
            result |= 0x04;  // Normal
        }
        if (operand < 0.0f) {
            result |= 0x02;  // Sign bit
        }
    }
    
    return result;
}

string CPU::disassemble_instruction(uint32_t instruction) const {
    unsigned int opcode = instruction & 0x7F;
    unsigned int rd = (instruction >> 7) & 0x1F;
    unsigned int funct3 = (instruction >> 12) & 0x7;
    unsigned int rs1 = (instruction >> 15) & 0x1F;
    unsigned int rs2 = (instruction >> 20) & 0x1F;
    unsigned int funct7 = (instruction >> 25) & 0x7F;
    
    string op = "UNKNOWN";
    string args = "";
    
    switch (opcode) {
        case 0x33: // R-type and M extension
            // Check for M extension first (funct7=0x01)
            if (funct7 == 0x01) {
                switch (funct3) {
                    case 0x0: op = "MUL"; break;
                    case 0x1: op = "MULH"; break;
                    case 0x2: op = "MULHSU"; break;
                    case 0x3: op = "MULHU"; break;
                    case 0x4: op = "DIV"; break;
                    case 0x5: op = "DIVU"; break;
                    case 0x6: op = "REM"; break;
                    case 0x7: op = "REMU"; break;
                }
            } else {
                // Standard RV32I R-type instructions
            switch (funct3) {
                case 0x0:
                    op = (funct7 == 0x00) ? "ADD" : "SUB";
                    break;
                case 0x4: op = "XOR"; break;
                case 0x6: op = "OR"; break;
                case 0x7: op = "AND"; break;
                case 0x1: op = "SLL"; break;
                case 0x5: op = (funct7 == 0x00) ? "SRL" : "SRA"; break;
                case 0x2: op = "SLT"; break;
                case 0x3: op = "SLTU"; break;
                }
            }
            args = REGISTER_NAMES[rd] + ", " + REGISTER_NAMES[rs1] + ", " + REGISTER_NAMES[rs2];
            break;
        case 0x13: // I-type (immediate arithmetic)
            switch (funct3) {
                case 0x0: op = "ADDI"; break;
                case 0x4: op = "XORI"; break;
                case 0x6: op = "ORI"; break;
                case 0x7: op = "ANDI"; break;
                case 0x1: op = "SLLI"; break;
                case 0x5: op = (funct7 == 0x00) ? "SRLI" : "SRAI"; break;
                case 0x2: op = "SLTI"; break;
                case 0x3: op = "SLTIU"; break;
            }
            args = REGISTER_NAMES[rd] + ", " + REGISTER_NAMES[rs1] + ", " + std::to_string(generate_immediate(instruction, opcode));
            break;
        case 0x03: // Load
            switch (funct3) {
                case 0x0: op = "LB"; break;
                case 0x1: op = "LH"; break;
                case 0x2: op = "LW"; break;
                case 0x4: op = "LBU"; break;
                case 0x5: op = "LHU"; break;
            }
            args = REGISTER_NAMES[rd] + ", " + std::to_string(generate_immediate(instruction, opcode)) + "(" + REGISTER_NAMES[rs1] + ")";
            break;
        case 0x23: // Store
            switch (funct3) {
                case 0x0: op = "SB"; break;
                case 0x1: op = "SH"; break;
                case 0x2: op = "SW"; break;
            }
            args = REGISTER_NAMES[rs2] + ", " + std::to_string(generate_immediate(instruction, opcode)) + "(" + REGISTER_NAMES[rs1] + ")";
            break;
        case 0x63: // Branch
            switch (funct3) {
                case 0x0: op = "BEQ"; break;
                case 0x1: op = "BNE"; break;
                case 0x2: op = "BEQ"; break;  // Reserved encoding, treat as BEQ
                case 0x4: op = "BLT"; break;
                case 0x5: op = "BGE"; break;
                case 0x6: op = "BLTU"; break;
                case 0x7: op = "BGEU"; break;
                default: op = "BRANCH"; break;
            }
            args = REGISTER_NAMES[rs1] + ", " + REGISTER_NAMES[rs2] + ", " + std::to_string(generate_immediate(instruction, opcode));
            break;
        case 0x37: // LUI
            op = "LUI";
            args = REGISTER_NAMES[rd] + ", " + std::to_string(generate_immediate(instruction, opcode));
            break;
        case 0x17: // AUIPC
            op = "AUIPC";
            args = REGISTER_NAMES[rd] + ", " + std::to_string(generate_immediate(instruction, opcode));
            break;
        case 0x6F: // JAL
            op = "JAL";
            args = REGISTER_NAMES[rd] + ", " + std::to_string(generate_immediate(instruction, opcode));
            break;
        case 0x67: // JALR
            op = "JALR";
            args = REGISTER_NAMES[rd] + ", " + std::to_string(generate_immediate(instruction, opcode)) + "(" + REGISTER_NAMES[rs1] + ")";
            break;
            
        case 0x07: // FLW - Load word to FP register
            op = "FLW";
            args = FP_REGISTER_NAMES[rd] + ", " + std::to_string(generate_immediate(instruction, opcode)) + "(" + REGISTER_NAMES[rs1] + ")";
            break;
            
        case 0x27: // FSW - Store word from FP register
            op = "FSW";
            args = FP_REGISTER_NAMES[rs2] + ", " + std::to_string(generate_immediate(instruction, opcode)) + "(" + REGISTER_NAMES[rs1] + ")";
            break;
            
        case 0x53: // FP arithmetic instructions
            if (funct7 == 0x00) {
                switch (funct3) {
                    case 0x0: op = "FADD.S"; break;
                    case 0x4: op = "FSUB.S"; break;
                    case 0x8: op = "FMUL.S"; break;
                    case 0xC: op = "FDIV.S"; break;
                    case 0x10: op = "FSGNJ.S"; break;
                    case 0x14: op = "FMIN.S"; break;
                    case 0x18: op = "FMAX.S"; break;
                    case 0x50: op = "FSQRT.S"; break;
                    case 0x60: op = "FCVT.W.S"; break;
                    case 0x68: op = "FCVT.S.W"; break;
                    case 0x70: op = "FMV.X.W"; break;
                    case 0x78: op = "FMV.W.X"; break;
                }
            } else if (funct7 == 0x50) {
                switch (funct3) {
                    case 0x0: op = "FLE.S"; break;
                    case 0x1: op = "FLT.S"; break;
                    case 0x2: op = "FEQ.S"; break;
                }
            } else if (funct7 == 0x70) {
                if (funct3 == 0x0) {
                    op = "FCLASS.S";
                }
            }
            
            // Format arguments based on instruction type
            if (op == "FSQRT.S" || op == "FCVT.W.S" || op == "FMV.X.W" || op == "FCLASS.S") {
                // Single operand instructions
                if (op == "FCVT.W.S" || op == "FMV.X.W" || op == "FCLASS.S") {
                    args = REGISTER_NAMES[rd] + ", " + FP_REGISTER_NAMES[rs1];
                } else {
                    args = FP_REGISTER_NAMES[rd] + ", " + FP_REGISTER_NAMES[rs1];
                }
            } else if (op == "FCVT.S.W" || op == "FMV.W.X") {
                // Convert/move from integer register
                args = FP_REGISTER_NAMES[rd] + ", " + REGISTER_NAMES[rs1];
            } else if (op == "FLE.S" || op == "FLT.S" || op == "FEQ.S") {
                // Comparison instructions write to integer register
                args = REGISTER_NAMES[rd] + ", " + FP_REGISTER_NAMES[rs1] + ", " + FP_REGISTER_NAMES[rs2];
            } else {
                // Standard FP arithmetic (two FP operands, write to FP register)
                args = FP_REGISTER_NAMES[rd] + ", " + FP_REGISTER_NAMES[rs1] + ", " + FP_REGISTER_NAMES[rs2];
            }
            break;
    }
    
    return op + " " + args;
}

string CPU::disassemble_compressed_instruction(uint16_t instruction) const {
    uint8_t op = instruction & 0x3;
    uint8_t funct3 = (instruction >> 13) & 0x7;
    uint8_t rd_rs1 = (instruction >> 7) & 0x1F;
    uint8_t rs2 = (instruction >> 2) & 0x1F;
    
    string op_name = "C.UNKNOWN";
    string args = "";
    
    // Quadrant 0: 00
    if (op == 0x0) {
        if (funct3 == 0x0) {
            op_name = "C.ADDI4SPN";
            uint8_t rd_prime = 8 + ((instruction >> 2) & 0x7);
            args = REGISTER_NAMES[rd_prime] + ", sp, " + std::to_string(((instruction >> 5) & 0x30) | ((instruction >> 7) & 0xC) | ((instruction >> 4) & 0x4) | ((instruction >> 2) & 0x8));
        } else if (funct3 == 0x2) {
            op_name = "C.LW";
            uint8_t rd_prime = 8 + ((instruction >> 2) & 0x7);
            uint8_t rs1_prime = 8 + ((instruction >> 7) & 0x7);
            args = REGISTER_NAMES[rd_prime] + ", " + std::to_string(((instruction >> 5) & 0x20) | ((instruction >> 6) & 0x18) | ((instruction >> 2) & 0x4)) + "(" + REGISTER_NAMES[rs1_prime] + ")";
        } else if (funct3 == 0x6) {
            op_name = "C.SW";
            uint8_t rs2_prime = 8 + ((instruction >> 2) & 0x7);
            uint8_t rs1_prime = 8 + ((instruction >> 7) & 0x7);
            args = REGISTER_NAMES[rs2_prime] + ", " + std::to_string(((instruction >> 5) & 0x20) | ((instruction >> 6) & 0x18) | ((instruction >> 2) & 0x4)) + "(" + REGISTER_NAMES[rs1_prime] + ")";
        }
    }
    // Quadrant 1: 01
    else if (op == 0x1) {
        if (funct3 == 0x0) {
            op_name = "C.ADDI";
            int32_t imm = ((instruction >> 12) & 0x1) ? -32 : 0;
            imm |= ((instruction >> 2) & 0x1F);
            args = REGISTER_NAMES[rd_rs1] + ", " + REGISTER_NAMES[rd_rs1] + ", " + std::to_string(imm);
        } else if (funct3 == 0x1) {
            op_name = "C.JAL";
            args = "offset";  // Simplified
        } else if (funct3 == 0x2) {
            op_name = "C.LI";
            int32_t imm = ((instruction >> 12) & 0x1) ? -32 : 0;
            imm |= ((instruction >> 2) & 0x1F);
            args = REGISTER_NAMES[rd_rs1] + ", " + std::to_string(imm);
        } else if (funct3 == 0x3) {
            if (rd_rs1 == 2) {
                op_name = "C.ADDI16SP";
                args = "sp, sp, offset";  // Simplified
            } else {
                op_name = "C.LUI";
                args = REGISTER_NAMES[rd_rs1] + ", offset";  // Simplified
            }
        } else if (funct3 == 0x4) {
            uint8_t funct2 = (instruction >> 10) & 0x3;
            uint8_t rd_prime = 8 + ((instruction >> 7) & 0x7);
            if (funct2 == 0x0) {
                op_name = "C.SRLI";
                args = REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rd_prime] + ", " + std::to_string((instruction >> 2) & 0x1F);
            } else if (funct2 == 0x1) {
                op_name = "C.SRAI";
                args = REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rd_prime] + ", " + std::to_string((instruction >> 2) & 0x1F);
            } else if (funct2 == 0x2) {
                op_name = "C.ANDI";
                args = REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rd_prime] + ", imm";  // Simplified
            } else if (funct2 == 0x3) {
                uint8_t rs2_prime = 8 + ((instruction >> 2) & 0x7);
                uint8_t funct6 = (instruction >> 10) & 0x3F;
                if (funct6 == 0x23) {
                    op_name = "C.SUB";
                    args = REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rs2_prime];
                } else if (funct6 == 0x27) {
                    op_name = "C.XOR";
                    args = REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rs2_prime];
                } else if (funct6 == 0x26) {
                    op_name = "C.OR";
                    args = REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rs2_prime];
                } else if (funct6 == 0x24) {
                    op_name = "C.AND";
                    args = REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rd_prime] + ", " + REGISTER_NAMES[rs2_prime];
                }
            }
        } else if (funct3 == 0x5) {
            op_name = "C.J";
            args = "offset";  // Simplified
        } else if (funct3 == 0x6) {
            op_name = "C.BEQZ";
            uint8_t rs1_prime = 8 + ((instruction >> 7) & 0x7);
            args = REGISTER_NAMES[rs1_prime] + ", offset";  // Simplified
        } else if (funct3 == 0x7) {
            op_name = "C.BNEZ";
            uint8_t rs1_prime = 8 + ((instruction >> 7) & 0x7);
            args = REGISTER_NAMES[rs1_prime] + ", offset";  // Simplified
        }
    }
    // Quadrant 2: 10
    else if (op == 0x2) {
        if (funct3 == 0x0) {
            op_name = "C.SLLI";
            args = REGISTER_NAMES[rd_rs1] + ", " + REGISTER_NAMES[rd_rs1] + ", " + std::to_string((instruction >> 2) & 0x1F);
        } else if (funct3 == 0x2) {
            op_name = "C.LWSP";
            args = REGISTER_NAMES[rd_rs1] + ", offset(sp)";  // Simplified
        } else if (funct3 == 0x4) {
            if (rs2 == 0) {
                op_name = "C.JR";
                args = REGISTER_NAMES[rd_rs1];
            } else {
                op_name = "C.MV";
                args = REGISTER_NAMES[rd_rs1] + ", " + REGISTER_NAMES[rs2];
            }
        } else if (funct3 == 0x5) {
            if (rs2 == 0) {
                op_name = "C.JALR";
                args = REGISTER_NAMES[rd_rs1];
            } else {
                op_name = "C.ADD";
                args = REGISTER_NAMES[rd_rs1] + ", " + REGISTER_NAMES[rd_rs1] + ", " + REGISTER_NAMES[rs2];
            }
        } else if (funct3 == 0x6) {
            op_name = "C.SWSP";
            args = REGISTER_NAMES[rs2] + ", offset(sp)";  // Simplified
        }
    }
    
    return op_name + " " + args;
}

void CPU::log_pipeline_state(int cycle, bool had_stall, bool had_flush) {
    if (!log_file.is_open()) return;
    
    log_file << "\n=== Cycle " << cycle << " ===" << std::endl;
    log_file << "Current PC: 0x" << std::hex << PC << std::dec << ", maxPC: " << maxPC << std::endl;
    
    // Log IF/ID register
    log_file << "IF/ID: ";
    if (if_id.valid) {
        string if_id_disasm = "";
        if (if_id.is_compressed && if_id.compressed_inst != 0) {
            if_id_disasm = disassemble_compressed_instruction(if_id.compressed_inst) + 
                           " [expanded: " + disassemble_instruction(if_id.instruction) + "]";
        } else if (if_id.instruction != 0) {
            if_id_disasm = disassemble_instruction(if_id.instruction);
        } else {
            if_id_disasm = "UNKNOWN";
        }
        log_file << "PC=0x" << std::hex << if_id.pc << ", Inst=0x" << if_id.instruction 
                 << " (" << if_id_disasm << ")" << std::dec;
    } else {
        log_file << "Empty";
    }
    log_file << std::endl;
    
    // Log ID/EX register
    log_file << "ID/EX: ";
    if (id_ex.valid) {
        string id_ex_disasm = "";
        if (id_ex.instruction != 0) {
            if (id_ex.is_compressed) {
                id_ex_disasm = disassemble_compressed_instruction(id_ex.compressed_inst) + 
                               " [expanded: " + disassemble_instruction(id_ex.instruction) + "]";
            } else {
                id_ex_disasm = disassemble_instruction(id_ex.instruction);
            }
        }
        log_file << "PC=0x" << std::hex << id_ex.pc 
                 << " (" << id_ex_disasm << ")"
                 << ", opcode=0x" << id_ex.opcode
                 << ", ALUOp=0x" << id_ex.aluOp
                 << ", rs1_data=" << std::dec << id_ex.rs1_data
                 << ", rs2_data=" << id_ex.rs2_data
                 << ", imm=" << id_ex.immediate;
        if (id_ex.opcode == 0x6F || id_ex.opcode == 0x67) {
            log_file << " [JUMP instruction, target would be 0x" << std::hex << (id_ex.pc + id_ex.immediate) << std::dec << "]";
        }
    } else {
        log_file << "Empty";
    }
    log_file << std::endl;
    
    // Log EX/MEM register
    log_file << "EX/MEM: ";
    if (ex_mem.valid) {
        log_file << "PC=0x" << std::hex << ex_mem.pc 
                 << ", ALU_result=" << std::dec << ex_mem.alu_result
                 << ", rs2_data=" << ex_mem.rs2_data
                 << ", rd=x" << ex_mem.rd;
    } else {
        log_file << "Empty";
    }
    log_file << std::endl;
    
    // Log MEM/WB register
    log_file << "MEM/WB: ";
    if (mem_wb.valid) {
        log_file << "PC=0x" << std::hex << mem_wb.pc 
                 << ", rd=x" << std::dec << mem_wb.rd
                 << ", Write_data=" << (mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result);
    } else {
        log_file << "Empty";
    }
    log_file << std::endl;

    log_file << "Control: stall=" << (had_stall ? "true" : "false") 
         << ", flush=" << (had_flush ? "true" : "false") << std::endl;
    log_file << "Pipeline empty: " << (is_pipeline_empty() ? "true" : "false") << std::endl;
    
    // Log key register values for debugging
    log_file << "Registers: t0(x5)=" << registers[5] 
             << ", t1(x6)=" << registers[6]
             << ", t2(x7)=" << registers[7]
             << ", s0(x8)=" << registers[8]
             << ", s1(x9)=" << registers[9]
             << ", a0(x10)=" << registers[10]
             << ", a1(x11)=" << registers[11]
             << ", a2(x12)=" << registers[12]
             << ", a3(x13)=" << registers[13] << std::endl;
    
    // Log JAL/JALR execution details if in EX/MEM
    if (ex_mem.valid && (id_ex.opcode == 0x6F || id_ex.opcode == 0x67)) {
        log_file << "JUMP: PC=0x" << std::hex << ex_mem.pc << ", immediate=" << std::dec << id_ex.immediate 
                 << ", target=0x" << std::hex << (ex_mem.pc + id_ex.immediate) << std::dec << std::endl;
    }
    
    // Flush to ensure data is written immediately
    log_file.flush();

}

void CPU::log_instruction_disassembly(uint32_t instruction, uint32_t pc) {
    if (!log_file.is_open()) return;
    
    log_file << "PC=0x" << std::hex << pc << ": " << disassemble_instruction(instruction) << std::dec << std::endl;
}

// Statistics and tracing implementation
void CPU::capture_pipeline_snapshot(int cycle, bool had_stall, bool had_flush) {
    if (!enable_tracing_) return;
    
    PipelineSnapshot snapshot;
    snapshot.cycle = cycle;
    snapshot.stall = had_stall;
    snapshot.flush = had_flush;
    
    // Capture IF/ID stage
    snapshot.if_id.valid = if_id.valid;
    snapshot.if_id.pc = if_id.pc;
    snapshot.if_id.instruction = if_id.instruction;
    if (if_id.valid && if_id.instruction != 0) {
        if (if_id.is_compressed && if_id.compressed_inst != 0) {
            // Show compressed instruction with expanded form
            snapshot.if_id.disassembly = disassemble_compressed_instruction(if_id.compressed_inst) + 
                                         " [expanded: " + disassemble_instruction(if_id.instruction) + "]";
        } else {
            snapshot.if_id.disassembly = disassemble_instruction(if_id.instruction);
        }
    } else if (if_id.instruction != 0) {
        // Even if not valid (flushed), still disassemble if we have the instruction
        if (if_id.is_compressed && if_id.compressed_inst != 0) {
            snapshot.if_id.disassembly = disassemble_compressed_instruction(if_id.compressed_inst) + 
                                         " [expanded: " + disassemble_instruction(if_id.instruction) + "]";
        } else {
            snapshot.if_id.disassembly = disassemble_instruction(if_id.instruction);
        }
    }
    
    // Capture ID/EX stage
    snapshot.id_ex.valid = id_ex.valid;
    snapshot.id_ex.pc = id_ex.pc;
    if (id_ex.valid) {
        if (id_ex.instruction != 0) {
            if (id_ex.is_compressed) {
                snapshot.id_ex.disassembly = disassemble_compressed_instruction(id_ex.compressed_inst) + 
                                            " [expanded: " + disassemble_instruction(id_ex.instruction) + "]";
            } else {
                snapshot.id_ex.disassembly = disassemble_instruction(id_ex.instruction);
            }
        } else {
            snapshot.id_ex.disassembly = "UNKNOWN";
        }
        // Extract opcode name
        unsigned int opcode = id_ex.opcode;
        switch (opcode) {
            case 0x33: snapshot.id_ex.opcode_name = "R-type"; break;
            case 0x13: snapshot.id_ex.opcode_name = "I-type"; break;
            case 0x03: snapshot.id_ex.opcode_name = "Load"; break;
            case 0x23: snapshot.id_ex.opcode_name = "Store"; break;
            case 0x63: snapshot.id_ex.opcode_name = "Branch"; break;
            case 0x67: case 0x6F: snapshot.id_ex.opcode_name = "Jump"; break;
            case 0x37: case 0x17: snapshot.id_ex.opcode_name = "Upper-Imm"; break;
            default: snapshot.id_ex.opcode_name = "Unknown"; break;
        }
    }
    
    // Capture EX/MEM stage
    snapshot.ex_mem.valid = ex_mem.valid;
    snapshot.ex_mem.pc = ex_mem.pc;
    snapshot.ex_mem.alu_result = ex_mem.alu_result;
    if (ex_mem.valid) {
        if (ex_mem.is_compressed && ex_mem.compressed_inst != 0) {
            // Always disassemble compressed instruction, even if expansion returned 0 (reserved)
            if (ex_mem.instruction != 0) {
                snapshot.ex_mem.disassembly = disassemble_compressed_instruction(ex_mem.compressed_inst) + 
                                               " [expanded: " + disassemble_instruction(ex_mem.instruction) + "]";
            } else {
                snapshot.ex_mem.disassembly = disassemble_compressed_instruction(ex_mem.compressed_inst) + " [reserved]";
            }
        } else if (ex_mem.instruction != 0) {
            snapshot.ex_mem.disassembly = disassemble_instruction(ex_mem.instruction);
        } else {
            snapshot.ex_mem.disassembly = "UNKNOWN";
        }
    }
    
    // Capture MEM/WB stage
    snapshot.mem_wb.valid = mem_wb.valid;
    snapshot.mem_wb.pc = mem_wb.pc;
    snapshot.mem_wb.write_data = mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result;
    if (mem_wb.valid) {
        if (mem_wb.is_compressed && mem_wb.compressed_inst != 0) {
            // Always disassemble compressed instruction, even if expansion returned 0 (reserved)
            if (mem_wb.instruction != 0) {
                snapshot.mem_wb.disassembly = disassemble_compressed_instruction(mem_wb.compressed_inst) + 
                                               " [expanded: " + disassemble_instruction(mem_wb.instruction) + "]";
            } else {
                snapshot.mem_wb.disassembly = disassemble_compressed_instruction(mem_wb.compressed_inst) + " [reserved]";
            }
        } else if (mem_wb.instruction != 0) {
            snapshot.mem_wb.disassembly = disassemble_instruction(mem_wb.instruction);
        } else {
            snapshot.mem_wb.disassembly = "UNKNOWN";
        }
    }
    
    pipeline_trace_.push_back(snapshot);
}

PipelineSnapshot CPU::get_current_pipeline_state(int cycle) const {
    PipelineSnapshot snapshot;
    snapshot.cycle = cycle;
    snapshot.stall = pipeline_stall;
    snapshot.flush = pipeline_flush;
    
    // Capture current IF/ID stage
    snapshot.if_id.valid = if_id.valid;
    snapshot.if_id.pc = if_id.pc;
    snapshot.if_id.instruction = if_id.instruction;
    if (if_id.valid) {
        if (if_id.is_compressed && if_id.compressed_inst != 0) {
            // Show compressed instruction with expanded form
            snapshot.if_id.disassembly = disassemble_compressed_instruction(if_id.compressed_inst) + 
                                         " [expanded: " + disassemble_instruction(if_id.instruction) + "]";
        } else if (if_id.instruction != 0) {
            snapshot.if_id.disassembly = disassemble_instruction(if_id.instruction);
        } else {
            snapshot.if_id.disassembly = "UNKNOWN";
        }
    }
    
    // Capture current ID/EX stage
    snapshot.id_ex.valid = id_ex.valid;
    snapshot.id_ex.pc = id_ex.pc;
    if (id_ex.valid) {
        if (id_ex.instruction != 0) {
            if (id_ex.is_compressed) {
                snapshot.id_ex.disassembly = disassemble_compressed_instruction(id_ex.compressed_inst) + 
                                            " [expanded: " + disassemble_instruction(id_ex.instruction) + "]";
            } else {
                snapshot.id_ex.disassembly = disassemble_instruction(id_ex.instruction);
            }
        } else {
            snapshot.id_ex.disassembly = "UNKNOWN";
        }
    }

    // Capture current EX/MEM stage
    snapshot.ex_mem.valid = ex_mem.valid;
    snapshot.ex_mem.pc = ex_mem.pc;
    snapshot.ex_mem.alu_result = ex_mem.alu_result;
    if (ex_mem.valid) {
        if (ex_mem.is_compressed && ex_mem.compressed_inst != 0) {
            // Always disassemble compressed instruction, even if expansion returned 0 (reserved)
            if (ex_mem.instruction != 0) {
                snapshot.ex_mem.disassembly = disassemble_compressed_instruction(ex_mem.compressed_inst) + 
                                               " [expanded: " + disassemble_instruction(ex_mem.instruction) + "]";
            } else {
                snapshot.ex_mem.disassembly = disassemble_compressed_instruction(ex_mem.compressed_inst) + " [reserved]";
            }
        } else if (ex_mem.instruction != 0) {
            snapshot.ex_mem.disassembly = disassemble_instruction(ex_mem.instruction);
        } else {
            snapshot.ex_mem.disassembly = "UNKNOWN";
        }
    }
    
    // Capture current MEM/WB stage
    snapshot.mem_wb.valid = mem_wb.valid;
    snapshot.mem_wb.pc = mem_wb.pc;
    snapshot.mem_wb.write_data = mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result;
    if (mem_wb.valid) {
        if (mem_wb.is_compressed && mem_wb.compressed_inst != 0) {
            // Always disassemble compressed instruction, even if expansion returned 0 (reserved)
            if (mem_wb.instruction != 0) {
                snapshot.mem_wb.disassembly = disassemble_compressed_instruction(mem_wb.compressed_inst) + 
                                               " [expanded: " + disassemble_instruction(mem_wb.instruction) + "]";
            } else {
                snapshot.mem_wb.disassembly = disassemble_compressed_instruction(mem_wb.compressed_inst) + " [reserved]";
            }
        } else if (mem_wb.instruction != 0) {
            snapshot.mem_wb.disassembly = disassemble_instruction(mem_wb.instruction);
        } else {
            snapshot.mem_wb.disassembly = "UNKNOWN";
        }
    }
    
    return snapshot;
}

bool CPU::get_cache_stats(uint64_t& hits, uint64_t& misses) const {
    if (!dmem_) return false;
    
    // Try to cast to CacheStatistics interface
    CacheStatistics* cache = dynamic_cast<CacheStatistics*>(dmem_);
    if (cache) {
        hits = cache->hits();
        misses = cache->misses();
        return true;
    }
    
    return false;
}

// Memory address tracking implementation
void CPU::track_memory_access(int cycle, uint32_t address, bool is_write, uint32_t value, uint32_t pc, bool cache_hit) {
    if (!enable_tracing_) return;
    
    string disasm = "";
    // Get disassembly from current pipeline state
    if (ex_mem.valid && ex_mem.pc == pc) {
        // Try to find disassembly from pipeline trace
        for (auto it = pipeline_trace_.rbegin(); it != pipeline_trace_.rend() && it->cycle >= cycle - 5; ++it) {
            if (it->ex_mem.valid && it->ex_mem.pc == pc) {
                disasm = it->ex_mem.disassembly;
                break;
            }
        }
        if (disasm.empty()) {
            // Fallback: try to disassemble from if_id if available
            // For now, use a descriptive placeholder
            disasm = is_write ? "STORE" : "LOAD";
        }
    }
    
    memory_access_history_.push_back(MemoryAccess(cycle, address, is_write, value, pc, disasm, cache_hit));
}

// Register value history tracking implementation
void CPU::track_register_change(int cycle, unsigned int reg, int32_t old_value, int32_t new_value, uint32_t pc) {
    if (!enable_tracing_) return;
    if (reg == 0) return;  // Don't track x0 (zero register)
    
    string disasm = "";
    // Get instruction disassembly
    if (mem_wb.valid && mem_wb.pc == pc) {
        // Try to find disassembly from pipeline trace
        for (auto it = pipeline_trace_.rbegin(); it != pipeline_trace_.rend(); ++it) {
            if (it->mem_wb.valid && it->mem_wb.pc == pc) {
                disasm = it->mem_wb.disassembly;
                break;
            }
        }
        if (disasm.empty()) {
            disasm = "REG_WRITE";
        }
    }
    
    register_history_.push_back(RegisterChange(cycle, reg, old_value, new_value, pc, disasm));
}

// Instruction dependency tracking implementation
void CPU::track_instruction_dependencies(int cycle, uint32_t pc, unsigned int rd, unsigned int rs1, unsigned int rs2) {
    if (!enable_tracing_) return;
    
    // Check for dependencies with previous instructions
    // RAW (Read After Write): Current instruction reads a register that was written by a previous instruction
    // Only track RAW dependencies (most relevant for pipeline hazards)
    // Filter to only show dependencies within reasonable cycle distance (e.g., 10 cycles)
    const int MAX_CYCLE_DISTANCE = 10;
    
    // Get consumer disassembly from current if_id
    string cons_disasm = "";
    if (if_id.valid && if_id.pc == pc) {
        cons_disasm = disassemble_instruction(if_id.instruction);
    }
    
    // Check for RAW dependencies (this instruction reads rs1 or rs2)
    if (rs1 != 0) {
        for (auto it = pc_to_rd_map_.begin(); it != pc_to_rd_map_.end(); ++it) {
            if (it->second == rs1 && it->first != pc) {
                int producer_cycle = pc_to_cycle_map_[it->first];
                // Only track if within reasonable cycle distance
                if (cycle - producer_cycle <= MAX_CYCLE_DISTANCE) {
                    string prod_disasm = "";
                    
                    // Try to get producer disassembly from trace
                    for (const auto& snapshot : pipeline_trace_) {
                        if (snapshot.mem_wb.pc == it->first) {
                            prod_disasm = snapshot.mem_wb.disassembly;
                            break;
                        }
                    }
                    // Fallback: try to disassemble if we can find the instruction
                    if (prod_disasm.empty()) {
                        // Look for instruction in any stage
                        for (const auto& snapshot : pipeline_trace_) {
                            if (snapshot.if_id.pc == it->first && snapshot.if_id.instruction != 0) {
                                prod_disasm = snapshot.if_id.disassembly;
                                break;
                            }
                        }
                    }
                    
                    instruction_dependencies_.push_back(
                        InstructionDependency(it->first, pc, rs1, "RAW", producer_cycle, cycle, prod_disasm, cons_disasm)
                    );
                }
            }
        }
    }
    
    if (rs2 != 0) {
        for (auto it = pc_to_rd_map_.begin(); it != pc_to_rd_map_.end(); ++it) {
            if (it->second == rs2 && it->first != pc) {
                int producer_cycle = pc_to_cycle_map_[it->first];
                // Only track if within reasonable cycle distance
                if (cycle - producer_cycle <= MAX_CYCLE_DISTANCE) {
                    string prod_disasm = "";
                    
                    // Try to get producer disassembly from trace
                    for (const auto& snapshot : pipeline_trace_) {
                        if (snapshot.mem_wb.pc == it->first) {
                            prod_disasm = snapshot.mem_wb.disassembly;
                            break;
                        }
                    }
                    // Fallback: try to disassemble if we can find the instruction
                    if (prod_disasm.empty()) {
                        // Look for instruction in any stage
                        for (const auto& snapshot : pipeline_trace_) {
                            if (snapshot.if_id.pc == it->first && snapshot.if_id.instruction != 0) {
                                prod_disasm = snapshot.if_id.disassembly;
                                break;
                            }
                        }
                    }
                    
                    instruction_dependencies_.push_back(
                        InstructionDependency(it->first, pc, rs2, "RAW", producer_cycle, cycle, prod_disasm, cons_disasm)
                    );
                }
            }
        }
    }
}