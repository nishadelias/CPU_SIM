#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "MemoryMap.h"
#include "HexLoader.h"
#include "ExecutionMode.h"
#include "IsaKind.h"
#include "test_common.h"

#include <cstdio>
#include <cstring>
#include <string>

static bool path_looks_arm(const char* path) {
    return path && (std::strstr(path, "arm") != nullptr || std::strstr(path, "ARM") != nullptr);
}

int main(int argc, char** argv) {
    const char* path = (argc >= 2) ? argv[1] : "instruction_memory/instMem-forward.txt";
    const bool arm = path_looks_arm(path);

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
    if (arm) {
        cpu.set_isa(IsaKind::Aarch32);
    }

    int c = 0;
    while (c < TestCommon::DEFAULT_TEST_MAX_CYCLES) {
        ++c;
        cpu.run_pipeline_cycle(c, false);
        if (cpu.exited_via_syscall()) {
            break;
        }
    }

    /* RV: a1=x11; ARM hex puts result in r1. */
    const int result_reg = arm ? 1 : 11;
    if (cpu.get_register_value(result_reg) != 6) {
        std::fprintf(stderr, "%s expected 6 got %d\n", arm ? "r1" : "x11",
                     cpu.get_register_value(result_reg));
        return 2;
    }
    std::printf("forwarding_test ok\n");
    return 0;
}
