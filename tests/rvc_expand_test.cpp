// Table-driven tests for RV32C/FC compressed instruction expansion.
#include "CPU.h"
#include <cstdio>
#include <cstdlib>

struct RvcCase {
    uint16_t rvc;
    uint32_t expect;
    const char* note;
};

static int fail_count = 0;

static void check(CPU& cpu, const RvcCase& c) {
    uint32_t got = cpu.expand_compressed_instruction(c.rvc);
    if (got != c.expect) {
        std::fprintf(stderr, "FAIL %s: rvc=0x%04x expect=0x%08x got=0x%08x\n",
                     c.note, (unsigned)c.rvc, c.expect, got);
        fail_count++;
    }
}

int main() {
    CPU cpu;

    // C.OR x9, x9, x10 — valid encoding 0x9cc9 ([6:5]=10)
    const uint32_t or_x9_x9_x10 = 0x33u | (9u << 7) | (6u << 12) | (9u << 15) | (10u << 20);

    static const RvcCase gold[] = {
        {0x9002u, 0x00100073u, "C.EBREAK"},
        {0x0000u, 0x00000000u, "zero reserved"},
        {0x9cc9u, or_x9_x9_x10, "C.OR x9,x9,x10"},
    };

    for (const auto& c : gold) {
        check(cpu, c);
    }

    // C.FLW: flw fa0, 0(a1) — fd'=2, rs1'=1 (halfword from riscv-assembler conventions)
    // 0x60d8 expands to non-zero FLW
    uint32_t flw = cpu.expand_compressed_instruction(0x60d8u);
    if (flw == 0) {
        std::fprintf(stderr, "FAIL C.FLW sample: 0x60d8 expanded to 0\n");
        fail_count++;
    }
    if ((flw & 0x7f) != 0x07) {
        std::fprintf(stderr, "FAIL C.FLW sample: expected opcode 0x07, got 0x%02x\n", flw & 0x7f);
        fail_count++;
    }

    if (fail_count != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", fail_count);
        return EXIT_FAILURE;
    }
    std::printf("rvc_expand_test: all checks passed\n");
    return EXIT_SUCCESS;
}
