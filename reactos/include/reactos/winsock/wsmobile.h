/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 NSP
 * FILE:        include/reactos/winsock/wsmobile.h
 * PURPOSE:     WinSock 2 NSP Header
 */

#ifndef __WSM_H
#define __WSM_H

/* nsp.cpp */
extern GUID gNLANamespaceGuid;

/*
 * nsp.cpp
 */
INT
WINAPI
WSM_NSPCleanup(IN LPGUID lpProviderId);

INT
WINAPI
WSM_NSPSetService(
    IN LPGUID lpProviderId,
    IN LPWSASERVICECLASSINFOW lpServiceClassInfo,
    IN LPWSAQUERYSETW lpqsRegInfo,
    IN WSAESETSERVICEOP essOperation,
    IN DWORD dwControlFlags
);

INT
WINAPI
WSM_NSPInstallServiceClass(
    IN LPGUID lpProviderId,
    IN LPWSASERVICECLASSINFOW lpServiceClassInfo
);

INT
WINAPI
WSM_NSPRemoveServiceClass(
    IN LPGUID lpProviderId,
    IN LPGUID lpServiceCallId
);

INT
WINAPI
WSM_NSPGetServiceClassInfo(
    IN LPGUID lpProviderId,
    IN OUT LPDWORD lpdwBufSize,
    IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo
);

INT 
WINAPI
WSM_NSPLookupServiceBegin(
    LPGUID lpProviderId,
    LPWSAQUERYSETW lpqsRestrictions,
    LPWSASERVICECLASSINFOW lpServiceClassInfo,
    DWORD dwControlFlags,
    LPHANDLE lphLookup
);

INT
WINAPI
WSM_NSPLookupServiceNext(
    IN HANDLE hLookup,
    IN DWORD dwControlFlags,
    IN OUT LPDWORD lpdwBufferLength,
    OUT LPWSAQUERYSETW lpqsResults
);

INT
WINAPI
WSM_NSPLookupServiceEnd(IN HANDLE hLookup);

INT 
WINAPI
WSM_NSPStartup(
    IN LPGUID lpProviderId,
    IN OUT LPNSP_ROUTINE lpsnpRoutines
);

#endif

