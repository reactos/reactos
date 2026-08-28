/*
 * Copyright (C) 2006 Dmitry Timoshkov
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
#include "mprapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mprapi);

/***********************************************************************
 * MprAdminGetErrorString (MPRAPI.@)
 *
 * Return a unicode string for the given mpr errorcode
 *
 * PARAMS
 *  mprerror [i] errorcode, for which a description is requested
 *  localstr [o] pointer, where a buffer with the error description is returned
 *
 * RETURNS
 *  Failure: ERROR_MR_MID_NOT_FOUND, when mprerror is not known
 *  Success: ERROR_SUCCESS, and in localstr a pointer to a buffer from LocalAlloc,
 *           which contains the error description.
 *
 * NOTES
 *  The caller must free the returned buffer with LocalFree
 *
 */
DWORD APIENTRY MprAdminGetErrorString(DWORD mprerror, LPWSTR *localstr)
{
    FIXME("(0x%lx/%lu, %p): stub!\n", mprerror, mprerror, localstr);

    *localstr = NULL;
    return ERROR_MR_MID_NOT_FOUND;
}

/***********************************************************************
 *      MprAdminIsServiceRunning (MPRAPI.@)
 */
BOOL APIENTRY MprAdminIsServiceRunning(LPWSTR server)
{
    FIXME("(%s): stub!\n", debugstr_w(server));

    return FALSE;
}

/***********************************************************************
 *      MprConfigServerConnect (MPRAPI.@)
 */
DWORD APIENTRY MprConfigServerConnect(LPWSTR server_name, HANDLE *hmprconfig)
{
    FIXME("server_name %s, hmprconfig %p stub.\n", debugstr_w(server_name), hmprconfig);

    *hmprconfig = NULL;

    return ERROR_NOT_SUPPORTED;
}
