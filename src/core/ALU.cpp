// file: ALU.cpp

#include "ALU.h"
#include <climits>
#include <cstdint>

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
            result = operand1 - operand2;
            zero_flag = (result == 0);
            break;

        case 0x35: // BNE
            result = operand1 - operand2;
            zero_flag = (result == 0);
            break;

        case 0x31: // BGE
            result = operand1 - operand2;
            zero_flag = (result >= 0);
            break;

        case 0x33: // BLT
            result = operand1 - operand2;
            zero_flag = (result < 0);
            break;

        case 0x32: // BGEU
            result = operand1 - operand2;
            zero_flag = (static_cast<uint32_t>(operand1) >= static_cast<uint32_t>(operand2));
            break;

        case 0x34: // BLTU
            result = operand1 - operand2;
            zero_flag = (static_cast<uint32_t>(operand1) < static_cast<uint32_t>(operand2));
            break;
            
        case 0xF: // LUI (Load Upper Immediate)
            // For LUI, operand1 contains the immediate value already shifted
            result = operand1;
            break;
            
        // M Extension (Multiply/Divide) operations
        case 0x60: // MUL: Multiply (32-bit × 32-bit → lower 32 bits)
            {
                int64_t product = static_cast<int64_t>(operand1) * static_cast<int64_t>(operand2);
                result = static_cast<int32_t>(product & 0xFFFFFFFF);
            }
            break;
            
        case 0x61: // MULH: Multiply high (signed × signed → upper 32 bits)
            {
                int64_t product = static_cast<int64_t>(operand1) * static_cast<int64_t>(operand2);
                result = static_cast<int32_t>((product >> 32) & 0xFFFFFFFF);
            }
            break;
            
        case 0x62: // MULHSU: Multiply high signed-unsigned
            {
                int64_t product = static_cast<int64_t>(operand1) * static_cast<uint64_t>(static_cast<uint32_t>(operand2));
                result = static_cast<int32_t>((product >> 32) & 0xFFFFFFFF);
            }
            break;
            
        case 0x63: // MULHU: Multiply high unsigned
            {
                uint64_t product = static_cast<uint64_t>(static_cast<uint32_t>(operand1)) * static_cast<uint64_t>(static_cast<uint32_t>(operand2));
                result = static_cast<int32_t>((product >> 32) & 0xFFFFFFFF);
            }
            break;
            
        case 0x64: // DIV: Divide (signed)
            if (operand2 == 0) {
                // Division by zero: return -1
                result = -1;
            } else if (operand1 == INT32_MIN && operand2 == -1) {
                // Overflow case: -2^31 / -1 = 2^31 (overflow, return -2^31)
                result = INT32_MIN;
            } else {
                result = operand1 / operand2;
            }
            break;
            
        case 0x65: // DIVU: Divide (unsigned)
            if (operand2 == 0) {
                // Division by zero: return max unsigned value
                result = 0xFFFFFFFF;
            } else {
                uint32_t u1 = static_cast<uint32_t>(operand1);
                uint32_t u2 = static_cast<uint32_t>(operand2);
                result = static_cast<int32_t>(u1 / u2);
            }
            break;
            
        case 0x66: // REM: Remainder (signed)
            if (operand2 == 0) {
                // Division by zero: return operand1
                result = operand1;
            } else if (operand1 == INT32_MIN && operand2 == -1) {
                // Overflow case: remainder is 0
                result = 0;
            } else {
                result = operand1 % operand2;
            }
            break;
            
        case 0x67: // REMU: Remainder (unsigned)
            if (operand2 == 0) {
                // Division by zero: return operand1
                result = operand1;
            } else {
                uint32_t u1 = static_cast<uint32_t>(operand1);
                uint32_t u2 = static_cast<uint32_t>(operand2);
                result = static_cast<int32_t>(u1 % u2);
            }
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