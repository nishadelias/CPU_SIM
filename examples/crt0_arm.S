/* Minimal AArch32 startup: stack, call main, exit via SVC. */
.syntax unified
.arm
.section .text.init
.global _start
_start:
  ldr sp, =__stack_top
  sub sp, sp, #16
  bl main
  mov r7, #93
  svc #0
1:
  b 1b
