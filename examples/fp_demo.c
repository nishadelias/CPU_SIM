#include "simlib.h"

__attribute__((naked)) void _start(void) {
    sim_init_stack();
    SIM_EXIT(0);
}
