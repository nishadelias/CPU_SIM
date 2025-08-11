// file: CPU.h

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include <fstream>
#include "ALU.h"
#include <cstdint>
using namespace std;

// Pipeline register structures
struct IF_ID_Register {
    uint32_t instruction;
    uint32_t pc;
    bool valid;
    
    IF_ID_Register() : instruction(0), pc(0), valid(false) {}
};

struct ID_EX_Register {
    // Control signals
    bool regWrite;
    bool aluSrc;
    bool branch;
    bool memRe;
    bool memWr;
    bool memToReg;
    bool upperIm;
    int aluOp;
    // memory width/type (so MEM can do LB/LH/LW & SB/SH/SW correctly)
    // Loads: 1=LB, 2=LBU, 3=LH, 4=LHU, 5=LW
    // Stores: 1=SB, 2=SH, 3=SW
    int memReadType;
    int memWriteType;
    
    // Instruction fields
    unsigned int opcode;
    unsigned int rd;
    unsigned int funct3;
    unsigned int rs1;
    unsigned int rs2;
    unsigned int funct7;
    
    // Data
    int32_t rs1_data;
    int32_t rs2_data;
    int32_t immediate;
    uint32_t pc;
    bool valid;
    
    ID_EX_Register() : regWrite(false), aluSrc(false), branch(false), memRe(false), 
                      memWr(false), memToReg(false), upperIm(false), aluOp(0),
                      memReadType(0), memWriteType(0),
                      opcode(0), rd(0), funct3(0), rs1(0), rs2(0), funct7(0),
                      rs1_data(0), rs2_data(0), immediate(0), pc(0), valid(false) {}
};

struct EX_MEM_Register {
    // Control signals
    bool regWrite;
    bool memRe;
    bool memWr;
    bool memToReg;
    int memReadType;
    int memWriteType;
    
    // Data
    int32_t alu_result;
    int32_t rs2_data;
    unsigned int rd;
    uint32_t pc;
    bool valid;
    
    EX_MEM_Register() : regWrite(false), memRe(false), memWr(false), memToReg(false),
                       memReadType(0), memWriteType(0),
                       alu_result(0), rs2_data(0), rd(0), pc(0), valid(false) {}
};

struct MEM_WB_Register {
    // Control signals
    bool regWrite;
    bool memToReg;
    
    // Data
    int32_t alu_result;
    int32_t mem_data;
    unsigned int rd;
    uint32_t pc;
    bool valid;
    
    MEM_WB_Register() : regWrite(false), memToReg(false),
                       alu_result(0), mem_data(0), rd(0), pc(0), valid(false) {}
};

// class instruction { // optional
// public:
// 	bitset<32> instr;//instruction
// 	instruction(bitset<32> fetch); // constructor

// };

class CPU {
private:
	static const int MEMORY_SIZE = 4096;
    uint8_t dmemory[MEMORY_SIZE]; 	// data memory: byte-addressable, little-endian
	unsigned long PC; //pc 
	int32_t registers[32];
	const string REGISTER_NAMES[32] = {"Zero","ra","sp","gp","tp","t0","t1","t2",
									"s0/fp","s1","a0","a1","a2","a3","a4","a5",
									"a6","a7","s2","s3","s4","s5","s6","s7",
									"s8","s9","s10","s11","t3","t4","t5","t6"};
	ALU alu;

	// Pipeline registers
	IF_ID_Register if_id;
	ID_EX_Register id_ex;
	EX_MEM_Register ex_mem;
	MEM_WB_Register mem_wb;

    // Previous-cycle snapshots for safe forwarding (read-only in EX)
    EX_MEM_Register ex_mem_prev;
    MEM_WB_Register mem_wb_prev;
	
	// Pipeline control
	bool pipeline_stall;
	bool pipeline_flush;
	
	// Program termination
	int maxPC;
	
	// Logging
	bool enable_logging;
	ofstream log_file;
	
	// Pipeline stage methods
	void instruction_fetch(char* instMem, bool debug);
	void instruction_decode(bool debug);
	void execute_stage(bool debug);
	void memory_stage(bool debug);
	void write_back_stage(bool debug);
	
	// Helper methods
	string disassemble_instruction(uint32_t instruction);
	void log_pipeline_state(int cycle);
	void log_instruction_disassembly(uint32_t instruction, uint32_t pc);

	int32_t generate_immediate(uint32_t instruction, int opcode);	// Generates the immediate value for a given instruction based on its opcode
    int32_t sign_extend(int32_t value, int bits);					// Performs sign extension on a given value for a specified bit width
    bool check_address_alignment(uint32_t address, uint32_t bytes);	// Checks whether a memory access address is properly aligned

public:
	CPU();
	~CPU();
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
	
	// Pipeline methods
	void run_pipeline_cycle(char* instMem, int cycle, bool debug);
	void set_logging(bool enable, string log_filename = "");
	bool is_pipeline_empty();
	void set_max_pc(int max_pc);
	
};

// add other functions and objects here
