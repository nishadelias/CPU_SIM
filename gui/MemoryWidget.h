#ifndef MEMORY_WIDGET_H
#define MEMORY_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include "CPU.h"

class MemoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit MemoryWidget(QWidget* parent = nullptr);
    void updateDisplay(CPU* cpu);

private:
    void setupUI();
    void updateMemoryTable(CPU* cpu);

    QVBoxLayout* layout_;
    QLabel* titleLabel_;
    QTableWidget* memoryTable_;
};

#endif // MEMORY_WIDGET_H

