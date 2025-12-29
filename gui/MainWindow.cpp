#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QPainter>
#include <QKeySequence>
#include <QAbstractItemView>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , controller_(new SimulatorController(this))
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    connectSignals();
    updateUI();
    
    setWindowTitle("RISC-V CPU Simulator GUI");
    resize(1400, 900);
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
    // Create main splitter
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter_);
    
    // Left side: Control panel and pipeline
    leftSplitter_ = new QSplitter(Qt::Vertical, mainSplitter_);
    
    // Control Panel
    controlPanel_ = new QWidget(leftSplitter_);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel_);
    
    QGroupBox* fileGroup = new QGroupBox("File", controlPanel_);
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    btnOpen_ = new QPushButton("Open Program", fileGroup);
    fileLayout->addWidget(btnOpen_);
    
    // Filename label (will be updated when file is opened)
    QLabel* filenameLabel = new QLabel("No file loaded", fileGroup);
    filenameLabel->setWordWrap(true);
    filenameLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    fileLayout->addWidget(filenameLabel);
    
    // Store reference to filename label
    lblFilename_ = filenameLabel;
    
    QGroupBox* controlGroup = new QGroupBox("Simulation Control", controlPanel_);
    QVBoxLayout* controlGroupLayout = new QVBoxLayout(controlGroup);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    btnStart_ = new QPushButton("Start", controlGroup);
    btnPause_ = new QPushButton("Pause", controlGroup);
    btnReset_ = new QPushButton("Reset", controlGroup);
    btnStep_ = new QPushButton("Step", controlGroup);
    buttonLayout->addWidget(btnStart_);
    buttonLayout->addWidget(btnPause_);
    buttonLayout->addWidget(btnReset_);
    buttonLayout->addWidget(btnStep_);
    controlGroupLayout->addLayout(buttonLayout);
    
    QFormLayout* speedLayout = new QFormLayout();
    lblSpeed_ = new QLabel("Speed:", controlGroup);
    speedSlider_ = new QSlider(Qt::Horizontal, controlGroup);
    speedSlider_->setRange(1, 100);
    speedSlider_->setValue(10);
    speedSpinBox_ = new QSpinBox(controlGroup);
    speedSpinBox_->setRange(1, 100);
    speedSpinBox_->setValue(10);
    speedSpinBox_->setSuffix(" cycles/sec");
    speedLayout->addRow(lblSpeed_, speedSlider_);
    speedLayout->addRow("", speedSpinBox_);
    controlGroupLayout->addLayout(speedLayout);
    
    controlLayout->addWidget(fileGroup);
    controlLayout->addWidget(controlGroup);
    controlLayout->addStretch();
    
    leftSplitter_->addWidget(controlPanel_);
    leftSplitter_->setStretchFactor(0, 0);
    
    // Right side: Tab widget with all visualizations
    tabWidget_ = new QTabWidget(mainSplitter_);
    tabWidget_->setTabsClosable(false);
    tabWidget_->setMovable(true);
    
    // Create all widgets
    pipelineWidget_ = new PipelineWidget(tabWidget_);
    statsWidget_ = new StatsWidget(tabWidget_);
    registerWidget_ = new RegisterWidget(tabWidget_);
    memoryWidget_ = new MemoryWidget(tabWidget_);
    dependencyWidget_ = new DependencyWidget(tabWidget_);
    
    // Add all widgets as tabs
    tabWidget_->addTab(pipelineWidget_, "Pipeline Execution Trace");
    tabWidget_->addTab(statsWidget_, "Statistics");
    tabWidget_->addTab(registerWidget_, "Register File");
    tabWidget_->addTab(memoryWidget_, "Memory Access History");
    tabWidget_->addTab(dependencyWidget_, "Instruction Dependencies");
    
    mainSplitter_->addWidget(leftSplitter_);
    mainSplitter_->addWidget(tabWidget_);
    mainSplitter_->setStretchFactor(0, 0);
    mainSplitter_->setStretchFactor(1, 1);
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("&File");
    actionOpen_ = fileMenu->addAction("&Open Program...", this, &MainWindow::openFile, QKeySequence::Open);
    fileMenu->addSeparator();
    actionExit_ = fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);
    
    QMenu* simulationMenu = menuBar()->addMenu("&Simulation");
    actionStart_ = simulationMenu->addAction("&Start", this, &MainWindow::startSimulation, QKeySequence(Qt::Key_F5));
    actionPause_ = simulationMenu->addAction("&Pause", this, &MainWindow::pauseSimulation, QKeySequence(Qt::Key_Space));
    actionReset_ = simulationMenu->addAction("&Reset", this, &MainWindow::resetSimulation, QKeySequence(Qt::Key_R));
    actionStep_ = simulationMenu->addAction("&Step", this, &MainWindow::stepSimulation, QKeySequence(Qt::Key_F10));
}

void MainWindow::setupToolBar() {
    QToolBar* toolBar = addToolBar("Main");
    toolBar->addAction(actionOpen_);
    toolBar->addSeparator();
    toolBar->addAction(actionStart_);
    toolBar->addAction(actionPause_);
    toolBar->addAction(actionReset_);
    toolBar->addAction(actionStep_);
}

void MainWindow::setupStatusBar() {
    lblCycle_ = new QLabel("Cycle: 0", statusBar());
    lblStatus_ = new QLabel("Ready", statusBar());
    statusBar()->addWidget(lblCycle_);
    statusBar()->addPermanentWidget(lblStatus_);
}

void MainWindow::connectSignals() {
    // File operations
    connect(btnOpen_, &QPushButton::clicked, this, &MainWindow::openFile);
    
    // Simulation control
    connect(btnStart_, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(btnPause_, &QPushButton::clicked, this, &MainWindow::pauseSimulation);
    connect(btnReset_, &QPushButton::clicked, this, &MainWindow::resetSimulation);
    connect(btnStep_, &QPushButton::clicked, this, &MainWindow::stepSimulation);
    
    // Speed control
    connect(speedSlider_, &QSlider::valueChanged, speedSpinBox_, &QSpinBox::setValue);
    connect(speedSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        speedSlider_->setValue(value);
    });
    connect(speedSlider_, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);
    
    // Controller signals
    connect(controller_, &SimulatorController::cycleCompleted, this, &MainWindow::onCycleCompleted);
    connect(controller_, &SimulatorController::simulationFinished, this, &MainWindow::onSimulationFinished);
    connect(controller_, &SimulatorController::simulationStarted, this, [this]() {
        lblStatus_->setText("Running");
        updateUI();
    });
    connect(controller_, &SimulatorController::simulationPaused, this, [this]() {
        lblStatus_->setText("Paused");
        updateUI();
    });
}

void MainWindow::updateUI() {
    bool running = controller_->isRunning();
    bool finished = (lblStatus_->text() == "Finished");
    btnStart_->setEnabled(!running && !finished && !currentFilename_.isEmpty());
    btnPause_->setEnabled(running);
    btnStep_->setEnabled(!running && !finished);
}

void MainWindow::openFile() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Open Instruction Memory File",
        "instruction_memory",
        "Text Files (*.txt);;All Files (*)"
    );
    
    if (!filename.isEmpty()) {
        if (controller_->loadProgram(filename)) {
            currentFilename_ = filename;
            QString fileName = QFileInfo(filename).fileName();
            lblFilename_->setText(fileName);
            lblFilename_->setStyleSheet("QLabel { color: black; font-style: normal; }");
            setWindowTitle("RISC-V CPU Simulator GUI - " + fileName);
            lblStatus_->setText("Ready");
            resetSimulation();
        } else {
            QMessageBox::warning(this, "Error", "Failed to load program file.");
        }
    }
}

void MainWindow::startSimulation() {
    controller_->startSimulation();
    updateUI();
}

void MainWindow::pauseSimulation() {
    controller_->pauseSimulation();
    updateUI();
}

void MainWindow::resetSimulation() {
    controller_->resetSimulation();
    lblCycle_->setText("Cycle: 0");
    lblStatus_->setText("Ready");
    updateUI();
    
    // Update all widgets
    pipelineWidget_->updateDisplay(controller_->getCPU());
    statsWidget_->updateDisplay(controller_->getCPU());
    registerWidget_->updateDisplay(controller_->getCPU());
    memoryWidget_->updateDisplay(controller_->getCPU());
    dependencyWidget_->updateDisplay(controller_->getCPU());
}

void MainWindow::stepSimulation() {
    controller_->stepSimulation();
    onCycleCompleted(controller_->getCurrentCycle());
}

void MainWindow::onCycleCompleted(int cycle) {
    lblCycle_->setText(QString("Cycle: %1").arg(cycle));
    
    // Update all widgets
    CPU* cpu = controller_->getCPU();
    pipelineWidget_->updateDisplay(cpu);
    statsWidget_->updateDisplay(cpu);
    registerWidget_->updateDisplay(cpu);
    memoryWidget_->updateDisplay(cpu);
    dependencyWidget_->updateDisplay(cpu);
}

void MainWindow::onSimulationFinished() {
    lblStatus_->setText("Finished");
    updateUI();
    // Disable step button when finished
    btnStep_->setEnabled(false);
}

void MainWindow::onSpeedChanged(int value) {
    controller_->setSpeed(value);
}

