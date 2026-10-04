// AArch32 decode / disassemble goldens
#include "isa/Aarch32Backend.h"
#include "CPU.h"

#include <cstdio>
#include <cstring>
#include <string>

static bool expect_kind(uint32_t inst, ArmOpKind kind, const char* label) {
    ArmDecoded d;
    if (!aarch32_decode(inst, d)) {
        std::fprintf(stderr, "FAIL %s: decode rejected 0x%08x\n", label, inst);
        return false;
    }
    if (d.kind != kind) {
        std::fprintf(stderr, "FAIL %s: kind mismatch\n", label);
        return false;
    }
    return true;
}

int main() {
    int fails = 0;

    // add r0, r1, #4  => cond=AL, I=1, opc=ADD, Rn=r1, Rd=r0, rot=0, imm=4
    // 0xE2810004
    if (!expect_kind(0xE2810004u, ArmOpKind::DataProc, "add imm")) fails++;

    // mov r0, #42 => 0xE3A0002A
    if (!expect_kind(0xE3A0002Au, ArmOpKind::DataProc, "mov imm")) fails++;

    // cmp r0, #0 => 0xE3500000
    {
        ArmDecoded d;
        if (!aarch32_decode(0xE3500000u, d) || d.kind != ArmOpKind::DataProc || !d.set_flags) {
            std::fprintf(stderr, "FAIL cmp\n");
            fails++;
        }
    }

    // beq .+0 (imm24=0) => 0x0A000000
    {
        ArmDecoded d;
        if (!aarch32_decode(0x0A000000u, d) || d.kind != ArmOpKind::Branch || d.cond != 0) {
            std::fprintf(stderr, "FAIL beq\n");
            fails++;
        }
    }

    // bl .+0 => 0xEB000000
    if (!expect_kind(0xEB000000u, ArmOpKind::BranchLink, "bl")) fails++;

    // bx lr => 0xE12FFF1E
    {
        ArmDecoded d;
        if (!aarch32_decode(0xE12FFF1Eu, d) || d.kind != ArmOpKind::Bx || d.rm != 14) {
            std::fprintf(stderr, "FAIL bx lr\n");
            fails++;
        }
    }

    // svc #0 => 0xEF000000
    {
        ArmDecoded d;
        if (!aarch32_decode(0xEF000000u, d) || d.kind != ArmOpKind::Svc || !d.ecall) {
            std::fprintf(stderr, "FAIL svc\n");
            fails++;
        }
    }

    // mul r3, r1, r2 => 0xE0030291 (Rd=3, Rn=0, Rs=2, Rm=1)
    if (!expect_kind(0xE0030291u, ArmOpKind::Multiply, "mul")) fails++;

    // ldr r0, [r1, #4] => 0xE5910004
    if (!expect_kind(0xE5910004u, ArmOpKind::LoadStore, "ldr")) fails++;

    // strb r0, [r1, #8] => 0xE5C10008
    if (!expect_kind(0xE5C10008u, ArmOpKind::LoadStore, "strb")) fails++;

    // push {r7, lr} approx STMDB sp! => 0xE92D4080
    if (!expect_kind(0xE92D4080u, ArmOpKind::BlockTransfer, "stmdb")) fails++;

    // Condition helpers
    ArmCpsr z{};
    z.z = true;
    if (!arm_condition_passed(0x0, z)) { // EQ
        std::fprintf(stderr, "FAIL cond EQ\n");
        fails++;
    }
    if (arm_condition_passed(0x1, z)) { // NE
        std::fprintf(stderr, "FAIL cond NE\n");
        fails++;
    }

    // apply_decode fills ID_EX
    {
        ID_EX_Register id{};
        bool ok = false;
        aarch32_apply_decode(0xEF000000u, id, ok);
        if (!ok || !id.ecall || !id.arm_is_arm) {
            std::fprintf(stderr, "FAIL apply_decode svc\n");
            fails++;
        }
    }

    std::string dis = aarch32_disassemble(0xE2810004u);
    if (dis.find("add") == std::string::npos) {
        std::fprintf(stderr, "FAIL disasm add: %s\n", dis.c_str());
        fails++;
    }

    if (fails) {
        std::fprintf(stderr, "aarch32_decode_test: %d failures\n", fails);
        return 1;
    }
    std::printf("aarch32_decode_test ok\n");
    return 0;
}
