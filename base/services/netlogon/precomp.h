/*
 * PROJECT:     ReactOS NetLogon Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     NetLogon service RPC server
 * COPYRIGHT:   Eric Kohl 2019 <eric.kohl@reactos.org>
 */

#ifndef _NETLOGON_PCH_
#define _NETLOGON_PCH_

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
#include <lmerr.h>

#include <netlogon_s.h>

#include <wine/debug.h>

extern HINSTANCE hDllInstance;

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter);

#endif /* _NETLOGON_PCH_ */
