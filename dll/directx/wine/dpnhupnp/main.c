/*
 *
 * Copyright (C) 2021 Alistair Leslie-Hughes
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnhupnp);

HRESULT WINAPI DirectPlayNATHelpCreate(LPCGUID pIID, void **ppv)
{
    FIXME("(%p, %p) stub\n", pIID, ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    FIXME("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return E_FAIL;
}
