/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004, 2005 Hans Leidekker
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

#include "config.h"

#include "wine/port.h"
#include "wine/debug.h"
#include "wine/library.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "mscms_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

#ifdef HAVE_LCMS
static int lcms_error_handler( int error, const char *text )
{
    switch (error)
    {
    case LCMS_ERRC_WARNING:
    case LCMS_ERRC_RECOVERABLE:
    case LCMS_ERRC_ABORTED:
        WARN("%d %s\n", error, debugstr_a(text));
        return 1;
    default:
        ERR("unknown error %d %s\n", error, debugstr_a(text));
        return 0;
    }
}
#endif

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE( "(%p, %d, %p)\n", hinst, reason, reserved );

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
#ifdef HAVE_LCMS
        cmsSetErrorHandler( lcms_error_handler );
#endif
        break;
    case DLL_PROCESS_DETACH:
#ifdef HAVE_LCMS
        free_handle_tables();
#endif
        break;
    }
    return TRUE;
}
