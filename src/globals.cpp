#include "globals.hpp"
#include "engine2/gameshareddefs.h"

IUtilsApi* utils = nullptr;
IPlayersApi* players_api = nullptr;
IVIPApi* vip_api = nullptr;
IAdminApi* admin_api = nullptr;
IMenusApi* menus_api = nullptr;
IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CGlobalVars* gpGlobals = nullptr;
IMySQLClient* mysql_client = nullptr;
IMySQLConnection* connection = nullptr;
bool _debug = false;