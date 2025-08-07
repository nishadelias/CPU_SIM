// file: ALU.cpp

#include "ALU.h"

ALU::ALU() : zero_flag(false), result(0) {}

int32_t ALU::execute(int32_t operand1, int32_t operand2, int aluOp) {
    switch(aluOp) {
        case 0x00: // ADD, ADDI, AUIPC
        case 0x40: // LB address calculation
        case 0x41: // LBU address calculation
        case 0x42: // LH address calculation
        case 0x43: // LHU address calculation
        case 0x44: // LW address calculation
        case 0x45: // SB address calculation
        case 0x46: // SH address calculation
        case 0x47: // SW address calculation
            result = operand1 + operand2;
            break;
            
        case 0x01: // SUB
            result = operand1 - operand2;
            break;
            
        case 0x10: // AND, ANDI
            result = operand1 & operand2;
            break;
            
        case 0x11: // OR
            result = operand1 | operand2;
            break;
            
        case 0x12: // XOR
            result = operand1 ^ operand2;
            break;
            
        case 0x13: // SLT (Set Less Than)
            result = (operand1 < operand2) ? 1 : 0;
            break;
            
        case 0x14: // SLTU (Set Less Than Unsigned)
            result = (static_cast<uint32_t>(operand1) < static_cast<uint32_t>(operand2)) ? 1 : 0;
            break;
            
        case 0x15: // SLTI (Set Less Than Immediate)
            result = (operand1 < operand2) ? 1 : 0;
            break;
            
        case 0x16: // SLTIU (Set Less Than Immediate Unsigned)
            result = (static_cast<uint32_t>(operand1) < static_cast<uint32_t>(operand2)) ? 1 : 0;
            break;
            
        case 0x17: // XORI
            result = operand1 ^ operand2;
            break;
            
        case 0x18: // ORI
            result = operand1 | operand2;
            break;
            
        case 0x19: // ANDI
            result = operand1 & operand2;
            break;
            
        case 0x20: // SLL (Shift Left Logical)
            result = operand1 << (operand2 & 0x1F); // Only use bottom 5 bits
            break;
            
        case 0x21: // SRL (Shift Right Logical)
            result = static_cast<uint32_t>(operand1) >> (operand2 & 0x1F); // Only use bottom 5 bits
            break;
            
        case 0x22: // SRA (Shift Right Arithmetic)
            result = operand1 >> (operand2 & 0x1F); // Only use bottom 5 bits
            // Preserve sign bit for arithmetic shift
            if (operand1 < 0) {
                result |= (~0U << (32 - (operand2 & 0x1F)));
            }
            break;
            
        case 0x23: // SLLI (Shift Left Logical Immediate)
            result = operand1 << (operand2 & 0x1F); // Only use bottom 5 bits
            break;
            
        case 0x24: // SRLI (Shift Right Logical Immediate)
            result = static_cast<uint32_t>(operand1) >> (operand2 & 0x1F); // Only use bottom 5 bits
            break;
            
        case 0x25: // SRAI (Shift Right Arithmetic Immediate)
            result = operand1 >> (operand2 & 0x1F); // Only use bottom 5 bits
            // Preserve sign bit for arithmetic shift
            if (operand1 < 0) {
                result |= (~0U << (32 - (operand2 & 0x1F)));
            }
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
    if (aluOp != 0x30 && aluOp != 0x31 && aluOp != 0x32 && aluOp != 0x33 && aluOp != 0x34 && aluOp != 0x35) {
        zero_flag = (result == 0);
    }
    
    return result;
}