#include "MemoryWidget.h"
#include <QHeaderView>
#include <QColor>
#include <QBrush>
#include <QTableWidgetItem>
#include <algorithm>

MemoryWidget::MemoryWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void MemoryWidget::setupUI() {
    layout_ = new QVBoxLayout(this);
    
    titleLabel_ = new QLabel("<h3>Memory Access History</h3>", this);
    layout_->addWidget(titleLabel_);
    
    memoryTable_ = new QTableWidget(this);
    memoryTable_->setColumnCount(6);
    memoryTable_->setHorizontalHeaderLabels({"Cycle", "Address", "Type", "Value", "Cache", "Instruction"});
    memoryTable_->setAlternatingRowColors(true);
    memoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoryTable_->horizontalHeader()->setStretchLastSection(true);
    memoryTable_->setColumnWidth(0, 60);
    memoryTable_->setColumnWidth(1, 100);
    memoryTable_->setColumnWidth(2, 60);
    memoryTable_->setColumnWidth(3, 100);
    memoryTable_->setColumnWidth(4, 60);
    
    layout_->addWidget(memoryTable_);
}

void MemoryWidget::updateDisplay(CPU* cpu) {
    if (!cpu) return;
    
    updateMemoryTable(cpu);
}

void MemoryWidget::updateMemoryTable(CPU* cpu) {
    const auto& memHistory = cpu->get_memory_access_history();
    
    // Show last 100 accesses
    int startRow = qMax(0, static_cast<int>(memHistory.size()) - 100);
    int rowCount = static_cast<int>(memHistory.size()) - startRow;
    
    memoryTable_->setRowCount(rowCount);
    
    for (int i = 0; i < rowCount; ++i) {
        const auto& access = memHistory[startRow + i];
        
        // Cycle
        memoryTable_->setItem(i, 0, new QTableWidgetItem(QString::number(access.cycle)));
        
        // Address
        memoryTable_->setItem(i, 1, new QTableWidgetItem(QString("0x%1").arg(access.address, 0, 16)));
        
        // Type
        QTableWidgetItem* typeItem = new QTableWidgetItem(access.is_write ? "Write" : "Read");
        typeItem->setBackground(access.is_write ? QBrush(QColor(255, 200, 200)) : QBrush(QColor(200, 200, 255)));
        memoryTable_->setItem(i, 2, typeItem);
        
        // Value
        memoryTable_->setItem(i, 3, new QTableWidgetItem(QString::number(access.value)));
        
        // Cache hit/miss
        QTableWidgetItem* cacheItem = new QTableWidgetItem(access.cache_hit ? "Hit" : "Miss");
        cacheItem->setBackground(access.cache_hit ? QBrush(QColor(200, 255, 200)) : QBrush(QColor(255, 200, 200)));
        memoryTable_->setItem(i, 4, cacheItem);
        
        // Instruction
        QString instruction = QString::fromStdString(access.instruction_disassembly);
        if (instruction.isEmpty()) {
            instruction = access.is_write ? "STORE" : "LOAD";
        }
        memoryTable_->setItem(i, 5, new QTableWidgetItem(instruction));
    }
    
    // Scroll to bottom
    if (rowCount > 0) {
        memoryTable_->scrollToBottom();
    }
}

