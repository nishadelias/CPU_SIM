// file: CPU.h
#pragma once

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "ALU.h"
#include <cstdint>
#include "MemoryIf.h"
#include "BranchPredictorScheme.h"

using namespace std;

// Pipeline register structures
struct IF_ID_Register {
    uint32_t instruction;
    uint32_t pc;
    bool valid;
    bool is_compressed;  // Track if instruction is 16-bit compressed
    uint16_t compressed_inst;  // Store original compressed instruction for disassembly
    IF_ID_Register() : instruction(0), pc(0), valid(false), is_compressed(false), compressed_inst(0) {}
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
    // Loads: 1=LB, 2=LBU, 3=LH, 4=LHU, 5=LW, 6=FLW
    // Stores: 1=SB, 2=SH, 3=SW, 4=FSW
    int memReadType;
    int memWriteType;
    
    // Floating-point control signals
    bool fpRegWrite;  // Write to FP register
    bool fpRegRead1;  // Read from FP register rs1
    bool fpRegRead2;  // Read from FP register rs2
    int fpOp;  // FP operation code

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
    float rs1_fp_data;  // FP register rs1 data
    float rs2_fp_data;  // FP register rs2 data
    uint32_t pc;
    uint32_t instruction;  // Store the instruction for disassembly
    bool is_compressed;    // Whether this was a compressed instruction
    uint16_t compressed_inst;  // Original compressed instruction if applicable
    bool valid;

    ID_EX_Register() : regWrite(false), aluSrc(false), branch(false), memRe(false),
                      memWr(false), memToReg(false), upperIm(false), aluOp(0),
                      memReadType(0), memWriteType(0),
                      fpRegWrite(false), fpRegRead1(false), fpRegRead2(false), fpOp(0),
                      opcode(0), rd(0), funct3(0), rs1(0), rs2(0), funct7(0),
                      rs1_data(0), rs2_data(0), immediate(0), rs1_fp_data(0.0f), rs2_fp_data(0.0f),
                      pc(0), instruction(0), is_compressed(false), compressed_inst(0), valid(false) {}
};

struct EX_MEM_Register {
    bool regWrite;
    bool memRe;
    bool memWr;
    bool memToReg;
    int memReadType;
    int memWriteType;
    
    // Floating-point control signals
    bool fpRegWrite;
    float fp_result;  // FP operation result

    int32_t alu_result;
    int32_t rs2_data;
    float rs2_fp_data;  // FP register rs2 data for stores
    unsigned int rd;
    uint32_t pc;
    uint32_t instruction;  // Store the instruction for disassembly
    bool is_compressed;    // Whether this was a compressed instruction
    uint16_t compressed_inst;  // Original compressed instruction if applicable
    bool valid;

    EX_MEM_Register() : regWrite(false), memRe(false), memWr(false), memToReg(false),
                       memReadType(0), memWriteType(0),
                       fpRegWrite(false), fp_result(0.0f),
                       alu_result(0), rs2_data(0), rs2_fp_data(0.0f), rd(0), pc(0),
                       instruction(0), is_compressed(false), compressed_inst(0), valid(false) {}
};

struct MEM_WB_Register {
    bool regWrite;
    bool memToReg;
    
    // Floating-point control signals
    bool fpRegWrite;
    float fp_result;
    float mem_fp_data;  // FP data loaded from memory

    int32_t alu_result;
    int32_t mem_data;
    unsigned int rd;
    uint32_t pc;
    uint32_t instruction;  // Store the instruction for disassembly
    bool is_compressed;    // Whether this was a compressed instruction
    uint16_t compressed_inst;  // Original compressed instruction if applicable
    bool valid;

    MEM_WB_Register() : regWrite(false), memToReg(false),
                       fpRegWrite(false), fp_result(0.0f), mem_fp_data(0.0f),
                       alu_result(0), mem_data(0), rd(0), pc(0),
                       instruction(0), is_compressed(false), compressed_inst(0), valid(false) {}
};

// Pipeline snapshot for GUI visualization
struct PipelineSnapshot {
    int cycle;
    bool stall;
    bool flush;
    
    // IF/ID stage
    struct {
        bool valid;
        uint32_t pc;
        uint32_t instruction;
        string disassembly;
    } if_id;
    
    // ID/EX stage
    struct {
        bool valid;
        uint32_t pc;
        string disassembly;
        string opcode_name;
    } id_ex;
    
    // EX/MEM stage
    struct {
        bool valid;
        uint32_t pc;
        string disassembly;
        int32_t alu_result;
    } ex_mem;
    
    // MEM/WB stage
    struct {
        bool valid;
        uint32_t pc;
        string disassembly;
        int32_t write_data;
    } mem_wb;
    
    PipelineSnapshot() : cycle(0), stall(false), flush(false) {
        if_id.valid = false;
        id_ex.valid = false;
        ex_mem.valid = false;
        mem_wb.valid = false;
    }
};

// Statistics tracking structure
struct CPUStatistics {
    // Instruction counts by type
    uint64_t total_instructions;
    uint64_t r_type_count;
    uint64_t i_type_count;
    uint64_t load_count;
    uint64_t store_count;
    uint64_t branch_count;
    uint64_t jump_count;
    uint64_t lui_auipc_count;
    
    // Pipeline events
    uint64_t stall_count;
    uint64_t flush_count;
    uint64_t branch_taken_count;
    uint64_t branch_not_taken_count;
    
    // Performance metrics
    uint64_t total_cycles;
    uint64_t instructions_retired;
    
    // Cache statistics (will be populated from cache)
    uint64_t cache_hits;
    uint64_t cache_misses;
    
    // Memory access tracking
    uint64_t memory_reads;
    uint64_t memory_writes;
    
    // Branch prediction statistics
    uint64_t branch_mispredictions;
    
    CPUStatistics() : total_instructions(0), r_type_count(0), i_type_count(0),
                     load_count(0), store_count(0), branch_count(0), jump_count(0),
                     lui_auipc_count(0), stall_count(0), flush_count(0),
                     branch_taken_count(0), branch_not_taken_count(0),
                     total_cycles(0), instructions_retired(0),
                     cache_hits(0), cache_misses(0),
                     memory_reads(0), memory_writes(0),
                     branch_mispredictions(0) {}
    
    double getCPI() const {
        if (instructions_retired == 0) return 0.0;
        return static_cast<double>(total_cycles) / static_cast<double>(instructions_retired);
    }
    
    double getCacheHitRate() const {
        uint64_t total_accesses = cache_hits + cache_misses;
        if (total_accesses == 0) return 0.0;
        return static_cast<double>(cache_hits) / static_cast<double>(total_accesses) * 100.0;
    }
    
    double getPipelineUtilization() const {
        if (total_cycles == 0) return 0.0;
        // Pipeline utilization: average number of stages with valid instructions
        // Ideal: 5 stages always full = 5.0
        // We approximate by: instructions_retired / cycles (since each instruction uses 5 cycles ideally)
        // But we need to account for the fact that instructions take multiple cycles
        // A better metric: (instructions_retired * 5) / (total_cycles * 5) = instructions_retired / total_cycles
        // But this doesn't account for pipeline bubbles. Let's use a simpler metric:
        // Utilization = (instructions_retired) / (total_cycles) * 100%
        // This gives the percentage of cycles that retired an instruction
        return (static_cast<double>(instructions_retired) / static_cast<double>(total_cycles)) * 100.0;
    }
};

// Memory access record for tracking memory addresses
struct MemoryAccess {
    int cycle;
    uint32_t address;
    bool is_write;
    uint32_t value;
    uint32_t pc;  // PC of instruction that accessed memory
    string instruction_disassembly;
    bool cache_hit;  // Whether this access was a cache hit
    
    MemoryAccess() : cycle(0), address(0), is_write(false), value(0), pc(0), cache_hit(false) {}
    MemoryAccess(int c, uint32_t addr, bool write, uint32_t val, uint32_t instruction_pc, const string& disasm, bool hit = false)
        : cycle(c), address(addr), is_write(write), value(val), pc(instruction_pc), instruction_disassembly(disasm), cache_hit(hit) {}
};

// Register value change record
struct RegisterChange {
    int cycle;
    unsigned int register_num;
    int32_t old_value;
    int32_t new_value;
    uint32_t pc;  // PC of instruction that changed register
    string instruction_disassembly;
    
    RegisterChange() : cycle(0), register_num(0), old_value(0), new_value(0), pc(0) {}
    RegisterChange(int c, unsigned int reg, int32_t old_val, int32_t new_val, uint32_t instruction_pc, const string& disasm)
        : cycle(c), register_num(reg), old_value(old_val), new_value(new_val), pc(instruction_pc), instruction_disassembly(disasm) {}
};

// Instruction dependency record
struct InstructionDependency {
    uint32_t producer_pc;      // PC of instruction that produces value
    uint32_t consumer_pc;       // PC of instruction that consumes value
    unsigned int register_num;  // Register involved in dependency
    string dependency_type;    // "RAW", "WAR", "WAW"
    int producer_cycle;        // Cycle when producer writes
    int consumer_cycle;         // Cycle when consumer reads/writes
    string producer_disassembly;
    string consumer_disassembly;
    
    InstructionDependency() : producer_pc(0), consumer_pc(0), register_num(0), 
                              producer_cycle(0), consumer_cycle(0) {}
    InstructionDependency(uint32_t prod_pc, uint32_t cons_pc, unsigned int reg, 
                          const string& dep_type, int prod_cyc, int cons_cyc,
                          const string& prod_disasm, const string& cons_disasm)
        : producer_pc(prod_pc), consumer_pc(cons_pc), register_num(reg), dependency_type(dep_type),
          producer_cycle(prod_cyc), consumer_cycle(cons_cyc),
          producer_disassembly(prod_disasm), consumer_disassembly(cons_disasm) {}
};

class CPU {
private:
    // Removed old internal dmemory[]. Now we go through a pluggable interface:
    MemoryDevice* dmem_;    // data memory interface (can be a cache)
    BranchPredictorScheme* branch_predictor_;  // branch predictor

    unsigned long PC; //pc 
    int32_t registers[32];
    float registers_fp[32];  // Floating-point registers (f0-f31)
    uint32_t fcsr;  // Floating-Point Control and Status Register
    const string REGISTER_NAMES[32] = {"Zero","ra","sp","gp","tp","t0","t1","t2",
                                    "s0/fp","s1","a0","a1","a2","a3","a4","a5",
                                    "a6","a7","s2","s3","s4","s5","s6","s7",
                                    "s8","s9","s10","s11","t3","t4","t5","t6"};
    const string FP_REGISTER_NAMES[32] = {"ft0","ft1","ft2","ft3","ft4","ft5","ft6","ft7",
                                         "fs0","fs1","fa0","fa1","fa2","fa3","fa4","fa5",
                                         "fa6","fa7","fs2","fs3","fs4","fs5","fs6","fs7",
                                         "fs8","fs9","fs10","fs11","ft8","ft9","ft10","ft11"};
    ALU alu;

    // Pipeline registers
    IF_ID_Register if_id;
    ID_EX_Register id_ex;
    EX_MEM_Register ex_mem;
    MEM_WB_Register mem_wb;

    // Previous-cycle snapshots for forwarding
    EX_MEM_Register ex_mem_prev;
    MEM_WB_Register mem_wb_prev;

    // Pipeline control
    bool pipeline_stall;
    bool pipeline_flush;
    
    // Branch prediction state
    bool branch_predicted_taken_;  // Whether current branch in pipeline was predicted taken
    uint32_t branch_predicted_target_;  // Predicted target address
    uint32_t branch_pc_;  // PC of branch instruction being predicted

    // Program termination
    int maxPC;

    // Logging
    bool enable_logging;
    ofstream log_file;
    
    // Statistics and tracing for GUI
    CPUStatistics stats_;
    vector<PipelineSnapshot> pipeline_trace_;
    bool enable_tracing_;
    
    // Memory address tracking
    vector<MemoryAccess> memory_access_history_;
    
    // Register value history tracking
    vector<RegisterChange> register_history_;
    int32_t previous_register_values_[32];  // Track previous values for change detection
    
    // Instruction dependency tracking
    vector<InstructionDependency> instruction_dependencies_;
    map<uint32_t, int> pc_to_cycle_map_;  // Map PC to cycle when instruction was in WB stage
    map<uint32_t, unsigned int> pc_to_rd_map_;  // Map PC to destination register
    
    // Helper to capture pipeline snapshot
    void capture_pipeline_snapshot(int cycle, bool had_stall, bool had_flush);
    
    // Helper methods for tracking
    void track_memory_access(int cycle, uint32_t address, bool is_write, uint32_t value, uint32_t pc, bool cache_hit = false);
    void track_register_change(int cycle, unsigned int reg, int32_t old_value, int32_t new_value, uint32_t pc);
    void track_instruction_dependencies(int cycle, uint32_t pc, unsigned int rd, unsigned int rs1, unsigned int rs2);

    // Pipeline stage methods
    void instruction_fetch(char* instMem, bool debug);
    void instruction_decode(bool debug);
    void execute_stage(bool debug);
    void memory_stage(bool debug);
    void write_back_stage(bool debug);

    // Helper methods
    string disassemble_instruction(uint32_t instruction) const;
    string disassemble_compressed_instruction(uint16_t instruction) const;
    void log_pipeline_state(int cycle, bool had_stall, bool had_flush);
    void log_instruction_disassembly(uint32_t instruction, uint32_t pc);

    int32_t generate_immediate(uint32_t instruction, int opcode) const;
    int32_t sign_extend(int32_t value, int bits) const;
    bool check_address_alignment(uint32_t address, uint32_t bytes);
    
    // Compressed instruction support
    uint32_t expand_compressed_instruction(uint16_t compressed_inst) const;
    bool is_compressed_instruction(uint16_t inst) const;
    
    // Floating-point unit operations
    float execute_fp_operation(float operand1, float operand2, int fpOp) const;
    int32_t execute_fp_compare(float operand1, float operand2, int fpOp) const;
    int32_t execute_fp_classify(float operand) const;

public:
    CPU();
    ~CPU();
    unsigned long readPC();
    void incPC(int increment = 4);
    string get_instruction(char *IM);
    string get_instruction_16bit(char *IM);  // Get 16-bit instruction
    int get_register_value(int reg);
    bool decode_instruction(string inst, bool *regWrite, bool *aluSrc, bool *branch, bool *memRe, bool *memWr, bool *memToReg, bool *upperIm, int *aluOp,
        unsigned int *opcode, unsigned int *rd, unsigned int *funct3, unsigned int *rs1, unsigned int *rs2, unsigned int *funct7, bool debug=false);

    void execute(int rd, int rs1, int rs2, int aluOp, int opcode, string inst, bool debug=false);

    // Data memory now goes through dmem_:
    int32_t read_memory(uint32_t address, int type);
    void    write_memory(uint32_t address, int32_t value, int type);

    void print_all_registers();

    // Pipeline
    void run_pipeline_cycle(char* instMem, int cycle, bool debug);
    void set_logging(bool enable, string log_filename = "");
    bool is_pipeline_empty();
    void set_max_pc(int max_pc);
    void reset();  // Reset CPU to initial state

    // NEW: wire in the memory hierarchy from main()
    void set_data_memory(MemoryDevice* dev) { dmem_ = dev; }
    
    // Branch predictor management
    void set_branch_predictor(BranchPredictorScheme* predictor) { branch_predictor_ = predictor; }
    BranchPredictorScheme* get_branch_predictor() { return branch_predictor_; }
    const BranchPredictorScheme* get_branch_predictor() const { return branch_predictor_; }
    
    // Statistics and tracing for GUI
    void enable_tracing(bool enable) { enable_tracing_ = enable; }
    const CPUStatistics& get_statistics() const { return stats_; }
    const vector<PipelineSnapshot>& get_pipeline_trace() const { return pipeline_trace_; }
    void clear_trace() { pipeline_trace_.clear(); }
    
    // Get current pipeline state (for real-time GUI updates)
    PipelineSnapshot get_current_pipeline_state(int cycle) const;
    
    // Get register values (for GUI)
    const int32_t* get_all_registers() const { return registers; }
    
    // Get cache statistics (requires cache to be set)
    bool get_cache_stats(uint64_t& hits, uint64_t& misses) const;
    
    // Memory address tracking accessors
    const vector<MemoryAccess>& get_memory_access_history() const { return memory_access_history_; }
    void clear_memory_history() { memory_access_history_.clear(); }
    
    // Register history accessors
    const vector<RegisterChange>& get_register_history() const { return register_history_; }
    void clear_register_history() { register_history_.clear(); }
    
    // Instruction dependency accessors
    const vector<InstructionDependency>& get_instruction_dependencies() const { return instruction_dependencies_; }
    void clear_dependencies() { instruction_dependencies_.clear(); }
};
