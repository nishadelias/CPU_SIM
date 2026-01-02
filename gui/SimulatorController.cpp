#include "SimulatorController.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <cstring>
#include <memory>
#include "MemoryIf.h"
#include "Cache.h"
#include "CacheScheme.h"
#include "BranchPredictor.h"

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
    currentCacheScheme_ = CacheSchemeType::DirectMapped;  // Default
    branch_predictor_ = nullptr;
    currentBranchPredictor_ = BranchPredictorType::AlwaysNotTaken;  // Default
    initializeMemoryHierarchy();
    initializeBranchPredictor();
    cpu_.enable_tracing(true);
}

SimulatorController::~SimulatorController() {
    cleanupMemory();
    if (dcache_) delete dcache_;
    if (dram_) delete dram_;
    if (branch_predictor_) delete branch_predictor_;
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
    
    // Enable logging to pipeline.log in the project root directory
    QFileInfo fileInfo(filename);
    logFilePath_ = fileInfo.absolutePath() + "/pipeline.log";
    // If loading from instruction_memory subdirectory, go up one level to project root
    if (fileInfo.absolutePath().endsWith("instruction_memory")) {
        QDir dir(fileInfo.absolutePath());
        dir.cdUp();
        logFilePath_ = dir.absoluteFilePath("pipeline.log");
    }
    qDebug() << "Logging to:" << logFilePath_;
    cpu_.set_logging(true, logFilePath_.toStdString());
    
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
    
    // Reset memory hierarchy (cache) to ensure consistent stats
    // This will use the current cache scheme
    initializeMemoryHierarchy();
    
    // Reset branch predictor
    initializeBranchPredictor();
    
    // Reset CPU state using reset method (avoids copy assignment issues)
    cpu_.reset();
    cpu_.enable_tracing(true);
    cpu_.set_data_memory(dcache_);  // Restore memory hierarchy
    cpu_.set_branch_predictor(branch_predictor_);  // Restore branch predictor
    cpu_.set_max_pc(maxPC_);
    
    // Re-enable logging after reset (reset closes the log file)
    if (maxPC_ > 0 && !logFilePath_.isEmpty()) {
        qDebug() << "Re-enabling logging to:" << logFilePath_;
        cpu_.set_logging(true, logFilePath_.toStdString());
    }
    
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
    
    // Check if program is finished: pipeline empty and PC beyond program end
    if (cpu_.is_pipeline_empty() && cpu_.readPC() >= maxPC_) {
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
    
    // Run cycle directly (don't call stepSimulation which checks isRunning_)
    currentCycle_++;
    cpu_.run_pipeline_cycle(instructionMemory_, currentCycle_, false);
    
    emit cycleCompleted(currentCycle_);
    
    // Check if program is finished: pipeline empty and PC beyond program end
    if (cpu_.is_pipeline_empty() && cpu_.readPC() >= maxPC_) {
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
    dcache_ = createCacheScheme(currentCacheScheme_, dram_, 4*1024, 32);
    cpu_.set_data_memory(dcache_);
}

void SimulatorController::setCacheScheme(CacheSchemeType scheme) {
    if (currentCacheScheme_ == scheme && dcache_ != nullptr) {
        return;  // No change needed
    }
    
    currentCacheScheme_ = scheme;
    
    // Only reinitialize if not currently running
    if (!isRunning_) {
        initializeMemoryHierarchy();
        cpu_.set_data_memory(dcache_);
    }
    // Otherwise, will be reinitialized on next reset
}

void SimulatorController::setBranchPredictor(BranchPredictorType type) {
    if (currentBranchPredictor_ == type && branch_predictor_ != nullptr) {
        return;  // No change needed
    }
    
    currentBranchPredictor_ = type;
    
    // Only reinitialize if not currently running
    if (!isRunning_) {
        initializeBranchPredictor();
        cpu_.set_branch_predictor(branch_predictor_);
    }
    // Otherwise, will be reinitialized on next reset
}

void SimulatorController::initializeBranchPredictor() {
    if (branch_predictor_) delete branch_predictor_;
    branch_predictor_ = createBranchPredictor(currentBranchPredictor_);
    cpu_.set_branch_predictor(branch_predictor_);
}

