#ifndef STATS_WIDGET_H
#define STATS_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QtCharts/QChartView>
#include "CPU.h"
#include "BranchPredictorScheme.h"

QT_BEGIN_NAMESPACE
class QPieSeries;
class QChart;
QT_END_NAMESPACE

class StatsWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatsWidget(QWidget* parent = nullptr);
    void updateDisplay(CPU* cpu);

private:
    void setupUI();
    void updateStatisticsTable(CPU* cpu);
    void updateInstructionChart(CPU* cpu);
    void updatePerformanceMetrics(CPU* cpu);

    QVBoxLayout* layout_;
    QLabel* titleLabel_;
    QTableWidget* statsTable_;
    QChartView* instructionChartView_;
    QLabel* performanceLabel_;
};

#endif // STATS_WIDGET_H

