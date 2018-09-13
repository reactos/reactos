/***********************************************************************
 *
 *  FORMWND.CPP
 *
 *
 *  Copyright 1986-1996 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

#include "padhead.hxx"

#ifndef X_MSG_HXX_
#define X_MSG_HXX_
#include "msg.hxx"
#endif

#ifndef X_MSGTRIPL_HXX_
#define X_MSGTRIPL_HXX_
#include "msgtripl.hxx"
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#include "commctrl.h"
#endif

#define  WM_DEFERCOMBOUPDATE        (WM_APP + 1)
const int cxMargin = 4;
const int cyMargin = 4;

LRESULT DlgChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC pWndProc;
    LRESULT lResult;

    pWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_USERDATA);

    switch ( msg )
    {
        case WM_LBUTTONUP:
        {
            CPadMessage * pPad;
            pPad = (CPadMessage *) GetWindowLong(GetParent(GetParent(hwnd)), GWL_USERDATA);

            // Hack: Need to do the default behavior before deactivating the pad so that the button
            // other the button has shifted underneath the mouse by the time the default processing
            // for WM_LBUTTONUP happens and it does not fire the WM_COMMAND
            lResult = CallWindowProc(pWndProc, hwnd, msg, wParam, lParam);

            pPad->OnDialogControlActivate(hwnd);

            return lResult;
        }
    }

    return CallWindowProc(pWndProc, hwnd, msg, wParam, lParam);
}


void
CPadMessage::HookControl(ULONG ulIdControl)
{
    WNDPROC pWndProc;
    HWND hwndControl;

    hwndControl = GetDlgItem(_hwndDialog, ulIdControl);
    pWndProc = (WNDPROC)SetWindowLong(hwndControl, GWL_WNDPROC, (LONG)DlgChildWndProc);
    SetWindowLong(hwndControl, GWL_USERDATA, (LONG)pWndProc);
}


void
CPadMessage::OnDialogControlActivate(HWND hwndControl)
{
    CPadDoc * pPad1;

    // Make sure the command has not deleted pPad
    for(pPad1 = g_pDocFirst; pPad1 != NULL; pPad1 = pPad1->_pDocNext)
    {
        if(pPad1 == this)
            break;
    }

    // If not, deactivate Trident DocObject
    if(pPad1)
    {
        Assert(pPad1 == this);
        IGNORE_HR(UIDeactivateDoc());
    }

    // BUGBUG: chrisf - hack: apparently the focus goes away from the control
    // during UIdeactivateDoc
    SetFocus(hwndControl);
}


HWND
CPadMessage::CreateField(
    ULONG ulIdField,
    ULONG ulIdLabel,
    DWORD dwStyle,
    BOOL fCreateOleCallBack)
{
    RECT rc;
    HWND hwndField;
    DWORD dwEventMask;
    HFONT hFont;
    CTripCall * pTripCall = NULL;

    hwndField = GetDlgItem(_hwndDialog, ulIdLabel);
    GetWindowRect(hwndField, &rc);

    hwndField = CreateWindowEx(
            WS_EX_CLIENTEDGE,	// extended window style
            TEXT("RichEdit"),	// pointer to registered class name
            LPCTSTR (NULL),	    // pointer to window name
            dwStyle |
            WS_CHILD | WS_VISIBLE | ES_MULTILINE |
            ES_SUNKEN | ES_SAVESEL | WS_TABSTOP,	// window style
            rc.right - _rcDialog.left + cxMargin,
            rc.top - _rcDialog.top,
            _rcDialog.right - rc.right - cxMargin * 2,	
            rc.bottom - rc.top,
            _hwndDialog,    // handle to parent or owner window
            NULL,	        // handle to menu, or child-window identifier
            g_hInstCore,	// handle to application instance
            NULL         	// pointer to window-creation data
    );

    if (!hwndField)
        goto Cleanup;

    SetWindowLong(hwndField, GWL_ID, ulIdField);

    dwEventMask = SendMessage(hwndField, EM_GETEVENTMASK, 0, 0);
    SendMessage(hwndField, EM_SETEVENTMASK, 0,
                    dwEventMask | ENM_REQUESTRESIZE | ENM_MOUSEEVENTS);

    hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hwndField, WM_SETFONT, (WPARAM)hFont, 0);

    SendMessage(hwndField, EM_REQUESTRESIZE, 0, 0);

    if (dwStyle & ES_READONLY)
        SendMessage(hwndField, EM_SETBKGNDCOLOR, FALSE, GetSysColor(COLOR_BTNFACE));

    if (fCreateOleCallBack)
    {
        pTripCall = new CTripCall(this, hwndField, TRUE);
        if (!pTripCall)
        {
            hwndField = NULL;
            goto Cleanup;
        }

        Verify(SendMessage(hwndField, EM_SETOLECALLBACK, 0, (LPARAM)pTripCall));
    }

Cleanup:
    if (pTripCall)
        pTripCall->Release();

    return hwndField;
}

HDWP
CPadMessage::FieldRequestResize (
    HDWP    hdwp,
    RECT *  prcResize,
    ULONG   ulIdResize,
    ULONG   ulIdField,
    ULONG   ulIdLabel,
    RECT *  prcSubmitBtn,
    int *   pcyDialogHeight)
{
    HWND    hwndField;
    HWND    hwndLabel;
    RECT    rc;
    int     cyHeight;

    hwndField = GetDlgItem(_hwndDialog, ulIdField);
    if (!hwndField)
        return hdwp;

    GetWindowRect(hwndField, &rc);

    cyHeight = (ulIdResize == ulIdField) ?
                prcResize->bottom - prcResize->top + 1:
                rc.bottom - rc.top;

    if (*pcyDialogHeight == 0)
        *pcyDialogHeight = rc.top - _rcDialog.top;

    if (rc.top - _rcDialog.top >= prcResize->top)
    {
         hdwp = DeferWindowPos(hdwp, hwndField, NULL,
                rc.left - _rcDialog.left,
                *pcyDialogHeight,
                _rcDialog.right - rc.left -
                    (prcSubmitBtn &&
                        (*pcyDialogHeight + _rcDialog.top <= prcSubmitBtn->bottom) ?
                            prcSubmitBtn->right - prcSubmitBtn->left + cxMargin * 2 :
                            cxMargin),
                cyHeight,
                SWP_NOACTIVATE | SWP_NOZORDER);
          if(!hdwp)
                return NULL;

         // reposition Field Button
         hwndLabel = GetDlgItem(_hwndDialog, ulIdLabel);
         if (hwndLabel)
         {
             GetWindowRect(hwndLabel, &rc);
             hdwp = DeferWindowPos(hdwp, hwndLabel, NULL,
                          rc.left - _rcDialog.left,
                          *pcyDialogHeight,
                          0, 0,
                          SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
             if(!hdwp)
                 return NULL;
         }
     }

     *pcyDialogHeight += cyHeight + cyMargin;

     return hdwp;
}


//+---------------------------------------------------------------------------
//
//  Member:     FormDlgProcSend
//              CPadMessage::DlgProcSend
//
//  Synopsis:   Dialog procedure for field dialogs, Send form
//
//----------------------------------------------------------------------------

BOOL CALLBACK FormDlgProcSend(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CPadMessage *       pPad;

    pPad = (CPadMessage *) GetWindowLong(GetParent(hwnd), GWL_USERDATA);

    Assert(pPad);

    return pPad->DlgProcSend(hwnd, msg, wParam, lParam);
}

BOOL
CPadMessage::DlgProcSend(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT                rc;
    int                 wmId;
    int                 wmEvent;
    HBITMAP             hBitmap;
    HWND                hButton;

    switch ( msg )
    {
    case WM_INITDIALOG:
    {
        _hwndDialog = hwnd;

        GetWindowRect(hwnd, &_rcDialog);

        // Load bitmap in "Send" button
        hBitmap = LoadBitmap(g_hInstResource, MAKEINTRESOURCE(IDB_MESSAGE_SEND));
        hButton = GetDlgItem(hwnd, ID_SUBMIT);
        SendMessage(hButton, BM_SETIMAGE,0, (LONG) hBitmap);

        // Create TO, CC and SUBJECT fields
        _rghwndEdit[0] = NULL;
        _rgulRecipTypes[0] = MAPI_ORIG;

        _rghwndEdit[1] = CreateField(ID_TO, ID_TO_BUTTON, 0, TRUE);
        _rgulRecipTypes[1] = MAPI_TO;

        _rghwndEdit[2] = CreateField(ID_CC, ID_CC_BUTTON, 0, TRUE);
        _rgulRecipTypes[2] = MAPI_CC;

        _cRecipTypes = 3;

        CreateField(ID_SUBJECT, ID_SUBJECT_LABEL, 0, FALSE);

        // Hook TO and CC buttons to trap mouse up
        HookControl(ID_TO_BUTTON);
        HookControl(ID_CC_BUTTON);

        return TRUE;
    }
    case WM_WINDOWPOSCHANGED:
        {
            HWND        hwndItem;
            HWND        hwndIcon;
            RECT        rc;
            RECT        rcSubmitBtn;
            int         cxSubmitBtn;
            int         xSubmitBtn;
            int         cxDialogOld;
            int         cyDialogOld;

            Assert(_hwndDialog == hwnd);

            cxDialogOld = _rcDialog.right - _rcDialog.left;
            cyDialogOld = _rcDialog.bottom - _rcDialog.top;

            GetWindowRect(hwnd, &_rcDialog);

            if (cxDialogOld != _rcDialog.right - _rcDialog.left)
            {
                hwndIcon = GetDlgItem(hwnd, ID_SUBMIT);
                GetWindowRect(hwndIcon, &rcSubmitBtn);
                ScreenToClient(hwnd, (POINT*)&rcSubmitBtn.left);
                ScreenToClient(hwnd, (POINT*)&rcSubmitBtn.right);
                cxSubmitBtn = rcSubmitBtn.right - rcSubmitBtn.left;

                HDWP hdwp = BeginDeferWindowPos(4);
                if(NULL == hdwp)
                    break;

                hwndItem = GetDlgItem(hwnd, ID_TO);
                if (hwndItem)
                {
                    GetWindowRect(hwndItem, &rc);
                    if(NULL == DeferWindowPos(hdwp, hwndItem, NULL, 0, 0, _rcDialog.right - rc.left - cxSubmitBtn - cxMargin * 2,
                                 rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE |
                                 SWP_NOZORDER))
                    {
                        break;
                    }
                }

                hwndItem = GetDlgItem(hwnd, ID_CC);
                if (hwndItem)
                {
                    GetWindowRect(hwndItem, &rc);
                    if(NULL == DeferWindowPos(hdwp, hwndItem, NULL, 0, 0, _rcDialog.right - rc.left - cxSubmitBtn - cxMargin * 2,
                                 rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE |
                                 SWP_NOZORDER))
                    {
                        break;
                    }
                }

                hwndItem = GetDlgItem(hwnd, ID_SUBJECT);
                if (hwndItem)
                {
                    GetWindowRect(hwndItem, &rc);
                    if(NULL == DeferWindowPos(hdwp, hwndItem, NULL, 0, 0, _rcDialog.right - rc.left - cxMargin,
                                 rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE |
                                 SWP_NOZORDER))
                    {
                        break;
                    }
                }

                xSubmitBtn = _rcDialog.right - _rcDialog.left - cxSubmitBtn - cxMargin;
                if(NULL == DeferWindowPos(hdwp, hwndIcon, NULL, xSubmitBtn, rcSubmitBtn.top,
                            rcSubmitBtn.right - rcSubmitBtn.left, rcSubmitBtn.bottom - rcSubmitBtn.top,
                            SWP_NOACTIVATE | SWP_NOZORDER))
                {
                    break;
                }

                EndDeferWindowPos(hdwp);
            }

            if (cyDialogOld != _rcDialog.bottom - _rcDialog.top)
            {
                Resize();
            }
        }
        break;

    case WM_NOTIFY:
        wmId = wParam;
        wmEvent = ((NMHDR*)lParam)->code;

        Assert(_hwndDialog == hwnd);

        switch(wmEvent)
        {
        case EN_MSGFILTER:
            if (((MSGFILTER*)lParam)->msg == WM_LBUTTONUP)
            {
                OnDialogControlActivate(((MSGFILTER*)lParam)->nmhdr.hwndFrom);
            }
            break;

        case EN_REQUESTRESIZE:
            {
            int             cyDialogHeight = 0;
            HWND            hwndItem;
            REQRESIZE *     lpResize;
            HWND            hwndIcon;
            HDWP            hdwp;
            RECT            rcSubmitBtn;

            // Get new height for item requesting size change
            lpResize = (REQRESIZE *)(lParam);

            // Break out if no change in size
            hwndItem = GetDlgItem(hwnd, wmId);
            GetWindowRect(hwndItem, &rc);
            if(lpResize->rc.bottom - lpResize->rc.top + 1 == rc.bottom - rc.top)
                break;

            // Get Submit button width
            hwndIcon = GetDlgItem(hwnd, ID_SUBMIT);
            GetWindowRect(hwndIcon, &rcSubmitBtn);

            hdwp = BeginDeferWindowPos(6);
            if(!hdwp)
                break;

            // Reposition TO field
            hdwp =  FieldRequestResize (hdwp, &lpResize->rc, wmId, ID_TO, ID_TO_BUTTON,
                                    &rcSubmitBtn, &cyDialogHeight);

            if(!hdwp)
                break;

            // Reposition CC field
            hdwp =  FieldRequestResize (hdwp, &lpResize->rc, wmId, ID_CC, ID_CC_BUTTON,
                                    &rcSubmitBtn, &cyDialogHeight);
            if(!hdwp)
                break;

            // Reposition Subject field
            hdwp =  FieldRequestResize (hdwp, &lpResize->rc, wmId, ID_SUBJECT, ID_SUBJECT_LABEL,
                                    &rcSubmitBtn, &cyDialogHeight);
            if(!hdwp)
                break;

            EndDeferWindowPos(hdwp);

            // Resize dialog as a whole
            SetWindowPos(hwnd, NULL, 0, 0, _rcDialog.right - _rcDialog.left,
                         cyDialogHeight, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
            }
        }
        break;

    case WM_COMMAND:
        wmId = GET_WM_COMMAND_ID(wParam, lParam);
        wmEvent = GET_WM_COMMAND_CMD(wParam, lParam);

        switch(wmId) {
        case ID_TO_BUTTON:
        case ID_CC_BUTTON:
            switch (wmEvent) {
            case BN_CLICKED:
                Address(wmId);
                break;

            default:
                return FALSE;
            }
            break;
        case ID_SUBJECT:
            if (wmEvent == EN_KILLFOCUS) {
                TCHAR    sz[250];

                if (Edit_GetText(GET_WM_COMMAND_HWND(wParam, lParam), sz, 200)) {
                    lstrcat(sz, TEXT (" - "));
                    lstrcat(sz, g_achWindowCaption);
                    SetWindowText(GetParent(hwnd), sz);
                }
                else
                    SetWindowText(GetParent(hwnd), g_achWindowCaption);

                break;
            }
            return FALSE;
        case ID_SUBMIT:
            if(wmEvent == BN_CLICKED)
            {
                SendMessage(GetParent(hwnd), WM_COMMAND, IDM_MESSAGE_SUBMIT, 0);
            }
            return TRUE;
        }
        return FALSE;

    }
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     FormDlgProcRead
//              CPadMessage::DlgProcRead
//
//  Synopsis:   Dialog procedure for field dialogs, Read form
//
//----------------------------------------------------------------------------

BOOL CALLBACK FormDlgProcRead(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CPadMessage *       pPad;

    pPad = (CPadMessage *) GetWindowLong(GetParent(hwnd), GWL_USERDATA);

    Assert(pPad);

    return pPad->DlgProcRead(hwnd, msg, wParam, lParam);
}

BOOL
CPadMessage::DlgProcRead(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT                rc;
    int                 wmId;
    int                 wmEvent;

    switch ( msg ) {
    case WM_INITDIALOG:
    {
        _hwndDialog = hwnd;

        GetWindowRect(hwnd, &_rcDialog);

        // Create TO, CC and SUBJECT fields
        CreateField(ID_FROM, ID_FROM_LABEL, ES_READONLY, FALSE);
        CreateField(ID_SENT, ID_SENT_LABEL, ES_READONLY, FALSE);
        CreateField(ID_TO, ID_TO_LABEL, ES_READONLY, FALSE);
        CreateField(ID_CC, ID_CC_LABEL, ES_READONLY, FALSE);
        CreateField(ID_SUBJECT, ID_SUBJECT_LABEL, ES_READONLY, FALSE);

        return TRUE;
    }

    case WM_NOTIFY:
        wmId = wParam;
        wmEvent = ((NMHDR*)lParam)->code;

        Assert(_hwndDialog == hwnd);

        switch(wmEvent)
        {
        case EN_MSGFILTER:
            if (((MSGFILTER*)lParam)->msg == WM_LBUTTONUP)
            {
                OnDialogControlActivate(((MSGFILTER*)lParam)->nmhdr.hwndFrom);
            }
            break;

        case EN_REQUESTRESIZE:
            {
            int             cyDialogHeight = 0;
            HWND            hwndItem;
            REQRESIZE *     lpResize;
            HDWP            hdwp;

            // Get new height for item requesting size change
            lpResize = (REQRESIZE *)(lParam);

            // Break out if no change in size
            hwndItem = GetDlgItem(hwnd, wmId);
            GetWindowRect(hwndItem, &rc);
            if(lpResize->rc.bottom - lpResize->rc.top + 1 == rc.bottom - rc.top)
                break;

            hdwp = BeginDeferWindowPos(10);
            if(!hdwp)
                break;

            // Reposition FROM field
            hdwp =  FieldRequestResize (hdwp, &lpResize->rc, wmId,
                        ID_FROM, ID_FROM_LABEL, NULL, &cyDialogHeight);

            if(!hdwp)
                break;

            // Reposition SENT field
            hdwp =  FieldRequestResize (hdwp, &lpResize->rc, wmId,
                        ID_SENT, ID_SENT_LABEL, NULL, &cyDialogHeight);

            if(!hdwp)
                break;

            // Reposition TO field
            hdwp =  FieldRequestResize (hdwp, &lpResize->rc, wmId,
                        ID_TO, ID_TO_LABEL, NULL, &cyDialogHeight);

            if(!hdwp)
                break;

            // Reposition CC field
            hdwp =  FieldRequestResize (hdwp, &lpResize->rc, wmId,
                        ID_CC, ID_CC_LABEL, NULL, &cyDialogHeight);

            if(!hdwp)
                break;

            // Reposition SUBJECT field
            hdwp =  FieldRequestResize (hdwp, &lpResize->rc, wmId,
                        ID_SUBJECT, ID_SUBJECT_LABEL, NULL, &cyDialogHeight);

            if(!hdwp)
                break;

            EndDeferWindowPos(hdwp);

            // Resize dialog as a whole
            SetWindowPos(hwnd, NULL, 0, 0, _rcDialog.right - _rcDialog.left,
                         cyDialogHeight, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
            }
        }
        break;


    case WM_WINDOWPOSCHANGED:
        {
            HWND        hwndItem;
            RECT        rc;
            int         cxDialogOld;
            int         cyDialogOld;

            Assert(_hwndDialog == hwnd);

            cxDialogOld = _rcDialog.right - _rcDialog.left;
            cyDialogOld = _rcDialog.bottom - _rcDialog.top;

            GetWindowRect(hwnd, &_rcDialog);

            if (cxDialogOld != _rcDialog.right - _rcDialog.left)
            {
                HDWP hdwp = BeginDeferWindowPos(5);
                if(NULL == hdwp)
                    break;

                hwndItem = GetDlgItem(hwnd, ID_FROM);
                if (hwndItem)
                {
                    GetWindowRect(hwndItem, &rc);
                    if(NULL == DeferWindowPos(hdwp, hwndItem, NULL, 0, 0, _rcDialog.right - rc.left - cxMargin,
                                 rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE |
                                 SWP_NOZORDER))
                    {
                        break;
                    }
                }

                hwndItem = GetDlgItem(hwnd, ID_SENT);
                if (hwndItem)
                {
                    GetWindowRect(hwndItem, &rc);
                    if(NULL == DeferWindowPos(hdwp, hwndItem, NULL, 0, 0, _rcDialog.right - rc.left - cxMargin,
                                 rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE |
                                 SWP_NOZORDER))
                    {
                        break;
                    }
                }

                hwndItem = GetDlgItem(hwnd, ID_TO);
                if (hwndItem)
                {
                    GetWindowRect(hwndItem, &rc);
                    if(NULL == DeferWindowPos(hdwp, hwndItem, NULL, 0, 0, _rcDialog.right - rc.left - cxMargin,
                                 rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE |
                                 SWP_NOZORDER))
                    {
                        break;
                    }
                }

                hwndItem = GetDlgItem(hwnd, ID_CC);
                if (hwndItem)
                {
                    GetWindowRect(hwndItem, &rc);
                    if(NULL == DeferWindowPos(hdwp, hwndItem, NULL, 0, 0, _rcDialog.right - rc.left - cxMargin,
                                 rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE |
                                 SWP_NOZORDER))
                    {
                        break;
                    }
                }

                hwndItem = GetDlgItem(hwnd, ID_SUBJECT);
                if (hwndItem)
                {
                    GetWindowRect(hwndItem, &rc);
                    if(NULL == DeferWindowPos(hdwp, hwndItem, NULL, 0, 0, _rcDialog.right - rc.left - cxMargin,
                                 rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE |
                                 SWP_NOZORDER))
                    {
                        break;
                    }
                }

                EndDeferWindowPos(hdwp);
        }

        if (cyDialogOld != _rcDialog.bottom - _rcDialog.top)
        {
           Resize();
        }
    }
        break;
    }
    return FALSE;
}


//
// Window proc for frame window of both the read and send form.
//
//  Send frame window does not receive IDC_VIEW_ITEMABOVE, IDC_VIEW_ITEMBELOW
// commands.
// Read frame window does not receive IDC_MESSAGE_SUBMIT command.
LRESULT
CPadMessage::PadWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    HRESULT             hr;
    HMENU               hMenu;
    int                 wmId;
    int                 wmEvent;

    switch( wm )
    {
    case WM_DESTROY:
        OnDestroy();
        break;

    case WM_DEFERCOMBOUPDATE:
        return OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);

    case WM_COMMAND:
        wmId = GET_WM_COMMAND_ID(wParam, lParam);
        wmEvent = GET_WM_COMMAND_CMD(wParam, lParam);

        switch( wmId )
        {
        case IDM_MESSAGE_OPENHTM:
            hr = THR(PromptOpenFile(_hwnd, &CLSID_HTMLDocument));
            _fUserMode = 0;
            SendAmbientPropertyChange(DISPID_AMBIENT_USERMODE);
            break;

        case IDM_MESSAGE_CLOSE:
            ShutdownForm(SAVEOPTS_PROMPTSAVE);
            break;

        case IDM_MESSAGE_SAVE:
            hr = THR(DoSave(FALSE));
            if ( hr == MSOCMDERR_E_CANCELED )
            {
                hr = S_OK;
            }
            break;

        case IDM_MESSAGE_SAVE_AS:
            hr = THR(DoSave(TRUE));
            if ( hr == MSOCMDERR_E_CANCELED )
            {
                hr = S_OK;
            }
            break;

        case IDM_VIEW_ITEMABOVE:
        case IDM_VIEW_ITEMBELOW:
            {
                ULONG ulDir = IDM_VIEW_ITEMABOVE == wmId ? VCDIR_PREV:VCDIR_NEXT;
                DoNext(ulDir);

                break;
            }

        case IDM_MESSAGE_DELETE:
            DoDelete();
            break;

        case IDM_MESSAGE_REPLY:
            DoReply(eREPLY);
            break;

        case IDM_MESSAGE_REPLY_ALL:
            DoReply(eREPLY_ALL);
            break;

        case IDM_MESSAGE_FORWARD:
            DoReply(eFORWARD);
            break;

        case IDM_MESSAGE_SUBMIT:
            DoSubmit();
            break;

        case IDM_MESSAGE_COPY:
            DoCopy();
            break;

        case IDM_MESSAGE_MOVE:
            DoMove();
            break;

        case IDM_MESSAGE_CHECK_NAMES:
            DoCheckNames();
            break;

        case IDM_FONTSIZE:
        case IDM_BLOCKFMT:
        case IDM_FONTNAME:
        case IDM_FORECOLOR:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                PostMessage(_hwnd, WM_DEFERCOMBOUPDATE, wParam, lParam);
            }
            break;

        default:
            return OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
        }
        break;

        //
        //  Do all of the correct menu graying
        //

    case WM_INITMENU:
        hMenu = (HMENU) wParam;
        ConfigMenu(hMenu);
        break;

        //
        //  Deal with the System Close message
        //
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE)
        {
            ShutdownForm(SAVEOPTS_PROMPTSAVE);
        }
        else
        {
            goto DoDefault;
        }
        break;

    case WM_SETFOCUS:
        SetFocus(_hwndDialog);
        break;

    default:

DoDefault:
        return CPadDoc::PadWndProc(hwnd, wm, wParam, lParam);
    }

    return 0;
}



HRESULT
CPadMessage::InitReadToolbar ()
{
    static const TBBUTTON tbButton[] =
    {
        { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        { 0, IDM_MESSAGE_PRINT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 1, IDM_MESSAGE_MOVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 2, IDM_MESSAGE_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 10, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        { 3, IDM_MESSAGE_REPLY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 4, IDM_MESSAGE_REPLY_ALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 5, IDM_MESSAGE_FORWARD, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 10, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        { 6, IDM_VIEW_ITEMABOVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 7, IDM_VIEW_ITEMBELOW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 10, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},


        { 8, IDM_PAD_ABOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 10, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

    };

    HRESULT     hr = S_OK;

    // Create the toolbar

    _hwndToolbar = CreateToolbarEx(
            _hwnd,
            WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
            IDR_MESSAGE_TOOLBAR,
            9,                             // number of bitmaps
            g_hInstResource,
            IDB_MESSAGE_TOOLBAR,
            (LPCTBBUTTON) &tbButton,
            ARRAY_SIZE(tbButton),
            16,
            16,
            16,
            16,
            sizeof(TBBUTTON));
    if (!_hwndToolbar)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}
