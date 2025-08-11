// file: CPU.cpp

#include "CPU.h"
#include <iomanip>
#include <iostream>


CPU::CPU()
{
	PC = 0; //set PC to 0
	for (int i = 0; i < 4096; i++) { dmemory[i] = 0; }

	for (int i = 0; i < 32; i++) {
		registers[i] = 0;
	}
	
	// Initialize pipeline control
	pipeline_stall = false;
	pipeline_flush = false;
	maxPC = 0;
	enable_logging = false;
}

CPU::~CPU()
{
	if (log_file.is_open()) {
		log_file.close();
	}
}


unsigned long CPU::readPC()
{
	return PC;
}
void CPU::incPC()
{
	PC += 4;
}

// returns the current instruction as a string
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

    // Decode based on opcode
    switch (*opcode) {
        case 0x33: // R-type (ADD, SUB, OR, XOR, SLL, SRL, SRA, SLT, SLTU)
            *regWrite = true;
            *aluSrc = false;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;
            
            if (*funct3 == 0x0 && *funct7 == 0x00) { // ADD
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
int32_t CPU::generate_immediate(uint32_t instruction, int opcode) {
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
            // Immediate is split into multiple parts
            imm = ((instruction >> 19) & 0x1000) | // imm[12]
                  ((instruction << 4) & 0x800) |    // imm[11]
                  ((instruction >> 20) & 0x7E0) |   // imm[10:5]
                  ((instruction >> 7) & 0x1E);      // imm[4:1]
            imm = sign_extend(imm, 13);
            imm <<= 1; // B-type encodes bit0=0; produce byte offset here
            break;
            
        case 0x6F: // JAL
            // 20-bit immediate
            imm = ((instruction >> 11) & 0x100000) | // imm[20]
                  (instruction & 0xFF000) |           // imm[19:12]
                  ((instruction >> 9) & 0x800) |      // imm[11]
                  ((instruction >> 20) & 0x7FE);      // imm[10:1]
            imm = sign_extend(imm, 21);
            imm <<= 1; // UJ-type encodes bit0=0; produce byte offset here
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
int32_t CPU::sign_extend(int32_t value, int bits) {
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

// Memory read operation
int32_t CPU::read_memory(uint32_t address, int type) {
    // type: 1=LB (byte, sign extended), 2=LBU (byte, zero extended), 
    //       3=LH (halfword, sign extended), 4=LHU (halfword, zero extended), 5=LW (word)
    
    uint32_t bytes = (type == 1 || type == 2) ? 1 : (type == 3 || type == 4) ? 2 : 4;
    
    if (!check_address_alignment(address, bytes)) {
        return 0;
    }
    
    if (type == 1) { // LB - Load byte (sign extended)
        int8_t byte_val = static_cast<int8_t>(dmemory[address]);
        return static_cast<int32_t>(byte_val);
    } else if (type == 2) { // LBU - Load byte unsigned (zero extended)
        uint8_t byte_val = static_cast<uint8_t>(dmemory[address]);
        return static_cast<int32_t>(byte_val);
    } else if (type == 3) { // LH - Load halfword (sign extended)
        int16_t halfword = 0;
        for (int i = 0; i < 2; i++) {
            halfword |= static_cast<uint16_t>(dmemory[address + i]) << (i * 8);
        }
        return static_cast<int32_t>(halfword);
    } else if (type == 4) { // LHU - Load halfword unsigned (zero extended)
        uint16_t halfword = 0;
        for (int i = 0; i < 2; i++) {
            halfword |= static_cast<uint16_t>(dmemory[address + i]) << (i * 8);
        }
        return static_cast<int32_t>(halfword);
    } else if (type == 5) { // LW - Load word
        int32_t word = 0;
        for (int i = 0; i < 4; i++) {
            word |= static_cast<int32_t>(dmemory[address + i]) << (i * 8);
        }
        return word;
    }
    
    return 0; // Invalid type
}

// Memory write operation
void CPU::write_memory(uint32_t address, int32_t value, int type) {
    // type: 1=SB (byte), 2=SH (halfword), 3=SW (word)
    
    uint32_t bytes = (type == 1) ? 1 : (type == 2) ? 2 : 4;
    
    if (!check_address_alignment(address, bytes)) {
        return;
    }
    
    if (type == 1) { // SB - Store byte
        dmemory[address] = static_cast<uint8_t>(value & 0xFF);
    } else if (type == 2) { // SH - Store halfword
        for (int i = 0; i < 2; i++) {
            dmemory[address + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }
    } else if (type == 3) { // SW - Store word
        for (int i = 0; i < 4; i++) {
            dmemory[address + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }
    }
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
        if (debug) {
            std::cout << "IF: Pipeline stalled, no instruction fetched" << std::endl;
        }
        return;
    }

    if (pipeline_flush) {
        if_id.valid = false;
        pipeline_flush = false;
        if (debug) {
            std::cout << "IF: Flushed due to branch" << std::endl;
        }
        return;
    }

    if (PC >= maxPC) {
        if_id.valid = false;
        if (debug) {
            std::cout << "IF: End of program reached at PC " << PC << std::endl;
        }
        return;
    }

    string inst_str = get_instruction(instMem);
    if (inst_str == "00000000") {
        if_id.valid = false;
        if (debug) {
            std::cout << "IF: NOP instruction (all zeros)" << std::endl;
        }
        return;
    }

    if_id.instruction = std::stoul(inst_str, nullptr, 16);
    if_id.pc = PC;
    if_id.valid = true;

    if (debug) {
        std::cout << "IF: Fetched instruction 0x" << std::hex << if_id.instruction 
                  << " at PC 0x" << if_id.pc << std::dec << std::endl;
        std::cout << "IF: Raw instruction string: " << inst_str << std::endl;
    }

    incPC();
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

    // Load-use hazard detection
    if (id_ex.memRe && (
        (id_ex.rd != 0 && id_ex.rd == rs1) || 
        (id_ex.rd != 0 && id_ex.rd == rs2))) {

        pipeline_stall = true;
        if (debug) {
            std::cout << "ID: Load-use hazard detected. Stalling pipeline." << std::endl;
        }
        return;
    }

    if (!valid) {
        id_ex.valid = false;
        if (debug) {
            std::cout << "ID: Invalid instruction decoded" << std::endl;
        }
        return;
    }

    // Read register values
    int32_t rs1_data = (rs1 != 0) ? get_register_value(rs1) : 0;
    int32_t rs2_data = (rs2 != 0) ? get_register_value(rs2) : 0;

    // Generate immediate value
    int32_t immediate = generate_immediate(if_id.instruction, opcode);

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
    id_ex.immediate = immediate;
    id_ex.pc = if_id.pc;
    id_ex.valid = true;

    if (debug) {
        std::cout << "ID: Decoded instruction - " << disassemble_instruction(if_id.instruction) << std::endl;
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

    // Call ALU
    int32_t alu_result = alu.execute(operand1, operand2, id_ex.aluOp);

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
        ex_mem.valid      = true;

        PC = id_ex.pc + id_ex.immediate;
        pipeline_flush = true;
        if (debug) { std::cout << "EX: JAL taken to " << PC << ", link=" << (id_ex.pc+4) << std::endl; }
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
        ex_mem.valid      = true;

        PC = static_cast<uint32_t>(target);
        pipeline_flush = true;
        if (debug) { std::cout << "EX: JALR taken to " << PC << ", link=" << (id_ex.pc+4) << std::endl; }
        return;
    }

    // Branch decision
    if (id_ex.branch) {
        bool should_branch = false;
        switch (id_ex.aluOp) {
            case 0x30: should_branch = alu.isZero(); break; // BEQ
            case 0x35: should_branch = !alu.isZero(); break; // BNE
            case 0x31: should_branch = alu.isZero(); break;  // BGE
            case 0x33: should_branch = !alu.isZero(); break; // BLT
            case 0x32: should_branch = alu.isZero(); break;  // BGEU
            case 0x34: should_branch = !alu.isZero(); break; // BLTU
        }

        if (should_branch) {
            // immediate already byte-scaled in generate_immediate()
            PC = id_ex.pc + id_ex.immediate;
            pipeline_flush = true;

            if (debug) {
                std::cout << "EX: Branch taken. Flushing pipeline." << std::endl;
            }
        }
    }

    // Update EX/MEM register
    ex_mem.regWrite = id_ex.regWrite;
    ex_mem.memRe = id_ex.memRe;
    ex_mem.memWr = id_ex.memWr;
    ex_mem.memToReg = id_ex.memToReg;
    ex_mem.memReadType  = id_ex.memReadType;
    ex_mem.memWriteType = id_ex.memWriteType;
    ex_mem.alu_result = alu_result;
    ex_mem.rs2_data = id_ex.rs2_data;
    ex_mem.rd = id_ex.rd;
    ex_mem.pc = id_ex.pc;
    ex_mem.valid = true;

    if (debug) {
        std::cout << "EX: ALU operation - " << operand1 << " op " << operand2 << " = " << alu_result << std::endl;

        if (id_ex.branch) {
            std::cout << "EX: Branch instruction ";
            std::cout << (alu.isZero() ? "taken" : "not taken");
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
    
    // Memory operations
    if (ex_mem.memRe) {
        // Load operation
        mem_data = read_memory(ex_mem.alu_result, ex_mem.memReadType);
        if (debug) {
            std::cout << "MEM: Load from address " << ex_mem.alu_result << " = " << mem_data << std::endl;
        }
    } else if (ex_mem.memWr) {
        // Store operation
        write_memory(ex_mem.alu_result, ex_mem.rs2_data, ex_mem.memWriteType);
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
        registers[mem_wb.rd] = write_data;
        
        if (debug) {
            std::cout << std::dec;
            std::cout << "WB: Write " << write_data << " to register " << REGISTER_NAMES[mem_wb.rd] << std::endl;
        }
    }
}

void CPU::run_pipeline_cycle(char* instMem, int cycle, bool debug) {
    if (debug) {
        std::cout << "\n=== Cycle " << cycle << " ===" << std::endl;
    }

    // Take snapshots so EX can see last cycle's values
    ex_mem_prev = ex_mem;
    mem_wb_prev = mem_wb;

    write_back_stage(debug);
    memory_stage(debug);
    execute_stage(debug);
    instruction_decode(debug);
    instruction_fetch(instMem, debug);

    if (enable_logging) {
        log_pipeline_state(cycle);
    }

    pipeline_stall = false;
}



void CPU::set_logging(bool enable, string log_filename) {
    enable_logging = enable;
    if (enable && !log_filename.empty()) {
        log_file.open(log_filename);
        if (log_file.is_open()) {
            log_file << "Pipeline Execution Log" << std::endl;
            log_file << "=====================" << std::endl;
        }
    }
}

bool CPU::is_pipeline_empty() {
    return !if_id.valid && !id_ex.valid && !ex_mem.valid && !mem_wb.valid;
}

void CPU::set_max_pc(int max_pc) {
    maxPC = max_pc;
}

string CPU::disassemble_instruction(uint32_t instruction) {
    unsigned int opcode = instruction & 0x7F;
    unsigned int rd = (instruction >> 7) & 0x1F;
    unsigned int funct3 = (instruction >> 12) & 0x7;
    unsigned int rs1 = (instruction >> 15) & 0x1F;
    unsigned int rs2 = (instruction >> 20) & 0x1F;
    unsigned int funct7 = (instruction >> 25) & 0x7F;
    
    string op = "UNKNOWN";
    string args = "";
    
    switch (opcode) {
        case 0x33: // R-type
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
                case 0x4: op = "BLT"; break;
                case 0x5: op = "BGE"; break;
                case 0x6: op = "BLTU"; break;
                case 0x7: op = "BGEU"; break;
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
    }
    
    return op + " " + args;
}

void CPU::log_pipeline_state(int cycle) {
    if (!log_file.is_open()) return;
    
    log_file << "\n=== Cycle " << cycle << " ===" << std::endl;
    
    // Log IF/ID register
    log_file << "IF/ID: ";
    if (if_id.valid) {
        log_file << "PC=0x" << std::hex << if_id.pc << ", Inst=0x" << if_id.instruction << std::dec;
    } else {
        log_file << "Empty";
    }
    log_file << std::endl;
    
    // Log ID/EX register
    log_file << "ID/EX: ";
    if (id_ex.valid) {
        log_file << "PC=0x" << std::hex << id_ex.pc 
                 << ", ALUOp=" << id_ex.aluOp
                 << ", rs1_data=" << id_ex.rs1_data
                 << ", rs2_data=" << id_ex.rs2_data
                 << ", imm=" << id_ex.immediate << std::dec;
    } else {
        log_file << "Empty";
    }
    log_file << std::endl;
    
    // Log EX/MEM register
    log_file << "EX/MEM: ";
    if (ex_mem.valid) {
        log_file << "PC=0x" << std::hex << ex_mem.pc 
                 << ", ALU_result=" << ex_mem.alu_result
                 << ", rs2_data=" << ex_mem.rs2_data
                 << ", rd=x" << std::dec << ex_mem.rd;
    } else {
        log_file << "Empty";
    }
    log_file << std::endl;
    
    // Log MEM/WB register
    log_file << "MEM/WB: ";
    if (mem_wb.valid) {
        log_file << "PC=0x" << std::hex << mem_wb.pc << ", Write_data=" << (mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result) << std::dec;
    } else {
        log_file << "Empty";
    }
    log_file << std::endl;

    log_file << ", rd=x" << std::dec << mem_wb.rd 
         << ", write_data=" << (mem_wb.memToReg ? mem_wb.mem_data : mem_wb.alu_result);

    log_file << "Control: stall=" << (pipeline_stall ? "true" : "false") 
         << ", flush=" << (pipeline_flush ? "true" : "false") << std::endl;

}

void CPU::log_instruction_disassembly(uint32_t instruction, uint32_t pc) {
    if (!log_file.is_open()) return;
    
    log_file << "PC=0x" << std::hex << pc << ": " << disassemble_instruction(instruction) << std::dec << std::endl;
}