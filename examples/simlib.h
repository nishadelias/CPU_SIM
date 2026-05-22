/* Minimal runtime for CPU_SIM — Linux RISC-V syscall ABI (no libc).
 * Helpers are always_inline. Use SIM_PRINT() for string literals (correct length).
 * Naked _start + .bss globals avoid stack save/restore bugs in the simulator. */
#pragma once

#define SIM_ALWAYS_INLINE static inline __attribute__((always_inline))

#define SIM_PRINT(s) sim_write(1, (s), (unsigned)(sizeof(s) - 1u))

SIM_ALWAYS_INLINE void sim_init_stack(void) {
    __asm__ volatile(
        "  la sp, __stack_top\n"
        "  addi sp, sp, -16\n");
}

SIM_ALWAYS_INLINE long sim_write(int fd, const void* buf, unsigned len) {
    register long a0 asm("a0") = fd;
    register long a1 asm("a1") = (long)(unsigned long)buf;
    register long a2 asm("a2") = (long)len;
    register long a7 asm("a7") = 64;
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7) : "memory");
    return a0;
}

#define SIM_EXIT(code)                                                          \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "li a7, 93\n\t"                                                     \
            "li a0, %0\n\t"                                                     \
            "ecall\n\t"                                                         \
            "1: j 1b\n"                                                         \
            :: "i"(code)                                                        \
            : "a0", "a7", "memory");                                            \
        __builtin_unreachable();                                                \
    } while (0)
