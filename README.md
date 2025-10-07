# RISC-V CPU Simulator

A comprehensive, cycle-accurate RISC-V CPU simulator implemented in C++ that demonstrates fundamental computer architecture concepts. This project serves as both an educational tool for understanding CPU design and a practical implementation of the RISC-V instruction set architecture.

## 🚀 Features

### **Implemented Instructions**
- **R-type Instructions**: ADD, SUB, OR, XOR, AND, SLL, SRL, SRA, SLT, SLTU
- **I-type Instructions**: ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
- **Load/Store Instructions**: LW, SW, LB, LBU, LH, LHU, SB, SH
- **Branch Instructions**: BEQ, BNE, BLT, BGE, BLTU, BGEU
- **Jump Instructions**: JAL, JALR
- **Upper Immediate**: LUI, AUIPC

### **Architecture Components**
- **5-stage Pipeline Simulation**: Fetch, Decode, Execute, Memory, Writeback
- **32-bit RISC-V Architecture**: Full register file with 32 general-purpose registers
- **Memory Hierarchy**: ✅ **Direct-mapped, write-through cache (configurable size/line) backed by main memory**
- **ALU Operations**: Arithmetic, logical, shift, and comparison operations
- **Control Unit**: Control signal generation for all instruction types
- **Branch Handling**: Branches resolved in EX stage (no prediction)

## 🛠️ Technical Implementation

### **Key Design Decisions**
- **Modular Architecture**: Separate ALU, CPU, Memory, and Cache classes
- **Little-Endian Memory**: Proper byte ordering for RISC-V compatibility
- **Sign Extension**: Correct handling of immediate values and memory loads
- **Alignment Checking**: Hardware-enforced alignment for load/store
- **Forwarding**: Basic data forwarding support in the pipeline

### **Performance Optimizations**
- **Efficient Instruction Decoding**: Bit-level manipulation for fast field extraction
- **Single-Cycle ALU**: All arithmetic/logical operations complete in one cycle
- **Cache Integration**: ✅ **Reduced average memory access latency via direct-mapped cache with hit/miss statistics**

## 📁 Project Structure



```
CPU_SIM/
├── CPU.cpp # Main CPU implementation
├── CPU.h # CPU class definition
├── ALU.cpp # Arithmetic Logic Unit
├── ALU.h # ALU class definition
├── Cache.h # ✅ Direct-mapped cache implementation (header-only)
├── cpusim.cpp # Simulator entry point with cache integration
├── MemoryIf.h # Memory interface abstraction
├── assembly_translations/ # Human-readable assembly programs
│ ├── r.txt # R-type tests
│ ├── swr.txt # Store/Write/Read tests
│ ├── jswr.txt # Jump/Store/Write/Read tests
│ └── all.txt # Comprehensive test
├── instruction_memory/ # Corresponding machine code
│ ├── instMem-r.txt
│ ├── instMem-swr.txt
│ ├── instMem-jswr.txt
│ └── instMem-all.txt
└── README.md
```

## 🚀 Quick Start

### **Compilation**
```bash
g++ *.cpp -o cpusim
```

### **Running Test Programs**
```bash
# Basic R-type instruction test
./cpusim instruction_memory/instMem-r.txt

# Store/Write/Read operations test
./cpusim instruction_memory/instMem-swr.txt

# Jump/Store/Write/Read operations test
./cpusim instruction_memory/instMem-jswr.txt

# Comprehensive instruction set test
./cpusim instruction_memory/instMem-all.txt
```

### **Optional Command-Line Flags**

--debug : Print detailed pipeline stage debug output
--log <filename> : Save pipeline execution trace to a log file

### **Expected Output**
The simulator displays final register values after program execution:
```
Register Values:
Zero: 0
ra: 0
sp: 0
...
t0: 4096
t1: 0
t2: 512
...
```

## 🧪 Testing and Validation

### **Test Programs**
- **r.txt**: Tests basic R-type arithmetic and logical operations
- **swr.txt**: Tests memory operations (load/store) with register operations
- **jswr.txt**: Tests control flow (jumps/branches) with memory operations
- **all.txt**: Complete instruction set validation

### **Cache System Verification**
The cache system has been tested and verified with:
- ✅ **Compilation**: Clean compilation with C++11 compatibility
- ✅ **Execution**: All test programs run successfully with cache integration
- ✅ **Memory Operations**: Load/store operations work correctly through cache
- ✅ **Hit/Miss Tracking**: Cache statistics are properly maintained
- ✅ **Write Policy**: Write-through with write-allocate policy functioning correctly

### **Verification**
Each test program includes expected output values for verification. The comprehensive test validates:
- Arithmetic operations (ADD, SUB, ADDI)
- Logical operations (AND, OR, XOR, ANDI, ORI, XORI)
- Shift operations (SLL, SRL, SRA, SLLI, SRLI, SRAI)
- Comparison operations (SLT, SLTU, SLTI, SLTIU)
- Memory operations (LW, SW, LB, SB, LH, SH)
- Control flow (BEQ, BNE, JAL)



## 🎯 Educational Value

This project demonstrates:
- **Computer Architecture Fundamentals**: Pipeline design, control signals, data paths
- **RISC-V ISA Understanding**: Instruction encoding, addressing modes, memory model
- **System Programming**: Memory management, byte ordering, alignment
- **Software Engineering**: Modular design, testing strategies, documentation

## 🔧 Technical Specifications

- **Architecture**: 32-bit RISC-V RV32I base integer instruction set
- **Memory**: 4KB data memory with byte-addressable access
- **Registers**: 32 general-purpose registers (x0-x31)
- **Endianness**: Little-endian byte ordering
- **Pipeline**: 5-stage pipeline simulation (fetch, decode, execute, memory, writeback)

## 🚀 Future Enhancements

Potential improvements for advanced features:
- **Pipelining**: Full 5-stage pipeline with hazard detection
- **Cache Simulation**: Multi-level cache hierarchy (L2/L3 caches)
- **Cache Policies**: Set-associative and fully-associative cache implementations
- **Floating Point**: RV32F floating-point instruction support
- **Interrupts**: Exception handling and interrupt processing
- **Performance Analysis**: Cycle counting and performance metrics
- **GUI Interface**: Visual pipeline and memory state display

## 📚 Learning Outcomes

This project provides hands-on experience with:
- **CPU Design**: Understanding of datapath and control unit design
- **Instruction Set Architecture**: RISC-V specification implementation
- **Computer Organization**: Memory hierarchy and addressing modes
- **Digital Logic**: ALU design and control signal generation
- **Software Testing**: Comprehensive test suite development

## 🤝 Contributing

This project serves as an educational resource for computer architecture students and enthusiasts. Contributions that improve documentation, add new features, or enhance testing are welcome.

## 📄 License

This project is open source and available for educational use.

---

**Built with passion for computer architecture and RISC-V technology** 🖥️⚡
