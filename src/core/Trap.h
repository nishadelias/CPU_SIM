#pragma once

#include <cstdint>

enum class PrivMode : uint8_t {
    User = 0,
    Machine = 3,
};

// RISC-V exception codes (mcause, interrupt=0)
namespace ExceptionCode {
constexpr uint32_t InstMisaligned = 0;
constexpr uint32_t InstAccessFault = 1;
constexpr uint32_t IllegalInsn = 2;
constexpr uint32_t Breakpoint = 3;
constexpr uint32_t LoadMisaligned = 4;
constexpr uint32_t LoadAccessFault = 5;
constexpr uint32_t StoreMisaligned = 6;
constexpr uint32_t StoreAccessFault = 7;
constexpr uint32_t EcallFromUMode = 8;
constexpr uint32_t EcallFromMMode = 11;
constexpr uint32_t InstPageFault = 12;
constexpr uint32_t LoadPageFault = 13;
constexpr uint32_t StorePageFault = 15;

constexpr uint32_t TimerInterrupt = 0x80000007;  // machine timer interrupt
}  // namespace ExceptionCode

namespace CsrAddr {
constexpr uint32_t Fflags = 0x001;
constexpr uint32_t Frm = 0x002;
constexpr uint32_t Fcsr = 0x003;
constexpr uint32_t Mstatus = 0x300;
constexpr uint32_t Mie = 0x304;
constexpr uint32_t Mtvec = 0x305;
constexpr uint32_t Mscratch = 0x340;
constexpr uint32_t Mepc = 0x341;
constexpr uint32_t Mcause = 0x342;
constexpr uint32_t Mtval = 0x343;
constexpr uint32_t Mip = 0x344;
constexpr uint32_t Satp = 0x180;
}  // namespace CsrAddr
