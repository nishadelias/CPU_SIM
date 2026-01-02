#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QComboBox>
#include "SimulatorController.h"
#include "CacheScheme.h"
#include "PipelineWidget.h"
#include "StatsWidget.h"
#include "RegisterWidget.h"
#include "MemoryWidget.h"
#include "DependencyWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void startSimulation();
    void pauseSimulation();
    void resetSimulation();
    void stepSimulation();
    void onCycleCompleted(int cycle);
    void onSimulationFinished();
    void onSpeedChanged(int value);
    void onCacheSchemeChanged(int index);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void connectSignals();
    void updateUI();

    SimulatorController* controller_;
    
    // UI Components
    QSplitter* mainSplitter_;
    QSplitter* leftSplitter_;
    QTabWidget* tabWidget_;
    
    // Control Panel
    QWidget* controlPanel_;
    QPushButton* btnOpen_;
    QPushButton* btnStart_;
    QPushButton* btnPause_;
    QPushButton* btnReset_;
    QPushButton* btnStep_;
    QLabel* lblSpeed_;
    QSlider* speedSlider_;
    QSpinBox* speedSpinBox_;
    QLabel* lblCacheScheme_;
    QComboBox* cacheSchemeCombo_;
    QLabel* lblCycle_;
    QLabel* lblStatus_;
    QLabel* lblFilename_;
    QString currentFilename_;
    
    // Visualization Widgets
    PipelineWidget* pipelineWidget_;
    StatsWidget* statsWidget_;
    RegisterWidget* registerWidget_;
    MemoryWidget* memoryWidget_;
    DependencyWidget* dependencyWidget_;
    
    // Menu items
    QAction* actionOpen_;
    QAction* actionStart_;
    QAction* actionPause_;
    QAction* actionReset_;
    QAction* actionStep_;
    QAction* actionExit_;
};

#endif // MAIN_WINDOW_H

