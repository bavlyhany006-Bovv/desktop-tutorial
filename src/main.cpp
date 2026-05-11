// ============================================================
//  main.cpp
//  Application entry point.
// ============================================================
#include <QApplication>
#include <QFont>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // HiDPI support
    app.setAttribute(Qt::AA_EnableHighDpiScaling);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Application metadata
    app.setApplicationName("Smart 8-Bit Arithmetic Hardware Solver");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Innovation University");

    // Launch the main window
    MainWindow window;
    window.show();

    return app.exec();
}
