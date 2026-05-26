#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "MemoryMap.h"
#include "HexLoader.h"
#include "ExecutionMode.h"
#include "test_common.h"

#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    const char* path = (argc >= 2) ? argv[1] : "instruction_memory/instMem-forward.txt";
    SimpleRAM dram(MemoryMap::RAM_SIZE);
    uint32_t nbytes = 0;
    if (!load_hex_text_file(path, dram, 0, nbytes)) {
        return 1;
    }

    DirectMappedCache dcache(&dram, 4 * 1024, 32);
    CPU cpu;
    cpu.set_data_memory(&dcache);
    cpu.set_ram_size(MemoryMap::RAM_SIZE);
    cpu.set_use_hex_bounds(true);
    cpu.set_max_pc(static_cast<int>(nbytes));

    int c = 0;
    while (c < TestCommon::DEFAULT_TEST_MAX_CYCLES) {
        ++c;
        cpu.run_pipeline_cycle(c, false);
        if (cpu.exited_via_syscall()) {
            break;
        }
    }

    if (cpu.get_register_value(11) != 6) {
        std::fprintf(stderr, "x11 expected 6 got %d\n", cpu.get_register_value(11));
        return 2;
    }
    std::printf("forwarding_test ok\n");
    return 0;
}
