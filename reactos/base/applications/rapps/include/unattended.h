#pragma once

#define CMD_KEY_INSTALL L"/INSTALL"
#define CMD_KEY_SETUP L"/SETUP"

// return TRUE if the SETUP key was valid
BOOL UseCmdParameters(LPWSTR lpCmdLine);
