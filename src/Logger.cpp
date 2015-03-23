#include "Logger.h"

DBG_MODULE_INIT("Logger");
namespace iez {

void Logger::init()
{
	using namespace log4cplus;
	initialize();
	log4cplus::BasicConfigurator config;
	config.configure();
	DBG_INFO("Logger configured");
}

void Logger::destroy()
{
	log4cplus::threadCleanup();
}


}
