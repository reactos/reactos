/*
 * clusapi main
 *
 * Copyright 2006 Benjamin Arai (Google)
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "clusapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clusapi);

/***********************************************************************
 *             GetClusterInformation   (CLUSAPI.@)
 *
 */
DWORD WINAPI GetClusterInformation(HCLUSTER hCluster, LPWSTR lpszClusterName,
                                   LPDWORD lpcchClusterName, LPCLUSTERVERSIONINFO lpClusterInfo)
{
    FIXME("(%p, %p, %p, %p) stub!\n", hCluster, lpszClusterName, lpcchClusterName, lpClusterInfo);

    *lpcchClusterName = 0;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *             GetNodeClusterState   (CLUSAPI.@)
 *
 * PARAMS
 *   lpszNodeName    [I] Optional Pointer to a NULL terminated unicode string
 *   pdwClusterState [O] Current state of the cluster
 *                        0x00 - Cluster not installed.
 *                        0x01 - Cluster not configured.
 *                        0x03 - Cluster not running.
 *                        0x13 - Cluster is running.
 */
DWORD WINAPI GetNodeClusterState(LPCWSTR lpszNodeName, LPDWORD pdwClusterState)
{
    FIXME("(%s,%p) stub!\n",debugstr_w(lpszNodeName),pdwClusterState);

    *pdwClusterState = 0;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *             OpenCluster   (CLUSAPI.@)
 *
 */
HCLUSTER WINAPI OpenCluster(LPCWSTR lpszClusterName)
{
    FIXME("(%s) stub!\n", debugstr_w(lpszClusterName));

    return (HCLUSTER)0xdeadbeef;
}

/***********************************************************************
 *             CloseCluster   (CLUSAPI.@)
 *
 */
BOOL WINAPI CloseCluster(HCLUSTER hCluster)
{
    FIXME("(%p) stub!\n", hCluster);

    return TRUE;
}

/***********************************************************************
 *             ClusterOpenEnum   (CLUSAPI.@)
 *
 */
HCLUSENUM WINAPI ClusterOpenEnum(HCLUSTER hCluster, DWORD dwType)
{
    FIXME("(%p, %lu) stub!\n", hCluster,dwType);

    return (HCLUSENUM)0xdeadbeef;
}

/***********************************************************************
 *             ClusterCloseEnum   (CLUSAPI.@)
 *
 */
DWORD WINAPI ClusterCloseEnum(HCLUSENUM hEnum)
{
    FIXME("(%p) stub!\n", hEnum);

    return ERROR_SUCCESS;
}

/***********************************************************************
 *             ClusterEnum   (CLUSAPI.@)
 *
 */
DWORD WINAPI ClusterEnum(HCLUSENUM hEnum, DWORD dwIndex, LPDWORD lpdwType, LPWSTR lpszName, LPDWORD lpcchName)
{
    FIXME("(%p, %lu, %p, %p, %lu) stub!\n", hEnum, dwIndex, lpdwType, lpszName, *lpcchName);

    return ERROR_NO_MORE_ITEMS;
}
