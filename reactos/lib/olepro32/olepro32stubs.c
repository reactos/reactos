/*
 * OlePro32 Stubs
 *
 * Copyright 1999 Corel Corporation
 *
 * Sean Langley
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

#define COM_NO_WINDOWS_H
#include <stdarg.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 *		DllUnregisterServer (OLEPRO32.258)
 */
HRESULT WINAPI OLEPRO32_DllUnregisterServer()
{
	FIXME("not implemented (olepro32.dll) \n");
	return S_OK;
}

/***********************************************************************
 *		DllRegisterServer (OLEPRO32.257)
 */
HRESULT WINAPI OLEPRO32_DllRegisterServer()
{
	FIXME("not implemented (olepro32.dll) \n");
	return S_OK;
}

/***********************************************************************
 *		DllCanUnloadNow (OLEPRO32.255)
 */
HRESULT WINAPI OLEPRO32_DllCanUnloadNow( )
{
	FIXME("not implemented (olepro32.dll) \n");
	return S_OK;
}

/***********************************************************************
 *		DllGetClassObject (OLEPRO32.256)
 */
HRESULT WINAPI OLEPRO32_DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppv )
{
	FIXME("not implemented (olepro32.dll) \n");
	return S_OK;
}
