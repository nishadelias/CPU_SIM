#ifndef REGISTER_WIDGET_H
#define REGISTER_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include "CPU.h"

class RegisterWidget : public QWidget {
    Q_OBJECT

public:
    explicit RegisterWidget(QWidget* parent = nullptr);
    void updateDisplay(CPU* cpu);

private:
    void setupUI();
    void updateRegisterTable(CPU* cpu);

    QVBoxLayout* layout_;
    QLabel* titleLabel_;
    QTableWidget* registerTable_;
    
    static const QStringList REGISTER_NAMES;
};

#endif // REGISTER_WIDGET_H

