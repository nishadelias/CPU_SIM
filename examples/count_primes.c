/* Count primes up to 400 via trial division (register-only, no RAM stores).
 * Heavy nested loops — good for branches, multiply, and long GUI runs. */
#include "simlib.h"

#define LIMIT 400u
#define EXPECTED 78u /* primes in [2, 400] */

__attribute__((section(".text.init")))
__attribute__((naked))
__attribute__((noreturn))
void _start(void) {
    register unsigned n asm("t0");
    register unsigned d asm("t1");
    register unsigned count asm("t2") = 0;
    register unsigned is_prime asm("t3");
    register unsigned prod asm("t4");

    sim_init_stack();
    SIM_PRINT("CPU_SIM: count primes 2..400\n");

    for (n = 2; n <= LIMIT; n++) {
        is_prime = 1;
        for (d = 2; d * d <= n; d++) {
            prod = d * d;
            if (prod > n) {
                break;
            }
            if ((n % d) == 0) {
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
