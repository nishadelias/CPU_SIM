# Branch Predictors Framework

This document explains how to use and extend the branch predictors framework in the CPU Simulator.

## Overview

The branch predictors framework allows you to:
- Select different branch predictors from the GUI before running a program
- Compare the accuracy of different branch predictors
- Easily add your own custom branch predictors

## Available Branch Predictors

1. **Always Not Taken** (`BranchPredictorType::AlwaysNotTaken`)
   - Always predicts that branches will not be taken
   - Simplest predictor with no state
   - Good baseline for comparison
   - Typically achieves 50-60% accuracy depending on branch behavior

2. **Always Taken** (`BranchPredictorType::AlwaysTaken`)
   - Always predicts that branches will be taken
   - Simple predictor with no state
   - Good for programs with many taken branches
   - Typically achieves 50-60% accuracy depending on branch behavior

3. **Bimodal (2-bit Saturating Counter)** (`BranchPredictorType::Bimodal`)
   - Uses a table of 2-bit saturating counters indexed by PC
   - States: Strongly Not Taken (00), Weakly Not Taken (01), Weakly Taken (10), Strongly Taken (11)
   - Default: 2048-entry prediction table
   - Good balance between accuracy and hardware cost
   - Typically achieves 80-90% accuracy

4. **GShare** (`BranchPredictorType::GShare`)
   - Uses global history register (GHR) XORed with PC to index prediction table
   - Default: 2048-entry table with 12-bit global history
   - Captures correlation between branches
   - Better accuracy than Bimodal for programs with correlated branches
   - Typically achieves 85-95% accuracy

5. **Tournament** (`BranchPredictorType::Tournament`)
   - Hybrid predictor combining Bimodal and GShare
   - Uses a selector table to choose between the two predictors
   - Adapts to program behavior (uses Bimodal for some branches, GShare for others)
   - Best overall accuracy
   - Typically achieves 90-98% accuracy

## Using Branch Predictors in the GUI

1. Open the CPU Simulator GUI
2. Before loading/running a program, select your desired branch predictor from the "Branch Predictor Configuration" dropdown
3. Load and run your program
4. The branch predictor statistics (accuracy, correct/incorrect predictions) will be displayed in the Statistics tab under "Performance Metrics"
5. Compare different predictors by:
   - Resetting the simulation
   - Changing the branch predictor
   - Running the same program again
   - Comparing the prediction accuracy in the Statistics tab

## How Branch Prediction Works

### Prediction Flow

1. **ID Stage (Decode)**: When a conditional branch instruction is decoded:
   - The predictor is queried with the branch PC and target address
   - If predicted taken, the PC is updated to the target address
   - The instruction in IF/ID is flushed (marked invalid)

2. **EX Stage (Execute)**: When the branch is resolved:
   - The actual outcome (taken/not taken) is determined
   - The predictor is updated with the actual outcome
   - If the prediction was incorrect (misprediction), the pipeline is flushed and PC is corrected

### Misprediction Handling

- **Correctly Predicted Taken**: PC already set in ID stage, pipeline continues normally
- **Correctly Predicted Not Taken**: Pipeline continues sequentially
- **Mispredicted**: Pipeline is flushed and PC is corrected, causing a performance penalty

## Adding Your Own Branch Predictor

To add a new branch predictor, follow these steps:

### Step 1: Create Your Predictor Class

Create a new class that inherits from `BranchPredictorScheme` in `BranchPredictor.h`:

```cpp
class YourCustomPredictor : public BranchPredictorScheme {
public:
    YourCustomPredictor(uint32_t table_size = 2048) 
        : table_size_(table_size), correct_(0), incorrect_(0) {
        // Initialize your data structures
        // Example: initialize prediction table
        predictions_.resize(table_size, 0);
    }

    BranchPrediction predict(uint32_t pc, uint32_t target) override {
        // Implement your prediction logic
        // Return BranchPrediction(predicted_taken, predicted_target)
        uint32_t index = hashPC(pc);
        bool predicted_taken = (predictions_[index] >= threshold);
        uint32_t predicted_target = predicted_taken ? target : (pc + 4);
        return BranchPrediction(predicted_taken, predicted_target);
    }

    void update(uint32_t pc, uint32_t target, bool taken) override {
        // Update your predictor state based on actual outcome
        uint32_t index = hashPC(pc);
        
        // Check if prediction was correct (before updating)
        bool predicted_taken = (predictions_[index] >= threshold);
        if (predicted_taken == taken) {
            correct_++;
        } else {
            incorrect_++;
        }
        
        // Update predictor state
        if (taken) {
            // Increment counter (saturate at max)
            if (predictions_[index] < max_value) predictions_[index]++;
        } else {
            // Decrement counter (saturate at min)
            if (predictions_[index] > 0) predictions_[index]--;
        }
    }

    void reset() override {
        // Reset all predictor state
        std::fill(predictions_.begin(), predictions_.end(), initial_value);
        correct_ = 0;
        incorrect_ = 0;
    }

    // Required: Implement BranchPredictorStatistics interface
    uint64_t correct_predictions() const override { return correct_; }
    uint64_t incorrect_predictions() const override { return incorrect_; }
    uint64_t total_predictions() const override { return correct_ + incorrect_; }
    double getAccuracy() const override {
        uint64_t total = total_predictions();
        if (total == 0) return 0.0;
        return (static_cast<double>(correct_) / static_cast<double>(total)) * 100.0;
    }
    std::string getSchemeName() const override { return "Your Predictor Name"; }
    std::string getDescription() const override {
        return "Description of your predictor algorithm";
    }

private:
    uint32_t table_size_;
    std::vector<uint8_t> predictions_;  // Your prediction table
    uint64_t correct_, incorrect_;
    
    uint32_t hashPC(uint32_t pc) const {
        return (pc >> 2) & (table_size_ - 1);
    }
};
```

### Step 2: Add to BranchPredictorType Enum

Add your new predictor type to the enum in `BranchPredictorScheme.h`:

```cpp
enum class BranchPredictorType {
    AlwaysNotTaken,
    AlwaysTaken,
    Bimodal,
    GShare,
    Tournament,
    YourCustomPredictor,  // Add your new type here
};
```

### Step 3: Update the Factory Function

Add a case for your predictor in the `createBranchPredictor` function in `BranchPredictor.h`:

```cpp
inline BranchPredictorScheme* createBranchPredictor(BranchPredictorType type) {
    switch (type) {
        // ... existing cases ...
        case BranchPredictorType::YourCustomPredictor:
            return new YourCustomPredictor(2048);  // Or your preferred table size
        default:
            return new AlwaysNotTakenPredictor();
    }
}
```

### Step 4: Add to GUI (Optional)

To make your branch predictor selectable from the GUI, add it to `MainWindow.cpp` in the `setupUI()` function:

```cpp
branchPredictorCombo_->addItem("Your Predictor Name", 
                               static_cast<int>(BranchPredictorType::YourCustomPredictor));
```

Also add it to the helper function in `BranchPredictorScheme.h`:

```cpp
inline std::string branchPredictorTypeToString(BranchPredictorType type) {
    switch (type) {
        // ... existing cases ...
        case BranchPredictorType::YourCustomPredictor:
            return "Your Predictor Name";
        default:
            return "Unknown";
    }
}
```

## Implementation Guidelines

### Branch Predictor Interface

Your predictor must implement the `BranchPredictorScheme` interface:
- `BranchPrediction predict(uint32_t pc, uint32_t target)` - Make a prediction
- `void update(uint32_t pc, uint32_t target, bool taken)` - Update with actual outcome
- `void reset()` - Reset predictor state

### Prediction Result

The `BranchPrediction` structure contains:
- `bool predicted_taken` - Whether the branch is predicted to be taken
- `uint32_t predicted_target` - Predicted target address (if taken) or PC+4 (if not taken)

### Statistics Tracking

Your predictor must track:
- `correct_` - Number of correct predictions
- `incorrect_` - Number of incorrect predictions

And implement the `BranchPredictorStatistics` interface methods.

### Common Patterns

**Saturating Counter Pattern** (used in Bimodal):
```cpp
if (taken) {
    if (counter < max) counter++;
} else {
    if (counter > min) counter--;
}
```

**History Register Pattern** (used in GShare):
```cpp
// Update history
history = ((history << 1) | (taken ? 1 : 0)) & history_mask;
// Use history in hash
index = (pc_hash ^ history) & table_mask;
```

**Hybrid Selection Pattern** (used in Tournament):
```cpp
// Get predictions from both predictors
pred1 = predictor1.predict(pc, target);
pred2 = predictor2.predict(pc, target);
// Choose based on selector
final_pred = (selector >= threshold) ? pred2 : pred1;
```

## Testing Your Predictor

1. Compile the project
2. Run the GUI
3. Select your branch predictor
4. Load a test program with branches (e.g., `instMem-all.txt`)
5. Run the simulation
6. Check the Statistics tab for prediction accuracy
7. Compare with other predictors on the same program

## Example: Adding a Local History Predictor

Here's a simplified example of adding a local history predictor (similar to GShare but uses per-branch history instead of global history):

```cpp
class LocalHistoryPredictor : public BranchPredictorScheme {
    std::vector<uint32_t> local_history_;  // Per-branch history
    std::vector<uint8_t> counters_;        // Prediction counters
    uint32_t history_bits_;
    
    uint32_t hashPC(uint32_t pc) const {
        return (pc >> 2) & (table_size_ - 1);
    }
    
    BranchPrediction predict(uint32_t pc, uint32_t target) override {
        uint32_t index = hashPC(pc);
        uint32_t history = local_history_[index];
        uint32_t pred_index = (history ^ index) & (table_size_ - 1);
        bool predicted_taken = (counters_[pred_index] >= 2);
        return BranchPrediction(predicted_taken, predicted_taken ? target : (pc + 4));
    }
    
    void update(uint32_t pc, uint32_t target, bool taken) override {
        uint32_t index = hashPC(pc);
        uint32_t history = local_history_[index];
        uint32_t pred_index = (history ^ index) & (table_size_ - 1);
        
        // Check accuracy before updating
        bool predicted_taken = (counters_[pred_index] >= 2);
        if (predicted_taken == taken) {
            correct_++;
        } else {
            incorrect_++;
        }
        
        // Update counter
        if (taken) {
            if (counters_[pred_index] < 3) counters_[pred_index]++;
        } else {
            if (counters_[pred_index] > 0) counters_[pred_index]--;
        }
        
        // Update local history
        local_history_[index] = ((history << 1) | (taken ? 1 : 0)) & history_mask_;
    }
};
```

## Architecture

```
CPU Pipeline
  └─> Branch Predictor (your predictor)
        └─> Prediction Table (your data structures)
```

The branch predictor sits in the CPU pipeline, making predictions in the ID stage and being updated in the EX stage when branches are resolved.

## Notes

- Predictors are queried in the ID stage (when branches are decoded)
- Predictors are updated in the EX stage (when branches are resolved)
- Mispredictions cause pipeline flushes and performance penalties
- All predictors must be thread-safe if used in multi-threaded scenarios (currently single-threaded)
- The predictor is reset when you reset the simulation
- Prediction accuracy is automatically tracked and displayed in the GUI

## Performance Considerations

- **Table Size**: Larger tables generally improve accuracy but use more memory
- **History Length**: Longer history captures more correlation but requires more bits
- **Update Latency**: Fast updates are important for high-frequency processors
- **Hash Function**: Good hash functions distribute branches evenly across the table

## Common Predictor Types

- **Static Predictors**: Always Not Taken, Always Taken, Backward Taken/Forward Not Taken
- **Dynamic Predictors**: Bimodal, GShare, Local History, Tournament
- **Hybrid Predictors**: Tournament (combines multiple predictors)
- **Advanced Predictors**: Perceptron, Neural Branch Predictor, TAGE (Tagged Geometric History Length)

