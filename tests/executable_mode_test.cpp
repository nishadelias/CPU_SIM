#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "MemoryMap.h"
#include "ExecutionMode.h"
#include "test_common.h"

#include <cstdio>

int main() {
    SimpleRAM dram(MemoryMap::RAM_SIZE);
    DirectMappedCache dcache(&dram, 4 * 1024, 32);
    CPU cpu;
    cpu.set_data_memory(&dcache);
    cpu.set_ram_size(MemoryMap::RAM_SIZE);
    cpu.set_execution_mode(ExecutionMode::Executable);
    cpu.set_pc(0);

    // Write illegal instruction word at 0
    dram.store(0, 0xFFFFFFFFu, AccessSize::Word);

    int c = 0;
    while (c < 1000) {
        ++c;
        cpu.run_pipeline_cycle(c, false);
        if (cpu.is_faulted() || cpu.is_halted()) {
            break;
        }
    }

    if (!cpu.is_faulted()) {
        std::fprintf(stderr, "expected fault in executable mode\n");
        return 1;
    }
    std::printf("executable_mode_test ok\n");
    return 0;
}
