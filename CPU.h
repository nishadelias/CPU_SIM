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
	const string REGISTER_NAMES[32] = {"Zero","ra","sp","gp","tp","t0","t1","t2",
									"s0/fp","s1","a0","a1","a2","a3","a4","a5",
									"a6","a7","s2","s3","s4","s5","s6","s7",
									"s8","s9","s10","s11","t3","t4","t5","t6"};
	ALU alu;

	int32_t generate_immediate(uint32_t instruction, int opcode);	// Generates the immediate value for a given instruction based on its opcode
    int32_t sign_extend(int32_t value, int bits);					// Performs sign extension on a given value for a specified bit width
    bool check_address_alignment(uint32_t address, uint32_t bytes);	// Checks whether a memory access address is properly aligned

public:
	CPU();
	unsigned long readPC();
	void incPC();							// Increments the program counter (PC) by 8 (i.e., one instruction)
	string get_instruction(char *IM);		// Fetches a 32-bit instruction (8 hex characters) from instruction memory
	int get_register_value(int reg);		// Returns the value stored in the specified register
	bool decode_instruction(string inst, bool *regWrite, bool *aluSrc, bool *branch, bool *memRe, bool *memWr, bool *memToReg, bool *upperIm, int *aluOp,
		unsigned int *opcode, unsigned int *rd, unsigned int *funct3, unsigned int *rs1, unsigned int *rs2, unsigned int *funct7, bool debug=false);

	void execute(int rd, int rs1, int rs2, int aluOp, int opcode, string inst, bool debug=false);		// Executes a decoded instruction by performing ALU/memory operations and updating state.

	int32_t read_memory(uint32_t address, int type);				// Reads a byte, halfword, or word from memory at the specified address
    void write_memory(uint32_t address, int32_t value, int type);	// Writes a byte, halfword, or word to memory at the specified address.
	void print_all_registers();
	
};

// add other functions and objects here
