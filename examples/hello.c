/* Minimal RV32 program: exit with code 42 (no stdout). */
#include "simlib.h"

__attribute__((section(".text.init")))
__attribute__((naked))
__attribute__((noreturn))
void _start(void) {
    sim_init_stack();
    SIM_EXIT(42);
}
