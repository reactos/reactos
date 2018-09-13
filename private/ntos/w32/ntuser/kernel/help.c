/**************************** Module Header ********************************\
* Module Name: help.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Help function
*
* History:
* 04-15-91 JimA             Ported.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

DWORD _GetWindowContextHelpId(PWND pWnd)
{
    return (DWORD)(ULONG_PTR)_GetProp(pWnd, MAKEINTATOM(gpsi->atomContextHelpIdProp),
            PROPF_INTERNAL);
}


BOOL _SetWindowContextHelpId(PWND pWnd, DWORD dwContextId)
{
    //If dwContextId is NULL, then this implies that the caller wants to
    // remove the dwContextId associated with this Window.
    if(dwContextId == 0) {
        InternalRemoveProp(pWnd, MAKEINTATOM(gpsi->atomContextHelpIdProp),
                PROPF_INTERNAL);
        return(TRUE);
      }

    return (InternalSetProp(pWnd, MAKEINTATOM(gpsi->atomContextHelpIdProp),
            (HANDLE)LongToHandle( dwContextId ), PROPF_INTERNAL | PROPF_NOPOOL));
}


/***************************************************************************\
* SendHelpMessage
*
*
\***************************************************************************/

void xxxSendHelpMessage(
    PWND   pwnd,
    int    iType,
    int    iCtrlId,
    HANDLE hItemHandle,
    DWORD  dwContextId)
{
    HELPINFO    HelpInfo;
    long        lValue;

    CheckLock(pwnd);

    HelpInfo.cbSize = sizeof(HELPINFO);
    HelpInfo.iContextType = iType;
    HelpInfo.iCtrlId = iCtrlId;
    HelpInfo.hItemHandle = hItemHandle;
    HelpInfo.dwContextId = dwContextId;

    lValue = _GetMessagePos();
    HelpInfo.MousePos.x = GET_X_LPARAM(lValue);
    HelpInfo.MousePos.y = GET_Y_LPARAM(lValue);

    xxxSendMessage(pwnd, WM_HELP, 0, (LPARAM)(LPHELPINFO)&HelpInfo);
}


/*
 * Modal loop for when the user has selected the help icon from the titlebar
 *
 */
VOID xxxHelpLoop(PWND pwnd)
{
    HWND        hwndChild;
    PWND        pwndChild;
    PWND        pwndControl;
    MSG         msg;
    RECT        rc;
    int         cBorders;
    PTHREADINFO ptiCurrent = PtiCurrent();
    DLGENUMDATA DlgEnumData;
    TL          tlpwndChild;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_CONTEXTHELPSTART, pwnd, OBJID_WINDOW,
            INDEXID_CONTAINER, 0);
    }

    zzzSetCursor(SYSCUR(HELP));
    xxxCapture(ptiCurrent, pwnd, SCREEN_CAPTURE);

    cBorders = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);

    CopyInflateRect(&rc, &pwnd->rcWindow, -cBorders * SYSMET(CXBORDER), -cBorders * SYSMET(CYBORDER));

    while (ptiCurrent->pq->spwndCapture == pwnd) {
        if (!xxxPeekMessage(&msg, NULL, 0, 0, PM_NOYIELD | PM_NOREMOVE)) {
            xxxWaitMessage();
            continue;
        }

        if (msg.message == WM_NCLBUTTONDOWN) {
            break;
        } else if (msg.message == WM_LBUTTONDOWN) {
            /*
             *  If user clicked outside of window client, bail out now.
             */
            if (!PtInRect(&rc, msg.pt))
                break;

            /*
             *  WindowHitTest() won't return a static control's handle
             */
            hwndChild = xxxWindowHitTest(pwnd, msg.pt, NULL, 0);
            pwndChild = ValidateHwnd( hwndChild );
            ThreadLock(pwndChild, &tlpwndChild);

            if (pwndChild && FIsParentDude(pwndChild))
            {
                /*
                 * If this is a dialog class, then one of three things has
                 * happened:
                 *
                 *  o   This is a static text control
                 *  o   This is the background of the dialog box.
                 *
                 * What we do is enumerate the child windows and see if
                 * any of them contain the current cursor point. If they do,
                 * change our window handle and continue on. Otherwise,
                 * return doing nothing -- we don't want context-sensitive
                 * help for a dialog background.
                 *
                 * If this is a group box, then we might have clicked on a
                 * disabled control, so we enumerate child windows to see
                 * if we get another control.
                 */

                /*
                 *  We're enumerating a dialog's children.  So, if we don't
                 *  find any matches, hwndChild will be NULL and the check
                 *  below will drop out.
                 */
                DlgEnumData.pwndDialog = pwndChild;
                DlgEnumData.pwndControl = NULL;
                DlgEnumData.ptCurHelp = msg.pt;
                xxxInternalEnumWindow(pwndChild, EnumPwndDlgChildProc, (LPARAM)&DlgEnumData, BWL_ENUMCHILDREN);
                pwndControl = DlgEnumData.pwndControl;
            } else {
                pwndControl = pwndChild;
            }

            /*
             * If we click on nothing, just exit.
             */
            if (pwndControl == pwnd) {
                pwndControl = NULL;
            }

            /*
             *  HACK ALERT (Visual Basic 4.0) - they have their own non-window
             *    based controls that draw directly on the main dialog.  In order
             *    to provide help for these controls, we pass along the WM_HELP
             *    message iff the main dialog has a context id assigned.
             *
             *  If the top level window has its own context help ID,
             *  then pass it in the context help message.
             */
            if (!pwndControl) {
                if (_GetProp(pwnd, MAKEINTATOM(gpsi->atomContextHelpIdProp), TRUE))
                    pwndControl = pwnd;
            }

            if (pwndControl) {
                PWND    pwndSend;
                int     id;
                TL      tlpwndSend;
                TL      tlpwndControl;

                ThreadLockAlways(pwndControl, &tlpwndControl);

                zzzSetCursor(SYSCUR(ARROW));
                xxxReleaseCapture();
                xxxRedrawTitle(pwnd, DC_BUTTONS);
                ClrWF(pwnd, WFHELPBUTTONDOWN);
                xxxGetMessage(&msg, NULL, 0, 0);

                if (FWINABLE()) {
                    xxxWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_TITLEBAR,
                        INDEX_TITLEBAR_HELPBUTTON, FALSE);

                    xxxWindowEvent(EVENT_SYSTEM_CONTEXTHELPEND, pwnd, OBJID_WINDOW,
                        INDEXID_CONTAINER, FALSE);
                }

                /*
                 * Determine the ID of the control
                 * We used to always sign extend, but Win98 doesn't do that
                 * so we only sign extend 0xffff.  MCostea #218711
                 */
                if (TestwndChild(pwndControl)) {
                    id = PTR_TO_ID(pwndControl->spmenu);
                    if (id == 0xffff) {
                        id = -1;
                    }
                } else {
                    id = -1;
                }

                /*
                 * Disabled controls and static controls won't pass this
                 * on to their parent, so instead, we send the message to
                 * their parent.
                 */

                if (TestWF(pwndControl, WFDISABLED)) {
                    PWND pwndParent = _GetParent(pwndControl);
                    if (!pwndParent)
                    {
                        ThreadUnlock( &tlpwndControl );
                        ThreadUnlock( &tlpwndChild );
                        return;
                    }
                    pwndSend = pwndParent;
                } else {
                    pwndSend = pwndControl;
                }

                ThreadLockAlways(pwndSend, &tlpwndSend);
                xxxSendHelpMessage( pwndSend, HELPINFO_WINDOW, id,
                    (HANDLE)HWq(pwndControl), GetContextHelpId(pwndControl));
                ThreadUnlock(&tlpwndSend);
                ThreadUnlock(&tlpwndControl);
                ThreadUnlock(&tlpwndChild);
                return;
            }
            ThreadUnlock(&tlpwndChild);
            break;

        }
        else if ((msg.message == WM_RBUTTONDOWN) || 
                 (msg.message == WM_MBUTTONDOWN) || 
                 (msg.message == WM_XBUTTONDOWN)) {
            /*
             *  fix bug 29852; break the loop for right and middle buttons
             *  and pass along the messages to the control
             */
            break;
        }
        else if (msg.message == WM_MOUSEMOVE) {
            if (PtInRect(&rc, msg.pt))
                zzzSetCursor(SYSCUR(HELP));
            else
                zzzSetCursor(SYSCUR(ARROW));
        }
        else if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
        {
            xxxGetMessage( &msg, NULL, 0, 0 );
            break;
        }

        xxxGetMessage(&msg, NULL, 0, 0);
        xxxTranslateMessage(&msg, 0);
        xxxDispatchMessage(&msg);
    }

    xxxReleaseCapture();
    zzzSetCursor(SYSCUR(ARROW));
    xxxRedrawTitle(pwnd, DC_BUTTONS);

    ClrWF(pwnd, WFHELPBUTTONDOWN);
    if (FWINABLE()) {
        xxxWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_TITLEBAR,
                INDEX_TITLEBAR_HELPBUTTON, 0);

        xxxWindowEvent(EVENT_SYSTEM_CONTEXTHELPEND, pwnd, OBJID_WINDOW,
                INDEXID_CONTAINER, 0);
    }
}
