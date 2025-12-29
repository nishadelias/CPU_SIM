#include "DependencyWidget.h"
#include <QHeaderView>
#include <QColor>
#include <QBrush>
#include <QTableWidgetItem>

DependencyWidget::DependencyWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void DependencyWidget::setupUI() {
    layout_ = new QVBoxLayout(this);
    
    titleLabel_ = new QLabel("<h3>Instruction Dependencies</h3>", this);
    layout_->addWidget(titleLabel_);
    
    dependencyTable_ = new QTableWidget(this);
    dependencyTable_->setColumnCount(6);
    dependencyTable_->setHorizontalHeaderLabels({"Type", "Register", "Producer", "Consumer", "Producer Inst", "Consumer Inst"});
    dependencyTable_->setAlternatingRowColors(true);
    dependencyTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    dependencyTable_->horizontalHeader()->setStretchLastSection(true);
    dependencyTable_->setColumnWidth(0, 60);
    dependencyTable_->setColumnWidth(1, 60);
    dependencyTable_->setColumnWidth(2, 100);
    dependencyTable_->setColumnWidth(3, 100);
    
    layout_->addWidget(dependencyTable_);
}

void DependencyWidget::updateDisplay(CPU* cpu) {
    if (!cpu) return;
    
    updateDependencyTable(cpu);
}

void DependencyWidget::updateDependencyTable(CPU* cpu) {
    const auto& dependencies = cpu->get_instruction_dependencies();
    
    dependencyTable_->setRowCount(static_cast<int>(dependencies.size()));
    
    for (size_t i = 0; i < dependencies.size(); ++i) {
        const auto& dep = dependencies[i];
        int row = static_cast<int>(i);
        
        // Type
        QTableWidgetItem* typeItem = new QTableWidgetItem(QString::fromStdString(dep.dependency_type));
        QColor typeColor;
        if (dep.dependency_type == "RAW") {
            typeColor = QColor(255, 200, 200);  // Red for RAW (most critical)
        } else if (dep.dependency_type == "WAR") {
            typeColor = QColor(255, 255, 200);  // Yellow for WAR
        } else {
            typeColor = QColor(200, 255, 255);  // Cyan for WAW
        }
        typeItem->setBackground(QBrush(typeColor));
        dependencyTable_->setItem(row, 0, typeItem);
        
        // Register
        dependencyTable_->setItem(row, 1, new QTableWidgetItem(QString("x%1").arg(dep.register_num)));
        
        // Producer PC with instruction
        QString producer = QString::fromStdString(dep.producer_disassembly);
        if (producer.isEmpty()) producer = "N/A";
        QString producerText = QString("0x%1: %2").arg(dep.producer_pc, 0, 16).arg(producer);
        dependencyTable_->setItem(row, 2, new QTableWidgetItem(producerText));
        
        // Consumer PC with instruction
        QString consumer = QString::fromStdString(dep.consumer_disassembly);
        if (consumer.isEmpty()) consumer = "N/A";
        QString consumerText = QString("0x%1: %2").arg(dep.consumer_pc, 0, 16).arg(consumer);
        dependencyTable_->setItem(row, 3, new QTableWidgetItem(consumerText));
        
        // Producer instruction (for sorting/filtering)
        dependencyTable_->setItem(row, 4, new QTableWidgetItem(producer));
        
        // Consumer instruction (for sorting/filtering)
        dependencyTable_->setItem(row, 5, new QTableWidgetItem(consumer));
    }
}

