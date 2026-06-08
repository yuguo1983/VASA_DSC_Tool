#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("DSC 1.2 Codec");
    a.setApplicationDisplayName("DSC 1.2 Reference Codec GUI");
    MainWindow w;
    w.show();
    return a.exec();
}
