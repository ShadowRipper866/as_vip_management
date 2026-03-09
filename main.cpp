#include "main.h"

#define MAX_PLAYERS 64
#define ACCOUNT_ID_OFFSET 76561197960265728

VipAdmin g_VipAdmin;
PLUGIN_EXPOSE(VipAdmin, g_VipAdmin);

IUtilsApi* utils;
IPlayersApi* players_api;
IVIPApi *vip_api;
IAdminApi *admin_api;
IMenusApi *menus_api;

IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;

IMySQLClient *mysql_client;
IMySQLConnection* connection;

int server_id;
const char *accessFlag;
const char *categoryName;
const char* itemName;
unordered_map<uint64, string> seconds_to_display_time;
vector<pair<string, uint64>> time_list;
unordered_map<string, string> actual_to_display_group;
vector<pair<string, string>> group_list;
unordered_map<int, PendingVip> pending_actions;
static bool debug = false;

CGameEntitySystem* GameEntitySystem() {
    return utils->GetCGameEntitySystem();
}
void StartupServer() {
    g_pGameEntitySystem = GameEntitySystem();
    g_pEntitySystem = utils->GetCEntitySystem();
}

string UnixTimeToReadableDate(long long unixTime) {
    time_t time = unixTime;
    tm* ptm = localtime(&time);
    if (!ptm)
        return "Invalid time";
    char szBuffer[32];
    if (unixTime != 0)
        strftime(szBuffer, sizeof(szBuffer), "%H:%M:%S (%d.%m.%Y)", ptm);
    else
        return admin_api->GetTranslation("Never");
    return string(szBuffer);
}

bool IsPlayerValid(int slot) {
    CCSPlayerController *player = CCSPlayerController::FromSlot(slot);
    if (!player || !engine->IsClientFullyAuthenticated(slot) || player->IsBot() || !player->IsConnected())
        return false;
    return true;
}

void dbgmsg(string content, ...) {
    if (debug) {
        char buf[1024];
        va_list va;
        va_start(va, content);
        V_vsnprintf(buf, sizeof(buf), content.c_str(), va);
        va_end(va);
        ConColorMsg(Color(0, 200, 255, 255), "%s %s\n", g_PLAPI->GetLogTag(), buf);
    }
}

void OpenGiveVipMenu(int adminSlot, int target);
void OpenVipGiveMenu(int slot);
void OpenVipChangeMenu(int adminSlot, int target);

void OpenGroupSelectionMenu(int adminSlot, int target) {
    Menu groupSelect;
    groupSelect.clear();
    menus_api->SetTitleMenu(groupSelect, admin_api->GetTranslation("VIPManagement_SelectGroup"));
    for (auto &g : group_list)
        menus_api->AddItemMenu(groupSelect, g.second.c_str(), g.first.c_str());
    menus_api->SetBackMenu(groupSelect, true);
    menus_api->SetExitMenu(groupSelect, true);
    menus_api->SetCallback(groupSelect, [adminSlot, target](const char* szBack, const char* szFront, int iItem, int iSlot) {
        if (!strcmp(szBack, "back")) {
            OpenGiveVipMenu(adminSlot, target);
            return;
        }
        if (iItem < 7) {
            pending_actions[adminSlot].selected_group = szBack;
            OpenGiveVipMenu(adminSlot, target);
        }
    });
    menus_api->DisplayPlayerMenu(groupSelect, adminSlot, true, true);
}

void OpenTimeSelectionMenu(int adminSlot, int target) {
    Menu timeSelect;
    timeSelect.clear();
    menus_api->SetTitleMenu(timeSelect, admin_api->GetTranslation("VIPManagement_SelectTime"));
    for (const auto &t : time_list)
        menus_api->AddItemMenu(timeSelect, to_string(t.second).c_str(), t.first.c_str());
    menus_api->SetBackMenu(timeSelect, true);
    menus_api->SetExitMenu(timeSelect, true);
    menus_api->SetCallback(timeSelect, [adminSlot, target](const char* szBack, const char* szFront, int iItem, int iSlot) {
        if (!strcmp(szBack, "back")) {
            OpenGiveVipMenu(adminSlot, target);
            return;
        }
        if (iItem < 7) {
            pending_actions[adminSlot].selected_time = atoll(szBack);
            OpenGiveVipMenu(adminSlot, target);
        }
    });
    menus_api->DisplayPlayerMenu(timeSelect, adminSlot, true, true);
}

void OpenGiveVipMenu(int adminSlot, int target) {
    auto& d = pending_actions[adminSlot];
    if (d.target_slot != target) {
        d.target_slot = target;
        d.selected_group = "";
        d.selected_time = -1;
    }

    Menu giveVip;
    giveVip.clear();
    menus_api->SetTitleMenu(giveVip, admin_api->GetTranslation("VIPManagement_GiveVIP"));

    char bufGroup[128];
    if (!d.selected_group.empty()) {
        string disp = actual_to_display_group.count(d.selected_group) ? actual_to_display_group[d.selected_group] : d.selected_group;
        snprintf(bufGroup, sizeof(bufGroup), "%s: %s", admin_api->GetTranslation("VIPManagement_VIPGroup"), disp.c_str());
    }
    else
        snprintf(bufGroup, sizeof(bufGroup), "%s", admin_api->GetTranslation("VIPManagement_SelectGroup"));
    menus_api->AddItemMenu(giveVip, "groupSelection", bufGroup);

    char bufTime[128];
    if (d.selected_time != -1) {
        string disp = seconds_to_display_time.count(d.selected_time) ? seconds_to_display_time[d.selected_time] : to_string(d.selected_time);
        snprintf(bufTime, sizeof(bufTime), "%s: %s", admin_api->GetTranslation("VIPManagement_VIPExpires"), disp.c_str());
    }
    else
        snprintf(bufTime, sizeof(bufTime), "%s", admin_api->GetTranslation("VIPManagement_SelectTime"));
    menus_api->AddItemMenu(giveVip, "timeSelection", bufTime);
    if (!d.selected_group.empty() && d.selected_time != -1)
        menus_api->AddItemMenu(giveVip, "applyVip", admin_api->GetTranslation("VIPManagement_Apply"));
    menus_api->SetBackMenu(giveVip, true);
    menus_api->SetExitMenu(giveVip, true);
    menus_api->SetCallback(giveVip, [adminSlot, target](const char* szBack, const char* szFront, int iItem, int iSlot) {
        if (!strcmp(szBack, "back")) {
            OpenVipGiveMenu(adminSlot);
            return;
        }
        if (iItem < 7) {
            if (!strcmp(szBack, "groupSelection"))
                OpenGroupSelectionMenu(adminSlot, target);
            else if (!strcmp(szBack, "timeSelection"))
                OpenTimeSelectionMenu(adminSlot, target);
            else if (!strcmp(szBack, "applyVip")) {
                auto& data = pending_actions[adminSlot];
                if (!data.selected_group.empty() && data.selected_time != -1) {
                    dbgmsg("Selected Group: %s; Selected Time: %lld; Slot: %i", data.selected_group.c_str(), data.selected_time, data.target_slot);
                    vip_api->VIP_GiveClientVIP(data.target_slot, data.selected_time, data.selected_group.c_str(), true);
                    pending_actions.erase(adminSlot);
                    OpenVipGiveMenu(adminSlot);
                }
            }
        }
    });
    menus_api->DisplayPlayerMenu(giveVip, adminSlot, true, true);
}

void OpenVipTimeChangeMenu(int adminSlot, int target, long long currentTime) {
    extern void OpenVipSelectChangeMode(int admin, int slot, long long time);
    Menu timesMenu;
    timesMenu.clear();
    menus_api->SetTitleMenu(timesMenu, admin_api->GetTranslation("VIPManagement_SelectTime"));
    for (const auto &t : time_list)
        menus_api->AddItemMenu(timesMenu, to_string(t.second).c_str(), t.first.c_str());
    menus_api->SetBackMenu(timesMenu, true);
    menus_api->SetExitMenu(timesMenu, true);
    menus_api->SetCallback(timesMenu, [adminSlot, target](const char* szBack, const char* szFront, int iItem, int iSlot) {
        if (strcmp(szBack, "back") == 0) {
            OpenVipChangeMenu(adminSlot, target);
            return;
        }
        if (iItem < 7) {
            long long selectedVipTime = atoll(szBack);
            long long currentTargetTime = vip_api->VIP_GetClientAccessTime(target);

            if (selectedVipTime == 0) {
                vip_api->VIP_SetClientAccessTime(target, 0, true);
                OpenVipChangeMenu(adminSlot, target);
                return;
            }
            if (currentTargetTime == 0) {
                long long newExpireTime = std::time(0) + selectedVipTime;
                vip_api->VIP_SetClientAccessTime(target, newExpireTime, true);
                OpenVipChangeMenu(adminSlot, target);
                return;
            }
            OpenVipSelectChangeMode(adminSlot, target, selectedVipTime);
        }
    });
    menus_api->DisplayPlayerMenu(timesMenu, adminSlot, true, true);
}

void LoadDatabase() {
    if (connection != nullptr)
        return;

    KeyValues *databases = new KeyValues("Databases");
    if (!databases->LoadFromFile(g_pFullFileSystem, "addons/configs/databases.cfg")) {
        utils->ErrorLog("%s failed to load databases.cfg", g_PLAPI->GetLogTag());
        return;
    }
    KeyValues *database = databases->FindKey("vip", false);
    if (!database) {
        utils->ErrorLog("%s No `vip` field in databases.cfg", g_PLAPI->GetLogTag());
        delete databases;
        return;
    }

    MySQLConnectionInfo info;
    info.host = database->GetString("host", "");
    info.user = database->GetString("user", "");
    info.pass = database->GetString("pass", "");
    info.database = database->GetString("database", "");
    info.port = database->GetInt("port", 3306);
    connection = mysql_client->CreateMySQLConnection(info);
    dbgmsg("[DEBUG] DB Host: '%s'", info.host);
    dbgmsg("[DEBUG] DB User: '%s'", info.user);
    dbgmsg("[DEBUG] DB Pass: '%s'", info.pass);
    connection->Connect([databases](bool connect) {
        if (!connect) {
            META_CONPRINTF("%s Failed to connect to MySQL\n", g_PLAPI->GetLogTag());
            connection = nullptr;
            delete databases;
            return;
        }
        dbgmsg("Database connection created successfully");
        delete databases;
    });
}

void ChangeVipGroupViaDatabase(int slot, const char* group) {

    dbgmsg("SERVER ID: %i", server_id);
    if (!IsPlayerValid(slot))
        return;
    char buf[1024];
    CCSPlayerController *player = CCSPlayerController::FromSlot(slot);
    uint64 acc = player->m_steamID.Get() - ACCOUNT_ID_OFFSET;
    snprintf(buf, sizeof(buf), "UPDATE `vip_users` SET `group` = '%s' WHERE `account_id` = %llu AND `sid` = %i", connection->Escape(group).c_str(), acc, server_id);
    dbgmsg(buf);
    connection->Query(buf, [](ISQLQuery *pq) {});
}

void OpenVipGroupChangeMenu(int adminSlot, int target, long long currentTime) {
    Menu groupsMenu;
    groupsMenu.clear();
    menus_api->SetTitleMenu(groupsMenu, admin_api->GetTranslation("VIPManagement_SelectGroup"));
    for (auto &g : group_list)
        menus_api->AddItemMenu(groupsMenu, g.second.c_str(), g.first.c_str());
    menus_api->SetBackMenu(groupsMenu, true);
    menus_api->SetExitMenu(groupsMenu, true);
    menus_api->SetCallback(groupsMenu, [adminSlot, target](const char* szBack, const char* szFront, int iItem, int iSlot) {
        if (!strcmp(szBack, "back")) {
            OpenVipChangeMenu(adminSlot, target);
            return;
        }
        if (iItem < 7) {
            dbgmsg("Target VIP: %i", target);
            ChangeVipGroupViaDatabase(target, szBack);
            char buf[128];
            uint64 acc = players_api->GetSteamID64(target);
            snprintf(buf, sizeof(buf), "mm_reload_vip %llu", acc);
            engine->ServerCommand(buf);
            pending_actions[adminSlot].selected_group = szBack;
            utils->CreateTimer(0.5f, [adminSlot, target](){
                OpenVipChangeMenu(adminSlot, target);
                return -1.0f;
            });
        }
    });
    menus_api->DisplayPlayerMenu(groupsMenu, adminSlot, true, true);
}

void OpenVipSelectChangeMode(int admin, int target, long long time) {
    Menu changeMode;
    changeMode.clear();
    menus_api->SetTitleMenu(changeMode, admin_api->GetTranslation("VIPManagement_SelectChangeMode"));
    menus_api->AddItemMenu(changeMode, "replaceTime", admin_api->GetTranslation("VIPManagement_ReplaceMode"));
    menus_api->AddItemMenu(changeMode, "addTime", admin_api->GetTranslation("VIPManagement_AddMode"));
    menus_api->SetBackMenu(changeMode, true);
    menus_api->SetExitMenu(changeMode, true);
    menus_api->SetCallback(changeMode, [admin, target, time](const char* szBack, const char* szFront, int iItem, int iSlot) {
        if (!strcmp(szBack, "back")) {
            OpenVipTimeChangeMenu(admin, target, vip_api->VIP_GetClientAccessTime(target));
            return;
        }
        if (iItem < 7) {
            if (!strcmp(szBack, "replaceTime")) {
                long long timeAccess = std::time(0) + time;
                vip_api->VIP_SetClientAccessTime(target, timeAccess);
                OpenVipChangeMenu(admin, target);
            }
            if (!strcmp(szBack, "addTime")) {
                long long timeAccess = time == 0 ? 0 : vip_api->VIP_GetClientAccessTime(target) + time;
                vip_api->VIP_SetClientAccessTime(target, timeAccess);
                OpenVipChangeMenu(admin, target);
            }
        }
    });
    menus_api->DisplayPlayerMenu(changeMode, admin, true, true);
}
void SelectVipPlayerMenu(int slot);
void OpenVipChangeMenu(int adminSlot, int target) {
    bool isVip = vip_api->VIP_IsClientVIP(target);
    if (!isVip) {
        OpenGiveVipMenu(adminSlot, target);
        return;
    }
    const char* group = vip_api->VIP_GetClientVIPGroup(target);
    long long time = vip_api->VIP_GetClientAccessTime(target);
    long long calculateTime = time - std::time(0);
    Menu changeVipMenu;
    changeVipMenu.clear();
    menus_api->SetTitleMenu(changeVipMenu, admin_api->GetTranslation("VIPManagement_ChangeVIPPlayer"));

    char bufGroup[128];
    string disp_group = actual_to_display_group.count(group) ? actual_to_display_group[group] : group;
    snprintf(bufGroup, sizeof(bufGroup), "%s: %s",  admin_api->GetTranslation("VIPManagement_VIPGroup"), disp_group.c_str());
    menus_api->AddItemMenu(changeVipMenu, "vipGroup", bufGroup);

    char bufExpires[128];
    snprintf(bufExpires, sizeof(bufExpires), "%s: %s", admin_api->GetTranslation("VIPManagement_VIPExpires"), time == 0 ? admin_api->GetTranslation("Never") :  to_string(calculateTime).c_str());
    menus_api->AddItemMenu(changeVipMenu, "vipExpires", bufExpires);

    char bufExpiresDisabled[128];
    snprintf(bufExpiresDisabled, sizeof(bufExpiresDisabled), "%s: %s", admin_api->GetTranslation("VIPManagement_VIPExpiresDisabled"), UnixTimeToReadableDate(time).c_str());
    menus_api->AddItemMenu(changeVipMenu, "vipExpires", bufExpiresDisabled, ITEM_DISABLED);

    menus_api->AddItemMenu(changeVipMenu, "deleteVip", admin_api->GetTranslation("VIPManagement_DeleteVIP"));
    menus_api->SetBackMenu(changeVipMenu, true);
    menus_api->SetExitMenu(changeVipMenu, true);
    menus_api->SetCallback(changeVipMenu, [target, time](const char* szBack, const char* szFront, int iItem, int adminSlot) {
        if (!strcmp(szBack, "back")) {
            OpenVipGiveMenu(adminSlot);
            return;
        }
        if (iItem < 7) {
            if (!strcmp(szBack, "deleteVip")) {
                vip_api->VIP_RemoveClientVIP(target, true);
                SelectVipPlayerMenu(adminSlot);
            }
            else if (!strcmp(szBack, "vipExpires"))
                OpenVipTimeChangeMenu(adminSlot, target, time);
            else if (!strcmp(szBack, "vipGroup"))
                OpenVipGroupChangeMenu(adminSlot, target, time);
        }
    });
    menus_api->DisplayPlayerMenu(changeVipMenu, adminSlot, true, true);
}

void SelectVipPlayerMenu(int slot) {
    Menu menu;
    menu.clear();
    menus_api->SetTitleMenu(menu, admin_api->GetTranslation("VIPManagement_SelectVIPPlayer"));
    bool found = false;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerValid(i) || !vip_api->VIP_IsClientVIP(i))
            continue;
        CCSPlayerController*player = CCSPlayerController::FromSlot(i);
        menus_api->AddItemMenu(menu, to_string(i).c_str(), player->m_iszPlayerName);
        found = true;
    }
    if (!found)
        menus_api->AddItemMenu(menu, "", admin_api->GetTranslation("CC_NoPlayers"), ITEM_DISABLED);
    menus_api->SetBackMenu(menu, true);
    menus_api->SetExitMenu(menu, true);
    menus_api->SetCallback(menu, [](const char* szBack, const char* szFront, int iItem, int slot) {
        if (!strcmp(szBack, "back")) {
            admin_api->ShowAdminCategoryMenu(slot, "vip_management");
            return;
        }
        if (iItem < 7) {
            int target = atoi(szBack);
            OpenVipChangeMenu(slot, target);
        }
    });
    menus_api->DisplayPlayerMenu(menu, slot, true, true);
}

void OpenVipGiveMenu(int slot) {
    Menu menu;
    menu.clear();
    menus_api->SetTitleMenu(menu , admin_api->GetTranslation("VIPManagement_SelectPlayer"));
    bool found = false;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!IsPlayerValid(i) || vip_api->VIP_IsClientVIP(i))
           continue;
        CCSPlayerController*player = CCSPlayerController::FromSlot(i);
        menus_api->AddItemMenu(menu, to_string(i).c_str(), player->m_iszPlayerName);
        found = true;
    }
    if (!found)
        menus_api->AddItemMenu(menu, "", admin_api->GetTranslation("CC_NoPlayers"), ITEM_DISABLED);
    menus_api->SetBackMenu(menu, true);
    menus_api->SetExitMenu(menu, true);
    menus_api->SetCallback(menu, [](const char* szBack, const char* szFront, int iItem, int slot) {
        if (!strcmp(szBack, "back")) {
            admin_api->ShowAdminCategoryMenu(slot, "vip_management");
            return;
        }
        if (iItem < 7) {
            int target = atoi(szBack);
            if (!IsPlayerValid(target)) {
                menus_api->ClosePlayerMenu(slot);
                return;
            }
            OpenGiveVipMenu(slot, target);
        }
    });
    menus_api->DisplayPlayerMenu(menu, slot, true, true);
}

void LoadConfig() {
    seconds_to_display_time.clear();
    time_list.clear();
    actual_to_display_group.clear();
    group_list.clear();
    KeyValues *vip = new KeyValues("VIP");
    const char *pathVip = "addons/configs/vip/groups.ini";
    if (!vip->LoadFromFile(g_pFullFileSystem, pathVip)) {
        utils->ErrorLog("Failed to load config file: %s", g_PLAPI->GetLogTag(), pathVip);
        delete vip;
        return;
    }
    server_id = vip->GetInt("server_id");
    KeyValues *config = new KeyValues("Config");
    const char *path = "addons/configs/admin_system/vip_admin.ini";
    if (!config->LoadFromFile(g_pFullFileSystem, path)) {
        utils->ErrorLog("Failed to load config file: %s", g_PLAPI->GetLogTag(), path);
        delete config;
        return;
    }

    KeyValues *kTimes = config->FindKey("times");
    if (kTimes) {
        for (KeyValues *sub = kTimes->GetFirstSubKey(); sub; sub = sub->GetNextKey()) {
            const char* timeName = sub->GetName();
            uint64 timeSec = sub->GetUint64(nullptr);
            if (timeName) {
                time_list.push_back({timeName, timeSec});
                seconds_to_display_time[timeSec] = timeName;
            }
        }
    }
    else
        dbgmsg("[DEBUG] WARNING: Section 'times' not found in config!\n");

    KeyValues *kGroups = config->FindKey("groups");
    if (kGroups) {
        dbgmsg("[DEBUG] Reading 'groups' section...\n");
        for (KeyValues *sub = kGroups->GetFirstSubKey(); sub; sub = sub->GetNextKey()) {
            const char* display = sub->GetName();
            const char* actual = sub->GetString(nullptr, display);
            if (display) {
                group_list.push_back({display, actual});
                actual_to_display_group[actual] = display;
                dbgmsg("[DEBUG] Loaded Group: %s -> %s\n", display, actual);
            }
        }
    }
    else
        META_CONPRINTF("%s WARNING: Section 'groups' not found in config!\n", g_PLAPI->GetLogTag());
    accessFlag = config->GetString("accessFlag", "@admin/root");
    debug = config->GetBool("debug", false);
    delete config;
    delete vip;
}

bool VipAdmin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
    PLUGIN_SAVEVARS();
    GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
    ConVar_Register(FCVAR_SERVER_CAN_EXECUTE | FCVAR_GAMEDLL);
    g_SMAPI->AddListener( this, this );
    return true;
}

void VipAdmin::AllPluginsLoaded() {
    int ret;
    char error[64] = { 0 };
    ISQLInterface* sqlInterface = (ISQLInterface *)g_SMAPI->MetaFactory(SQLMM_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) {
        utils->ErrorLog("[%s] Missing MYSQL plugin", g_PLAPI->GetLogTag());
        string sBuffer = "meta unload "+to_string(g_PLID);
        engine->ServerCommand(sBuffer.c_str());
        return;
    }
    mysql_client = sqlInterface->GetMySQLClient();
    utils = (IUtilsApi *)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, NULL);
    if (ret == META_IFACE_FAILED)
    {
        utils->ErrorLog("%s Missing UTILS system plugin", g_VipAdmin.GetLogTag());
        string sBuffer = "meta unload " + to_string(g_PLID);
        engine->ServerCommand(sBuffer.c_str());
        return;
    }
    menus_api = (IMenusApi *)g_SMAPI->MetaFactory(Menus_INTERFACE, &ret, NULL);
    if (ret == META_IFACE_FAILED)
    {
        utils->ErrorLog("%s Missing UTILS(Menus) system plugin", g_VipAdmin.GetLogTag());
        string sBuffer = "meta unload " + to_string(g_PLID);
        engine->ServerCommand(sBuffer.c_str());
        return;
    }
    players_api = (IPlayersApi *)g_SMAPI->MetaFactory(PLAYERS_INTERFACE, &ret, NULL);
    if (ret == META_IFACE_FAILED)
    {
        utils->ErrorLog("%s Missing UTILS(Players) system plugin", g_VipAdmin.GetLogTag());
        string sBuffer = "meta unload " + to_string(g_PLID);
        engine->ServerCommand(sBuffer.c_str());
        return;
    }
    vip_api = (IVIPApi*)g_SMAPI->MetaFactory(VIP_INTERFACE, &ret, NULL);
    if (ret == META_IFACE_FAILED)
    {
        utils->ErrorLog("%s Missing VIP CORE plugin", g_VipAdmin.GetLogTag());
        string sBuffer = "meta unload " + to_string(g_PLID);
        engine->ServerCommand(sBuffer.c_str());
        return;
    }
    admin_api = (IAdminApi*)g_SMAPI->MetaFactory(Admin_INTERFACE, &ret, NULL);
    if (ret == META_IFACE_FAILED)
    {
        utils->ErrorLog("%s Missing ADMIN SYSTEM plugin", g_VipAdmin.GetLogTag());
        string sBuffer = "meta unload " + to_string(g_PLID);
        engine->ServerCommand(sBuffer.c_str());
        return;
    }
    LoadConfig();
    LoadDatabase();
    utils->HookEvent(g_PLID, "player_disconnect", [](const char* content, IGameEvent* event, bool broadcast) {
        int slot = event->GetInt("userid");
        pending_actions.erase(slot);
    });
    utils->StartupServer(g_PLID, StartupServer);
    admin_api->OnCoreLoaded(g_PLID, []() {
        admin_api->RegisterCategory("vip_management", "Category_VIPManagement", nullptr);
        admin_api->RegisterItem("give_vip", "Item_GiveVIP", "vip_management", accessFlag, nullptr, [](int iSlot, const char* szCategory, const char* szIdentity, const char* szItem){
            OpenVipGiveMenu(iSlot);
        });
        admin_api->RegisterItem("edit_vip", "Item_EditVip", "vip_management", accessFlag, nullptr, [](int iSlot, const char* szCategory, const char* szIdentity, const char* szItem){
            SelectVipPlayerMenu(iSlot);
        });
    });
}

bool VipAdmin::Unload(char *error, size_t maxlen)
{
    ConVar_Unregister();
    utils->ClearAllHooks(g_PLID);
    return true;
}
const char *VipAdmin::GetAuthor() {
    return "ShadowRipper";
}
const char *VipAdmin::GetDate() {
    return __DATE__;
}
const char *VipAdmin::GetDescription() {
    return "VIP Management Module";
}
const char *VipAdmin::GetLicense() {
    return "Free";
}
const char *VipAdmin::GetLogTag() {
    return "[VipAdmin]";
}
const char *VipAdmin::GetName() {
    return "[AS] VIP Management";
}
const char *VipAdmin::GetURL() {
    return "";
}
const char *VipAdmin::GetVersion() {
    return "1.0.2";
}
