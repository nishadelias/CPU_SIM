# RISC-V CPU Simulator

A comprehensive, cycle-accurate RISC-V CPU simulator with a graphical user interface. This project simulates a 5-stage pipelined RISC-V processor, complete with cache memory, instruction execution, and detailed performance statistics. Perfect for learning computer architecture, understanding how CPUs work, and visualizing pipeline execution.

**✨ Key Educational Features**: The simulator includes **extensible frameworks** that allow you to easily implement and test your own custom components! 
- **Cache Framework**: Compare built-in schemes (direct-mapped, fully associative, set-associative) or write your own to experiment with different replacement policies, write policies, and cache organizations.
- **Branch Predictor Framework**: Compare built-in predictors (Always Not Taken, Bimodal, GShare, Tournament) or write your own to experiment with different prediction algorithms and accuracy improvements.

## 📖 What is This Project?

This project simulates a **RISC-V CPU** - a simplified but realistic processor that executes RISC-V assembly instructions. It includes:

- **5-Stage Pipeline**: Simulates how modern CPUs process instructions through Fetch, Decode, Execute, Memory, and Writeback stages
- **Multiple Cache Schemes**: Compare different cache organizations (Direct-mapped, Fully Associative, Set-Associative) with performance metrics
- **Extensible Cache Framework**: **Easily add your own custom cache schemes!** The framework makes it simple to implement new cache replacement policies, write policies, or organizational structures for educational experiments
- **Multiple Branch Predictors**: Compare different branch prediction algorithms (Always Not Taken, Always Taken, Bimodal, GShare, Tournament) with accuracy metrics
- **Extensible Branch Predictor Framework**: **Easily add your own custom branch predictors!** The framework makes it simple to implement new prediction algorithms, history mechanisms, or hybrid approaches for educational experiments
- **RV32IMCF-class ISA**: Integer (RV32I), multiply/divide (M), 16-bit compressed (C), single-precision float (F), plus syscall emulation and a Zicsr subset for **FCSR**
- **Graphical Interface**: Visualize pipeline execution, register values, memory accesses, and statistics in real-time
- **Command-Line Interface**: Run simulations from the terminal with detailed logging

## 🚀 Quick Start

### Prerequisites

**For Command-Line Version:**
- C++ compiler (g++ or clang++)
- Make or CMake

**For GUI Version:**
- Qt6 (Core, Widgets, Charts modules)
- CMake 3.16 or higher
- C++17 compiler

**Installing Qt6:**

**macOS:**
```bash
brew install qt6
export PATH="/opt/homebrew/opt/qt6/bin:$PATH"  # Add to ~/.zshrc
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install qt6-base-dev qt6-charts-dev cmake build-essential
```

**Windows:**
1. Download Qt6 from https://www.qt.io/download
2. Install CMake from https://cmake.org/download/
3. Add both to your PATH

## 🎓 Educational Focus: Extensible Frameworks

**One of the key educational features of this simulator is the extensible frameworks for both caches and branch predictors.** 

### Custom Cache Schemes

You can easily implement and test your own cache schemes! The framework provides:
- A simple interface (`CacheScheme`) that any cache implementation must follow
- Built-in cache statistics tracking (hits, misses, hit rates)
- Seamless integration with the GUI - your custom cache will automatically appear in the dropdown
- Examples of different cache organizations (direct-mapped, fully associative, set-associative) to learn from

**Want to add your own cache scheme?** See the [CACHE_SCHEMES.md](CACHE_SCHEMES.md) guide for step-by-step instructions, code examples, and implementation guidelines. Perfect for:
- Computer architecture courses
- Cache design experiments
- Understanding cache replacement policies (LRU, FIFO, Random, etc.)
- Learning different write policies (write-through, write-back)

### Custom Branch Predictors

You can easily implement and test your own branch predictors! The framework provides:
- A simple interface (`BranchPredictorScheme`) that any predictor implementation must follow
- Built-in prediction statistics tracking (correct/incorrect predictions, accuracy)
- Seamless integration with the GUI - your custom predictor will automatically appear in the dropdown
- Examples of different prediction algorithms (Always Not Taken, Bimodal, GShare, Tournament) to learn from

**Want to add your own branch predictor?** See the [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md) guide for step-by-step instructions, code examples, and implementation guidelines. Perfect for:
- Computer architecture courses
- Branch prediction algorithm experiments
- Understanding prediction mechanisms (saturating counters, history registers, hybrid approaches)
- Learning how to improve prediction accuracy

### Building the Project

#### Option 1: Build GUI Version (Recommended)

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run the GUI
./cpusim_gui
```

#### Option 2: Build command-line `cpusim` and tests

```bash
cmake -S . -B build
cmake --build build -j
./build/cpusim instruction_memory/instMem-m-ext.txt
ctest --test-dir build
```

## 🎮 How to Use

### Using the GUI (Recommended for Beginners)

1. **Launch the Application**
   ```bash
   cd build
   ./cpusim_gui
   ```

2. **Configure Cache Scheme** (Optional)
   - In the "Cache Configuration" section, select your desired cache scheme
   - Available options:
     - **Direct Mapped**: Each memory block maps to one cache line
     - **Fully Associative**: Any block can go in any cache line (LRU replacement)
     - **2/4/8-Way Set Associative**: Compromise between direct-mapped and fully associative
   - Default is Direct Mapped
   - The cache scheme is applied when you reset or start the simulation

3. **Configure Branch Predictor** (Optional)
   - In the "Branch Predictor Configuration" section, select your desired branch predictor
   - Available options:
     - **Always Not Taken**: Always predicts branches will not be taken
     - **Always Taken**: Always predicts branches will be taken
     - **Bimodal (2-bit)**: 2-bit saturating counter predictor
     - **GShare**: Global history register predictor
     - **Tournament**: Hybrid predictor combining Bimodal and GShare
   - Default is Always Not Taken
   - The branch predictor is applied when you reset or start the simulation

3. **Load a Program**
   - Click the **"Open Program"** button
   - Navigate to the `instruction_memory/` directory
   - Select a program file (e.g., `instMem-all.txt`)
   - The filename will appear below the button

4. **Control the Simulation**
   - **Start**: Begin continuous execution at the selected speed
   - **Pause**: Pause the simulation (can resume with Start)
   - **Reset**: Reset the simulation to the beginning
   - **Step**: Execute one pipeline cycle at a time
   - **Speed Slider**: Adjust execution speed (1-100 cycles per second)

5. **Compare Cache Schemes**
   - Run a program with one cache scheme
   - Note the cache hit rate in the Statistics tab
   - Reset the simulation
   - Change the cache scheme
   - Run the same program again
   - Compare hit rates to understand cache performance differences

6. **Compare Branch Predictors**
   - Run a program with one branch predictor
   - Note the prediction accuracy in the Statistics tab (under Performance Metrics)
   - Reset the simulation
   - Change the branch predictor
   - Run the same program again
   - Compare accuracy rates to understand branch prediction performance differences

7. **Explore the Interface**
   The GUI is organized into tabs that you can view side-by-side:
   
   - **Statistics Tab**: 
     - Performance metrics (CPI, cache hit rate, pipeline utilization)
     - Instruction counts by type
     - Cache statistics (hits, misses, hit rate)
     - Branch predictor statistics (accuracy, correct/incorrect predictions)
     - Instruction distribution pie chart
   
   - **Register File Tab**:
     - Current values of all 32 RISC-V registers (x0-x31)
     - ABI names (zero, ra, sp, gp, tp, t0-t6, s0-s11, a0-a7)
     - Highlights recently changed registers
   
   - **Memory Access History Tab**:
     - Timeline of all memory read/write operations
     - Shows address, value, instruction, and cache hit/miss status
     - Color-coded by operation type
   
   - **Instruction Dependencies Tab**:
     - Shows RAW (Read After Write) dependencies between instructions
     - Displays producer and consumer instructions with their PCs
     - Helps understand data hazards in the pipeline
   
   - **Pipeline Execution Trace Tab**:
     - Real-time view of instructions in each pipeline stage
     - Shows PC address and instruction name (e.g., "PC 0x10: ADD")
     - Displays stall and flush status
     - Tracks instruction flow through IF, ID, EX, MEM, WB stages

8. **Monitor Execution**
   - Watch the pipeline trace update in real-time
   - Observe register values change as instructions execute
   - See memory accesses and cache behavior
   - Check statistics to understand performance

### Using the Command-Line Version

```bash
# Run a test program
./cpusim instruction_memory/instMem-all.txt

# With debug output
./cpusim instruction_memory/instMem-all.txt --debug

# Save pipeline trace to log file
./cpusim instruction_memory/instMem-all.txt --log pipeline.log
```

The command-line version displays final register values after execution.

## 📁 Project Structure

```
CPU_SIM/
├── CPU.cpp/h              # Main CPU implementation and pipeline
├── ALU.cpp/h              # Arithmetic Logic Unit
├── Cache.h                # Cache implementations
├── CacheScheme.h          # Cache scheme framework
├── BranchPredictor.h        # Branch predictor implementations
├── BranchPredictorScheme.h
├── MemoryIf.h             # Memory interface abstraction
├── cpusim.cpp             # Command-line simulator entry point
├── CMakeLists.txt         # Build configuration (cpusim, cpusim_gui, unit tests)
├── CACHE_SCHEMES.md
├── BRANCH_PREDICTORS.md
├── scripts/
│   └── run_riscv_integration.sh   # Build + ctest + M/syscall smoke check
├── tests/
│   └── rvc_expand_test.cpp          # Compressed-instruction expansion tests
├── gui/                   # Qt6 GUI
├── instruction_memory/   # Hex programs (two hex chars per byte line, little-endian words)
└── build/                 # CMake output (cpusim, cpusim_gui, rvc_expand_test)
```

## 🎯 Features

### Implemented instructions (summary)

**RV32I**: Core integer ops including loads/stores, branches, `JAL`/`JALR`, `LUI`/`AUIPC`. **FENCE** / unknown primary opcodes are treated as NOPs for forward progress.

**RV32M**: `MUL`, `MULH`, `MULHSU`, `MULHU`, `DIV`, `DIVU`, `REM`, `REMU`.

**RV32C** (+ **FC**): Compressed fetch/expand/disassembly for the standard 32-bit-oriented mapping, including **C.FLW** / **C.FSW** / **C.FLWSP** / **C.FSWSP** and **C.EBREAK**.

**RV32F** (subset): `FLW`, `FSW`, `FADD.S`, `FSUB.S`, `FMUL.S`, `FDIV.S`, `FSGNJ.S`, `FMIN.S`, `FMAX.S`, `FSQRT.S`, `FMADD.S`, `FMSUB.S`, `FNMSUB.S`, `FNMADD.S`, comparisons (`FEQ`/`FLT`/`FLE`), `FCVT.W.S`, `FCVT.S.W`, `FMV.X.W`, `FMV.W.X`, `FCLASS.S`. *Not* full spec: e.g. no `FCVT.WU.S` / `FCVT.S.WU`, no `FSGNJN`/`FSGNJX`, fused ops are **single-precision only** (`funct2=00`).

**System / ABI**: `ECALL` with Linux/RISC-V–style nr **93** (exit), **64** (write stdout), **214** (`brk`); `EBREAK` halts the simulation. **Zicsr**: `CSRRW`/`CSRRS`/`CSRRC` and immediate forms decode and execute; **FCSR (`0x001`)** is modeled; other CSRs read as **0** and writes are ignored.

**Running arbitrary compiled C** still depends on your newlib/runtime: only the above syscalls and memory model are emulated—link with a script that matches this environment.

### Architecture Components

- **5-Stage Pipeline**: IF (Instruction Fetch), ID (Decode), EX (Execute), MEM (Memory), WB (Writeback)
- **32-bit RISC-V Architecture**: Full 32-register file (x0-x31)
- **Multiple Cache Schemes**: 
  - Direct-Mapped Cache (1-way set associative)
  - Fully Associative Cache (LRU replacement)
  - 2-way, 4-way, and 8-way Set-Associative Caches (LRU replacement)
  - All caches use write-through and write-allocate policies
  - 4KB cache size with 32-byte cache lines (default)
- **Multiple Branch Predictors**:
  - Always Not Taken (baseline)
  - Always Taken (baseline)
  - Bimodal (2-bit saturating counter, 2048 entries)
  - GShare (global history, 2048 entries, 12-bit history)
  - Tournament (hybrid Bimodal + GShare, 2048 entries)
- **Memory Hierarchy**: Cache backed by main memory (64KB RAM)
- **Pipeline Hazards**: Handles data hazards, control hazards, and structural hazards
- **Branch Prediction**: Predicts branches in ID stage, resolves in EX stage, handles mispredictions
- **Statistics Tracking**: Comprehensive performance metrics, instruction counts, cache statistics, and branch prediction accuracy

### GUI Features

- **Real-Time Visualization**: Watch instructions flow through the pipeline
- **Tabbed Interface**: View multiple panels side-by-side
- **Interactive Controls**: Start, pause, step, reset, and speed control
- **Detailed Statistics**: CPI, cache hit rate, pipeline utilization, instruction distribution
- **Branch Predictor Metrics**: Prediction accuracy, correct/incorrect predictions, total predictions
- **Memory Tracking**: See every memory access with cache hit/miss information
- **Dependency Analysis**: Understand data dependencies between instructions
- **Pipeline Trace**: Complete history of pipeline execution with PC and instruction names

## 📊 Understanding the Output

### Statistics Explained

- **CPI (Cycles Per Instruction)**: Average number of cycles needed per instruction
- **Cache Hit Rate**: Percentage of memory accesses that hit in the cache
- **Pipeline Utilization**: Percentage of pipeline stages that are actively processing instructions
- **Branch Predictor Accuracy**: Percentage of branch predictions that were correct
- **Instruction Counts**: Breakdown by instruction type (R-type, I-type, Load, Store, Branch, Jump, etc.)

### Pipeline Stages

1. **IF (Instruction Fetch)**: Fetches instruction from instruction memory
2. **ID (Decode)**: Decodes instruction, reads registers, generates control signals
3. **EX (Execute)**: Performs ALU operations, calculates branch targets
4. **MEM (Memory)**: Accesses data memory (load/store operations)
5. **WB (Writeback)**: Writes results back to register file

### Cache Behavior

- **Cache Hit**: Data found in cache (fast access)
- **Cache Miss**: Data not in cache, must fetch from main memory (slower)
- **Write-Through**: All writes go to both cache and main memory
- **Write-Allocate**: On cache miss, allocate cache line before write
- **Cache Schemes**: Different cache organizations affect hit rates:
  - **Direct-Mapped**: Fastest lookup, but can have conflicts
  - **Fully Associative**: Best hit rate, but slower lookup
  - **Set-Associative**: Balance between hit rate and lookup speed
- **Cache Line Size**: 32 bytes (one cache miss brings in 32 consecutive memory addresses)

For detailed information about cache schemes and how to add custom ones, see [CACHE_SCHEMES.md](CACHE_SCHEMES.md).

For detailed information about branch predictors and how to add custom ones, see [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md).

## 🧪 Tests

**CMake / CTest** (after `cmake -B build`):

- `rvc_expand_test` — compressed expansion goldens
- `m_ext_syscall` — `instruction_memory/instMem-m-ext.txt` (`MUL` + `ECALL` exit)

**Scripts**: `scripts/run_riscv_integration.sh` rebuilds, runs **ctest**, and checks syscall output.

**Sample hex programs** live under `instruction_memory/` (e.g. `instMem-all.txt`, `instMem-m-ext.txt`, `instMem-rvc-smoke.txt`).

## 🔧 Technical Details

### Architecture Specifications

- **ISA**: RISC-V **RV32IMCF**-oriented (see limitations above); not a complete formal compliance suite
- **Endianness**: Little-endian byte ordering
- **Memory**: 64KB main memory (RAM), byte-addressable
- **Registers**: 32 general-purpose registers (x0 always zero)
- **Pipeline**: 5-stage pipeline with hazard detection
- **Cache**: Selectable cache scheme (Direct-mapped, Fully Associative, or Set-Associative)
  - Default: 4KB cache with 32-byte lines
  - Write-through and write-allocate policies
  - LRU replacement for associative caches
- **Branch Prediction**: Selectable branch predictor (Always Not Taken, Always Taken, Bimodal, GShare, or Tournament)
  - Default: Always Not Taken
  - Predictions made in ID stage, resolved in EX stage
  - Mispredictions cause pipeline flushes

### Design Decisions

- **Modular Architecture**: Separate classes for CPU, ALU, Cache, Memory
- **Cycle-Accurate Simulation**: Each pipeline cycle is simulated accurately
- **Forwarding Support**: Basic data forwarding to reduce stalls
- **Branch Prediction**: Branches predicted in ID stage, resolved in EX stage
  - Multiple predictor algorithms available
  - Mispredictions cause pipeline flushes and performance penalties
- **Alignment Checking**: Hardware-enforced alignment for load/store operations

## 📚 Learning Resources

This simulator is excellent for learning:

- **Computer Architecture**: How CPUs process instructions
- **Pipeline Design**: Understanding instruction pipelining and hazards
- **Memory Hierarchy**: Cache organization and behavior
  - Compare different cache schemes (direct-mapped vs. set-associative vs. fully associative)
  - Understand cache hit/miss behavior and replacement policies
  - See how cache organization affects performance
- **Branch Prediction**: Prediction algorithms and accuracy
  - Compare different branch predictors (Always Not Taken vs. Bimodal vs. GShare vs. Tournament)
  - Understand prediction mechanisms (saturating counters, history registers, hybrid approaches)
  - See how prediction accuracy affects pipeline performance
- **RISC-V ISA**: Instruction encoding and execution
- **Performance Analysis**: Understanding CPI, cache hit rates, branch prediction accuracy, and pipeline efficiency

For detailed guides, see:
- [CACHE_SCHEMES.md](CACHE_SCHEMES.md) - Cache schemes framework
- [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md) - Branch predictors framework

## 🐛 Troubleshooting

### GUI Won't Build

- **Qt6 not found**: Make sure Qt6 is installed and in your PATH
  - macOS: `brew install qt6`
  - Verify: `qmake6 --version`
- **CMake errors**: Ensure CMake >= 3.16
  - Try: `cmake -DCMAKE_PREFIX_PATH=/path/to/qt6 ..`
- **Compilation errors**: Check that all source files are present and Qt6 Charts is installed

### GUI Won't Run

- **Program won't load**: Make sure you're selecting files from `instruction_memory/` directory
- **Simulation stuck**: Check the pipeline log file (`pipeline.log`) for details
- **No updates**: Make sure you've clicked "Start" or "Step" to begin execution

### Command-Line Issues

- **Program not found**: Use relative paths from project root
- **No output**: Try adding `--debug` flag to see detailed execution

## 📝 Logging

The simulator can generate detailed execution logs:

- **GUI**: Automatically writes to `pipeline.log` in the project root
- **Command-Line**: Use `--log <filename>` to specify log file
- **Log Format**: Shows pipeline state, register values, memory accesses, and control signals for each cycle

## 🚀 Future Enhancements

Possible extensions:

- Full **RV32F** coverage (unsigned conversions, sign-injection variants, all rounding modes)
- Precise **IEEE 754** exception flags and full **FCSR** behavior
- Exceptions, interrupts, **U-mode** / virtual memory
- Additional syscalls (`read`, `open`, …) if you target a richer libc
- Multi-level caches, write-back policies, more predictors

Caches and branch predictors already include several schemes; see [CACHE_SCHEMES.md](CACHE_SCHEMES.md) and [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md).

## 🤝 Contributing

This project serves as an educational resource. Contributions that improve:
- Documentation and examples
- New features and instructions
- Bug fixes and performance improvements
- Test programs and validation

are welcome!

## 📄 License

This project is open source and available for educational use.

---

**Built with passion for computer architecture and RISC-V technology** 🖥️⚡

For detailed GUI build instructions, see `GUI_BUILD.md`.
