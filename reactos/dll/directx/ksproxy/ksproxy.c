/*
 * KSPROXY.AX - ReactOS WDM Streaming ActiveMovie Proxy
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
#include <ks.h>
#include <dshow.h>

HRESULT WINAPI
KsSynchronousDeviceControl(
    HANDLE     Handle,
    ULONG      IoControl,
    PVOID      InBuffer,
    ULONG      InLength,
    PVOID      OutBuffer,
    ULONG      OutLength,
    PULONG     BytesReturned)
{
    return NOERROR;
}

HRESULT WINAPI
KsResolveRequiredAttributes(
    PKSDATARANGE     DataRange,
    KSMULTIPLE_ITEM  *Attributes OPTIONAL)
{
    return NOERROR;
}

HRESULT WINAPI
KsOpenDefaultDevice(
    REFGUID      Category,
    ACCESS_MASK  Access,
    PHANDLE      DeviceHandle)
{
    return NOERROR;
}

HRESULT WINAPI
KsGetMultiplePinFactoryItems(
    HANDLE  FilterHandle,
    ULONG   PinFactoryId,
    ULONG   PropertyId,
    PVOID   *Items)
{
    return NOERROR;
}

HRESULT WINAPI
KsGetMediaTypeCount(
    HANDLE  FilterHandle,
    ULONG   PinFactoryId,
    ULONG   *MediaTypeCount)
{
    return NOERROR;
}

HRESULT WINAPI
KsGetMediaType(
    int  Position,
    AM_MEDIA_TYPE  *AmMediaType,
    HANDLE         FilterHandle,
    ULONG          PinFactoryId)
{
    return NOERROR;
}

HRESULT WINAPI
DllUnregisterServer(void)
{
    return S_OK;
}

HRESULT WINAPI
DllRegisterServer(void)
{
    return S_OK;
}

HRESULT WINAPI
DllGetClassObject(
	REFCLSID rclsid,
	REFIID riid,
	LPVOID *ppv)
{
    return S_OK;
}

HRESULT WINAPI
DllCanUnloadNow(void)
{
    return S_OK;
}

