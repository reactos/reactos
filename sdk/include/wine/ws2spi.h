/*
 * WS2SPI.H -- definitions to be used with the WinSock service provider.
 *
 * Copyright (C) 2001 Patrik Stridvall
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

#ifndef _WINSOCK2SPI_
#define _WINSOCK2SPI_

#ifndef _WINSOCK2API_
#include <winsock2.h>
#endif /* !defined(_WINSOCK2API_) */

#include <pshpack4.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef BOOL (WINAPI *LPWPUPOSTMESSAGE)(HWND,UINT,WPARAM,LPARAM);

typedef enum _WSC_PROVIDER_INFO_TYPE
{
    ProviderInfoLspCategories,
    ProviderInfoAudit,
} WSC_PROVIDER_INFO_TYPE;

WSAEVENT WINAPI WPUCompleteOverlappedRequest(SOCKET,LPWSAOVERLAPPED,DWORD,DWORD,LPINT);
INT      WINAPI WSCInstallProvider(const LPGUID,LPCWSTR,const LPWSAPROTOCOL_INFOW,
                                   DWORD,LPINT);
INT      WINAPI WSCDeinstallProvider(LPGUID,LPINT);
INT      WINAPI WSCEnableNSProvider(LPGUID,BOOL);
INT      WINAPI WSCEnumProtocols(LPINT,LPWSAPROTOCOL_INFOW,LPDWORD,LPINT);
INT      WINAPI WSCGetApplicationCategory(LPCWSTR,DWORD,LPCWSTR,DWORD,DWORD*,LPINT);
INT      WINAPI WSCGetProviderInfo(LPGUID,WSC_PROVIDER_INFO_TYPE,PBYTE,size_t*,DWORD,LPINT);
INT      WINAPI WSCGetProviderPath(LPGUID,LPWSTR,LPINT,LPINT);
INT      WINAPI WSCInstallNameSpace(LPWSTR,LPWSTR,DWORD,DWORD,LPGUID);
INT      WINAPI WSCSetApplicationCategory(LPCWSTR,DWORD,LPCWSTR,DWORD,DWORD,DWORD*,LPINT);
INT      WINAPI WSCUnInstallNameSpace(LPGUID);
INT      WINAPI WSCUpdateProvider(LPGUID, const WCHAR *, const LPWSAPROTOCOL_INFOW, DWORD, LPINT);
INT      WINAPI WSCWriteProviderOrder(LPDWORD,DWORD);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#include <poppack.h>

#endif /* !defined(_WINSOCK2SPI_) */
