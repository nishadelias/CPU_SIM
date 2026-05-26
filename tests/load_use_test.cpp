#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "MemoryMap.h"
#include "HexLoader.h"
#include "ExecutionMode.h"
#include "test_common.h"

#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    const char* path = (argc >= 2) ? argv[1] : "instruction_memory/instMem-load-use.txt";
    SimpleRAM dram(MemoryMap::RAM_SIZE);
    uint32_t nbytes = 0;
    if (!load_hex_text_file(path, dram, MemoryMap::HEX_PROGRAM_BASE, nbytes)) {
        std::fprintf(stderr, "load failed\n");
        return 1;
    }

    DirectMappedCache dcache(&dram, 4 * 1024, 32);
    CPU cpu;
    cpu.set_data_memory(&dcache);
    cpu.set_ram_size(MemoryMap::RAM_SIZE);
    cpu.set_use_hex_bounds(true);
    cpu.set_max_pc(static_cast<int>(nbytes));
    cpu.set_pc(0);
    cpu.set_execution_mode(ExecutionMode::Educational);

    int c = 0;
    while (c < TestCommon::DEFAULT_TEST_MAX_CYCLES) {
        ++c;
        cpu.run_pipeline_cycle(c, false);
        if (cpu.is_halted() || cpu.exited_via_syscall()) {
            break;
        }
    }

    if (!cpu.exited_via_syscall() || cpu.get_syscall_exit_code() != 42) {
        std::fprintf(stderr, "expected syscall exit 42, got %d\n", cpu.get_syscall_exit_code());
        return 2;
    }
    if (cpu.get_register_value(13) != 6) {
        std::fprintf(stderr, "x13 expected 6, got %d\n", cpu.get_register_value(13));
        return 3;
    }
    std::printf("load_use_test ok\n");
    return 0;
}
