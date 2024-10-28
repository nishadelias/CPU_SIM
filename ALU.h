// file: ALU.h

#ifndef ALU_H
#define ALU_H

#include <cstdint>

class ALU {
private:
    // Internal flags
    bool zero_flag;
    int32_t result;

public:
    ALU();
    
    // Main ALU operation function
    int32_t execute(int32_t operand1, int32_t operand2, int aluOp);
    
    // Flag access methods
    bool isZero() const { return zero_flag; }
    int32_t getResult() const { return result; }
};

#endif