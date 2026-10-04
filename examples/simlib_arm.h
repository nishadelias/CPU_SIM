/* Minimal AArch32 runtime for CPU_SIM — Linux ARM EABI-style syscalls (no libc).
 * Number in r7, args in r0–r2, result in r0. Same educational numbers as RV simlib. */
#pragma once

#define SIM_ALWAYS_INLINE static inline __attribute__((always_inline))

#define SIM_PRINT(s) sim_write(1, (s), (unsigned)(sizeof(s) - 1u))

SIM_ALWAYS_INLINE void sim_init_stack(void) {
    __asm__ volatile(
        "  ldr sp, =__stack_top\n"
        "  sub sp, sp, #16\n");
}

SIM_ALWAYS_INLINE long sim_write(int fd, const void* buf, unsigned len) {
    register long r0 asm("r0") = fd;
    register long r1 asm("r1") = (long)(unsigned long)buf;
    register long r2 asm("r2") = (long)len;
    register long r7 asm("r7") = 64;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory");
    return r0;
}

#define SIM_EXIT(code)                                                          \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "mov r7, #93\n\t"                                                   \
            "mov r0, %0\n\t"                                                    \
            "svc #0\n\t"                                                        \
            "1: b 1b\n"                                                         \
            :: "i"(code)                                                        \
            : "r0", "r7", "memory");                                            \
        __builtin_unreachable();                                                \
    } while (0)
