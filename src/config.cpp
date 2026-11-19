#include "config.hpp"
#include "interfaces/interfaces.h"
#include "keyvalues.h"
#include "logger.hpp"
#include <memory>

void Config::load(const char* path)
{
	auto config = std::make_unique<KeyValues>("Config");
	if ( !config->LoadFromFile(g_pFullFileSystem, path) )
	{
		logger::error("Config file not found");
		return;
	}
	std::unique_ptr<KeyValues> vip = std::make_unique<KeyValues>("VIP");
	const char* path_vip = "addons/configs/vip/groups.ini";
	if ( !vip->LoadFromFile(g_pFullFileSystem, path_vip) )
	{
		logger::error("VIP config file not found");
		return;
	}

	server_id = vip->GetInt("server_id");

	KeyValues* times = config->FindKey("times");
	if ( times )
	{
		FOR_EACH_SUBKEY(times, sub)
		{
			const char* time_name = sub->GetName();
			uint64 time_sec = sub->GetUint64(nullptr);
			if ( time_name )
			{
				time_list.emplace_back(time_name, time_sec);
				seconds_to_display_time[time_sec] = time_name;
			}
		}
	}
	else
		logger::info("WARNING: Section 'times' not found in config!");

	KeyValues* kGroups = config->FindKey("groups");
	if ( kGroups )
	{
		logger::debug("Reading 'groups' section...\n");

		for ( KeyValues* sub = kGroups->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
		{
			const char* display = sub->GetName();
			const char* actual = sub->GetString(nullptr, display);
			if ( display )
			{
				group_list.emplace_back(display, actual);
				actual_to_display_group[actual] = display;

				logger::debug("Loaded Group: %s -> %s\n", display, actual);
			}
		}
	}
	else
		logger::info("WARNING: Section 'groups' not found in config!");

	access_flag = config->GetString("accessFlag", "@admin/root");
	_debug = config->GetBool("debug", false);
}