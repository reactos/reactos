/****************************** Module Header ******************************\
* Module Name: clenum
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* For enumeration functions
*
* 04-27-91 ScottLu Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define IEP_UNICODE 0x1 // Convert Atom to unicode string (vs ANSI)
#define IEP_ENUMEX 0x2 // Pass lParam back to callback function (vs no lParam)

HWND *phwndCache = NULL;


/***************************************************************************\
* InternalEnumWindows
*
* Calls server and gets back a window list. This list is enumerated, for each
* window the callback address is called (into the application), until either
* end-of-list is reached or FALSE is return ed. lParam is passed into the
* callback function for app reference.
*
*
* If any windows are returned (cHwnd > 0) the caller is responsible for
* freeing the window buffer when done with the list
*
*
* 04-27-91 ScottLu Created.
\***************************************************************************/

DWORD BuildHwndList(
    HDESK hdesk,
    HWND hwndNext,
    BOOL fEnumChildren,
    DWORD idThread,
    HWND **pphwndFirst)
{
    UINT cHwnd;
    HWND *phwndFirst;
    NTSTATUS Status;
    int cTries;

    /*
     * Allocate a buffer to hold the names.
     */
    cHwnd = 64;
    phwndFirst = (HWND *)InterlockedExchangePointer(&(PVOID)phwndCache, 0);
    if (phwndFirst == NULL) {
        phwndFirst = UserLocalAlloc(0, cHwnd * sizeof(HWND));
        if (phwndFirst == NULL)
            return 0;
    }

    Status = NtUserBuildHwndList(hdesk, hwndNext, fEnumChildren,
            idThread, cHwnd, phwndFirst, &cHwnd);

    /*
     * If the buffer wasn't big enough, reallocate
     * the buffer and try again.
     */
    cTries = 0;
    while (Status == STATUS_BUFFER_TOO_SMALL) {
        UserLocalFree(phwndFirst);

        /*
         * If we can't seem to get it right,
         * call it quits
         */
        if (cTries++ == 10)
            return 0;

        phwndFirst = UserLocalAlloc(0, cHwnd * sizeof(HWND));
        if (phwndFirst == NULL)
            return 0;

        Status = NtUserBuildHwndList(hdesk, hwndNext, fEnumChildren,
                idThread, cHwnd, phwndFirst, &cHwnd);
    }

    if (!NT_SUCCESS(Status) || cHwnd <= 1) {
        UserLocalFree(phwndFirst);
        return 0;
    }

    *pphwndFirst = phwndFirst;
    return cHwnd - 1;
}

BOOL InternalEnumWindows(
    HDESK hdesk,
    HWND hwnd,
    WNDENUMPROC lpfn,
    LPARAM lParam,
    DWORD idThread,
    BOOL fEnumChildren)
{
    UINT i;
    UINT cHwnd;
    HWND *phwndT;
    HWND *phwndFirst;
    BOOL fSuccess = TRUE;

    /*
     * Get the hwnd list.  It is returned in a block of memory
     * allocated with LocalAlloc.
     */
    if ((cHwnd = BuildHwndList(hdesk, hwnd, fEnumChildren, idThread,
            &phwndFirst)) == -1) {
        return FALSE;
    }

    /*
     * In Win 3.1 it was not an error if there were no windows in the thread
     */
    if (cHwnd == 0) {
        if (idThread == 0)
            return FALSE;
        else
            return TRUE;
    }


    /*
     * Loop through the windows, call the function pointer back for each
     * one. End loop if either FALSE is return ed or the end-of-list is
     * reached.
     */
    phwndT = phwndFirst;
    for (i = 0; i < cHwnd; i++) {

        /*
         * call ValidateHwnd instead of RevalidateHwnd so that
         * restricted processes don't see handles they are not
         * suppose to see.
         */
        if (ValidateHwnd(*phwndT)) {
            if (!(fSuccess = (*lpfn)(*phwndT, lParam)))
                break;
        }
        phwndT++;
    }

    /*
     * Free up buffer and return status - TRUE if entire list was enumerated,
     * FALSE otherwise.
     */
    phwndT = (HWND *)InterlockedExchangePointer(&(PVOID)phwndCache, phwndFirst);
    if (phwndT != NULL) {
        UserLocalFree(phwndT);
    }
    return fSuccess;
}


/***************************************************************************\
* EnumWindows
*
* Enumerates all top-level windows. Calls back lpfn with each hwnd until
* either end-of-list or FALSE is return ed. lParam is passed into callback
* function for app reference.
*
* 04-27-91 ScottLu Created.
\***************************************************************************/

BOOL WINAPI EnumWindows(
    WNDENUMPROC lpfn,
    LPARAM lParam)
{
    return InternalEnumWindows(NULL, NULL, lpfn, lParam, 0L, FALSE);
}

/***************************************************************************\
* EnumChildWindows
*
* Enumerates all children of the passed in window. Calls back lpfn with each
* hwnd until either end-of-list or FALSE is return ed. lParam is passed into
* callback function for app reference.
*
* 04-27-91 ScottLu Created.
\***************************************************************************/

BOOL WINAPI EnumChildWindows(
    HWND hwnd,
    WNDENUMPROC lpfn,
    LPARAM lParam)
{
    return InternalEnumWindows(NULL, hwnd, lpfn, lParam, 0L, TRUE);
}

/***************************************************************************\
* EnumThreadWindows
*
* Enumerates all top level windows created by idThread. Calls back lpfn with
* each hwnd until either end-of-list or FALSE is return ed. lParam is passed
* into callback function for app reference.
*
* 06-23-91 ScottLu Created.
\***************************************************************************/

BOOL EnumThreadWindows(
    DWORD idThread,
    WNDENUMPROC lpfn,
    LPARAM lParam)
{
    return InternalEnumWindows(NULL, NULL, lpfn, lParam, idThread, FALSE);
}

/***************************************************************************\
* EnumDesktopWindows
*
* Enumerates all top level windows on the desktop specified by hdesk.
* Calls back lpfn with each hwnd until either end-of-list or FALSE
* is returned. lParam is passed into callback function for app reference.
*
* 10-10-94 JimA     Created.
\***************************************************************************/

BOOL EnumDesktopWindows(
    HDESK hdesk,
    WNDENUMPROC lpfn,
    LPARAM lParam)
{
    return InternalEnumWindows(hdesk, NULL, lpfn, lParam, 0, FALSE);
}




/***************************************************************************\
* InternalEnumProps
*
* Calls server and gets back a list of props for the specified window.
* The callback address is called (into the application), until either
* end-of-list is reached or FALSE is return ed.
* lParam is passed into the callback function for app reference when
* IEP_ENUMEX is set. Atoms are turned into UNICODE string if IEP_UNICODE
* is set.
*
* 22-Jan-1992 JohnC Created.
\***************************************************************************/

#define MAX_ATOM_SIZE 512
#define ISSTRINGATOM(atom)     ((WORD)(atom) >= 0xc000)

INT InternalEnumProps(
    HWND hwnd,
    PROPENUMPROC lpfn,
    LPARAM lParam,
    UINT flags)
{
    DWORD ii;
    DWORD cPropSets;
    PPROPSET pPropSet;
    WCHAR awch[MAX_ATOM_SIZE];
    PVOID pKey;
    INT iRetVal;
    DWORD cchName;
    NTSTATUS Status;
    int cTries;

    /*
     * Allocate a buffer to hold the names.
     */
    cPropSets = 32;
    pPropSet = UserLocalAlloc(0, cPropSets * sizeof(PROPSET));
    if (pPropSet == NULL)
        return -1;

    Status = NtUserBuildPropList(hwnd, cPropSets, pPropSet, &cPropSets);

    /*
     * If the buffer wasn't big enough, reallocate
     * the buffer and try again.
     */
    cTries = 0;
    while (Status == STATUS_BUFFER_TOO_SMALL) {
        UserLocalFree(pPropSet);

        /*
         * If we can't seem to get it right,
         * call it quits
         */
        if (cTries++ == 10)
            return -1;

        pPropSet = UserLocalAlloc(0, cPropSets * sizeof(PROPSET));
        if (pPropSet == NULL)
            return -1;

        Status = NtUserBuildPropList(hwnd, cPropSets, pPropSet, &cPropSets);
    }

    if (!NT_SUCCESS(Status)) {
        UserLocalFree(pPropSet);
        return -1;
    }

    for (ii=0; ii<cPropSets; ii++) {

        if (ISSTRINGATOM(pPropSet[ii].atom)) {

            pKey = (PVOID)awch;
            if (flags & IEP_UNICODE)
                cchName = GlobalGetAtomNameW(pPropSet[ii].atom, (LPWSTR)pKey, MAX_ATOM_SIZE);
            else
                cchName = GlobalGetAtomNameA(pPropSet[ii].atom, (LPSTR)pKey, sizeof(awch));

            /*
             * If cchName is zero, we must assume that the property belongs
             * to another process.  Because we can't get the name, just skip
             * it.
             */
            if (cchName == 0)
                continue;

        } else {
            pKey = (PVOID)pPropSet[ii].atom;
        }

        if (flags & IEP_ENUMEX) {
            iRetVal = (*(PROPENUMPROCEX)lpfn)(hwnd, pKey,
                    pPropSet[ii].hData, lParam);
        } else {
            iRetVal = (*lpfn)(hwnd, pKey, pPropSet[ii].hData);
        }

        if (!iRetVal)
            break;
    }

    UserLocalFree(pPropSet);

    return iRetVal;
}


/***************************************************************************\
* EnumProps
*
* This function enumerates all entries in the property list of the specified
* window. It enumerates the entries by passing them, one by one, to the
* callback function specified by lpEnumFunc. EnumProps continues until the
* last entry is enumerated or the callback function return s zero.
*
* 22-Jan-1992 JohnC Created.
\***************************************************************************/

INT WINAPI EnumPropsA(
    HWND hwnd,
    PROPENUMPROCA lpfn)
{
    return InternalEnumProps(hwnd, (PROPENUMPROC)lpfn, 0, 0);
}


INT WINAPI EnumPropsW(
    HWND hwnd,
    PROPENUMPROCW lpfn)
{
    return InternalEnumProps(hwnd, (PROPENUMPROC)lpfn, 0, IEP_UNICODE);
}

/***************************************************************************\
* EnumPropsEx
*
* This function enumerates all entries in the property list of the specified
* window. It enumerates the entries by passing them, one by one, to the
* callback function specified by lpEnumFunc. EnumProps continues until the
* last entry is enumerated or the callback function return s zero.
*
* 22-Jan-1992 JohnC Created.
\***************************************************************************/

BOOL WINAPI EnumPropsExA(
    HWND hwnd,
    PROPENUMPROCEXA lpfn,
    LPARAM lParam)
{
    return InternalEnumProps(hwnd, (PROPENUMPROC)lpfn, lParam, IEP_ENUMEX);
}

BOOL WINAPI EnumPropsExW(
    HWND hwnd,
    PROPENUMPROCEXW lpfn,
    LPARAM lParam)
{
    return InternalEnumProps(hwnd, (PROPENUMPROC)lpfn, lParam, IEP_UNICODE|IEP_ENUMEX);
}



BOOL InternalEnumObjects(
    HWINSTA hwinsta,
    NAMEENUMPROCW lpfn,
    LPARAM lParam,
    BOOL fAnsi)
{
    PNAMELIST pNameList;
    DWORD i;
    UINT cbData;
    PWCHAR pwch;
    PCHAR pch;
    CHAR achTmp[MAX_PATH];
    BOOL iRetVal;
    NTSTATUS Status;
    int cTries;

    /*
     * Allocate a buffer to hold the names.
     */
    cbData = 256;
    pNameList = UserLocalAlloc(0, cbData);
    if (pNameList == NULL)
        return FALSE;

    Status = NtUserBuildNameList(hwinsta, cbData, pNameList, &cbData);

    /*
     * If the buffer wasn't big enough, reallocate
     * the buffer and try again.
     */
    cTries = 0;
    while (Status == STATUS_BUFFER_TOO_SMALL) {
        UserLocalFree(pNameList);

        /*
         * If we can't seem to get it right,
         * call it quits
         */
        if (cTries++ == 10)
            break;

        pNameList = UserLocalAlloc(0, cbData);
        if (pNameList == NULL)
            break;

        Status = NtUserBuildNameList(hwinsta, cbData, pNameList, &cbData);
    }

    if (!NT_SUCCESS(Status)) {
        UserLocalFree(pNameList);
        return FALSE;
    }

    pwch = pNameList->awchNames;
    pch = achTmp;

    for (i = 0; i < pNameList->cNames; i++) {
        if (fAnsi) {
            if (WCSToMB(pwch, -1, &pch, sizeof(achTmp), FALSE) ==
                    sizeof(achTmp)) {

                /*
                 * The buffer may have overflowed, so force it to be
                 * allocated.
                 */
                if (WCSToMB(pwch, -1, &pch, -1, TRUE) == 0) {
                    iRetVal = FALSE;
                    break;
                }
            }
            iRetVal = (*(NAMEENUMPROCA)lpfn)(pch, lParam);
            if (pch != achTmp) {
                UserLocalFree(pch);
                pch = achTmp;
            }
        } else {
            iRetVal = (*(NAMEENUMPROCW)lpfn)(pwch, lParam);
        }
        if (!iRetVal)
            break;

        pwch = pwch + wcslen(pwch) + 1;
    }

    UserLocalFree(pNameList);

    return iRetVal;
}

BOOL WINAPI EnumWindowStationsA(
    WINSTAENUMPROCA lpEnumFunc,
    LPARAM lParam)
{
    return InternalEnumObjects(NULL, (NAMEENUMPROCW)lpEnumFunc, lParam, TRUE);
}

BOOL WINAPI EnumWindowStationsW(
    WINSTAENUMPROCW lpEnumFunc,
    LPARAM lParam)
{
    return InternalEnumObjects(NULL, (NAMEENUMPROCW)lpEnumFunc, lParam, FALSE);
}


BOOL WINAPI EnumDesktopsA(
    HWINSTA hwinsta,
    DESKTOPENUMPROCA lpEnumFunc,
    LPARAM lParam)
{
    return InternalEnumObjects(hwinsta, (NAMEENUMPROCW)lpEnumFunc, lParam, TRUE);
}

BOOL WINAPI EnumDesktopsW(
    HWINSTA hwinsta,
    DESKTOPENUMPROCW lpEnumFunc,
    LPARAM lParam)
{
    return InternalEnumObjects(hwinsta, (NAMEENUMPROCW)lpEnumFunc, lParam, FALSE);
}
