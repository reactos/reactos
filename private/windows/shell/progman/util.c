/****************************** Module Header ******************************\
* Module Name: winutil.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements windows specific utility functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "sec.h"
#include <string.h>
#include <stdio.h>


/****************************************************************************

   FUNCTION: Alloc

   PURPOSE: Allocates memory to hold the specified number of bytes

   RETURNS : Pointer to allocated memory or NULL on failure

****************************************************************************/

PVOID Alloc(
    DWORD   Bytes)
{
    HANDLE  hMem;
    PVOID   Buffer;

    hMem = LocalAlloc(LMEM_MOVEABLE, Bytes + sizeof(hMem));

    if (hMem == NULL) {
        return(NULL);
    }

    // Lock down the memory
    //
    Buffer = LocalLock(hMem);
    if (Buffer == NULL) {
        LocalFree(hMem);
        return(NULL);
    }

    //
    // Store the handle at the start of the memory block and return
    // a pointer to just beyond it.
    //

    *((PHANDLE)Buffer) = hMem;

    return (PVOID)(((PHANDLE)Buffer)+1);
}


/****************************************************************************

   FUNCTION: Free

   PURPOSE: Frees the memory previously allocated with Alloc

   RETURNS : TRUE on success, otherwise FALSE

****************************************************************************/

BOOL Free(
    PVOID   Buffer)
{
    HANDLE  hMem;

    hMem = *(((PHANDLE)Buffer) - 1);

    LocalUnlock(hMem);

    return(LocalFree(hMem) == NULL);
}

