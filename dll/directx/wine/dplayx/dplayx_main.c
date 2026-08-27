/*
 * DPLAYX.DLL LibMain
 *
 * Copyright 1999,2000 - Peter Hunnisett
 *
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
 *
 * NOTES
 *  o DPMSGCMD_ENUMSESSIONSREPLY & DPMSGCMD_ENUMSESSIONSREQUEST
 *    Have most fields understood, but not all.  Everything seems to work.
 *  o DPMSGCMD_REQUESTNEWPLAYERID & DPMSGCMD_NEWPLAYERIDREPLY
 *    Barely works. This needs to be completed for sessions to start.
 *  o A small issue will be the fact that DirectX 6.1(ie. DirectPlay4)
 *    introduces a layer of functionality inside the DP objects which 
 *    provide guaranteed protocol delivery.  This is even if the native
 *    protocol, IPX or modem for instance, doesn't guarantee it. I'm going
 *    to leave this kind of implementation to as close to the end as 
 *    possible. However, I will implement an abstraction layer, where 
 *    possible, for this functionality. It will do nothing to start, but 
 *    will require only the implementation of the guarantee to give
 *    final implementation.
 *
 * TODO:
 *  - Implement mutual exclusion on object data for existing functions
 *  - Ensure that all dll stubs are present and the ordinals are correct
 *  - Addition of DirectX 7.0 functionality for direct play
 *  - Implement some WineLib test programs using sdk programs as a skeleton
 *  - Change RegEnumKeyEx enumeration pattern to allow error handling and to
 *    share registry implementation (or at least simplify).
 *  - Add in appropriate RegCloseKey calls for all the opening we're doing...
 *  - Fix all the buffer sizes for registry calls. They're off by one - 
 *    but in a safe direction.
 *  - Fix race condition on interface destruction
 *  - Handles need to be correctly reference counted
 *  - Check if we need to deallocate any list objects when destroying 
 *    a dplay interface
 *  - RunApplication process spawning needs to have correct synchronization.
 *  - Need to get inter lobby messages working.
 *  - Decipher dplay messages between applications and implement...
 *  - Need to implement lobby session spawning.
 *  - Improve footprint and realtime blocking by setting up a separate data share
 *    between lobby application and client since there can be multiple apps per
 *    client. Also get rid of offset dependency by making data offset independent
 *    somehow.
 */
#include <stdarg.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "wine/debug.h"
#include "dplay_global.h"
#include "dplayx_global.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

/* This is a globally exported variable at ordinal 6 of DPLAYX.DLL */
DWORD gdwDPlaySPRefCount = 0;


BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{

  TRACE( "(%p,%ld,%p)\n", hinstDLL, fdwReason, lpvReserved );

  switch ( fdwReason )
  {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        /* First instance perform construction of global processor data */
        return DPLAYX_ConstructData();

    case DLL_PROCESS_DETACH:
        DP_FreeConnections();
        /* Last instance performs destruction of global processor data */
        return DPLAYX_DestructData();

    default:
      break;

  }

  return TRUE;
}
