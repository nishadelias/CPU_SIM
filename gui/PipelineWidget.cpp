#include "PipelineWidget.h"
#include <QHeaderView>
#include <QColor>
#include <QBrush>
#include <QTableWidgetItem>

const QStringList PipelineWidget::COLUMN_HEADERS = {
    "Cycle", "IF/ID", "ID/EX", "EX/MEM", "MEM/WB", "Stall", "Flush"
};

PipelineWidget::PipelineWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PipelineWidget::setupUI() {
    layout_ = new QVBoxLayout(this);
    
    titleLabel_ = new QLabel("<h2>Pipeline Execution Trace</h2>", this);
    layout_->addWidget(titleLabel_);
    
    pipelineTable_ = new QTableWidget(this);
    pipelineTable_->setColumnCount(COLUMN_HEADERS.size());
    pipelineTable_->setHorizontalHeaderLabels(COLUMN_HEADERS);
    pipelineTable_->setAlternatingRowColors(true);
    pipelineTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    pipelineTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pipelineTable_->horizontalHeader()->setStretchLastSection(true);
    pipelineTable_->setColumnWidth(0, 60);  // Cycle
    pipelineTable_->setColumnWidth(1, 200); // IF/ID
    pipelineTable_->setColumnWidth(2, 200); // ID/EX
    pipelineTable_->setColumnWidth(3, 200); // EX/MEM
    pipelineTable_->setColumnWidth(4, 200); // MEM/WB
    pipelineTable_->setColumnWidth(5, 60);  // Stall
    pipelineTable_->setColumnWidth(6, 60);  // Flush
    
    layout_->addWidget(pipelineTable_);
}

void PipelineWidget::updateDisplay(CPU* cpu) {
    if (!cpu) return;
    
    updatePipelineTable(cpu);
}

void PipelineWidget::updatePipelineTable(CPU* cpu) {
    const auto& trace = cpu->get_pipeline_trace();
    
    pipelineTable_->setRowCount(trace.size());
    
    for (size_t i = 0; i < trace.size(); ++i) {
        const auto& snapshot = trace[i];
        int row = static_cast<int>(i);
        
        // Cycle
        pipelineTable_->setItem(row, 0, new QTableWidgetItem(QString::number(snapshot.cycle)));
        
        // IF/ID
        QString ifIdText = snapshot.if_id.valid 
            ? QString("PC: 0x%1\n%2")
                .arg(snapshot.if_id.pc, 0, 16)
                .arg(QString::fromStdString(snapshot.if_id.disassembly))
            : "Empty";
        QTableWidgetItem* ifIdItem = new QTableWidgetItem(ifIdText);
        ifIdItem->setBackground(snapshot.if_id.valid ? QBrush(QColor(200, 255, 200)) : QBrush(QColor(240, 240, 240)));
        pipelineTable_->setItem(row, 1, ifIdItem);
        
        // ID/EX
        QString idExText = snapshot.id_ex.valid
            ? QString("PC: 0x%1\n%2")
                .arg(snapshot.id_ex.pc, 0, 16)
                .arg(QString::fromStdString(snapshot.id_ex.disassembly))
            : "Empty";
        QTableWidgetItem* idExItem = new QTableWidgetItem(idExText);
        idExItem->setBackground(snapshot.id_ex.valid ? QBrush(QColor(200, 255, 200)) : QBrush(QColor(240, 240, 240)));
        pipelineTable_->setItem(row, 2, idExItem);
        
        // EX/MEM
        QString exMemText = snapshot.ex_mem.valid
            ? QString("PC: 0x%1\n%2\nALU: %3")
                .arg(snapshot.ex_mem.pc, 0, 16)
                .arg(QString::fromStdString(snapshot.ex_mem.disassembly))
                .arg(snapshot.ex_mem.alu_result)
            : "Empty";
        QTableWidgetItem* exMemItem = new QTableWidgetItem(exMemText);
        exMemItem->setBackground(snapshot.ex_mem.valid ? QBrush(QColor(200, 255, 200)) : QBrush(QColor(240, 240, 240)));
        pipelineTable_->setItem(row, 3, exMemItem);
        
        // MEM/WB
        QString memWbText = snapshot.mem_wb.valid
            ? QString("PC: 0x%1\n%2\nWrite: %3")
                .arg(snapshot.mem_wb.pc, 0, 16)
                .arg(QString::fromStdString(snapshot.mem_wb.disassembly))
                .arg(snapshot.mem_wb.write_data)
            : "Empty";
        QTableWidgetItem* memWbItem = new QTableWidgetItem(memWbText);
        memWbItem->setBackground(snapshot.mem_wb.valid ? QBrush(QColor(200, 255, 200)) : QBrush(QColor(240, 240, 240)));
        pipelineTable_->setItem(row, 4, memWbItem);
        
        // Stall
        QTableWidgetItem* stallItem = new QTableWidgetItem(snapshot.stall ? "Yes" : "No");
        stallItem->setBackground(snapshot.stall ? QBrush(QColor(255, 200, 200)) : QBrush(QColor(255, 255, 255)));
        pipelineTable_->setItem(row, 5, stallItem);
        
        // Flush
        QTableWidgetItem* flushItem = new QTableWidgetItem(snapshot.flush ? "Yes" : "No");
        flushItem->setBackground(snapshot.flush ? QBrush(QColor(255, 255, 200)) : QBrush(QColor(255, 255, 255)));
        pipelineTable_->setItem(row, 6, flushItem);
    }
    
    // Scroll to bottom to show latest cycle
    if (!trace.empty()) {
        pipelineTable_->scrollToBottom();
    }
}

QString PipelineWidget::formatInstruction(const PipelineSnapshot& snapshot, const QString& stageName) {
    // Helper method for formatting (can be expanded)
    return stageName;
}

