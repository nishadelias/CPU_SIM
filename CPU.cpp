// file: CPU.cpp

#include "CPU.h"
#include <iomanip>


CPU::CPU()
{
	PC = 0; //set PC to 0
	for (int i = 0; i < 4096; i++) //copy instrMEM
	{
		dmemory[i] = (0);
	}

	for (int i = 0; i < 32; i++) {
		registers[i] = 0;
	}
}


unsigned long CPU::readPC()
{
	return PC;
}
void CPU::incPC()
{
    // 4 bytes is 8 hex values in the instruction array
	PC += 8;

}

// returns the current instruction as a string
string CPU::get_instruction(char *IM) {
	string inst = "";
	if (IM[PC] == '0' && IM[PC+1] == '0') {
		return "00000000";
	}
	for (int i  = 0; i < 4; i++) {
		inst += IM[PC + 6 - (i*2)];
		inst += IM[PC + 7 - (i*2)];
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
	unsigned int *opcode, unsigned int *rd, unsigned int *funct3, unsigned int *rs1, unsigned int *rs2, unsigned int *funct7) {
	    
    unsigned int instruction = std::stoul(inst, nullptr, 16);

    // Extract instruction fields
    *opcode = instruction & 0x7F;
    *rd = (instruction >> 7) & 0x1F;
    *funct3 = (instruction >> 12) & 0x7;
    *rs1 = (instruction >> 15) & 0x1F;
    *rs2 = (instruction >> 20) & 0x1F;
    *funct7 = (instruction >> 25) & 0x7F;
    
    // For debugging
        // std::cout << "Hex Instruction: " << inst << std::endl;
        // std::cout << "Decoded fields:" << std::endl;
        // std::cout << "  opcode: 0x" << std::hex << *opcode << std::endl;
        // std::cout << "  rd: " << std::dec << *rd << std::endl;
        // std::cout << "  funct3: " << std::dec << *funct3 << std::endl;
        // std::cout << "  rs1: " << std::dec << *rs1 << std::endl;
        // std::cout << "  rs2: " << std::dec << *rs2 << std::endl;
        // std::cout << "  funct7: 0x" << std::hex << *funct7 << std::endl;

    // Default control signals
    *regWrite = false;
    *aluSrc = false;
    *branch = false;
    *memRe = false;
    *memWr = false;
    *memToReg = false;
    *upperIm = false;
    *aluOp = 0;

    // Decode based on opcode
    switch (*opcode) {
        case 0x33: // R-type (ADD, XOR)
            *regWrite = true;
            *aluSrc = false;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;
            
            if (*funct3 == 0x0 && *funct7 == 0x00) { // ADD
                *aluOp = 0x0;
            }
            else if (*funct3 == 0x4) { // XOR
                *aluOp = 0x4;
            }
            break;

        case 0x13: // I-type (SRAI, ORI)
            *regWrite = true;
            *aluSrc = true;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;
            
            if (*funct3 == 0x5 && *funct7 == 0x20) { // SRAI
                *aluOp = 0x5;
            }
            else if (*funct3 == 0x6) { // ORI
                *aluOp = 0x6;
            }
            break;

        case 0x03: // Load instructions (LB, LW)
            *regWrite = true;
            *aluSrc = true;
            *branch = false;
            *memRe = true;
            *memWr = false;
            *memToReg = true;
            *upperIm = false;
            
            if (*funct3 == 0x0) { // LB
                *aluOp = 0x8;
            }
            else if (*funct3 == 0x2) { // LW
                *aluOp = 0x9;
            }
            break;

        case 0x23: // Store instructions (SB, SW)
            *regWrite = false;
            *aluSrc = true;
            *branch = false;
            *memRe = false;
            *memWr = true;
            *memToReg = false;
            *upperIm = false;
            if (*funct3 == 0x0) { // SB
                *aluOp = 0xA;
            }
            else if (*funct3 == 0x2) { // SW
                *aluOp = 0xB;
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
            *aluOp = 0xC;
            break;

        case 0x6F: // JAL
            *regWrite = true;
            *aluSrc = false;
            *branch = true;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = false;
            *aluOp = 0xD;
            break;

        case 0x37: // LUI
            *regWrite = true;
            *aluSrc = true;
            *branch = false;
            *memRe = false;
            *memWr = false;
            *memToReg = false;
            *upperIm = true;
            *aluOp = 0xE;
            break;

        case 0x00: // NULL instruction (program end)
			// cout << "Program end" << endl;
			return false;

        default:
            std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
            break;
	}
	return true;

}

// executes instructions by updating register values, loading from memory, and storing in memory
void CPU::execute(int rd, int rs1, int rs2, int aluOp, int opcode, string inst) {
    
	unsigned int instruction = std::stoul(inst, nullptr, 16);

	int32_t rs1_value = registers[rs1];  
	int32_t rs2_value = registers[rs2];
	int32_t immediate = generate_immediate(instruction, opcode);  

	// For R-type instructions
	if (opcode == 0x33) {
		int32_t result = alu.execute(rs1_value, rs2_value, aluOp);
		// cout << "R-type instruction" << endl << "Register " << rd << " is being set to " << result << endl;
		if (rd != 0)
			registers[rd] = result;
	}
	// For I-type instructions
	else if (opcode == 0x13) {
		int32_t result = alu.execute(rs1_value, immediate, aluOp);
		// cout << "I-type instruction" << endl << "Register " << rd << " is being set to " << result << endl;
		if (rd != 0)
			registers[rd] = result;
	}
	// For branches (BEQ)
	else if (opcode == 0x63) {
		alu.execute(rs1_value, rs2_value, aluOp);
		if (alu.isZero()) {

			// cout << "Branching forward " << immediate << " bytes" << endl << endl;
			PC += immediate * 2 - 8;  // Take branch
            // Subtract 8 because the incPC() will add this later
		}
	}

    // JAL
    else if (opcode == 0x6f) {
        registers[rd] = PC/2 + 4;
        PC += immediate * 2 - 8;
        // Subtract 8 because the incPC() will add this later
    }

	else if (opcode == 0x03) { // Load instructions
        // Use ALU to calculate effective address (base + offset)
        int32_t effective_address = alu.execute(registers[rs1], immediate, aluOp); // ALU_OP for address calculation
        int32_t result = 0;
        if (aluOp == 0x8) { // LB
			result = read_memory(effective_address, true);
			// cout << "Loading register " << rd << " with " << result << endl;
            registers[rd] = result;
        } else if (aluOp == 0x9) { // LW
			result = read_memory(effective_address, false);
            // cout << "Effective address: " << effective_address << endl;
			// cout << "Loading register " << rd << " with " << result << endl;
            registers[rd] = result;
        }
    }

    else if (opcode == 0x23) { // Store instructions
        // Use ALU to calculate effective address (base + offset)
        int32_t effective_address = alu.execute(registers[rs1], immediate, aluOp); // ALU_OP for address calculation

        if (aluOp == 0xa) { // SB
            // cout << "Storing byte of " << registers[rs2] << " to memory address " << effective_address << endl;
            write_memory(effective_address, registers[rs2], true);
            // cout << "Stored the value " << dmemory[effective_address] << " in memory location " << effective_address << endl;
        } else if (aluOp == 0xb) { // SW
			// cout << "Storing word of " << registers[rs2] << " to memory address " << effective_address << endl;
            write_memory(effective_address, registers[rs2], false);
            // cout << "Stored the value " << dmemory[effective_address] << " in memory location " << effective_address << endl;
        }
    }

    // LUI
	else if (opcode == 0x37) {
		if (rd != 0) { // Don't write to x0
                // For LUI, we just need to pass the immediate value through the ALU
                // The immediate generation already handled the shifting
				int32_t result = alu.execute(immediate, 0, aluOp);
				// cout << "U-type instruction" << endl << "Register " << rd << " is being set to " << result << endl;
                registers[rd] = result;
            }
	}

}

// generates immediate for the given instruction
int32_t CPU::generate_immediate(uint32_t instruction, int opcode) {
    int32_t imm = 0;
    
    switch(opcode) {
        case 0x13: // I-type (SRAI, ORI)
            imm = instruction >> 20;
            // For SRAI, only use bottom 5 bits
            if (((instruction >> 12) & 0x7) == 0x5) {
                imm = imm & 0x1F;
            } else {
                imm = sign_extend(imm, 12);
            }
            break;
            
        case 0x03: // Load instructions (LB, LW)
            imm = instruction >> 20;
            imm = sign_extend(imm, 12);
            break;
            
        case 0x23: // Store instructions (SB, SW)
            // Immediate is split into two parts
            imm = ((instruction >> 20) & 0xFE0) | ((instruction >> 7) & 0x1F);
            imm = sign_extend(imm, 12);
            break;
            
        case 0x63: // BEQ
            // Immediate is split into multiple parts
            imm = ((instruction >> 19) & 0x1000) | // imm[12]
                  ((instruction << 4) & 0x800) |    // imm[11]
                  ((instruction >> 20) & 0x7E0) |   // imm[10:5]
                  ((instruction >> 7) & 0x1E);      // imm[4:1]
            imm = sign_extend(imm, 13);
            break;
            
        case 0x6F: // JAL
            // 20-bit immediate
            imm = ((instruction >> 11) & 0x100000) | // imm[20]
                  (instruction & 0xFF000) |           // imm[19:12]
                  ((instruction >> 9) & 0x800) |      // imm[11]
                  ((instruction >> 20) & 0x7FE);      // imm[10:1]
            imm = sign_extend(imm, 21);
            break;
            
        case 0x37: // LUI
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
    
    if (bytes == 4 && (address % 4 != 0)) {
        std::cerr << "Unaligned word access at address: " << address << std::endl;
        return false;
    }
    
    return true;
}

// Memory read operation
int32_t CPU::read_memory(uint32_t address, bool is_byte) {
    if (!check_address_alignment(address, is_byte ? 1 : 4)) {
        return 0;
    }
    
    if (is_byte) {
        // Load byte (LB) - sign extend from 8 bits
        int8_t byte_val = dmemory[address];
        return static_cast<int32_t>(byte_val);
    } else {
        // Load word (LW) - little endian
        int32_t word = 0;
        for (int i = 0; i < 4; i++) {
            word |= (dmemory[address + i] & 0xFF) << (i * 8);
        }
        return word;
    }
}

// Memory write operation
void CPU::write_memory(uint32_t address, int32_t value, bool is_byte) {
    if (!check_address_alignment(address, is_byte ? 1 : 4)) {
        return;
    }
    
    if (is_byte) {
        // Store byte (SB)
        dmemory[address] = value & 0xFF;
    } else {
        // Store word (SW) - little endian
        for (int i = 0; i < 4; i++) {
            dmemory[address + i] = (value >> (i * 8)) & 0xFF;
        }
    }
}
