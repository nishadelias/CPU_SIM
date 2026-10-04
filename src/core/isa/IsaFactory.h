#pragma once
#include "isa/IsaBackend.h"
#include <memory>

std::unique_ptr<IsaBackend> create_isa_backend(IsaKind kind);
IsaKind isa_kind_from_elf_machine(uint16_t machine);
