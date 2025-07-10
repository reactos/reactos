/*
 * Implementation of DCIMAN32 - DCI Manager
 * "Device Context Interface" ?
 *
 * Copyright 2000 Marcus Meissner
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
#include <stdio.h> 

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "dciman.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dciman);

/***********************************************************************
 *		DCIOpenProvider (DCIMAN32.@)
 */
HDC WINAPI
DCIOpenProvider(void) {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/***********************************************************************
 *		DCICloseProvider (DCIMAN32.@)
 */
void WINAPI
DCICloseProvider(HDC hdc) {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return;
}

/**************************************************************************
 *                 DCICreatePrimary (DCIMAN32.@)
 */
int WINAPI 
DCICreatePrimary(HDC hdc, LPDCISURFACEINFO *pDciSurfaceInfo)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    FIXME("%p %p\n", hdc, pDciSurfaceInfo);
    return DCI_FAIL_UNSUPPORTED;
}
