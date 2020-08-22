#pragma once

#define CMD_KEY_INSTALL L"INSTALL"
#define CMD_KEY_SETUP L"SETUP"
#define CMD_KEY_HELP L"HELP"

const WCHAR UsageString[] = L"RAPPS [/INSTALL packagename] [/SETUP filename]";

// return TRUE if the SETUP key was valid
BOOL ParseCmdAndExecute(LPWSTR lpCmdLine, BOOL bIsFirstLaunch, int nCmdShow);
