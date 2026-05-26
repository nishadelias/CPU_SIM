#include "CSRFile.h"

CSRFile::CSRFile() {
    reset();
}

void CSRFile::reset() {
    fcsr_ = 0;
    prv_ = PrivMode::Machine;
    traps_enabled_ = false;
    mstatus_ = 0;
    mie_ = 0;
    mip_ = 0;
    mtvec_ = 0;
    mepc_ = 0;
    mcause_ = 0;
    mtval_ = 0;
    mscratch_ = 0;
    satp_ = 0;
}

uint32_t CSRFile::read(uint32_t addr, bool& ok) const {
    ok = true;
    switch (addr) {
        case CsrAddr::Fflags:
            return fcsr_ & 0x1F;
        case CsrAddr::Frm:
            return (fcsr_ >> 5) & 0x7;
        case CsrAddr::Fcsr:
            return fcsr_;
        case CsrAddr::Mstatus:
            return mstatus_;
        case CsrAddr::Mie:
            return mie_;
        case CsrAddr::Mip:
            return mip_;
        case CsrAddr::Mtvec:
            return mtvec_;
        case CsrAddr::Mepc:
            return mepc_;
        case CsrAddr::Mcause:
            return mcause_;
        case CsrAddr::Mtval:
            return mtval_;
        case CsrAddr::Mscratch:
            return mscratch_;
        case CsrAddr::Satp:
            return satp_;
        default:
            ok = false;
            return 0;
    }
}

bool CSRFile::write(uint32_t addr, uint32_t value, uint32_t mask) {
    const uint32_t m = (mask == 0) ? 0xFFFFFFFFu : mask;
    switch (addr) {
        case CsrAddr::Fflags:
            fcsr_ = (fcsr_ & ~0x1F) | (value & m & 0x1F);
            return true;
        case CsrAddr::Frm:
            fcsr_ = (fcsr_ & ~(0x7u << 5)) | ((value & m & 0x7) << 5);
            return true;
        case CsrAddr::Fcsr:
            fcsr_ = (fcsr_ & ~m) | (value & m & 0xFFFF);
            return true;
        case CsrAddr::Mstatus:
            mstatus_ = (mstatus_ & ~m) | (value & m);
            return true;
        case CsrAddr::Mie:
            mie_ = (mie_ & ~m) | (value & m);
            return true;
        case CsrAddr::Mip:
            mip_ = (mip_ & ~m) | (value & m);
            return true;
        case CsrAddr::Mtvec:
            mtvec_ = (mtvec_ & ~m) | (value & m);
            return true;
        case CsrAddr::Mepc:
            mepc_ = (mepc_ & ~m) | (value & m);
            return true;
        case CsrAddr::Mcause:
            mcause_ = (mcause_ & ~m) | (value & m);
            return true;
        case CsrAddr::Mtval:
            mtval_ = (mtval_ & ~m) | (value & m);
            return true;
        case CsrAddr::Mscratch:
            mscratch_ = (mscratch_ & ~m) | (value & m);
            return true;
        case CsrAddr::Satp:
            satp_ = (satp_ & ~m) | (value & m);
            return true;
        default:
            return false;
    }
}

void CSRFile::set_mip_bit(uint32_t bit, bool on) {
    if (on) {
        mip_ |= bit;
    } else {
        mip_ &= ~bit;
    }
}

bool CSRFile::global_interrupt_enabled() const {
    return (mstatus_ & (1u << 3)) != 0;
}

bool CSRFile::pending_enabled_interrupt() const {
    return (mip_ & mie_) != 0;
}

uint32_t CSRFile::enter_trap(uint32_t pc, uint32_t cause, uint32_t tval, PrivMode target) {
    const unsigned mpp = static_cast<unsigned>(prv_) & 3u;
    mstatus_ = (mstatus_ & ~(3u << 11)) | (mpp << 11);
    mstatus_ &= ~(1u << 7);
    if (mstatus_ & (1u << 3)) {
        mstatus_ |= (1u << 7);
    }
    mstatus_ &= ~(1u << 3);

    mepc_ = pc;
    mcause_ = cause;
    mtval_ = tval;
    prv_ = target;

    const bool vectored = (mtvec_ & 1u) != 0;
    const uint32_t base = mtvec_ & ~1u;
    if (vectored && (cause & 0x80000000u)) {
        const uint32_t code = cause & 0xFFu;
        return base + 4u * code;
    }
    return base;
}

bool CSRFile::mret(uint32_t& out_pc, PrivMode& out_prv) {
    const unsigned mpp = (mstatus_ >> 11) & 3u;
    out_prv = (mpp == 0) ? PrivMode::User : PrivMode::Machine;
    prv_ = out_prv;

    const bool mpie = (mstatus_ & (1u << 7)) != 0;
    mstatus_ = (mstatus_ & ~(1u << 3)) | (mpie ? (1u << 3) : 0);
    mstatus_ |= (1u << 7);
    mstatus_ &= ~(3u << 11);

    out_pc = mepc_;
    return true;
}
