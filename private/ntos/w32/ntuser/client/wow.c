/****************************** Module Header ******************************\
* Module Name: wow.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains shared code between USER32 and USER16
* No New CODE should be added to this file, unless its shared
* with USER16.
*
* History:
* 29-DEC-93 NanduriR      shared user32/user16 code.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* ValidateHwnd
*
* Verify that the handle is valid.  If the handle is invalid or access
* cannot be granted fail.
*
* History:
* 03-18-92 DarrinM      Created from pieces of misc server-side funcs.
\***************************************************************************/

PWND FASTCALL ValidateHwnd(
    HWND hwnd)
{
    PCLIENTINFO pci = GetClientInfo();

    /*
     * Attempt fast window validation
     */
    if (hwnd != NULL && hwnd == pci->CallbackWnd.hwnd) {
        return pci->CallbackWnd.pwnd;
    }

    /*
     * Validate the handle is of the proper type.
     */
    return HMValidateHandle(hwnd, TYPE_WINDOW);
}


PWND FASTCALL ValidateHwndNoRip(
    HWND hwnd)
{
    PCLIENTINFO pci = GetClientInfo();

    /*
     * Attempt fast window validation
     */
    if (hwnd != NULL && hwnd == pci->CallbackWnd.hwnd) {
        return pci->CallbackWnd.pwnd;
    }

    /*
     * Validate the handle is of the proper type.
     */
    return HMValidateHandleNoRip(hwnd, TYPE_WINDOW);
}



int WINAPI GetClassNameA(
    HWND hwnd,
    LPSTR lpClassName,
    int nMaxCount)
{
    PCLS pcls;
    LPSTR lpszClassNameSrc;
    PWND pwnd;
    int cchSrc;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    try {
        if (nMaxCount != 0) {
            pcls = (PCLS)REBASEALWAYS(pwnd, pcls);
            lpszClassNameSrc = REBASEPTR(pwnd, pcls->lpszAnsiClassName);
            cchSrc = lstrlenA(lpszClassNameSrc);
            nMaxCount = min(cchSrc, nMaxCount - 1);
            RtlCopyMemory(lpClassName, lpszClassNameSrc, nMaxCount);
            lpClassName[nMaxCount] = '\0';
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        nMaxCount = 0;
    }

    return nMaxCount;
}

/***************************************************************************\
* _GetDesktopWindow (API)
*
*
*
* History:
* 11-07-90 darrinm      Implemented.
\***************************************************************************/

PWND _GetDesktopWindow(void)
{
    PCLIENTINFO pci;

    ConnectIfNecessary();

    pci = GetClientInfo();
    return (PWND)((KERNEL_ULONG_PTR)pci->pDeskInfo->spwnd -
            pci->ulClientDelta);
}



HWND GetDesktopWindow(void)
{
    PWND        pwnd;
    PCLIENTINFO pci;

    pwnd = _GetDesktopWindow();

    pci = GetClientInfo();

    /*
     * validate the parent window's handle if a restricted process
     */
    if (pci && (pci->dwTIFlags & TIF_RESTRICTED)) {
        if (ValidateHwnd(HW(pwnd)) == NULL) {
            return NULL;
        }
    }
    return HW(pwnd);
}


PWND _GetDlgItem(
    PWND pwnd,
    int id)
{
    if (pwnd != NULL) {
        pwnd = REBASEPWND(pwnd, spwndChild);
        while (pwnd != NULL) {
            if (PtrToLong(pwnd->spmenu) == id)
                break;
            pwnd = REBASEPWND(pwnd, spwndNext);
        }
    }

    return pwnd;
}


HWND GetDlgItem(
    HWND hwnd,
    int id)
{
    PWND pwnd;
    HWND hwndRet;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return NULL;

    pwnd = _GetDlgItem(pwnd, id);

    hwndRet = HW(pwnd);

    if (hwndRet == (HWND)0)
        RIPERR0(ERROR_CONTROL_ID_NOT_FOUND, RIP_VERBOSE, "");

    return hwndRet;
}


HMENU GetMenu(
    HWND hwnd)
{
    PWND pwnd;
    PMENU pmenu;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    /*
     * Some ill-behaved apps use GetMenu to get the child id, so
     * only map to the handle for non-child windows.
     */
    if (!TestwndChild(pwnd)) {
        pmenu = REBASE(pwnd, spmenu);
        return (HMENU)PtoH(pmenu);
    } else {
        return (HMENU)pwnd->spmenu;
    }
}


/***************************************************************************\
* GetMenuItemCount
*
* Returns a count of the number of items in the menu. Returns -1 if
* invalid menu.
*
* History:
\***************************************************************************/

int GetMenuItemCount(
    HMENU hMenu)
{
    PMENU pMenu;

    pMenu = VALIDATEHMENU(hMenu);

    if (pMenu == NULL)
        return -1;

    return pMenu->cItems;
}

/***************************************************************************\
* GetMenuItemID
*
* Return the ID of a menu item at the specified position.
*
* History:
\***************************************************************************/

UINT GetMenuItemID(
    HMENU hMenu,
    int nPos)
{
    PMENU pMenu;
    PITEM pItem;

    pMenu = VALIDATEHMENU(hMenu);

    if (pMenu == NULL)
        return (UINT)-1;

    /*
     * If the position is valid and the item is not a popup, get the ID
     * Don't allow negative indexes, because that'll cause an access violation.
     */
    if (nPos < (int)pMenu->cItems && nPos >= 0) {
        pItem = &((PITEM)REBASEALWAYS(pMenu, rgItems))[nPos];
        if (pItem->spSubMenu == NULL)
            return pItem->wID;
    }

    return (UINT)-1;
}


UINT GetMenuState(
    HMENU hMenu,
    UINT uId,
    UINT uFlags)
{
    PMENU pMenu;

    pMenu = VALIDATEHMENU(hMenu);

    if (pMenu == NULL || (uFlags & ~MF_VALID) != 0) {
        return (UINT)-1;
    }

    return _GetMenuState(pMenu, uId, uFlags);
}


BOOL IsWindow(
    HWND hwnd)
{
    PWND pwnd;

    /*
     * Validate the handle is of type window
     */
    pwnd = ValidateHwndNoRip(hwnd);

    /*
     * And validate this handle is valid for this desktop by trying to read it
     */
    if (pwnd != NULL) {
        try {
            if (pwnd->fnid & FNID_DELETED_BIT) {
                pwnd = 0;
            }
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            RIPMSG1(RIP_WARNING, "IsWindow: Window %#p not of this desktop",
                    pwnd);
            pwnd = 0;
        }
    }
    return !!pwnd;
}


HWND GetWindow(
    HWND hwnd,
    UINT wCmd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return NULL;

    pwnd = _GetWindow(pwnd, wCmd);
    return HW(pwnd);
}

HWND GetParent(
    HWND hwnd)
{
    PWND        pwnd;
    PCLIENTINFO pci;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return NULL;

    try {
        pwnd = _GetParent(pwnd);
        hwnd = HW(pwnd);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        hwnd = NULL;
    }

    pci = GetClientInfo();

    /*
     * validate the parent window's handle if a restricted process
     */
    if (pci && (pci->dwTIFlags & TIF_RESTRICTED)) {
        if (ValidateHwnd(hwnd) == NULL) {
            return NULL;
        }
    }

    return hwnd;
}

HMENU GetSubMenu(
    HMENU hMenu,
    int nPos)
{
    PMENU pMenu;

    pMenu = VALIDATEHMENU(hMenu);

    if (pMenu == NULL)
        return 0;

    pMenu = _GetSubMenu(pMenu, nPos);
    return (HMENU)PtoH(pMenu);
}


DWORD GetSysColor(
    int nIndex)
{

    /*
     * Currently we don't do client side checks because they do not really
     * make sense;  someone can read the data even with the checks.  We
     * leave in the attribute values in case we want to move these values
     * back to the server side someday
     */
#ifdef ENABLE_CLIENTSIDE_ACCESSCHECK
    /*
     * Make sure we have access to the system colors.
     */
    if (!(gamWinSta & WINSTA_READATTRIBUTES)) {
        return 0;
    }
#endif

    /*
     * Return 0 if the index is out of range.
     */
    if (nIndex < 0 || nIndex >= COLOR_MAX) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Invalid parameter \"nIndex\" (%ld) to GetSysColor",
                nIndex);

        return 0;
    }

    return (gpsi->argbSystem[nIndex]);
}


int GetSystemMetrics(
    int index)
{
    ConnectIfNecessary();

    /*
     * check first for values that are out of the 'aiSysMet' array
     */
    switch (index) {

    case SM_REMOTESESSION:
        return ISREMOTESESSION();
    }

    if ((index < 0) || (index >= SM_CMETRICS))
        return 0;

    switch (index) {
    case SM_DBCSENABLED:
#ifdef FE_SB
        return TEST_BOOL_FLAG(gpsi->wSRVIFlags, SRVIF_DBCS);
#else
        return FALSE;
#endif
    case SM_IMMENABLED:
#ifdef FE_IME
        return TEST_BOOL_FLAG(gpsi->wSRVIFlags, SRVIF_IME);
#else
        return FALSE;
#endif

    case SM_MIDEASTENABLED:
        return TEST_BOOL_FLAG(gpsi->wSRVIFlags, SRVIF_MIDEAST);
    }

    if (GetClientInfo()->dwExpWinVer < VER40) {
        /*
         * SCROLL BAR
         * before 4.0, the scroll bars and the border overlapped by a pixel.  Many apps
         * rely on this overlap when they compute dimensions.  Now, in 4.0, this pixel
         * overlap is no longer there.  So for old apps, we lie and pretend the overlap
         * is there by making the scroll bar widths one bigger.
         *
         * DLGFRAME
         * In Win3.1, SM_CXDLGFRAME & SM_CYDLGFRAME were border space MINUS 1
         * In Win4.0, they are border space
         *
         * CAPTION
         * In Win3.1, SM_CYCAPTION was the caption height PLUS 1
         * In Win4.0, SM_CYCAPTION is the caption height
         *
         * MENU
         * In Win3.1, SM_CYMENU was the menu height MINUS 1
         * In Win4.0, SM_CYMENU is the menu height
         */

        switch (index) {

        case SM_CXDLGFRAME:
        case SM_CYDLGFRAME:
        case SM_CYMENU:
        case SM_CYFULLSCREEN:
            return (gpsi->aiSysMet)[index] - 1;

        case SM_CYCAPTION:
        case SM_CXVSCROLL:
        case SM_CYHSCROLL:
            return (gpsi->aiSysMet)[index] + 1;
        }
    }

    return gpsi->aiSysMet[index];
}

/***************************************************************************\
* GetTopWindow (API)
*
* This poorly named API should really be called 'GetFirstChild', which is
* what it does.
*
* History:
* 11-12-90 darrinm      Ported.
* 02-19-91 JimA         Added enum access check
* 05-04-02 DarrinM      Removed enum access check and moved to USERRTL.DLL
\***************************************************************************/

HWND GetTopWindow(
    HWND hwnd)
{
    PWND pwnd;

    /*
     * Allow a NULL hwnd to go through here.
     */
    if (hwnd == NULL) {
        pwnd = _GetDesktopWindow();
    } else {
        pwnd = ValidateHwnd(hwnd);
    }
    if (pwnd == NULL)
        return NULL;

    pwnd = REBASEPWND(pwnd, spwndChild);
    return HW(pwnd);
}


BOOL IsChild(
    HWND hwndParent,
    HWND hwnd)
{
    PWND pwnd, pwndParent;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return FALSE;

    pwndParent = ValidateHwnd(hwndParent);
    if (pwndParent == NULL)
        return FALSE;

    return _IsChild(pwndParent, pwnd);
}

BOOL IsIconic(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    return _IsIconic(pwnd);
}

BOOL IsWindowEnabled(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    return _IsWindowEnabled(pwnd);
}

BOOL IsWindowVisible(
    HWND hwnd)
{
    PWND pwnd;
    BOOL bRet;

    pwnd = ValidateHwnd(hwnd);

    /*
     * We have have to try - except this call because there is no
     * synchronization on the window structure on the client side.
     * If the window is deleted after it is validated then we can
     * fault so we catch that on return that the window is not
     * visible.  As soon as this API returns there is no guarentee
     * the return is still valid in a muli-tasking environment.
     */
    try {
        if (pwnd == NULL) {
            bRet = FALSE;
        } else {
            bRet = _IsWindowVisible(pwnd);
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        bRet = FALSE;
    }

    return bRet;
}

BOOL IsZoomed(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    return _IsZoomed(pwnd);
}

BOOL ClientToScreen(
    HWND hwnd,
    LPPOINT ppoint)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    _ClientToScreen(pwnd, ppoint);
    return TRUE;
}

BOOL GetClientRect(
    HWND   hwnd,
    LPRECT prect)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    _GetClientRect(pwnd, prect);
    return TRUE;
}


BOOL GetCursorPos(
    LPPOINT lpPoint)
{
#ifdef REDIRECTION
    PCLIENTINFO pci;
#endif // REDIRECTION

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
#ifdef ENABLE_CLIENTSIDE_ACCESSCHECK
    if (!(gamWinSta & WINSTA_READATTRIBUTES)) {
        lpPoint->x = 0;
        lpPoint->y = 0;
        return FALSE;
    }
#endif

    *lpPoint = gpsi->ptCursor;
    
#ifdef REDIRECTION
    pci = GetClientInfo();

    if (pci != NULL && pci->pDeskInfo != NULL && IsHooked(pci, WHF_CBT)) {
        NtUserCallOneParam((ULONG_PTR)lpPoint, SFI_XXXGETCURSORPOS);
    }
#endif // REDIRECTION
    
    return TRUE;
}

BOOL GetWindowRect(
    HWND hwnd,
    LPRECT prect)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    _GetWindowRect(pwnd, prect);
    return TRUE;
}

BOOL ScreenToClient(
    HWND hwnd,
    LPPOINT ppoint)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    _ScreenToClient(pwnd, ppoint);
    return TRUE;
}

BOOL EnableMenuItem(
    HMENU hMenu,
    UINT uIDEnableItem,
    UINT uEnable)
{
    PMENU pMenu;
    PITEM pItem;

    pMenu = VALIDATEHMENU(hMenu);
    if (pMenu == NULL) {
        return (BOOL)-1;
    }

    /*
     * Get a pointer the the menu item
     */
    if ((pItem = MNLookUpItem(pMenu, uIDEnableItem, (BOOL) (uEnable & MF_BYPOSITION), NULL)) == NULL)
        return (DWORD)-1;

    /*
     * If the item is already in the state we're
     * trying to set, just return.
     */
    if ((pItem->fState & MFS_GRAYED) ==
            (uEnable & MFS_GRAYED)) {
        return pItem->fState & MFS_GRAYED;
    }

    return NtUserEnableMenuItem(hMenu, uIDEnableItem, uEnable);
}

/***************************************************************************\
* CallNextHookEx
*
* This routine is called to call the next hook in the hook chain.
*
* 05-09-91 ScottLu Created.
\***************************************************************************/

LRESULT WINAPI CallNextHookEx(
    HHOOK hhk,
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT nRet;
    BOOL  bAnsi;
    DWORD dwHookCurrent;
    PCLIENTINFO pci;
    ULONG_PTR dwHookData;
    ULONG_PTR dwFlags;

    DBG_UNREFERENCED_PARAMETER(hhk);

    ConnectIfNecessary();

    pci = GetClientInfo();
    dwHookCurrent = pci->dwHookCurrent;
    bAnsi = LOWORD(dwHookCurrent);

    /*
     * If this is the last hook in the hook chain then return 0; we're done
     */
    UserAssert(pci->phkCurrent);
    if (PhkNextValid((PHOOK)((KERNEL_ULONG_PTR)pci->phkCurrent - pci->ulClientDelta)) == NULL) {
        return 0;
    }

    switch ((INT)(SHORT)HIWORD(dwHookCurrent)) {
    case WH_CALLWNDPROC:
    case WH_CALLWNDPROCRET:
        /*
         * This is the hardest of the hooks because we need to thunk through
         * the message hooks in order to deal with synchronously sent messages
         * that point to structures - to get the structures passed across
         * alright, etc.
         *
         * This will call a special kernel-side routine that'll rebundle the
         * arguments and call the hook in the right format.
         *
         * Currently, the message thunk callbacks to the client-side don't take
         * enough parameters to pass wParam (which == fInterThread send msg).
         * To do this, save the state of wParam in the CLIENTINFO structure.
         */
        dwFlags = KERNEL_ULONG_PTR_TO_ULONG_PTR(pci->CI_flags) & CI_INTERTHREAD_HOOK;
        dwHookData = KERNEL_ULONG_PTR_TO_ULONG_PTR(pci->dwHookData);
        if (wParam) {
            pci->CI_flags |= CI_INTERTHREAD_HOOK;
        } else {
            pci->CI_flags &= ~CI_INTERTHREAD_HOOK;
        }

        if ((INT)(SHORT)HIWORD(dwHookCurrent) == WH_CALLWNDPROC) {
            nRet = CsSendMessage(
                    ((LPCWPSTRUCT)lParam)->hwnd,
                    ((LPCWPSTRUCT)lParam)->message,
                    ((LPCWPSTRUCT)lParam)->wParam,
                    ((LPCWPSTRUCT)lParam)->lParam,
                    0, FNID_HKINLPCWPEXSTRUCT, bAnsi);
        } else {
            pci->dwHookData = ((LPCWPRETSTRUCT)lParam)->lResult;
            nRet = CsSendMessage(
                    ((LPCWPRETSTRUCT)lParam)->hwnd,
                    ((LPCWPRETSTRUCT)lParam)->message,
                    ((LPCWPRETSTRUCT)lParam)->wParam,
                    ((LPCWPRETSTRUCT)lParam)->lParam,
                    0, FNID_HKINLPCWPRETEXSTRUCT, bAnsi);
        }

        /*
         * Restore previous hook state.
         */
        pci->CI_flags ^= ((pci->CI_flags ^ dwFlags) & CI_INTERTHREAD_HOOK);
        pci->dwHookData = dwHookData;
        break;

    default:
        nRet = NtUserCallNextHookEx(
                nCode,
                wParam,
                lParam,
                bAnsi);
    }

    return nRet;
}

