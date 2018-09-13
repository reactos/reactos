//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1995
//
// File: shared.c
//
// History:
//  06-07-95 BobDay     Created.
//
// This file contains a set of routines for the management of shared memory.
//
//---------------------------------------------------------------------------
#include "priv.h"
#pragma  hdrstop

//---------------------------------------------------------------------------
// SHAllocShared  - Allocates a handle (in a given process) to a copy of a
//                  memory block in this process.
// SHFreeShared   - Releases the handle (and the copy of the memory block)
//
// SHLockShared   - Maps a handle (from a given process) into a memory block
//                  in this process.  Has the option of transfering the handle
//                  to this process, thereby deleting it from the given process
// SHUnlockShared - Opposite of SHLockShared, unmaps the memory block
//---------------------------------------------------------------------------

typedef struct _shmapheader {
    DWORD   dwSize;
} SHMAPHEADER, *LPSHMAPHEADER;


HANDLE SHMapHandle(HANDLE hData, DWORD dwSource, DWORD dwDest, DWORD dwDesiredAccess, DWORD dwFlags)
{
    HANDLE hSource = NULL;
    HANDLE hDest = NULL;
    HANDLE hNew = NULL;
    BOOL fOk;

    if (dwSource == GetCurrentProcessId())
        hSource = GetCurrentProcess();
    else
        hSource = OpenProcess( PROCESS_DUP_HANDLE, FALSE, dwSource);

    if (!hSource)
        goto DoExit;

    if (dwDest == GetCurrentProcessId())
        hDest = GetCurrentProcess();
    else
        hDest = OpenProcess( PROCESS_DUP_HANDLE, FALSE, dwDest);

    if (!hDest)
        goto DoExit;

    fOk = DuplicateHandle( hSource, hData,
                           hDest, &hNew,
                           dwDesiredAccess,
                           FALSE, dwFlags | DUPLICATE_SAME_ACCESS);
    if (!fOk)
        hNew = (HANDLE)NULL;

DoExit:
    if (hSource && dwSource != GetCurrentProcessId())
        CloseHandle(hSource);

    if (hDest && dwDest != GetCurrentProcessId())
        CloseHandle(hDest);

    return hNew;
}

HANDLE SHAllocShared(
    LPCVOID lpvData,
    DWORD   dwSize,
    DWORD   dwDestinationProcessId
) {
    HANDLE  hData;
    LPSHMAPHEADER lpmh;
    HANDLE hUsableData;

    //
    // Make a filemapping handle with this data in it.
    //
    hData = CreateFileMapping( (HANDLE)-1, NULL, PAGE_READWRITE,0,
                               dwSize+sizeof(SHMAPHEADER),NULL);
    if (hData == NULL)
    {
        // DebugMsg...
        return NULL;
    }

    lpmh = MapViewOfFile(hData, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (!lpmh)
    {
        // DebugMsg...
        CloseHandle(hData);
        return NULL;
    }
    lpmh->dwSize = dwSize;

    if (lpvData)
        memcpy((LPVOID)(lpmh+1),lpvData,dwSize);

    UnmapViewOfFile(lpmh);

    hUsableData = SHMapHandle(hData,
                            GetCurrentProcessId(),
                            dwDestinationProcessId,
                            FILE_MAP_ALL_ACCESS,
                            DUPLICATE_SAME_ACCESS);
    
    CloseHandle(hData);

    return hUsableData;
}

LPVOID SHLockShared(
    HANDLE  hData,
    DWORD   dwSourceProcessId
) {
    LPSHMAPHEADER   lpmh;
    HANDLE          hUsableData;

    hUsableData = SHMapHandle(hData,dwSourceProcessId,GetCurrentProcessId(),FILE_MAP_ALL_ACCESS,0);

    //
    // Now map that new process specific handle and close it
    //
    lpmh = MapViewOfFile( hUsableData, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    CloseHandle(hUsableData);

    if (!lpmh)
        return NULL;

    return (LPVOID)(lpmh+1);
}

BOOL SHUnlockShared(
    LPVOID  lpvData
) {
    LPSHMAPHEADER lpmh = (LPSHMAPHEADER)lpvData;

    //
    // Now just unmap the view of the file
    //
    return UnmapViewOfFile(lpmh-1);
}

BOOL SHFreeShared(
    HANDLE hData,
    DWORD dwSourceProcessId
) {
    HANDLE hUsableData;

    //
    // The below call closes the original handle in whatever process it
    // came from.
    //
    hUsableData = SHMapHandle(hData,dwSourceProcessId,
                            GetCurrentProcessId(),
                            FILE_MAP_ALL_ACCESS,DUPLICATE_CLOSE_SOURCE);

    //
    // Now free up the local handle
    //
    return CloseHandle(hUsableData);
}
