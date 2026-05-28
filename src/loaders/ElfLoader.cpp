#include "ElfLoader.h"
#include "MemoryMap.h"

#include <cstring>
#include <fstream>
#include <vector>
#include <algorithm>

namespace {

constexpr uint32_t PT_LOAD = 1;
constexpr uint16_t EM_RISCV = 0xF3;

struct Elf32_Ehdr {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf32_Phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

}  // namespace

ElfLoadResult load_elf32_into_ram(const std::string& path, SimpleRAM& ram) {
    ElfLoadResult r;
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        r.error = "cannot open file";
        return r;
    }

    Elf32_Ehdr eh{};
    f.read(reinterpret_cast<char*>(&eh), sizeof(eh));
    if (!f || f.gcount() != static_cast<std::streamsize>(sizeof(eh))) {
        r.error = "truncated ELF header";
        return r;
    }

    if (eh.e_ident[0] != 0x7f || eh.e_ident[1] != 'E' || eh.e_ident[2] != 'L' || eh.e_ident[3] != 'F') {
        r.error = "not an ELF file";
        return r;
    }
    if (eh.e_ident[4] != 1) {  // ELFCLASS32
        r.error = "expected ELFCLASS32";
        return r;
    }
    if (eh.e_machine != EM_RISCV) {
        r.error = "expected EM_RISCV";
        return r;
    }
    if (eh.e_phnum == 0 || eh.e_phentsize < sizeof(Elf32_Phdr)) {
        r.error = "no program headers";
        return r;
    }

    uint32_t max_end = 0;

    for (uint16_t i = 0; i < eh.e_phnum; ++i) {
        Elf32_Phdr ph{};
        f.seekg(static_cast<std::streamoff>(eh.e_phoff) +
                static_cast<std::streamoff>(i) * static_cast<std::streamoff>(eh.e_phentsize));
        f.read(reinterpret_cast<char*>(&ph), sizeof(ph));
        if (!f || f.gcount() != static_cast<std::streamsize>(sizeof(ph))) {
            r.error = "failed to read program header";
            return r;
        }
        if (ph.p_type != PT_LOAD) {
            continue;
        }

        if (ph.p_vaddr > MemoryMap::RAM_SIZE || ph.p_memsz > MemoryMap::RAM_SIZE - ph.p_vaddr) {
            r.error = "PT_LOAD does not fit in RAM";
            return r;
        }

        if (ph.p_filesz > 0) {
            std::vector<uint8_t> buf(ph.p_filesz);
            f.clear();
            f.seekg(static_cast<std::streamoff>(ph.p_offset));
            f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(ph.p_filesz));
            if (!f || static_cast<uint32_t>(f.gcount()) != ph.p_filesz) {
                r.error = "failed to read segment data";
                return r;
            }
            if (!ram.poke_bytes(ph.p_vaddr, buf.data(), ph.p_filesz)) {
                r.error = "poke_bytes failed";
                return r;
            }
        }

        if (ph.p_memsz > ph.p_filesz) {
            std::vector<uint8_t> zero(ph.p_memsz - ph.p_filesz, 0);
            if (!ram.poke_bytes(ph.p_vaddr + ph.p_filesz, zero.data(), zero.size())) {
                r.error = "bss poke failed";
                return r;
            }
        }

        max_end = std::max(max_end, ph.p_vaddr + ph.p_memsz);
    }

    r.entry = eh.e_entry;
    r.heap_brk = (max_end + 7u) & ~7u;  // 8-byte align brk
    if (r.heap_brk > MemoryMap::RAM_SIZE) {
        r.heap_brk = MemoryMap::RAM_SIZE;
    }
    r.ok = true;
    return r;
}
