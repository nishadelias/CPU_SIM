// BranchPredictor.h - Branch predictor implementations
#pragma once

#include "BranchPredictorScheme.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <map>

// ============================================================================
// Always Not Taken Predictor
// ============================================================================
// Simplest predictor: always predicts branches will not be taken
class AlwaysNotTakenPredictor : public BranchPredictorScheme {
public:
    AlwaysNotTakenPredictor() 
        : correct_(0), incorrect_(0) {}

    BranchPrediction predict(uint32_t pc, uint32_t target) override {
        // Always predict not taken
        return BranchPrediction(false, pc + 4);
    }

    void update(uint32_t pc, uint32_t target, bool taken) override {
        if (!taken) {
            correct_++;
        } else {
            incorrect_++;
        }
    }

    void reset() override {
        correct_ = 0;
        incorrect_ = 0;
    }

    // BranchPredictorStatistics interface
    uint64_t correct_predictions() const override { return correct_; }
    uint64_t incorrect_predictions() const override { return incorrect_; }
    uint64_t total_predictions() const override { return correct_ + incorrect_; }
    double getAccuracy() const override {
        uint64_t total = total_predictions();
        if (total == 0) return 0.0;
        return (static_cast<double>(correct_) / static_cast<double>(total)) * 100.0;
    }
    std::string getSchemeName() const override { return "Always Not Taken"; }
    std::string getDescription() const override {
        return "Always predicts branches will not be taken";
    }

private:
    uint64_t correct_;
    uint64_t incorrect_;
};

// ============================================================================
// Always Taken Predictor
// ============================================================================
// Simple predictor: always predicts branches will be taken
class AlwaysTakenPredictor : public BranchPredictorScheme {
public:
    AlwaysTakenPredictor() 
        : correct_(0), incorrect_(0) {}

    BranchPrediction predict(uint32_t pc, uint32_t target) override {
        // Always predict taken
        return BranchPrediction(true, target);
    }

    void update(uint32_t pc, uint32_t target, bool taken) override {
        if (taken) {
            correct_++;
        } else {
            incorrect_++;
        }
    }

    void reset() override {
        correct_ = 0;
        incorrect_ = 0;
    }

    // BranchPredictorStatistics interface
    uint64_t correct_predictions() const override { return correct_; }
    uint64_t incorrect_predictions() const override { return incorrect_; }
    uint64_t total_predictions() const override { return correct_ + incorrect_; }
    double getAccuracy() const override {
        uint64_t total = total_predictions();
        if (total == 0) return 0.0;
        return (static_cast<double>(correct_) / static_cast<double>(total)) * 100.0;
    }
    std::string getSchemeName() const override { return "Always Taken"; }
    std::string getDescription() const override {
        return "Always predicts branches will be taken";
    }

private:
    uint64_t correct_;
    uint64_t incorrect_;
};

// ============================================================================
// Bimodal (2-bit Saturating Counter) Predictor
// ============================================================================
// Uses a table of 2-bit saturating counters indexed by PC
// States: 00 (Strongly Not Taken), 01 (Weakly Not Taken), 
//         10 (Weakly Taken), 11 (Strongly Taken)
class BimodalPredictor : public BranchPredictorScheme {
public:
    BimodalPredictor(uint32_t table_size = 2048) 
        : table_size_(table_size), correct_(0), incorrect_(0) {
        // Initialize all counters to "Weakly Not Taken" (01)
        counters_.resize(table_size_, 1);
    }

    BranchPrediction predict(uint32_t pc, uint32_t target) override {
        uint32_t index = hashPC(pc);
        uint8_t counter = counters_[index];
        
        // Predict taken if counter >= 2 (Weakly Taken or Strongly Taken)
        bool predicted_taken = (counter >= 2);
        uint32_t predicted_target = predicted_taken ? target : (pc + 4);
        
        return BranchPrediction(predicted_taken, predicted_target);
    }

    void update(uint32_t pc, uint32_t target, bool taken) override {
        uint32_t index = hashPC(pc);
        uint8_t& counter = counters_[index];
        
        // Check if prediction was correct (before updating counter)
        bool predicted_taken = (counter >= 2);
        if (predicted_taken == taken) {
            correct_++;
        } else {
            incorrect_++;
        }
        
        // Update counter based on actual outcome
        if (taken) {
            // Increment (saturate at 3)
            if (counter < 3) counter++;
        } else {
            // Decrement (saturate at 0)
            if (counter > 0) counter--;
        }
    }

    void reset() override {
        // Reset all counters to "Weakly Not Taken" (01)
        std::fill(counters_.begin(), counters_.end(), 1);
        correct_ = 0;
        incorrect_ = 0;
    }

    // BranchPredictorStatistics interface
    uint64_t correct_predictions() const override { return correct_; }
    uint64_t incorrect_predictions() const override { return incorrect_; }
    uint64_t total_predictions() const override { return correct_ + incorrect_; }
    double getAccuracy() const override {
        uint64_t total = total_predictions();
        if (total == 0) return 0.0;
        return (static_cast<double>(correct_) / static_cast<double>(total)) * 100.0;
    }
    std::string getSchemeName() const override { return "Bimodal (2-bit)"; }
    std::string getDescription() const override {
        return "2-bit saturating counter predictor with " + 
               std::to_string(table_size_) + " entries";
    }

private:
    uint32_t table_size_;
    std::vector<uint8_t> counters_;  // 2-bit counters (0-3)
    uint64_t correct_;
    uint64_t incorrect_;

    uint32_t hashPC(uint32_t pc) const {
        // Simple hash: use lower bits of PC
        return (pc >> 2) & (table_size_ - 1);
    }
};

// ============================================================================
// GShare Predictor
// ============================================================================
// Uses global history register (GHR) XORed with PC to index prediction table
class GSharePredictor : public BranchPredictorScheme {
public:
    GSharePredictor(uint32_t table_size = 2048, uint32_t history_bits = 12) 
        : table_size_(table_size), history_bits_(history_bits), 
          ghr_(0), correct_(0), incorrect_(0) {
        // Initialize all counters to "Weakly Not Taken" (01)
        counters_.resize(table_size_, 1);
        // Mask for history register
        history_mask_ = (1U << history_bits_) - 1;
    }

    BranchPrediction predict(uint32_t pc, uint32_t target) override {
        uint32_t index = hashPC(pc);
        uint8_t counter = counters_[index];
        
        // Predict taken if counter >= 2
        bool predicted_taken = (counter >= 2);
        uint32_t predicted_target = predicted_taken ? target : (pc + 4);
        
        return BranchPrediction(predicted_taken, predicted_target);
    }

    void update(uint32_t pc, uint32_t target, bool taken) override {
        uint32_t index = hashPC(pc);
        uint8_t& counter = counters_[index];
        
        // Check if prediction was correct (before updating counter)
        bool predicted_taken = (counter >= 2);
        if (predicted_taken == taken) {
            correct_++;
        } else {
            incorrect_++;
        }
        
        // Update counter
        if (taken) {
            if (counter < 3) counter++;
        } else {
            if (counter > 0) counter--;
        }
        
        // Update global history register
        ghr_ = ((ghr_ << 1) | (taken ? 1 : 0)) & history_mask_;
    }

    void reset() override {
        std::fill(counters_.begin(), counters_.end(), 1);
        ghr_ = 0;
        correct_ = 0;
        incorrect_ = 0;
    }

    // BranchPredictorStatistics interface
    uint64_t correct_predictions() const override { return correct_; }
    uint64_t incorrect_predictions() const override { return incorrect_; }
    uint64_t total_predictions() const override { return correct_ + incorrect_; }
    double getAccuracy() const override {
        uint64_t total = total_predictions();
        if (total == 0) return 0.0;
        return (static_cast<double>(correct_) / static_cast<double>(total)) * 100.0;
    }
    std::string getSchemeName() const override { return "GShare"; }
    std::string getDescription() const override {
        return "GShare predictor with " + std::to_string(table_size_) + 
               " entries and " + std::to_string(history_bits_) + "-bit global history";
    }

private:
    uint32_t table_size_;
    uint32_t history_bits_;
    uint32_t history_mask_;
    uint32_t ghr_;  // Global History Register
    std::vector<uint8_t> counters_;
    uint64_t correct_;
    uint64_t incorrect_;

    uint32_t hashPC(uint32_t pc) const {
        // XOR PC with global history register
        uint32_t pc_bits = (pc >> 2) & (table_size_ - 1);
        return (pc_bits ^ ghr_) & (table_size_ - 1);
    }
};

// ============================================================================
// Tournament Predictor
// ============================================================================
// Hybrid predictor that chooses between two predictors (Bimodal and GShare)
// Uses a selector table to decide which predictor to use
class TournamentPredictor : public BranchPredictorScheme {
public:
    TournamentPredictor(uint32_t table_size = 2048, uint32_t history_bits = 12) 
        : table_size_(table_size),
          bimodal_(table_size), gshare_(table_size, history_bits),
          correct_(0), incorrect_(0) {
        // Initialize selector table: 0-1 prefer bimodal, 2-3 prefer gshare
        // Start with "Weakly Prefer Bimodal" (1)
        selectors_.resize(table_size, 1);
    }

    BranchPrediction predict(uint32_t pc, uint32_t target) override {
        uint32_t index = hashPC(pc);
        uint8_t selector = selectors_[index];
        
        // Get predictions from both predictors
        BranchPrediction bimodal_pred = bimodal_.predict(pc, target);
        BranchPrediction gshare_pred = gshare_.predict(pc, target);
        
        // Choose based on selector: 0-1 prefer bimodal, 2-3 prefer gshare
        bool use_gshare = (selector >= 2);
        BranchPrediction final_pred = use_gshare ? gshare_pred : bimodal_pred;
        
        return final_pred;
    }

    void update(uint32_t pc, uint32_t target, bool taken) override {
        uint32_t index = hashPC(pc);
        
        // Get predictions from both predictors (before updating)
        BranchPrediction bimodal_pred = bimodal_.predict(pc, target);
        BranchPrediction gshare_pred = gshare_.predict(pc, target);
        
        // Determine which prediction was used
        uint8_t selector = selectors_[index];
        bool use_gshare = (selector >= 2);
        BranchPrediction used_pred = use_gshare ? gshare_pred : bimodal_pred;
        
        // Update both predictors
        bimodal_.update(pc, target, taken);
        gshare_.update(pc, target, taken);
        
        // Update selector based on which predictor was correct
        uint8_t& sel = selectors_[index];
        bool bimodal_correct = (bimodal_pred.predicted_taken == taken);
        bool gshare_correct = (gshare_pred.predicted_taken == taken);
        
        if (bimodal_correct && !gshare_correct) {
            // Bimodal was correct, gshare was wrong: prefer bimodal
            if (sel > 0) sel--;
        } else if (!bimodal_correct && gshare_correct) {
            // GShare was correct, bimodal was wrong: prefer gshare
            if (sel < 3) sel++;
        }
        
        // Track accuracy of the prediction we actually used
        if (used_pred.predicted_taken == taken) {
            correct_++;
        } else {
            incorrect_++;
        }
    }

    void reset() override {
        std::fill(selectors_.begin(), selectors_.end(), 1);
        bimodal_.reset();
        gshare_.reset();
        correct_ = 0;
        incorrect_ = 0;
    }

    // BranchPredictorStatistics interface
    uint64_t correct_predictions() const override { return correct_; }
    uint64_t incorrect_predictions() const override { return incorrect_; }
    uint64_t total_predictions() const override { return correct_ + incorrect_; }
    double getAccuracy() const override {
        uint64_t total = total_predictions();
        if (total == 0) return 0.0;
        return (static_cast<double>(correct_) / static_cast<double>(total)) * 100.0;
    }
    std::string getSchemeName() const override { return "Tournament"; }
    std::string getDescription() const override {
        return "Tournament predictor combining Bimodal and GShare with " + 
               std::to_string(table_size_) + " entries";
    }

private:
    uint32_t table_size_;
    BimodalPredictor bimodal_;
    GSharePredictor gshare_;
    std::vector<uint8_t> selectors_;  // 2-bit selectors (0-3)
    uint64_t correct_;
    uint64_t incorrect_;

    uint32_t hashPC(uint32_t pc) const {
        return (pc >> 2) & (table_size_ - 1);
    }
};

// ============================================================================
// Factory function implementation
// ============================================================================
inline BranchPredictorScheme* createBranchPredictor(BranchPredictorType type) {
    switch (type) {
        case BranchPredictorType::AlwaysNotTaken:
            return new AlwaysNotTakenPredictor();
        case BranchPredictorType::AlwaysTaken:
            return new AlwaysTakenPredictor();
        case BranchPredictorType::Bimodal:
            return new BimodalPredictor(2048);  // 2K entries
        case BranchPredictorType::GShare:
            return new GSharePredictor(2048, 12);  // 2K entries, 12-bit history
        case BranchPredictorType::Tournament:
            return new TournamentPredictor(2048, 12);  // 2K entries, 12-bit history
        default:
            return new AlwaysNotTakenPredictor();
    }
}

