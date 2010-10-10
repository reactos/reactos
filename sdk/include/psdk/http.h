/*
 * HTTP Server API definitions
 *
 * Copyright (C) 2009 Andrey Turkin
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

#ifndef __WINE_HTTP_H
#define __WINE_HTTP_H

#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HTTPAPI_VERSION
{
    USHORT HttpApiMajorVersion;
    USHORT HttpApiMinorVersion;
} HTTPAPI_VERSION, *PHTTPAPI_VERSION;

#define HTTPAPI_VERSION_1 {1,0}
#define HTTPAPI_VERSION_2 {2,0}

/* HttpInitialize and HttpTerminate flags */
#define HTTP_INITIALIZE_SERVER 0x00000001
#define HTTP_INITIALIZE_CONFIG 0x00000002

typedef enum _HTTP_SERVICE_CONFIG_ID
{
    HttpServiceConfigIPListenList,
    HttpServiceConfigSSLCertInfo,
    HttpServiceConfigUrlAclInfo,
    HttpServiceConfigTimeout,
    HttpServiceConfigMax
} HTTP_SERVICE_CONFIG_ID, *PHTTP_SERVICE_CONFIG_ID;

ULONG WINAPI HttpInitialize(HTTPAPI_VERSION,ULONG,PVOID);
ULONG WINAPI HttpTerminate(ULONG,PVOID);

ULONG WINAPI HttpDeleteServiceConfiguration(HANDLE,HTTP_SERVICE_CONFIG_ID,PVOID,ULONG,LPOVERLAPPED);
ULONG WINAPI HttpQueryServiceConfiguration(HANDLE,HTTP_SERVICE_CONFIG_ID,PVOID,ULONG,PVOID,ULONG,PULONG,LPOVERLAPPED);
ULONG WINAPI HttpSetServiceConfiguration(HANDLE,HTTP_SERVICE_CONFIG_ID,PVOID,ULONG,LPOVERLAPPED);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_HTTP_H */
