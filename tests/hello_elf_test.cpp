// Smoke test: RV32 ELF load + run (same path as GUI SimulatorController).
#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "MemoryMap.h"
#include "ElfLoader.h"
#include "ExecutionMode.h"
#include "test_common.h"

#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: hello_elf_test <program.elf>\n");
        return 1;
    }

    const std::string elf_path = argv[1];

    SimpleRAM dram(MemoryMap::RAM_SIZE);
    ElfLoadResult lr = load_elf32_into_ram(elf_path, dram);
    if (!lr.ok) {
        std::fprintf(stderr, "ELF load failed: %s\n", lr.error.c_str());
        return 2;
    }

    DirectMappedCache dcache(&dram, 4 * 1024, 32);
    CPU cpu;
    cpu.set_data_memory(&dcache);
    cpu.set_ram_size(MemoryMap::RAM_SIZE);
    cpu.set_use_hex_bounds(false);
    cpu.set_pc(lr.entry);
    cpu.set_heap_brk(lr.heap_brk);
    cpu.set_register_value(2, static_cast<int32_t>(MemoryMap::STACK_TOP - 16));
    cpu.set_execution_mode(ExecutionMode::Educational);

    const int max_cycles = TestCommon::DEFAULT_TEST_MAX_CYCLES;
    int c = 0;
    while (c < max_cycles) {
        ++c;
        cpu.run_pipeline_cycle(c, false);
        if (cpu.is_halted()) {
            break;
        }
        if (cpu.is_faulted()) {
            std::fprintf(stderr, "unexpected fault\n");
            return 3;
        }
    }

    if (c >= max_cycles) {
        std::fprintf(stderr, "timeout\n");
        return 4;
    }

    if (!cpu.exited_via_syscall()) {
        std::fprintf(stderr, "expected syscall exit\n");
        return 5;
    }
    if (cpu.get_syscall_exit_code() != 42) {
        std::fprintf(stderr, "bad exit code %d\n", cpu.get_syscall_exit_code());
        return 6;
    }

    std::printf("hello_elf_test ok (cycles=%d)\n", c);
    return 0;
}
