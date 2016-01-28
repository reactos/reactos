/*
 * Copyright 2014 Qian Hong for CodeWeavers
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

#ifndef __WINE_ATLCOM_H__
#define __WINE_ATLCOM_H__

#ifndef __WINE_ATLBASE_H__
# error You must include atlbase.h to use this header
#endif

typedef struct ATL_PROPMAP_ENTRY
{
    LPCOLESTR szDesc;
#if _ATL_VER < _ATL_VER_100
    DISPID dispid;
    const CLSID *pclsidPropPage;
    const IID *piidDispatch;
#else
    const CLSID *pclsidPropPage;
    const IID *piidDispatch;
    void *rgclsidAllowed;
    DWORD cclsidAllowed;
    DISPID dispid;
#endif
    DWORD dwOffsetData;
    DWORD dwSizeData;
    VARTYPE vt;
} ATL_PROPMAP_ENTRY;

HRESULT WINAPI AtlIPersistStreamInit_Load(IStream*, ATL_PROPMAP_ENTRY*, void*, IUnknown*);
HRESULT WINAPI AtlIPersistStreamInit_Save(IStream*, BOOL, ATL_PROPMAP_ENTRY*, void*, IUnknown*);
HRESULT WINAPI AtlIPersistPropertyBag_Load(IPropertyBag*, IErrorLog*, ATL_PROPMAP_ENTRY*, void*, IUnknown*);
HRESULT WINAPI AtlIPersistPropertyBag_Save(IPropertyBag*, BOOL, BOOL, ATL_PROPMAP_ENTRY*, void*, IUnknown*);

#endif /* __WINE_ATLCOM_H__ */
