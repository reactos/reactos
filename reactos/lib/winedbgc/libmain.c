/*
 * Win32 winedbgc functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <ntddk.h>
#include <wine/debugtools.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
/*
#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "winbase.h"
#include "wine/debug.h"
#include "wine/library.h"

#include "proxywinedbgc.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbgc);
 */

/***********************************************************************
 * DllMain [Internal] Initializes the internal 'winedbgc32.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    
/*
    TRACE("Initializing or Finalizing proxy winedbgc: %p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
       TRACE("Loading winedbgc...\n");
#ifndef __REACTOS__
       if (winedbgc_LoadDriverManager())
          winedbgc_LoadDMFunctions();
#endif
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
      TRACE("Unloading winedbgc...\n");
#ifndef __REACTOS__
      if (gProxyHandle.bFunctionReady)
      {
         int i;
         for ( i = 0; i < NUM_SQLFUNC; i ++ )
         {
            gProxyHandle.functions[i].func = SQLDummyFunc;
         }
      }
      if (gProxyHandle.dmHandle)
      {
         wine_dlclose(gProxyHandle.dmHandle,NULL,0);
         gProxyHandle.dmHandle = NULL;
      }
#endif
    }
 */
    return TRUE;
}


/* EOF */
