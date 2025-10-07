// MemoryIf.h
#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <cassert>

enum AccessSize { Byte = 1, Half = 2, Word = 4 };


struct MemResp {
    bool ok;
    uint32_t data; // valid only for loads
};

class MemoryDevice {
public:
    virtual ~MemoryDevice() {}
    virtual MemResp load(uint32_t addr, AccessSize size) = 0;
    virtual bool   store(uint32_t addr, uint32_t data, AccessSize size) = 0;
};

class SimpleRAM : public MemoryDevice {
public:
    explicit SimpleRAM(size_t bytes) : mem_(bytes, 0) {}

    // Little-endian helper
    static uint32_t packLE(const uint8_t* p, AccessSize sz) {
        switch (sz) {
            case AccessSize::Byte: return p[0];
            case AccessSize::Half: return uint32_t(p[0]) | (uint32_t(p[1]) << 8);
            case AccessSize::Word: return uint32_t(p[0]) | (uint32_t(p[1]) << 8)
                                   | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
        }
        return 0;
    }
    static void unpackLE(uint32_t v, uint8_t* p, AccessSize sz) {
        p[0] = uint8_t(v & 0xFF);
        if (sz == AccessSize::Half || sz == AccessSize::Word) p[1] = uint8_t((v >> 8) & 0xFF);
        if (sz == AccessSize::Word) {
            p[2] = uint8_t((v >> 16) & 0xFF);
            p[3] = uint8_t((v >> 24) & 0xFF);
        }
    }

    MemResp load(uint32_t addr, AccessSize size) override {
        size_t need = static_cast<size_t>(size);
        if (addr + need > mem_.size()) {
            MemResp resp; resp.ok = false; resp.data = 0; return resp;
        }
        uint32_t val = packLE(&mem_[addr], size);
        MemResp resp; resp.ok = true; resp.data = val; return resp;
    }

    bool store(uint32_t addr, uint32_t data, AccessSize size) override {
        size_t need = static_cast<size_t>(size);
        if (addr + need > mem_.size()) return false;
        unpackLE(data, &mem_[addr], size);
        return true;
    }

    // Optional helper to pre-load memory
    bool poke_bytes(uint32_t addr, const void* src, size_t n) {
        if (addr + n > mem_.size()) return false;
        std::memcpy(&mem_[addr], src, n);
        return true;
    }

    uint8_t* data() { return mem_.data(); }
    size_t size() const { return mem_.size(); }

private:
    std::vector<uint8_t> mem_;
};
