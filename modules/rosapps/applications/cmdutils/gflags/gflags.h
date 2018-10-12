/*
* PROJECT:     Global Flags utility
* LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
* PURPOSE:     Global Flags utility page heap options
* COPYRIGHT:   Copyright 2017 Pierre Schweitzer (pierre@reactos.org)
*/

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <stdio.h>
#include <stdlib.h>


/* Option specific commandline parsing */
BOOL PageHeap_ParseCmdline(INT i, int argc, LPWSTR argv[]);

/* Execute parsed options */
INT PageHeap_Execute();

/* Common functions */
DWORD ReagFlagsFromRegistry(HKEY SubKey, PVOID Buffer, PWSTR Value, DWORD MaxLen);

