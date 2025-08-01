// file: ALU.cpp

#include "ALU.h"

ALU::ALU() : zero_flag(false), result(0) {}

int32_t ALU::execute(int32_t operand1, int32_t operand2, int aluOp) {
    switch(aluOp) {
        case 0x00: // ADD, AUIPC
        case 0x8: // LB/SB address calculation
        case 0x9: // LW/SW address calculation
        case 0xA: // SB address calculation
        case 0xB: // SW address calculation
            result = operand1 + operand2;
            break;
            
        case 0x4: // XOR
            result = operand1 ^ operand2;
            break;
            
        case 0x5: // SRAI (Shift Right Arithmetic Immediate)
            // operand2 contains the shift amount (shamt)
            result = operand1 >> (operand2 & 0x1F); // Only use bottom 5 bits
            // Preserve sign bit for arithmetic shift
            if (operand1 < 0) {
                result |= (~0U << (32 - (operand2 & 0x1F)));
            }
            break;
            
        case 0x6: // ORI
            result = operand1 | operand2;
            break;

        case 0x10: // AND, ANDI
            result = operand1 & operand2;
            break;
            
        case 0x30: // BEQ
        case 0x35: // BNE
            result = (operand1 == operand2) ? 1 : 0;
            zero_flag = (result == 1);
            break;

        case 0x31: // BGE
        case 0x33: // BLT
            result = (operand1 >= operand2) ? 1 : 0;
            zero_flag = (result == 1);
            break;

        case 0x32: // BGEU
        case 0x34: // BLTU
            result = (static_cast<unsigned int>(operand1) >= static_cast<unsigned int>(operand2)) ? 1 : 0;
            zero_flag = (result == 1);
            break;
            
        case 0xF: // LUI (Load Upper Immediate)
            // For LUI, operand1 contains the immediate value already shifted
            result = operand1;
            break;
            
        default:
            // Handle invalid ALU operation
            result = 0;
            break;
    }
    
    // Update zero flag (except for branch operations where it's set explicitly)
    if (aluOp != 0xC) {
        zero_flag = (result == 0);
    }
    
    return result;
}