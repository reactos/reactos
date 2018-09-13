/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1992, Microsoft Corporation
 *
 *  WKMEM.C
 *  WOW32 KRNL386 Virtual Memory Management Functions
 *
 *  History:
 *  Created 3-Dec-1992 by Matt Felton (mattfe)
 *
--*/

#include "precomp.h"
#pragma hdrstop
#include "memapi.h"

MODNAME(wkman.c);


ULONG FASTCALL WK32VirtualAlloc(PVDMFRAME pFrame)
{
    PVIRTUALALLOC16 parg16;
    ULONG lpBaseAddress;
#ifndef i386
    NTSTATUS Status;
#endif

    GETARGPTR(pFrame, sizeof(VIRTUALALLOC16), parg16);


#ifndef i386
    Status = VdmAllocateVirtualMemory(&lpBaseAddress,
                                      parg16->cbSize,
                                      TRUE);

    if (!NT_SUCCESS(Status)) {

        if (Status == STATUS_NOT_IMPLEMENTED) {
#endif // i386

            lpBaseAddress = (ULONG) VirtualAlloc((LPVOID)parg16->lpvAddress,
                                                  parg16->cbSize,
                                                  parg16->fdwAllocationType,
                                                  parg16->fdwProtect);


#ifndef i386
        } else {

            lpBaseAddress = 0;
        }

    }
#endif // i386

#ifdef i386
//BUGBUG we need to either get this working on the new emulator, or
//       fix the problem the "other" way, by letting the app fault and
//       zap in just enough 'WOW's to avoid the problem.
    if (lpBaseAddress) {
        // Virtual alloc Zero's the allocated memory. We un-zero it by
        // filling in with ' WOW'. This is required for Lotus Improv.
        // When no printer is installed, Lotus Improv dies with divide
        // by zero error (while opening the expenses.imp file) because
        // it doesn't initialize a relevant portion of its data area.
        //
        // So we decided that this a convenient place to initialize
        // the memory to a non-zero value.
        //                                           - Nanduri
        //
        // Dbase 5.0 for windows erroneously loops through (past its valid
        // data) its data buffer till it finds a '\0' at some location -
        // Most of the time the loop terminates before it reaches the segment
        // limit. However if the block that was allocated is a 'fresh' block' ie
        // the block is filled with ' WOW' it never finds a NULL in the buffer
        // and thus loops past the segment limit to its death
        //
        // So we initialize the buffer with '\0WOW' instead of ' WOW'.
        //                                            - Nanduri

        WOW32ASSERT((parg16->cbSize % 4) == 0);      // DWORD aligned?
        RtlFillMemoryUlong((PVOID)lpBaseAddress, parg16->cbSize, (ULONG)'\0WOW');
    }
#endif

    FREEARGPTR(parg16);
    return (lpBaseAddress);
}

ULONG FASTCALL WK32VirtualFree(PVDMFRAME pFrame)
{
    PVIRTUALFREE16 parg16;
    ULONG fResult;
#ifndef i386
    NTSTATUS Status;
#endif

    GETARGPTR(pFrame, sizeof(VIRTUALFREE16), parg16);

#ifndef i386
    Status = VdmFreeVirtualMemory((ULONG)parg16->lpvAddress);
    fResult = NT_SUCCESS(Status);

    if (Status == STATUS_NOT_IMPLEMENTED) {
#endif // i386

        fResult = VirtualFree((LPVOID)parg16->lpvAddress,
                                 parg16->cbSize,
                                 parg16->fdwFreeType);


#ifndef i386
    }
#endif // i386

    FREEARGPTR(parg16);
    return (fResult);
}


#if 0
ULONG FASTCALL WK32VirtualLock(PVDMFRAME pFrame)
{
    PVIRTUALLOCK16 parg16;
    BOOL fResult;

    WOW32ASSERT(FALSE);     //BUGBUG we don't appear to ever use this function

    GETARGPTR(pFrame, sizeof(VIRTUALLOCK16), parg16);

    fResult = VirtualLock((LPVOID)parg16->lpvAddress,
                             parg16->cbSize);

    FREEARGPTR(parg16);
    return (fResult);
}

ULONG FASTCALL WK32VirtualUnLock(PVDMFRAME pFrame)
{
    PVIRTUALUNLOCK16 parg16;
    BOOL fResult;

    WOW32ASSERT(FALSE);     //BUGBUG we don't appear to ever use this function

    GETARGPTR(pFrame, sizeof(VIRTUALUNLOCK16), parg16);

    fResult = VirtualUnlock((LPVOID)parg16->lpvAddress,
                               parg16->cbSize);

    FREEARGPTR(parg16);
    return (fResult);
}
#endif


ULONG FASTCALL WK32GlobalMemoryStatus(PVDMFRAME pFrame)
{
    PGLOBALMEMORYSTATUS16 parg16;
    LPMEMORYSTATUS pMemStat;

    GETARGPTR(pFrame, sizeof(GLOBALMEMORYSTATUS16), parg16);
    GETVDMPTR(parg16->lpmstMemStat, 32, pMemStat);

    GlobalMemoryStatus(pMemStat);

    FREEVDMPTR(pMemStat);
    FREEARGPTR(parg16);
    return 0;  // unused
}
