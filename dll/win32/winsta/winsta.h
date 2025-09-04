/*
 * PROJECT:         ReactOS winsta.dll
 * FILE:            dll/win32/winsta/winsta.h
 * PURPOSE:         WinStation
 * PROGRAMMER:      Aleksey Bragin
 * NOTES:           This file contains exported functions relevant to
 *                  userinit, winlogon, lsass and friends.
 */

#ifndef _WINSTA_H
#define _WINSTA_H

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winsta);

/* WinSta calling convention */
#define WINSTAAPI WINAPI

#endif /* _WINSTA_H */
