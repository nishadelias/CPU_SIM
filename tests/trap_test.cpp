#include "CPU.h"
#include "MemoryIf.h"
#include "Cache.h"
#include "MemoryMap.h"
#include "ExecutionMode.h"
#include "Trap.h"
#include "test_common.h"

#include <cstdio>
#include <cstring>

int main() {
    SimpleRAM dram(MemoryMap::RAM_SIZE);
    DirectMappedCache dcache(&dram, 4 * 1024, 32);
    CPU cpu;
    cpu.set_data_memory(&dcache);
    cpu.set_ram_backing(&dram);
    cpu.set_ram_size(MemoryMap::RAM_SIZE);
    cpu.set_traps_enabled(true);
    cpu.set_execution_mode(ExecutionMode::Executable);

  // Handler at 0x100: mret
  const uint32_t mret = 0x30200073;
  dram.store(0x100, mret, AccessSize::Word);
  cpu.csr_file().write(CsrAddr::Mtvec, 0x100, 0);

  // Program: illegal insn at 0
  dram.store(0, 0xFFFFFFFFu, AccessSize::Word);

  int c = 0;
  while (c < 5000) {
    ++c;
    cpu.run_pipeline_cycle(c, false);
    if (cpu.csr_file().mepc() != 0 && cpu.readPC() == 0x100) {
      std::printf("trap_test ok\n");
      return 0;
    }
  }
  std::fprintf(stderr, "trap not delivered\n");
  return 1;
}
