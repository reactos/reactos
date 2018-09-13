/**************************** Module Header ********************************\
* Module Name: mndstry.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Menu Destruction Routines
*
* History:
* 10-10-90 JimA       Created.
* 02-11-91 JimA       Added access checks.
* 03-18-91 IanJa      Window revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* DestroyMenu
*
* Destroy a menu and free its memory.
*
* History:
* 10-11-90 JimA         Translated from ASM.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

BOOL _DestroyMenu(
    PMENU pMenu)
{
    PITEM pItem;
    int i;
    PDESKTOP rpdeskLock;

    if (pMenu == NULL)
        return FALSE;

    /*
     * If the object is locked, just mark it for destroy and don't
     * free it yet.
     */
    if (!HMMarkObjectDestroy(pMenu))
        return TRUE;

    /*
     * Go down the item list and free the items
     */
    pItem = pMenu->rgItems;
    for (i = pMenu->cItems; i--; ++pItem)
        MNFreeItem(pMenu, pItem, TRUE);

    /*
     * free the menu items
     */
    if (pMenu->rgItems)
        DesktopFree(pMenu->head.rpdesk, pMenu->rgItems);

    /*
     * Because menus are the only objects on the desktop owned
     * by the process and process cleanup is done after thread
     * cleanup, this may be the last reference to the desktop.
     * We must lock the desktop before unlocking
     * the parent desktop reference and freeing the menu to
     * ensure that the desktop will not be freed until after
     * the menu is freed.  Don't use static locks because
     * the pti for this thread will not be valid during
     * process cleanup.
     */
    rpdeskLock = NULL;
    LockDesktop(&rpdeskLock, pMenu->head.rpdesk, LDL_FN_DESTROYMENU, (ULONG_PTR)PtiCurrent());

    /*
     * Unlock all menu objects.
     */
    Unlock(&pMenu->spwndNotify);

    HMFreeObject(pMenu);

    UnlockDesktop(&rpdeskLock, LDU_FN_DESTROYMENU, (ULONG_PTR)PtiCurrent());

    return TRUE;
}
