#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setWindowIcon(QIcon(":/antenna_log_trasparente.png"));

    a.setApplicationName("RFLog");
    a.setOrganizationName("");
    a.setOrganizationDomain("");

    MainWindow w;
    w.show();
    return a.exec();
}
