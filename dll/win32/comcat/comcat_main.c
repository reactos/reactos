/*
 *	exported dll functions for comcat.dll
 *
 * Copyright (C) 2002-2003 John K. Hohm
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

#include "comcat_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

LONG dll_ref = 0;

/***********************************************************************
 *		Global string constant definitions
 */
const WCHAR clsid_keyname[6] = { 'C', 'L', 'S', 'I', 'D', 0 };

/***********************************************************************
 *		DllGetClassObject (COMCAT.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualGUID(rclsid, &CLSID_StdComponentCategoriesMgr)) {
	return IClassFactory_QueryInterface((LPCLASSFACTORY)&COMCAT_ClassFactory, iid, ppv);
    }
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllCanUnloadNow (COMCAT.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return dll_ref != 0 ? S_FALSE : S_OK;
}

/* NOTE: DllRegisterServer and DllUnregisterServer are in regsvr.c */
