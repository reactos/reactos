/****************************** Module Header ******************************\
*
* Module Name: mncreate.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Creation routines for menus
*
* Public Functions:
*
* _CreateMenu()
* _CreatePopupMenu()
*
* History:
* 09-24-90 mikeke    from win30
* 02-11-91 JimA      Added access checks.
* 03-18-91 IanJa     Window revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* InternalCreateMenu
*
* Creates and returns a handle to an empty menu structure. Returns
* NULL if unsuccessful in allocating the memory.  If PtiCurrent() ==
* NULL, create an unowned menu, probably the system menu.
*
* History:
* 28-Sep-1990 mikeke     from win30
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

PMENU InternalCreateMenu(
    BOOL fPopup)
{
    PMENU pmenu;
    PTHREADINFO ptiCurrent = PtiCurrent();
    PDESKTOP pdesk = NULL;

    /*
     * If the windowstation has been initialized, allocate from
     * the current desktop.
     */
    pdesk = ptiCurrent->rpdesk;
    /*
     * Just like in xxxCreateWindowEx, bypass the security check if hdesk is NULL
     * This allows CSR worker threads (ie harderror boxes) to do what they need to
     */
    if (ptiCurrent->hdesk != NULL) {
        RETURN_IF_ACCESS_DENIED(ptiCurrent->amdesk, DESKTOP_CREATEMENU, NULL);
    } else {
        UserAssert(ptiCurrent->TIF_flags & TIF_CSRSSTHREAD);
    }

    pmenu = HMAllocObject(ptiCurrent, pdesk, TYPE_MENU, sizeof(MENU));

    if (pmenu != NULL) {
        if (fPopup) {
            pmenu->fFlags = MFISPOPUP;
        }
    }
    return pmenu;
}


/***************************************************************************\
* CreateMenu
*
* Creates and returns a handle to an empty menu structure. Returns
* NULL if unsuccessful in allocating the memory.  If PtiCurrent() ==
* NULL, create an unowned menu, probably the system menu.
*
* History:
* 28-Sep-1990 mikeke     from win30
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

PMENU _CreateMenu()
{
    return InternalCreateMenu(FALSE);
}


/***************************************************************************\
* CreatePopupMenu
*
* Creates and returns a handle to an empty POPUP menu structure. Returns
* NULL if unsuccessful in allocating the memory.
*
* History:
* 28-Sep-1990 mikeke     from win30
\***************************************************************************/

PMENU _CreatePopupMenu()
{
    return InternalCreateMenu(TRUE);
}
