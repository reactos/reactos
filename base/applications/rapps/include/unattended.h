#pragma once

#define CMD_KEY_INSTALL L"INSTALL"
#define CMD_KEY_SETUP L"SETUP"
#define CMD_KEY_FIND L"FIND"
#define CMD_KEY_INFO L"INFO"
#define CMD_KEY_HELP L"?"

const WCHAR UsageString[] = L"RAPPS [/" CMD_KEY_HELP "] [/" CMD_KEY_INSTALL " packagename] [/" CMD_KEY_SETUP " filename]";

BOOL ParseCmdAndExecute(LPWSTR lpCmdLine, BOOL bIsFirstLaunch, int nCmdShow);
