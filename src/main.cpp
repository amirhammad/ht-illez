#include "main.h"
#include "MainWindow.h"

#include <QApplication>
#include <chrono>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// Initialize random number generator
	std::srand(std::time(0));

	new iez::MainWindow();

	return QApplication::exec();
}
