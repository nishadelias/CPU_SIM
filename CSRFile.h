#pragma once

#include "Trap.h"
#include <cstdint>

class CSRFile {
public:
    CSRFile();

    void reset();

    uint32_t fcsr() const { return fcsr_; }
    void set_fcsr(uint32_t v) { fcsr_ = v & 0xFFFF; }

    PrivMode prv() const { return prv_; }
    void set_prv(PrivMode p) { prv_ = p; }

    bool traps_enabled() const { return traps_enabled_; }
    void set_traps_enabled(bool on) { traps_enabled_ = on; }

    uint32_t read(uint32_t addr, bool& ok) const;
    bool write(uint32_t addr, uint32_t value, uint32_t mask);

    uint32_t mstatus() const { return mstatus_; }
    uint32_t mie() const { return mie_; }
    uint32_t mip() const { return mip_; }
    uint32_t mtvec() const { return mtvec_; }
    uint32_t mepc() const { return mepc_; }
    uint32_t mcause() const { return mcause_; }
    uint32_t mtval() const { return mtval_; }
    uint32_t satp() const { return satp_; }

    void set_mip_bit(uint32_t bit, bool on);
    bool global_interrupt_enabled() const;
    bool pending_enabled_interrupt() const;

    // Returns trap vector PC (after enter_trap)
    uint32_t enter_trap(uint32_t pc, uint32_t cause, uint32_t tval, PrivMode target = PrivMode::Machine);
    bool mret(uint32_t& out_pc, PrivMode& out_prv);

private:
    uint32_t fcsr_;
    PrivMode prv_;
    bool traps_enabled_;

    uint32_t mstatus_;
    uint32_t mie_;
    uint32_t mip_;
    uint32_t mtvec_;
    uint32_t mepc_;
    uint32_t mcause_;
    uint32_t mtval_;
    uint32_t mscratch_;
    uint32_t satp_;
};
