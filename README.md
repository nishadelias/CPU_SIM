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
1. Install **Qt6** (MSVC or MinGW kit) from [Qt Online Installer](https://www.qt.io/download-qt-installer) — enable **Qt Charts** for the same kit you use to compile.
2. Install **CMake** from [cmake.org](https://cmake.org/download/) and a C++ toolchain (Visual Studio with “Desktop development with C++”, or MinGW).
3. When configuring CMake, point Qt6:  
   `cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"`  
   (adjust the path to your Qt version and compiler folder).
4. Build: `cmake --build build --config Release`  
   On Windows with Visual Studio, GUI and test binaries are often under `build/Release/`.

**Path separators:** Examples below use `/` (macOS, Linux, Git Bash, WSL). On **Windows Command Prompt or PowerShell**, use backslashes or quoted paths, e.g. `build\Release\cpusim_gui.exe`.

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
# From project root (macOS / Linux)
cmake -S . -B build -DBUILD_GUI=ON -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6 2>/dev/null || brew --prefix qt)"

# Build CLI + GUI
cmake --build build -j
# Or GUI only after a headless configure: cmake --build build --target cpusim_gui

# Run the GUI
./build/cpusim_gui

# Windows (Visual Studio generator — release output folder)
# .\Release\cpusim_gui.exe
```

#### Option 2: Build command-line `cpusim` and tests

```bash
cmake -S . -B build
cmake --build build -j
./build/cpusim instruction_memory/instMem-m-ext.txt
ctest --test-dir build
```

On **Windows** with Visual Studio generators, the executables may be under `build/Release/` (or `build/Debug/`). Example:

```text
build\Release\cpusim.exe instruction_memory\instMem-m-ext.txt
ctest --test-dir build -C Release
```

**Git Bash** and **WSL** on Windows can use the Unix-style `./build/cpusim` paths if you build from those environments.

## 🎮 How to Use

### Programs you can run: ELF (compiled) vs hex text

The simulator uses a **single 64 KiB RAM** at address `0x00000000`. Instruction fetch and loads/stores share that memory.

| Kind | How it is detected | Typical use |
|------|--------------------|-------------|
| **ELF** | File starts with the ELF magic bytes `7F 45 4C 46` | Programs compiled and linked for this environment (see below) |
| **Hex text** | Anything else | **Educational** mode: same format as the files in `instruction_memory/` (whitespace-separated **two hex digits per byte**, little-endian instruction encoding) |

- **GUI (and `cpusim`)** choose the loader automatically: **no separate “mode” switch** — open the `.elf` or `.txt` / `.hex` file.
- After opening, the **File** panel shows a line such as:
  - **ELF (compiled C/RISC-V)** — entry address, stack pointer, and program break (`brk`) are set for you.
  - **Hex text (instruction memory)** — byte count and load address `0x00000000`.

### Compiling your own RISC-V programs for CPU_SIM

The GUI and CLI load **32-bit little-endian RISC-V ELF** files linked for a **flat 64 KiB RAM** at address `0x00000000`. You cross-compile on your machine, then open the `.elf` in the simulator (file picker or drag-and-drop).

#### 1. Install a cross-compiler

| Platform | Command |
|----------|---------|
| macOS (Homebrew) | `brew install riscv64-elf-gcc` |
| Linux (Debian/Ubuntu) | `sudo apt install gcc-riscv64-unknown-elf` (or distro equivalent) |

Verify: `riscv64-elf-gcc --version` (or `riscv32-unknown-elf-gcc`).

#### 2. Know the execution environment

| Item | Value |
|------|--------|
| ISA | **RV32** — use `-march=rv32im -mabi=ilp32` (recommended; see note on compressed instructions below) |
| RAM | 64 KiB, byte-addressable, base `0x00000000` |
| Linker script | [`examples/linker.ld`](examples/linker.ld) — all segments must fit in 64 KiB |
| Entry | `_start` at the beginning of `.text.init` (see example sources) |
| Stack | Simulator sets `sp` to top of RAM minus 16 when loading ELF; examples call `sim_init_stack()` for the same layout |
| libc | **Not available** — use the emulated syscalls below (or [`examples/simlib.h`](examples/simlib.h)) |

**Emulated syscalls** (Linux RISC-V calling convention: number in `a7`, args in `a0`–`a2`, result in `a0`):

| `a7` | Name | Behavior in simulator |
|------|------|------------------------|
| 93 | exit | Stops simulation; exit code = `a0` |
| 64 | write | `write(fd, buf, count)` — only **fd = 1** (stdout) is supported; bytes appear in CLI and the GUI **Program Output** tab |
| 214 | brk | Heap break for dynamic allocation (advanced) |

#### 3. Build the included examples

```bash
./scripts/build_example_elf.sh
```

This produces:

| Output | Source | What it does |
|--------|--------|----------------|
| `build/hello.elf` | `examples/hello.c` | Exits with code 42 (short smoke test) |
| `build/fib_print.elf` | `examples/fib_print.c` | 24 Fibonacci lines to stdout (~140 cycles) |
| `build/count_primes.elf` | `examples/count_primes.c` | Counts primes 2..400 (~30k+ cycles; nested loops + multiply) |

Run in CLI or GUI:

```bash
./build/cpusim build/count_primes.elf --max-cycles 200000
./build/cpusim_gui    # Open Program → build/count_primes.elf → Start
```

#### 4. Compile your own program

**Recommended pattern** (matches the examples; avoids `call`/`ret` stack issues in the current simulator):

1. Copy [`examples/simlib.h`](examples/simlib.h) or include it from your tree.
2. Provide `_start` in section `.text.init` with `__attribute__((naked))` (see `examples/fib_print.c`, `examples/count_primes.c`).
3. Use `SIM_PRINT("literal\n")` and `SIM_EXIT(code)` from [`examples/simlib.h`](examples/simlib.h) — do not use stack locals or `call`/`ret` across functions if possible.
4. Put loop counters in **static volatile** globals (`.bss`), not stack variables, when using `naked` `_start`.

```bash
export PATH="/opt/homebrew/bin:$PATH"   # macOS Homebrew, if needed

riscv64-elf-gcc -march=rv32im -mabi=ilp32 -nostdlib \
  -T examples/linker.ld \
  my_program.c -o build/my_program.elf
```

If `my_program.c` is in the project root, add `-I examples` when using `#include "simlib.h"`.

**Alternative: `crt0` + `main`** — link [`examples/crt0.S`](examples/crt0.S) with your `main()`:

```bash
riscv64-elf-gcc -march=rv32im -mabi=ilp32 -nostdlib -T examples/linker.ld \
  examples/crt0.S my_program.c -o build/my_program.elf
```

Programs that save/restore `ra` on the stack (`call`/`ret` with a normal prologue) may misbehave until pipeline forwarding is improved; prefer the `_start` + `simlib.h` style for reliable demos.

**Do not** use host `gcc` or link macOS/Linux x86 binaries — only **RV32 ELF** for this memory map will load.

#### 5. Load and run in the GUI

1. Build the simulator with the GUI enabled: `cmake -S . -B build -DBUILD_GUI=ON && cmake --build build --target cpusim_gui`
2. Launch `./build/cpusim_gui` (not an older copy from another build folder)
3. **Open Program** or drag-and-drop your `.elf`
4. Confirm the file line shows **ELF (compiled C/RISC-V)** with entry and `brk`
5. Click **Start** or **Step** — watch **Pipeline**, **Registers**, **Statistics**
6. If your program calls **write** to fd 1, text appears in **Program Output**

CLI sanity check before using the GUI:

```bash
./build/cpusim build/my_program.elf --max-cycles 200000
```

#### 6. Troubleshooting

| Problem | Likely cause |
|---------|----------------|
| “Could not load program” | Not RV32 ELF, segments > 64 KiB, or wrong endianness |
| **Program Output** empty | Program never called `write` (exit-only programs are normal) |
| Simulation stops at 200k cycles | GUI cycle cap; shorten loops or raise `MAX_CYCLES` in `SimulatorController` |
| `call main` / `ret` loops forever | Known forwarding issue with stack-saved `ra`; use `_start` + `simlib.h` pattern |
| `riscv64-elf-gcc` not found | Install toolchain; add `/opt/homebrew/bin` to `PATH` on macOS |

### Using the GUI (Recommended for Beginners)

1. **Launch the Application**

   From a terminal (project root), after building:

   ```bash
   # macOS / Linux
   ./build/cpusim_gui
   ```

   ```powershell
   # Windows (adjust path to your build output)
   .\build\Release\cpusim_gui.exe
   ```

   You can also double-click the executable from your file manager if your platform allows it.

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
   - Click **"Open Program"** (or use **File → Open Program**).
   - Pick either:
     - A **hex text** file (e.g. `instruction_memory/instMem-m-ext.txt`, `instMem-forward.txt`), or
     - A **compiled** ELF (`program.elf`) built for this simulator’s memory map.
   - The **filename** and a **format line** (ELF vs hex, entry/byte count) appear under the button.
   - **Reset** (or **Simulation → Reset**) reloads the same file from disk and reapplies cache/predictor settings.

4. **Control the Simulation**
   - **Start**: Begin continuous execution at the selected speed
   - **Pause**: Pause the simulation (can resume with Start)
   - **Reset**: Reset the simulation to the beginning
   - **Step**: Execute one pipeline cycle at a time
   - **Speed Slider**: Target simulation rate (1-500 cycles per second). While **Start** is active, the simulator runs in a fast mode: multiple cycles per screen refresh (~60 Hz), with tracing and heavy table updates disabled so higher speeds are reachable. **Pause** or **Step** restores full pipeline trace and memory/dependency views.

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
# Hex text program (same format as instruction_memory/*.txt)
./build/cpusim instruction_memory/instMem-m-ext.txt

# Compiled ELF (auto-detected)
./build/cpusim path/to/program.elf

# Debug trace to stdout
./build/cpusim instruction_memory/instMem-m-ext.txt --debug

# Pipeline log file
./build/cpusim instruction_memory/instMem-m-ext.txt --log pipeline.log

# Executable / strict mode: illegal instructions and bad memory accesses halt with fault (not NOP)
./build/cpusim program.elf --executable

# Benchmark-style stats (CSV or JSON; optional cache / branch predictor)
./build/cpusim program.elf --bench --json --cache 4way --predictor gshare --max-cycles 200000
```

Use `--help`-style usage: run `cpusim` with no arguments to print the usage message (see `cpusim.cpp`).

**Exit codes:** `0` on normal completion; non-zero if the simulator reports a fault in strict mode (see implementation).

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
├── MemoryMap.h            # RAM size / base addresses for the execution environment
├── ElfLoader.cpp/h        # ELF32 PT_LOAD loader
├── HexLoader.cpp/h        # Legacy hex text loader
├── ExecutionMode.h        # Educational vs executable (strict traps)
├── examples/
│   ├── linker.ld          # Link for 64 KiB RAM at 0x00000000
│   ├── crt0.S             # Optional _start (stack, call main, ECALL exit)
│   ├── simlib.h           # Syscall helpers (write, exit, decimal print)
│   ├── hello.c            # Minimal exit-42 demo
│   ├── count_primes.c     # Prime sieve 2..400 (longer nested-loop demo)
│   └── fib_print.c        # 24 Fibonacci lines + stdout
├── CACHE_SCHEMES.md
├── BRANCH_PREDICTORS.md
├── scripts/
│   ├── build_example_elf.sh       # Build hello.elf, fib_print.elf, count_primes.elf
│   └── run_riscv_integration.sh   # Build + ctest + M/syscall smoke check (bash: macOS/Linux)
├── tests/
│   ├── rvc_expand_test.cpp        # Compressed-instruction expansion tests
│   └── golden_kernel_test.cpp     # Hex kernel syscall smoke test
├── gui/                   # Qt6 GUI
├── instruction_memory/   # Hex programs (two hex chars per byte, whitespace-separated)
└── build/                 # CMake output (cpusim, cpusim_gui, tests)
```

## 🎯 Features

### Implemented instructions (summary)

**RV32I**: Core integer ops including loads/stores, branches, `JAL`/`JALR`, `LUI`/`AUIPC`. **FENCE** / unknown primary opcodes are treated as NOPs for forward progress.

**RV32M**: `MUL`, `MULH`, `MULHSU`, `MULHU`, `DIV`, `DIVU`, `REM`, `REMU`.

**RV32C** (+ **FC**): Compressed fetch/expand/disassembly for the standard 32-bit-oriented mapping, including **C.FLW** / **C.FSW** / **C.FLWSP** / **C.FSWSP** and **C.EBREAK**.

**RV32F** (subset): `FLW`, `FSW`, `FADD.S`, `FSUB.S`, `FMUL.S`, `FDIV.S`, `FSGNJ.S`, `FMIN.S`, `FMAX.S`, `FSQRT.S`, `FMADD.S`, `FMSUB.S`, `FNMSUB.S`, `FNMADD.S`, comparisons (`FEQ`/`FLT`/`FLE`), `FCVT.W.S`, `FCVT.S.W`, `FMV.X.W`, `FMV.W.X`, `FCLASS.S`. *Not* full spec: e.g. no `FCVT.WU.S` / `FCVT.S.WU`, no `FSGNJN`/`FSGNJX`, fused ops are **single-precision only** (`funct2=00`).

**System / ABI**: `ECALL` with Linux/RISC-V–style nr **93** (exit), **64** (write stdout), **214** (`brk`); `EBREAK` halts the simulation. **Zicsr**: `CSRRW`/`CSRRS`/`CSRRC` and immediate forms decode and execute; **FCSR (`0x001`)** is modeled; other CSRs read as **0** and writes are ignored.

**Running arbitrary compiled C** still depends on your libc/runtime: only the above syscalls and memory model are emulated—link with [`examples/linker.ld`](examples/linker.ld) (or equivalent) so **PT_LOAD** segments fit in **64 KiB** at `0x00000000`.

**Execution environment (summary)**  
| Item | Value |
|------|--------|
| ISA | RV32IMC-oriented (optional F; see limitations above) |
| RAM | 64 KiB byte-addressable, base `0x00000000` |
| Instruction fetch | Same physical RAM as data (unified address space) |
| `ExecutionMode` | **Educational** (default): unknown primary opcodes as NOP. **Executable** (`--executable`): faults on illegal instructions / bad memory accesses. |

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

1. **IF (Instruction Fetch)**: Fetches instructions from **unified memory** (same 64 KiB RAM as load/store)
2. **ID (Decode)**: Decodes instruction, reads registers, generates control signals
3. **EX (Execute)**: Performs ALU operations, calculates branch targets
4. **MEM (Memory)**: Load/store path through the memory interface (typically D-cache backed by RAM)
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
- `golden_kernel_test` — same hex kernel as above, asserts syscall exit code 42
- `hello_elf_test` — ELF load + run via CPU API (requires RISC-V cross-compiler at configure time)
- `load_use_test`, `forwarding_test` — pipeline hazard hex kernels
- `fpu_decode_test`, `trap_test`, `mmu_test`, `executable_mode_test`
- `call_ret_elf_test` — `crt0.S` + `main` with normal `call`/`ret` (requires cross-compiler)
- `fib_print_cpusim`, `count_primes_cpusim`, `bench_smoke_test` — integration smokes (with cross-compiler)

**Scripts**: `scripts/run_riscv_integration.sh` (bash) rebuilds, runs **ctest**, and checks syscall output.

**Windows:** Run the same tests manually: `ctest --test-dir build` from the project root after configuring and building.

**Sample hex programs** live under `instruction_memory/` (e.g. `instMem-m-ext.txt`, `instMem-forward.txt`, `instMem-load-use.txt`, `instMem-rvc-smoke.txt`).

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

- **Qt6 not found**: Install Qt6 and **Charts** for your toolchain.
  - **macOS:** `brew install qt6` — ensure `CMAKE_PREFIX_PATH` points at the Qt prefix if CMake fails (`brew --prefix qt6`).
  - **Linux:** `qt6-base-dev`, `qt6-charts-dev` (or distro-specific Qt6 packages).
  - **Windows:** Set `-DCMAKE_PREFIX_PATH` to your Qt kit’s `lib/cmake` parent (e.g. `C:/Qt/6.7.0/msvc2019_64`).
- **CMake errors**: Ensure CMake >= 3.16
  - Try: `cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/qt6`
- **Compilation errors**: Check that all source files are present and Qt6 Charts is installed

### GUI Won't Run

- **Speed slider stops at 100**: You are likely running an old `cpusim_gui` binary. Reconfigure with `-DBUILD_GUI=ON`, then `cmake --build build --target cpusim_gui` and run `./build/cpusim_gui`. The current max is **500 cycles/sec** (see `SimLimits::MAX_SIM_SPEED_CPS`).

- **Program won't load**: Use a valid **hex text** file (non-empty hex byte pairs) or a **32-bit ELF** linked for `0x00000000` / 64 KiB RAM. If the dialog fails, read the error message — ELF must be **EM_RISCV** with **PT_LOAD** segments that fit.
- **Simulation stuck**: Check the pipeline log file (`pipeline.log`) for details
- **No updates**: Make sure you've clicked "Start" or "Step" to begin execution

### Command-Line Issues

- **Program not found**: Use paths relative to your current directory, or absolute paths. On Windows, quote paths that contain spaces.
- **No output**: Try adding `--debug` flag to see detailed execution

## 📝 Logging

The simulator can generate detailed execution logs:

- **GUI**: Writes `pipeline.log` (the GUI tries to place it near the project root when it can find `CMakeLists.txt`; otherwise next to the opened program). Check the status/debug output if you need the exact path.
- **Command-Line**: Use `--log <filename>` to specify log file (use a path that works on your OS).
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
