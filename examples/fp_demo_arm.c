/* Soft-float AArch32 stub — VFP/NEON are not in the educational AArch32 backend.
 * Mirrors examples/fp_demo.c (RV stub) so both ISAs have a matching build target. */
#include "simlib_arm.h"

__attribute__((naked)) void _start(void) {
    sim_init_stack();
    SIM_EXIT(0);
}
