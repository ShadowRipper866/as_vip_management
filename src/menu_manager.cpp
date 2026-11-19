#include "menu_manager.hpp"
#include "database_manager.hpp"
#include "globals.hpp"
#include "time_formatter.hpp"
#include "vip_management.hpp"
#include <exception>
#include <string>
#include <string_view>
#include <unordered_map>
#include <Helper.hpp>
#include "logger.hpp"

namespace menus
{
	namespace
	{

		inline std::string get_group(const Draft& draft)
		{
			const auto& actual_to_display = vip_admin.get_config().actual_to_display_group;
			auto it = actual_to_display.find(draft.selected_group);

			std::string display = draft.selected_group;
			if ( it != actual_to_display.end() )
				display = it->second;

			return std::string(admin_api->GetTranslation("VIPManagement_VIPGroup")) + ": " + display;
		}
		inline std::string get_time(const Draft& draft)
		{
			const auto& seconds_to_display = vip_admin.get_config().seconds_to_display_time;
			auto it = seconds_to_display.find(draft.selected_time);

			std::string display = std::to_string(draft.selected_time);
			if ( it != seconds_to_display.end() )
				display = it->second;

			return std::string(admin_api->GetTranslation("VIPManagement_VIPExpires")) + ": " + display;
		}
	} // namespace

	void open_vip_management(int slot)
	{
		Menu menu;
		menu.clear();
		menus_api->SetTitleMenu(menu, admin_api->GetTranslation("VIPManagement_SelectPlayer"));
		bool found = false;
		FOR_EACH_VALID_CONTROLLER(i, player)
		{
			if ( vip_api->VIP_IsClientVIP(i) )
				continue;

			menus_api->AddItemMenu(menu, std::to_string(i).c_str(), player->m_iszPlayerName);
			found = true;
		}
		if ( !found )
			menus_api->AddItemMenu(menu, "", admin_api->GetTranslation("CC_NoPlayers"), ITEM_DISABLED);
		menus_api->SetBackMenu(menu, true);
		menus_api->SetExitMenu(menu, true);
		menus_api->SetCallback(menu, [](const char* szBack, const char* szFront, int iItem, int slot)
		{
			if ( std::string_view(szBack) == "back" )
			{
				admin_api->ShowAdminCategoryMenu(slot, "vip_management");
				return;
			}
			if ( iItem < 7 )
			{
				try
				{
					int target = std::stoi(szBack);
					if ( !Utilities::IsPlayerValid(target) )
					{
						menus_api->ClosePlayerMenu(slot);
						return;
					}
					Draft draft = {
					    .target_slot = target,
					};
					open_vip_setting(slot, draft);
				}
				catch ( const std::exception& e )
				{
					logger::error("Faied to close player menu: %s | %s", e.what(), szBack);
				}
			}
		});
		menus_api->DisplayPlayerMenu(menu, slot, true, true);
	}

	void open_vip_time_selection(const int admin_slot, Draft draft)
	{
		Menu time_select;
		time_select.clear();
		menus_api->SetTitleMenu(time_select, admin_api->GetTranslation("VIPManagement_SelectTime"));

		for ( const auto& time : vip_admin.get_config().time_list )
			menus_api->AddItemMenu(time_select, std::to_string(time.second).c_str(), time.first.c_str());

		menus_api->SetBackMenu(time_select, true);
		menus_api->SetExitMenu(time_select, true);
		menus_api->SetCallback(time_select, [admin_slot, draft](const char* szBack, const char* szFront, int iItem, int iSlot) mutable
		{
			if ( std::string_view(szBack) == "back" )
			{
				open_vip_setting(admin_slot, draft);
				return;
			}
			if ( iItem < 7 )
			{
				try
				{
					draft.selected_time = std::stoi(szBack);
					open_vip_setting(admin_slot, draft);
				}
				catch ( const std::exception& e )
				{
					logger::error("Failed to select time: %s | %s", e.what(), szBack);
					return;
				}
			}
		});
		menus_api->DisplayPlayerMenu(time_select, admin_slot, true, true);
	}

	void open_vip_group_selection(const int admin_slot, Draft draft)
	{
		Menu groups_menu;
		groups_menu.clear();
		menus_api->SetTitleMenu(groups_menu, admin_api->GetTranslation("VIPManagement_SelectGroup"));
		for ( const auto& group : vip_admin.get_config().group_list )
			menus_api->AddItemMenu(groups_menu, group.second.c_str(), group.first.c_str());

		menus_api->SetBackMenu(groups_menu, true);
		menus_api->SetExitMenu(groups_menu, true);
		menus_api->SetCallback(groups_menu, [admin_slot, draft](const char* szBack, const char* szFront, int iItem, int iSlot) mutable
		{
			if ( std::string_view(szBack) == "back" )
			{
				open_vip_setting(admin_slot, draft);
				return;
			}
			if ( iItem < 7 )
			{
				logger::debug("Target VIP: %i", draft.target_slot);

				draft.selected_group = szBack;
				open_vip_setting(admin_slot, draft);
			}
		});
		menus_api->DisplayPlayerMenu(groups_menu, admin_slot, true, true);
	}

	void open_select_vip_player(int slot)
	{
		Menu menu;
		menu.clear();
		menus_api->SetTitleMenu(menu, admin_api->GetTranslation("VIPManagement_SelectVIPPlayer"));
		bool found = false;
		FOR_EACH_VALID_CONTROLLER(i, player)
		{
			if ( !vip_api->VIP_IsClientVIP(i) )
				continue;
			menus_api->AddItemMenu(menu, std::to_string(i).c_str(), player->m_iszPlayerName);
			found = true;
		}
		if ( !found )
			menus_api->AddItemMenu(menu, "", admin_api->GetTranslation("CC_NoPlayers"), ITEM_DISABLED);

		menus_api->SetBackMenu(menu, true);
		menus_api->SetExitMenu(menu, true);
		menus_api->SetCallback(menu, [](const char* szBack, const char* szFront, int iItem, int slot)
		{
			if ( std::string_view(szBack) == "back" )
			{
				admin_api->ShowAdminCategoryMenu(slot, "vip_management");
				return;
			}
			if ( iItem < 7 )
			{
				try
				{
					int target = std::stoi(szBack);
					change_menus::open_vip_change(slot, {.target_slot = target});
				}
				catch ( const std::exception& e )
				{
					logger::error("Failed to get target slot: %s | %s", e.what(), szBack);
					return;
				}
			}
		});
		menus_api->DisplayPlayerMenu(menu, slot, true, true);
	}

	void open_vip_setting(const int admin_slot, Draft draft)
	{
		Menu give_vip;
		give_vip.clear();
		menus_api->SetTitleMenu(give_vip, admin_api->GetTranslation("VIPManagement_GiveVIP"));

		std::string group;
		if ( !draft.selected_group.empty() )
			group = get_group(draft);
		else
			group = admin_api->GetTranslation("VIPManagement_SelectGroup");

		menus_api->AddItemMenu(give_vip, "groupSelection", group.c_str());

		std::string time;
		if ( draft.selected_time != -1 )
			time = get_time(draft);
		else
			time = admin_api->GetTranslation("VIPManagement_SelectTime");

		menus_api->AddItemMenu(give_vip, "timeSelection", time.c_str());

		if ( draft.is_ready() )
			menus_api->AddItemMenu(give_vip, "applyVip", admin_api->GetTranslation("VIPManagement_Apply"));

		menus_api->SetBackMenu(give_vip, true);
		menus_api->SetExitMenu(give_vip, true);
		menus_api->SetCallback(give_vip, [admin_slot, draft](const char* szBack, const char* szFront, int iItem, int iSlot)
		{
			if ( std::string_view(szBack) == "back" )
			{
				open_vip_management(admin_slot);
				return;
			}
			if ( iItem < 7 )
			{
				if ( std::string_view(szBack) == "groupSelection" )
					open_vip_group_selection(admin_slot, draft);
				else if ( std::string_view(szBack) == "timeSelection" )
					open_vip_time_selection(admin_slot, draft);
				else if ( std::string_view(szBack) == "applyVip" && draft.is_ready() )
				{
					vip_api->VIP_GiveClientVIP(draft.target_slot, draft.selected_time, draft.selected_group.c_str(), true);
					open_vip_management(admin_slot);
				}
			}
		});

		menus_api->DisplayPlayerMenu(give_vip, admin_slot, true, true);
	}

	namespace change_menus
	{
		void open_vip_change(const int admin_slot, Draft draft)
		{
			bool is_vip = vip_api->VIP_IsClientVIP(draft.target_slot);
			if ( !is_vip )
			{
				open_vip_setting(admin_slot, draft);
				return;
			}
			const char* group = vip_api->VIP_GetClientVIPGroup(draft.target_slot);
			if ( draft.selected_group.empty() )
				draft.selected_group = group;

			long long time = vip_api->VIP_GetClientAccessTime(draft.target_slot);
			long long calculate_time = time - std::time(nullptr);

			Menu change_vip_menu;
			change_vip_menu.clear();
			menus_api->SetTitleMenu(change_vip_menu, admin_api->GetTranslation("VIPManagement_ChangeVIPPlayer"));

			std::string group_item = get_group(draft);
			menus_api->AddItemMenu(change_vip_menu, "vipGroup", group_item.c_str());

			std::string expires_item = std::string(admin_api->GetTranslation("VIPManagement_VIPExpires")) + ": " + (time == 0 ? admin_api->GetTranslation("Never") : std::to_string(calculate_time));
			menus_api->AddItemMenu(change_vip_menu, "vipExpires", expires_item.c_str());

			std::string expires_disabled_item = std::string(admin_api->GetTranslation("VIPManagement_VIPExpiresDisabled")) + ": " + datetime::unix_time_to_readable_date(time);
			menus_api->AddItemMenu(change_vip_menu, "vipExpires", expires_disabled_item.c_str(), ITEM_DISABLED);

			menus_api->AddItemMenu(change_vip_menu, "deleteVip", admin_api->GetTranslation("VIPManagement_DeleteVIP"));

			menus_api->SetBackMenu(change_vip_menu, true);
			menus_api->SetExitMenu(change_vip_menu, true);

			menus_api->SetCallback(change_vip_menu, [admin_slot, draft, time](const char* szBack, const char* szFront, int iItem, int adminSlot)
			{
				if ( std::string_view(szBack) == "back" )
				{
					open_select_vip_player(adminSlot);
					return;
				}
				if ( iItem < 7 )
				{
					if ( std::string_view(szBack) == "deleteVip" )
					{
						vip_api->VIP_RemoveClientVIP(draft.target_slot, true);
						open_select_vip_player(adminSlot);
					}
					else if ( std::string_view(szBack) == "vipExpires" )
						change_menus::open_vip_time_change(admin_slot, draft);
					else if ( std::string_view(szBack) == "vipGroup" )
						change_menus::open_vip_group_change(admin_slot, draft);
				}
			});
			menus_api->DisplayPlayerMenu(change_vip_menu, admin_slot, true, true);
		}

		void open_vip_time_change_mode(const int admin_slot, Draft draft, long long time)
		{
			Menu change_mode;
			change_mode.clear();

			menus_api->SetTitleMenu(change_mode, admin_api->GetTranslation("VIPManagement_SelectChangeMode"));
			menus_api->AddItemMenu(change_mode, "replaceTime", admin_api->GetTranslation("VIPManagement_ReplaceMode"));
			menus_api->AddItemMenu(change_mode, "addTime", admin_api->GetTranslation("VIPManagement_AddMode"));

			menus_api->SetBackMenu(change_mode, true);
			menus_api->SetExitMenu(change_mode, true);

			menus_api->SetCallback(change_mode, [admin_slot, draft, time](const char* szBack, const char* szFront, int iItem, int iSlot)
			{
				if ( std::string_view(szBack) == "back" )
				{
					open_vip_time_change(admin_slot, draft);
					return;
				}
				if ( iItem < 7 )
				{
					if ( std::string_view(szBack) == "replaceTime" )
					{
						long long time_access = std::time(0) + time;
						vip_api->VIP_SetClientAccessTime(draft.target_slot, time_access);
						open_vip_change(admin_slot, draft);
					}
					else if ( std::string_view(szBack) == "addTime" )
					{
						long long time_access = time == 0 ? 0 : vip_api->VIP_GetClientAccessTime(draft.target_slot) + time;
						vip_api->VIP_SetClientAccessTime(draft.target_slot, time_access);
						open_vip_change(admin_slot, draft);
					}
				}
			});
			menus_api->DisplayPlayerMenu(change_mode, admin_slot, true, true);
		}

		void open_vip_time_change(const int admin_slot, Draft draft)
		{
			Menu times_menu;
			times_menu.clear();
			menus_api->SetTitleMenu(times_menu, admin_api->GetTranslation("VIPManagement_SelectTime"));
			for ( const auto& time : vip_admin.get_config().time_list )
				menus_api->AddItemMenu(times_menu, std::to_string(time.second).c_str(), time.first.c_str());

			menus_api->SetBackMenu(times_menu, true);
			menus_api->SetExitMenu(times_menu, true);
			menus_api->SetCallback(times_menu, [admin_slot, draft](const char* szBack, const char* szFront, int iItem, int iSlot)
			{
				if ( std::string_view(szBack) == "back" )
				{
					open_vip_change(admin_slot, draft);
					return;
				}
				if ( iItem < 7 )
				{
					try
					{
						long long selected_vip_time = std::stoll(szBack);
						long long current_target_time = vip_api->VIP_GetClientAccessTime(draft.target_slot);
						if ( selected_vip_time == 0 )
						{
							vip_api->VIP_SetClientAccessTime(draft.target_slot, 0, true);
							open_vip_change(admin_slot, draft);
							return;
						}
						if ( current_target_time == 0 )
						{
							long long new_expires_time = std::time(0) + selected_vip_time;
							vip_api->VIP_SetClientAccessTime(draft.target_slot, new_expires_time, true);
							open_vip_change(admin_slot, draft);
							return;
						}
						open_vip_time_change_mode(admin_slot, draft, selected_vip_time);
					}
					catch ( const std::exception& e )
					{
						logger::error("Failed to change vip time: %s | %s", e.what(), szBack);
					}
				}
			});
			menus_api->DisplayPlayerMenu(times_menu, admin_slot, true, true);
		}
		void open_vip_group_change(const int admin_slot, Draft draft)
		{
			Menu groups_menu;
			groups_menu.clear();
			menus_api->SetTitleMenu(groups_menu, admin_api->GetTranslation("VIPManagement_SelectGroup"));
			for ( const auto& group : vip_admin.get_config().group_list )
				menus_api->AddItemMenu(groups_menu, group.second.c_str(), group.first.c_str());

			menus_api->SetBackMenu(groups_menu, true);
			menus_api->SetExitMenu(groups_menu, true);
			menus_api->SetCallback(groups_menu, [admin_slot, draft](const char* szBack, const char* szFront, int iItem, int iSlot) mutable
			{
				if ( std::string_view(szBack) == "back" )
				{
					change_menus::open_vip_change(admin_slot, draft);
					return;
				}
				if ( iItem < 7 )
				{
					logger::debug("Target VIP: %i", draft.target_slot);
					database::change_vip_group_via_database(draft.target_slot, szBack);

					uint64 acc = players_api->GetSteamID64(draft.target_slot);
					std::string reload_cmd = "mm_reload_vip " + std::to_string(acc);
					engine->ServerCommand(reload_cmd.c_str());
					draft.selected_group = szBack;

					utils->CreateTimer(0.3F, [admin_slot, draft]() -> float
					{
						change_menus::open_vip_change(admin_slot, draft);
						return -1.0F;
					});
				}
			});
			menus_api->DisplayPlayerMenu(groups_menu, admin_slot, true, true);
		}
	} // namespace change_menus

} // namespace menus