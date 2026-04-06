/* Minimal startup: stack, call main, exit via ECALL. */
.section .text.init
.global _start
_start:
  la sp, __stack_top
  addi sp, sp, -16
  call main
  li a7, 93
  ecall
