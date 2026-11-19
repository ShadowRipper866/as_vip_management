#include "time_formatter.hpp"
#include "globals.hpp"

namespace datetime
{
	std::string unix_time_to_readable_date(long long unix_time)
	{
		time_t time = unix_time;
		tm* ptm = localtime(&time);
		if ( !ptm )
			return "Invalid time";
		char szBuffer[32];
		if ( unix_time != 0 )
			strftime(szBuffer, sizeof(szBuffer), "%H:%M:%S (%d.%m.%Y)", ptm);
		else
			return admin_api->GetTranslation("Never");
		return std::string(szBuffer);
	}

} // namespace datetime