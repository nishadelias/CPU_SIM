#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "MemoryMap.h"
#include "ElfLoader.h"
#include "ExecutionMode.h"
#include "test_common.h"

#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: call_ret_elf_test <program.elf>\n");
        return 1;
    }

    SimpleRAM dram(MemoryMap::RAM_SIZE);
    ElfLoadResult lr = load_elf32_into_ram(argv[1], dram);
    if (!lr.ok) {
        std::fprintf(stderr, "elf load failed: %s\n", lr.error.c_str());
        return 2;
    }

    DirectMappedCache dcache(&dram, 4 * 1024, 32);
    CPU cpu;
    cpu.set_data_memory(&dcache);
    cpu.set_ram_size(MemoryMap::RAM_SIZE);
    cpu.set_pc(lr.entry);
    cpu.set_heap_brk(lr.heap_brk);
    cpu.set_register_value(2, static_cast<int32_t>(MemoryMap::STACK_TOP - 16));

    int c = 0;
    while (c < TestCommon::DEFAULT_TEST_MAX_CYCLES) {
        ++c;
        cpu.run_pipeline_cycle(c, false);
        if (cpu.exited_via_syscall() || cpu.is_halted()) {
            break;
        }
    }

    if (!cpu.exited_via_syscall() || cpu.get_syscall_exit_code() != 42) {
        std::fprintf(stderr, "expected exit 42\n");
        return 3;
    }
    std::printf("call_ret_elf_test ok\n");
    return 0;
}
