/*
 * PROJECT:    ReactOS IF Monitor DLL
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell main header file
 * COPYRIGHT:  Copyright 2025 Eric Kohl <eric.kohl@reactos.org>
 */

#ifndef PRECOMP_H
#define PRECOMP_H

/* INCLUDES ******************************************************************/

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#define _INC_WINDOWS
#include <winsock2.h>
#include <iphlpapi.h>
#include <ip2string.h>


#include <netsh.h>

extern HINSTANCE hDllInstance;

DWORD
WINAPI
RegisterInterfaceHelper(VOID);

DWORD
WINAPI
RegisterIpHelper(VOID);

DWORD
WINAPI
RegisterWinsockHelper(VOID);

#endif /* PRECOMP_H */
