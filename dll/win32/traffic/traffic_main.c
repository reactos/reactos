/*
 * QoS traffic control implementation
 *
 * Copyright 2009 Austin English
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "traffic.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(traffic);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        default:
            break;
    }

    return TRUE;
}

/*****************************************************************************
 * TcRegisterClient [TRAFFIC.@]
 */
ULONG WINAPI TcRegisterClient(ULONG version, HANDLE context,
                              PTCI_CLIENT_FUNC_LIST list, PHANDLE buffer)
{
    FIXME("(%u %p %p %p) stub\n", version, context, list, buffer);
    if(buffer) *buffer = INVALID_HANDLE_VALUE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}
