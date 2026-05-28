// CacheScheme.h - Cache scheme framework
#pragma once

#include "MemoryIf.h"
#include <cstdint>
#include <string>

// Enum for different cache scheme types
enum class CacheSchemeType {
    DirectMapped,
    FullyAssociative,
    SetAssociative2Way,
    SetAssociative4Way,
    SetAssociative8Way
};

// Base interface for cache statistics
class CacheStatistics {
public:
    virtual ~CacheStatistics() {}
    virtual uint64_t hits() const = 0;
    virtual uint64_t misses() const = 0;
    virtual std::string getSchemeName() const = 0;
    virtual std::string getDescription() const = 0;
};

// Base class for all cache schemes
// All cache implementations should inherit from this
class CacheScheme : public MemoryDevice, public CacheStatistics {
public:
    virtual ~CacheScheme() {}
    
    // MemoryDevice interface is already pure virtual in MemoryDevice
    // CacheStatistics interface methods should be implemented by subclasses
};

// Factory function declaration (implemented in Cache.h)
CacheScheme* createCacheScheme(CacheSchemeType type, MemoryDevice* lower, 
                                uint32_t totalSizeBytes, uint32_t lineSizeBytes);

// Helper function to get cache scheme name as string
inline std::string cacheSchemeTypeToString(CacheSchemeType type) {
    switch (type) {
        case CacheSchemeType::DirectMapped:
            return "Direct Mapped";
        case CacheSchemeType::FullyAssociative:
            return "Fully Associative";
        case CacheSchemeType::SetAssociative2Way:
            return "2-Way Set Associative";
        case CacheSchemeType::SetAssociative4Way:
            return "4-Way Set Associative";
        case CacheSchemeType::SetAssociative8Way:
            return "8-Way Set Associative";
        default:
            return "Unknown";
    }
}

