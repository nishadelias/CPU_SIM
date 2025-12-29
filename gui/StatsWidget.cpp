#include "StatsWidget.h"
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>
#include <QHeaderView>
#include <QPainter>
#include <QTableWidgetItem>
#include <QList>

StatsWidget::StatsWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void StatsWidget::setupUI() {
    layout_ = new QVBoxLayout(this);
    
    titleLabel_ = new QLabel("<h2>Statistics</h2>", this);
    layout_->addWidget(titleLabel_);
    
    // Statistics table
    statsTable_ = new QTableWidget(this);
    statsTable_->setColumnCount(2);
    statsTable_->setHorizontalHeaderLabels({"Metric", "Value"});
    statsTable_->setAlternatingRowColors(true);
    statsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    statsTable_->horizontalHeader()->setStretchLastSection(true);
    statsTable_->setColumnWidth(0, 200);
    layout_->addWidget(statsTable_);
    
    // Performance metrics label
    performanceLabel_ = new QLabel(this);
    performanceLabel_->setWordWrap(true);
    layout_->addWidget(performanceLabel_);
    
    // Instruction distribution chart
    instructionChartView_ = new QChartView(this);
    instructionChartView_->setRenderHint(QPainter::Antialiasing);
    instructionChartView_->setMinimumHeight(200);
    layout_->addWidget(instructionChartView_);
}

void StatsWidget::updateDisplay(CPU* cpu) {
    if (!cpu) return;
    
    updateStatisticsTable(cpu);
    updateInstructionChart(cpu);
    updatePerformanceMetrics(cpu);
}

void StatsWidget::updateStatisticsTable(CPU* cpu) {
    const auto& stats = cpu->get_statistics();
    
    QStringList metrics = {
        "Total Instructions",
        "R-type Instructions",
        "I-type Instructions",
        "Load Instructions",
        "Store Instructions",
        "Branch Instructions",
        "Jump Instructions",
        "LUI/AUIPC Instructions",
        "Pipeline Stalls",
        "Pipeline Flushes",
        "Branches Taken",
        "Branches Not Taken",
        "Total Cycles",
        "Instructions Retired",
        "Cache Hits",
        "Cache Misses",
        "Memory Reads",
        "Memory Writes"
    };
    
    QList<quint64> values = {
        stats.total_instructions,
        stats.r_type_count,
        stats.i_type_count,
        stats.load_count,
        stats.store_count,
        stats.branch_count,
        stats.jump_count,
        stats.lui_auipc_count,
        stats.stall_count,
        stats.flush_count,
        stats.branch_taken_count,
        stats.branch_not_taken_count,
        stats.total_cycles,
        stats.instructions_retired,
        stats.cache_hits,
        stats.cache_misses,
        stats.memory_reads,
        stats.memory_writes
    };
    
    statsTable_->setRowCount(metrics.size());
    
    for (int i = 0; i < metrics.size(); ++i) {
        statsTable_->setItem(i, 0, new QTableWidgetItem(metrics[i]));
        statsTable_->setItem(i, 1, new QTableWidgetItem(QString::number(values[i])));
    }
}

void StatsWidget::updateInstructionChart(CPU* cpu) {
    const auto& stats = cpu->get_statistics();
    
    QPieSeries* series = new QPieSeries();
    
    if (stats.r_type_count > 0)
        series->append("R-type", stats.r_type_count);
    if (stats.i_type_count > 0)
        series->append("I-type", stats.i_type_count);
    if (stats.load_count > 0)
        series->append("Load", stats.load_count);
    if (stats.store_count > 0)
        series->append("Store", stats.store_count);
    if (stats.branch_count > 0)
        series->append("Branch", stats.branch_count);
    if (stats.jump_count > 0)
        series->append("Jump", stats.jump_count);
    if (stats.lui_auipc_count > 0)
        series->append("LUI/AUIPC", stats.lui_auipc_count);
    
    if (series->count() == 0) {
        series->append("No instructions", 1);
    }
    
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Instruction Distribution");
    chart->legend()->setAlignment(Qt::AlignRight);
    
    instructionChartView_->setChart(chart);
}

void StatsWidget::updatePerformanceMetrics(CPU* cpu) {
    const auto& stats = cpu->get_statistics();
    
    double cpi = stats.getCPI();
    double hitRate = stats.getCacheHitRate();
    double utilization = stats.getPipelineUtilization();
    
    QString text = QString(
        "<b>Performance Metrics:</b><br>"
        "CPI: %1<br>"
        "Cache Hit Rate: %2%<br>"
        "Pipeline Utilization: %3%"
    ).arg(cpi, 0, 'f', 2)
     .arg(hitRate, 0, 'f', 2)
     .arg(utilization, 0, 'f', 2);
    
    performanceLabel_->setText(text);
}

