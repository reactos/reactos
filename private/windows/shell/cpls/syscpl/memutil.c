/* ---File: memutil.c -----------------------------------------------------
 *
 *  Description:
 *    Contains Control Panel memory allocation routines.
 *
 *    This document contains confidential/proprietary information.
 *    Copyright (c) 1990-1994 Microsoft Corporation, All Rights Reserved.
 *
 * Revision History:
 *  6/3/94  [stevecat]  Added ANSI only string allocators
 *
 * ---------------------------------------------------------------------- */
/* Notes -

    Global Functions:

        AllocMem () - Allocate a block of memory, with error checks
        AllocStr () - Allocate memory for a string
        AllocStrA () - Allocate memory for an ANSI string
        FreeMem () - Check and free memory block allocated with AllocMem
        FreeStr () - Free string memory allocated with AllocStr
        FreeStrA () - Free string memory allocated with AllocStrA
        ReallocMem () - Resize a memory block originally from AllocMem
        ReallocStr () - Resize a string memory block from AllocStr
        ReallocStrA () - Resize a string memory block from AllocStrA

    Local Functions:

 */
//==========================================================================
//                              Include files
//==========================================================================
// C Runtime
#include <string.h>
#include <memory.h>

// Application specific
#include "system.h"


/////////////////////////////////////////////////////////////////////////////
// 
// Routine Description:
// 
//     This function will allocate local memory. It will possibly allocate
//     extra memory and fill this with debugging information for the
//     debugging version.
// 
// Arguments:
// 
//     cb - The amount of memory to allocate
// 
// Return Value:
// 
//     NON-NULL - A pointer to the allocated memory
// 
//     FALSE/NULL - The operation failed. Extended error status is available
//         using GetLastError.
// 
/////////////////////////////////////////////////////////////////////////////

LPVOID AllocMem (DWORD cb)
{
    LPDWORD  pMem;
    DWORD    cbNew;

    cbNew = cb + 2 * sizeof(DWORD);
    if (cbNew & 3)
        cbNew += sizeof(DWORD) - (cbNew & 3);

    pMem = (LPDWORD)LocalAlloc (LPTR, cbNew);
    if (!pMem)
        return NULL;

    // memset (pMem, 0, cbNew);     // This might go later if done in NT

    *pMem = cb;
    *(LPDWORD)((LPBYTE)pMem+cbNew-sizeof(DWORD)) = 0xdeadbeef;

    return (LPVOID)(pMem+1);
}


BOOL FreeMem (LPVOID pMem, DWORD  cb)
{
    DWORD   cbNew;
    LPDWORD pNewMem;
    TCHAR   szError[128];

    if (!pMem)
        return TRUE;

    pNewMem = pMem;
    pNewMem--;

    cbNew = cb+2*sizeof(DWORD);
    if (cbNew & 3)
        cbNew += sizeof(DWORD) - (cbNew & 3);

    if ((*pNewMem != cb) ||
       (*(LPDWORD)((LPBYTE)pNewMem + cbNew - sizeof(DWORD)) != 0xdeadbeef))
    {
#if DBG
        wsprintf (szError, TEXT("Corrupt Memory in Control Panel : %0lx\n"), pMem);
        OutputDebugString(szError);
        // DbgBreakPoint();
        //  Corrupt Memory in Control Panel - try to free it anyway
        return FALSE;
#endif  // DBG
    }

    return (((HLOCAL) pNewMem == LocalFree ((LPVOID)pNewMem)));
}

LPVOID ReallocMem (LPVOID lpOldMem, DWORD cbOld, DWORD cbNew)
{
   LPVOID lpNewMem;

   lpNewMem = AllocMem (cbNew);
   if (lpOldMem)
   {
      memcpy (lpNewMem, lpOldMem, min(cbNew, cbOld));
      FreeMem (lpOldMem, cbOld);
   }
   return lpNewMem;
}


/////////////////////////////////////////////////////////////////////////////
// 
// Routine Description:
// 
//     These functions will allocate or reallocate enough local memory to
//     store the specified  string, and copy that string to the allocated
//     memory.  The FreeStr function frees memory that was initially
//     allocated by AllocStr.
//
// Arguments:
// 
//     lpStr - Pointer to the string that needs to be allocated and stored
// 
// Return Value:
// 
//     NON-NULL - A pointer to the allocated memory containing the string
// 
//     FALSE/NULL - The operation failed. Extended error status is available
//         using GetLastError.
// 
/////////////////////////////////////////////////////////////////////////////

LPTSTR AllocStr (LPTSTR lpStr)
{
   LPTSTR lpMem;

   if (!lpStr)
      return NULL;

   if (lpMem = AllocMem (ByteCountOf(lstrlen(lpStr)+1)))
      lstrcpy (lpMem, lpStr);

   return lpMem;
}


BOOL FreeStr (LPTSTR lpStr)
{
   return lpStr ? FreeMem (lpStr, ByteCountOf(lstrlen(lpStr)+1)) : FALSE;
}


BOOL ReallocStr (LPTSTR *plpStr, LPTSTR lpStr)
{
   FreeStr (*plpStr);
   *plpStr = AllocStr (lpStr);

   return TRUE;
}

#ifdef ANSI_FUNCTIONS

/////////////////////////////////////////////////////////////////////////////
//
// Routine Descriptions:
//
//     These functions will allocate or reallocate enough local memory to
//     store the specified  string, and copy that string to the allocated
//     memory.  The FreeStrA function frees memory that was initially
//     allocated by AllocStrA.
//
//     THESE ARE ANSI only string functions.  UNICODE versions are above.
//
// Arguments:
// 
//     lpStr - Pointer to the string that needs to be allocated and stored
// 
// Return Value:
// 
//     NON-NULL - A pointer to the allocated memory containing the string
// 
//     FALSE/NULL - The operation failed. Extended error status is available
//         using GetLastError.
// 
/////////////////////////////////////////////////////////////////////////////

LPSTR AllocStrA (LPSTR lpStr)
{
    LPSTR lpMem;

    if (!lpStr)
        return NULL;

    if (lpMem = AllocMem (strlen(lpStr)+1))
        strcpy (lpMem, lpStr);

    return lpMem;
}


BOOL FreeStrA (LPSTR lpStr)
{
    return lpStr ? FreeMem (lpStr, strlen(lpStr)+1) : FALSE;
}


BOOL ReallocStrA (LPSTR *plpStr, LPSTR lpStr)
{
    if (FreeStrA (*plpStr))
    {
        if (*plpStr = AllocStrA (lpStr))
            return TRUE;
    }

    return FALSE;
}
#endif  // ANSI_FUNCTIONS

