#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("System Monitor");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("SystemMonitor");

    MainWindow window;
    window.show();

    return app.exec();
}
