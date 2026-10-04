# Video Scripts — CPU_SIM Educational Demo Series

A 4-video series demonstrating how to use this **dual-ISA** (RISC-V + AArch32) CPU simulator as a computer architecture teaching tool. Each video is roughly **5–8 minutes**.

Primary demos use **RISC-V** (faster ELFs, hex files without a cross-compiler). AArch32 mirrors the same labs — call that out in Video 1 and optionally show a short ARM clip.

---

## Before You Record (One-Time Prep)

**Build and verify everything works:**

```bash
cmake -S . -B build -DBUILD_GUI=ON \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6 2>/dev/null || brew --prefix qt 2>/dev/null || echo "")"
cmake --build build -j
./scripts/build_example_elf.sh
./scripts/build_example_elf_arm.sh   # optional but recommended for dual-ISA clips
./scripts/demo.sh
```

**Have these ready to open:**

- `build/cpusim_gui`
- RISC-V: `build/hello.elf`, `build/fib_print.elf`, `build/count_primes.elf`
- AArch32 (optional clips): `build/hello_arm.elf`, `build/fib_print_arm.elf`, `build/count_primes_arm.elf`
- Hex (no toolchain): `instruction_memory/instMem-forward.txt` (RV) and `instMem-forward-arm.txt` (ARM)
- [INSTRUCTOR.md](INSTRUCTOR.md) or [DATAPATH+Controller.pdf](DATAPATH%2BController.pdf) (optional architecture slide)

**Recording tips:**

- Use 1400×900 window (default GUI size) or zoom the pipeline panel
- Hide desktop clutter; use a clean terminal theme
- Pause simulation before switching cache/predictor (changes apply on Reset)
- For ARM `count_primes_arm.elf`, allow ~1M cycles (soft remainder; slower than RV)

**Suggested recording order:** Record Video 2 first (pipeline stepping), then Videos 3 and 4, then Video 1 as a voice-over with B-roll from the other sessions.

---

## Video 1 — “What Is This, and Why Use It in Class?”

**Target length:** 5–6 min  
**Goal:** Hook viewers, explain the educational niche, show that it runs

### Opening (0:00–0:45)

**SHOW:** Title card → GitHub repo README → quick scroll of project structure (`src/core`, `src/core/isa`, `src/memory`, `gui/`)

**SAY:**

> This is a cycle-accurate CPU simulator built for computer architecture education. It runs **two ISAs** — 32-bit RISC-V and educational AArch32 — in the *same* five-stage pipeline shell, with the same caches and branch predictors. Unlike a disassembler or a trace dump, you can *watch* instructions move through fetch, decode, execute, memory, and writeback cycle by cycle. You can also swap cache organizations and branch predictors and immediately see the impact on hit rate, prediction accuracy, and CPI. And if you're teaching a lab, students can implement their own cache or branch predictor and test it in the same GUI.

### What It Models (0:45–2:00)

**SHOW:** Simple diagram (from [INSTRUCTOR.md](INSTRUCTOR.md) or draw on screen):

```
IF → ID → EX → MEM → WB
         ↓         ↓
   Branch Predictor  Cache → 64 KiB RAM
         ↑
   RV32 or AArch32 backend (from ELF / hex name)
```

**SAY:**

> The simulator models a five-stage pipeline with hazard detection, forwarding, branch prediction, and a unified cache in front of 64 kilobytes of RAM. Load a RISC-V or ARM ELF and the ISA is selected automatically. Hex teaching files work too — names with “arm” use AArch32 encodings. It's a teaching tool — not a replacement for QEMU or Spike — but that's the point. Everything is visible and measurable.

> The code is organized the same way you'd teach the machine: core datapath and ISA backends in `src/core`, memory hierarchy and branch prediction in `src/memory`, and a Qt GUI for visualization.

### Quick Start Demo (2:00–4:30)

**SHOW:** Terminal — build commands, then launch GUI:

```bash
./build/cpusim_gui
```

**SAY:**

> Build once with CMake and Qt 6. The GUI is the main teaching interface — the command-line tool is there for scripting and benchmarks.

**SHOW:** Open `build/hello.elf` → click **Start** → switch to **Program Output** tab. Optionally flash `build/hello_arm.elf` and note **Register File (AArch32)** + CPSR.

**SAY:**

> Programs are bare-metal ELF files cross-compiled for this memory map — RISC-V or AArch32 — or hex instruction files in `instruction_memory/`. Hello is tiny — it exits with a syscall — but it proves the full path works: load, execute, syscall emulation, done. On ARM you also see CPSR flags in the register pane.

**SHOW:** Briefly flash **Statistics** tab (CPI, instruction counts)

**SAY:**

> Every run collects performance stats automatically. We'll dig into those in the next videos.

### What Makes It Educational (4:30–5:30)

**SHOW:** README “For Instructors” section + links to [CACHE_SCHEMES.md](CACHE_SCHEMES.md) and [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md)

**SAY:**

> Three things make this useful in a classroom: visualization — you see stalls and flushes; experimentation — swap cache or predictor and rerun; and implementation — students extend the simulator with their own algorithms, with step-by-step guides included. Dual ISA means you can compare the *same* lab on RISC-V and ARM without switching tools.

### Outro (5:30–6:00)

**SAY:**

> In the next video, we'll step through the pipeline and look at hazards, forwarding, and dependencies — the material that's usually hardest to teach from a textbook alone.

**Suggested title:** *Dual-ISA CPU Simulator for Computer Architecture Classes (RISC-V + ARM Overview)*

---

## Video 2 — “Seeing the Pipeline: Stalls, Forwarding, and Dependencies”

**Target length:** 6–8 min  
**Goal:** Demonstrate core pedagogical value — cycle-by-cycle visibility

### Setup (0:00–0:30)

**SHOW:** GUI with `instruction_memory/instMem-forward.txt` loaded (or `instMem-load-use.txt` for stalls)

**SAY:**

> Textbooks show pipeline diagrams as static pictures. Here, the pipeline is live. I'll use a hand-written hex program first — no cross-compiler needed — so you can reproduce this exactly. These encodings are RISC-V; the repo also has `instMem-forward-arm.txt` if your course prefers AArch32.

### Pipeline Tab Walkthrough (0:30–2:30)

**SHOW:** **Pipeline Execution Trace** tab → click **Reset** → click **Step** repeatedly (5–10 times)

**SAY (while stepping):**

> Each row is one cycle. You can see which stage each instruction is in: IF, ID, EX, MEM, WB. The PC updates every cycle. When an instruction moves from decode to execute, you're watching the same flow you'd draw on a whiteboard — except it's accurate.

**SHOW:** Point at an instruction name/disassembly in the pipeline view

**SAY:**

> Instructions are disassembled in place, so students connect assembly to pipeline behavior without switching tools.

### Register File (2:30–3:30)

**SHOW:** **Register File** tab while stepping

**SAY:**

> The register file shows the active ISA's GPRs updating in real time — x0 through x31 on RISC-V, or r0 through r15 plus CPSR on AArch32. On RISC-V, x0 stays zero — the simulator enforces that. When writeback commits a result, you see it here the same cycle.

### Forwarding Demo (3:30–5:00)

**SHOW:** Load `instMem-forward.txt` if not already loaded → Step until a forwarding case is visible (or run slowly with **Start** then **Pause**)

**SAY:**

> This program is designed to exercise data forwarding. When an instruction needs a register value that's still in the pipeline, the simulator forwards from EX/MEM or MEM/WB instead of stalling. In a lecture, you'd pause right here and ask: "Where is the producer? Where is the consumer? Which forwarding path is used?"

**SHOW:** **Instruction Dependencies** tab

**SAY:**

> The dependency view records RAW — read-after-write — relationships automatically. That's useful for grading labs: students can verify their hazard analysis against what the simulator detected.

### Load-Use Stall (optional, 5:00–6:00)

**SHOW:** Load `instMem-load-use.txt` → Step through a load-use hazard

**SAY:**

> When forwarding isn't enough — like a load followed immediately by a use — the pipeline stalls. You'll see the bubble in the pipeline trace and the stall count increment in Statistics. This is the classic pipeline hazard lecture, but interactive.

### Memory Access History (6:00–6:45)

**SHOW:** **Memory Access History** tab

**SAY:**

> Every load and store is logged with address, value, and cache hit or miss. Instruction fetches also go through the cache, so students see the full memory traffic picture.

### Optional 20-sec ARM hex cutaway

**SHOW:** Open `instMem-forward-arm.txt` → Step once or twice → Registers show **AArch32** + CPSR

**SAY:**

> Same hazard idea on ARM encodings — filename picks the ISA automatically. Pipeline shell, cache, and predictors don't change.

### Outro (6:45–7:15)

**SAY:**

> The pipeline view answers "what happens cycle by cycle?" Next, we'll use a real C program and experiment with branch predictors — where small algorithm changes produce big CPI swings.

**Suggested title:** *Watch a 5-Stage Pipeline Run Cycle-by-Cycle (Educational CPU Sim)*

---

## Video 3 — “Branch Prediction Lab: Compare, Measure, Learn”

**Target length:** 6–7 min  
**Goal:** Show experimentation workflow for branch prediction

### Setup (0:00–0:30)

**SHOW:** Open `build/count_primes.elf` — show source briefly (`examples/count_primes.c`)

**SAY:**

> This program counts primes from 2 to 400 using nested loops and trial division. It's branch-heavy — perfect for branch prediction labs. The comment in the source even says so. There's a matching `count_primes_arm.elf` if you want the same experiment on AArch32 — it takes longer because ARM demos use a soft remainder instead of hardware divide.

### Baseline: Always Not Taken (0:30–2:30)

**SHOW:** Branch Predictor dropdown → **Always Not Taken** → **Reset** → **Start** → let it run → **Statistics** tab

**SAY:**

> I'll start with the simplest predictor: always not taken. This is the baseline every textbook uses. After the run, look at Statistics.

**SHOW:** Point to:

- Branch Predictor accuracy (~37% for ANT on the RISC-V program)
- CPI
- Branch mispredictions / flushes

**SAY:**

> On this program, always-not-taken gets roughly a third of branches right — many loops back and take the branch, so the predictor is wrong constantly. Mispredictions cause pipeline flushes, and CPI goes up. Students can connect the algorithm to measurable performance.

### Upgrade: GShare (2:30–4:30)

**SHOW:** **Reset** → switch to **GShare** → **Start** → Statistics again

**SAY:**

> Now GShare — it XORs a global history register with the PC to index a table of two-bit saturating counters. Same program, same hardware, different predictor.

**SHOW:** Side-by-side or cut between the two accuracy numbers (ANT ~37% vs GShare ~96% on RISC-V)

**SAY:**

> Accuracy jumps dramatically. CPI drops. Flushes drop. This is the "aha moment" for branch prediction — the hardware cost is a table and some bits of history, but the performance win is real.

### Step Through a Mispredict (4:30–5:30)

**SHOW:** **Reset** → **Always Not Taken** → **Step** through cycles near a branch instruction

**SAY:**

> When you step, you can catch a mispredict in the act: the predictor guesses in ID, fetch continues down the wrong path, then EX resolves the branch and the pipeline flushes. That's control hazard material that's hard to convey with a static slide.

### CLI Benchmark (optional, 5:30–6:15)

**SHOW:** Terminal:

```bash
./build/cpusim build/count_primes.elf --predictor ant --bench --json
./build/cpusim build/count_primes.elf --predictor gshare --bench --json
# Optional ARM twin (needs higher cycle budget):
./build/cpusim build/count_primes_arm.elf --predictor ant --bench --json --max-cycles 1500000
./build/cpusim build/count_primes_arm.elf --predictor gshare --bench --json --max-cycles 1500000
```

**SAY:**

> For assignments, the CLI exports JSON with cycles, CPI, mispredictions, and predictor accuracy. Students can script comparisons across all five built-in predictors: always taken, always not taken, bimodal, GShare, and tournament — on RISC-V or ARM ELFs.

### Classroom Assignment Hook (6:15–6:45)

**SHOW:** [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md) — scroll to “Adding Your Own Branch Predictor”

**SAY:**

> The lab assignment writes itself: implement a predictor — maybe local history or a hybrid — wire it into the enum and GUI dropdown, rerun count_primes, and report accuracy versus CPI. The guide walks through all four touch points.

### Outro

**SAY:**

> Next: cache organizations — same workflow, different metric. Hit rate instead of prediction accuracy.

**Suggested title:** *Branch Prediction Lab: From 37% to 96% Accuracy in One Dropdown*

---

## Video 4 — “Cache Experiments and Building Your Own Components”

**Target length:** 7–8 min  
**Goal:** Cache comparison + extensibility as capstone educational message

### Cache Comparison Setup (0:00–1:00)

**SHOW:** Open `build/fib_print.elf` — briefly show `examples/fib_print.c` (loop + many prints)

**SAY:**

> Fibonacci prints 24 lines via syscalls. It has a loop plus lots of memory traffic from printing — good for comparing cache organizations. `fib_print_arm.elf` is the same lab on AArch32.

**SHOW:** Cache dropdown → **Direct Mapped** → **Reset** → **Start** → **Statistics**

**SAY:**

> Direct-mapped cache: each memory block maps to exactly one line. Fast lookup, but conflict misses when two blocks fight for the same line.

**SHOW:** Note cache hits, misses, hit rate

### Compare Set-Associative (1:00–2:30)

**SHOW:** **Reset** → **4-Way Set Associative** → **Start** → Statistics

**SAY:**

> Four-way set associative: blocks map to a set, then LRU picks a victim within that set. On programs with more complex access patterns, hit rate often improves over direct-mapped — though on small programs the difference can be modest.

**SHOW:** Optionally try **Fully Associative** for “best hit rate, highest lookup cost” talking point

**SAY:**

> The GUI also offers 2-way, 4-way, 8-way, and fully associative. The lab question is always: given a fixed 4 KB cache with 32-byte lines, which organization wins on *your* workload — and why?

### Memory Tab Tie-In (2:30–3:30)

**SHOW:** **Memory Access History** — filter visually for HIT vs MISS rows

**SAY:**

> Students can correlate misses with specific instructions and addresses. That connects the abstract "conflict miss" concept to concrete addresses in the trace.

### Student Extension: Cache (3:30–5:00)

**SHOW:** [CACHE_SCHEMES.md](CACHE_SCHEMES.md) — Step 1 (implement class in `src/memory/Cache.h`)

**SAY:**

> For a cache lab, students implement a new class — FIFO or random replacement, for example — inherit from CacheScheme, implement load and store, track hits and misses. Then they add an enum value, a factory case, and a GUI dropdown entry. Four touch points, documented step by step.

**SHOW:** Briefly flash `src/memory/CacheScheme.h` interface (`load`, `store`, `hits`, `misses`)

**SAY:**

> The interface is intentionally small. Students focus on the replacement policy, not boilerplate.

### Student Extension: Branch Predictor (5:00–6:00)

**SHOW:** [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md) — same four-step pattern in `src/memory/BranchPredictor.h`

**SAY:**

> Branch predictors follow the same pattern: predict in ID, update in EX, expose accuracy statistics. The pipeline already calls your code — you just implement the algorithm.

### Putting It Together: A Full Course Narrative (6:00–7:15)

**SHOW:** Montage or checklist on screen:

| Week / Topic | Tool in CPU_SIM |
|--------------|-----------------|
| Pipeline basics | Step + Pipeline tab (RV or ARM hex) |
| Hazards & forwarding | `instMem-forward.txt` / `*-arm.txt`, Dependencies tab |
| Branch prediction | `count_primes.elf` (or `_arm`), predictor dropdown |
| Memory hierarchy | `fib_print.elf` (or `_arm`), cache dropdown |
| Dual-ISA compare | Same lab on RV32 vs AArch32 ELF |
| Implementation project | Custom cache or predictor |

**SAY:**

> A full architecture course can progress from "watch the pipeline" to "measure predictors" to "implement your own" — and optionally compare RISC-V versus ARM in the same GUI. The simulator grows with the syllabus instead of switching tools every unit.

### Honest Limitations (7:15–7:45)

**SAY:**

> Quick scope note for instructors: 64 KB RAM, unified cache, write-through only, conditional branches only for prediction. RISC-V has a larger teaching subset including compressed and floating-point decode; AArch32 v1 is integer A32 without Thumb or VFP. It's a teaching model, not silicon-accurate. Documenting those limits is part of teaching students to read assumptions — which is itself a valuable skill.

### Closing (7:45–8:00)

**SHOW:** Repo URL, `./scripts/demo.sh` running successfully (RV + ARM sections)

**SAY:**

> Everything is open source with CI, tests for both ISAs, and a one-command demo script. If you're teaching computer architecture — or learning it — this simulator is built to make the invisible parts of a CPU visible. Link in the description.

**Suggested title:** *Cache Experiments + How Students Build Their Own Predictors*

---

## Bonus Clip (30 sec) — “Hex Programs, No Toolchain”

For students who can't install a cross-compiler:

**SHOW:**

```bash
./build/cpusim instruction_memory/instMem-forward.txt
./build/cpusim instruction_memory/instMem-forward-arm.txt   # AArch32 hex, auto ISA
```

**SAY:**

> You don't need to compile C to use this. The `instruction_memory/` folder has ready-made hex programs for forwarding, load-use hazards, and syscalls — RISC-V and AArch32 — zero setup beyond building the simulator.

---

## Suggested YouTube Titles

| # | Title |
|---|-------|
| 1 | Dual-ISA CPU Simulator for Computer Architecture Classes (RISC-V + ARM Overview) |
| 2 | Watch a 5-Stage Pipeline Run Cycle-by-Cycle (Educational CPU Sim) |
| 3 | Branch Prediction Lab: From 37% to 96% Accuracy in One Dropdown |
| 4 | Cache Experiments + How Students Build Their Own Predictors |

---

## Related Docs

- [INSTRUCTOR.md](INSTRUCTOR.md) — 10-minute live demo script for professors
- [README.md](README.md) — full user documentation
- [GUI_BUILD.md](GUI_BUILD.md) — Qt build instructions
- [CACHE_SCHEMES.md](CACHE_SCHEMES.md) — custom cache lab guide
- [BRANCH_PREDICTORS.md](BRANCH_PREDICTORS.md) — custom branch predictor lab guide
