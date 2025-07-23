// file: cpusim.cpp

#include "CPU.h"

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include<fstream>
#include <sstream>
using namespace std;


int main(int argc, char* argv[])
{

	char instMem[4096];


	if (argc < 2) {
		cout << "No file name entered. Exiting...";
		return -1;
	}

	ifstream infile(argv[1]); //open the file
	if (!(infile.is_open() && infile.good())) {
		cout<<"error opening file\n";
		return 0; 
	}
	string line; 
	int i = 0;
	while (infile) {
			infile>>line;
			stringstream line2(line);
			char x; 
			line2>>x;
			instMem[i] = x; // be careful about hex
			i++;
			line2>>x;
			instMem[i] = x; // be careful about hex
			i++;
		}
	int maxPC= i/4; 
	

	CPU myCPU;  
	
	// Control signals
	bool done = true;
	bool regWrite = false;
	bool aluSrc = false;
	bool branch = false;
	bool memRe = false;
	bool memWr = false;
	bool memToReg = false;
	bool upperIm = false;
	int aluOp = 0;

	string curr_instruction = "";

	// Decoded instruction
	unsigned int opcode;
	unsigned int rd;
    unsigned int funct3;
    unsigned int rs1;
    unsigned int rs2;
    unsigned int funct7;

	while (done == true) // processor's main loop. Each iteration is equal to one clock cycle.  
	{
		//fetch
		curr_instruction = myCPU.get_instruction(instMem);

		// cout << curr_instruction << endl;

		// decode
		done = myCPU.decode_instruction(curr_instruction, &regWrite, &aluSrc, &branch, & memRe, &memWr, &memToReg, &upperIm, &aluOp,
			&opcode, &rd, &funct3, &rs1, &rs2, &funct7);

		// execute
		myCPU.execute(rd, rs1, rs2, aluOp, opcode, curr_instruction);
		
		// increment PC
		myCPU.incPC();
		if (myCPU.readPC() > maxPC * 8) {
			break;
		}
	}
	int a0 = myCPU.get_register_value(10);	// a0
	int a1 = myCPU.get_register_value(11);  //a1
	
	// print the results 
	  myCPU.print_all_registers();

	return 0;

}