#include "main.h"
#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	new iez::MainWindow();

	return QApplication::exec();
}
