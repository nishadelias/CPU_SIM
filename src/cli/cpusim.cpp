// file: cpusim.cpp

#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "CacheScheme.h"
#include "BranchPredictor.h"
#include "MemoryMap.h"
#include "ElfLoader.h"
#include "HexLoader.h"
#include "ExecutionMode.h"
#include "SimLimits.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>

using namespace std;

static bool is_elf_file(const string& path) {
    ifstream f(path, ios::binary);
    if (!f) {
        return false;
    }
    char magic[4];
    f.read(magic, 4);
    return f.gcount() == 4 && static_cast<unsigned char>(magic[0]) == 0x7f &&
           magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F';
}

static CacheSchemeType parse_cache(const string& s) {
    if (s == "direct" || s == "dm") {
        return CacheSchemeType::DirectMapped;
    }
    if (s == "fa" || s == "fully") {
        return CacheSchemeType::FullyAssociative;
    }
    if (s == "2way" || s == "2") {
        return CacheSchemeType::SetAssociative2Way;
    }
    if (s == "4way" || s == "4") {
        return CacheSchemeType::SetAssociative4Way;
    }
    if (s == "8way" || s == "8") {
        return CacheSchemeType::SetAssociative8Way;
    }
    return CacheSchemeType::DirectMapped;
}

static BranchPredictorType parse_predictor(const string& s) {
    if (s == "ant" || s == "not_taken") {
        return BranchPredictorType::AlwaysNotTaken;
    }
    if (s == "at" || s == "taken") {
        return BranchPredictorType::AlwaysTaken;
    }
    if (s == "bimodal" || s == "bi") {
        return BranchPredictorType::Bimodal;
    }
    if (s == "gshare" || s == "gs") {
        return BranchPredictorType::GShare;
    }
    if (s == "tournament" || s == "tour") {
        return BranchPredictorType::Tournament;
    }
    return BranchPredictorType::AlwaysNotTaken;
}

static void print_usage(const char* prog) {
    cout << "Usage:\n  " << prog
         << " <program.hex|program.elf> [options]\n"
            "Options:\n"
            "  --debug                 Verbose pipeline trace\n"
            "  --log <file>            Pipeline log file\n"
            "  --executable            Strict RV32 trap behavior (illegal insn / memory faults)\n"
            "  --bench                 Benchmark mode: print CSV/JSON stats and exit\n"
            "  --json                  With --bench: JSON output (default CSV)\n"
            "  --cache <scheme>        direct|fa|2way|4way|8way (default direct)\n"
            "  --predictor <p>         ant|at|bimodal|gshare|tournament (default ant)\n"
            "  --max-cycles <n>        Cycle limit (default 200000)\n";
}

static void run_print_results(CPU& cpu, int cycles, bool bench_json) {
    const CPUStatistics& st = cpu.get_statistics();
    if (bench_json) {
        cout << "{\"cycles\":" << cycles
             << ",\"instructions_retired\":" << st.instructions_retired
             << ",\"cpi\":" << st.getCPI()
             << ",\"cache_hits\":" << st.cache_hits
             << ",\"cache_misses\":" << st.cache_misses
             << ",\"cache_hit_rate\":" << st.getCacheHitRate()
             << ",\"stalls\":" << st.stall_count
             << ",\"flushes\":" << st.flush_count
             << ",\"branch_mispredictions\":" << st.branch_mispredictions
             << ",\"branch_taken\":" << st.branch_taken_count
             << ",\"branch_not_taken\":" << st.branch_not_taken_count
             << ",\"exited_syscall\":" << (cpu.exited_via_syscall() ? "true" : "false")
             << ",\"syscall_exit_code\":" << cpu.get_syscall_exit_code()
             << ",\"faulted\":" << (cpu.is_faulted() ? "true" : "false")
             << ",\"fault_cause\":" << static_cast<int>(cpu.get_fault_cause())
             << "}\n";
    } else {
        cout << "cycles," << cycles << "\n";
        cout << "instructions_retired," << st.instructions_retired << "\n";
        cout << "cpi," << st.getCPI() << "\n";
        cout << "cache_hits," << st.cache_hits << "\n";
        cout << "cache_misses," << st.cache_misses << "\n";
        cout << "cache_hit_rate," << st.getCacheHitRate() << "\n";
        cout << "stalls," << st.stall_count << "\n";
        cout << "flushes," << st.flush_count << "\n";
        cout << "branch_mispredictions," << st.branch_mispredictions << "\n";
    }
}

int main(int argc, char* argv[]) {
    bool debug = false;
    bool enable_logging = false;
    string log_filename;
    bool executable_mode = false;
    bool bench = false;
    bool bench_json = false;
    CacheSchemeType cache_type = CacheSchemeType::DirectMapped;
    BranchPredictorType pred_type = BranchPredictorType::AlwaysNotTaken;
    int max_cycles = SimLimits::DEFAULT_MAX_CYCLES;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    string program_path = argv[1];

    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--debug") {
            debug = true;
        } else if (arg == "--log" && i + 1 < argc) {
            enable_logging = true;
            log_filename = argv[++i];
        } else if (arg == "--executable") {
            executable_mode = true;
        } else if (arg == "--bench") {
            bench = true;
        } else if (arg == "--json") {
            bench_json = true;
        } else if (arg == "--cache" && i + 1 < argc) {
            cache_type = parse_cache(argv[++i]);
        } else if (arg == "--predictor" && i + 1 < argc) {
            pred_type = parse_predictor(argv[++i]);
        } else if (arg == "--max-cycles" && i + 1 < argc) {
            max_cycles = atoi(argv[++i]);
        } else {
            cerr << "Unknown option: " << arg << endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    SimpleRAM dram(MemoryMap::RAM_SIZE);
    bool loaded_elf = is_elf_file(program_path);
    uint32_t hex_bytes = 0;
    ElfLoadResult elf_r;

    if (loaded_elf) {
        elf_r = load_elf32_into_ram(program_path, dram);
        if (!elf_r.ok) {
            cerr << "ELF load error: " << elf_r.error << endl;
            return 1;
        }
    } else {
        if (!load_hex_text_file(program_path, dram, MemoryMap::HEX_PROGRAM_BASE, hex_bytes)) {
            cerr << "error loading hex file\n";
            return 1;
        }
    }

    CacheScheme* dcache = createCacheScheme(cache_type, &dram, 4 * 1024, 32);
    BranchPredictorScheme* bp = createBranchPredictor(pred_type);

    CPU myCPU;
    myCPU.set_data_memory(dcache);
    myCPU.set_branch_predictor(bp);
    myCPU.set_ram_size(MemoryMap::RAM_SIZE);
    myCPU.set_execution_mode(executable_mode ? ExecutionMode::Executable : ExecutionMode::Educational);

    if (loaded_elf) {
        myCPU.set_use_hex_bounds(false);
        myCPU.set_max_pc(0);
        myCPU.set_pc(elf_r.entry);
        myCPU.set_heap_brk(elf_r.heap_brk);
        myCPU.set_register_value(2, static_cast<int32_t>(MemoryMap::STACK_TOP - 16));
    } else {
        myCPU.set_use_hex_bounds(true);
        myCPU.set_max_pc(static_cast<int>(hex_bytes));
        myCPU.set_pc(MemoryMap::HEX_PROGRAM_BASE);
        myCPU.set_heap_brk(0);
    }

    if (enable_logging) {
        myCPU.set_logging(true, log_filename);
    }

    if (debug) {
        cout << dec;
        cout << "Starting pipeline simulation...\n";
        if (loaded_elf) {
            cout << "ELF entry: 0x" << hex << elf_r.entry << dec << " heap_brk: 0x" << hex << elf_r.heap_brk << dec
                 << "\n";
        } else {
            cout << "Hex bytes: " << hex_bytes << " maxPC: " << hex_bytes << "\n";
        }
    }

    int cycle = 0;
    while (cycle < max_cycles) {
        cycle++;
        myCPU.run_pipeline_cycle(cycle, debug);

        if (myCPU.is_halted()) {
            if (debug) {
                if (myCPU.exited_via_syscall()) {
                    cout << "Simulation halted (ECALL exit) at cycle " << cycle << endl;
                } else if (myCPU.is_faulted()) {
                    cout << "Simulation halted (fault) at cycle " << cycle << endl;
                } else {
                    cout << "Simulation halted (EBREAK) at cycle " << cycle << endl;
                }
            }
            break;
        }

        if (myCPU.is_faulted()) {
            break;
        }

        if (!loaded_elf && myCPU.is_pipeline_empty() && myCPU.readPC() >= static_cast<unsigned long>(hex_bytes)) {
            if (debug) {
                cout << "Pipeline empty and end of program reached at cycle " << cycle << endl;
            }
            break;
        }

        if (debug && cycle % 100 == 0) {
            cout << "Cycle " << cycle << ": PC=0x" << hex << myCPU.readPC() << dec << endl;
        }
    }

    if (cycle >= max_cycles) {
        cout << "Warning: Maximum cycles reached. Simulation stopped." << endl;
    }

    if (bench) {
        run_print_results(myCPU, cycle, bench_json);
    } else {
        cout << dec;
        cout << "\n=== Final Results ===" << endl;
        cout << "Total cycles: " << cycle << endl;
        if (myCPU.exited_via_syscall()) {
            cout << "Syscall exit code: " << myCPU.get_syscall_exit_code() << endl;
        }
        if (myCPU.is_faulted()) {
            cout << "Fault cause: " << static_cast<int>(myCPU.get_fault_cause())
                 << " tval: 0x" << hex << myCPU.get_fault_tval() << dec << endl;
        }
        myCPU.print_all_registers();
    }

    delete dcache;
    delete bp;
    return myCPU.is_faulted() ? 2 : 0;
}
