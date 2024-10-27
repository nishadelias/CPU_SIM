// file: CPU.h

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;


// class instruction { // optional
// public:
// 	bitset<32> instr;//instruction
// 	instruction(bitset<32> fetch); // constructor

// };

class CPU {
private:
	int dmemory[4096]; //data memory byte addressable in little endian fashion;
	unsigned long PC; //pc 

public:
	CPU();
	unsigned long readPC();
	void incPC();
	string get_instruction(char *IM);
	bool decode_instruction(string inst, bool *regWrite, bool *aluSrc, bool *branch, bool *memRe, bool *memWr, bool *memToReg, bool *upperIm, int *aluOp,
		unsigned int *rd, unsigned int *funct3, unsigned int *rs1, unsigned int *rs2, unsigned int *funct7);
	
};

// add other functions and objects here
