#include <QApplication>
#include <QDebug>
#include "core/CueManager.h"
#include "core/ErrorHandler.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("CueForge");
    app.setApplicationName("CueForge");
    app.setApplicationVersion("2.0.0");

    qDebug() << "CueForge Qt6 starting...";

    CueForge::ErrorHandler errorHandler;
    CueForge::CueManager cueManager;

    qDebug() << "Core systems initialized";
    qDebug() << "Press Ctrl+C to exit (UI not yet implemented)";

    return app.exec();
}