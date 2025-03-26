#pragma once

#define CMD_KEY_APPWIZ L"APPWIZ"
#define CMD_KEY_GENINST L"GENINST"
#define CMD_KEY_UNINSTALL L"UNINSTALL"
#define CMD_KEY_INSTALL L"INSTALL"
#define CMD_KEY_SETUP L"SETUP"
#define CMD_KEY_FIND L"FIND"
#define CMD_KEY_INFO L"INFO"
#define CMD_KEY_HELP L"?"
#define CMD_KEY_HELP_ALT L"HELP"


const WCHAR UsageString[] = L"RAPPS \
[/" CMD_KEY_HELP L"] \
[/" CMD_KEY_INSTALL L" packagename] \
[/" CMD_KEY_UNINSTALL L" packagename|displayname] \
[/" CMD_KEY_SETUP L" filename] \
[/" CMD_KEY_FIND L" string] \
[/" CMD_KEY_INFO L" packagename]";

BOOL ParseCmdAndExecute(LPWSTR lpCmdLine, BOOL bIsFirstLaunch, int nCmdShow);
