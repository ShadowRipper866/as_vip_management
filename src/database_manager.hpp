#pragma once

namespace database
{
	void load_database();
	void change_vip_group_via_database(const int slot, const char* group);
} // namespace database