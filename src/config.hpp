#pragma once

#include "steamtypes.h"
#include <unordered_map>
#include <vector>

struct Config
{
	std::unordered_map<uint64, std::string> seconds_to_display_time;
	std::vector<std::pair<std::string, uint64>> time_list;
	std::unordered_map<std::string, std::string> actual_to_display_group;
	// название в меню - факт. название
	std::vector<std::pair<std::string, std::string>> group_list;
	int server_id = -1;
	std::string access_flag;
	// unordered_map<int, PendingVip> pending_actions;
	void load(const char* path);
};