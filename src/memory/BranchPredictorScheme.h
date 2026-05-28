// BranchPredictorScheme.h - Branch predictor framework
#pragma once

#include <cstdint>
#include <string>

// Enum for different branch predictor types
enum class BranchPredictorType {
    AlwaysNotTaken,      // Always predict not taken
    AlwaysTaken,         // Always predict taken
    Bimodal,             // 2-bit saturating counter predictor
    GShare,              // Global history register predictor
    Tournament           // Hybrid predictor (tournament between two predictors)
};

// Result of a branch prediction
struct BranchPrediction {
    bool predicted_taken;    // Whether the branch is predicted to be taken
    uint32_t predicted_target; // Predicted target address (if taken)
    
    BranchPrediction() : predicted_taken(false), predicted_target(0) {}
    BranchPrediction(bool taken, uint32_t target) 
        : predicted_taken(taken), predicted_target(target) {}
};

// Base interface for branch predictor statistics
class BranchPredictorStatistics {
public:
    virtual ~BranchPredictorStatistics() {}
    virtual uint64_t correct_predictions() const = 0;
    virtual uint64_t incorrect_predictions() const = 0;
    virtual uint64_t total_predictions() const = 0;
    virtual double getAccuracy() const = 0;
    virtual std::string getSchemeName() const = 0;
    virtual std::string getDescription() const = 0;
};

// Base class for all branch predictors
// All branch predictor implementations should inherit from this
class BranchPredictorScheme : public BranchPredictorStatistics {
public:
    virtual ~BranchPredictorScheme() {}
    
    // Predict whether a branch will be taken
    // pc: Program counter of the branch instruction
    // target: Target address if branch is taken (computed from immediate)
    // Returns: BranchPrediction with prediction result
    virtual BranchPrediction predict(uint32_t pc, uint32_t target) = 0;
    
    // Update the predictor with the actual outcome
    // pc: Program counter of the branch instruction
    // target: Actual target address (if taken)
    // taken: Whether the branch was actually taken
    virtual void update(uint32_t pc, uint32_t target, bool taken) = 0;
    
    // Reset the predictor state (for simulation reset)
    virtual void reset() = 0;
    
    // BranchPredictorStatistics interface methods should be implemented by subclasses
};

// Factory function declaration (implemented in BranchPredictor.h)
BranchPredictorScheme* createBranchPredictor(BranchPredictorType type);

// Helper function to get branch predictor name as string
inline std::string branchPredictorTypeToString(BranchPredictorType type) {
    switch (type) {
        case BranchPredictorType::AlwaysNotTaken:
            return "Always Not Taken";
        case BranchPredictorType::AlwaysTaken:
            return "Always Taken";
        case BranchPredictorType::Bimodal:
            return "Bimodal (2-bit)";
        case BranchPredictorType::GShare:
            return "GShare";
        case BranchPredictorType::Tournament:
            return "Tournament";
        default:
            return "Unknown";
    }
}

