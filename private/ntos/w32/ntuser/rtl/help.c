/****************************** Module Header ******************************\
* Module Name: help.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the various rectangle manipulation APIs.
*
* History:
* 23-May-95 BradG   Created for Kernel mode
\***************************************************************************/

BOOL FIsParentDude(PWND pwnd)
{
    return(TestWF(pwnd, WEFCONTROLPARENT) || TestWF(pwnd, WFDIALOGWINDOW) ||
        ((GETFNID(pwnd) == FNID_BUTTON) &&
         (TestWF(pwnd, BFTYPEMASK) == BS_GROUPBOX)));
}


/***************************************************************************\
* GetContextHelpId()
*   Given a pwnd, this returns the Help Context Id for that window;
* Note: If a window does not have a Context Id of its own, then it inherits
* the ContextId of it's parent, if it is a child window; else, from its owner,
* it is a owned popup.
\***************************************************************************/

DWORD GetContextHelpId(
    PWND pwnd)
{
    DWORD  dwContextId;

    while (!(dwContextId = (DWORD)(ULONG_PTR)_GetProp(pwnd,
            MAKEINTATOM(gpsi->atomContextHelpIdProp), PROPF_INTERNAL))) {
        pwnd = (TestwndChild(pwnd) ?
                REBASEPWND(pwnd, spwndParent) :
                REBASEPWND(pwnd, spwndOwner));
        if (!pwnd || (GETFNID(pwnd) == FNID_DESKTOP))
            break;
    }

    return dwContextId;
}




/*
 * Dialog Child enumeration proc
 *
 * Enumerates children of a dialog looking for the child under the mouse.
 *
 */
BOOL CALLBACK EnumPwndDlgChildProc(PWND pwnd, LPARAM lParam)
{
    PDLGENUMDATA pDlgEnumData = (PDLGENUMDATA)lParam;

    if (pwnd != pDlgEnumData->pwndDialog && IsVisible(pwnd) &&
            PtInRect(&((WND *)pwnd)->rcWindow, pDlgEnumData->ptCurHelp)) {
        /*
         * If it's a group box, keep enumerating. This takes care of
         * the case where we have a disabled control in a group box.
         * We'll find the group box first, and keep enumerating until we
         * hit the disabled control.
         */
        pDlgEnumData->pwndControl = pwnd;
        return (FIsParentDude(pwnd));
    }
    return TRUE;
}
