/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#ifndef __NFS41_NP_H__
#define __NFS41_NP_H__

#define NFS41NP_MUTEX_NAME  "NFS41NPMUTEX"

#define NFS41NP_MAX_DEVICES 26

typedef struct __NFS41NP_NETRESOURCE {
    BOOL    InUse;
    USHORT  LocalNameLength;
    USHORT  RemoteNameLength;
    USHORT  ConnectionNameLength;
    DWORD   dwScope;
    DWORD   dwType;
    DWORD   dwDisplayType;
    DWORD   dwUsage;
    WCHAR   LocalName[MAX_PATH];
    WCHAR   RemoteName[MAX_PATH];
    WCHAR   ConnectionName[MAX_PATH];
    WCHAR   Options[MAX_PATH];
} NFS41NP_NETRESOURCE, *PNFS41NP_NETRESOURCE;

typedef struct __NFS41NP_SHARED_MEMORY {
    INT                 NextAvailableIndex;
    INT                 NumberOfResourcesInUse;
    NFS41NP_NETRESOURCE NetResources[NFS41NP_MAX_DEVICES];
} NFS41NP_SHARED_MEMORY, *PNFS41NP_SHARED_MEMORY;

#endif /* !__NFS41_NP_H__ */
