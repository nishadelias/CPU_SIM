// Cache.h - Cache implementations
#pragma once
#include "CacheScheme.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <map>
#include <list>

// Simple direct-mapped cache, write-through + write-allocate.
// Line size and total size must be powers of two.
class DirectMappedCache : public CacheScheme {
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
        assert((size_ & (size_-1)) == 0); // power-of-two check
        assert((lineSize_ & (lineSize_-1)) == 0); // power-of-two check
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

    // CacheStatistics interface
    uint64_t hits() const override { return hits_; }
    uint64_t misses() const override { return misses_; }
    std::string getSchemeName() const override { return "Direct Mapped"; }
    std::string getDescription() const override { 
        return "Direct-mapped cache with write-through and write-allocate policy";
    }

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

// Fully Associative Cache with LRU replacement
// Write-through + write-allocate
class FullyAssociativeCache : public CacheScheme {
public:
    FullyAssociativeCache(MemoryDevice* lower, uint32_t totalSizeBytes, uint32_t lineSizeBytes)
    : lower_(lower),
      size_(totalSizeBytes),
      lineSize_(lineSizeBytes),
      numLines_(totalSizeBytes / lineSizeBytes),
      data_(totalSizeBytes, 0),
      tags_(numLines_, 0),
      valids_(numLines_, false),
      hits_(0), misses_(0)
    {
        assert(lower_);
        assert((size_ & (size_-1)) == 0);
        assert((lineSize_ & (lineSize_-1)) == 0);
        assert(numLines_ > 0);
        
        // Initialize LRU list (all indices initially in order)
        for (uint32_t i = 0; i < numLines_; i++) {
            lruList_.push_back(i);
        }
    }

    MemResp load(uint32_t addr, AccessSize size) override {
        uint32_t tag, lineBase;
        decode(addr, tag, lineBase);

        // Search all lines for matching tag
        for (uint32_t i = 0; i < numLines_; i++) {
            if (valids_[i] && tags_[i] == tag) {
                // Hit - update LRU
                hits_++;
                updateLRU(i);
                uint32_t off = addr - lineBase;
                return {true, readFromLine(i, off, size)};
            }
        }

        // Miss - find LRU line
        misses_++;
        uint32_t victimIdx = getLRUVictim();
        if (!fillLine(victimIdx, tag, lineBase)) return {false, 0};

        uint32_t off = addr - lineBase;
        return {true, readFromLine(victimIdx, off, size)};
    }

    bool store(uint32_t addr, uint32_t data, AccessSize size) override {
        uint32_t tag, lineBase;
        decode(addr, tag, lineBase);

        // Search for line
        for (uint32_t i = 0; i < numLines_; i++) {
            if (valids_[i] && tags_[i] == tag) {
                // Hit - update line and LRU
                hits_++;
                updateLRU(i);
                uint32_t off = addr - lineBase;
                writeToLine(i, off, data, size);
                return lowerWrite(addr, data, size);
            }
        }

        // Miss - write-allocate
        misses_++;
        uint32_t victimIdx = getLRUVictim();
        if (!fillLine(victimIdx, tag, lineBase)) return false;

        uint32_t off = addr - lineBase;
        writeToLine(victimIdx, off, data, size);
        return lowerWrite(addr, data, size);
    }

    // CacheStatistics interface
    uint64_t hits() const override { return hits_; }
    uint64_t misses() const override { return misses_; }
    std::string getSchemeName() const override { return "Fully Associative"; }
    std::string getDescription() const override {
        return "Fully associative cache with LRU replacement, write-through and write-allocate";
    }

private:
    MemoryDevice* lower_;
    uint32_t size_, lineSize_, numLines_;
    std::vector<uint8_t> data_;
    std::vector<uint32_t> tags_;
    std::vector<bool> valids_;
    uint64_t hits_, misses_;
    
    // LRU tracking: list of indices, front is MRU, back is LRU
    std::list<uint32_t> lruList_;

    void decode(uint32_t addr, uint32_t& tag, uint32_t& lineBase) const {
        uint32_t lineMask = ~(lineSize_ - 1u);
        lineBase = addr & lineMask;
        tag = lineBase >> __builtin_ctz(lineSize_);
    }

    void updateLRU(uint32_t idx) {
        // Move idx to front (MRU position)
        lruList_.remove(idx);
        lruList_.push_front(idx);
    }

    uint32_t getLRUVictim() {
        // Return LRU (back of list)
        uint32_t victim = lruList_.back();
        lruList_.pop_back();
        lruList_.push_front(victim);  // Will be updated by updateLRU after fill
        return victim;
    }

    bool fillLine(uint32_t idx, uint32_t tag, uint32_t lineBase) {
        for (uint32_t i = 0; i < lineSize_; i += 4) {
            uint32_t a = lineBase + i;
            auto r = lower_->load(a, AccessSize::Word);
            if (!r.ok) return false;
            SimpleRAM::unpackLE(r.data, &data_[idx * lineSize_ + i], AccessSize::Word);
        }
        tags_[idx] = tag;
        valids_[idx] = true;
        updateLRU(idx);
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

// Set-Associative Cache with LRU replacement
// Write-through + write-allocate
class SetAssociativeCache : public CacheScheme {
public:
    SetAssociativeCache(MemoryDevice* lower, uint32_t totalSizeBytes, uint32_t lineSizeBytes, uint32_t associativity)
    : lower_(lower),
      size_(totalSizeBytes),
      lineSize_(lineSizeBytes),
      associativity_(associativity),
      numSets_(totalSizeBytes / (lineSizeBytes * associativity)),
      numLines_(numSets_ * associativity),
      data_(totalSizeBytes, 0),
      tags_(numLines_, 0),
      valids_(numLines_, false),
      hits_(0), misses_(0)
    {
        assert(lower_);
        assert((size_ & (size_-1)) == 0);
        assert((lineSize_ & (lineSize_-1)) == 0);
        assert(numSets_ > 0);
        assert(associativity_ > 0);
        assert((numSets_ & (numSets_-1)) == 0);  // numSets must be power of 2
        
        // Initialize LRU lists for each set
        for (uint32_t set = 0; set < numSets_; set++) {
            std::list<uint32_t> lruList;
            for (uint32_t way = 0; way < associativity_; way++) {
                lruList.push_back(way);
            }
            lruLists_.push_back(lruList);
        }
    }

    MemResp load(uint32_t addr, AccessSize size) override {
        uint32_t set, tag, lineBase;
        decode(addr, set, tag, lineBase);

        // Search ways in this set
        uint32_t setBase = set * associativity_;
        for (uint32_t way = 0; way < associativity_; way++) {
            uint32_t idx = setBase + way;
            if (valids_[idx] && tags_[idx] == tag) {
                // Hit - update LRU
                hits_++;
                updateLRU(set, way);
                uint32_t off = addr - lineBase;
                return {true, readFromLine(idx, off, size)};
            }
        }

        // Miss - find LRU way in set
        misses_++;
        uint32_t victimWay = getLRUVictim(set);
        uint32_t victimIdx = setBase + victimWay;
        if (!fillLine(victimIdx, tag, lineBase)) return {false, 0};

        uint32_t off = addr - lineBase;
        return {true, readFromLine(victimIdx, off, size)};
    }

    bool store(uint32_t addr, uint32_t data, AccessSize size) override {
        uint32_t set, tag, lineBase;
        decode(addr, set, tag, lineBase);

        // Search for line in set
        uint32_t setBase = set * associativity_;
        for (uint32_t way = 0; way < associativity_; way++) {
            uint32_t idx = setBase + way;
            if (valids_[idx] && tags_[idx] == tag) {
                // Hit - update line and LRU
                hits_++;
                updateLRU(set, way);
                uint32_t off = addr - lineBase;
                writeToLine(idx, off, data, size);
                return lowerWrite(addr, data, size);
            }
        }

        // Miss - write-allocate
        misses_++;
        uint32_t victimWay = getLRUVictim(set);
        uint32_t victimIdx = setBase + victimWay;
        if (!fillLine(victimIdx, tag, lineBase)) return false;

        uint32_t off = addr - lineBase;
        writeToLine(victimIdx, off, data, size);
        return lowerWrite(addr, data, size);
    }

    // CacheStatistics interface
    uint64_t hits() const override { return hits_; }
    uint64_t misses() const override { return misses_; }
    std::string getSchemeName() const override {
        return std::to_string(associativity_) + "-Way Set Associative";
    }
    std::string getDescription() const override {
        return std::to_string(associativity_) + "-way set-associative cache with LRU replacement, write-through and write-allocate";
    }

private:
    MemoryDevice* lower_;
    uint32_t size_, lineSize_, associativity_, numSets_, numLines_;
    std::vector<uint8_t> data_;
    std::vector<uint32_t> tags_;
    std::vector<bool> valids_;
    uint64_t hits_, misses_;
    
    // LRU tracking: one list per set
    std::vector<std::list<uint32_t>> lruLists_;

    void decode(uint32_t addr, uint32_t& set, uint32_t& tag, uint32_t& lineBase) const {
        uint32_t lineMask = ~(lineSize_ - 1u);
        lineBase = addr & lineMask;
        uint32_t setBits = __builtin_ctz(numSets_);
        set = (lineBase / lineSize_) & (numSets_ - 1u);
        tag = lineBase >> (__builtin_ctz(lineSize_) + setBits);
    }

    void updateLRU(uint32_t set, uint32_t way) {
        // Move way to front (MRU) in this set's LRU list
        lruLists_[set].remove(way);
        lruLists_[set].push_front(way);
    }

    uint32_t getLRUVictim(uint32_t set) {
        // Return LRU way (back of list) for this set
        uint32_t victim = lruLists_[set].back();
        lruLists_[set].pop_back();
        lruLists_[set].push_front(victim);  // Will be updated after fill
        return victim;
    }

    bool fillLine(uint32_t idx, uint32_t tag, uint32_t lineBase) {
        for (uint32_t i = 0; i < lineSize_; i += 4) {
            uint32_t a = lineBase + i;
            auto r = lower_->load(a, AccessSize::Word);
            if (!r.ok) return false;
            SimpleRAM::unpackLE(r.data, &data_[idx * lineSize_ + i], AccessSize::Word);
        }
        tags_[idx] = tag;
        valids_[idx] = true;
        
        // Update LRU for this line
        uint32_t set = idx / associativity_;
        uint32_t way = idx % associativity_;
        updateLRU(set, way);
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

// Factory function implementation
inline CacheScheme* createCacheScheme(CacheSchemeType type, MemoryDevice* lower, 
                                      uint32_t totalSizeBytes, uint32_t lineSizeBytes) {
    switch (type) {
        case CacheSchemeType::DirectMapped:
            return new DirectMappedCache(lower, totalSizeBytes, lineSizeBytes);
        case CacheSchemeType::FullyAssociative:
            return new FullyAssociativeCache(lower, totalSizeBytes, lineSizeBytes);
        case CacheSchemeType::SetAssociative2Way:
            return new SetAssociativeCache(lower, totalSizeBytes, lineSizeBytes, 2);
        case CacheSchemeType::SetAssociative4Way:
            return new SetAssociativeCache(lower, totalSizeBytes, lineSizeBytes, 4);
        case CacheSchemeType::SetAssociative8Way:
            return new SetAssociativeCache(lower, totalSizeBytes, lineSizeBytes, 8);
        default:
            return new DirectMappedCache(lower, totalSizeBytes, lineSizeBytes);
    }
}
