/* Compute Fibonacci for 24 steps, then print all lines (long rodata blob, many ECALLs). */
#include "simlib.h"

static volatile unsigned fib_a;
static volatile unsigned fib_b;
static volatile unsigned fib_n;

__attribute__((section(".text.init")))
__attribute__((naked))
__attribute__((noreturn))
void _start(void) {
    sim_init_stack();
    SIM_PRINT("CPU_SIM: Fibonacci (24 terms)\n");

    fib_a = 0;
    fib_b = 1;
    for (fib_n = 0; fib_n < 24u; fib_n++) {
        unsigned next = fib_a + fib_b;
        fib_a = fib_b;
        fib_b = next;
    }

    SIM_PRINT("fib[0] = 0\n");
    SIM_PRINT("fib[1] = 1\n");
    SIM_PRINT("fib[2] = 1\n");
    SIM_PRINT("fib[3] = 2\n");
    SIM_PRINT("fib[4] = 3\n");
    SIM_PRINT("fib[5] = 5\n");
    SIM_PRINT("fib[6] = 8\n");
    SIM_PRINT("fib[7] = 13\n");
    SIM_PRINT("fib[8] = 21\n");
    SIM_PRINT("fib[9] = 34\n");
    SIM_PRINT("fib[10] = 55\n");
    SIM_PRINT("fib[11] = 89\n");
    SIM_PRINT("fib[12] = 144\n");
    SIM_PRINT("fib[13] = 233\n");
    SIM_PRINT("fib[14] = 377\n");
    SIM_PRINT("fib[15] = 610\n");
    SIM_PRINT("fib[16] = 987\n");
    SIM_PRINT("fib[17] = 1597\n");
    SIM_PRINT("fib[18] = 2584\n");
    SIM_PRINT("fib[19] = 4181\n");
    SIM_PRINT("fib[20] = 6765\n");
    SIM_PRINT("fib[21] = 10946\n");
    SIM_PRINT("fib[22] = 17711\n");
    SIM_PRINT("fib[23] = 28657\n");

    SIM_EXIT(0);
}
