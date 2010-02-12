/*
 * nddeapi main
 *
 * Copyright 2006 Benjamin Arai (Google)
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nddeapi);

/***********************************************************************
 *             NDdeGetErrorStringA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeGetErrorStringA(UINT uErrorCode, LPSTR lpszErrorString, DWORD cBufSize)
{
    FIXME("(%u, %s, %d): stub!\n",uErrorCode,debugstr_a(lpszErrorString), cBufSize);

    return E_NOTIMPL;
}

/***********************************************************************
 *             NDdeGetErrorStringW   (NDDEAPI.@)
 *
*/
UINT WINAPI NDdeGetErrorStringW(UINT uErrorCode, LPWSTR lpszErrorString, DWORD cBufSize)
{
    FIXME("(%u, %s, %d): stub!\n",uErrorCode,debugstr_w(lpszErrorString), cBufSize);

    return E_NOTIMPL;
}
