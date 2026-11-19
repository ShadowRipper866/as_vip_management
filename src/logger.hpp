#pragma once

#include "dbg.h"
#include "globals.hpp"
#include <cstdarg>
#include <cstdio>

static constexpr const char* TAG = "[VIP_ADMIN]";

namespace logger
{
	void info(const char* msg, ...);

	void error(const char* msg, ...);

	void debug(const char* msg, ...);
} // namespace logger