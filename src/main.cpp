#include <QApplication>
#include <QDebug>
#include "core/CueManager.h"
#include "core/ErrorHandler.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("CueForge");
    app.setOrganizationDomain("cueforge.com");
    app.setApplicationName("CueForge");
    app.setApplicationVersion("2.0.0");

    qDebug() << "========================================";
    qDebug() << "CueForge Qt6 v2.0.0";
    qDebug() << "========================================";
    qDebug() << "";

    // Create core systems
    CueForge::ErrorHandler errorHandler;
    CueForge::CueManager cueManager;

    qDebug() << "✓ Error handler initialized";
    qDebug() << "✓ Cue manager initialized";

    // Create and show main window
    CueForge::MainWindow mainWindow(&cueManager, &errorHandler);
    mainWindow.show();

    qDebug() << "✓ Main window displayed";
    qDebug() << "";
    qDebug() << "CueForge ready!";
    qDebug() << "";

    return app.exec();
}