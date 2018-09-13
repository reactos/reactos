//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       msghost.cxx
//
//  Contents:   CPadMessage's implementation of IDocHostUIHandler and
//              IDocHostShowUI
//
//------------------------------------------------------------------------


#include "padhead.hxx"

#ifndef X_MSG_HXX_
#define X_MSG_HXX_
#include "msg.hxx"
#endif

#ifndef X_MSGCIDX_H_
#define X_MSGCIDX_H_
#include "msgcidx.h"
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#undef WINCOMMCTRLAPI
#define WINCOMMCTRLAPI
#include "commctrl.h"
#endif

#define         MAXLABELLEN 32
#define         MAX_COMBO_VISUAL_ITEMS 20

IMPLEMENT_SUBOBJECT_IUNKNOWN(CPadMessageDocHost, CPadMessage, PadMessage, _DocHost);

STDMETHODIMP
CPadMessageDocHost::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IUnknown ||
        iid == IID_IDocHostUIHandler)
    {
        *ppv = (IDocHostUIHandler *) this;
    }
    else if (iid == IID_IDocHostShowUI)
    {
        *ppv = (IDocHostShowUI *) this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}


//+---------------------------------------------------------------
//
//      Implementation of IDocHostUIHandler
//
//+---------------------------------------------------------------


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::GetHostInfo
//
//  Synopsis:   Fetch information and flags from the host.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::GetHostInfo(DOCHOSTUIINFO * pInfo)
{
    Assert(pInfo);
    if (pInfo->cbSize < sizeof(DOCHOSTUIINFO))
        return E_INVALIDARG;

    pInfo->dwFlags = 0;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::ShowUI
//
//  Synopsis:   This method allows the host replace object's menu
//              and toolbars. It returns S_OK if host display
//              menu and toolbar, otherwise, returns S_FALSE.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::ShowUI(
        DWORD dwID,
        IOleInPlaceActiveObject * pActiveObject,
        IOleCommandTarget * pCommandTarget,
        IOleInPlaceFrame * pFrame,
        IOleInPlaceUIWindow * pDoc)
{
    CPadMessage *   pPad = PadMessage();

    if (!pPad->_hwndToolbar)
        pPad->CreateToolbarUI();

    pPad->_fShowUI = TRUE;
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::HideUI
//
//  Synopsis:   Remove menus and toolbars cretaed during the call
//              to ShowUI.
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::HideUI(void)
{
    CPadMessage *   pPad = PadMessage();

    pPad->_fShowUI = FALSE;
    // Do not hide menu/toolbar, just update status
    pPad->UpdateToolbarUI();
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::UpdateUI
//
//  Synopsis:   Update the state of toolbar buttons.
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::UpdateUI(void)
{
    CPadMessage *   pPad = PadMessage();
    pPad->UpdateToolbarUI();
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::EnableModeless
//
//  Synopsis:   Enable or disable modless UI.
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::EnableModeless(BOOL fEnable)
{
    // BUGBUG : To be implemented
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::OnDocWindowActivate
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::OnDocWindowActivate(BOOL fActivate)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::OnFrameWindowActivate
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::OnFrameWindowActivate(BOOL fActivate)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::ResizeBorder
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::ResizeBorder(
        LPCRECT prc,
        IOleInPlaceUIWindow * pUIWindow,
        BOOL fFrameWindow)
{
    CPadMessage *   pPad = PadMessage();
    BORDERWIDTHS bw;

    ::SetRect((LPRECT)&bw, 0, 0, 0, 0);
    pPad->_Frame.SetBorderSpace(&bw);
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::ShowContextMenu
//
//  Returns:    S_OK -- Host displayed its own UI.
//              S_FALSE -- Host did not display any UI.
//              DOCHOST_E_UNKNOWN -- The menu ID is unknown..
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::ShowContextMenu(
            DWORD dwID,
            POINT * pptPosition,
            IUnknown * pcmdtReserved,
            IDispatch * pDispatchObjectHit)
{
    HRESULT                 hr = S_FALSE;
    HMENU                   hmenu;
    HCURSOR                 hcursor;
    HCURSOR                 hcursorOld;
    HWND                    hwnd;
    CPadMessage *           pPad = PadMessage();

    if (!pPad->_pInPlaceObject)
        goto Cleanup;

    hr = THR(pPad->_pInPlaceObject->GetWindow(&hwnd));
    if (hr)
        goto Cleanup;

    hr = THR(pPad->GetContextMenu(&hmenu, dwID));
    if (hr)
        goto Cleanup;

    hcursor = LoadCursorA(NULL, (char *)IDC_ARROW);
    hcursorOld = ::SetCursor(hcursor);

    TrackPopupMenu(
            hmenu,
#ifndef _MAC
            TPM_LEFTALIGN | TPM_RIGHTBUTTON,
#else
            0,
#endif
            pptPosition->x,
            pptPosition->y,
            0,
            hwnd,
            NULL);

    ::SetCursor(hcursorOld);

Cleanup:
    return !hr? S_OK: S_FALSE;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::TranslateAccelerator
//
//  Returns:    S_OK -- The mesage was translated successfully.
//              S_FALSE -- The message was not translated.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::TranslateAccelerator(
            LPMSG lpmsg,
            const GUID * pguidCmdGroup,
            DWORD nCmdID)
{
    CPadMessage *   pPad = PadMessage();

    // If a control on the toolbar has the focus
    if ((::GetFocus() == lpmsg->hwnd) &&
            ((GetParent(lpmsg->hwnd) == pPad->_hwndToolbar) ||
             (GetParent(lpmsg->hwnd) == pPad->_hwndTBFormat)))
    {
        // if the key pressed is arrow keys and combo box is not dropped down,
        // drop down the combo box.
        //
        if ((lpmsg->message == WM_KEYDOWN) &&
                (((short) lpmsg->wParam == VK_UP) ||
                        ((short) lpmsg->wParam == VK_DOWN)))
        {
            if (!SendMessage(lpmsg->hwnd, CB_GETDROPPEDSTATE, 0, 0))
            {
                SendMessage(lpmsg->hwnd, CB_SHOWDROPDOWN, TRUE, 0);
            }
        }
        return S_OK;
    }
    return S_FALSE;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::GetOptionKeyPath
//
//  Synopsis:   Get the registry key where host stores its default
//              options.
//
//  Returns:    S_OK          -- Success.
//              E_OUTOFMEMORY -- Fail.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::GetOptionKeyPath(LPOLESTR * ppchKey, DWORD dw)
{
    HRESULT hr = E_INVALIDARG;

    if (dw == 0)
    {
        if (ppchKey)
        {
            *ppchKey = (LPOLESTR)CoTaskMemAlloc((_tcslen(_T("Software\\Microsoft\\Microsoft HTML Pad"))+1)*sizeof(TCHAR));
            if (*ppchKey)
                _tcscpy(*ppchKey, _T("Software\\Microsoft\\Microsoft HTML Pad"));
            hr = (*ppchKey) ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
        hr = S_FALSE;

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::GetDropTarget
//
//  Returns:    S_OK -- Host will return its droptarget to overwrite given one.
//              S_FALSE -- Host does not want to overwrite droptarget
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::GetDropTarget(
        IDropTarget * pDropTarget,
        IDropTarget ** ppDropTarget)
{
    return S_FALSE;
}


STDMETHODIMP
CPadMessageDocHost::GetExternal(IDispatch** ppDisp)
{
    HRESULT     hr;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppDisp = NULL;
        hr = S_OK;
    }
   
    return hr;
}


STDMETHODIMP
CPadMessageDocHost::TranslateUrl(DWORD dwTranslate,
                                 OLECHAR *pchURLIn,
                                 OLECHAR **ppchURLOut)
{
    HRESULT     hr;

    if (!ppchURLOut)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppchURLOut = NULL;
        hr = S_OK;
    }
   
    return hr;
}


STDMETHODIMP
CPadMessageDocHost::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
    HRESULT     hr;

    if (!ppDORet)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppDORet = NULL;
        hr = S_OK;
    }
   
    return hr;
}


////////////////////////////////////////////////////////////////////////
//
//      Implementation of IDocHostShowUI
//
////////////////////////////////////////////////////////////////////////


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::ShowMessage
//
//  Returns:    S_OK -- Host displayed its own UI.
//              S_FALSE -- Host did not display its own UI.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::ShowMessage(
        HWND hwnd,
        LPOLESTR lpstrText,
        LPOLESTR lpstrCaption,
        DWORD dwType,
        LPOLESTR lpstrHelpFile,
        DWORD dwHelpContext,
        LRESULT * plResult)
{
    LRESULT     lResult;

    lResult = MessageBoxEx(hwnd, lpstrText, lpstrCaption, dwType, 0);
    if (plResult)
        *plResult = lResult;

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadMessageDocHost::ShowHelp
//
//  Returns:    S_OK -- Host displayed its own help.
//              S_FALSE -- Host did not display its help.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadMessageDocHost::ShowHelp(
        HWND hwnd,
        LPOLESTR pszHelpFile,
        UINT uCommand,
        DWORD dwData,
        POINT ptMouse,
        IDispatch * pDispatchObjectHit)
{
    // BUGBUG: Temporary fix for beta1 to append window style.
    // Append ">LangRef"
    _tcscat(pszHelpFile, _T(">LangRef"));
    WinHelp(hwnd, pszHelpFile, uCommand, dwData);

    return S_OK;
}


HRESULT
CPadMessage::QueryService(REFGUID sid, REFIID iid, void ** ppv)
{
    *ppv = NULL;
    RRETURN(E_NOINTERFACE);
}

////////////////////////////////////////////////////////////////////////
//
//      Helper functions
//
////////////////////////////////////////////////////////////////////////

HRESULT
CPadMessage::CreateToolbarUI()
{
    HRESULT     hr = S_OK;

    HWND *      pHwndCombo = NULL;
    HWND        hwndToolbar = NULL;
    HFONT       hFont;
    DWORD       cTBCombos;
    TEXTMETRIC  tm;
    HDC         hdc;

    struct ComboInfo {
        UINT ComboIDM;
        UINT ToolbarIDR;
        LONG cx;
        LONG dx;
        LONG cElements;
        UINT ComboStyle;
    };

    static const ComboInfo tbCombos[] =
    {
        { IDM_BLOCKFMT,  IDR_HTMLPAD_TBFORMAT,       5, 100, 16,
                CBS_DROPDOWNLIST},
        { IDM_FONTNAME,    IDR_HTMLPAD_TBFORMAT,   110, 150, 43,
                CBS_SORT | CBS_DROPDOWNLIST},
        { IDM_FONTSIZE,    IDR_HTMLPAD_TBFORMAT,   265,  40,  8,
                CBS_DROPDOWNLIST},
        { IDM_FORECOLOR,   IDR_HTMLPAD_TBFORMAT,   384,  55, 12,
                CBS_DROPDOWNLIST},
        { 0, 0, 0, 0, 0}
    };

    static const TBBUTTON tbStdButton[] =
    {
        { 5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        { 0, IDM_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 1, IDM_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 2, IDM_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        { 3, IDM_CUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 4, IDM_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 5, IDM_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        { 6, IDM_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 7, IDM_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        { 8, IDM_BOOKMARK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 9, IDM_HYPERLINK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {10, IDM_HORIZONTALLINE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {11, IDM_IMAGE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {12, IDM_TABLE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {13, IDM_SHOWHIDE_CODE, TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},
        {60, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {14, IDM_PROPERTIES, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {15, IDM_PAGEINFO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        { 5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {16, IDM_PAD_FONTINC, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {17, IDM_PAD_FONTDEC, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
   };

    static const TBBUTTON tbFmtButton[] =
    {
        // reserved space for HTML Markup Tag,FontName, and FontSize Comboboxes
        {110, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
        {155, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
        { 45, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {0, IDM_BOLD,      TBSTATE_ENABLED, TBSTYLE_CHECK,  0L, 0},
        {1, IDM_ITALIC,    TBSTATE_ENABLED, TBSTYLE_CHECK,  0L, 0},
        {2, IDM_UNDERLINE, TBSTATE_ENABLED, TBSTYLE_CHECK,  0L, 0},

        // reserved space for BackGroundColor Combobox.
        { 65, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {3, IDM_JUSTIFYLEFT,   TBSTATE_CHECKED, TBSTYLE_CHECKGROUP, 0L, 0},
        {4, IDM_JUSTIFYCENTER, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0L, 0},
        {5, IDM_JUSTIFYRIGHT,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0L, 0},
        {5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {6, IDM_ORDERLIST,  TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0L, 0},
        {7, IDM_UNORDERLIST, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {8, IDM_OUTDENT,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {9, IDM_INDENT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
    };

    // Ensure that the common control DLL is loaded for status window.
    InitCommonControls();

    // Create toolbars.
    _hwndToolbar = CreateToolbarEx(
            _hwnd,
            WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
            IDR_HTMLPAD_TBSTANDARD,
            18,
            g_hInstResource,
            IDB_HTMLPAD_TBSTANDARD,
            (LPCTBBUTTON) &tbStdButton,
            ARRAY_SIZE(tbStdButton),
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

    _hwndTBFormat = CreateToolbarEx(
            _hwnd,
            WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
            IDR_HTMLPAD_TBFORMAT,
            10,
            g_hInstResource,
            IDB_HTMLPAD_TBFORMAT,
            (LPCTBBUTTON) &tbFmtButton,
            ARRAY_SIZE(tbFmtButton),
            16,
            16,
            16,
            16,
            sizeof(TBBUTTON));
    if (!(_hwndTBFormat))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    for (cTBCombos = 0; tbCombos[cTBCombos].ComboIDM; cTBCombos ++)
    {
        switch (tbCombos[cTBCombos].ComboIDM)
        {
        case IDM_BLOCKFMT:
            pHwndCombo = &_hwndComboTag;
            break;
        case IDM_FONTNAME:
            pHwndCombo = &_hwndComboFont;
            break;
        case IDM_FONTSIZE:
            pHwndCombo = &_hwndComboSize;
            break;
        case IDM_FORECOLOR:
            pHwndCombo = &_hwndComboColor;
            break;
        }
        switch (tbCombos[cTBCombos].ToolbarIDR)
        {
        case IDR_HTMLPAD_TBSTANDARD:
            hwndToolbar = _hwndToolbar;
            break;
        case IDR_HTMLPAD_TBFORMAT:
            hwndToolbar = _hwndTBFormat;
            break;
        }

        *pHwndCombo = CreateWindow(
                TEXT("COMBOBOX"),
                TEXT(""),
                WS_CHILD | WS_VSCROLL | tbCombos[cTBCombos].ComboStyle,
                0,
                0,
                tbCombos[cTBCombos].dx,
                0,
                _hwnd,
                (HMENU)tbCombos[cTBCombos].ComboIDM,
                g_hInstResource,
                NULL);
        if (!(*pHwndCombo))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hFont = (HFONT) GetStockObject(ANSI_VAR_FONT);
        SendMessage(*pHwndCombo, WM_SETFONT, (WPARAM) hFont, FALSE);
        hdc   = ::GetDC(*pHwndCombo);
        GetTextMetrics(hdc, &tm);
        ::ReleaseDC(*pHwndCombo, hdc);
        DeleteObject(hFont);

        EnableWindow(*pHwndCombo, TRUE);
        SetParent(*pHwndCombo, hwndToolbar);
        ::ShowWindow(*pHwndCombo, SW_SHOW);
        ::MoveWindow(
                *pHwndCombo,
                tbCombos[cTBCombos].cx,
                2,
                tbCombos[cTBCombos].dx,
                tm.tmHeight * min((long) MAX_COMBO_VISUAL_ITEMS,
                        (long) tbCombos[cTBCombos].cElements),
                FALSE);
    }

    LoadToolbarUI();
    ::ShowWindow(_hwndToolbar, SW_SHOW);
    ::ShowWindow(_hwndTBFormat, SW_SHOW);

Cleanup:
    return hr;
}

//+-------------------------------------------------------------------
//
//  Callback:   FillFontProc
//
//  This procedure is called by the EnumFontFamilies call.
//  It fills the combobox with the font facename.
//
//--------------------------------------------------------------------

int CALLBACK
FillFontProc(LOGFONT FAR *    lplf,
             TEXTMETRIC FAR * lptm,
             int              iFontType,
             LPARAM           lParam)
{
    int  fontStyle[3];
    char szFontName[128];

    fontStyle[0] = (lplf->lfWeight == FW_BOLD) ? (1) : (0);
    fontStyle[1] = (lplf->lfItalic == TRUE)    ? (1) : (0);
    fontStyle[2] = (lplf->lfUnderline == TRUE) ? (1) : (0);
    WideCharToMultiByte(
            CP_ACP,
            0,
            (const wchar_t *) lplf->lfFaceName,
            -1,
            szFontName,
            ARRAY_SIZE(szFontName),
            NULL,
            NULL);

    if (CB_ERR == (WPARAM) SendMessage((HWND) lParam,CB_FINDSTRING,
                  (WPARAM) -1,(LPARAM)(lplf->lfFaceName)))
    {
        SendMessage((HWND)lParam,CB_ADDSTRING,
                    (WPARAM) 0,(LPARAM)(lplf->lfFaceName));
    }
    return TRUE;
}


//+-------------------------------------------------------------------
//
// Local Helper Functions: AddComboboxItems, ConvColorrefToString
//
//--------------------------------------------------------------------

struct ComboItem {
        TCHAR * pName;
        LONG   lData;
};

static const ComboItem ComboColorItems[] =
{
    {TEXT("Black"),      RGB(0, 0, 0)},
    {TEXT("Navy"),       RGB(0, 0, 128)},
    {TEXT("Blue"),       RGB(0, 0, 255)},
    {TEXT("Cyan"),       RGB(0, 255, 255)},
    {TEXT("Red"),        RGB(255, 0, 0)},
    {TEXT("Lime"),       RGB(0, 255, 0)},
    {TEXT("Gray"),       RGB(128, 128, 128)},
    {TEXT("Green"),      RGB(0, 128, 0)},
    {TEXT("Yellow"),     RGB(255, 255, 0)},
    {TEXT("Pink"),       RGB(255, 192, 203)},
    {TEXT("Violet"),     RGB(238, 130, 238)},
    {TEXT("White"),      RGB(255, 255, 255)},
    {NULL, 0L}
};


DWORD AddComboboxItems(HWND hwndCombo,
                       BOOL fItemData,
                       const ComboItem * pComboItems)
{
    DWORD   dwIndex = 0;

    while(pComboItems->pName)
    {
        dwIndex = SendMessage(
                hwndCombo,
                CB_ADDSTRING,
                0,
                (LPARAM) pComboItems->pName);
        if (fItemData)
        {
            SendMessage(
                    hwndCombo,
                    CB_SETITEMDATA,
                    dwIndex,
                    (LPARAM) pComboItems->lData);
        }
        pComboItems ++;
    }
    return dwIndex;
}



void ConvColorrefToString(COLORREF crColor, LPTSTR szName)
{
    int     i;
    BOOL fFound = FALSE;

    for(i = 0; ComboColorItems[i].pName != NULL; i++)
    {
        if(ComboColorItems[i].lData == (LONG)crColor)  {
            fFound = TRUE;
            break;
        }
    }

    if(fFound)
        _tcscpy(szName, ComboColorItems[i].pName);
    else
        szName[0] = 0;
}


//+-------------------------------------------------------------------
//
// Member:  CPadMessage::LoadToolbarUI
//
//--------------------------------------------------------------------

void
CPadMessage::LoadToolbarUI()
{
    HDC      hdc;
    HRESULT  hr;

    // load items into ComboTag, ComboZoom and ComboSize comboboxes
    static const UINT ComboLoad[] = {
            IDM_FONTSIZE,
            IDM_GETBLOCKFMTS,
            0
    };

    VARIANTARG varRange;
    LONG lLBound, lUBound, lIndex, lValue;
    BSTR  bstrBuf;
    TCHAR szBuf[64];
    SAFEARRAY * psa = NULL;
    HWND hwndCombobox = NULL;
    int i;
    IOleCommandTarget * pCommandTarget = NULL;

     if ( !_pInPlaceObject  ||
     _pInPlaceObject->QueryInterface(
        IID_IOleCommandTarget, (void **)&pCommandTarget) )
    {
        // FAIL
        return;
    }

    for (i = 0; ComboLoad[i]; i ++)
    {
        switch (ComboLoad[i])
        {
        case IDM_FONTSIZE:
            hwndCombobox = _hwndComboSize;
            break;
        case IDM_GETBLOCKFMTS:
            hwndCombobox = _hwndComboTag;
            break;
        }
        varRange.vt = VT_ARRAY;
        varRange.parray = psa;

       hr = THR(pCommandTarget->Exec(
                (GUID *)&CGID_MSHTML,
                ComboLoad[i],
                MSOCMDEXECOPT_DONTPROMPTUSER,
                NULL,
                &varRange));
        if (OK(hr))
        {
            psa = V_ARRAY(&varRange);
            SafeArrayGetLBound(psa, 1, &lLBound);
            SafeArrayGetUBound(psa, 1, &lUBound);
            for (lIndex = lLBound; lIndex <= lUBound; lIndex ++)
            {
                switch (ComboLoad[i])
                {
                case IDM_GETBLOCKFMTS:
                    SafeArrayGetElement(psa, &lIndex, &bstrBuf);
                    SendMessage(hwndCombobox, CB_ADDSTRING, 0, (LPARAM) bstrBuf);
                    SysFreeString(bstrBuf);
                    break;

                case IDM_FONTSIZE:
                    SafeArrayGetElement(psa, &lIndex, &lValue);
                    Format(0, szBuf, ARRAY_SIZE(szBuf), TEXT("<0d>"), lValue);
                    SendMessage(hwndCombobox, CB_ADDSTRING, 0, (LPARAM) szBuf);
                    break;
                }
            }
            SafeArrayDestroyData(psa);
            SafeArrayDestroy(psa);
        }
    }

    HWND hWndInPlace;

    if (_pInPlaceObject)
    {
        _pInPlaceObject->GetWindow(&hWndInPlace);
    }
    if (!hWndInPlace)
        hWndInPlace = _hwnd;

    // insert font facenames to Font combobox.
    hdc = ::GetDC(hWndInPlace);
    EnumFontFamilies(
            hdc,
            NULL,
            (FONTENUMPROC) FillFontProc,
            (LPARAM)_hwndComboFont);
    ::ReleaseDC(hWndInPlace, hdc);

    // load items into ComboColor combobox and set default selection.
    AddComboboxItems(_hwndComboColor, TRUE, ComboColorItems);
    SendMessage(_hwndComboColor, CB_SETCURSEL, 0, 0);

    ReleaseInterface(pCommandTarget);
}



LRESULT
CPadMessage::OnCommand(WORD wNotifyCode, WORD idm, HWND hwndCtl)
{
    HRESULT             hr = S_OK;
    IOleCommandTarget * pCommandTarget = NULL;
    VARIANTARG *        pvarIn  = NULL;
    VARIANTARG *        pvarOut = NULL;
    VARIANTARG          var;
    TCHAR               achBuffer[64];
    BOOL                fRestoreFocus = FALSE;
    DWORD               nCmdexecopt = MSOCMDEXECOPT_DONTPROMPTUSER;

    switch (idm)
    {
        case IDM_FONTSIZE:
            fRestoreFocus = TRUE;

            GetWindowText(
                    _hwndComboSize,
                    achBuffer,
                    ARRAY_SIZE(achBuffer));
            var.vt   = VT_I4;
            var.lVal = _wtoi(achBuffer);
            pvarIn   = &var;
            break;

        case IDM_BLOCKFMT:
            fRestoreFocus = TRUE;

            GetWindowText(
                    _hwndComboTag,
                    achBuffer,
                    ARRAY_SIZE(achBuffer));
            var.vt      = VT_BSTR;
            var.bstrVal = achBuffer;
            pvarIn      = &var;
            break;

        case IDM_FONTNAME:
            fRestoreFocus = TRUE;

            GetWindowText(
                    _hwndComboFont,
                    achBuffer,
                    ARRAY_SIZE(achBuffer));
            var.vt      = VT_BSTR;
            var.bstrVal = achBuffer;
            pvarIn      = &var;
            break;

        case IDM_FORECOLOR:
            fRestoreFocus = TRUE;

            var.lVal = SendMessage(
                    _hwndComboColor,
                    CB_GETITEMDATA,
                    (WPARAM) SendMessage(_hwndComboColor, CB_GETCURSEL, 0, 0),
                    0);
            var.vt      = VT_I4;
            pvarIn      = &var;
            break;
    }

    switch(idm)
    {
        case IDM_MESSAGE_PRINT:
            idm = IDM_PRINT;
            // fall through

        case IDM_IMAGE:
        case IDM_HYPERLINK:
        case IDM_BOOKMARK:
        case IDM_FIND:
        case IDM_REPLACE:
        case IDM_PARAGRAPH:
        case IDM_GOTO:
        case IDM_FONT:
        case IDM_INSERTOBJECT:
            nCmdexecopt = 0;
            // fall through

        case IDM_CUT:
        case IDM_COPY:
        case IDM_PASTE:
        case IDM_PASTEINSERT:
        case IDM_DELETE:
        case IDM_SELECTALL:

        case IDM_NEW:
        case IDM_OPEN:
        case IDM_SAVE:
        case IDM_PASTESPECIAL:
        case IDM_UNDO:
        case IDM_REDO:
        case IDM_HORIZONTALLINE:
        case IDM_SHOWHIDE_CODE:
        case IDM_PROPERTIES:
        case IDM_BOLD:
        case IDM_ITALIC:
        case IDM_UNDERLINE:
        case IDM_JUSTIFYLEFT:
        case IDM_JUSTIFYCENTER:
        case IDM_JUSTIFYRIGHT:
        case IDM_ORDERLIST:
        case IDM_UNORDERLIST:
        case IDM_OUTDENT:
        case IDM_INDENT:
        case IDM_BLOCKFMT:
        case IDM_FONTNAME:
        case IDM_FONTSIZE:
        case IDM_FORECOLOR:

        case IDM_UNLINK:
        case IDM_UNBOOKMARK:
        case IDM_TOOLBARS:
        case IDM_STATUSBAR:
        case IDM_FORMATMARK:
        case IDM_TEXTONLY:
        case IDM_BASELINEFONT5:
        case IDM_BASELINEFONT4:
        case IDM_BASELINEFONT3:
        case IDM_BASELINEFONT2:
        case IDM_BASELINEFONT1:
        case IDM_PAD_REFRESH:
        case IDM_EDITSOURCE:
        case IDM_FOLLOWLINKC:
        case IDM_FOLLOWLINKN:
        case IDM_OPTIONS:
        case IDM_LINEBREAKNORMAL:
        case IDM_LINEBREAKLEFT:
        case IDM_LINEBREAKRIGHT:
        case IDM_LINEBREAKBOTH:
        case IDM_NONBREAK:
        case IDM_IFRAME:
        case IDM_1D:
        case IDM_TEXTBOX:
        case IDM_TEXTAREA:
#ifdef NEVER        
        case IDM_HTMLAREA:
#endif        
        case IDM_CHECKBOX:
        case IDM_RADIOBUTTON:
        case IDM_DROPDOWNBOX:
        case IDM_LISTBOX:
        case IDM_BUTTON:
        case IDM_FORM:
        case IDM_MARQUEE:
        case IDM_LIST:
        case IDM_PREFORMATTED:
        case IDM_ADDRESS:
        case IDM_BLINK:
        case IDM_DIV:
        case IDM_TABLEINSERT:
        case IDM_ROWINSERT:
        case IDM_COLUMNSELECT:
        case IDM_TABLESELECT:
        case IDM_CELLPROPERTIES:
        case IDM_TABLEPROPERTIES:
        case IDM_HELP_CONTENT:
        case IDM_HELP_ABOUT:

            if ( _pInPlaceObject &&
                 OK(_pInPlaceObject->QueryInterface(
                    IID_IOleCommandTarget, (void **)&pCommandTarget)) )
            {
                hr = pCommandTarget->Exec(
                        &CGID_MSHTML,
                        idm,
                        nCmdexecopt,
                        pvarIn,
                        pvarOut);
                // When the user selects a combo item, pop the focus pack into the document.
                if (fRestoreFocus)
                {

                    HWND    hWndInPlace;

                    _pInPlaceObject->GetWindow(&hWndInPlace);
                    if (hWndInPlace)
                    {
                        ::SetFocus (hWndInPlace);
                    }
                }
            }
            break;

        default:
            CPadDoc::OnCommand(wNotifyCode, idm, hwndCtl);
    }

    CheckError(_hwnd, hr);
    ReleaseInterface(pCommandTarget);
    return 0;
}


//+-------------------------------------------------------------------
//
//  Member:     CPadMessage::UpdateToolbarUI
//
//  Synopsis:
//
//--------------------------------------------------------------------

LRESULT
CPadMessage::UpdateToolbarUI()
{
    struct ButtonInfo {
        UINT ButtonIDM;
        UINT ToolbarIDR;
    };

    static const ButtonInfo tbButtons[] =
    {
        { IDM_NEW,              IDR_HTMLPAD_TBSTANDARD },
        { IDM_OPEN,             IDR_HTMLPAD_TBSTANDARD },
        { IDM_SAVE,             IDR_HTMLPAD_TBSTANDARD },
        { IDM_UNDO,             IDR_HTMLPAD_TBSTANDARD },
        { IDM_REDO,             IDR_HTMLPAD_TBSTANDARD },
        { IDM_TABLE,            IDR_HTMLPAD_TBSTANDARD },
        { IDM_BOOKMARK,         IDR_HTMLPAD_TBSTANDARD },
        { IDM_HYPERLINK,        IDR_HTMLPAD_TBSTANDARD },
        { IDM_HORIZONTALLINE,   IDR_HTMLPAD_TBSTANDARD },
        { IDM_IMAGE,            IDR_HTMLPAD_TBSTANDARD },
        { IDM_SHOWHIDE_CODE,    IDR_HTMLPAD_TBSTANDARD },
        { IDM_PROPERTIES,       IDR_HTMLPAD_TBSTANDARD },
        { IDM_PAGEINFO,         IDR_HTMLPAD_TBSTANDARD },
        { IDM_CUT,              IDR_HTMLPAD_TBSTANDARD },
        { IDM_COPY,             IDR_HTMLPAD_TBSTANDARD },
        { IDM_PASTE,            IDR_HTMLPAD_TBSTANDARD },
        { IDM_PAD_FONTINC,      IDR_HTMLPAD_TBSTANDARD },
        { IDM_PAD_FONTDEC,      IDR_HTMLPAD_TBSTANDARD },

        { IDM_BOLD,             IDR_HTMLPAD_TBFORMAT },
        { IDM_ITALIC,           IDR_HTMLPAD_TBFORMAT },
        { IDM_UNDERLINE,        IDR_HTMLPAD_TBFORMAT },
        { IDM_ORDERLIST,        IDR_HTMLPAD_TBFORMAT },
        { IDM_UNORDERLIST,      IDR_HTMLPAD_TBFORMAT },
        { IDM_INDENT,           IDR_HTMLPAD_TBFORMAT },
        { IDM_OUTDENT,          IDR_HTMLPAD_TBFORMAT },
        { IDM_JUSTIFYLEFT,      IDR_HTMLPAD_TBFORMAT },
        { IDM_JUSTIFYCENTER,    IDR_HTMLPAD_TBFORMAT },
        { IDM_JUSTIFYRIGHT,     IDR_HTMLPAD_TBFORMAT },
        { 0, 0}
    };

    UINT        cButtons;
    HWND        hwndToolbar = NULL;
    HRESULT     hr = S_OK;
    MSOCMD      msocmd;
    IOleCommandTarget * pCommandTarget = NULL;
    HWND  hwndCombobox = NULL;

    static const UINT ComboSet[] = {
            IDM_FONTNAME,
            IDM_FONTSIZE,
            IDM_BLOCKFMT,
            IDM_FORECOLOR,
            0 };

     if ( !_pInPlaceObject  ||
     _pInPlaceObject->QueryInterface(
        IID_IOleCommandTarget, (void **)&pCommandTarget) )
    {
        // FAIL
        goto Cleanup;
    }
    // update zoom combobox status

    VARIANTARG var;
    int j, iIndex, iCurrentIndex;
    TCHAR szBuf[128];

    for (j = 0; ComboSet[j]; j ++)
    {
        switch (ComboSet[j])
        {
        case IDM_FONTSIZE:
            hwndCombobox = _hwndComboSize;
            var.vt   = VT_I4;
            var.lVal = 0;
            break;
        case IDM_BLOCKFMT:
            hwndCombobox = _hwndComboTag;
            var.vt      = VT_BSTR;
            var.bstrVal = NULL;
            break;
        case IDM_FONTNAME:
            hwndCombobox = _hwndComboFont;
            var.vt      = VT_BSTR;
            var.bstrVal = NULL;
            break;
        case IDM_FORECOLOR:
            hwndCombobox = _hwndComboColor;
            var.vt      = VT_I4;
            var.lVal = 0;
            break;
        }

        msocmd.cmdID = ComboSet[j];
        msocmd.cmdf  = 0;

        // Only if object is active
        if (_fShowUI)
        {
            hr = THR(pCommandTarget->QueryStatus((GUID *)&CGID_MSHTML, 1, &msocmd, NULL));
        }
        switch (msocmd.cmdf)
        {
        case MSOCMDSTATE_UP:
        case MSOCMDSTATE_DOWN:
        case MSOCMDSTATE_NINCHED:
            EnableWindow(hwndCombobox, TRUE);
            break;

        case MSOCMDSTATE_DISABLED:
        default:
            EnableWindow(hwndCombobox, FALSE);
            break;
        }

        // Only if object is active
        if (_fShowUI)
        {
            hr = THR_NOTRACE(pCommandTarget->Exec((GUID *)&CGID_MSHTML, ComboSet[j],
                    MSOCMDEXECOPT_DONTPROMPTUSER, NULL, &var));
            if (FAILED(hr))
                continue;
        }

        switch (ComboSet[j])
        {
        case IDM_BLOCKFMT:
        case IDM_FONTNAME:
            // It is legal for the returned bstr to be NULL.
            wcscpy(szBuf, var.bstrVal ? var.bstrVal : TEXT(""));
            break;

        case IDM_FORECOLOR:
            if(V_VT(&var) == VT_NULL)
                szBuf[0] = 0;
            else
                ConvColorrefToString(V_I4(&var), szBuf);
            break;

        case IDM_FONTSIZE:
            // If the font size is changing in the selection VT_NULL is returned
            if(V_VT(&var) == VT_NULL)
            {
                szBuf[0] = 0;
            }
            else
            {
                Format(0, szBuf, ARRAY_SIZE(szBuf), TEXT("<0d>"), var.lVal);
            }
            break;
        }

        iIndex = SendMessage(
                hwndCombobox,
                CB_FINDSTRINGEXACT,
                (WPARAM) -1,
                (LPARAM)(LPTSTR) szBuf);

        if (iIndex == CB_ERR)
        {
            // CB_FINDSTRINGEXACT cannot find the string in the combobox.
            //
            switch (ComboSet[j])
            {
            case IDM_BLOCKFMT:
                // GetBlockFormat returns something not in the BlockFormat
                // combobox, display empty string.
                //
                iIndex = -1;
                break;

            case IDM_FONTSIZE:
            case IDM_FONTNAME:
            case IDM_FORECOLOR:
                // Nothing is selected
                iIndex = -1;
                break;
            }
        }

        iCurrentIndex = SendMessage(hwndCombobox, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
        if ( iCurrentIndex != iIndex )
            SendMessage(hwndCombobox, CB_SETCURSEL, (WPARAM) iIndex, (LPARAM) 0);

        // If the return value was a string free it
        if(var.vt == VT_BSTR && var.bstrVal != NULL)
            SysFreeString(var.bstrVal);
    }

    // update buttons status
    BOOL fEnabled;
    BOOL fChecked;
    for (cButtons = 0; tbButtons[cButtons].ButtonIDM != 0; cButtons ++)
    {
        switch (tbButtons[cButtons].ToolbarIDR)
        {
        case IDR_HTMLPAD_TBSTANDARD:
            hwndToolbar = _hwndToolbar;
            break;
        case IDR_HTMLPAD_TBFORMAT:
            hwndToolbar = _hwndTBFormat;
            break;
        }

        if (hwndToolbar)
        {
            msocmd.cmdID = tbButtons[cButtons].ButtonIDM;
            msocmd.cmdf  = 0;

            // Only if object is active
            if (_fActive)
            {
                hr = pCommandTarget->QueryStatus((GUID *)&CGID_MSHTML, 1, &msocmd, NULL);
            }

            switch (msocmd.cmdf)
            {
            case MSOCMDSTATE_UP:
            case MSOCMDSTATE_DOWN:
            case MSOCMDSTATE_NINCHED:
                fEnabled = TRUE;
                fChecked = (msocmd.cmdf == MSOCMDSTATE_DOWN) ? TRUE : FALSE;
                break;

            case MSOCMDSTATE_DISABLED:
            default:
                fEnabled = FALSE;
                fChecked = FALSE;
                break;
            }
            SendMessage(
                    hwndToolbar,
                    TB_ENABLEBUTTON,
                    (WPARAM) tbButtons[cButtons].ButtonIDM,
                    (LPARAM) MAKELONG(fEnabled, 0));
            SendMessage(
                    hwndToolbar,
                    TB_CHECKBUTTON,
                    (WPARAM) tbButtons[cButtons].ButtonIDM,
                    (LPARAM) MAKELONG(fChecked, 0));
        }
    }

Cleanup:
    ReleaseInterface(pCommandTarget);
    return 0;
}


LRESULT
CPadMessage::OnInitMenuPopup(HMENU hmenuPopup, UINT uPos, BOOL fSystemMenu)
{
    HRESULT     hr = S_OK;
    int         cMenuItem;
    MSOCMD      msocmd;
    UINT        mf;
    IOleCommandTarget * pCommandTarget =NULL;

    UINT    MenuItem [] =  {
        IDM_UNDO,
        IDM_REDO,
        IDM_CUT,
        IDM_COPY,
        IDM_PASTE,
        IDM_PASTEINSERT,
        IDM_DELETE,
        IDM_SELECTALL,
        IDM_FIND,
        IDM_REPLACE,
        IDM_GOTO,
        IDM_BOOKMARK,
        IDM_HYPERLINK,
        IDM_UNLINK,
        IDM_UNBOOKMARK,
        IDM_TOOLBARS,
        IDM_STATUSBAR,
        IDM_FORMATMARK,
        IDM_TEXTONLY,
        IDM_BASELINEFONT5,
        IDM_BASELINEFONT4,
        IDM_BASELINEFONT3,
        IDM_BASELINEFONT2,
        IDM_BASELINEFONT1,
        IDM_PAD_REFRESH,
        IDM_EDITSOURCE,
        IDM_FOLLOWLINKC,
        IDM_FOLLOWLINKN,
        IDM_PROPERTIES,
        IDM_OPTIONS,
        IDM_HORIZONTALLINE,
        IDM_LINEBREAKNORMAL,
        IDM_LINEBREAKLEFT,
        IDM_LINEBREAKRIGHT,
        IDM_LINEBREAKBOTH,
        IDM_NONBREAK,
        IDM_SPECIALCHAR,
        IDM_HTMLSOURCE,
        IDM_IFRAME,
        IDM_1D,
        IDM_TEXTBOX,
        IDM_TEXTAREA,
        IDM_HTMLAREA,
        IDM_CHECKBOX,
        IDM_RADIOBUTTON,
        IDM_DROPDOWNBOX,
        IDM_LISTBOX,
        IDM_BUTTON,
        IDM_IMAGE,
        IDM_INSERTOBJECT,
        IDM_FONT,
        IDM_PARAGRAPH,
        IDM_FORM,
        IDM_MARQUEE,
        IDM_LIST,
        IDM_INDENT,
        IDM_OUTDENT,
        IDM_PREFORMATTED,
        IDM_ADDRESS,
        IDM_BLINK,
        IDM_DIV,
        IDM_TABLEINSERT,
        IDM_ROWINSERT,
        IDM_COLUMNINSERT,
        IDM_CELLINSERT,
        IDM_CAPTIONINSERT,
        IDM_CELLMERGE,
        IDM_CELLSPLIT,
        IDM_CELLSELECT,
        IDM_ROWSELECT,
        IDM_COLUMNSELECT,
        IDM_TABLESELECT,
        IDM_CELLPROPERTIES,
        IDM_TABLEPROPERTIES,
        IDM_HELP_CONTENT,
        IDM_HELP_ABOUT,
        0
    };

    if ( !_pInPlaceObject )
        goto Cleanup;

    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if (hr)
        goto Cleanup;

    for (cMenuItem = 0; MenuItem[cMenuItem]; cMenuItem ++)
    {
        msocmd.cmdID = MenuItem[cMenuItem];
        msocmd.cmdf  = 0;

        // Only if object is active
        if (_fActive)
        {
            hr = pCommandTarget->QueryStatus(
                    (GUID *)&CGID_MSHTML,
                    1,
                    &msocmd,
                    NULL);
        }

        switch (msocmd.cmdf)
        {
            case MSOCMDSTATE_UP:
            case MSOCMDSTATE_NINCHED:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_UNCHECKED;
                break;

            case MSOCMDSTATE_DOWN:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_CHECKED;
                break;

            case MSOCMDSTATE_DISABLED:
            default:
                mf = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
                break;
        }
        CheckMenuItem(hmenuPopup, msocmd.cmdID, mf);
        EnableMenuItem(hmenuPopup, msocmd.cmdID, mf);
    }

    CPadDoc::OnInitMenuPopup(hmenuPopup, uPos, fSystemMenu);

Cleanup:
    ReleaseInterface(pCommandTarget);
    return 0;
}


//+------------------------------------------------------------------------
//
//  Member:     CPadMessage::GetContextMenu
//
//  Synopsis:   Returns the context menu based on the sub-menu id.
//
//-------------------------------------------------------------------------

HRESULT
CPadMessage::GetContextMenu(HMENU *phmenu, int id)
{
    if (!_hMenuCtx)
    {
        _hMenuCtx = LoadMenu(
                g_hInstResource,
                MAKEINTRESOURCE(IDR_HTMLPAD_CONTEXT_MENU));
        if (!_hMenuCtx)
            goto Error;
    }

    *phmenu = GetSubMenu(_hMenuCtx, id);
    if (!*phmenu)
        goto Error;

    return S_OK;

Error:
    RRETURN(GetLastWin32Error());
}
