/*
 * Copyright 2000 Peter Hunnisett
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

#ifndef __WINE_DPLAYX_NAMESERVER
#define __WINE_DPLAYX_NAMESERVER

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dplay.h"
#include "wine/dplaysp.h"
#include "dplayx_messages.h"
#include "dplay_global.h"

void NS_SetLocalComputerAsNameServer( LPCDPSESSIONDESC2 lpsd, LPVOID lpNSInfo ) DECLSPEC_HIDDEN;
void NS_AddRemoteComputerAsNameServer( LPCVOID lpNSAddrHdr,
                                       DWORD dwHdrSize,
                                       LPCDPMSG_ENUMSESSIONSREPLY lpcMsg,
                                       LPVOID lpNSInfo ) DECLSPEC_HIDDEN;
LPVOID NS_GetNSAddr( LPVOID lpNSInfo ) DECLSPEC_HIDDEN;
DWORD NS_GetNsMagic( LPVOID lpNSInfo ) DECLSPEC_HIDDEN;
void NS_SetLocalAddr( LPVOID lpNSInfo, LPCVOID lpHdr, DWORD dwHdrSize ) DECLSPEC_HIDDEN;

void NS_ReplyToEnumSessionsRequest( LPCVOID lpcMsg,
                                    LPVOID* lplpReplyData,
                                    LPDWORD lpdwReplySize,
                                    IDirectPlayImpl *lpDP ) DECLSPEC_HIDDEN;

HRESULT NS_SendSessionRequestBroadcast( LPCGUID lpcGuid,
                                        DWORD dwFlags,
                                        const SPINITDATA *lpSpData ) DECLSPEC_HIDDEN;


BOOL NS_InitializeSessionCache( LPVOID* lplpNSInfo ) DECLSPEC_HIDDEN;
void NS_DeleteSessionCache( LPVOID lpNSInfo ) DECLSPEC_HIDDEN;
void NS_InvalidateSessionCache( LPVOID lpNSInfo ) DECLSPEC_HIDDEN;


void NS_ResetSessionEnumeration( LPVOID lpNSInfo ) DECLSPEC_HIDDEN;
LPDPSESSIONDESC2 NS_WalkSessions( LPVOID lpNSInfo ) DECLSPEC_HIDDEN;
void NS_PruneSessionCache( LPVOID lpNSInfo ) DECLSPEC_HIDDEN;

#endif /* __WINE_DPLAYX_NAMESERVER */
