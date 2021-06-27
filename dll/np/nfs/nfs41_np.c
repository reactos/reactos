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

#include <windows.h>
#include <npapi.h>
#include <devioctl.h>
#include <strsafe.h>

#include "nfs41_driver.h"
#include "nfs41_np.h"
#include "options.h"

#include <pseh/pseh2.h>

#ifdef DBG
#define DbgP(_x_) NFS41DbgPrint _x_
#else
#define DbgP(_x_)
#endif
#define TRACE_TAG   L"[NFS41_NP]"
#define WNNC_DRIVER( major, minor ) ( major * 0x00010000 + minor )


ULONG _cdecl NFS41DbgPrint( __in LPTSTR Format, ... )
{
    ULONG rc = 0;
    TCHAR szbuffer[256];

    va_list marker;
    va_start( marker, Format );
    {

        //StringCchVPrintfW( szbuffer, 127, Format, marker );
        StringCchVPrintfW( szbuffer, 256, Format, marker );
        szbuffer[255] = (TCHAR)0;
        OutputDebugString( TRACE_TAG );
        OutputDebugString( szbuffer );
    }

    return rc;
}

int filter(unsigned int code)
{
    DbgP((L"####Got exception %u\n", code));
    return EXCEPTION_CONTINUE_SEARCH;
}

DWORD
OpenSharedMemory(
    PHANDLE phMutex,
    PHANDLE phMemory,
    PVOID   *pMemory)
/*++

Routine Description:

    This routine opens the shared memory for exclusive manipulation

Arguments:

    phMutex - the mutex handle

    phMemory - the memory handle

    pMemory - a ptr. to the shared memory which is set if successful

Return Value:

    WN_SUCCESS -- if successful

--*/
{
    DWORD dwStatus;

    *phMutex = 0;
    *phMemory = 0;
    *pMemory = NULL;

    *phMutex = CreateMutex(NULL, FALSE, TEXT(NFS41NP_MUTEX_NAME));
    if (*phMutex == NULL)
    {
        dwStatus = GetLastError();
        DbgP((TEXT("OpenSharedMemory:  OpenMutex failed\n")));
        goto OpenSharedMemoryAbort1;
    }

    WaitForSingleObject(*phMutex, INFINITE);

    *phMemory = OpenFileMapping(FILE_MAP_WRITE,
                                FALSE,
                                TEXT(NFS41_USER_SHARED_MEMORY_NAME));
    if (*phMemory == NULL)
    {
        dwStatus = GetLastError();
        DbgP((TEXT("OpenSharedMemory:  OpenFileMapping failed\n")));
        goto OpenSharedMemoryAbort2;
    }

    *pMemory = MapViewOfFile(*phMemory, FILE_MAP_WRITE, 0, 0, 0);
    if (*pMemory == NULL)
    {
        dwStatus = GetLastError();
        DbgP((TEXT("OpenSharedMemory:  MapViewOfFile failed\n")));
        goto OpenSharedMemoryAbort3;
    }

    return ERROR_SUCCESS;

OpenSharedMemoryAbort3:
    CloseHandle(*phMemory);

OpenSharedMemoryAbort2:
    ReleaseMutex(*phMutex);
    CloseHandle(*phMutex);
    *phMutex = NULL;

OpenSharedMemoryAbort1:
    DbgP((TEXT("OpenSharedMemory: return dwStatus: %d\n"), dwStatus));

    return dwStatus;
}

VOID
CloseSharedMemory(
    PHANDLE hMutex,
    PHANDLE hMemory,
    PVOID   *pMemory)
/*++

Routine Description:

    This routine relinquishes control of the shared memory after exclusive
    manipulation

Arguments:

    hMutex - the mutex handle

    hMemory  - the memory handle

    pMemory - a ptr. to the shared memory which is set if successful

Return Value:

--*/
{
    if (*pMemory)
    {
        UnmapViewOfFile(*pMemory);
        *pMemory = NULL;
    }
    if (*hMemory)
    {
        CloseHandle(*hMemory);
        *hMemory = 0;
    }
    if (*hMutex)
    {
        if (ReleaseMutex(*hMutex) == FALSE)
        {
            DbgP((TEXT("CloseSharedMemory: ReleaseMutex error: %d\n"), GetLastError()));
        }
        CloseHandle(*hMutex);
        *hMutex = 0;
    }
}

static DWORD StoreConnectionInfo(
    IN LPCWSTR LocalName,
    IN LPCWSTR ConnectionName,
    IN USHORT ConnectionNameLength,
    IN LPNETRESOURCE lpNetResource)
{
    DWORD status;
    HANDLE hMutex, hMemory;
    PNFS41NP_SHARED_MEMORY pSharedMemory;
    PNFS41NP_NETRESOURCE pNfs41NetResource;
    INT Index;
    BOOLEAN FreeEntryFound = FALSE;

#ifdef __REACTOS__
    status = OpenSharedMemory(&hMutex, &hMemory, (PVOID *)&pSharedMemory);
#else
    status = OpenSharedMemory(&hMutex, &hMemory, &(PVOID)pSharedMemory);
#endif
    if (status)
        goto out;

    DbgP((TEXT("StoreConnectionInfo: NextIndex %d, NumResources %d\n"),
        pSharedMemory->NextAvailableIndex,
        pSharedMemory->NumberOfResourcesInUse));

    for (Index = 0; Index < pSharedMemory->NextAvailableIndex; Index++)
    {
        if (!pSharedMemory->NetResources[Index].InUse)
        {
            FreeEntryFound = TRUE;
            DbgP((TEXT("Reusing existing index %d\n"), Index));
            break;
        }
    }

    if (!FreeEntryFound)
    {
        if (pSharedMemory->NextAvailableIndex >= NFS41NP_MAX_DEVICES) {
            status = WN_NO_MORE_DEVICES;
            goto out_close;
        }
        Index = pSharedMemory->NextAvailableIndex++;
        DbgP((TEXT("Using new index %d\n"), Index));
    }

    pSharedMemory->NumberOfResourcesInUse += 1;

    pNfs41NetResource = &pSharedMemory->NetResources[Index];

    pNfs41NetResource->InUse                = TRUE;
    pNfs41NetResource->dwScope              = lpNetResource->dwScope;
    pNfs41NetResource->dwType               = lpNetResource->dwType;
    pNfs41NetResource->dwDisplayType        = lpNetResource->dwDisplayType;
    pNfs41NetResource->dwUsage              = RESOURCEUSAGE_CONNECTABLE;
    pNfs41NetResource->LocalNameLength      = (USHORT)(wcslen(LocalName) + 1) * sizeof(WCHAR);
    pNfs41NetResource->RemoteNameLength     = (USHORT)(wcslen(lpNetResource->lpRemoteName) + 1) * sizeof(WCHAR);
    pNfs41NetResource->ConnectionNameLength = ConnectionNameLength;

    StringCchCopy(pNfs41NetResource->LocalName,
        pNfs41NetResource->LocalNameLength,
        LocalName);
    StringCchCopy(pNfs41NetResource->RemoteName,
        pNfs41NetResource->RemoteNameLength,
        lpNetResource->lpRemoteName);
    StringCchCopy(pNfs41NetResource->ConnectionName,
        pNfs41NetResource->ConnectionNameLength,
        ConnectionName);

    // TODO: copy mount options -cbodley

out_close:
#ifdef __REACTOS__
    CloseSharedMemory(&hMutex, &hMemory, (PVOID *)&pSharedMemory);
#else
    CloseSharedMemory(&hMutex, &hMemory, &(PVOID)pSharedMemory);
#endif
out:
    return status;
}

ULONG
SendTo_NFS41Driver(
    IN ULONG            IoctlCode,
    IN PVOID            InputDataBuf,
    IN ULONG            InputDataLen,
    IN PVOID            OutputDataBuf,
    IN PULONG           pOutputDataLen)
{
    HANDLE  DeviceHandle;       // The mini rdr device handle
    BOOL    rc = FALSE;
    ULONG   Status;

    Status = WN_SUCCESS;
    DbgP((L"[aglo] calling CreateFile\n"));
    DeviceHandle = CreateFile(
        NFS41_USER_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        (LPSECURITY_ATTRIBUTES)NULL,
        OPEN_EXISTING,
        0,
        (HANDLE) NULL );

    DbgP((L"[aglo] after CreateFile Device Handle\n"));
    if ( INVALID_HANDLE_VALUE != DeviceHandle )
    {
        _SEH2_TRY {
        DbgP((L"[aglo] calling DeviceIoControl\n"));
        rc = DeviceIoControl(
            DeviceHandle,
            IoctlCode,
            InputDataBuf,
            InputDataLen,
            OutputDataBuf,
            *pOutputDataLen,
            pOutputDataLen,
            NULL );
        } _SEH2_EXCEPT(_SEH2_GetExceptionCode()) {
            DbgP((L"#### In except\n"));
        } _SEH2_END;
        DbgP((L"[aglo] returned from DeviceIoControl %08lx\n", rc));
            if ( !rc )
            {
                DbgP((L"[aglo] SendTo_NFS41Driver: returning error from DeviceIoctl\n"));
                Status = GetLastError( );
            }
            else
            {
                DbgP((L"[aglo] SendTo_NFS41Driver: The DeviceIoctl call succeded\n"));
            }
            CloseHandle(DeviceHandle);
    }
    else
    {
        Status = GetLastError( );
        DbgP((L"[aglo] SendTo_NFS41Driver: error %08lx opening device \n", Status));
    }
    DbgP((L"[aglo] returned from SendTo_NFS41Driver %08lx\n", Status));
    return Status;
}

DWORD APIENTRY
NPGetCaps(
    DWORD nIndex )
{
   DWORD rc = 0;

#ifndef __REACTOS__
    DbgP(( L"[aglo] GetNetCaps %d\n", nIndex ));
#endif
    switch ( nIndex )
    {
        case WNNC_SPEC_VERSION:
            rc = WNNC_SPEC_VERSION51;
            break;

        case WNNC_NET_TYPE:
            rc = WNNC_NET_RDR2SAMPLE;
            break;

        case WNNC_DRIVER_VERSION:
            rc = WNNC_DRIVER(1, 0);
            break;

        case WNNC_CONNECTION:
            rc = WNNC_CON_GETCONNECTIONS |
                 WNNC_CON_CANCELCONNECTION |
                 WNNC_CON_ADDCONNECTION |
                 WNNC_CON_ADDCONNECTION3;
            break;

        case WNNC_ENUMERATION:
            rc = WNNC_ENUM_LOCAL;
            break;

        case WNNC_START:
            rc = 1;
            break;

        case WNNC_USER:
        case WNNC_DIALOG:
        case WNNC_ADMIN:
        default:
            rc = 0;
            break;
    }

    return rc;
}

DWORD APIENTRY
NPLogonNotify(
    __in PLUID   lpLogonId,
    __in PCWSTR lpAuthentInfoType,
    __in PVOID  lpAuthentInfo,
    __in PCWSTR lpPreviousAuthentInfoType,
    __in PVOID  lpPreviousAuthentInfo,
    __in PWSTR  lpStationName,
    __in PVOID  StationHandle,
    __out PWSTR  *lpLogonScript)
{
    *lpLogonScript = NULL;
    DbgP(( L"[aglo] NPLogonNotify: returning WN_SUCCESS\n" ));
    return WN_SUCCESS;
}

DWORD APIENTRY
NPPasswordChangeNotify (
    __in LPCWSTR lpAuthentInfoType,
    __in LPVOID  lpAuthentInfo,
    __in LPCWSTR lpPreviousAuthentInfoType,
    __in LPVOID  lpPreviousAuthentInfo,
    __in LPWSTR  lpStationName,
    LPVOID  StationHandle,
    DWORD   dwChangeInfo )
{
    DbgP(( L"[aglo] NPPasswordChangeNotify: WN_NOT_SUPPORTED\n" ));
    SetLastError( WN_NOT_SUPPORTED );
    return WN_NOT_SUPPORTED;
}

#ifdef __REACTOS__
DWORD APIENTRY
NPAddConnection3(
    __in HWND           hwndOwner,
    __in LPNETRESOURCE  lpNetResource,
    __in_opt LPWSTR     lpPassword,
    __in_opt LPWSTR     lpUserName,
    __in DWORD          dwFlags);
#endif

DWORD APIENTRY
NPAddConnection(
    __in LPNETRESOURCE   lpNetResource,
    __in_opt LPWSTR      lpPassword,
    __in_opt LPWSTR      lpUserName )
{
    return NPAddConnection3( NULL, lpNetResource, lpPassword, lpUserName, 0 );
}

DWORD APIENTRY
NPAddConnection3(
    __in HWND           hwndOwner,
    __in LPNETRESOURCE  lpNetResource,
    __in_opt LPWSTR     lpPassword,
    __in_opt LPWSTR     lpUserName,
    __in DWORD          dwFlags)
{
    DWORD   Status;
    WCHAR   wszScratch[128];
    WCHAR   LocalName[3];
    DWORD   CopyBytes = 0;
    CONNECTION_INFO Connection;
    LPWSTR  ConnectionName;
    WCHAR ServerName[MAX_PATH];
    PWCHAR p;
    DWORD i;

    DbgP(( L"[aglo] NPAddConnection3('%s', '%s')\n",
        lpNetResource->lpLocalName, lpNetResource->lpRemoteName ));
    DbgP(( L"[aglo] username = '%s', passwd = '%s'\n", lpUserName, lpPassword));

    Status = InitializeConnectionInfo(&Connection,
        (PMOUNT_OPTION_BUFFER)lpNetResource->lpComment,
        &ConnectionName);
    if (Status)  {
        DbgP(( L"InitializeConnectionInfo failed with %d\n", Status ));
        goto out;
    }

    //  \device\miniredirector\;<DriveLetter>:\Server\Share

    // local name, must start with "X:"
    if (lstrlen(lpNetResource->lpLocalName) < 2 ||
        lpNetResource->lpLocalName[1] != L':') {
        Status = WN_BAD_LOCALNAME;
        goto out;
    }

    LocalName[0] = (WCHAR) toupper(lpNetResource->lpLocalName[0]);
    LocalName[1] = L':';
    LocalName[2] = L'\0';
    StringCchCopyW( ConnectionName, MAX_PATH, NFS41_DEVICE_NAME );
    StringCchCatW( ConnectionName, MAX_PATH, L"\\;" );
    StringCchCatW( ConnectionName, MAX_PATH, LocalName );

    // remote name, must start with "\\"
    if (lpNetResource->lpRemoteName[0] == L'\0' ||
        lpNetResource->lpRemoteName[0] != L'\\' ||
        lpNetResource->lpRemoteName[1] != L'\\') {
        Status = WN_BAD_NETNAME;
        goto out;
    }

    /* note: remotename comes as \\server but we need to add \server thus +1 pointer */
    p = lpNetResource->lpRemoteName + 1;
    ServerName[0] = L'\\';
    i = 1;
    for(;;) {
        /* convert servername ending unix slash to windows slash */
        if (p[i] == L'/')
            p[i] = L'\\';
        /* deal with servername ending with any slash */
        if (p[i] == L'\0')
            p[i] = L'\\';
        ServerName[i] = p[i];
        if (p[i] == L'\\') break;
        i++;
    }
    ServerName[i] = L'\0';
    StringCchCatW( ConnectionName, MAX_PATH, ServerName);
    /* insert the "nfs4" in between the server name and the path,
     * just to make sure all calls to our driver come thru this */
    StringCchCatW( ConnectionName, MAX_PATH, L"\\nfs4" );

#ifdef CONVERT_2_UNIX_SLASHES
    /* convert all windows slashes to unix slashes */
    {
        PWCHAR q = p;
        DWORD j = 0;
        for(;;) {
            if(q[j] == L'\0') break;
            if (q[j] == L'\\') q[j] = L'/';
            j++;
        }
    }
#else
    /* convert all unix slashes to windows slashes */
    {
        PWCHAR q = p;
        DWORD j = 0;
        for(;;) {
            if(q[j] == L'\0') break;
            if (q[j] == L'/') q[j] = L'\\';
            j++;
        }
    }
#endif
    StringCchCatW( ConnectionName, MAX_PATH, &p[i]);
    DbgP(( L"[aglo] Full Connect Name: %s\n", ConnectionName ));
    DbgP(( L"[aglo] Full Connect Name Length: %d %d\n",
        (wcslen(ConnectionName) + 1) * sizeof(WCHAR),
        (lstrlen(ConnectionName) + 1) * sizeof(WCHAR)));

    if ( QueryDosDevice( LocalName, wszScratch, 128 )
        || GetLastError() != ERROR_FILE_NOT_FOUND) {
        Status = WN_ALREADY_CONNECTED;
        goto out;
    }

    MarshalConnectionInfo(&Connection);

    Status = SendTo_NFS41Driver( IOCTL_NFS41_ADDCONN,
        Connection.Buffer, Connection.BufferSize,
        NULL, &CopyBytes );
    if (Status) {
        DbgP(( L"[aglo] SendTo_NFS41Driver failed with %d\n", Status));
        goto out;
    }

    DbgP(( L"[aglo] calling DefineDosDevice\n"));
    if ( !DefineDosDevice( DDD_RAW_TARGET_PATH |
                           DDD_NO_BROADCAST_SYSTEM,
                           lpNetResource->lpLocalName,
                           ConnectionName ) ) {
        Status = GetLastError();
        DbgP(( L"[aglo] DefineDosDevice failed with %d\n", Status));
        goto out_delconn;
    }

    // The connection was established and the local device mapping
    // added. Include this in the list of mapped devices.
    Status = StoreConnectionInfo(LocalName, ConnectionName,
        Connection.Buffer->NameLength, lpNetResource);
    if (Status) {
        DbgP(( L"[aglo] StoreConnectionInfo failed with %d\n", Status));
        goto out_undefine;
    }

out:
    FreeConnectionInfo(&Connection);
    DbgP(( L"[aglo] NPAddConnection3: status %08X\n", Status));
    return Status;
out_undefine:
    DefineDosDevice(DDD_REMOVE_DEFINITION | DDD_RAW_TARGET_PATH |
        DDD_EXACT_MATCH_ON_REMOVE, LocalName, ConnectionName);
out_delconn:
    SendTo_NFS41Driver(IOCTL_NFS41_DELCONN, ConnectionName,
        Connection.Buffer->NameLength, NULL, &CopyBytes);
    goto out;
}

DWORD APIENTRY
NPCancelConnection(
    __in LPWSTR  lpName,
    __in BOOL    fForce )
{
    DWORD   Status = 0;

    HANDLE  hMutex, hMemory;
    PNFS41NP_SHARED_MEMORY  pSharedMemory;

    DbgP((TEXT("NPCancelConnection\n")));
    DbgP((TEXT("NPCancelConnection: ConnectionName: %S\n"), lpName));

    Status = OpenSharedMemory( &hMutex,
                               &hMemory,
                               (PVOID)&pSharedMemory);

    if (Status == WN_SUCCESS)
    {
        INT  Index;
        PNFS41NP_NETRESOURCE pNetResource;
        Status = WN_NOT_CONNECTED;

        DbgP((TEXT("NPCancelConnection: NextIndex %d, NumResources %d\n"),
                    pSharedMemory->NextAvailableIndex,
                    pSharedMemory->NumberOfResourcesInUse));

        for (Index = 0; Index < pSharedMemory->NextAvailableIndex; Index++)
        {
            pNetResource = &pSharedMemory->NetResources[Index];

            if (pNetResource->InUse)
            {
                if ( ( (wcslen(lpName) + 1) * sizeof(WCHAR) ==
                        pNetResource->LocalNameLength)
                        && ( !wcscmp(lpName, pNetResource->LocalName) ))
                {
                    ULONG CopyBytes;

                    DbgP((TEXT("NPCancelConnection: Connection Found:\n")));

                    CopyBytes = 0;

                    Status = SendTo_NFS41Driver( IOCTL_NFS41_DELCONN,
                                pNetResource->ConnectionName,
                                pNetResource->ConnectionNameLength,
                                NULL,
                                &CopyBytes );

                    if (Status != WN_SUCCESS)
                    {
                        DbgP((TEXT("NPCancelConnection: SendToMiniRdr returned Status %lx\n"),Status));
                        break;
                    }

                    if (DefineDosDevice(DDD_REMOVE_DEFINITION | DDD_RAW_TARGET_PATH | DDD_EXACT_MATCH_ON_REMOVE,
                            lpName,
                            pNetResource->ConnectionName) == FALSE)
                    {
                        DbgP((TEXT("RemoveDosDevice:  DefineDosDevice error: %d\n"), GetLastError()));
                        Status = GetLastError();
                    }
                    else
                    {
                        pNetResource->InUse = FALSE;
                        pSharedMemory->NumberOfResourcesInUse--;

                        if (Index+1 == pSharedMemory->NextAvailableIndex)
                            pSharedMemory->NextAvailableIndex--;
                    }
                    break;
                }

                DbgP((TEXT("NPCancelConnection: Name %S EntryName %S\n"),
                            lpName,pNetResource->LocalName));
#ifndef __REACTOS__
                DbgP((TEXT("NPCancelConnection: Name Length %d Entry Name Length %d\n"),
                           pNetResource->LocalNameLength,pNetResource->LocalName));
#else
                DbgP((TEXT("NPCancelConnection: Name Length %d Entry Name Length %d\n"),
                           (wcslen(lpName) + 1) * sizeof(WCHAR), pNetResource->LocalNameLength));
#endif

            }
        }

        CloseSharedMemory( &hMutex,
                           &hMemory,
                          (PVOID)&pSharedMemory);
    }

    return Status;
}

DWORD APIENTRY
NPGetConnection(
    __in LPWSTR  lpLocalName,
    __out_bcount(*lpBufferSize) LPWSTR  lpRemoteName,
    __inout LPDWORD lpBufferSize )
{
    DWORD   Status = 0;

    HANDLE  hMutex, hMemory;
    PNFS41NP_SHARED_MEMORY  pSharedMemory;

    Status = OpenSharedMemory( &hMutex,
                               &hMemory,
                               (PVOID)&pSharedMemory);

    if (Status == WN_SUCCESS)
    {
        INT  Index;
        PNFS41NP_NETRESOURCE pNetResource;
        Status = WN_NOT_CONNECTED;

        for (Index = 0; Index < pSharedMemory->NextAvailableIndex; Index++)
        {
            pNetResource = &pSharedMemory->NetResources[Index];

            if (pNetResource->InUse)
            {
                if ( ( (wcslen(lpLocalName) + 1) * sizeof(WCHAR) ==
                        pNetResource->LocalNameLength)
                        && ( !wcscmp(lpLocalName, pNetResource->LocalName) ))
                {
                    if (*lpBufferSize < pNetResource->RemoteNameLength)
                    {
                        *lpBufferSize = pNetResource->RemoteNameLength;
                        Status = WN_MORE_DATA;
                    }
                    else
                    {
                        *lpBufferSize = pNetResource->RemoteNameLength;
                        CopyMemory( lpRemoteName,
                                    pNetResource->RemoteName,
                                    pNetResource->RemoteNameLength);
                        Status = WN_SUCCESS;
                    }
                    break;
                }
            }
        }

        CloseSharedMemory( &hMutex, &hMemory, (PVOID)&pSharedMemory);
    }

    return Status;
}

DWORD APIENTRY
NPOpenEnum(
    DWORD          dwScope,
    DWORD          dwType,
    DWORD          dwUsage,
    LPNETRESOURCE  lpNetResource,
    LPHANDLE       lphEnum )
{
    DWORD   Status;

    DbgP((L"[aglo] NPOpenEnum\n"));

    *lphEnum = NULL;

    switch ( dwScope )
    {
        case RESOURCE_CONNECTED:
        {
            *lphEnum = HeapAlloc( GetProcessHeap( ), HEAP_ZERO_MEMORY, sizeof( ULONG ) );

            if (*lphEnum )
            {
                Status = WN_SUCCESS;
            }
            else
            {
                Status = WN_OUT_OF_MEMORY;
            }
            break;
        }
        break;

        case RESOURCE_CONTEXT:
        default:
            Status  = WN_NOT_SUPPORTED;
            break;
    }


    DbgP((L"[aglo] NPOpenEnum returning Status %lx\n",Status));

    return(Status);
}

DWORD APIENTRY
NPEnumResource(
    HANDLE  hEnum,
    LPDWORD lpcCount,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize)
{
    DWORD           Status = WN_SUCCESS;
    ULONG           EntriesCopied;
    LPNETRESOURCE   pNetResource;
    ULONG           SpaceNeeded = 0;
    ULONG           SpaceAvailable;
    PWCHAR          StringZone;
    HANDLE  hMutex, hMemory;
    PNFS41NP_SHARED_MEMORY  pSharedMemory;
    PNFS41NP_NETRESOURCE pNfsNetResource;
    INT  Index = *(PULONG)hEnum;


    DbgP((L"[aglo] NPEnumResource\n"));

    DbgP((L"[aglo] NPEnumResource Count Requested %d\n", *lpcCount));

    pNetResource = (LPNETRESOURCE) lpBuffer;
    SpaceAvailable = *lpBufferSize;
    EntriesCopied = 0;
    StringZone = (PWCHAR) ((PBYTE)lpBuffer + *lpBufferSize);

    Status = OpenSharedMemory( &hMutex,
                               &hMemory,
                               (PVOID)&pSharedMemory);

    if ( Status == WN_SUCCESS)
    {
        Status = WN_NO_MORE_ENTRIES;
        for (Index = *(PULONG)hEnum; EntriesCopied < *lpcCount &&
                Index < pSharedMemory->NextAvailableIndex; Index++)
        {
            pNfsNetResource = &pSharedMemory->NetResources[Index];

            if (pNfsNetResource->InUse)
            {
                SpaceNeeded  = sizeof( NETRESOURCE );
                SpaceNeeded += pNfsNetResource->LocalNameLength;
                SpaceNeeded += pNfsNetResource->RemoteNameLength;
                SpaceNeeded += 5 * sizeof(WCHAR);               // comment
                SpaceNeeded += sizeof(NFS41_PROVIDER_NAME_U);  // provider name
                if ( SpaceNeeded > SpaceAvailable )
                {
                    Status = WN_MORE_DATA;
                    DbgP((L"[aglo] NPEnumResource More Data Needed - %d\n", SpaceNeeded));
                    *lpBufferSize = SpaceNeeded;
                    break;
                }
                else
                {
                    SpaceAvailable -= SpaceNeeded;

                    pNetResource->dwScope       = pNfsNetResource->dwScope;
                    pNetResource->dwType        = pNfsNetResource->dwType;
                    pNetResource->dwDisplayType = pNfsNetResource->dwDisplayType;
                    pNetResource->dwUsage       = pNfsNetResource->dwUsage;

                    // setup string area at opposite end of buffer
                    SpaceNeeded -= sizeof( NETRESOURCE );
                    StringZone = (PWCHAR)( (PBYTE) StringZone - SpaceNeeded );
                    // copy local name
                    StringCchCopy( StringZone,
                        pNfsNetResource->LocalNameLength,
                        pNfsNetResource->LocalName );
                    pNetResource->lpLocalName = StringZone;
                    StringZone += pNfsNetResource->LocalNameLength/sizeof(WCHAR);
                    // copy remote name
                    StringCchCopy( StringZone,
                        pNfsNetResource->RemoteNameLength,
                        pNfsNetResource->RemoteName );
                    pNetResource->lpRemoteName = StringZone;
                    StringZone += pNfsNetResource->RemoteNameLength/sizeof(WCHAR);
                    // copy comment
                    pNetResource->lpComment = StringZone;
                    *StringZone++ = L'A';
                    *StringZone++ = L'_';
                    *StringZone++ = L'O';
                    *StringZone++ = L'K';
                    *StringZone++ = L'\0';
                    // copy provider name
                    pNetResource->lpProvider = StringZone;
                    StringCbCopyW( StringZone, sizeof(NFS41_PROVIDER_NAME_U), NFS41_PROVIDER_NAME_U );
                    StringZone += sizeof(NFS41_PROVIDER_NAME_U)/sizeof(WCHAR);
                    EntriesCopied++;
                    // set new bottom of string zone
                    StringZone = (PWCHAR)( (PBYTE) StringZone - SpaceNeeded );
                    Status = WN_SUCCESS;
                }
                pNetResource++;
            }
        }
        CloseSharedMemory( &hMutex, &hMemory, (PVOID*)&pSharedMemory);
    }

    *lpcCount = EntriesCopied;
    *(PULONG) hEnum = Index;

    DbgP((L"[aglo] NPEnumResource entries returned: %d\n", EntriesCopied));

    return Status;
}

DWORD APIENTRY
NPCloseEnum(
    HANDLE hEnum )
{
    DbgP((L"[aglo] NPCloseEnum\n"));
    HeapFree( GetProcessHeap( ), 0, (PVOID) hEnum );
    return WN_SUCCESS;
}

DWORD APIENTRY
NPGetResourceParent(
    LPNETRESOURCE   lpNetResource,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize )
{
    DbgP(( L"[aglo] NPGetResourceParent: WN_NOT_SUPPORTED\n" ));
    return WN_NOT_SUPPORTED;
}

DWORD APIENTRY
NPGetResourceInformation(
    __in LPNETRESOURCE   lpNetResource,
    __out_bcount(*lpBufferSize) LPVOID  lpBuffer,
    __inout LPDWORD lpBufferSize,
    __deref_out LPWSTR *lplpSystem )
{
    DbgP(( L"[aglo] NPGetResourceInformation: WN_NOT_SUPPORTED\n" ));
    return WN_NOT_SUPPORTED;
}

DWORD APIENTRY
NPGetUniversalName(
    LPCWSTR lpLocalPath,
    DWORD   dwInfoLevel,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize )
{
    DbgP(( L"[aglo] NPGetUniversalName: WN_NOT_SUPPORTED\n" ));
    return WN_NOT_SUPPORTED;
}
