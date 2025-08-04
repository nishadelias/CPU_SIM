# RISC-V CPU Simulator

A comprehensive, cycle-accurate RISC-V CPU simulator implemented in C++ that demonstrates fundamental computer architecture concepts. This project serves as both an educational tool for understanding CPU design and a practical implementation of the RISC-V instruction set architecture.

## 🚀 Features

### **Implemented Instructions**
- **R-type Instructions**: ADD, SUB, OR, XOR, AND, SLL, SRL, SRA, SLT, SLTU
- **I-type Instructions**: ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
- **Load/Store Instructions**: LW, SW, LB, LBU, LH, LHU, SB, SH
- **Branch Instructions**: BEQ, BNE, BLT, BGE, BLTU, BGEU
- **Jump Instructions**: JAL, JALR
- **Upper Immediate**: LUI

### **Architecture Components**
- **5-stage Pipeline Simulation**: Fetch, Decode, Execute, Memory, Writeback
- **32-bit RISC-V Architecture**: Full register file with 32 general-purpose registers
- **Memory Management**: 4KB data memory with proper alignment checking
- **ALU Operations**: Arithmetic, logical, shift, and comparison operations
- **Control Unit**: Comprehensive control signal generation for all instruction types

## 🛠️ Technical Implementation

### **Key Design Decisions**
- **Modular Architecture**: Separate ALU and CPU classes for clean separation of concerns
- **Little-Endian Memory**: Proper byte ordering for RISC-V compatibility
- **Sign Extension**: Correct handling of immediate values and memory loads
- **Branch Prediction**: Simple static branch prediction with PC-relative addressing
- **Memory Alignment**: Hardware-enforced alignment checking for load/store operations

### **Performance Optimizations**
- **Efficient Instruction Decoding**: Bit-level manipulation for fast field extraction
- **Optimized ALU**: Single-cycle execution for all arithmetic and logical operations
- **Memory Access Patterns**: Optimized for typical RISC-V memory access patterns

## 📁 Project Structure

```
CPU_SIM/
├── CPU.cpp              # Main CPU implementation
├── CPU.h                # CPU class definition
├── ALU.cpp              # Arithmetic Logic Unit
├── ALU.h                # ALU class definition
├── cpusim.cpp           # Main simulator entry point
├── assembly_translations/  # Human-readable assembly code
│   ├── 24r.txt          # R-type instruction tests
│   ├── 24swr.txt        # Store/Write/Read tests
│   ├── 24jswr.txt       # Jump/Store/Write/Read tests
│   └── comprehensive_test.txt  # Complete instruction set test
├── instruction_memory/  # Machine code (little-endian)
│   ├── 24instMem-r.txt
│   ├── 24instMem-swr.txt
│   ├── 24instMem-jswr.txt
│   └── comprehensive_test.txt
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
./cpusim instruction_memory/24instMem-r.txt

# Store/Write/Read operations test
./cpusim instruction_memory/24instMem-swr.txt

# Jump/Store/Write/Read operations test
./cpusim instruction_memory/24instMem-jswr.txt

# Comprehensive instruction set test
./cpusim instruction_memory/comprehensive_test.txt
```

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
- **24r.txt**: Tests basic R-type arithmetic and logical operations
- **24swr.txt**: Tests memory operations (load/store) with register operations
- **24jswr.txt**: Tests control flow (jumps/branches) with memory operations
- **comprehensive_test.txt**: Complete instruction set validation

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
- **Cache Simulation**: Multi-level cache hierarchy
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
