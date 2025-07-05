/* DirectPlay NAT Helper Past Main
 *
 * Copyright (C) 2003 Rok Mandeljc
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

WINE_DEFAULT_DEBUG_CHANNEL(dpnhpast);

/******************************************************************
 *		DirectPlayNATHelpCreate (DPNHPAST.1)
 *
 *
 */
HRESULT WINAPI DirectPlayNATHelpCreate(LPCGUID pIID, PVOID *ppvInterface)
{
    TRACE("(%p, %p) stub\n", pIID, ppvInterface);
    return E_NOTIMPL;
}


/******************************************************************
 *		DllGetClassObject (DPNHPAST.@)
 *
 *
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	FIXME(":stub\n");
	return E_FAIL;
}
