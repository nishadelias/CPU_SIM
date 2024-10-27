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
}


unsigned long CPU::readPC()
{
	return PC;
}
void CPU::incPC()
{
	PC++;
}

string CPU::get_instruction(char *IM) {
	string inst = "";
	if (IM[PC*8] == '0' && IM[PC*8+1] == '0') {
		return "00000000";
	}
	for (int i  = 0; i < 4; i++) {
		inst += IM[(PC*8) + 6 - (i*2)];
		inst += IM[(PC*8) + 7 - (i*2)];
	}
	return inst;
}

bool CPU::decode_instruction(string inst, bool *regWrite, bool *aluSrc, bool *branch, bool *memRe, bool *memWr, bool *memToReg, bool *upperIm, int *aluOp,
	unsigned int *rd, unsigned int *funct3, unsigned int *rs1, unsigned int *rs2, unsigned int *funct7) {
	
    unsigned int instruction = std::stoul(inst, nullptr, 16);
    
    // Extract instruction fields
    unsigned int opcode = instruction & 0x7F;
    *rd = (instruction >> 7) & 0x1F;
    *funct3 = (instruction >> 12) & 0x7;
    *rs1 = (instruction >> 15) & 0x1F;
    *rs2 = (instruction >> 20) & 0x1F;
    *funct7 = (instruction >> 25) & 0x7F;
    
    // For debugging
        std::cout << "Hex Instruction: " << inst << std::endl;
        std::cout << "Decoded fields:" << std::endl;
        std::cout << "  opcode: 0x" << std::hex << opcode << std::endl;
        std::cout << "  rd: " << std::dec << *rd << std::endl;
        std::cout << "  funct3: " << std::dec << *funct3 << std::endl;
        std::cout << "  rs1: " << std::dec << *rs1 << std::endl;
        std::cout << "  rs2: " << std::dec << *rs2 << std::endl;
        std::cout << "  funct7: 0x" << std::hex << *funct7 << std::endl;

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
    switch (opcode) {
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
            
            if (funct3 == 0x0) { // SB
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
			cout << "Program end" << endl;
			return false;

        default:
            std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
            break;
	}
	return true;

}

// Add other functions here ... 