#pragma once

#include <string>

namespace menus
{
	struct Draft
	{
		int target_slot = -1;
		std::string selected_group;
		int selected_time = -1;

		bool is_ready() const { return !selected_group.empty() && selected_time != -1 && target_slot != -1; }
	};

	void open_vip_management(int slot);

	void open_vip_group_selection(const int admin_slot, Draft draft);
	void open_vip_time_selection(const int admin_slot, Draft draft);

	void open_select_player(int slot);
	void open_select_vip_player(int slot);

	void open_give_vip(const int admin_slot, Draft draft);
	void open_vip_setting(const int admin_slot, Draft draft);

	namespace change_menus
	{
		void open_vip_change(const int admin_slot, Draft draft);
		void open_vip_time_change(const int admin_slot, Draft draft);
		void open_vip_group_change(const int admin_slot, Draft draft);
		void open_vip_time_change_mode(const int admin_slot, Draft draft, long long time);
	} // namespace change_menus

} // namespace menus