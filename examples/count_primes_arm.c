/* Count primes up to 400 via trial division (register-only, no RAM stores).
 * Heavy nested loops — good for branches, multiply, and long GUI runs.
 * Mirrors examples/count_primes.c for AArch32.
 *
 * Remainder is inlined (restoring bit-shift) so the binary stays pure A32 and
 * never calls out to libgcc Thumb helpers or a stack-using urem(). */
#include "simlib_arm.h"

#define LIMIT 400u
#define EXPECTED 78u /* primes in [2, 400] */

__attribute__((section(".text.init")))
__attribute__((naked))
__attribute__((noreturn))
void _start(void) {
    /* Avoid r0–r2 and r7 — used by simlib_arm.h syscalls. */
    register unsigned n asm("r4");
    register unsigned d asm("r5");
    register unsigned count asm("r6") = 0;
    register unsigned is_prime asm("r8");
    register unsigned prod asm("r9");
    register unsigned rem asm("r10");
    register unsigned bit asm("r3");

    sim_init_stack();
    SIM_PRINT("CPU_SIM: count primes 2..400\n");

    for (n = 2; n <= LIMIT; n++) {
        is_prime = 1;
        for (d = 2; d * d <= n; d++) {
            prod = d * d;
            if (prod > n) {
                break;
            }
            /* Inlined unsigned remainder (no function call / no libgcc). */
            rem = 0;
            for (bit = 1u << 31; bit != 0u; bit >>= 1) {
                rem <<= 1;
                if (n & bit) {
                    rem |= 1u;
                }
                if (rem >= d) {
                    rem -= d;
                }
            }
            if (rem == 0) {
                is_prime = 0;
                break;
            }
        }
        if (is_prime) {
            count++;
        }
    }

    if (count != EXPECTED) {
        SIM_EXIT(1);
    }
    SIM_PRINT("primes found = 78\n");
    SIM_EXIT(0);
}
