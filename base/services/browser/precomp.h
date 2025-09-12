/*
 * PROJECT:     ReactOS Browser Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Browser service RPC server
 * COPYRIGHT:   Eric Kohl 2020 <eric.kohl@reactos.org>
 */

#ifndef _BROWSER_PCH_
#define _BROWSER_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>

#include <browser_s.h>

#include <wine/debug.h>

extern HINSTANCE hDllInstance;

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter);

#endif /* _BROWSER_PCH_ */
