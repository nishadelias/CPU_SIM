#include "SimulatorController.h"
#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include "MemoryIf.h"
#include "Cache.h"
#include "CacheScheme.h"
#include "BranchPredictor.h"
#include "ElfLoader.h"
#include "HexLoader.h"
#include "MemoryMap.h"
#include "ExecutionMode.h"
#include "SimLimits.h"
#include <QSettings>

static bool peek_is_elf(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray h = f.read(4);
    return h.size() >= 4 && static_cast<unsigned char>(h[0]) == 0x7f && h[1] == 'E' && h[2] == 'L' &&
           h[3] == 'F';
}

QString SimulatorController::loadedProgramDescription() const {
    if (lastProgramPath_.isEmpty()) {
        return {};
    }
    if (lastLoadElf_) {
        return QStringLiteral("ELF (compiled C/RISC-V) — entry 0x%1, sp set, brk=0x%2")
            .arg(elf_entry_, 8, 16, QLatin1Char('0'))
            .arg(elf_heap_brk_, 8, 16, QLatin1Char('0'));
    }
    return QStringLiteral("Hex text (instruction memory) — %1 bytes loaded at 0x%2")
        .arg(maxPC_)
        .arg(MemoryMap::HEX_PROGRAM_BASE, 8, 16, QLatin1Char('0'));
}

SimulatorController::SimulatorController(QObject* parent)
    : QObject(parent)
    , maxPC_(0)
    , currentCycle_(0)
    , isRunning_(false)
    , cyclesPerSecond_(10)
    , dram_(nullptr)
    , dcache_(nullptr)
    , currentCacheScheme_(CacheSchemeType::DirectMapped)
    , branch_predictor_(nullptr)
    , currentBranchPredictor_(BranchPredictorType::AlwaysNotTaken)
    , lastLoadElf_(false)
    , elf_entry_(0)
    , elf_heap_brk_(0)
    , maxCycles_(SimLimits::DEFAULT_MAX_CYCLES)
    , cycleLimitReached_(false)
    , fastRunActive_(false)
    , fileLoggingActive_(false)
    , cycleMilliDebt_(0)
{
    QSettings settings;
    maxCycles_ = settings.value("simulation/maxCycles", SimLimits::DEFAULT_MAX_CYCLES).toInt();

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &SimulatorController::onTimerTick);

    initializeMemoryHierarchy();
    initializeBranchPredictor();
    cpu_.enable_tracing(true);
}

SimulatorController::~SimulatorController() {
    if (dcache_) {
        delete dcache_;
    }
    if (dram_) {
        delete dram_;
    }
    if (branch_predictor_) {
        delete branch_predictor_;
    }
}

void SimulatorController::reloadProgramIntoRam() {
    if (lastProgramPath_.isEmpty() || !dram_) {
        return;
    }
    if (lastLoadElf_) {
        ElfLoadResult r = load_elf32_into_ram(lastProgramPath_.toStdString(), *dram_);
        if (r.ok) {
            elf_entry_ = r.entry;
            elf_heap_brk_ = r.heap_brk;
            maxPC_ = 0;
        }
    } else {
        uint32_t nb = 0;
        if (load_hex_text_file(lastProgramPath_.toStdString(), *dram_, MemoryMap::HEX_PROGRAM_BASE, nb)) {
            maxPC_ = static_cast<int>(nb);
        }
    }
}

void SimulatorController::applyCpuLoadState() {
    cpu_.set_ram_size(MemoryMap::RAM_SIZE);
    cpu_.set_execution_mode(ExecutionMode::Educational);
    if (lastLoadElf_) {
        cpu_.set_use_hex_bounds(false);
        cpu_.set_max_pc(0);
        cpu_.set_pc(elf_entry_);
        cpu_.set_heap_brk(elf_heap_brk_);
        cpu_.set_register_value(2, static_cast<int32_t>(MemoryMap::STACK_TOP - 16));
    } else {
        cpu_.set_use_hex_bounds(true);
        cpu_.set_max_pc(maxPC_);
        cpu_.set_pc(MemoryMap::HEX_PROGRAM_BASE);
        cpu_.set_heap_brk(0);
    }
}

bool SimulatorController::simulationShouldFinish() {
    if (cpu_.is_faulted()) {
        return true;
    }
    if (lastLoadElf_) {
        return cpu_.is_halted();
    }
    return cpu_.is_pipeline_empty() && cpu_.readPC() >= static_cast<unsigned long>(maxPC_);
}

bool SimulatorController::loadProgram(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << filename;
        return false;
    }
    file.close();

    const bool is_elf = peek_is_elf(filename);

    if (is_elf) {
        SimpleRAM tmp(MemoryMap::RAM_SIZE);
        ElfLoadResult er = load_elf32_into_ram(filename.toStdString(), tmp);
        if (!er.ok) {
            lastLoadError_ = QString::fromStdString(er.error);
            qDebug() << "ELF load failed:" << lastLoadError_;
            return false;
        }
    } else {
        SimpleRAM tmp(MemoryMap::RAM_SIZE);
        uint32_t nb = 0;
        if (!load_hex_text_file(filename.toStdString(), tmp, MemoryMap::HEX_PROGRAM_BASE, nb)) {
            lastLoadError_ = QStringLiteral("Hex text load failed (unreadable or out of range).");
            qDebug() << lastLoadError_;
            return false;
        }
        if (nb == 0) {
            lastLoadError_ = QStringLiteral("Hex text file has no valid byte tokens (need pairs like 93 00 ...).");
            qDebug() << lastLoadError_;
            return false;
        }
    }

    lastProgramPath_ = filename;
    lastLoadElf_ = is_elf;
    lastLoadError_.clear();

    QFileInfo fileInfo(filename);
    QDir dir(fileInfo.absolutePath());
    bool foundRoot = false;
    for (int i = 0; i < 5 && !dir.isRoot(); ++i) {
        if (dir.exists("CMakeLists.txt") || dir.dirName() == "CPU_SIM") {
            foundRoot = true;
            break;
        }
        dir.cdUp();
    }
    if (foundRoot) {
        logFilePath_ = dir.absoluteFilePath("pipeline.log");
    } else {
        logFilePath_ = fileInfo.absolutePath() + "/pipeline.log";
    }
    qDebug() << "Logging to:" << logFilePath_;

    resetSimulation();

    fileLoggingActive_ = !logFilePath_.isEmpty();
    if (fileLoggingActive_) {
        cpu_.set_logging(true, logFilePath_.toStdString());
    }

    qDebug() << "Loaded program:" << filename << "elf:" << lastLoadElf_ << "Max PC (hex bytes):" << maxPC_;
    return true;
}

void SimulatorController::startSimulation() {
    if (isRunning_) {
        return;
    }

    // Keep tracing on so pipeline/memory/dependency tabs populate; disable file I/O only.
    fastRunActive_ = true;
    cpu_.enable_tracing(true);
    cpu_.set_logging(false, "");

    cycleMilliDebt_ = 0;
    isRunning_ = true;
    timer_->start(SimLimits::GUI_REFRESH_INTERVAL_MS);
    emit simulationStarted();
}

void SimulatorController::pauseSimulation() {
    if (!isRunning_) {
        return;
    }

    isRunning_ = false;
    timer_->stop();

    if (fastRunActive_) {
        fastRunActive_ = false;
        cpu_.enable_tracing(true);
        if (fileLoggingActive_ && !logFilePath_.isEmpty()) {
            cpu_.set_logging(true, logFilePath_.toStdString());
        }
    }

    emit simulationPaused();
}

void SimulatorController::resetSimulation() {
    pauseSimulation();

    initializeMemoryHierarchy();
    initializeBranchPredictor();

    reloadProgramIntoRam();

    cpu_.reset();
    cpu_.enable_tracing(true);
    cpu_.set_data_memory(dcache_);
    cpu_.set_branch_predictor(branch_predictor_);
    applyCpuLoadState();

    if (fileLoggingActive_ && !logFilePath_.isEmpty() && !lastProgramPath_.isEmpty()) {
        cpu_.set_logging(true, logFilePath_.toStdString());
    }

    currentCycle_ = 0;
    cycleMilliDebt_ = 0;
    fastRunActive_ = false;
}

void SimulatorController::stepSimulation() {
    if (isRunning_) {
        return;
    }

    if (currentCycle_ >= maxCycles_) {
        qDebug() << "Maximum cycles reached. Stopping simulation.";
        cycleLimitReached_ = true;
        pauseSimulation();
        emit cycleLimitReached();
        emit simulationFinished();
        return;
    }

    currentCycle_++;
    cpu_.run_pipeline_cycle(currentCycle_, false);

    emit cycleCompleted(currentCycle_);

    if (simulationShouldFinish()) {
        emit simulationFinished();
    }
}

void SimulatorController::setMaxCycles(int maxCycles) {
    maxCycles_ = qMax(1000, maxCycles);
    QSettings settings;
    settings.setValue("simulation/maxCycles", maxCycles_);
}

void SimulatorController::setSpeed(int cyclesPerSecond) {
    cyclesPerSecond_ = qMax(1, qMin(SimLimits::MAX_SIM_SPEED_CPS, cyclesPerSecond));
    if (isRunning_) {
        timer_->start(SimLimits::GUI_REFRESH_INTERVAL_MS);
    }
}

void SimulatorController::onTimerTick() {
    // Accumulate fractional cycles across 16 ms ticks so low speeds (e.g. 1 cps)
    // are not rounded up to ~62 cps by a minimum batch of 1.
    cycleMilliDebt_ += cyclesPerSecond_ * SimLimits::GUI_REFRESH_INTERVAL_MS;
    const int batch = cycleMilliDebt_ / 1000;
    cycleMilliDebt_ %= 1000;
    if (batch <= 0) {
        return;
    }

    for (int i = 0; i < batch; ++i) {
        if (currentCycle_ >= maxCycles_) {
            qDebug() << "Maximum cycles reached. Stopping simulation.";
            cycleLimitReached_ = true;
            pauseSimulation();
            emit cycleLimitReached();
            emit simulationFinished();
            return;
        }

        currentCycle_++;
        cpu_.run_pipeline_cycle(currentCycle_, false);

        if (simulationShouldFinish()) {
            pauseSimulation();
            emit cycleCompleted(currentCycle_);
            emit simulationFinished();
            return;
        }
    }

    emit cycleCompleted(currentCycle_);
}

void SimulatorController::initializeMemoryHierarchy() {
    if (dram_) {
        delete dram_;
    }
    if (dcache_) {
        delete dcache_;
    }

    dram_ = new SimpleRAM(64 * 1024);
    dcache_ = createCacheScheme(currentCacheScheme_, dram_, 4 * 1024, 32);
    cpu_.set_data_memory(dcache_);
}

void SimulatorController::setCacheScheme(CacheSchemeType scheme) {
    if (currentCacheScheme_ == scheme && dcache_ != nullptr) {
        return;
    }

    currentCacheScheme_ = scheme;

    if (!isRunning_) {
        initializeMemoryHierarchy();
        cpu_.set_data_memory(dcache_);
    }
}

void SimulatorController::setBranchPredictor(BranchPredictorType type) {
    if (currentBranchPredictor_ == type && branch_predictor_ != nullptr) {
        return;
    }

    currentBranchPredictor_ = type;

    if (!isRunning_) {
        initializeBranchPredictor();
        cpu_.set_branch_predictor(branch_predictor_);
    }
}

void SimulatorController::initializeBranchPredictor() {
    if (branch_predictor_) {
        delete branch_predictor_;
    }
    branch_predictor_ = createBranchPredictor(currentBranchPredictor_);
    cpu_.set_branch_predictor(branch_predictor_);
}
