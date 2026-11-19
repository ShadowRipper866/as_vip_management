#include "database_manager.hpp"
#include "CCSPlayerController.h"
#include "globals.hpp"
#include "logger.hpp"
#include "include/mysql_mm.h"
#include "vip_management.hpp"
#include <Helper.hpp>

static constexpr uint64 ACCOUNT_ID_OFFSET = 76561197960265728;

namespace database
{
	void load_database()
	{
		if ( connection != nullptr )
			return;

		auto databases = std::make_shared<KeyValues>("Databases");
		if ( !databases->LoadFromFile(g_pFullFileSystem, "addons/configs/databases.cfg") )
		{
			logger::error("Failed to load databases.cfg");
			return;
		}
		KeyValues* database = databases->FindKey("vip", false);
		if ( !database )
		{
			logger::error("No `vip` field in databases.cfg. You doesn`t have VIP core installed?");
			return;
		}

		MySQLConnectionInfo info;
		info.host = database->GetString("host", "");
		info.user = database->GetString("user", "");
		info.pass = database->GetString("pass", "");
		info.database = database->GetString("database", "");
		info.port = database->GetInt("port", 3306);
		connection = mysql_client->CreateMySQLConnection(info);
		// какой дебыл выводит данные от бд в консоль (я)
		connection->Connect([databases](bool connect)
		{
			if ( !connect )
			{
				logger::error("Failed to connect to MySQL");
				connection = nullptr;
				return;
			}
			logger::debug("Database connection created successfully");
		});
	}
	void change_vip_group_via_database(const int slot, const char* group)
	{
		const int server_id = vip_admin.get_config().server_id;
		if ( CCSPlayerController* player = Utilities::GetController(slot) )
		{
			char buf[1024];
			uint64 account_id = player->m_steamID.Get() - ACCOUNT_ID_OFFSET;
			snprintf(buf, sizeof(buf), "UPDATE `vip_users` SET `group` = '%s' WHERE `account_id` = %llu AND `sid` = %d",
			    connection->Escape(group).c_str(), account_id, server_id);
			logger::debug(buf);
			connection->Query(buf, [](ISQLQuery* query)
			{
			});
		}
	}
} // namespace database