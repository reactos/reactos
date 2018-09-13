#include "ctlspriv.h"

// Stuff taken from USER 4.0 for dialog handling

HWND FAR PASCAL PrevGroupItem(HWND hwndDlg, HWND hwndCurrent);
HWND FAR PASCAL NextGroupItem(HWND hwndDlg, HWND hwndCurrent);
HWND FAR PASCAL IGetNextDlgTabItem(HWND hwndDlg, HWND hwnd, BOOL fPrev);
HWND FAR PASCAL IGetNextDlgGroupItem(HWND hwndDlg, HWND hwnd, BOOL fPrev);
void FAR PASCAL DlgSetFocus(HWND hwnd);
void FAR PASCAL CheckDefPushButton(HWND hwndRoot, HWND hwndOldFocus, HWND hwndNewFocus);
void FAR PASCAL RemoveDefaultButton(HWND hwndRoot, HWND hwndStart);
HWND FAR PASCAL NextControl(HWND hwndRoot, HWND hwndStart, BOOL fSkipInvDis);
HWND FAR PASCAL PrevControl(HWND hwndRoot, HWND hwndStart, BOOL fSkipInvDis);
HWND FAR PASCAL GetChildControl(HWND hwndRoot, HWND hwndChild);


BOOL FAR PASCAL Win31IsKeyMessage(HWND hwndDlg, LPMSG lpmsg)
{
    HWND    hwnd;
    HWND    hwndNext;
    UINT    code;
    BOOL    fBack;
    LONG    lT;
    DWORD   iOK;

    // Is tab message?
    if (lpmsg->message != WM_KEYDOWN)
        return FALSE;

    // Get the current item
    hwnd = lpmsg->hwnd;
    if (!hwnd)
        return FALSE;
    
    // Check the windows...
    if ((hwnd != hwndDlg) && !IsChild(hwndDlg, hwnd))
        return FALSE;

    // Check if the control wants the message
    code = FORWARD_WM_GETDLGCODE(hwnd,lpmsg,SendMessage);
    if (code & (DLGC_WANTALLKEYS | DLGC_WANTMESSAGE))
        return FALSE;

    // Init for arrow and cancel keys
    fBack = FALSE;
    iOK = IDCANCEL;

    switch (lpmsg->wParam)
    {
    case VK_TAB:
        if (code & DLGC_WANTTAB)
            return FALSE;

        // Get the next tab item
        hwndNext = IGetNextDlgTabItem(hwndDlg, hwnd, (GetKeyState(VK_SHIFT) < 0));

        // Set focus to the next item
        DlgSetFocus(hwndNext);

        // Check the default push button
        CheckDefPushButton(hwndDlg, hwnd, hwndNext);
        return TRUE;

    case VK_LEFT:
    case VK_UP:
        fBack = TRUE;
        // fall thru
    case VK_RIGHT:
    case VK_DOWN:
        if (code & DLGC_WANTARROWS)
            return FALSE;

        hwndNext = IGetNextDlgGroupItem(hwndDlg, hwnd, fBack);
        code = FORWARD_WM_GETDLGCODE(hwndNext,lpmsg,SendMessage);

        //
        // We are just moving the focus rect around! So, do not
        // send BN_CLICK messages, when WM_SETFOCUSing.  Fix for
        // Bug #4358.
        //
        if (code & (DLGC_UNDEFPUSHBUTTON | DLGC_DEFPUSHBUTTON))
        {
            // BUGBUG this buttonstate think only seems to be
            // important for radio buttons if it is the default
            // radio button
            // BUTTONSTATE can only be used within the user module

            // BUTTONSTATE(hwndNext) |= BFDONTCLICK;
            DlgSetFocus(hwndNext);
            // BUTTONSTATE(hwndNext) &= ~BFDONTCLICK;
            CheckDefPushButton(hwndDlg, hwnd, hwndNext);
        }
        else if (code & DLGC_RADIOBUTTON)
        {
            DlgSetFocus(hwndNext);
            CheckDefPushButton(hwndDlg, hwnd, hwndNext);

            if (GetWindowStyle(hwnd) & BS_AUTORADIOBUTTON)
            {
                // So that auto radio buttons get clicked on
                if (!SendMessage(hwndNext, BM_GETCHECK, 0, 0L))
                    SendMessage(hwndNext, BM_CLICK, (WPARAM)TRUE, 0L);
            }
        }
        else if (!(code & DLGC_STATIC))
        {
            DlgSetFocus(hwndNext);
            CheckDefPushButton(hwndDlg, hwnd, hwndNext);
        }
        return TRUE;

    case VK_EXECUTE:
    case VK_RETURN:

        //
        // Return was pressed.  If button w/ focus is default,
        // return its ID.  Otherwise, return id of original
        // defpushbutton.
        //
        code = (WORD)(DWORD)SendMessage((hwnd=GetFocus()),WM_GETDLGCODE,0,0L);

        if (code & DLGC_DEFPUSHBUTTON)
        {
            iOK = (DWORD)GetDlgCtrlID(hwnd);
            hwndNext = hwnd;
            goto HaveWindow;
        }
        else
        {
            lT = (LONG)SendMessage(hwndDlg, DM_GETDEFID, 0, 0L);

            // WIN31 only - if no defid and recursive, send it up.
            if (!(HIWORD(lT)))
            {
                code = (WORD)(DWORD)SendMessage(hwndDlg,WM_GETDLGCODE,0,0L);
                if (code & DLGC_RECURSE)
                {
                    SendMessage(GetParent(hwndDlg),lpmsg->message,lpmsg->wParam,lpmsg->lParam);
                    return TRUE;
                }
            }

            iOK = MAKELONG( (HIWORD(lT)==DC_HASDEFID ? LOWORD(lT) : IDOK), 0);
        }
        // FALL THRU

    case VK_ESCAPE:
    case VK_CANCEL:

        //
        // NOTE THAT THIS ONLY WORKS IF hwndNext IS AN IMMEDIATE
        // CHILD OF THE TOP-LEVEL DIALOG
        //
        // Make sure button is not disabled.
#ifdef WIN31
        hwndNext = GetDlgItem(hwndDlg, (int)iOK);
#else
        hwndNext = GetDlgItem(hwndDlg, iOK);
#endif

HaveWindow:
        if (hwndNext && !IsWindowEnabled(hwndNext))
        {
            MessageBeep(0);
            return TRUE;
        }

        if (!hwndNext)
        {
            // WIN31 only - if no control found and recursive, send it up.
            code = (WORD)(DWORD)SendMessage(hwndDlg,WM_GETDLGCODE,0,0L);
            if (code & DLGC_RECURSE)
            {
                SendMessage(GetParent(hwndDlg),lpmsg->message,lpmsg->wParam,lpmsg->lParam);
                return TRUE;
            }
        }

        // Note:  hwndDlg is toast after this call!
        // DONTREVALIDATE();    WIN31 can't call outside of USER
        // WIN31 using postmessage instead
        FORWARD_WM_COMMAND(hwndDlg,iOK,hwndNext,BN_CLICKED,PostMessage);
        return(TRUE);

    default:
        // Pass all other keys along
        return FALSE;
    }

    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  PrevGroupItem()
//
//  (1) If NO current item, return the last item in the dialog window
//  (2) If current item is NOT the start of a group, return the previous item
//  (3) Else current item IS the start of a group --
//      Walk forward through the dialog items until the same item or the 
//      start of a new group is encountered; return the item found JUST 
//      BEFORE that
//
// ----------------------------------------------------------------------------
HWND FAR PASCAL PrevGroupItem(HWND hwndDlg, HWND hwndCurrent)
{
    HWND  hwnd, hwndPrev;

    if (!hwndCurrent || !(GetWindowStyle(hwndCurrent) & WS_GROUP))
        return(PrevControl(hwndDlg, hwndCurrent, TRUE));

    hwndPrev = hwndCurrent;

    while (TRUE)
    {
        hwnd = NextControl(hwndDlg, hwndPrev, TRUE);

        if ((GetWindowStyle(hwnd) & WS_GROUP) || (hwnd == hwndCurrent))
	        return(hwndPrev);

        hwndPrev = hwnd;
    }
}



// ----------------------------------------------------------------------------
//
//  NextGroupItem()
//
//  (1) If NO current item, return the first item in the dialog window
//  (2) If item after current item is NOT the start of a group, return that 
//      item
//  (3) Else item after current item IS the start of a group --
//      Walk backward through the dialog items until the same item or the 
//      start of a new group is encountered; return the item found JUST BEFORE 
//      that.
//
// ----------------------------------------------------------------------------
HWND FAR PASCAL NextGroupItem(HWND hwndDlg, HWND hwndCurrent)
{
    HWND  hwnd, hwndNext;
  
    hwnd = NextControl(hwndDlg, hwndCurrent, TRUE);

    if (!hwndCurrent || !(GetWindowStyle(hwnd) & WS_GROUP))
        return(hwnd);

    hwndNext = hwndCurrent;

    while (!(GetWindowStyle(hwndNext) & WS_GROUP))
    {
        hwnd = PrevControl(hwndDlg, hwndNext, TRUE);

        if (hwnd == hwndCurrent)
            return(hwndNext);

        hwndNext = hwnd;
    }
    return(hwndNext);
}


// ----------------------------------------------------------------------------
//
//  GetNextDlgGroupItem()
//
// ----------------------------------------------------------------------------
HWND FAR PASCAL IGetNextDlgGroupItem(HWND hwndDlg, HWND hwnd, BOOL fPrev)
{
    HWND hwndCurrent;
    BOOL fOnceAround = FALSE;

    hwnd = hwndCurrent = GetChildControl(hwndDlg, hwnd);

    do
    {
        hwnd = (fPrev ? PrevGroupItem(hwndDlg, hwnd) : NextGroupItem(hwndDlg, hwnd));

        if (hwnd == hwndCurrent)
            fOnceAround = TRUE;

        if (!hwndCurrent)
            hwndCurrent = hwnd;
    }
    while (!fOnceAround && ((!IsWindowEnabled(hwnd) || !IsWindowVisible(hwnd))));

    return hwnd;
}


// ----------------------------------------------------------------------------
//
//  IGetNextDlgTabItem()
//
// ----------------------------------------------------------------------------
HWND FAR PASCAL IGetNextDlgTabItem(HWND hwndDlg, HWND hwnd, BOOL fPrev)
{
    HWND hwndSave;

    hwnd = GetChildControl(hwndDlg, hwnd);

    if (hwnd)
    {
        if (!IsChild(hwndDlg, hwnd))
            return(NULL);
    }

    //
    // BACKWARD COMPATIBILITY
    //
    // Note that the result when there are no tabstops of 
    // IGetNextDlgTabItem(hwndDlg, NULL, FALSE) was the last item, now
    // will be the first item.  We could put a check for fRecurse here
    // and do the old thing if not set.
    //

    // We are going to bug out if we hit the first child a second time.
    
    hwndSave = hwnd; 
    hwnd = (fPrev ? PrevControl(hwndDlg, hwnd, TRUE) : NextControl(hwndDlg, hwnd, TRUE));

    if(hwndDlg == hwnd)
        goto FoundIt;

    while (hwnd != hwndSave)
    {
        Assert(hwnd);

        if (!hwndSave)
            hwndSave = hwnd;

        if ((GetWindowStyle(hwnd) & (WS_TABSTOP | WS_VISIBLE | WS_DISABLED))
                == (WS_TABSTOP | WS_VISIBLE))
            // Found it.
            break;

        hwnd = (fPrev ? PrevControl(hwndDlg, hwnd, TRUE) : NextControl(hwndDlg, hwnd, TRUE));
    }

FoundIt:
    return hwnd;
}



// ----------------------------------------------------------------------------
//
//  DlgSetFocus()
//
// ----------------------------------------------------------------------------
void FAR PASCAL DlgSetFocus(HWND hwnd)
{
    if (((WORD)(DWORD)SendMessage(hwnd, WM_GETDLGCODE, 0, 0L)) & DLGC_HASSETSEL)
    {
        //
        // BOGUS
        //
        // Select all of the text in the edit control.  We use 0xFFFE to
        // avoid -1 problems.  Should be ok unless/until we extend the
        // text capacity of edit fields.
        //
        SendMessage(hwnd, EM_SETSEL, 0, MAKELPARAM(0, 0xFFFE));
    }

    SetFocus(hwnd);
}


// ----------------------------------------------------------------------------
//
//  CheckDefPushButton()
//
// ----------------------------------------------------------------------------

void FAR PASCAL CheckDefPushButton(HWND hwndRoot, HWND hwndOldFocus, HWND hwndNewFocus)
{
    WORD    wDlgCode = 0;
    WORD    styleT;
    LONG    lT;
    WORD    id;
    HWND    hwndT;

    if (hwndNewFocus)
    {
        wDlgCode = (WORD)(DWORD)SendMessage(hwndNewFocus, WM_GETDLGCODE, 0, 0L);
        
        // Do nothing if clicking on dialog background or recursive dialog
        // background.
        if (wDlgCode & DLGC_RECURSE)
            return;
    }

    if (hwndOldFocus == hwndNewFocus)
    {
        if (wDlgCode & DLGC_UNDEFPUSHBUTTON)
            SendMessage(hwndNewFocus, BM_SETSTYLE, (WPARAM)BS_DEFPUSHBUTTON, (LPARAM)(LONG)TRUE);
        return;
    }

    //
    // If the focus is changing to or from a pushbutton, then remove the 
    // default style from the current default button 
    //
    if ((hwndOldFocus &&
       ((WORD)(DWORD)SendMessage(hwndOldFocus, WM_GETDLGCODE, 0, 0L)
        & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON))) ||
       (hwndNewFocus && (wDlgCode & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON))))
    {
        RemoveDefaultButton(hwndRoot, hwndNewFocus);
    }

    // If moving to a button, make that button the default.
    if (wDlgCode & DLGC_UNDEFPUSHBUTTON)
        SendMessage(hwndNewFocus, BM_SETSTYLE, (WPARAM)BS_DEFPUSHBUTTON,
            (LPARAM)(LONG)TRUE);
    else
    {
        //
        // Otherwise, make sure the original default button is default
        //
        // BUG BUG BUG BUG BUG !!!!!!!  There is one case where two buttons
        // will appear to have the default style.  This is, if original def
        // button is disabled, tab through control to enable another button 
        // (the if default disabled case below).  Then enable the original 
        // default and tab between two non-pushbuttons.  This won't call 
        // RemoveDefaultButton above since they are both not buttons.  
        // This should be fixed sometime.  
        //

        // Get the original default button handle

        //
        // BOGUS
        // DWORD IDs don't work for this reason!  How will the Mouse 9.0
        // driver work with SDM32?  It has a feature that snaps the cursor
        // to the control with the default, and SDM32 needs DWORD IDs since
        // they are local pointers.
        //
        lT = (LONG)SendMessage(hwndRoot, DM_GETDEFID, 0, 0L);
        id = (HIWORD(lT) == DC_HASDEFID ? LOWORD(lT) : IDOK);

        hwndT = GetDlgItem(hwndRoot, id);
        if (!hwndT)
            return;

        // If it already has the default button style, do nothing.
        if ((styleT = (WORD)(DWORD)SendMessage(hwndT, WM_GETDLGCODE, 0, 0L))
            & DLGC_DEFPUSHBUTTON)
            return;

        // Also check to make sure it is really a button.
        if (!(styleT & DLGC_UNDEFPUSHBUTTON))
            return;

        if (IsWindowEnabled(hwndT))
            SendMessage(hwndT, BM_SETSTYLE, (WPARAM)BS_DEFPUSHBUTTON, (LPARAM)(LONG)TRUE);
   }
}

// ----------------------------------------------------------------------------
//
//  RemoveDefaultButton()
//
//  Scans through all the controls in the dialog box and removes the default
//  button style from any button that has it.  This is done since sometimes
//  we don't know who has the default.
//
// ----------------------------------------------------------------------------
void FAR PASCAL RemoveDefaultButton(HWND hwndRoot, HWND hwndStart)
{
    WORD  code;
    HWND hwnd;
    HWND hwndNext;

    if (!hwndStart)
        hwndStart = NextControl(hwndRoot, NULL, FALSE);
    else
        hwndStart = GetChildControl(hwndRoot, hwndStart);

    if (!hwndStart)
        return;

    //
    // Get first real leaf control.  Leaves are direct children of DLGC_
    // RECURSE windows.
    //
    hwnd = hwndStart;

    do
    {
        code = (WORD)(DWORD)SendMessage(hwnd, WM_GETDLGCODE, 0, 0L);
        if (code & DLGC_DEFPUSHBUTTON)
            SendMessage(hwnd, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, (LPARAM)(DWORD)TRUE);

        hwndNext = NextControl(hwndRoot, hwnd, FALSE);

        hwnd = hwndNext;
    }
    while (hwnd && (hwnd != hwndStart));
}


// ----------------------------------------------------------------------------
//
//  NextControl()
//
//  Finds the next valid control within the window tree specified by hwndRoot.
//  A valid control is a DLGC_RECURSE window, or a direct child thereof.
//
//  We traverse the tree by enumerating a DLGC_RECURSE window first, then its
//  first level children.
//
// ----------------------------------------------------------------------------
HWND FAR PASCAL NextControl(HWND hwndRoot, HWND hwndStart, BOOL fSkipInvDis)
{
    // if hwndStart is already equal to hwndRoot, this confuses this function
    // badly.
    Assert(hwndRoot != hwndStart);

    if (!hwndStart)
    {
FirstChild:
        hwndStart = GetWindow(hwndRoot,GW_CHILD);
        if (!hwndStart)
            return(hwndRoot);

        goto Found;
    }
    else
    {
        //
        // Are we at the last control within some parent?  If so, pop back up.
        //
        while (GetNextSibling(hwndStart) == NULL)
        {
            //
            // Popup to previous real ancestor.  hwndStart will be NULL, 
            // hwndRoot, or the child of a recursive dialog.
            //
            hwndStart = GetChildControl(hwndRoot, GetParent(hwndStart));
            if (!hwndStart || (hwndStart == hwndRoot))
            {
                goto FirstChild;
            }
        }

        Assert(hwndStart);
        hwndStart = GetNextSibling(hwndStart);
    }

Found:
    // Find first valid control within hwndStart.
    if (SendMessage(hwndStart, WM_GETDLGCODE, 0, 0L) & DLGC_RECURSE)
    {
        if (fSkipInvDis && (!IsWindowVisible(hwndStart) || !IsWindowEnabled(hwndStart)))
            hwndStart = NextControl(hwndRoot, hwndStart, fSkipInvDis);
        else
            hwndStart = NextControl(hwndStart, NULL, fSkipInvDis);

    }

    return(hwndStart);
}


// ----------------------------------------------------------------------------
//
//  PrevControl()
//
//  Finds the previous child within the ancestor window.  Returns NULL if
//  there is no descendant.  
//
//  This function is kind of lame.  We basically start at the beginning of
//  the leaves, and walk until the next leaf after the current one is 
//  hwndStart.
//
// ----------------------------------------------------------------------------
HWND FAR PASCAL PrevControl(HWND hwndRoot, HWND hwndStart, BOOL fSkipInvDis)
{
    HWND hwnd;
    HWND hwndNext;

    Assert(hwndRoot);
    // If hwndStart is already hwndRoot, this will confuse this function.
    Assert(hwndRoot != hwndStart);

    hwnd = NextControl(hwndRoot, NULL, fSkipInvDis);

    while (hwndNext = NextControl(hwndRoot, hwnd, fSkipInvDis))
    {
        if (hwndNext == hwndStart)
            break;

        hwnd = hwndNext;
    }

    return(hwnd);
}



// ----------------------------------------------------------------------------
//
//  GetChildControl()
//
//  Gets valid ancestor of given window.
//  A valid dialog control is a direct descendant of a DLGC_RECURSE control.
//
// ----------------------------------------------------------------------------
HWND FAR PASCAL GetChildControl(HWND hwndRoot, HWND hwndChild)
{
    HWND    hwndControl = NULL;
    
    while (hwndChild && (GetWindowStyle(hwndChild) & WS_CHILD) && (hwndChild != hwndRoot))
    {
        hwndControl = hwndChild;
        hwndChild = GetParent(hwndChild);

        if (SendMessage(hwndChild, WM_GETDLGCODE, 0, 0L) & DLGC_RECURSE)
            break;
    }

    return(hwndControl);
}


