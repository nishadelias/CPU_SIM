#ifndef SIMULATOR_CONTROLLER_H
#define SIMULATOR_CONTROLLER_H

#include "CPU.h"
#include "CacheScheme.h"
#include "BranchPredictorScheme.h"
#include <QObject>
#include <QTimer>
#include <QString>
#include <memory>
#include <cstdint>

class SimpleRAM;

class SimulatorController : public QObject {
    Q_OBJECT

public:
    explicit SimulatorController(QObject* parent = nullptr);
    ~SimulatorController();

    bool loadProgram(const QString& filename);
    QString lastLoadError() const { return lastLoadError_; }
    void startSimulation();
    void pauseSimulation();
    void resetSimulation();
    void stepSimulation();
    void setSpeed(int cyclesPerSecond);

    CPU* getCPU() { return &cpu_; }
    bool isRunning() const { return isRunning_; }
    int getCurrentCycle() const { return currentCycle_; }
    bool lastLoadedElf() const { return lastLoadElf_; }

    /** Short description for the UI: ELF vs hex, entry/byte count. Empty if nothing loaded. */
    QString loadedProgramDescription() const;

    // Cache scheme management
    void setCacheScheme(CacheSchemeType scheme);
    CacheSchemeType getCacheScheme() const { return currentCacheScheme_; }

    // Branch predictor management
    void setBranchPredictor(BranchPredictorType type);
    BranchPredictorType getBranchPredictor() const { return currentBranchPredictor_; }

signals:
    void cycleCompleted(int cycle);
    void simulationFinished();
    void simulationStarted();
    void simulationPaused();

private slots:
    void onTimerTick();

private:
    CPU cpu_;
    QTimer* timer_;
    int maxPC_;
    int currentCycle_;
    bool isRunning_;
    int cyclesPerSecond_;
    SimpleRAM* dram_;
    CacheScheme* dcache_;
    CacheSchemeType currentCacheScheme_;
    BranchPredictorScheme* branch_predictor_;
    BranchPredictorType currentBranchPredictor_;
    QString logFilePath_;
    QString lastProgramPath_;
    QString lastLoadError_;
    bool lastLoadElf_;
    uint32_t elf_entry_;
    uint32_t elf_heap_brk_;

    static const int MAX_CYCLES = 200000;  // Prevent infinite loops (matches CLI default scale)

    void initializeMemoryHierarchy();
    void initializeBranchPredictor();
    void reloadProgramIntoRam();
    void applyCpuLoadState();
    bool simulationShouldFinish();
};

#endif // SIMULATOR_CONTROLLER_H
