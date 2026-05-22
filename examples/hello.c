/* Minimal RV32 program for CPU_SIM: exit 42 via ECALL (no call/ret stack). */
__attribute__((section(".text.init")))
__attribute__((noreturn))
void _start(void) {
    __asm__ volatile(
        "  la sp, __stack_top\n"
        "  addi sp, sp, -16\n"
        "  li a0, 42\n"
        "  li a7, 93\n"
        "  ecall\n"
        "1: j 1b\n");
    __builtin_unreachable();
}
