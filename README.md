# RISC-V CPU Simulator

A comprehensive, cycle-accurate RISC-V CPU simulator with a graphical user interface. This project simulates a 5-stage pipelined RISC-V processor, complete with cache memory, instruction execution, and detailed performance statistics. Perfect for learning computer architecture, understanding how CPUs work, and visualizing pipeline execution.

**✨ Key Educational Feature**: The simulator includes an **extensible cache framework** that allows you to easily implement and test your own custom cache schemes! Compare built-in schemes (direct-mapped, fully associative, set-associative) or write your own to experiment with different replacement policies, write policies, and cache organizations.

## 📖 What is This Project?

This project simulates a **RISC-V CPU** - a simplified but realistic processor that executes RISC-V assembly instructions. It includes:

- **5-Stage Pipeline**: Simulates how modern CPUs process instructions through Fetch, Decode, Execute, Memory, and Writeback stages
- **Multiple Cache Schemes**: Compare different cache organizations (Direct-mapped, Fully Associative, Set-Associative) with performance metrics
- **Extensible Cache Framework**: **Easily add your own custom cache schemes!** The framework makes it simple to implement new cache replacement policies, write policies, or organizational structures for educational experiments
- **Full RISC-V Instruction Set**: Supports arithmetic, logical, memory, branch, and jump instructions
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

## 🎓 Educational Focus: Custom Cache Schemes

**One of the key educational features of this simulator is the extensible cache framework.** 

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

#### Option 2: Build Command-Line Version

```bash
# Simple compilation
g++ CPU.cpp ALU.cpp cpusim.cpp -o cpusim

# Or using CMake (if you modify CMakeLists.txt)
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

6. **Explore the Interface**
   The GUI is organized into tabs that you can view side-by-side:
   
   - **Statistics Tab**: 
     - Performance metrics (CPI, cache hit rate, pipeline utilization)
     - Instruction counts by type
     - Cache statistics (hits, misses, hit rate)
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

7. **Monitor Execution**
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
├── Cache.h                # Cache implementations (Direct-mapped, Fully Associative, Set-Associative)
├── CacheScheme.h          # Cache scheme framework and interface
├── MemoryIf.h             # Memory interface abstraction
├── cpusim.cpp             # Command-line simulator entry point
├── CMakeLists.txt         # Build configuration
├── CACHE_SCHEMES.md       # Detailed guide on cache schemes and adding custom ones
│
├── gui/                   # GUI source files
│   ├── main.cpp           # Application entry point
│   ├── MainWindow.cpp/h   # Main window and UI layout
│   ├── SimulatorController.cpp/h  # Simulation control logic
│   ├── PipelineWidget.cpp/h       # Pipeline visualization
│   ├── StatsWidget.cpp/h          # Statistics display
│   ├── RegisterWidget.cpp/h       # Register file display
│   ├── MemoryWidget.cpp/h         # Memory access display
│   └── DependencyWidget.cpp/h    # Dependency visualization
│
├── instruction_memory/    # Machine code programs (hex format)
│   ├── instMem-r.txt      # R-type instruction tests
│   ├── instMem-swr.txt    # Store/Write/Read tests
│   ├── instMem-jswr.txt   # Jump/Store/Write/Read tests
│   └── instMem-all.txt    # Comprehensive test suite
│
├── assembly_translations/ # Human-readable assembly (for reference)
│   ├── r.txt
│   ├── swr.txt
│   ├── jswr.txt
│   └── all.txt
│
└── build/                 # Build directory (created during build)
    └── cpusim_gui         # Executable (after building)
```

## 🎯 Features

### Implemented Instructions

- **R-type** (Register): ADD, SUB, OR, XOR, AND, SLL, SRL, SRA, SLT, SLTU
- **I-type** (Immediate): ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
- **Load/Store**: LW, SW, LB, LBU, LH, LHU, SB, SH
- **Branch**: BEQ, BNE, BLT, BGE, BLTU, BGEU
- **Jump**: JAL, JALR
- **Upper Immediate**: LUI, AUIPC

### Architecture Components

- **5-Stage Pipeline**: IF (Instruction Fetch), ID (Decode), EX (Execute), MEM (Memory), WB (Writeback)
- **32-bit RISC-V Architecture**: Full 32-register file (x0-x31)
- **Multiple Cache Schemes**: 
  - Direct-Mapped Cache (1-way set associative)
  - Fully Associative Cache (LRU replacement)
  - 2-way, 4-way, and 8-way Set-Associative Caches (LRU replacement)
  - All caches use write-through and write-allocate policies
  - 4KB cache size with 32-byte cache lines (default)
- **Memory Hierarchy**: Cache backed by main memory (64KB RAM)
- **Pipeline Hazards**: Handles data hazards, control hazards, and structural hazards
- **Statistics Tracking**: Comprehensive performance metrics and instruction counts

### GUI Features

- **Real-Time Visualization**: Watch instructions flow through the pipeline
- **Tabbed Interface**: View multiple panels side-by-side
- **Interactive Controls**: Start, pause, step, reset, and speed control
- **Detailed Statistics**: CPI, cache hit rate, pipeline utilization, instruction distribution
- **Memory Tracking**: See every memory access with cache hit/miss information
- **Dependency Analysis**: Understand data dependencies between instructions
- **Pipeline Trace**: Complete history of pipeline execution with PC and instruction names

## 📊 Understanding the Output

### Statistics Explained

- **CPI (Cycles Per Instruction)**: Average number of cycles needed per instruction
- **Cache Hit Rate**: Percentage of memory accesses that hit in the cache
- **Pipeline Utilization**: Percentage of pipeline stages that are actively processing instructions
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

## 🧪 Test Programs

The project includes several test programs:

- **instMem-r.txt**: Tests basic R-type arithmetic and logical operations
- **instMem-swr.txt**: Tests memory operations (load/store) with register operations
- **instMem-jswr.txt**: Tests control flow (jumps/branches) with memory operations
- **instMem-all.txt**: Comprehensive test covering all instruction types

Each program has a corresponding assembly translation file in `assembly_translations/` for reference.

## 🔧 Technical Details

### Architecture Specifications

- **ISA**: RISC-V RV32I (32-bit base integer instruction set)
- **Endianness**: Little-endian byte ordering
- **Memory**: 64KB main memory (RAM), byte-addressable
- **Registers**: 32 general-purpose registers (x0 always zero)
- **Pipeline**: 5-stage pipeline with hazard detection
- **Cache**: Selectable cache scheme (Direct-mapped, Fully Associative, or Set-Associative)
  - Default: 4KB cache with 32-byte lines
  - Write-through and write-allocate policies
  - LRU replacement for associative caches

### Design Decisions

- **Modular Architecture**: Separate classes for CPU, ALU, Cache, Memory
- **Cycle-Accurate Simulation**: Each pipeline cycle is simulated accurately
- **Forwarding Support**: Basic data forwarding to reduce stalls
- **Branch Resolution**: Branches resolved in EX stage (no branch prediction)
- **Alignment Checking**: Hardware-enforced alignment for load/store operations

## 📚 Learning Resources

This simulator is excellent for learning:

- **Computer Architecture**: How CPUs process instructions
- **Pipeline Design**: Understanding instruction pipelining and hazards
- **Memory Hierarchy**: Cache organization and behavior
  - Compare different cache schemes (direct-mapped vs. set-associative vs. fully associative)
  - Understand cache hit/miss behavior and replacement policies
  - See how cache organization affects performance
- **RISC-V ISA**: Instruction encoding and execution
- **Performance Analysis**: Understanding CPI, cache hit rates, and pipeline efficiency

For a detailed guide on cache schemes, see [CACHE_SCHEMES.md](CACHE_SCHEMES.md).

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

Potential improvements:

- Multi-level cache hierarchy (L2/L3 caches)
- Additional cache replacement policies (FIFO, Random, etc.)
- Write-back cache policies
- Branch prediction (static and dynamic)
- Floating-point instruction support (RV32F)
- Exception handling and interrupts
- More advanced forwarding and hazard detection
- Performance profiling tools

Note: Multiple cache schemes (Direct-mapped, Fully Associative, Set-Associative) are already implemented! See [CACHE_SCHEMES.md](CACHE_SCHEMES.md) for details.

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
