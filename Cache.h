// Cache.h
#pragma once
#include "MemoryIf.h"
#include <vector>
#include <cstdint>
#include <cstring>

// Simple direct-mapped cache, write-through + write-allocate.
// Line size and total size must be powers of two.
class DirectMappedCache : public MemoryDevice {
public:
    DirectMappedCache(MemoryDevice* lower, uint32_t totalSizeBytes, uint32_t lineSizeBytes)
    : lower_(lower),
      size_(totalSizeBytes),
      lineSize_(lineSizeBytes),
      numLines_(totalSizeBytes / lineSizeBytes),
      data_(totalSizeBytes, 0),
      tags_(numLines_, 0),
      valids_(numLines_, false),
      hits_(0), misses_(0) 
    {
        // basic sanity
        assert(lower_);
        assert((size_ & (size_-1)) != 1); // power-of-two check (relaxed)
        assert((lineSize_ & (lineSize_-1)) != 1);
        assert(numLines_ > 0);
    }

    MemResp load(uint32_t addr, AccessSize size) override {
        uint32_t idx, tag, lineBase;
        decode(addr, idx, tag, lineBase);

        // Hit?
        if (valids_[idx] && tags_[idx] == tag) {
            hits_++;
            uint32_t off = addr - lineBase;
            return {true, readFromLine(idx, off, size)};
        }

        // Miss: fill line from lower
        misses_++;
        if (!fillLine(idx, tag, lineBase)) return {false, 0};

        uint32_t off = addr - lineBase;
        return {true, readFromLine(idx, off, size)};
    }

    bool store(uint32_t addr, uint32_t data, AccessSize size) override {
        uint32_t idx, tag, lineBase;
        decode(addr, idx, tag, lineBase);

        // Write-allocate: ensure line is present
        if (!(valids_[idx] && tags_[idx] == tag)) {
            // Bring line
            if (!fillLine(idx, tag, lineBase)) return false;
            misses_++;
        } else {
            hits_++;
        }

        // Update cached line
        uint32_t off = addr - lineBase;
        writeToLine(idx, off, data, size);

        // Write-through to lower
        return lowerWrite(addr, data, size);
    }

    // Simple stats
    uint64_t hits()   const { return hits_; }
    uint64_t misses() const { return misses_; }

private:
    MemoryDevice* lower_;
    uint32_t size_, lineSize_, numLines_;
    std::vector<uint8_t> data_;
    std::vector<uint32_t> tags_;
    std::vector<bool>     valids_;
    uint64_t hits_, misses_;

    static uint32_t maskPow2(uint32_t x) { return x - 1; }

    void decode(uint32_t addr, uint32_t& idx, uint32_t& tag, uint32_t& lineBase) const {
        uint32_t lineMask = ~(lineSize_ - 1u);
        lineBase = addr & lineMask;
        idx = (lineBase / lineSize_) & (numLines_ - 1u);
        tag = lineBase >> __builtin_ctz(lineSize_);
    }

    bool fillLine(uint32_t idx, uint32_t tag, uint32_t lineBase) {
        // Read an entire line from lower
        for (uint32_t i = 0; i < lineSize_; i += 4) {
            uint32_t a = lineBase + i;
            auto r = lower_->load(a, AccessSize::Word);
            if (!r.ok) return false;
            SimpleRAM::unpackLE(r.data, &data_[idx * lineSize_ + i], AccessSize::Word);
        }
        tags_[idx] = tag;
        valids_[idx] = true;
        return true;
    }

    static uint32_t packFrom(const uint8_t* p, AccessSize sz) {
        return SimpleRAM::packLE(p, sz);
    }
    static void unpackTo(uint32_t v, uint8_t* p, AccessSize sz) {
        SimpleRAM::unpackLE(v, p, sz);
    }

    uint32_t readFromLine(uint32_t idx, uint32_t off, AccessSize sz) const {
        const uint8_t* base = &data_[idx * lineSize_ + off];
        return packFrom(base, sz);
    }

    void writeToLine(uint32_t idx, uint32_t off, uint32_t value, AccessSize sz) {
        uint8_t* base = &data_[idx * lineSize_ + off];
        unpackTo(value, base, sz);
    }

    bool lowerWrite(uint32_t addr, uint32_t data, AccessSize size) {
        return lower_->store(addr, data, size);
    }
};
