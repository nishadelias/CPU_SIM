#ifndef DEPENDENCY_WIDGET_H
#define DEPENDENCY_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include "CPU.h"

class DependencyWidget : public QWidget {
    Q_OBJECT

public:
    explicit DependencyWidget(QWidget* parent = nullptr);
    void updateDisplay(CPU* cpu);

private:
    void setupUI();
    void updateDependencyTable(CPU* cpu);

    QVBoxLayout* layout_;
    QLabel* titleLabel_;
    QTableWidget* dependencyTable_;
};

#endif // DEPENDENCY_WIDGET_H

