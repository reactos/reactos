/* ---File: memutil.c -----------------------------------------------------
 *
 *  Description:
 *    Contains Control Panel memory allocation routines.
 *
 *    This document contains confidential/proprietary information.
 *    Copyright (c) 1990-1992 Microsoft Corporation, All Rights Reserved.
 *
 * Revision History:
 *
 * ---------------------------------------------------------------------- */
/* Notes -

    Global Functions:

        AllocMem () -
        AllocStr () -
        FreeMem () -
        FreeStr () -
        ReallocMem () -
        ReallocStr () -

    Local Functions:

 */
//==========================================================================
//                              Include files
//==========================================================================
// C Runtime
#include <string.h>
#include <memory.h>

// Application specific
#include "ups.h"


LPVOID AllocMem (DWORD cb)
/*++

Routine Description:

    This function will allocate local memory. It will possibly allocate extra
    memory and fill this with debugging information for the debugging version.

Arguments:

    cb - The amount of memory to allocate

Return Value:

    NON-NULL - A pointer to the allocated memory

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    LPDWORD  pMem;
    DWORD    cbNew;

    cbNew = cb+2*sizeof(DWORD);
    if (cbNew & 3)
        cbNew += sizeof(DWORD) - (cbNew & 3);

//    pMem = (LPDWORD)HeapAlloc (hHeap, 0, cbNew);

    pMem = (LPDWORD)LocalAlloc (LMEM_FIXED, cbNew);
    memset (pMem, 0, cbNew);     // This might go later if done in NT
    *pMem = cb;
    *(LPDWORD)((LPTSTR)pMem+cbNew-sizeof(DWORD)) = 0xdeadbeef;

    return (LPVOID)(pMem+1);
}


BOOL FreeMem (LPVOID pMem, DWORD  cb)
{
    DWORD   cbNew;
    LPDWORD pNewMem;

    if (!pMem)
        return TRUE;

    pNewMem = pMem;
    pNewMem--;

    cbNew = cb+2*sizeof(DWORD);
    if (cbNew & 3)
        cbNew += sizeof(DWORD) - (cbNew & 3);

#ifdef DEBU
    if ((*pNewMem != cb) ||
       (*(LPDWORD)((LPTSTR)pNewMem + cbNew - sizeof(DWORD)) != 0xdeadbeef))
    {
	OutputDebugStringA("Corrupt Memory in Control Panel : %0lx\n");
        DbgBreakPoint();
    }
#endif
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


LPTSTR AllocStr (LPTSTR lpStr)
/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    lpStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
   LPTSTR lpMem;

   if (!lpStr)
      return 0;

   if (lpMem = AllocMem (strlen (lpStr) + sizeof(TCHAR)))
      strcpy (lpMem, lpStr);

   return lpMem;
}


BOOL FreeStr (LPTSTR lpStr)
{
   return lpStr ? FreeMem (lpStr, strlen (lpStr) + sizeof(TCHAR)) : FALSE;
}


BOOL ReallocStr (LPTSTR *plpStr, LPTSTR lpStr)
{
   FreeStr (*plpStr);
   *plpStr = AllocStr (lpStr);

   return TRUE;
}

int MyMessageBox (HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ...)
{
    char szText[256+PATHMAX], szCaption[256];
    int ival;
    va_list parg;

    va_start (parg, wType);

    if (wText == LSFAIL)
        goto NoMem;

    if (!LoadString(hModule, wText, szCaption, sizeof (szCaption)))
        goto NoMem;

    wvsprintf(szText, szCaption, parg);

    if (!LoadString(hModule, wCaption, szCaption, sizeof (szCaption)))
        goto NoMem;

    if ((ival = MessageBox(hWnd, szText, szCaption, wType)) == 0)
        goto NoMem;

    return(ival);

NoMem:
    va_end (parg);

    ErrLoadString(hWnd);
    return 0;
}

void ErrLoadString (HWND hParent)
{
    MessageBox (hParent, szErrLS, szCtlPanel, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
}

void ErrMemDlg (HWND hParent)
{
    MessageBox (hParent, szErrMem, szCtlPanel, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
}
