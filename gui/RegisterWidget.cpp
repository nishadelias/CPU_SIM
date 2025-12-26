#include "RegisterWidget.h"
#include <QHeaderView>
#include <QColor>
#include <QBrush>
#include <QMap>
#include <QTableWidgetItem>

const QStringList RegisterWidget::REGISTER_NAMES = {
    "Zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

RegisterWidget::RegisterWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void RegisterWidget::setupUI() {
    layout_ = new QVBoxLayout(this);
    
    titleLabel_ = new QLabel("<h3>Register File</h3>", this);
    layout_->addWidget(titleLabel_);
    
    registerTable_ = new QTableWidget(this);
    registerTable_->setColumnCount(3);
    registerTable_->setHorizontalHeaderLabels({"Register", "Name", "Value"});
    registerTable_->setAlternatingRowColors(true);
    registerTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    registerTable_->horizontalHeader()->setStretchLastSection(true);
    registerTable_->setColumnWidth(0, 60);
    registerTable_->setColumnWidth(1, 80);
    
    layout_->addWidget(registerTable_);
}

void RegisterWidget::updateDisplay(CPU* cpu) {
    if (!cpu) return;
    
    updateRegisterTable(cpu);
}

void RegisterWidget::updateRegisterTable(CPU* cpu) {
    const int32_t* registers = cpu->get_all_registers();
    const auto& regHistory = cpu->get_register_history();
    
    // Create a map of recent register changes
    QMap<int, int32_t> recentChanges;
    uint64_t currentCycle = cpu->get_statistics().total_cycles;
    for (const auto& change : regHistory) {
        if (static_cast<uint64_t>(change.cycle) == currentCycle) {
            recentChanges[change.register_num] = change.new_value;
        }
    }
    
    registerTable_->setRowCount(32);
    
    for (int i = 0; i < 32; ++i) {
        // Register number
        registerTable_->setItem(i, 0, new QTableWidgetItem(QString("x%1").arg(i)));
        
        // Register name
        registerTable_->setItem(i, 1, new QTableWidgetItem(REGISTER_NAMES[i]));
        
        // Register value
        int32_t value = registers[i];
        QTableWidgetItem* valueItem = new QTableWidgetItem(QString::number(value));
        
        // Highlight if recently changed
        if (recentChanges.contains(i) && recentChanges[i] == value) {
            valueItem->setBackground(QBrush(QColor(200, 255, 200)));
        }
        
        registerTable_->setItem(i, 2, valueItem);
    }
}

