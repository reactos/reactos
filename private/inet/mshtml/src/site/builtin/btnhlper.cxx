//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       btnhelp.cxx
//
//  Contents:   Button helper class implementation
//
//  Classes:    CButtonHelper
//
//  Functions:
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_BTNHLPER_HXX_
#define X_BTNHLPER_HXX_
#include "btnhlper.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif


// Used by CInput on TYPE=file to differentiate clicks on the text and button portions.
const DWORD CLKDATA_BUTTON = 1;


//+------------------------------------------------------------------------
//
//  Member:     CBtnHelper::BtnTakeCapture
//
//  Synopsis:   take capture for the button helper and remember it
//
//-------------------------------------------------------------------------

void
CBtnHelper::BtnTakeCapture(BOOL fTake)
{
    GetElement()->TakeCapture(fTake);
    _fButtonHasCapture = fTake;
}


//+------------------------------------------------------------------------
//
//  Member:     CBtnHelper::BtnHasCapture
//
//  Synopsis:   check if the button helper really has capture
//
//-------------------------------------------------------------------------

BOOL
CBtnHelper::BtnHasCapture()
{
    return GetElement()->HasCapture() && _fButtonHasCapture;
}


//+---------------------------------------------------------------------------
//
// Member:      CBtnHelper::BtnHandleMessage
//
// Synopsis:    Handle window message
//
//----------------------------------------------------------------------------

HRESULT
CBtnHelper::BtnHandleMessage(CMessage * pMessage)
{
    HRESULT         hr      = S_FALSE;
    CElement *      pElem   = GetElement();
    BOOL            fCtrl   = pMessage->dwKeyState & FCONTROL;

    switch (pMessage->message)
    {
    case WM_KILLFOCUS:
        BTN_RESETSTATUS(_wBtnStatus);
        ChangePressedLook();
        hr = S_FALSE;
        break;
    case WM_SETFOCUS:
        _wBtnStatus = BTN_SETSTATUS(_wBtnStatus, FLAG_HASFOCUS);
        ChangePressedLook();
        hr = S_FALSE; //THR(SiteHandleMessage(pMessage, pChild));
        break;

    case WM_SETCURSOR:
        pElem->SetCursorStyle(IDC_ARROW);
        hr = S_OK;
        break;
    case WM_KEYDOWN:
        switch (pMessage->wParam)
        {
        case 'M':
        case 'm':
            if (!fCtrl)
            {
                break;
            }
            // fall thru
        case VK_TAB:
            // change the look if this is not done yet.
            BTN_RESETSTATUS(_wBtnStatus);
            ChangePressedLook();
            hr = S_FALSE;
            break;

        case VK_BACK:
            // do not allow navigate back
            hr = S_OK;
            break;

        case VK_SPACE:
            // do not change look&feel if we already have the capture
            if (!BtnHasCapture())
            {
                PressButton(PRESSED_KEYBOARD);
                BtnTakeCapture(TRUE);
                hr = S_OK;
            }
             break;
        }
        break;

    case WM_CHAR:
        switch (pMessage->wParam)
        {
        case VK_SPACE:
            hr = S_OK;
        }
        break;

    case WM_KEYUP:
    {
        if (BtnHasCapture() && BTN_GETSTATUS(_wBtnStatus, PRESSED_KEYBOARD))
        {
            if (!BTN_GETSTATUS(_wBtnStatus, PRESSED_MOUSE))
            {
                WORD    wOld = _wBtnStatus;

                _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, PRESSED_KEYBOARD);  // reset PRESSED_KEY bit
                BtnTakeCapture(FALSE);
                _wBtnStatus = wOld;
            }
            else
            {
            // the user still presses on the mouse
            // we should change the press look
                // reverse the bit
                _wBtnStatus = BTN_REVSTATUS(_wBtnStatus, PRESSED_MOUSE);
                if (PressedLooksDifferent(PRESSED_MOUSE))
                {
                    ChangePressedLook();
                }
            }
            ReleaseButton(PRESSED_KEYBOARD, pMessage);
        }

        hr = S_OK;
        break;
    }
    case WM_MOUSEMOVE:

        // If the mouse is captured, that means that the user clicked
        // over the control. Else, it is moving over us, but we were not pressed.

        if (BtnHasCapture() && !BTN_GETSTATUS(_wBtnStatus, PRESSED_KEYBOARD))
        {
            // See whether the mouse is still over us. If it goes from
            // being over to not, or visa versa, flip the _wBtnStatus flag
            // and, if that would affect visible image, redraw.

            if (((_wBtnStatus & PRESSED_MOUSE) != 0) !=
                  MouseIsOver(MAKEPOINTS(pMessage->lParam).x,
                              MAKEPOINTS(pMessage->lParam).y))
            {
                // reverse the bit
                _wBtnStatus = BTN_REVSTATUS(_wBtnStatus, PRESSED_MOUSE);
                if (PressedLooksDifferent(PRESSED_MOUSE))
                {
                    ChangePressedLook();
                }
            }
            if (!(pMessage->wParam & BTN_GETPRESSSTATUS(_wBtnStatus)))
            {
                BtnTakeCapture(FALSE);
            }
            hr = S_OK;
        }

        break;
    case WM_CAPTURECHANGED:
        // If the mouse is over the control at the time we lose capture,
        // then we need to remove the pressed state, just as though the
        // mouse had moved off the control.
#if DBG==1
        TLS(fHandleCaptureChanged) = TRUE;
#endif //DBG==1

        _fButtonHasCapture = FALSE;

        if (BTN_GETSTATUS(_wBtnStatus, PRESSED_MOUSE))
        {
            // if it is a mouse capture
            // reset the mouse pressed bit
            _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, PRESSED_MOUSE);

            // If the pressed state looks different from the unpressed
            // state, redraw.

            if (PressedLooksDifferent(PRESSED_MOUSE))
            {
                ChangePressedLook();
            }
            hr = S_OK;
        }
        else if (BTN_GETSTATUS(_wBtnStatus, PRESSED_KEYBOARD))
        {
            // if it is a keyboard capture
            // reset the space pressed bit
            _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, PRESSED_KEYBOARD);

            // If the pressed state looks different from the unpressed
            // state, redraw.

            if (PressedLooksDifferent(PRESSED_KEYBOARD))
            {
                ChangePressedLook();
            }
            hr = S_OK;
        }
#if DBG==1
        TLS(fHandleCaptureChanged) = FALSE;
#endif //DBG==1
        break;

    case WM_LBUTTONDBLCLK:
        _wBtnStatus = BTN_SETSTATUS(_wBtnStatus, FLAG_DOUBLECLICK);
        // fall through
    case WM_LBUTTONDOWN:

        if (pElem->HasCurrency())
        {
            BTN_PRESSLEFT(_wBtnStatus);
            PressButton(PRESSED_MOUSE);
            BtnTakeCapture(TRUE);
            hr = S_OK;
        }
        break;

    case WM_LBUTTONUP:
    {
        if (BtnHasCapture())
        {
            // if space key still down, we don't do anything
            if (!BTN_GETSTATUS(_wBtnStatus, PRESSED_KEYBOARD))
            {
                WORD    wOld = _wBtnStatus;

                // temporarily reset mouse pressed bit to clear the capture
                _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, PRESSED_MOUSE);
                BtnTakeCapture(FALSE);
                _wBtnStatus = wOld;
            }
            ReleaseButton(PRESSED_MOUSE, pMessage);
            hr = S_OK;
        }
        break;
    }
#ifdef NEVER
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        if (HasCapture())
        {
            // clear bit
            _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, PRESSED_MOUSE);
            TakeCapture(FALSE);
        }
#endif

    case WM_CONTEXTMENU:
        hr = THR(pElem->OnContextMenu(
                (short)LOWORD(pMessage->lParam),
                (short)HIWORD(pMessage->lParam),
                CONTEXT_MENU_CONTROL));
        hr = S_OK;
        break;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CBtnHelper::PressButton
//
//  Synopsis:   Enter the pressed state.
//
//  Arguments:
//
//  Notes:      PressButton and ReleaseButton keep track of which device has
//              pressed and released, so that we only release the button when
//              all devices have released. This matches VB behavior.
//                We also avoid redundant presses in case of repeated key down
//              without intervening key up.
//
//----------------------------------------------------------------------------

void
CBtnHelper::PressButton(WORD wWhoPressed)
{
    if (!BTN_PRESSED(_wBtnStatus))
    {
        _wBtnStatus = BTN_SETSTATUS(_wBtnStatus, wWhoPressed);

        // A button that is already down does not change its image on being pressed, only on
        // being released. Other states do change on being pressed.

        if (PressedLooksDifferent(wWhoPressed))
        {
            ChangePressedLook();
        }
    }
    else
    {
        _wBtnStatus = BTN_SETSTATUS(_wBtnStatus, wWhoPressed);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBtnHelper::ReleaseButton
//
//  Synopsis:   Exit the pressed state.
//
//  Arguments:  fVisual TRUE if we should show visual effects.
//
//  Note:       Checks that the button was actually pressed by the same agency
//              as is trying to release it. This is to cover any possible
//              case in which we get a button or key up without a corresponding
//              down (or get two ups in a row).
//

//----------------------------------------------------------------------------

void
CBtnHelper::ReleaseButton(WORD wWhoPressed, CMessage * pMessage)
{
    if (BTN_GETSTATUS(_wBtnStatus, wWhoPressed))
    {
        // mouse/spcae key pressed and released, clear the bit
        _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, wWhoPressed);

        if (!BTN_PRESSED(_wBtnStatus))                          // If no remaining presses...
        {
            // If this was a command button, there is no value and therefore
            // the call before would not invalidate and Fire OnViewChange
            // Make sure we do it if the pressed state looked different than
            // the released stated.
            if (PressedLooksDifferent(wWhoPressed))
            {
                ChangePressedLook();
            }
            if (pMessage)
            {
                pMessage->SetNodeClk(GetElement()->GetFirstBranch());
                pMessage->dwClkData = CLKDATA_BUTTON;
            }
        }
    }
    // reset double click flag
    _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, FLAG_DOUBLECLICK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBtnHelper::MouseIsOver
//
//  Synopsis:   TRUE iff this point is over us.
//
//  Arguments:
//
//----------------------------------------------------------------------------

BOOL
CBtnHelper::MouseIsOver(LONG x, LONG y)
{
    CRect rc;
    POINT pt = { x, y };

    GetElement()->GetCurLayout()->GetRect(&rc, COORDSYS_GLOBAL);
    return rc.Contains(pt);
}

//+---------------------------------------------------------------------------
//
//  Member:     PressedLooksDifferent
//
//  Synopsis:   Returns true if the pressed value looks different from the
//              unpressed.
//
//              An up (FALSE) button changes its appearance when pressed,
//              a down (TRUE) button changes when released.
//
//              Some causes of a button being momentarily depressed do not
//              have visual effect. See the notes on SimulateClick for a
//              discussion. If the button does not hold a value, then we check
//              to see whether the source of the button press causes a
//              redraw. After all, since the button does not hold a value,
//              any visual display on press will be reversed on release,
//              bringing us back where we started. So, for buttons that do not
//              hold a value, we permit optimizing by eliminating both draws.
//
//  Arguments:  [wWoPressed] -- bit flags of who has pressed the control.
//
//----------------------------------------------------------------------------

BOOL
CBtnHelper::PressedLooksDifferent(WORD wWho)
{
    // Verify that no extra bits are in the wWho.

    Assert((wWho & ~(PRESSED_ASSIGNVALUE | PRESSED_MNEMONIC | PRESSED_MOUSE | PRESSED_KEYBOARD)) == 0);

    // All sources of clicks cause visible change except PRESSED_ASSIGNVALUE and PRESSED_MNEMONIC
    // for which we don't show the "pressed" state

    return (wWho & (PRESSED_KEYBOARD | PRESSED_MOUSE));
}


#ifdef NEVER
//[TRISTATE]
OLE_TRISTATE
CBtnHelper::BtnValue(void)
{
    return TRISTATE_FALSE;
}

//[TRISTATE]
void
CBtnHelper::BtnSetValue(OLE_TRISTATE triValue)
{
    IGNORE_HR(FireStdControlEvent_Click());
    // Do nothing else.  we don't store a value
}

//[TRISTATE]
OLE_TRISTATE
CBtnHelper::NextValue(void)
{
    if (BtnStyle() == GLYPHSTYLE_OPTION)
    {
        // A click on an Option button always sets to true.
        return TRISTATE_TRUE;
    }
    else if (BtnIsTripleState())
    {
        switch (BtnValue())
        {
        case TRISTATE_MIXED:
            return TRISTATE_TRUE;

        case TRISTATE_TRUE:
            return TRISTATE_FALSE;

        case TRISTATE_FALSE:
            return TRISTATE_MIXED;

        default:
            Assert(0 && "Invalid button value");
            return TRISTATE_FALSE;      // to keep the compiler happy
        }
    }
    else
    {
        // NOTE: if the current value is TRISTATE_MIXED, this returns TRISTATE_TRUE,
        //       as per Word and other apps.  From then on it goes to TRISTATE_FALSE,
        //       to TRISTATE_TRUE and back.

        return (BtnValue() == TRISTATE_TRUE) ? TRISTATE_FALSE : TRISTATE_TRUE;
    }
}

//[TRISTATE]
HRESULT
CBtnHelper::BtnValueChanged(OLE_TRISTATE triValue)
{
    // In my testing, VB4 only fires a click event or otherwise responds to a
    // value assignment if it is in fact different from the current value. This
    // is true of both command and option buttons. Neither VB4 nor Access show
    // visual feedback on value assignment.

    if (triValue != TRISTATE_FALSE)
    {
        if (_pDoc->_fDesignMode)
        {
            SimulateClick(PRESSED_ASSIGNVALUE); // All effects except visual.
        }
        else
        {
            BtnSetValue(triValue);
        }
    }

    return S_OK;
}

//[TRISTATE]
BOOL
CBtnHelper::Pressed()
{
    return BtnValue() == TRISTATE_TRUE ? TRUE : (BTN_PRESSED(_wBtnStatus) ? TRUE : FALSE);
}
#endif

void
CBtnHelper::ChangePressedLook()
{
    const SIZE      s_sizeDepressed = { 1, 1 };

    Assert(GetElement());
    Assert(GetElement()->GetCurLayout());

    CLayout *   pLayout   = GetElement()->GetCurLayout();

    if (!pLayout)
        return;

    if (!pLayout->TestLayoutDescFlag(LAYOUTDESC_NOTALTERINSET))
    {
        CDispNode * pDispNode = pLayout->GetElementDispNode();

        if (    pDispNode
            &&  pDispNode->HasInset())
        {
            const CSize &   sizeOldInset = pDispNode->GetInset();
            CSize           sizeNewInset = _sizeInset + (BTN_PRESSED(_wBtnStatus)
                                                                ? (const CSize &)s_sizeDepressed
                                                                : (const CSize &)g_Zero.size);

            if (sizeNewInset != sizeOldInset)
            {
                if (pLayout->OpenView())
                {
                    pDispNode->SetInset(sizeNewInset);
                }
            }
        }
    }
    pLayout->Invalidate();
}
