/****************************** Module Header ******************************\
* Module Name: metrics.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* MenuRecalc
*
* Loops through all menus and resets size and item info stuff.  That's
* because it is invalid when the menu font changes.
*
* History:
\***************************************************************************/
void MenuRecalc(void)
{
    PMENU   lpMenu;
    UINT    iItem;
    PHE     pheT;
    DWORD   i;

    /*
     *  for (ppi = gppiFirst; ppi; ppi = ppi->ppiNext)
     *  {
     *      for (pMenu = ppi->lpMenus; TESTFAR(lpMenu); lpMenu = lpMenu->lpMenuNext)
     *      {
     *
     * That was the Chicao way of walking the objects.  In NT, we
     * walk the handle table.
     */
    for (pheT = gSharedInfo.aheList, i = 0; i <= giheLast; i++, pheT++) {

        if (pheT->bType == TYPE_MENU) {
            /*
             * Get a pointer to the menu.
             */
            lpMenu = (PMENU)pheT->phead;

            /*
             * Set menu size to 0 so it recalculates later when we go to
             * draw it again.
             */
            lpMenu->cxMenu = 0;
            lpMenu->cyMenu = 0;

            /*
             * Reset mnemonic underline info
             */
            for (iItem = 0; iItem < lpMenu->cItems; iItem++) {
                lpMenu->rgItems[iItem].ulX = UNDERLINE_RECALC;
                lpMenu->rgItems[iItem].ulWidth = 0;
                lpMenu->rgItems[iItem].cxBmp = MNIS_MEASUREBMP;
            }
        }
    }
}


/***************************************************************************\
* xxxRecreateSmallIcons()
*
* Recreates the class and/or window small icon when the caption height
* changes.  This needs to be done in context so that LR_COPYFROMRESOURCE
* can work right.
*
* History:
* 22-Jun-95 BradG   Ported from Win95
\***************************************************************************/

VOID xxxRecreateSmallIcons(PWND pwnd)
{
    BOOL    fSmQueryDrag;

    CheckLock(pwnd);

    if (DestroyClassSmIcon(pwnd->pcls))
        xxxCreateClassSmIcon(pwnd->pcls);

    fSmQueryDrag = (TestWF(pwnd, WFSMQUERYDRAGICON) != 0);
    if (DestroyWindowSmIcon(pwnd) && !fSmQueryDrag)
        xxxCreateWindowSmIcon(pwnd, (HICON)_GetProp(pwnd, MAKEINTATOM(gpsi->atomIconProp), PROPF_INTERNAL), TRUE);
}
