#pragma once

#include "engine2/gameshareddefs.h"
#include "include/menus.h"
#include "include/utils.h"
#include "include/players.h"
#include "include/admin.h"
#include "include/sql_mm.h"
#include "include/vip.h"

extern IUtilsApi* utils;
extern IPlayersApi* players_api;
extern IVIPApi* vip_api;
extern IAdminApi* admin_api;
extern IMenusApi* menus_api;
extern IVEngineServer2* engine;
extern CGameEntitySystem* g_pGameEntitySystem;
extern CEntitySystem* g_pEntitySystem;
extern CGlobalVars* gpGlobals;
extern IMySQLClient* mysql_client;
extern IMySQLConnection* connection;
extern bool _debug;