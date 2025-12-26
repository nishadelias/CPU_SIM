#include "SimulatorController.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <cstring>
#include <memory>
#include "MemoryIf.h"
#include "Cache.h"

SimulatorController::SimulatorController(QObject* parent)
    : QObject(parent)
    , instructionMemory_(nullptr)
    , maxPC_(0)
    , currentCycle_(0)
    , isRunning_(false)
    , cyclesPerSecond_(10)
{
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &SimulatorController::onTimerTick);
    
    instructionMemory_ = new char[MAX_MEMORY_SIZE * 2];
    memset(instructionMemory_, '0', MAX_MEMORY_SIZE * 2);
    
    // Initialize memory hierarchy
    dram_ = nullptr;
    dcache_ = nullptr;
    initializeMemoryHierarchy();
    cpu_.enable_tracing(true);
}

SimulatorController::~SimulatorController() {
    cleanupMemory();
    if (dcache_) delete dcache_;
    if (dram_) delete dram_;
}

bool SimulatorController::loadProgram(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << filename;
        return false;
    }
    
    QTextStream in(&file);
    int i = 0;
    QString line;
    
    while (!in.atEnd() && i < MAX_MEMORY_SIZE * 2) {
        in >> line;
        if (line.length() >= 2) {
            instructionMemory_[i] = line[0].toLatin1();
            i++;
            if (i < MAX_MEMORY_SIZE * 2) {
                instructionMemory_[i] = line[1].toLatin1();
                i++;
            }
        }
    }
    
    maxPC_ = i / 2;
    cpu_.set_max_pc(maxPC_);
    
    // Reset CPU
    resetSimulation();
    
    qDebug() << "Loaded program:" << filename << "Max PC:" << maxPC_;
    return true;
}

void SimulatorController::startSimulation() {
    if (isRunning_) return;
    
    isRunning_ = true;
    timer_->start(1000 / cyclesPerSecond_);
    emit simulationStarted();
}

void SimulatorController::pauseSimulation() {
    if (!isRunning_) return;
    
    isRunning_ = false;
    timer_->stop();
    emit simulationPaused();
}

void SimulatorController::resetSimulation() {
    pauseSimulation();
    
    // Reset CPU state using reset method (avoids copy assignment issues)
    cpu_.reset();
    cpu_.enable_tracing(true);
    cpu_.set_data_memory(dcache_);  // Restore memory hierarchy
    cpu_.set_max_pc(maxPC_);
    
    currentCycle_ = 0;
}

void SimulatorController::stepSimulation() {
    if (isRunning_) return;
    
    if (currentCycle_ >= MAX_CYCLES) {
        qDebug() << "Maximum cycles reached. Stopping simulation.";
        pauseSimulation();
        emit simulationFinished();
        return;
    }
    
    currentCycle_++;
    cpu_.run_pipeline_cycle(instructionMemory_, currentCycle_, false);
    
    emit cycleCompleted(currentCycle_);
    
    if (cpu_.is_pipeline_empty() && cpu_.readPC() >= maxPC_ - 4) {
        emit simulationFinished();
    }
}

void SimulatorController::setSpeed(int cyclesPerSecond) {
    cyclesPerSecond_ = qMax(1, qMin(1000, cyclesPerSecond));
    if (isRunning_) {
        timer_->setInterval(1000 / cyclesPerSecond_);
    }
}

void SimulatorController::onTimerTick() {
    if (currentCycle_ >= MAX_CYCLES) {
        qDebug() << "Maximum cycles reached. Stopping simulation.";
        pauseSimulation();
        emit simulationFinished();
        return;
    }
    
    stepSimulation();
    
    if (cpu_.is_pipeline_empty() && cpu_.readPC() >= maxPC_ - 4) {
        pauseSimulation();
        emit simulationFinished();
    }
}

void SimulatorController::initializeMemory() {
    if (!instructionMemory_) {
        instructionMemory_ = new char[MAX_MEMORY_SIZE * 2];
        memset(instructionMemory_, '0', MAX_MEMORY_SIZE * 2);
    }
}

void SimulatorController::cleanupMemory() {
    if (instructionMemory_) {
        delete[] instructionMemory_;
        instructionMemory_ = nullptr;
    }
}

void SimulatorController::initializeMemoryHierarchy() {
    if (dram_) delete dram_;
    if (dcache_) delete dcache_;
    
    dram_ = new SimpleRAM(64 * 1024);
    dcache_ = new DirectMappedCache(dram_, 4*1024, 32);
    cpu_.set_data_memory(dcache_);
}

