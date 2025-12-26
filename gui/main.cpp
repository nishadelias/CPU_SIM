#include "MainWindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("RISC-V CPU Simulator");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("CPU_SIM");
    
    // Use a modern style if available
    app.setStyle(QStyleFactory::create("Fusion"));
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

