/*
 * MSDMO.DLL - ReactOS DMO Runtime
 *
 * Copyright 2008 Dmitry Chapyshev
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

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <tchar.h>

typedef struct _DMO_PARTIAL_MEDIATYPE
{
  GUID type;
  GUID subtype;
} DMO_PARTIAL_MEDIATYPE, *PDMO_PARTIAL_MEDIATYPE;

typedef struct _IEnumDMO IEnumDMO;

typedef struct _DMOMediaType
{
    GUID majortype;
    GUID subtype;
    BOOL bFixedSizeSamples;
    BOOL bTemporalCompression;
    ULONG lSampleSize;
    GUID formattype;
    IUnknown *pUnk;
    ULONG cbFormat;
    BYTE *pbFormat;
} DMO_MEDIA_TYPE;

HRESULT WINAPI DMOEnum(
    REFGUID guidCategory,
    DWORD dwFlags,
    DWORD cInTypes,
    const DMO_PARTIAL_MEDIATYPE *pInTypes,
    DWORD cOutTypes,
    const DMO_PARTIAL_MEDIATYPE *pOutTypes,
    IEnumDMO **ppEnum)
{
	return S_OK;
}

HRESULT WINAPI DMOGetName(REFCLSID clsidDMO, WCHAR szName[])
{
	return S_OK;
}

HRESULT WINAPI DMOGetTypes(REFCLSID clsidDMO,
               ULONG ulInputTypesRequested,
               ULONG* pulInputTypesSupplied,
               DMO_PARTIAL_MEDIATYPE* pInputTypes,
               ULONG ulOutputTypesRequested,
               ULONG* pulOutputTypesSupplied,
               DMO_PARTIAL_MEDIATYPE* pOutputTypes)
{
    return S_OK;
}

HRESULT WINAPI DMORegister(
   LPCWSTR szName,
   REFCLSID clsidDMO,
   REFGUID guidCategory,
   DWORD dwFlags,
   DWORD cInTypes,
   const DMO_PARTIAL_MEDIATYPE *pInTypes,
   DWORD cOutTypes,
   const DMO_PARTIAL_MEDIATYPE *pOutTypes
)
{
    return S_OK;
}

HRESULT WINAPI DMOUnregister(REFCLSID clsidDMO, REFGUID guidCategory)
{
    return S_OK;
}

HRESULT WINAPI MoCopyMediaType(DMO_MEDIA_TYPE* pdst,
                               const DMO_MEDIA_TYPE* psrc)
{
    return S_OK;
}

HRESULT WINAPI MoCreateMediaType(DMO_MEDIA_TYPE** ppmedia, DWORD cbFormat)
{
    return S_OK;
}

HRESULT WINAPI MoDeleteMediaType(DMO_MEDIA_TYPE* pmedia)
{
    return S_OK;
}

HRESULT WINAPI MoDuplicateMediaType(DMO_MEDIA_TYPE** ppdst,
                                    const DMO_MEDIA_TYPE* psrc)
{
    return S_OK;
}

HRESULT WINAPI MoFreeMediaType(DMO_MEDIA_TYPE* pmedia)
{
    return S_OK;
}

HRESULT WINAPI MoInitMediaType(DMO_MEDIA_TYPE* pmedia, DWORD cbFormat)
{
    return S_OK;
}
