#include "CPU.h"
#include <cstdio>
#include <cstring>

static bool decode_fp(CPU& cpu, uint32_t word, int& fpOp) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%08x", word);
    bool rw = false, as = false, br = false, mr = false, mw = false, mt = false, ui = false;
    int aluOp = 0;
    unsigned opcode = 0, rd = 0, f3 = 0, rs1 = 0, rs2 = 0, f7 = 0;
    const bool ok = cpu.decode_instruction(buf, &rw, &as, &br, &mr, &mw, &mt, &ui, &aluOp,
                                           &opcode, &rd, &f3, &rs1, &rs2, &f7, false);
    if (!ok) {
        return false;
    }
    fpOp = cpu.get_statistics().total_instructions;  // not stored on cpu publicly
    (void)fpOp;
    return opcode == 0x53;
}

int main() {
    CPU cpu;
    // FADD.S: opcode=0x53, f7=0, f3=0
    const uint32_t fadd = 0x00000053;  // minimal pattern - use proper encoding
    // fadd.s f0, f1, f2 => 0x00200053 style
    const uint32_t fadd_enc = 0x00208053;  // rd=1 rs1=1 rs2=0 f7=0

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%08x", fadd_enc);
    bool rw = false, as = false, br = false, mr = false, mw = false, mt = false, ui = false;
    int aluOp = 0;
    unsigned opcode = 0, rd = 0, f3 = 0, rs1 = 0, rs2 = 0, f7 = 0;
    cpu.set_execution_mode(ExecutionMode::Executable);
    if (!cpu.decode_instruction(buf, &rw, &as, &br, &mr, &mw, &mt, &ui, &aluOp,
                                &opcode, &rd, &f3, &rs1, &rs2, &f7, false)) {
        std::fprintf(stderr, "FADD decode failed\n");
        return 1;
    }
    if (opcode != 0x53) {
        std::fprintf(stderr, "bad opcode\n");
        return 2;
    }
    (void)fadd;
    std::printf("fpu_decode_test ok\n");
    return 0;
}
