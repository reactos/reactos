/*
 * Copyright 1999, 2000 Peter Hunnisett
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_DPLAYX_GLOBAL
#define __WINE_DPLAYX_GLOBAL

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <dplay.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "dplaysp.h"
#include "lobbysp.h"
#include "dplayx_queue.h"
#include "dplay_global.h"
#include "dplayx_messages.h"
#include "name_server.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

BOOL DPLAYX_ConstructData(void) DECLSPEC_HIDDEN;
BOOL DPLAYX_DestructData(void) DECLSPEC_HIDDEN;

HRESULT DPLAYX_GetConnectionSettingsA ( DWORD dwAppID,
                                        LPVOID lpData,
                                        LPDWORD lpdwDataSize ) DECLSPEC_HIDDEN;
HRESULT DPLAYX_GetConnectionSettingsW ( DWORD dwAppID,
                                        LPVOID lpData,
                                        LPDWORD lpdwDataSize ) DECLSPEC_HIDDEN;

HRESULT DPLAYX_SetConnectionSettingsA ( DWORD dwFlags,
                                        DWORD dwAppID,
                                        const DPLCONNECTION *lpConn ) DECLSPEC_HIDDEN;
HRESULT DPLAYX_SetConnectionSettingsW ( DWORD dwFlags,
                                        DWORD dwAppID,
                                        const DPLCONNECTION *lpConn ) DECLSPEC_HIDDEN;

BOOL DPLAYX_CreateLobbyApplication( DWORD dwAppID ) DECLSPEC_HIDDEN;

BOOL DPLAYX_WaitForConnectionSettings( BOOL bWait ) DECLSPEC_HIDDEN;
BOOL DPLAYX_AnyLobbiesWaitingForConnSettings(void) DECLSPEC_HIDDEN;

BOOL DPLAYX_SetLobbyHandles( DWORD dwAppID,
                             HANDLE hStart, HANDLE hDeath, HANDLE hConnRead ) DECLSPEC_HIDDEN;

BOOL DPLAYX_SetLobbyMsgThreadId( DWORD dwAppId, DWORD dwThreadId ) DECLSPEC_HIDDEN;


/* Convert a DP or DPL HRESULT code into a string for human consumption */
LPCSTR DPLAYX_HresultToString( HRESULT hr ) DECLSPEC_HIDDEN;

#endif /* __WINE_DPLAYX_GLOBAL */
