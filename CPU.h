// file: CPU.h

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include "ALU.h"
using namespace std;


// class instruction { // optional
// public:
// 	bitset<32> instr;//instruction
// 	instruction(bitset<32> fetch); // constructor

// };

class CPU {
private:
	static const int MEMORY_SIZE = 4096;
    int dmemory[MEMORY_SIZE]; 	//data memory byte addressable in little endian fashion;
	unsigned long PC; //pc 
	int32_t registers[32];
	ALU alu;

	int32_t generate_immediate(uint32_t instruction, int opcode);
    int32_t sign_extend(int32_t value, int bits);
    bool check_address_alignment(uint32_t address, uint32_t bytes);

public:
	CPU();
	unsigned long readPC();
	void incPC();
	string get_instruction(char *IM);
	int get_register_value(int reg);
	bool decode_instruction(string inst, bool *regWrite, bool *aluSrc, bool *branch, bool *memRe, bool *memWr, bool *memToReg, bool *upperIm, int *aluOp,
		unsigned int *opcode, unsigned int *rd, unsigned int *funct3, unsigned int *rs1, unsigned int *rs2, unsigned int *funct7);

	void execute(int rd, int rs1, int rs2, int aluOp, int opcode, string inst);

	int32_t read_memory(uint32_t address, bool is_byte);
    void write_memory(uint32_t address, int32_t value, bool is_byte);
	
};

// add other functions and objects here
