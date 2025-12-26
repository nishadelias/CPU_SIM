#ifndef PIPELINE_WIDGET_H
#define PIPELINE_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QScrollBar>
#include "CPU.h"

class PipelineWidget : public QWidget {
    Q_OBJECT

public:
    explicit PipelineWidget(QWidget* parent = nullptr);
    void updateDisplay(CPU* cpu);

private:
    void setupUI();
    void updatePipelineTable(CPU* cpu);
    QString formatInstruction(const PipelineSnapshot& snapshot, const QString& stageName);

    QVBoxLayout* layout_;
    QLabel* titleLabel_;
    QTableWidget* pipelineTable_;
    QScrollBar* cycleScrollBar_;
    
    static const QStringList COLUMN_HEADERS;
};

#endif // PIPELINE_WIDGET_H

