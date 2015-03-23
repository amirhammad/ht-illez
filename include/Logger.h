#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
namespace iez {

#define DBG_MODULE_INIT(name) static log4cplus::Logger loggerInstance =\
	log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(name))
#define DBG_FATAL(arg) LOG4CPLUS_FATAL(loggerInstance, arg)
#define DBG_TRACE(arg) LOG4CPLUS_TRACE(loggerInstance, arg)
#define DBG_DEBUG(arg) LOG4CPLUS_DEBUG(loggerInstance, arg)
#define DBG_WARN(arg) LOG4CPLUS_WARN(loggerInstance, arg)
#define DBG_INFO(arg) LOG4CPLUS_INFO(loggerInstance, arg)

class Logger
{
public:
	static void init();
	static void destroy();
private:
	Logger();
};

}
