#ifndef SIMULATOR_CONTROLLER_H
#define SIMULATOR_CONTROLLER_H

#include "CPU.h"
#include <QObject>
#include <QTimer>
#include <QString>
#include <memory>

// Forward declarations
class SimpleRAM;
class DirectMappedCache;

class SimulatorController : public QObject {
    Q_OBJECT

public:
    explicit SimulatorController(QObject* parent = nullptr);
    ~SimulatorController();

    bool loadProgram(const QString& filename);
    void startSimulation();
    void pauseSimulation();
    void resetSimulation();
    void stepSimulation();
    void setSpeed(int cyclesPerSecond);

    CPU* getCPU() { return &cpu_; }
    bool isRunning() const { return isRunning_; }
    int getCurrentCycle() const { return currentCycle_; }

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
    char* instructionMemory_;
    int maxPC_;
    int currentCycle_;
    bool isRunning_;
    int cyclesPerSecond_;
    SimpleRAM* dram_;
    DirectMappedCache* dcache_;
    static const int MAX_MEMORY_SIZE = 4096;
    static const int MAX_CYCLES = 10000;  // Prevent infinite loops

    void initializeMemory();
    void cleanupMemory();
    void initializeMemoryHierarchy();
};

#endif // SIMULATOR_CONTROLLER_H

