# Cache Schemes Framework

This document explains how to use and extend the cache schemes framework in the CPU Simulator.

## Overview

The cache schemes framework allows you to:
- Select different cache schemes from the GUI before running a program
- Compare the performance of different cache schemes
- Easily add your own custom cache schemes

## Available Cache Schemes

1. **Direct Mapped Cache** (`CacheSchemeType::DirectMapped`)
   - Each memory block maps to exactly one cache line
   - Simple and fast, but can have more conflicts
   - Write-through with write-allocate policy

2. **Fully Associative Cache** (`CacheSchemeType::FullyAssociative`)
   - Any memory block can be stored in any cache line
   - Uses LRU (Least Recently Used) replacement policy
   - Best hit rate but more complex
   - Write-through with write-allocate policy

3. **Set-Associative Caches** (`CacheSchemeType::SetAssociative2Way`, `SetAssociative4Way`, `SetAssociative8Way`)
   - Compromise between direct-mapped and fully associative
   - Memory blocks map to a set, can be in any way within that set
   - Uses LRU replacement within each set
   - Write-through with write-allocate policy

## Using Cache Schemes in the GUI

1. Open the CPU Simulator GUI
2. Before loading/running a program, select your desired cache scheme from the "Cache Configuration" dropdown
3. Load and run your program
4. The cache statistics (hits, misses, hit rate) will be displayed in the Statistics tab
5. Compare different schemes by:
   - Resetting the simulation
   - Changing the cache scheme
   - Running the same program again
   - Comparing the cache hit rates in the Statistics tab

## Adding Your Own Cache Scheme

To add a new cache scheme, follow these steps:

### Step 1: Create Your Cache Class

Create a new class that inherits from `CacheScheme` in `Cache.h`:

```cpp
class YourCustomCache : public CacheScheme {
public:
    YourCustomCache(MemoryDevice* lower, uint32_t totalSizeBytes, uint32_t lineSizeBytes)
    : lower_(lower),
      size_(totalSizeBytes),
      lineSize_(lineSizeBytes),
      // ... initialize your data structures ...
      hits_(0), misses_(0)
    {
        // Validation and initialization
        assert(lower_);
        assert((size_ & (size_-1)) == 0);  // size must be power of 2
        assert((lineSize_ & (lineSize_-1)) == 0);  // lineSize must be power of 2
    }

    MemResp load(uint32_t addr, AccessSize size) override {
        // Implement your load logic
        // Track hits_ and misses_
        // Return {true, data} on success, {false, 0} on failure
    }

    bool store(uint32_t addr, uint32_t data, AccessSize size) override {
        // Implement your store logic
        // Track hits_ and misses_
        // Return true on success, false on failure
    }

    // Required: Implement CacheStatistics interface
    uint64_t hits() const override { return hits_; }
    uint64_t misses() const override { return misses_; }
    std::string getSchemeName() const override { return "Your Cache Name"; }
    std::string getDescription() const override { 
        return "Description of your cache scheme"; 
    }

private:
    MemoryDevice* lower_;
    uint32_t size_, lineSize_;
    uint64_t hits_, misses_;
    // ... your cache data structures ...
};
```

### Step 2: Add to CacheSchemeType Enum

Add your new cache type to the enum in `CacheScheme.h`:

```cpp
enum class CacheSchemeType {
    DirectMapped,
    FullyAssociative,
    SetAssociative2Way,
    SetAssociative4Way,
    SetAssociative8Way,
    YourCustomCache,  // Add your new type here
};
```

### Step 3: Update the Factory Function

Add a case for your cache in the `createCacheScheme` function in `Cache.h`:

```cpp
inline CacheScheme* createCacheScheme(CacheSchemeType type, MemoryDevice* lower, 
                                      uint32_t totalSizeBytes, uint32_t lineSizeBytes) {
    switch (type) {
        // ... existing cases ...
        case CacheSchemeType::YourCustomCache:
            return new YourCustomCache(lower, totalSizeBytes, lineSizeBytes);
        default:
            return new DirectMappedCache(lower, totalSizeBytes, lineSizeBytes);
    }
}
```

### Step 4: Add to GUI (Optional)

To make your cache scheme selectable from the GUI, add it to `MainWindow.cpp` in the `setupUI()` function:

```cpp
cacheSchemeCombo_->addItem("Your Cache Name", 
                           static_cast<int>(CacheSchemeType::YourCustomCache));
```

Also add it to the helper function in `CacheScheme.h`:

```cpp
inline std::string cacheSchemeTypeToString(CacheSchemeType type) {
    switch (type) {
        // ... existing cases ...
        case CacheSchemeType::YourCustomCache:
            return "Your Cache Name";
        default:
            return "Unknown";
    }
}
```

## Implementation Guidelines

### Memory Device Interface

Your cache must implement the `MemoryDevice` interface:
- `MemResp load(uint32_t addr, AccessSize size)` - Read from cache
- `bool store(uint32_t addr, uint32_t data, AccessSize size)` - Write to cache

### Cache Statistics

Your cache must track:
- `hits_` - Number of cache hits
- `misses_` - Number of cache misses

And implement the `CacheStatistics` interface methods.

### Helper Functions

You may find these helper functions useful (from existing implementations):

```cpp
// Convert address to cache line
uint32_t lineBase = addr & ~(lineSize_ - 1u);  // Align to line boundary
uint32_t offset = addr - lineBase;              // Offset within line

// Pack/unpack data (little-endian)
uint32_t value = SimpleRAM::packLE(bytes, AccessSize::Word);
SimpleRAM::unpackLE(value, bytes, AccessSize::Word);

// Read/write from lower memory hierarchy
MemResp resp = lower_->load(addr, AccessSize::Word);
bool success = lower_->store(addr, data, AccessSize::Word);
```

### Common Policies

Current implementations use:
- **Write Policy**: Write-through (writes go to cache and lower memory)
- **Allocation Policy**: Write-allocate (on write miss, bring line into cache)
- **Replacement Policy**: 
  - Direct-mapped: No choice (only one line per set)
  - Fully associative: LRU (Least Recently Used)
  - Set-associative: LRU within each set

You can implement different policies (write-back, write-no-allocate, FIFO, random, etc.) in your custom cache.

## Testing Your Cache

1. Compile the project
2. Run the GUI
3. Select your cache scheme
4. Load a test program
5. Run the simulation
6. Check the Statistics tab for cache hit rate
7. Compare with other cache schemes on the same program

## Example: Adding a FIFO Cache

Here's a simplified example of adding a FIFO (First-In-First-Out) fully associative cache:

```cpp
class FIFOCache : public CacheScheme {
    // ... similar to FullyAssociativeCache but use a queue for FIFO instead of list for LRU ...
    std::queue<uint32_t> fifoQueue_;  // Instead of LRU list
};
```

The main difference would be in replacement:
- FIFO: Replace the oldest line (front of queue)
- LRU: Replace the least recently used line (back of list, after moving accessed line to front)

## Architecture

```
CPU
  └─> CacheScheme (your cache)
        └─> MemoryDevice (lower level, typically SimpleRAM)
```

The cache sits between the CPU and main memory, intercepting all memory accesses. All cache schemes implement the same interface, making them interchangeable.

## Notes

- All cache sizes and line sizes must be powers of 2
- The framework uses polymorphic interfaces, so adding new schemes doesn't require changing the CPU code
- Cache statistics are automatically collected and displayed in the GUI
- The cache is reset when you reset the simulation

