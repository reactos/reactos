/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WCURCASH.H
 *  WOW32 Cursor & Icon cash worker routines.
 *
 *  History:
 *  Created on Jan 27th-93 by ChandanC
 *
--*/


#include "precomp.h"
#pragma hdrstop


STATIC PCURICON pCurIconFirst = NULL;       // pointer to first hCurIcon entry


HICON16 W32CheckWOWCashforIconCursors(VPVOID pData, WORD ResType)
{
    register PICONCUR16 parg16;
    HICON16 hIcon16;
    HICON16 hRes16;
    PSZ psz;
    HAND32 h32;

    GETMISCPTR(pData, parg16);
    GETPSZIDPTR(parg16->lpStr, psz);

    hIcon16 = W32FindCursorIcon (parg16->hInst, psz, ResType, &hRes16);

    if (hIcon16) {
        if (ResType == (WORD) RT_ICON) {
            h32 = HICON32(hIcon16);
            ResType = HANDLE_TYPE_ICON;
        }
        else {
            h32 = HCURSOR32(hIcon16);
            ResType = HANDLE_TYPE_CURSOR;
        }

        hIcon16 = SetupResCursorIconAlias((HAND16) parg16->hInst, h32, (HANDLE) hRes16, (UINT) ResType);
    }

    FREEPSZIDPTR(psz);
    FREEMISCPTR(parg16);

    return (hIcon16);
}


HICON16 W32FindCursorIcon (WORD hInst, LPSTR psz, WORD ResType, HICON16 *phRes16)
{
    PCURICON   pTemp;

    pTemp = pCurIconFirst;

    while (pTemp) {
        if (pTemp->ResType == ResType) {
            if (pTemp->hInst == hInst) {
                if ((HIWORD(psz) != 0) && (HIWORD(pTemp->lpszIcon) != 0))  {
                    if (!(WOW32_stricmp(psz, (LPSTR)pTemp->lpszIcon))) {
                        *phRes16 = pTemp->hRes16;
                        return (pTemp->hIcon16);
                    }
                }
                else if ((HIWORD(psz) == 0) && (HIWORD(pTemp->lpszIcon) == 0))  {
                    if ((WORD) pTemp->lpszIcon == (WORD)psz) {
                        *phRes16 = pTemp->hRes16;
                        return (pTemp->hIcon16);
                    }
                }
            }
        }
        pTemp = pTemp->pNext;
    }
    return 0;
}


BOOL W32AddCursorIconCash (WORD hInst, LPSTR psz1, HICON16 hIcon16, HICON16 hRes16, WORD ResType)
{
    PCURICON pCurIcon;
    PSZ psz2;
    WORD cb;

    // if "psz1" is a string, allocate the memory for it

    if ((WORD)HIWORD(psz1) != (WORD)NULL) {
        cb = strlen(psz1)+1;
        if (psz2 = malloc_w_small(cb)) {
            memcpy (psz2, psz1, cb);
        }
        else {
            LOGDEBUG (0, ("WOW::W32AddCursorIcon: Memory allocation failed *** \n"));
            return (0);
        }
    }
    else {
        psz2 = psz1;
    }

    if (pCurIcon = malloc_w_small (sizeof(CURICON))) {
        pCurIcon->pNext     = pCurIconFirst;
        pCurIconFirst       = pCurIcon;               // update list head
        pCurIcon->hInst     = hInst;
        pCurIcon->lpszIcon  = (DWORD)psz2;
        pCurIcon->hIcon16   = hIcon16;
        pCurIcon->hRes16    = hRes16;
        pCurIcon->ResType   = ResType;
        pCurIcon->dwThreadID = CURRENTPTD()->dwThreadID;
        return (TRUE);
    }
    else {
        LOGDEBUG(0, ("WOW::WAddCursorIcon(): *** memory allocation failed *** \n"));
        return (FALSE);
    }
}


// This routine deletes a resource (Cursor or Icon) from the cash.

VOID W32DeleteCursorIconCash (HICON16 hRes16)
{
    PCURICON   pTemp;
    PCURICON   pTempLast;

    pTemp = pCurIconFirst;

    while (pTemp) {
        if (pTemp->hRes16 == hRes16) {
            if (pTemp == pCurIconFirst) {
                pCurIconFirst = pTemp->pNext;
            }
            else {
                pTempLast->pNext = pTemp->pNext;
            }

            // if its a string, delete the memory that we allocated for it

            if ((WORD)HIWORD(pTemp->lpszIcon) != (WORD)NULL) {
                free_w_small ((PVOID)pTemp->lpszIcon);
            }

            free_w_small (pTemp);
            pTemp = NULL;
        }
        else {
            pTempLast = pTemp;
            pTemp = pTemp->pNext;
        }
    }
}


// This routine deletes all the cursors and Icons when a task terminates.

VOID W32DeleteCursorIconCashForTask ()
{
    DWORD dwThreadID;
    PCURICON   pTemp;
    PCURICON   pTempLast;
    PCURICON   pTempNext;

    dwThreadID = CURRENTPTD()->dwThreadID;

    pTemp = pCurIconFirst;

    while (pTemp) {
        if (pTemp->dwThreadID == dwThreadID) {
            if (pTemp == pCurIconFirst) {
                pCurIconFirst = pTemp->pNext;
            }
            else {
                pTempLast->pNext = pTemp->pNext;
            }

            // if its a string, delete the memory that we allocated for it

            if ((WORD)HIWORD(pTemp->lpszIcon) != (WORD)NULL) {
                free_w_small ((PVOID)pTemp->lpszIcon);
            }

            pTempNext = pTemp->pNext;
            free_w_small (pTemp);
            pTemp = pTempNext;
        }
        else {
            pTempLast = pTemp;
            pTemp = pTemp->pNext;
        }
    }
}
