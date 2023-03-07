#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationName("Serial Reader");
	a.setWindowIcon(QIcon(":/images/serial.png"));
	MainWindow w;
	w.setWindowTitle(QApplication::applicationName());
	w.show();
	return a.exec();
}
