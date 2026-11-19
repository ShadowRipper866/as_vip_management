#include "logger.hpp"

namespace logger
{
	void info(const char* msg, ...)
	{
		char buf[1024];
		va_list va;
		va_start(va, msg);
		vsnprintf(buf, sizeof(buf), msg, va);
		va_end(va);
		Msg("%s %s\n", TAG, buf);
	}

	void error(const char* msg, ...)
	{
		char buf[1024];
		va_list va;
		va_start(va, msg);
		vsnprintf(buf, sizeof(buf), msg, va);
		va_end(va);
		utils->ErrorLog("%s %s\n", TAG, buf);
	}

	void debug(const char* msg, ...)
	{
		if ( _debug )
		{
			char buf[1024];
			va_list va;
			va_start(va, msg);
			vsnprintf(buf, sizeof(buf), msg, va);
			va_end(va);
			Msg("[DEBUG] %s %s\n", TAG, buf);
		}
	}
} // namespace logger