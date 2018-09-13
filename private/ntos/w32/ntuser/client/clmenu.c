/****************************** Module Header ******************************\
*
* Module Name: clmenu.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Menu Loading Routines
*
* History:
* 24-Sep-1990 mikeke        From win30
* 29-Nov-1994 JimA          Moved from server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* MenuLoadWinTemplates
*
* Recursive routine that loads in the new style menu template and
* builds the menu. Assumes that the menu template header has already been
* read in and processed elsewhere...
*
* History:
* 28-Sep-1990 mikeke     from win30
\***************************************************************************/

LPBYTE MenuLoadWinTemplates(
    LPBYTE lpMenuTemplate,
    HMENU *phMenu)
{
    HMENU hMenu;
    UINT menuFlags = 0;
    ULONG_PTR menuId = 0;
    LPWSTR lpmenuText;
    MENUITEMINFO    mii;
    UNICODE_STRING str;

    if (!(hMenu = NtUserCreateMenu()))
        goto memoryerror;

    do {

        /*
         * Get the menu flags.
         */
        menuFlags = (UINT)(*(WORD *)lpMenuTemplate);
        lpMenuTemplate += 2;

        if (menuFlags & ~MF_VALID) {
            RIPERR1(ERROR_INVALID_DATA, RIP_WARNING, "Menu Flags %lX are invalid", menuFlags);
            goto memoryerror;
        }


        if (!(menuFlags & MF_POPUP)) {
            menuId = *(WORD *)lpMenuTemplate;
            lpMenuTemplate += 2;
        }

        lpmenuText = (LPWSTR)lpMenuTemplate;

        if (*lpmenuText) {
            /*
             * Some Win3.1 and Win95 16 bit apps (chessmaster, mavis typing) know that
             * dwItemData for MFT_OWNERDRAW items is a pointer to a string in the resource data.
             * So WOW has given us the proper pointer from the 16 bit resource.
             *
             * Sundown Note: 
             * __unaligned unsigned long value pointed by lpMenuTemplate is zero-extended to 
             * update lpmenuText. WOW restrictions.
             */
            if ((menuFlags & MFT_OWNERDRAW)
                    && (GetClientInfo()->dwTIFlags & TIF_16BIT)) {
                lpmenuText = (LPWSTR)ULongToPtr( (*(DWORD UNALIGNED *)lpMenuTemplate) );
                /*
                 * We'll skip one WCHAR later; so skip only the difference now.
                 */
                lpMenuTemplate += sizeof(DWORD) - sizeof(WCHAR);
            } else {
                /*
                 * If a string exists, then skip to the end of it.
                 */
                RtlInitUnicodeString(&str, lpmenuText);
                lpMenuTemplate = lpMenuTemplate + str.Length;
            }

        } else {
            lpmenuText = NULL;
        }

        /*
         * Skip over terminating NULL of the string (or the single NULL
         * if empty string).
         */
        lpMenuTemplate += sizeof(WCHAR);
        lpMenuTemplate = NextWordBoundary(lpMenuTemplate);

        RtlZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_STATE | MIIM_FTYPE;
        if (lpmenuText) {
            mii.fMask |= MIIM_STRING;
        }

        if (menuFlags & MF_POPUP) {
            mii.fMask |= MIIM_SUBMENU;
            lpMenuTemplate = MenuLoadWinTemplates(lpMenuTemplate,
                    (HMENU *)&menuId);
            if (!lpMenuTemplate)
                goto memoryerror;

            mii.hSubMenu = (HMENU)menuId;
        }

        /*
         * We have to take out MF_HILITE since that bit marks the end of a
         * menu in a resource file.  Since we shouldn't have any pre hilited
         * items in the menu anyway, this is no big deal.
         */
        if (menuFlags & MF_BITMAP) {

            /*
             * Don't allow bitmaps from the resource file.
             */
            menuFlags = (UINT)((menuFlags | MFT_RIGHTJUSTIFY) & ~MF_BITMAP);
        }

        // We have to take out MFS_HILITE since that bit marks the end of a menu in
        // a resource file.  Since we shouldn't have any pre hilited items in the
        // menu anyway, this is no big deal.
        mii.fState = (menuFlags & MFS_OLDAPI_MASK) & ~MFS_HILITE;
        mii.fType = (menuFlags & MFT_OLDAPI_MASK);
        if (menuFlags & MFT_OWNERDRAW)
        {
            mii.fMask |= MIIM_DATA;
            mii.dwItemData = (ULONG_PTR) lpmenuText;
            lpmenuText = 0;
        }
        mii.dwTypeData = (LPWSTR) lpmenuText;
        mii.cch = (UINT)-1;
        mii.wID = (UINT)menuId;

        if (!NtUserThunkedMenuItemInfo(hMenu, MFMWFP_NOITEM, TRUE, TRUE,
                    &mii, lpmenuText ? &str : NULL)) {
            if (menuFlags & MF_POPUP)
                NtUserDestroyMenu(mii.hSubMenu);
            goto memoryerror;
        }

    } while (!(menuFlags & MF_END));

    *phMenu = hMenu;
    return lpMenuTemplate;

memoryerror:
    if (hMenu != NULL)
        NtUserDestroyMenu(hMenu);
    *phMenu = NULL;
    return NULL;
}


/***************************************************************************\
* MenuLoadChicagoTemplates
*
* Recursive routine that loads in the new new style menu template and
* builds the menu. Assumes that the menu template header has already been
* read in and processed elsewhere...
*
* History:
* 15-Dec-93 SanfordS    Created
\***************************************************************************/

PMENUITEMTEMPLATE2 MenuLoadChicagoTemplates(
    PMENUITEMTEMPLATE2 lpMenuTemplate,
    HMENU *phMenu,
    WORD wResInfo,
    UINT mftRtl)
{
    HMENU hMenu;
    HMENU hSubMenu;
    long menuId = 0;
    LPWSTR lpmenuText;
    MENUITEMINFO    mii;
    UNICODE_STRING str;
    DWORD           dwHelpID;

    if (!(hMenu = NtUserCreateMenu()))
        goto memoryerror;

    do {
        if (!(wResInfo & MFR_POPUP)) {
            /*
             * If the PREVIOUS wResInfo field was not a POPUP, the
             * dwHelpID field is not there.  Back up so things fit.
             */
            lpMenuTemplate = (PMENUITEMTEMPLATE2)(((LPBYTE)lpMenuTemplate) -
                    sizeof(lpMenuTemplate->dwHelpID));
            dwHelpID = 0;
        } else
            dwHelpID = lpMenuTemplate->dwHelpID;

        menuId = lpMenuTemplate->menuId;

        RtlZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_STATE | MIIM_FTYPE ;

        mii.fType = lpMenuTemplate->fType | mftRtl;
        if (mii.fType & ~MFT_MASK) {
            RIPERR1(ERROR_INVALID_DATA, RIP_WARNING, "Menu Type flags %lX are invalid", mii.fType);
            goto memoryerror;
        }

        mii.fState  = lpMenuTemplate->fState;
        if (mii.fState & ~MFS_MASK) {
            RIPERR1(ERROR_INVALID_DATA, RIP_WARNING, "Menu State flags %lX are invalid", mii.fState);
            goto memoryerror;
        }

        wResInfo = lpMenuTemplate->wResInfo;
        if (wResInfo & ~(MF_END | MFR_POPUP)) {
            RIPERR1(ERROR_INVALID_DATA, RIP_WARNING, "Menu ResInfo flags %lX are invalid", wResInfo);
            goto memoryerror;
        }

        if (dwHelpID) {
            NtUserSetMenuContextHelpId(hMenu,dwHelpID);
        }
        if (lpMenuTemplate->mtString[0]) {
            lpmenuText = lpMenuTemplate->mtString;
            mii.fMask |= MIIM_STRING;
        } else {
            lpmenuText = NULL;
        }
        RtlInitUnicodeString(&str, lpmenuText);

        mii.dwTypeData = (LPWSTR) lpmenuText;

        /*
         * skip to next menu item template (DWORD boundary)
         */
        lpMenuTemplate = (PMENUITEMTEMPLATE2)
                (((LPBYTE)lpMenuTemplate) +
                sizeof(MENUITEMTEMPLATE2) +
                ((str.Length + 3) & ~3));

        if (mii.fType & MFT_OWNERDRAW)
        {
            mii.fMask |= MIIM_DATA;
            mii.dwItemData = (ULONG_PTR) mii.dwTypeData;
            mii.dwTypeData = 0;
        }

        /*
         * If MFT_RIGHTORDER is specified then all subsequent
         * menus are right-to-left as well.
         */
        if (mii.fType & MFT_RIGHTORDER)
        {
            mftRtl = MFT_RIGHTORDER;
            NtUserSetMenuFlagRtoL(hMenu);
        }

        if (wResInfo & MFR_POPUP) {
            mii.fMask |= MIIM_SUBMENU;
            lpMenuTemplate = MenuLoadChicagoTemplates(lpMenuTemplate,
                    &hSubMenu, MFR_POPUP, mftRtl);
            if (lpMenuTemplate == NULL)
                goto memoryerror;
            mii.hSubMenu = hSubMenu;
        }

        if (mii.fType & MFT_BITMAP) {

            /*
             * Don't allow bitmaps from the resource file.
             */
            mii.fType = (mii.fType | MFT_RIGHTJUSTIFY) & ~MFT_BITMAP;
        }

        mii.cch = (UINT)-1;
        mii.wID = menuId;
        if (!NtUserThunkedMenuItemInfo(hMenu, MFMWFP_NOITEM, TRUE, TRUE,
                    &mii, &str)) {
            if (wResInfo & MFR_POPUP)
                NtUserDestroyMenu(mii.hSubMenu);
            goto memoryerror;
        }
        wResInfo &= ~MFR_POPUP;
    } while (!(wResInfo & MFR_END));

    *phMenu = hMenu;
    return lpMenuTemplate;

memoryerror:
    if (hMenu != NULL)
        NtUserDestroyMenu(hMenu);
    *phMenu = NULL;
    return NULL;
}


/***************************************************************************\
* CreateMenuFromResource
*
* Loads the menu resource named by the lpMenuTemplate parameter. The
* template specified by lpMenuTemplate is a collection of one or more
* MENUITEMTEMPLATE structures, each of which may contain one or more items
* and popup menus. If successful, returns a handle to the menu otherwise
* returns NULL.
*
* History:
* 28-Sep-1990 mikeke     from win30
\***************************************************************************/

HMENU CreateMenuFromResource(
    LPBYTE lpMenuTemplate)
{
    HMENU hMenu = NULL;
    UINT menuTemplateVersion;
    UINT menuTemplateHeaderSize;

    /*
     * Win3 menu resource: First, strip version number word out of the menu
     * template.  This value should be 0 for Win3, 1 for win4.
     */
    menuTemplateVersion = *((WORD *)lpMenuTemplate)++;
    if (menuTemplateVersion > 1) {
        RIPMSG0(RIP_WARNING, "Menu Version number > 1");
        return NULL;
    }
    menuTemplateHeaderSize = *((WORD *)lpMenuTemplate)++;
    lpMenuTemplate += menuTemplateHeaderSize;
    switch (menuTemplateVersion) {
    case 0:
        MenuLoadWinTemplates(lpMenuTemplate, &hMenu);
        break;

    case 1:
        MenuLoadChicagoTemplates((PMENUITEMTEMPLATE2)lpMenuTemplate, &hMenu, 0, 0);
        break;
    }
    return hMenu;
}

/***************************************************************************\
* SetMenu (API)
*
* Sets the menu for the hwnd.
*
* History:
* 10-Mar-1996 ChrisWil  Created.
\***************************************************************************/

BOOL SetMenu(
    HWND  hwnd,
    HMENU hmenu)
{
    return NtUserSetMenu(hwnd, hmenu, TRUE);
}

/***************************************************************************\
* LoadMenu (API)
*
* Loads the menu resource named by lpMenuName from the executable
* file associated by the module specified by the hInstance parameter. The
* menu is loaded only if it hasn't been previously loaded. Otherwise it
* retrieves a handle to the loaded resource. Returns NULL if unsuccessful.
*
* History:
* 04-05-91 ScottLu Fixed to work with client/server.
* 28-Sep-1990 mikeke from win30
\***************************************************************************/

HMENU CommonLoadMenu(
    HINSTANCE hmod,
    HANDLE hResInfo
    )
{
    HANDLE h;
    PVOID p;
    HMENU hMenu = NULL;

    if (h = LOADRESOURCE(hmod, hResInfo)) {

        if (p = LOCKRESOURCE(h, hmod)) {

            hMenu = CreateMenuFromResource(p);

            UNLOCKRESOURCE(h, hmod);
        }
        /*
         * Win95 and Win3.1 do not free this resource; some 16 bit apps (chessmaster
         * and mavis typing) require this for their ownerdraw menu stuff.
         * For 32 bit apps, FreeResource is a nop anyway. For 16 bit apps,
         * Wow frees the 32 bit resource (returned by LockResource16)
         * in UnlockResource16; the actual 16 bit resource is freed when the task
         * goes away.
         *
         *   FREERESOURCE(h, hmod);
         */
    }

    return (hMenu);
}

HMENU WINAPI LoadMenuA(
    HINSTANCE hmod,
    LPCSTR lpName)
{
    HANDLE hRes;

    if (hRes = FINDRESOURCEA(hmod, (LPSTR)lpName, (LPSTR)RT_MENU))
        return CommonLoadMenu(hmod, hRes);
    else
        return NULL;
}

HMENU WINAPI LoadMenuW(
    HINSTANCE hmod,
    LPCWSTR lpName)
{
    HANDLE hRes;

    if (hRes = FINDRESOURCEW(hmod, (LPWSTR)lpName, RT_MENU))
        return CommonLoadMenu(hmod, hRes);
    else
        return NULL;
}
/***************************************************************************\
* InternalInsertMenuItem
*
* History:
*  09/20/96 GerardoB - Created
\***************************************************************************/
BOOL InternalInsertMenuItem (HMENU hMenu, UINT uID, BOOL fByPosition, LPCMENUITEMINFO lpmii)
{
 return ThunkedMenuItemInfo(hMenu, uID, fByPosition, TRUE, (LPMENUITEMINFOW)lpmii, FALSE);
}

/***************************************************************************\
* ValidateMENUITEMINFO() -
*   it converts and validates a MENUITEMINFO95 or a new-MENUITEMINFO-with-old-flags
*     to a new MENUITEMINFO -- this way all internal code can assume one look for the
*   structure
*
* History:
*  12-08-95 Ported from Nashville - jjk
*  07-19-96 GerardoB - Fixed up for 5.0
\***************************************************************************/
BOOL ValidateMENUITEMINFO(LPMENUITEMINFO lpmiiIn, LPMENUITEMINFO lpmii, DWORD dwAPICode)
{

    VALIDATIONFNNAME(ValidateMENUITEMINFO)
    BOOL fOldApp;

    if (lpmiiIn == NULL) {
        VALIDATIONFAIL(lpmiiIn);
    }

    /*
     * In order to map the old flags to the new ones, we might have to modify
     *  the lpmiiIn structure. So we make a copy to avoid breaking anyone.
     */
    fOldApp = (lpmiiIn->cbSize == SIZEOFMENUITEMINFO95);
    UserAssert(SIZEOFMENUITEMINFO95 < sizeof(MENUITEMINFO));
    RtlCopyMemory(lpmii, lpmiiIn, SIZEOFMENUITEMINFO95);
    if (fOldApp) {
        lpmii->cbSize = sizeof(MENUITEMINFO);
        lpmii->hbmpItem = NULL;
    } else if (lpmiiIn->cbSize == sizeof(MENUITEMINFO)) {
        lpmii->hbmpItem = lpmiiIn->hbmpItem;
    } else {
        VALIDATIONFAIL(lpmiiIn->cbSize);
    }


    if (lpmii->fMask & ~MIIM_MASK) {
        VALIDATIONFAIL(lpmii->fMask);
    } else if ((lpmii->fMask & MIIM_TYPE)
            && (lpmii->fMask & (MIIM_FTYPE | MIIM_STRING | MIIM_BITMAP))) {
        /*
         * Don't let them mix new and old flags
         */
        VALIDATIONFAIL(lpmii->fMask);
    }

    /*
     * No more validation needed for Get calls
     */
    if (dwAPICode == MENUAPI_GET) {
        /*
         * Map MIIM_TYPE for old apps doing a Get.
         * Keep the MIIM_TYPE flag so we'll know this guy passed the old flags.
         * GetMenuItemInfo uses lpmii->hbmpItem to determine if a bitmap
         *  was returned. So we NULL it out here. The caller is using the
         *  old flags so he shouldn't care about it.
         */
        if (lpmii->fMask & MIIM_TYPE) {
            lpmii->fMask |= MIIM_FTYPE | MIIM_BITMAP | MIIM_STRING;
            lpmii->hbmpItem = NULL;
        }
        return TRUE;
    }

    /*
     * Map MIIM_TYPE to MIIM_FTYPE
     */
    if (lpmii->fMask & MIIM_TYPE) {
        lpmii->fMask |= MIIM_FTYPE;
    }

    if (lpmii->fMask & MIIM_FTYPE) {
        if (lpmii->fType & ~MFT_MASK) {
            VALIDATIONFAIL(lpmii->fType);
        }
        /*
         * If using MIIM_TYPE, Map MFT_BITMAP to MIIM_BITMAP
         *  and MFT_NONSTRING to MIIM_STRING.
         * Old applications couldn't use string and bitmap simultaneously
         *  so setting one implies clearing the other.
         */
        if (lpmii->fMask & MIIM_TYPE) {
            if (lpmii->fType & MFT_BITMAP) {
                /*
                 * Don't display a warning. A lot of shell menus hit this
                 * if (!fOldApp) {
                 *     VALIDATIONOBSOLETE(MFT_BITMAP, MIIM_BITMAP);
                 *  }
                 */
                lpmii->fMask |= MIIM_BITMAP | MIIM_STRING;
                lpmii->hbmpItem = (HBITMAP)lpmii->dwTypeData;
                lpmii->dwTypeData = 0;
            } else if (!(lpmii->fType & MFT_NONSTRING)) {
                /*
                 * Don't display a warning. A lot of shell menus hit this
                 * if (!fOldApp) {
                 *     VALIDATIONOBSOLETE(MFT_STRING, MIIM_STRING);
                 *  }
                 */
                lpmii->fMask |= MIIM_BITMAP | MIIM_STRING;
                lpmii->hbmpItem = NULL;
            }
        } else if (lpmii->fType & MFT_BITMAP) {
            /*
             * Don't let them mix new and old flags
             */
            VALIDATIONFAIL(lpmii->fType);
        }
    }

    if ((lpmii->fMask & MIIM_STATE) && (lpmii->fState & ~MFS_MASK)){
        VALIDATIONFAIL(lpmii->fState);
    }

    if (lpmii->fMask & MIIM_CHECKMARKS) {
        if ((lpmii->hbmpChecked != NULL) && !GdiValidateHandle((HBITMAP)lpmii->hbmpChecked)) {
            VALIDATIONFAIL(lpmii->hbmpChecked);
        }
        if ((lpmii->hbmpUnchecked != NULL) && !GdiValidateHandle((HBITMAP)lpmii->hbmpUnchecked)) {
            VALIDATIONFAIL(lpmii->hbmpUnchecked);
        }
    }

    if (lpmii->fMask & MIIM_SUBMENU) {
        if ((lpmii->hSubMenu != NULL) && !VALIDATEHMENU(lpmii->hSubMenu)) {
            VALIDATIONFAIL(lpmii->hSubMenu);
        }
    }

    /*
     * Warning: NULL lpmii->hbmpItem accepted as valid (or the explorer breaks)
     */
    if (lpmii->fMask & MIIM_BITMAP) {
        if ((lpmii->hbmpItem != HBMMENU_CALLBACK)
                && (lpmii->hbmpItem >= HBMMENU_MAX)
                && !GdiValidateHandle(lpmii->hbmpItem)) {

            /*
             * Compatibility hack
             */
            if (((HBITMAP)LOWORD(HandleToUlong(lpmii->hbmpItem)) >= HBMMENU_MAX) || !IS_PTR(lpmii->hbmpItem)) {
                VALIDATIONFAIL(lpmii->hbmpItem);
            }
        }
    }

    /*
     * Warning: No dwTypeData / cch validation
     */

    return TRUE;

    VALIDATIONERROR(FALSE);
}

/***************************************************************************\
* ValidateMENUINFO() -
*
* History:
*  07-22-96 GerardoB - Added header and Fixed up for 5.0
\***************************************************************************/

BOOL ValidateMENUINFO(LPCMENUINFO lpmi, DWORD dwAPICode)
{
    VALIDATIONFNNAME(ValidateMENUINFO)

    if (lpmi == NULL) {
        VALIDATIONFAIL(lpmi);
    }

    if (lpmi->cbSize != sizeof(MENUINFO)) {
        VALIDATIONFAIL(lpmi->cbSize);
    }

    if (lpmi->fMask & ~MIM_MASK) {
        VALIDATIONFAIL(lpmi->fMask);
    }

    /*
     * No more validation needed for Get calls
     */
    if (dwAPICode == MENUAPI_GET){
        return TRUE;
    }

    if ((lpmi->fMask & MIM_STYLE) && (lpmi->dwStyle & ~MNS_VALID)) {
        VALIDATIONFAIL(lpmi->dwStyle);
    }

    if (lpmi->fMask & MIM_BACKGROUND) {
        if ((lpmi->hbrBack != NULL)
                && !GdiValidateHandle((HBRUSH)lpmi->hbrBack)) {

            VALIDATIONFAIL(lpmi->hbrBack);
        }
    }

    return TRUE;

    VALIDATIONERROR(FALSE);
}
/***************************************************************************\
* GetMenuInfo
*
* History:
*  07-22-96 GerardoB - Added header and Fixed up for 5.0
\***************************************************************************/
BOOL GetMenuInfo(HMENU hMenu, LPMENUINFO lpmi)
{
    PMENU pMenu;

    if (!ValidateMENUINFO(lpmi, MENUAPI_GET)) {
        return FALSE;
    }

    pMenu = VALIDATEHMENU(hMenu);
    if (pMenu == NULL) {
        return FALSE;
    }

    if (lpmi->fMask & MIM_STYLE) {
        lpmi->dwStyle = pMenu->fFlags & MNS_VALID;
    }

    if (lpmi->fMask & MIM_MAXHEIGHT) {
        lpmi->cyMax = pMenu->cyMax;
    }

    if (lpmi->fMask & MIM_BACKGROUND) {
        lpmi->hbrBack = pMenu->hbrBack;
    }

    if (lpmi->fMask & MIM_HELPID) {
        lpmi->dwContextHelpID = pMenu->dwContextHelpId;
    }

    if (lpmi->fMask & MIM_MENUDATA) {
        lpmi->dwMenuData = KERNEL_ULONG_PTR_TO_ULONG_PTR(pMenu->dwMenuData);
    }

    return TRUE;
}
