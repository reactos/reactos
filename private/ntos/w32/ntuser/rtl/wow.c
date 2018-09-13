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

#include "wow.h"


#ifdef _USERK_
    #define CHECK_RESTRICTED()                                                      \
        if (((PTHREADINFO)W32GetCurrentThread())->TIF_flags & TIF_RESTRICTED) {     \
            if (!ValidateHandleSecure(h))                                           \
                pobj = NULL;                                                        \
        }                                                                           \

#else
    #define CHECK_RESTRICTED()                                              \
        if (pci && (pci->dwTIFlags & TIF_RESTRICTED) && pobj) {             \
            if (!NtUserValidateHandleSecure(h))                             \
                pobj = NULL;                                                \
        }                                                                   \

#endif



#ifdef _USERK_
    #define GET_CURRENT_CLIENTINFO()                            \
        {                                                       \
            PW32THREAD pW32Thread;                              \
                                                                \
            pW32Thread = W32GetCurrentThread();                 \
                                                                \
            if (pW32Thread) {                                   \
                pci = ((PTHREADINFO)pW32Thread)->pClientInfo;   \
            } else {                                            \
                pci = NULL;                                     \
            }                                                   \
        }

#else
    #define GET_CURRENT_CLIENTINFO()                            \
        pci = GetClientInfo();

#endif // _USERK_


/*
 * We have two types of desktop validation:
 *
 */

#ifdef _USERK_

#define DESKTOPVALIDATE(pci, pobj) \
            UNREFERENCED_PARAMETER(pci);

#define DESKTOPVALIDATECCX(pci, pobj)                               \
        if (    ((PVOID)pobj >= pci->pDeskInfo->pvDesktopBase) &&  \
                ((PVOID)pobj < pci->pDeskInfo->pvDesktopLimit)) {  \
            pobj = (PBYTE)pobj - pci->ulClientDelta;               \
        }                                                           \

#define SHAREDVALIDATE(pobj)

#else

#define DESKTOPVALIDATE(pci, pobj)                              \
        if (    pci->pDeskInfo &&                               \
                pobj >= pci->pDeskInfo->pvDesktopBase &&        \
                pobj < pci->pDeskInfo->pvDesktopLimit) {        \
            pobj = (KERNEL_PVOID)((KERNEL_ULONG_PTR)pobj - pci->ulClientDelta);         \
        } else {                                                \
            pobj = (KERNEL_PVOID)NtUserCallOneParam((ULONG_PTR)h,                       \
                    SFI__MAPDESKTOPOBJECT);                     \
        }                                                       \

#define SHAREDVALIDATE(pobj)                                    \
        pobj = REBASESHAREDPTRALWAYS(pobj);

#endif // _USERK_


/*
 * Keep the general path through validation straight without jumps - that
 * means tunneling if()'s for this routine - this'll make validation fastest
 * because of instruction caching.
 *
 * In order to have the validation code in one place only, we define
 *  the *ValidateHandleMacro macros which are to be included by the
 *  HMValidateHanlde* routines. We don't make these into functions
 *  because we're optimizing on time, not size.
 */
#define ValidateHandleMacro(pci, pobj, h, bType) \
    StartValidateHandleMacro(h) \
    BeginAliveValidateHandleMacro()  \
    BeginTypeValidateHandleMacro(pobj, bType) \
    DESKTOPVALIDATE(pci, pobj) \
    EndTypeValidateHandleMacro \
    EndAliveValidateHandleMacro()  \
    EndValidateHandleMacro

#ifdef _USERK_
#define ValidateCatHandleMacro(pci, pobj, h, bType) \
    StartValidateHandleMacro(h) \
    BeginTypeValidateHandleMacro(pobj, bType) \
    DESKTOPVALIDATE(pci, pobj) \
    EndTypeValidateHandleMacro \
    EndValidateHandleMacro
#define ValidateCatHandleMacroCcx(pci, pobj, h, bType) \
    StartValidateHandleMacro(h) \
    BeginTypeValidateHandleMacro(pobj, bType) \
    DESKTOPVALIDATECCX(pci, pobj) \
    EndTypeValidateHandleMacro \
    EndValidateHandleMacro
#endif

#define ValidateSharedHandleMacro(pobj, h, bType) \
    StartValidateHandleMacro(h)  \
    BeginAliveValidateHandleMacro()  \
    BeginTypeValidateHandleMacro(pobj, bType) \
    SHAREDVALIDATE(pobj)        \
    EndTypeValidateHandleMacro \
    EndAliveValidateHandleMacro()  \
    EndValidateHandleMacro


/*
 * The handle validation routines should be optimized for time, not size,
 * since they get called so often.
 */
#pragma optimize("t", on)

/***************************************************************************\
* HMValidateHandle
*
* This routine validates a handle manager handle.
*
* 01-22-92 ScottLu      Created.
\***************************************************************************/

PVOID FASTCALL HMValidateHandle(
    HANDLE h,
    BYTE bType)
{
    DWORD       dwError;
    KERNEL_PVOID pobj = NULL;
    PCLIENTINFO pci;

    GET_CURRENT_CLIENTINFO();

#if DBG != 0 && !defined(_USERK_)
    /*
     * We don't want 32 bit apps passing 16 bit handles
     *  we should consider failing this before we get
     *  stuck supporting it (Some VB apps do this).
     */
    if (pci && (h != NULL)
           && (HMUniqFromHandle(h) == 0)
           && !(pci->dwTIFlags & TIF_16BIT)) {
        RIPMSG3(RIP_WARNING, "HMValidateHandle: 32bit process [%d] using 16 bit handle [%#p] bType:%#lx",
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess), h, (DWORD)bType);
    }
#endif

    /*
     * Object can't be located in shared memory.
     */
    UserAssert(bType != TYPE_MONITOR);

    /*
     * Validation macro. Falls through if the handle is invalid.
     */
    ValidateHandleMacro(pci, pobj, h, bType);

    /*
     * check for secure process
     */
    CHECK_RESTRICTED();

    if (pobj != NULL) {
        return pobj;
    }

    switch (bType) {

    case TYPE_WINDOW:
        dwError = ERROR_INVALID_WINDOW_HANDLE;
        break;

    case TYPE_MENU:
        dwError = ERROR_INVALID_MENU_HANDLE;
        break;

    case TYPE_CURSOR:
        dwError = ERROR_INVALID_CURSOR_HANDLE;
        break;

    case TYPE_ACCELTABLE:
        dwError = ERROR_INVALID_ACCEL_HANDLE;
        break;

    case TYPE_HOOK:
        dwError = ERROR_INVALID_HOOK_HANDLE;
        break;

    case TYPE_SETWINDOWPOS:
        dwError = ERROR_INVALID_DWP_HANDLE;
        break;

    default:
        dwError = ERROR_INVALID_HANDLE;
        break;
    }

    RIPERR2(dwError,
            RIP_WARNING,
            "HMValidateHandle: Invalid:%#p Type:%#lx",
            h, (DWORD)bType);

    /*
     * If we get here, it's an error.
     */
    return NULL;
}

/***************************************************************************\
* HMValidateHandleNoSecure
*
* This routine validates a handle manager handle.
*
* 01-22-92 ScottLu      Created.
\***************************************************************************/
PVOID FASTCALL HMValidateHandleNoSecure(
    HANDLE h,
    BYTE bType)
{
    KERNEL_PVOID pobj = NULL;
    PCLIENTINFO pci;

    GET_CURRENT_CLIENTINFO();

#if !defined(_USERK_)
    /*
     * We don't want 32 bit apps passing 16 bit handles
     *  we should consider failing this before we get
     *  stuck supporting it (Some VB apps do this).
     */
    if (pci && (h != NULL)
           && (HMUniqFromHandle(h) == 0)
           && !(pci->dwTIFlags & TIF_16BIT)) {
        RIPMSG3(RIP_WARNING, "HMValidateHandle: 32bit process [%d] using 16 bit handle [%#p] bType:%#lx",
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess), h, (DWORD)bType);
    }
#endif

    /*
     * Object can't be located in shared memory.
     */
    UserAssert(bType != TYPE_MONITOR);

    /*
     * Validation macro.
     */
    ValidateHandleMacro(pci, pobj, h, bType);

    return pobj;
}

#if defined(_USERK_)
PVOID FASTCALL HMValidateCatHandleNoSecure(
    HANDLE h,
    BYTE bType)
{
    PVOID       pobj = NULL;
    PCLIENTINFO pci;

    GET_CURRENT_CLIENTINFO();

    /*
     * Object can't be located in shared memory.
     */
    UserAssert(bType != TYPE_MONITOR);

    /*
     * Validation macro.
     */
    ValidateCatHandleMacro(pci, pobj, h, bType);

    return pobj;
}
PVOID FASTCALL HMValidateCatHandleNoSecureCCX(
    HANDLE h,
    BYTE bType,
    PCLIENTINFO ccxPci)
{
    PVOID       pobj = NULL;

    /*
     * Object can't be located in shared memory.
     */
    UserAssert(bType != TYPE_MONITOR);

    /*
     * Validation macro.
     */
    ValidateCatHandleMacroCcx(ccxPci, pobj, h, bType);

    return pobj;
}

PVOID FASTCALL HMValidateCatHandleNoRip(
    HANDLE h,
    BYTE bType)
{
    PVOID       pobj = NULL;
    PCLIENTINFO pci;

    /*
     * This is the fastest way way to do validation, because
     *  unlike HMValidateHandle, this function doesn't set the
     *  last error.
     *
     * Validation macro. Falls through if the handle is invalid.
     */

    GET_CURRENT_CLIENTINFO();

    /*
     * Object can't be located in shared memory.
     */
    UserAssert(bType != TYPE_MONITOR);

    ValidateCatHandleMacro(pci, pobj, h, bType);

    /*
     * check for secure process
     */
    CHECK_RESTRICTED();

    return pobj;
}
#endif

PVOID FASTCALL HMValidateHandleNoRip(
    HANDLE h,
    BYTE bType)
{
    KERNEL_PVOID pobj = NULL;
    PCLIENTINFO pci;

    /*
     * This is the fastest way way to do validation, because
     *  unlike HMValidateHandle, this function doesn't set the
     *  last error.
     *
     * Validation macro. Falls through if the handle is invalid.
     */

    GET_CURRENT_CLIENTINFO();

    /*
     * Object can't be located in shared memory.
     */
    UserAssert(bType != TYPE_MONITOR);

    ValidateHandleMacro(pci, pobj, h, bType);

    /*
     * check for secure process
     */
    CHECK_RESTRICTED();

    return pobj;
}

#if DBG != 0 && !defined(_USERK_)
/*
 * HMValidateHandleNoDesktop is a debug-client-side only function
 *  used to verify a given handle without calling DESKTOPVALIDATE.
 * If the handle is valid, it returns the object's kernel pointer
 *  which can be used as a BOOL value only.
 * Use this function to verify handles for which corresponding phe->phead
 *  is a pool allocation (as opposed to desktop-heap allocations).
 */
KERNEL_PVOID FASTCALL HMValidateHandleNoDesktop(
    HANDLE h,
    BYTE bType)
{
    KERNEL_PVOID pobj = NULL;

    StartValidateHandleMacro(h)
    BeginTypeValidateHandleMacro(pobj, bType)
    EndTypeValidateHandleMacro
    EndValidateHandleMacro
    return pobj;
}
#endif


/***************************************************************************\
* HMValidateSharedHandle
*
* This routine validates a handle manager handle allocated in
* shared memory.
*
* History:
* 02-Apr-1997 adams     Created.
\***************************************************************************/

PVOID FASTCALL HMValidateSharedHandle(
    HANDLE h,
    BYTE bType)
{
    DWORD dwError;
    KERNEL_PVOID pobj = NULL;

#if DBG != 0 && !defined(_USERK_)

    /*
     * We don't want 32 bit apps passing 16 bit handles
     *  we should consider failing this before we get
     *  stuck supporting it (Some VB apps do this).
     */
    if ((h != NULL)
           && (HMUniqFromHandle(h) == 0)
           && !(GetClientInfo()->dwTIFlags & TIF_16BIT)) {
        RIPMSG3(RIP_WARNING, "HMValidateHandle: 32bit process [%d] using 16 bit handle [%#p] bType:%#lx",
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess), h, (DWORD)bType);
    }
#endif

    /*
     * Validation macro. Falls through if the handle is invalid.
     */
    ValidateSharedHandleMacro(pobj, h, bType);

    if (pobj != NULL)
        return pobj;

    switch (bType) {
        case TYPE_MONITOR:
            dwError = ERROR_INVALID_MONITOR_HANDLE;
            break;

        default:
            UserAssertMsg0(0, "Logic error in HMValidateSharedHandle");
            break;
    }

    RIPERR2(dwError,
            RIP_WARNING,
            "HMValidateSharedHandle: Invalid:%#p Type:%#lx",
            h, (DWORD)bType);

    /*
     * If we get here, it's an error.
     */
    return NULL;
}


/*
 * Switch back to default optimization.
 */
#pragma optimize("", on)

/***************************************************************************\
* MNLookUpItem
*
* Return a pointer to the menu item specified by wCmd and wFlags
*
* History:
*   10-11-90 JimA       Translated from ASM
*   01-07-93 FritzS     Ported from Chicago
\***************************************************************************/

PITEM MNLookUpItem(
    PMENU pMenu,
    UINT wCmd,
    BOOL fByPosition,
    PMENU *ppMenuItemIsOn)
{
    PITEM pItem;
    PITEM pItemRet = NULL;
    PITEM  pItemMaybe;
    PMENU   pMenuMaybe = NULL;
    int i;

    if (ppMenuItemIsOn != NULL)
        *ppMenuItemIsOn = NULL;

    if (pMenu == NULL || !pMenu->cItems || wCmd == MFMWFP_NOITEM) {
//      RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "MNLookUpItem: invalid item");
        return NULL;
    }

    /*
     * dwFlags determines how we do the search
     */
    if (fByPosition) {
        if (wCmd < (UINT)pMenu->cItems) {
            pItemRet = &((PITEM)REBASEALWAYS(pMenu, rgItems))[wCmd];
            if (ppMenuItemIsOn != NULL)
                *ppMenuItemIsOn = pMenu;
            return (pItemRet);
        } else
            return NULL;
    }
    /*
     * Walk down the menu and try to find an item with an ID of wCmd.
     * The search procedes from the end of the menu (as was done in
     * assembler).
     */

/* this is the Chicago code, which walks from the front of the menu -- Fritz */


//        for (pItem = &pMenu->rgItems[i - 1]; pItemRet == NULL && i--; --pItem) {
    for (i = 0, pItem = REBASEALWAYS(pMenu, rgItems); i < (int)pMenu->cItems;
            i++, pItem++) {

        /*
         * If the item is a popup, recurse down the tree
         */
        if (pItem->spSubMenu != NULL) {
        //
        // COMPAT:
        // Allow apps to pass in menu handle as ID in menu APIs.  We
        // remember that this popup had a menu handle with the same ID
        // value.  This is a 2nd choice though.  We still want to see
        // if there's some actual command that has this ID value first.
        //
            if (pItem->wID == wCmd) {
                pMenuMaybe = pMenu;
                pItemMaybe = pItem;
            }

            pItemRet = MNLookUpItem((PMENU)REBASEPTR(pMenu, pItem->spSubMenu),
                    wCmd, FALSE, ppMenuItemIsOn);
            if (pItemRet != NULL)
                return pItemRet;
        } else if (pItem->wID == wCmd) {

                /*
                 * Found the item, now save things for later
                 */
                if (ppMenuItemIsOn != NULL)
                    *ppMenuItemIsOn = pMenu;
                return pItem;
        }
    }

    if (pMenuMaybe) {
        // no non popup menu match found -- use the 2nd choice popup menu
        // match
        if (ppMenuItemIsOn != NULL)
            *ppMenuItemIsOn = pMenuMaybe;
        return(pItemMaybe);
    }

    return(NULL);
}

/***************************************************************************\
* GetMenuState
*
* Either returns the state of a menu item or the state and item count
* of a popup.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

UINT _GetMenuState(
    PMENU pMenu,
    UINT wId,
    UINT dwFlags)
{
    PITEM pItem;
    DWORD fFlags;

    /*
     * If the item does not exist, leave
     */
    if ((pItem = MNLookUpItem(pMenu, wId, (BOOL) (dwFlags & MF_BYPOSITION), NULL)) == NULL)
        return (UINT)-1;

    fFlags = pItem->fState | pItem->fType;

#ifndef _USERK_
    /*
     * Add old MFT_BITMAP flag to keep old apps happy
     */
    if ((pItem->hbmp != NULL) && (pItem->lpstr == NULL)) {
        fFlags |= MFT_BITMAP;
    }
#endif

    if (pItem->spSubMenu != NULL) {
        /*
         * If the item is a popup, return item count in high byte and
         * popup flags in low byte
         */

        fFlags = ((fFlags | MF_POPUP) & 0x00FF) +
            (((PMENU)REBASEPTR(pMenu, pItem->spSubMenu))->cItems << 8);
    }

    return fFlags;
}


/***************************************************************************\
* GetPrevPwnd
*
*
*
* History:
* 11-05-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

PWND GetPrevPwnd(
    PWND pwndList,
    PWND pwndFind)
{
    PWND pwndFound, pwndNext;

    if (pwndList == NULL)
        return NULL;

    if (pwndList->spwndParent == NULL)
        return NULL;

    pwndNext = REBASEPWND(pwndList, spwndParent);
    pwndNext = REBASEPWND(pwndNext, spwndChild);
    pwndFound = NULL;

    while (pwndNext != NULL) {
        if (pwndNext == pwndFind)
            break;
        pwndFound = pwndNext;
        pwndNext = REBASEPWND(pwndNext, spwndNext);
    }

    return (pwndNext == pwndFind) ? pwndFound : NULL;
}


/***************************************************************************\
* _GetWindow (API)
*
*
* History:
* 11-05-90 darrinm      Ported from Win 3.0 sources.
* 02-19-91 JimA         Added enum access check
* 05-04-02 DarrinM      Removed enum access check and moved to USERRTL.DLL
\***************************************************************************/

PWND _GetWindow(
    PWND pwnd,
    UINT cmd)
{
    PWND pwndT;
    BOOL fRebase = FALSE;

    /*
     * If this is a desktop window, return NULL for sibling or
     * parent information.
     */
    if (GETFNID(pwnd) == FNID_DESKTOP) {
        switch (cmd) {
        case GW_CHILD:
            break;

        default:
            return NULL;
            break;
        }
    }

    /*
     * Rebase the returned window at the end of the routine
     * to avoid multiple test for pwndT == NULL.
     */
    pwndT = NULL;
    switch (cmd) {
    case GW_HWNDNEXT:
        pwndT = pwnd->spwndNext;
        fRebase = TRUE;
        break;

    case GW_HWNDFIRST:
        if (pwnd->spwndParent) {
            pwndT = REBASEPWND(pwnd, spwndParent);
            pwndT = REBASEPWND(pwndT, spwndChild);
            if (GetAppCompatFlags(NULL) & GACF_IGNORETOPMOST) {
                while (pwndT != NULL) {
                    if (!TestWF(pwndT, WEFTOPMOST))
                        break;
                    pwndT = REBASEPWND(pwndT, spwndNext);
                }
            }
        }
        break;

    case GW_HWNDLAST:
        pwndT = GetPrevPwnd(pwnd, NULL);
        break;

    case GW_HWNDPREV:
        pwndT = GetPrevPwnd(pwnd, pwnd);
        break;

    case GW_OWNER:
        pwndT = pwnd->spwndOwner;
        fRebase = TRUE;
        break;

    case GW_CHILD:
        pwndT = pwnd->spwndChild;
        fRebase = TRUE;
        break;

#if !defined(_USERK_)
    case GW_ENABLEDPOPUP:
       pwndT = (PWND)NtUserCallHwnd(PtoHq(pwnd), SFI_DWP_GETENABLEDPOPUP);
       fRebase = TRUE;
       break;
#endif

    default:
        RIPERR0(ERROR_INVALID_GW_COMMAND, RIP_VERBOSE, "");
        return NULL;
    }

    if (pwndT != NULL && fRebase)
        pwndT = REBASEPTR(pwnd, pwndT);

    return pwndT;
}

/***************************************************************************\
* _GetParent (API)
*
*
*
* History:
* 11-12-90 darrinm      Ported.
* 02-19-91 JimA         Added enum access check
* 05-04-92 DarrinM      Removed enum access check and moved to USERRTL.DLL
\***************************************************************************/

PWND _GetParent(
    PWND pwnd)
{
    /*
     * For 1.03 compatibility reasons, we should return NULL
     * for top level "tiled" windows and owner for other popups.
     * pwndOwner is set to NULL in xxxCreateWindow for top level
     * "tiled" windows.
     */
    if (!(TestwndTiled(pwnd))) {
        if (TestwndChild(pwnd))
            pwnd = REBASEPWND(pwnd, spwndParent);
        else
            pwnd = REBASEPWND(pwnd, spwndOwner);
        return pwnd;
    }

    /*
     * The window was not a child window; they may have been just testing
     * if it was
     */
    return NULL;
}


/***************************************************************************\
* GetSubMenu
*
* Return the handle of a popup menu.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

PMENU _GetSubMenu(
    PMENU pMenu,
    int nPos)
{
    PITEM pItem;
    PMENU pPopup = NULL;

    /*
     * Make sure nPos refers to a valid popup
     */
    if ((UINT)nPos < (UINT)((PMENU)pMenu)->cItems) {
        pItem = &((PITEM)REBASEALWAYS(pMenu, rgItems))[nPos];
        if (pItem->spSubMenu != NULL)
            pPopup = (PMENU)REBASEPTR(pMenu, pItem->spSubMenu);

    }

    return (PVOID)pPopup;
}


/***************************************************************************\
* _IsChild (API)
*
*
*
* History:
* 11-07-90 darrinm      Translated from Win 3.0 ASM code.
\***************************************************************************/

BOOL _IsChild(
    PWND pwndParent,
    PWND pwnd)
{
    /*
     * Don't need a test to get out of the loop because the
     * desktop is not a child.
     */
    while (pwnd != NULL) {
        if (!TestwndChild(pwnd))
            return FALSE;

        pwnd = REBASEPWND(pwnd, spwndParent);
        if (pwndParent == pwnd)
            return TRUE;
    }
    return FALSE;
}



/***************************************************************************\
* _IsWindowVisible (API)
*
* IsWindowVisible returns the TRUEVIS state of a window, rather than just
* the state of its WFVISIBLE flag.  According to this routine, a window is
* considered visible when it and all the windows on its parent chain are
* visible (WFVISIBLE flag set).  A special case hack was put in that causes
* any icon window being dragged to be considered as visible.
*
* History:
* 11-12-90 darrinm      Ported.
\***************************************************************************/

BOOL _IsWindowVisible(
    PWND pwnd)
{
    /*
     * Check if this is the iconic window being moved around with a mouse
     * If so, return a TRUE, though, strictly speaking, it is hidden.
     * This helps the Tracer guys from going crazy!
     * Fix for Bug #57 -- SANKAR -- 08-08-89 --
     */
    if (pwnd == NULL)
        return TRUE;

    for (;;) {
        if (!TestWF(pwnd, WFVISIBLE))
            return FALSE;
        if (GETFNID(pwnd) == FNID_DESKTOP)
            break;
        pwnd = REBASEPWND(pwnd, spwndParent);
    }

    return TRUE;
}


/***************************************************************************\
* _ClientToScreen (API)
*
* Map a point from client to screen-relative coordinates.
*
* History:
* 11-12-90 darrinm      Translated from Win 3.0 ASM code.
\***************************************************************************/

VOID _ClientToScreen(
    PWND pwnd,
    PPOINT ppt)
{
    /*
     * Client and screen coordinates are the same for the
     * desktop window.
     */
    if (GETFNID(pwnd) != FNID_DESKTOP) {
#ifdef USE_MIRRORING
        if (TestWF(pwnd, WEFLAYOUTRTL)) {
            ppt->x  = pwnd->rcClient.right - ppt->x;
        } else
#endif
        {
            ppt->x += pwnd->rcClient.left;
        }
        ppt->y += pwnd->rcClient.top;
    }
}


/***************************************************************************\
* _GetClientRect (API)
*
*
*
* History:
* 26-Oct-1990 DarrinM   Implemented.
\***************************************************************************/

VOID _GetClientRect(
    PWND   pwnd,
    LPRECT prc)
{
    /*
     * If this is a 3.1 app, and it's minimized, then we need to return
     * a rectangle other than the real-client-rect.  This is necessary since
     * there is no client-rect-size in Win4.0.  Apps such as PackRat 1.0
     * will GPF if returned a empty-rect.
     */
    if (TestWF(pwnd, WFMINIMIZED) && !TestWF(pwnd, WFWIN40COMPAT)) {
        prc->left   = 0;
        prc->top    = 0;
        prc->right  = SYSMETRTL(CXMINIMIZED);
        prc->bottom = SYSMETRTL(CYMINIMIZED);

    } else {

        if (GETFNID(pwnd) != FNID_DESKTOP) {
            *prc = pwnd->rcClient;
            OffsetRect(prc, -pwnd->rcClient.left, -pwnd->rcClient.top);
        } else {
            /*
             * For compatibility, return the rect of the primary
             * monitor for the desktop window.
             */
            prc->left = prc->top = 0;
            prc->right = SYSMETRTL(CXSCREEN);
            prc->bottom = SYSMETRTL(CYSCREEN);
        }
    }
}


/***************************************************************************\
* _GetWindowRect (API)
*
*
*
* History:
* 26-Oct-1990 DarrinM   Implemented.
\***************************************************************************/

VOID _GetWindowRect(
    PWND   pwnd,
    LPRECT prc)
{

    if (GETFNID(pwnd) != FNID_DESKTOP) {
        *prc = pwnd->rcWindow;
    } else {
        /*
         * For compatibility, return the rect of the primary
         * monitor for the desktop window.
         */
        prc->left   = 0;
        prc->top    = 0;
        prc->right  = SYSMETRTL(CXSCREEN);
        prc->bottom = SYSMETRTL(CYSCREEN);
    }
}

/***************************************************************************\
* _ScreenToClient (API)
*
* Map a point from screen to client-relative coordinates.
*
* History:
* 11-12-90 darrinm      Translated from Win 3.0 ASM code.
\***************************************************************************/

VOID _ScreenToClient(
    PWND pwnd,
    PPOINT ppt)
{
    /*
     * Client and screen coordinates are the same for the
     * desktop window.
     */
    if (GETFNID(pwnd) != FNID_DESKTOP) {
#ifdef USE_MIRRORING
        if (TestWF(pwnd, WEFLAYOUTRTL)) {
            ppt->x  = pwnd->rcClient.right - ppt->x;
        } else
#endif
        {
            ppt->x -= pwnd->rcClient.left;
        }
        ppt->y -= pwnd->rcClient.top;
    }
}
/***************************************************************************\
* PhkNextValid
*
* This helper routine walk the phkNext chain looking for the next valid
*  hook (i.e., not marked as destroyed). If the end of the local (or
*  thread specific) hook chain is reached, then it jumps to the global
*  (or desktop) chain.
*
* Once a hook is destroyed, we don't want anymore activity on it; however,
*  if the hook is locked at destroy time (= someone is calling it), then
*  we keep it in the list so CallNextHook will work properly
*
* History:
* 03/24/96  GerardoB        Moved to rtl and added *Valid stuff.
* 01-30-91  DavidPe         Created.
\***************************************************************************/
PHOOK PhkNextValid(PHOOK phk)
{

#if DBG
    int iHook = phk->iHook;
#ifdef _USERK_
    CheckCritInShared();
#endif
#endif

    do {
        /*
         * If this hook is marked as destroyed, it must be either
         *  locked or we should be in the process of destroying it
         */
        UserAssert(!(phk->flags & HF_DESTROYED)
                    || (((PHEAD)phk)->cLockObj != 0)
                    || (phk->flags & HF_INCHECKWHF));
        /*
         * Get the next hook
         */
        if (phk->phkNext != NULL) {
            phk = REBASEALWAYS(phk, phkNext);
        } else if (!(phk->flags & HF_GLOBAL)) {
#ifdef _USERK_
            phk = PtiCurrent()->pDeskInfo->aphkStart[phk->iHook + 1];
#else
            PCLIENTINFO pci = GetClientInfo();
            phk = pci->pDeskInfo->aphkStart[phk->iHook + 1];
            /*
             * If it found a pointer, rebase it.
             */
            if (phk != NULL) {
                (KPBYTE)phk -= pci->ulClientDelta;
            }
#endif
            UserAssert((phk == NULL) || (phk->flags & HF_GLOBAL));
        } else {
            return NULL;
        }
        /*
         * If destroyed, keep looking.
         */
    } while ((phk != NULL) && (phk->flags & HF_DESTROYED));

#ifdef _USERK_
    DbgValidateHooks(phk, iHook);
#endif

    return phk;
}
