#include "vip_management.hpp"
#include "database_manager.hpp"
#include "entitysystem.h"
#include "globals.hpp"
#include "include/menus.h"
#include "include/utils.h"
#include "menu_manager.hpp"
#include "include/sql_mm.h"
#include "include/mysql_mm.h"

VipAdmin vip_admin;
PLUGIN_EXPOSE(VipAdmin, vip_admin);

static constexpr const char* CONFIG_FILE = "addons/configs/admin_system/vip_admin.ini";

void startup_server()
{
	g_pGameEntitySystem = GameEntitySystem();
	g_pEntitySystem = utils->GetCEntitySystem();
	gpGlobals = utils->GetCGlobalVars();
}

bool VipAdmin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();
	GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);

	ConVar_Register(FCVAR_SERVER_CAN_EXECUTE | FCVAR_GAMEDLL);
	g_SMAPI->AddListener(this, this);
	return true;
}

void VipAdmin::AllPluginsLoaded()
{
	int ret;
	utils = static_cast<IUtilsApi*>(g_SMAPI->MetaFactory(UTILS_INTERFACE, &ret, NULL));
	if ( ret == META_IFACE_FAILED )
	{
		META_CONPRINTF("%s Missing UTILS system plugin", this->GetLogTag());
		std::string cmd = "meta unload " + std::to_string(g_PLID);
		engine->ServerCommand(cmd.c_str());
		return;
	}
	ISQLInterface* sql_interface = static_cast<ISQLInterface*>(g_SMAPI->MetaFactory(SQLMM_INTERFACE, &ret, nullptr));
	if ( ret == META_IFACE_FAILED )
	{
		utils->ErrorLog("%s Missing MYSQL plugin", this->GetLogTag());
		std::string cmd = "meta unload " + std::to_string(g_PLID);
		engine->ServerCommand(cmd.c_str());
		return;
	}
	mysql_client = sql_interface->GetMySQLClient();

	menus_api = static_cast<IMenusApi*>(g_SMAPI->MetaFactory(MENUS_INTERFACE, &ret, NULL));
	if ( ret == META_IFACE_FAILED )
	{
		utils->ErrorLog("%s Missing UTILS(Menus) system plugin", this->GetLogTag());
		std::string cmd = "meta unload " + std::to_string(g_PLID);
		engine->ServerCommand(cmd.c_str());
		return;
	}

	players_api = static_cast<IPlayersApi*>(g_SMAPI->MetaFactory(PLAYERS_INTERFACE, &ret, NULL));
	if ( ret == META_IFACE_FAILED )
	{
		utils->ErrorLog("%s Missing UTILS(Players) system plugin", this->GetLogTag());
		std::string cmd = "meta unload " + std::to_string(g_PLID);
		engine->ServerCommand(cmd.c_str());
		return;
	}

	vip_api = static_cast<IVIPApi*>(g_SMAPI->MetaFactory(VIP_INTERFACE, &ret, NULL));
	if ( ret == META_IFACE_FAILED )
	{
		utils->ErrorLog("%s Missing VIP CORE plugin", this->GetLogTag());
		std::string cmd = "meta unload " + std::to_string(g_PLID);
		engine->ServerCommand(cmd.c_str());
		return;
	}

	admin_api = static_cast<IAdminApi*>(g_SMAPI->MetaFactory(Admin_INTERFACE, &ret, NULL));
	if ( ret == META_IFACE_FAILED )
	{
		utils->ErrorLog("%s Missing ADMIN SYSTEM plugin", this->GetLogTag());
		std::string cmd = "meta unload " + std::to_string(g_PLID);
		engine->ServerCommand(cmd.c_str());
		return;
	}

	m_config.load(CONFIG_FILE);
	database::load_database();

	utils->StartupServer(g_PLID, startup_server);
	admin_api->OnCoreLoaded(g_PLID, [this]()
	{
		admin_api->RegisterCategory("vip_management", "Category_VIPManagement", nullptr);
		admin_api->RegisterItem("give_vip", "Item_GiveVIP", "vip_management", m_config.access_flag.c_str(), nullptr, [](int iSlot, const char* szCategory, const char* szIdentity, const char* szItem)
		{
			menus::open_vip_management(iSlot);
		});
		admin_api->RegisterItem("edit_vip", "Item_EditVip", "vip_management", m_config.access_flag.c_str(), nullptr, [](int iSlot, const char* szCategory, const char* szIdentity, const char* szItem)
		{
			menus::open_select_vip_player(iSlot);
		});
	});
}

bool VipAdmin::Unload(char* error, size_t maxlen)
{
	ConVar_Unregister();
	utils->ClearAllHooks(g_PLID);
	connection->Destroy();
	return true;
}
const char* VipAdmin::GetAuthor()
{
	return "ShadowRipper";
}
const char* VipAdmin::GetDate()
{
	return __DATE__;
}
const char* VipAdmin::GetDescription()
{
	return "VIP Management Module";
}
const char* VipAdmin::GetLicense()
{
	return "Free";
}
const char* VipAdmin::GetLogTag()
{
	return "[VipAdmin]";
}
const char* VipAdmin::GetName()
{
	return "[AS] VIP Management";
}
const char* VipAdmin::GetURL()
{
	return "";
}
const char* VipAdmin::GetVersion()
{
	return "1.1.2";
}