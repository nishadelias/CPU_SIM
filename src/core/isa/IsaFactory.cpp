#include "isa/IsaFactory.h"
#include "isa/Riscv32Backend.h"
#include "isa/Aarch32Backend.h"

std::unique_ptr<IsaBackend> create_isa_backend(IsaKind kind) {
  switch (kind) {
    case IsaKind::Aarch32:
      return std::make_unique<Aarch32Backend>();
    case IsaKind::Riscv32:
    default:
      return std::make_unique<Riscv32Backend>();
  }
}

IsaKind isa_kind_from_elf_machine(uint16_t machine) {
  if (machine == EM_ARM) {
    return IsaKind::Aarch32;
  }
  return IsaKind::Riscv32;
}
