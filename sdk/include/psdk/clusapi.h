/*
 * Copyright 2006 James Hawkins
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

#ifndef __WINE_CLUSAPI_H
#define __WINE_CLUSAPI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HCLUSTER *HCLUSTER;
typedef struct _HCLUSENUM *HCLUSENUM;


typedef struct _CLUSTERVERSIONINFO
{
    DWORD   dwVersionInfoSize;
    WORD    MajorVersion;
    WORD    MinorVersion;
    WORD    BuildNumber;
    WCHAR   szVendorId[64];
    WCHAR   szCSDVersion[64];
    DWORD   dwClusterHighestVersion;
    DWORD   dwClusterLowestVersion;
    DWORD   dwFlags;
    DWORD   dwReserved;
} CLUSTERVERSIONINFO, *LPCLUSTERVERSIONINFO;

BOOL WINAPI CloseCluster(HCLUSTER hCluster);
DWORD WINAPI GetClusterInformation(HCLUSTER hCluster, LPWSTR lpszClusterName,
                                   LPDWORD lpcchClusterName, LPCLUSTERVERSIONINFO lpClusterInfo);
DWORD WINAPI GetNodeClusterState(LPCWSTR lpszNodeName, LPDWORD pdwClusterState);
HCLUSTER WINAPI OpenCluster(LPCWSTR lpszClusterName);
HCLUSENUM WINAPI ClusterOpenEnum(HCLUSTER hCluster, DWORD dwType);
DWORD WINAPI ClusterEnum(HCLUSENUM hEnum, DWORD dwIndex, LPDWORD lpdwType, LPWSTR lpszName, LPDWORD lpcchName);
DWORD WINAPI ClusterCloseEnum(HCLUSENUM hEnum);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_CLUSAPI_H */
