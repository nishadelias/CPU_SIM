#include "RegisterWidget.h"
#include <QHeaderView>
#include <QColor>
#include <QBrush>
#include <QMap>
#include <QTableWidgetItem>

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

    cpsrLabel_ = new QLabel(this);
    cpsrLabel_->setVisible(false);
    layout_->addWidget(cpsrLabel_);
}

void RegisterWidget::updateDisplay(CPU* cpu) {
    if (!cpu) return;
    
    updateRegisterTable(cpu);
}

void RegisterWidget::updateRegisterTable(CPU* cpu) {
    const int32_t* registers = cpu->get_all_registers();
    const auto& regHistory = cpu->get_register_history();
    const int n = cpu->gpr_count();
    
    QMap<int, int32_t> recentChanges;
    uint64_t currentCycle = cpu->get_statistics().total_cycles;
    for (const auto& change : regHistory) {
        if (static_cast<uint64_t>(change.cycle) == currentCycle) {
            recentChanges[change.register_num] = change.new_value;
        }
    }
    
    registerTable_->setRowCount(n);
    
    for (int i = 0; i < n; ++i) {
        const char* abi = cpu->isa().abi_name(i);
        if (cpu->get_isa() == IsaKind::Aarch32) {
            registerTable_->setItem(i, 0, new QTableWidgetItem(QString("r%1").arg(i)));
        } else {
            registerTable_->setItem(i, 0, new QTableWidgetItem(QString("x%1").arg(i)));
        }
        registerTable_->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(abi)));
        
        int32_t value = registers[i];
        QTableWidgetItem* valueItem = new QTableWidgetItem(QString("0x%1 (%2)")
            .arg(static_cast<uint32_t>(value), 8, 16, QLatin1Char('0'))
            .arg(value));
        
        if (recentChanges.contains(i) && recentChanges[i] == value) {
            valueItem->setBackground(QBrush(QColor(200, 255, 200)));
        }
        
        registerTable_->setItem(i, 2, valueItem);
    }

    if (cpu->get_isa() == IsaKind::Aarch32) {
        ArmCpsr c = cpu->get_cpsr();
        cpsrLabel_->setText(QStringLiteral("CPSR flags: N=%1 Z=%2 C=%3 V=%4")
                                .arg(c.n ? 1 : 0)
                                .arg(c.z ? 1 : 0)
                                .arg(c.c ? 1 : 0)
                                .arg(c.v ? 1 : 0));
        cpsrLabel_->setVisible(true);
        titleLabel_->setText(QStringLiteral("<h3>Register File (AArch32)</h3>"));
    } else {
        cpsrLabel_->setVisible(false);
        titleLabel_->setText(QStringLiteral("<h3>Register File (RV32)</h3>"));
    }
}
