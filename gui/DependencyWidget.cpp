#include "DependencyWidget.h"
#include <QHeaderView>
#include <QColor>
#include <QBrush>
#include <QTableWidgetItem>

namespace {
constexpr int kMaxDependencyRows = 400;
}

DependencyWidget::DependencyWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void DependencyWidget::setupUI() {
    layout_ = new QVBoxLayout(this);
    
    titleLabel_ = new QLabel(
        "<h3>Instruction Dependencies</h3>"
        "<p style='font-size:11px;color:#444;'>RAW hazards only, while producer and consumer "
        "can still occupy the pipeline at the same time (consumer in ID while producer "
        "has not finished WB). Older, non-overlapping pairs are omitted.</p>",
        this);
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
    const int startRow = qMax(0, static_cast<int>(dependencies.size()) - kMaxDependencyRows);
    const int rowCount = static_cast<int>(dependencies.size()) - startRow;

    dependencyTable_->setRowCount(rowCount);
    
    for (int i = 0; i < rowCount; ++i) {
        const auto& dep = dependencies[static_cast<size_t>(startRow + i)];
        const int row = i;
        
        QTableWidgetItem* typeItem = new QTableWidgetItem(QString::fromStdString(dep.dependency_type));
        QColor typeColor;
        if (dep.dependency_type == "RAW") {
            typeColor = QColor(255, 200, 200);
        } else if (dep.dependency_type == "WAR") {
            typeColor = QColor(255, 255, 200);
        } else {
            typeColor = QColor(200, 255, 255);
        }
        typeItem->setBackground(QBrush(typeColor));
        dependencyTable_->setItem(row, 0, typeItem);
        
        dependencyTable_->setItem(row, 1, new QTableWidgetItem(QString("x%1").arg(dep.register_num)));
        
        QString producer = QString::fromStdString(dep.producer_disassembly);
        if (producer.isEmpty()) producer = "N/A";
        QString producerText = QString("0x%1: %2").arg(dep.producer_pc, 0, 16).arg(producer);
        dependencyTable_->setItem(row, 2, new QTableWidgetItem(producerText));
        
        QString consumer = QString::fromStdString(dep.consumer_disassembly);
        if (consumer.isEmpty()) consumer = "N/A";
        QString consumerText = QString("0x%1: %2").arg(dep.consumer_pc, 0, 16).arg(consumer);
        dependencyTable_->setItem(row, 3, new QTableWidgetItem(consumerText));
        
        dependencyTable_->setItem(row, 4, new QTableWidgetItem(producer));
        dependencyTable_->setItem(row, 5, new QTableWidgetItem(consumer));
    }

    if (rowCount > 0) {
        dependencyTable_->scrollToBottom();
    }
}
