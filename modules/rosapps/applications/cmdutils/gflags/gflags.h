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
#include <pstypes.h>

extern
const WCHAR ImageExecOptionsString[];

/* Option specific commandline parsing */
BOOL PageHeap_ParseCmdline(INT i, int argc, LPWSTR argv[]);

/* Execute parsed options */
INT PageHeap_Execute();

/* Common functions */
DWORD ReadSZFlagsFromRegistry(HKEY SubKey, PWSTR Value);
BOOL OpenImageFileExecOptions(IN REGSAM SamDesired, IN OPTIONAL PCWSTR ImageName, OUT HKEY* Key);

