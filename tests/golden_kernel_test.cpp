// Compare simulator end state against expected values for a known hex kernel.
#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "MemoryMap.h"
#include "HexLoader.h"
#include "ExecutionMode.h"

#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: golden_kernel_test <program.hex>\n");
        return 1;
    }

    const std::string hex_path = argv[1];

    SimpleRAM dram(MemoryMap::RAM_SIZE);
    uint32_t nbytes = 0;
    if (!load_hex_text_file(hex_path, dram, MemoryMap::HEX_PROGRAM_BASE, nbytes)) {
        std::fprintf(stderr, "failed to load %s\n", hex_path.c_str());
        return 2;
    }

    DirectMappedCache dcache(&dram, 4 * 1024, 32);
    CPU cpu;
    cpu.set_data_memory(&dcache);
    cpu.set_ram_size(MemoryMap::RAM_SIZE);
    cpu.set_use_hex_bounds(true);
    cpu.set_max_pc(static_cast<int>(nbytes));
    cpu.set_pc(MemoryMap::HEX_PROGRAM_BASE);
    cpu.set_execution_mode(ExecutionMode::Educational);

    const int max_cycles = 50000;
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
        if (cpu.is_pipeline_empty() && cpu.readPC() >= static_cast<unsigned long>(nbytes)) {
            break;
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

    std::printf("golden_kernel_test ok (cycles=%d)\n", c);
    return 0;
}
