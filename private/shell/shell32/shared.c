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
#include "shellprv.h"
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

HANDLE _SHAllocShared(
    LPCVOID lpvData,
    DWORD   dwSize,
    DWORD   dwDestinationProcessId
) {
    return SHAllocShared(lpvData, dwSize, dwDestinationProcessId);
}

LPVOID _SHLockShared(
    HANDLE  hData,
    DWORD   dwSourceProcessId
) {
    return SHLockShared(hData, dwSourceProcessId);
}

BOOL _SHUnlockShared(
    LPVOID  lpvData
) {
    return SHUnlockShared(lpvData);
}

BOOL _SHFreeShared(
    HANDLE hData,
    DWORD dwSourceProcessId
) {
    return SHFreeShared(hData, dwSourceProcessId);
}
