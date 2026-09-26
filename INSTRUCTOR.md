# Instructor Guide — Dual-ISA CPU Simulator (RISC-V + AArch32)

A concise overview for instructors evaluating this simulator for a computer architecture course.

## Elevator Pitch

- **Visual 5-stage pipeline** — Cycle-accurate simulation with real-time pipeline, register, memory, and dependency views. Students see stalls, forwarding, flushes, and branch mispredictions as they happen.
- **Dual ISA** — Same pipeline shell, caches, and predictors for **RV32** and educational **AArch32**. ELF auto-selects ISA; hex filenames containing `arm` select AArch32.
- **Unified cache framework** — Compare direct-mapped, fully associative, and set-associative (2/4/8-way) caches. Students implement custom replacement policies (FIFO, random, etc.) following [CACHE_SCHEMES.md](CACHE_SCHEMES.md).
- **Branch predictor framework** — Compare static baselines through bimodal, GShare, and tournament predictors. Students implement custom algorithms following [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md).

## Model Assumptions (Scope)

This is a **teaching simulator**, not a formal ISA validator or QEMU replacement.

| Item | Value |
|------|--------|
| ISA | RV32IMCF-oriented subset **and** educational AArch32 A32 (see [README.md](README.md#-features)) |
| RAM | 64 KiB flat physical memory at `0x00000000` |
| Cache | Unified 4 KiB, 32-byte lines (not split I/D) |
| Write policy | Write-through, write-allocate only |
| Branch prediction | Conditional branches only; unconditional jumps bypass predictor |
| Mispredict penalty | Pipeline flush (no extra stall cycles beyond flush) |
| Program loading | Hex text (RV or `*-arm.txt`) or ELF (`EM_RISCV` / `EM_ARM`) |

**Demo tip:** Use the `_start` + [examples/simlib.h](examples/simlib.h) / [simlib_arm.h](examples/simlib_arm.h) pattern. Programs that use normal `call`/`ret` or `BL` with stack-saved link may misbehave due to a known forwarding limitation.

## Recommended Demo Programs

| Demo point | RISC-V | AArch32 |
|------------|--------|---------|
| Pipeline + syscalls | `build/hello.elf` | `build/hello_arm.elf` |
| Cache hit-rate contrast | `build/fib_print.elf` | `build/fib_print_arm.elf` |
| Branch predictor contrast | `build/count_primes.elf` | `build/count_primes_arm.elf` (~900k cycles) |
| Hex-only fallback | `instMem-forward.txt` | `instMem-forward-arm.txt` |

```bash
./scripts/build_example_elf.sh       # needs riscv64-elf-gcc (or equiv.)
./scripts/build_example_elf_arm.sh   # needs arm-none-eabi-gcc
```

## 10-Minute Live Demo Script

### GUI path (primary — ~8 min)

1. **Build and launch**
   ```bash
   cmake -S . -B build -DBUILD_GUI=ON \
     -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6 2>/dev/null || brew --prefix qt 2>/dev/null || echo "")"
   cmake --build build -j
   ./build/cpusim_gui
   ```

2. **Load a branch-heavy program** — Open `build/count_primes.elf` (build first with `./scripts/build_example_elf.sh`).

3. **Branch predictors** — With **Always Not Taken** selected, click **Start**. Open the **Statistics** tab and note prediction accuracy and CPI. **Reset**, switch to **GShare**, run again, and compare accuracy and CPI.

4. **Pipeline visualization** — **Reset**, use **Step** through a few cycles near a branch. Point out the predictor decision in ID and a flush on mispredict.

5. **Cache schemes** — **Reset**, open `build/fib_print.elf`. Run with **Direct Mapped**, note hit rate. **Reset**, switch to **4-way Set-Associative**, run again, compare hit rate.

6. **Optional AArch32 beat (30–60s)** — Open `build/hello_arm.elf`, show **Register File (AArch32)** and CPSR, Start → exit 42. Same pipeline/cache UI.

7. **Extension hook** — Open [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md) and show Step 1: students add a class in `src/memory/BranchPredictor.h`, wire the enum, factory, and GUI dropdown.

### CLI backup (~2 min)

If Qt is unavailable on the demo machine:

```bash
./scripts/demo.sh
```

Or manually:

```bash
./scripts/build_example_elf.sh
./build/cpusim build/count_primes.elf --predictor ant --bench --json
./build/cpusim build/count_primes.elf --predictor gshare --bench --json
./build/cpusim build/fib_print.elf --cache direct --bench --json
./build/cpusim build/fib_print.elf --cache 4way --bench --json
```

## Architecture Overview

```mermaid
flowchart LR
  subgraph pipeline [FiveStagePipeline]
    IF[IF] --> ID[ID]
    ID --> EX[EX]
    EX --> MEM[MEM]
    MEM --> WB[WB]
  end
  ID -->|"predict branch"| BP[BranchPredictor]
  EX -->|"update + resolve"| BP
  IF -->|"fetch"| Cache[UnifiedCache]
  MEM -->|"load/store"| Cache
  Cache --> RAM[SimpleRAM_64KiB]
  ELF[ELF_or_hex] -->|"IsaKind"| ISA[RV32_or_AArch32]
  ISA --> ID
```

For a detailed datapath diagram, see [DATAPATH+Controller.pdf](DATAPATH%2BController.pdf).

**Pipeline stages:**
1. **IF** — Fetch from unified memory (through cache)
2. **ID** — Decode, read registers, branch prediction
3. **EX** — ALU, branch resolution, forwarding
4. **MEM** — Load/store through cache
5. **WB** — Writeback to register file

## Student Extension Workflow

Students implement custom predictors or caches by editing core headers (four touch points):

1. Implement the class in [`src/memory/BranchPredictor.h`](src/memory/BranchPredictor.h) or [`src/memory/Cache.h`](src/memory/Cache.h)
2. Add an enum value in [`src/memory/BranchPredictorScheme.h`](src/memory/BranchPredictorScheme.h) or [`src/memory/CacheScheme.h`](src/memory/CacheScheme.h)
3. Add a factory case in `createBranchPredictor()` or `createCacheScheme()`
4. Add a GUI dropdown entry in [gui/MainWindow.cpp](gui/MainWindow.cpp)

After these steps, the new scheme appears in the GUI dropdown. See the step-by-step guides:

- [CACHE_SCHEMES.md](CACHE_SCHEMES.md) — cache replacement policies, write policies
- [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md) — saturating counters, global history, hybrids

## Pre-Demo Checklist

Run before meeting with your professor:

```bash
./scripts/demo.sh          # build + canned comparisons (RV + ARM if toolchains present)
ctest --test-dir build     # regression tests
```

Verify `./build/cpusim_gui` launches and can open an ELF file.

## Future Classroom Enhancements

If adopted for a full course, natural next steps include:

- Pre-written lab assignments and grading scripts
- Dedicated unit tests with golden hit-rate / accuracy baselines
- Configurable cache size and associativity via CLI flags
- Predictor state visualization in the GUI
- Fix for `call`/`ret` forwarding limitation
- Broader AArch32 (Thumb / VFP) if the course needs it

## Additional Resources

- [README.md](README.md) — full user documentation
- [VIDEO_SCRIPTS.md](VIDEO_SCRIPTS.md) — 4-video recording scripts
- [GUI_BUILD.md](GUI_BUILD.md) — Qt build instructions
- CI: [GitHub Actions](https://github.com/nishadelias/CPU_SIM/actions) (Ubuntu + macOS; RISC-V and ARM toolchains)
