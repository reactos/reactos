//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       sui.cxx
//
//  Contents:   Implementation of CServer UI
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#include "commctrl.h"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_FRAMELYT_HXX_
#define X_FRAMELYT_HXX_
#include "framelyt.hxx"
#endif

#ifndef X_HTIFRAME_H_
#define X_HTIFRAME_H_
#include <htiframe.h>
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#include "eventobj.h"

#ifndef NO_IME
extern HRESULT DeactivateDIMM(); // imm32.cxx
#endif // ndef NO_IME

#if DBG == 1
#define MAXLABELLEN 32

static HMENU hMenuHelp;

//+---------------------------------------------------------------
//
//  Member:     CDoc::InsertSharedMenus
//
//  Synopsis:   Inserts the objects menus into a shared menu after
//              the top-level application has inserted its menus
//
//  Arguments:  [hmenuShared] -- the shared menu to recieve the objects menus
//              [hmenuObject] -- all of the objects menus
//              [lpmgw] -- menu group widths indicating where the menus
//                          should be inserted
//              [lOffset] -- Server position offset for hmenuObject
//
//  Returns:    Success if the menus were merged successfully
//
//  Notes:      The function does most of the shared menu work
//              by the object between the IOleInPlaceFrame::InsertMenus and
//              IOleInPlaceFrame::SetMenu method calls.
//              c.f. RemoveServerMenus
//
//----------------------------------------------------------------

HRESULT
CDoc::InsertSharedMenus(
        HMENU hmenuShared,
        HMENU hmenuObject,
        LPOLEMENUGROUPWIDTHS lpmgw)
{
    int i, j;
    UINT SvrPos, ShdPos;
    HMENU hmenuXfer;
    TCHAR szLabel[MAXLABELLEN];

    SvrPos = 0;
    ShdPos = 0;

    // for each of the Edit, Object, and Help menu groups
    for (j = 1; j <= 5; j += 2)
    {
        // advance over container menus
        ShdPos += (UINT)lpmgw->width[j-1];

        // special consideration for j = 5 because of
        // DocObject HELP menu merging.

        if (j == 5)
        {
            if (lpmgw->width[j] != 0)
            {
                BOOL    fHide = FALSE;

                // check if host want hide our help menu,
                if (_pHostUIHandler &&
                    (_dwFlagsHostInfo & DOCHOSTUIFLAG_DISABLE_HELP_MENU))
                {
                    fHide = TRUE;
                }

                if (!fHide)
                {
                    GetMenuString(
                                hmenuObject,
                                ++SvrPos,
                                szLabel,
                                MAXLABELLEN,
                                MF_BYPOSITION);

                    hMenuHelp = GetSubMenu(hmenuShared, ShdPos);
                    hmenuXfer = GetSubMenu(hmenuObject, SvrPos);
                    if (!AppendMenu(
                            hMenuHelp,
                            MF_POPUP | MF_STRING,
                            (UINT_PTR) hmenuXfer,
                            szLabel))
                    {
                        return HRESULT_FROM_WIN32(GetLastError());
                    }
                }

                lpmgw->width[j - 1] += lpmgw->width[j];
                lpmgw->width[j]      = 0;
                continue;
            }
            else
            {
                // no HELP menu in the container
                //
                hMenuHelp = NULL;
                lpmgw->width[j] = 1;
            }
        }

        // pull out the popup menus from servers menu
        for (i = 0; i < lpmgw->width[j]; i++)
        {
            GetMenuString(hmenuObject,
                    SvrPos,
                    szLabel,
                    MAXLABELLEN,
                    MF_BYPOSITION);
            hmenuXfer = GetSubMenu(hmenuObject, SvrPos++);
            if (!InsertMenu(hmenuShared,
                        ShdPos++,
                        MF_BYPOSITION | MF_POPUP | MF_STRING,
                        (UINT_PTR)hmenuXfer,
                        szLabel))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::RemoveSharedMenus
//
//  Synopsis:   Removes the objects menus from a shared menu
//
//  Arguments:  [hmenuShared] -- the menu contain both the application's
//                              and the object's menus
//              [lpmgw] -- menu group widths indicating which menus should
//                          be removed
//
//  Notes:      c.f. InsertServerMenus
//
//----------------------------------------------------------------

void
CDoc::RemoveSharedMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpmgw)
{
    int i, j;
    UINT ShdPos;

    ShdPos = 0;

    if (hMenuHelp)
    {
        // remove the Object part of the merged HELP menu
        //
        RemoveMenu(hMenuHelp, GetMenuItemCount(hMenuHelp) - 1, MF_BYPOSITION);
    }

    // for each of the Edit, Object, and Help menu groups
    for (j = 1; j <= 5; j += 2)
    {
        // advance over container menus
        ShdPos += (UINT)lpmgw->width[j-1];

        // pull out the popup menus from shared menu
        for (i = 0; i < lpmgw->width[j]; i++)
        {
            RemoveMenu(hmenuShared, ShdPos, MF_BYPOSITION);
        }
    }

    return;
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::CreateMenuUI
//
//  Synopsis:   Creates menu UI elements using menu specified
//              in class descriptor.
//
//---------------------------------------------------------------


HRESULT
CDoc::CreateMenuUI()
{
    HRESULT             hr = S_OK;
#ifndef NO_OLEUI
    HMENU               hmenu;
    HMENU               hmenuShared;
    OLEMENUGROUPWIDTHS *pmgw;

    Assert(InPlace());

    //  We ignore any failures; a failure just means some part of
    //    the UI won't appear.

    Assert((InPlace()->_hmenuShared) == NULL);
    Assert((InPlace()->_hmenu) == NULL);

    pmgw = &(InPlace()->_mgw);

    if (_fDesignMode)
    {
        hmenu = TFAIL_NOTRACE(0, LoadMenu(
                GetResourceHInst(),
                MAKEINTRESOURCE(IDR_HTMLFORM_MENUDESIGN)));
        *pmgw = s_amgw[0];
    }
    else
    {
        hmenu = TFAIL_NOTRACE(0, LoadMenu(
                GetResourceHInst(),
                MAKEINTRESOURCE(IDR_HTMLFORM_MENURUN)));
#if 0
        // Note: this will go away with BeomOh's com work
        extern HMENU CreateMimeCSetMenu();

        // Dynamic adding language menu
        if (hmenu)
        {
            // Get "View" menu
            HMENU hmenuView = GetSubMenu(hmenu, 1);

            // Consider: Write a wrapper for SetMenuItemInfo.  We are using
            //  SetMenuItemInfo directly since we never need to set the one
            //  LPTSTR member of the MENUITEMINFO structure.
            MENUITEMINFO mii;

            mii.cbSize = sizeof(mii);
            mii.fMask  = MIIM_SUBMENU;
            mii.hSubMenu = CreateMimeCSetMenu();
            SetMenuItemInfo(hmenuView, IDM_LANGUAGE, FALSE, &mii);
        }
#endif
        *pmgw = s_amgw[1];
    }
    if (!hmenu)
        goto Cleanup;

    hmenuShared = TFAIL(0, CreateMenu());
    if (!hmenuShared)
        goto Cleanup;

    hr = THR(InPlace()->_pFrame->InsertMenus(hmenuShared, pmgw));
    if (hr)
        goto Cleanup;

    hr = THR(InsertSharedMenus(hmenuShared, hmenu, pmgw));
    if (hr)
        goto Cleanup;

    InPlace()->_hmenu = hmenu;
    InPlace()->_hmenuShared = hmenuShared;
    InPlace()->_hOleMenu = OleCreateMenuDescriptor(hmenuShared, pmgw);

Cleanup:
#endif // NO_OLEUI
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::DestroyMenuUI, protected
//
//  Synopsis:   This method "undoes" everything that was done in
//              CreateMenuUI -- destroys the shared menu and OLE menu
//              descriptor.
//---------------------------------------------------------------

void
CDoc::DestroyMenuUI(void)
{
#ifndef NO_OLEUI
    Assert(InPlace());

    if (InPlace()->_hmenuShared)
    {
        HMENU hmenu = InPlace()->_hmenuShared;

        RemoveSharedMenus(hmenu, &InPlace()->_mgw);
        InPlace()->_pFrame->RemoveMenus(hmenu);
        DestroyMenu(hmenu);
        InPlace()->_hmenuShared = NULL;
    }

    if (InPlace()->_hmenu)
    {
        DestroyMenu(InPlace()->_hmenu);
        InPlace()->_hmenu = NULL;
    }
    if (InPlace()->_hOleMenu)
    {
        OleDestroyMenuDescriptor(InPlace()->_hOleMenu);
        InPlace()->_hOleMenu = NULL;
    }
#endif // NO_OLEUI
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::InstallMenuUI, protected
//
//  Synopsis:   This method uses IOleInPlaceFrame::SetMenu to
//              install the shared menu constructed in CreateMenuUI.
//
//---------------------------------------------------------------

HRESULT
CDoc::InstallMenuUI()
{
#ifdef NO_OLEUI
    return S_OK;
#else
    // Ignore spurious WM_ERASEBACKGROUNDs generated by SetMenu
    CLock   Lock(this, SERVERLOCK_IGNOREERASEBKGND);
    HRESULT hr;

    if (!InPlace()->_hOleMenu)
    {
        hr = CreateMenuUI();
        if (hr)
            goto Cleanup;
    }

    hr = THR(InPlace()->_pFrame->SetMenu(
            InPlace()->_hmenuShared,
            InPlace()->_hOleMenu,
            InPlace()->_hwnd));
    if (hr)
        goto Cleanup;

    InPlace()->_fMenusMerged = 1;

Cleanup:
    RRETURN(hr);
#endif // NO_OLEUI
}

//+-------------------------------------------------------------------
//
//  Window Procedure: ComboWndProc
//
//  Synopsis:
//
//--------------------------------------------------------------------

#if defined(DBG_TOOLTIPS)

static WNDPROC lpfnDefCombo;

LRESULT CALLBACK
ComboWndProc(HWND hWnd,
             UINT uMessage,
             WPARAM wParam,
             LPARAM lParam)
{
    MSG        msg;
    HWND       hwndTooltip;

    switch (uMessage)
    {
    case WM_MOUSEMOVE :
    case WM_LBUTTONDOWN :
    case WM_LBUTTONUP :
        msg.lParam  = lParam;
        msg.wParam  = wParam;
        msg.message = uMessage;
        msg.hwnd    = hWnd;
        hwndTooltip = (HWND) SendMessage(GetParent(hWnd), TB_GETTOOLTIPS, 0, 0);
        SendMessage(hwndTooltip, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
        break;

    case WM_CHAR:
        // Don't allow the user to type anything into the combo
        return 0;
    }
    return CallWindowProc(lpfnDefCombo, hWnd, uMessage, wParam, lParam);
}

#endif

//+-------------------------------------------------------------------
//
// Local Helper Function: InstallComboboxTooltip
//
//--------------------------------------------------------------------

#if defined(DBG_TOOLTIPS)

void InstallComboboxTooltip(HWND hwndCombo, UINT IDMmessage)
{
    HWND     hwndTooltip;
    TOOLINFO tbToolInfo;

    hwndTooltip = (HWND) SendMessage(
            GetParent(hwndCombo),
            TB_GETTOOLTIPS,
            0,
            0);

    SetWindowLongPtr(hwndCombo, GWLP_WNDPROC, (LONG_PTR)ComboWndProc);

    tbToolInfo.cbSize   = sizeof(TOOLINFO);
    tbToolInfo.uFlags   = TTF_IDISHWND;
    tbToolInfo.hwnd     = GetParent(hwndCombo);
    tbToolInfo.uId      = (UINT) hwndCombo;
    tbToolInfo.hinst    = 0;

#ifndef WINCE
    DWORD dwVersion = GetVersion();
#else
    DWORD dwVersion = 0;
#endif

#ifdef WIN16
    // BUGWIN16: something different for Win16 ??
    // vamshi - 1/24/97
#else
    tbToolInfo.lpszText = (dwVersion < 0x80000000) ?
            ((LPTSTR) LPSTR_TEXTCALLBACKW) : ((LPTSTR) LPSTR_TEXTCALLBACKA);
    SendMessage(
            hwndTooltip,
            (dwVersion < 0x80000000) ? (TTM_ADDTOOLW) : (TTM_ADDTOOLA),
            0,
            (LPARAM)(LPTOOLINFO)&tbToolInfo);
#endif
}

#endif

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

    // We don't want to list the vertical fonts.
    // These by convention start with an @ symbol.
    if (lplf->lfFaceName[0] == L'@')
        return TRUE;

    fontStyle[0] = (lplf->lfWeight == FW_BOLD) ? (1) : (0);
    fontStyle[1] = (lplf->lfItalic == TRUE)    ? (1) : (0);
    fontStyle[2] = (lplf->lfUnderline == TRUE) ? (1) : (0);
    WideCharToMultiByte(
            CP_ACP,
            0,
            (const WCHAR *) lplf->lfFaceName,
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
        INT     iIdm;
        LONG    lData;
};

static const ComboItem ComboColorItems[] =
{
    {IDS_COLOR_BLACK,      RGB(0, 0, 0)},
    {IDS_COLOR_NAVY,       RGB(0, 0, 128)},
    {IDS_COLOR_BLUE,       RGB(0, 0, 255)},
    {IDS_COLOR_CYAN,       RGB(0, 255, 255)},
    {IDS_COLOR_RED,        RGB(255, 0, 0)},
    {IDS_COLOR_LIME,       RGB(0, 255, 0)},
    {IDS_COLOR_GRAY,       RGB(128, 128, 128)},
    {IDS_COLOR_GREEN,      RGB(0, 128, 0)},
    {IDS_COLOR_YELLOW,     RGB(255, 255, 0)},
    {IDS_COLOR_PINK,       RGB(255, 192, 203)},
    {IDS_COLOR_VIOLET,     RGB(238, 130, 238)},
    {IDS_COLOR_WHITE,      RGB(255, 255, 255)},
    {0, 0L}
};


void ConvColorrefToString(COLORREF crColor, LPTSTR szName, int cchName )
{
    int     i;
    BOOL fFound = FALSE;

    if(crColor == (COLORREF)-1)
    {
        szName[0] = 0;
        return;
    }

    // Reset the upper 8 bits because palettergb type color values have them
    // set to 0x20 and the compare will fail
    crColor &= 0xFFFFFF;

    for(i = 0; ComboColorItems[i].iIdm != 0; i++)
    {
        if(ComboColorItems[i].lData == (LONG)crColor)  {
            fFound = TRUE;
            break;
        }
    }

    if(fFound)
        Format(0, szName, cchName, MAKEINTRESOURCE(ComboColorItems[i].iIdm));
    else
        szName[0] = 0;
}


DWORD AddComboboxItems(HWND hwndCombo,
                       BOOL fItemData,
                       const ComboItem * pComboItems)
{
    DWORD   dwIndex = 0;
    TCHAR   achColor[128];

    while(pComboItems->iIdm)
    {
        Format(0, achColor, ARRAY_SIZE(achColor),
                  MAKEINTRESOURCE(pComboItems->iIdm));

        dwIndex = SendMessage(
                hwndCombo,
                CB_ADDSTRING,
                0,
                (LPARAM) achColor);
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
#endif // DBG == 1

//+---------------------------------------------------------------
//
//  Member:     CDoc::OnFrameWindowActivate, IOleInPlaceActiveObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//---------------------------------------------------------------

HRESULT
CDoc::OnFrameWindowActivate(BOOL fActivate)
{
    if (_pHostUIHandler)
    {
        _pHostUIHandler->OnFrameWindowActivate(fActivate);
    }

    return super::OnFrameWindowActivate(fActivate);
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::OnDocWindowActivate, IOleInPlaceActiveObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//  Notes:      This method will install or remove the frame
//              U.I. elements using the InstallFrameUI or RemoveFrameUI
//              methods.  This is to properly handle the MDI application
//              case.  It also updates our shading color.
//
//---------------------------------------------------------------

HRESULT
CDoc::OnDocWindowActivate(BOOL fActivate)
{
    if (_pHostUIHandler)
    {
        _pHostUIHandler->OnDocWindowActivate(fActivate);
    }

    return super::OnDocWindowActivate(fActivate);

}


//+---------------------------------------------------------------
//
//  Member:     CDoc::ResizeBorder, IOleInPlaceActiveObject
//
//  Synopsis:   Handle border space change.
//
//---------------------------------------------------------------

HRESULT
CDoc::ResizeBorder(
        LPCOLERECT prc,
        LPOLEINPLACEUIWINDOW pUIWindow,
        BOOL fFrameWindow)
{
    HRESULT hr = S_OK;

    if (!_pInPlace)
        return S_OK;

    if(InPlace()->_fHostShowUI)
    {
        Assert(_pHostUIHandler);
        _pHostUIHandler->ResizeBorder((RECT *)prc, pUIWindow, fFrameWindow); //RECT * is for win16. doesn't affect 32bit.
        return S_OK;
    }

    RRETURN_NOTRACE(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::EnableModeless, IOleInPlaceActiveObject
//
//  Synopsis:   Method of IOleInPlaceObject interface
//
//---------------------------------------------------------------

HRESULT
CDoc::EnableModeless(BOOL fEnable)
{
    _fSuspendTimeout = !fEnable;

    if(_pHostUIHandler)
    {
        _pHostUIHandler->EnableModeless(fEnable);
    }

    // This is just a hack for PumpMessageHelper(0 to know
    // if a modal dialog has been brought in an event handler.
    // The flag is cleared by PumpMessageHelper().
    if (!fEnable)
        _fModalDialogInScript = TRUE;

    _fModalDialogUp = !fEnable;

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CDoEnableModeless::CDoEnableModeless
//
//  Synopsis:   ctor of object which wraps enable modeless 
//              functionality.
//
//---------------------------------------------------------------

CDoEnableModeless::CDoEnableModeless(CDoc *pDoc, BOOL fAuto)
{
    _pDoc = pDoc;
    _fCallEnableModeless = FALSE;
    _fAuto = FALSE;
    
    if (!_pDoc)
        return;

    if (fAuto)
    {
        _fAuto = TRUE;
        DisableModeless();
    }
}


//+---------------------------------------------------------------
//
//  Member:     CDoEnableModeless::~CDoEnableModeless
//
//  Synopsis:   dtor
//
//---------------------------------------------------------------

CDoEnableModeless::~CDoEnableModeless()
{
    if (_fAuto)
    {
        EnableModeless();
    }
}


//+---------------------------------------------------------------
//
//  Member:     CDoEnableModeless::DisableModeless
//
//  Synopsis:   Worker which goes about disabling modeless.
//              Sets state on whether or not EnableModeless does
//              any work.
//
//---------------------------------------------------------------

void
CDoEnableModeless::DisableModeless()
{
    IOleWindow *    pOleWindow = NULL;

    if (_pDoc->_state >= OS_INPLACE)
    {
        // If we're not interactive yet, we should be.
        _pDoc->PrimaryMarkup()->OnLoadStatus(LOADSTATUS_INTERACTIVE);
    }

    _pDoc->EnableModeless(FALSE);
    if (_pDoc->_pInPlace && _pDoc->_pInPlace->_pFrame)
    {
        _fCallEnableModeless = TRUE;
        _pDoc->_pInPlace->_pFrame->EnableModeless(FALSE);
    }

    _hwnd = _pDoc->GetHWND();
    if (!_hwnd)
    {
        if (_pDoc->_pClientSite && !_pDoc->_pClientSite->QueryInterface(
                IID_IOleWindow, (void **)&pOleWindow))
        {
            IGNORE_HR(pOleWindow->GetWindow(&_hwnd));
        }
    }

    // CHROME
    // If Chrome hosted use the container's windowless interface
    // rather than Win32's ReleaseCaptureas we are windowless and
    // have no HWND.
    if (!_pDoc->IsChromeHosted())
        ::ReleaseCapture(); // To cancel any drag-drop operation
    else
        _pDoc->SetCapture(FALSE);

    ReleaseInterface(pOleWindow);
}


//+---------------------------------------------------------------
//
//  Member:     CDoEnableModeless::EnableModeless
//
//  Synopsis:   Worker which does opposite of DisableModeless
//
//---------------------------------------------------------------

void
CDoEnableModeless::EnableModeless(BOOL fForce)
{
    _pDoc->EnableModeless(TRUE);
    if (_fCallEnableModeless || fForce)
    {
        if (_pDoc->_pInPlace && _pDoc->_pInPlace->_pFrame)
        {
            _pDoc->_pInPlace->_pFrame->EnableModeless(TRUE);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ShowMessage/ShowMessageV
//
//  Synopsis:   Show display a message box to the user.
//
//  Arguments:  pnResult      Return result per Windows MessageBox. Can be null.
//              dwFlags       Flags taken from the Windows MB_ enumeration.
//              dwHelpContext Help context in Forms^3 help file
//              idsMessage    String id of message. Use null terminator in
//                            message to separate message parts (bold/norma).
//
//--------------------------------------------------------------------------

HRESULT __cdecl
CDoc::ShowMessage(        
        int * pnResult,
        DWORD dwFlags,
        DWORD dwHelpContext,
        UINT  idsMessage, ...) 
{
    HRESULT hr;
    va_list arg;

    va_start(arg, idsMessage);
    hr = THR(ShowMessageV(pnResult, dwFlags, dwHelpContext, idsMessage, &arg));
    va_end(arg);
    RRETURN(hr);
}

HRESULT
CDoc::ShowMessageV(      
        int *pnResult,
        DWORD dwFlags,
        DWORD dwHelpContext,
        UINT idsMessage,
        void *pvArgs)    
{
    HRESULT hr = S_OK;
    TCHAR * pch = NULL;

    if (THR(VFormat(FMT_OUT_ALLOC,
            &pch,
            0,
            MAKEINTRESOURCE(idsMessage),
            pvArgs)))                
        goto Cleanup;

    hr = THR(ShowMessageEx(            
            pnResult,
            dwFlags,
            GetFormsHelpFile(),
            dwHelpContext,
            pch));          

Cleanup:
    delete [] pch;
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ShowMessageEx
//
//  Synopsis:   Show a message to the user. This is a helper function
//              for ShowMessage, ShowMessageV, and DisplayLastError.
//              We first query the IDocHostUIHandler, if that fails,
//              we make an Exec call on the host, if that fails,
//              we make an Exec call on the backup shdocvw.              
//             
//--------------------------------------------------------------------------

HRESULT
CDoc::ShowMessageEx(       
        int   * pnResult,
        DWORD   dwStyle,
        TCHAR * pchHelpFile,
        DWORD   dwHelpContext,
        TCHAR * pchText)       
{       
    int                 nResult = 0;
    HWND                hwnd = NULL;          
    IOleCommandTarget * pBackupUICommandHandler = NULL;           
    TCHAR             * pchCaption = NULL;
    EVENTPARAM          param(this, TRUE);  
    VARIANT             varIn, varOut;              
    CDoEnableModeless   dem(this);
    
    hwnd = dem._hwnd;
    if (hwnd)                   
        MakeThisTopBrowserInProcess(hwnd);
    
    if (Format(FMT_OUT_ALLOC, &pchCaption, 0, MAKEINTRESOURCE(IDS_MESSAGE_BOX_TITLE)))
        goto Cleanup;             

    // See if the IDocHostShowUI interface is implemented
    if (InPlace() && InPlace()->_pHostShowUI)
    {         
        if (!InPlace()->_pHostShowUI->ShowMessage(
                _pInPlace->_hwnd,
                pchText,
                pchCaption,
                dwStyle,
                pchHelpFile,
                dwHelpContext,
                (LRESULT *)&nResult))             
            goto Cleanup;
    }

    // Fill out expandos
    param.SetType(_T("message"));
    param.messageParams.pchMessageText          = pchText;
    param.messageParams.pchMessageCaption       = pchCaption;
    param.messageParams.dwMessageStyle          = dwStyle;
    param.messageParams.pchMessageHelpFile      = pchHelpFile;   
    param.messageParams.dwMessageHelpContext    = dwHelpContext;                            

    V_VT(&varIn) = VT_UNKNOWN;
    V_UNKNOWN(&varIn) = (IUnknown*)(IPrivateUnknown *)this;

    // Query the host to handle showing the message
    if (_pHostUICommandHandler && !_fOutlook98)
    {        
         // If host displayed message box, Forms3 will not display its own.
        if (!_pHostUICommandHandler->Exec(
                &CGID_DocHostCommandHandler, 
                OLECMDID_SHOWMESSAGE, 
                0, 
                &varIn,
                &varOut))
        {
            nResult = V_I4(&varOut);
            goto Cleanup;        
        }
    }
              
    // Let backup show the message          
    EnsureBackupUIHandler();
    if (!_pBackupHostUIHandler)                            
        goto Cleanup;       
                        
    if (_pBackupHostUIHandler->QueryInterface(IID_IOleCommandTarget,
            (void **) &pBackupUICommandHandler))  
        goto Cleanup;                               
    
    if (!THR(pBackupUICommandHandler->Exec(
            &CGID_DocHostCommandHandler,         
            OLECMDID_SHOWMESSAGE,         
            0,         
            &varIn,        
            &varOut)))
        nResult = V_I4(&varOut);
                                                                                           
Cleanup:
    if (pnResult)            
        *pnResult = nResult;
    
    ReleaseInterface(pBackupUICommandHandler);  

    MemFreeString(pchCaption);

    RRETURN (S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ShowLastErrorInfo
//
//  Synopsis:   Show the current error object.
//
//--------------------------------------------------------------------------

HRESULT
CDoc::ShowLastErrorInfo(HRESULT hrError, int idsDefault)
{
    if (OK(hrError))
        return S_OK;

    HRESULT      hr;
    IErrorInfo * pErrorInfo = NULL;
    CErrorInfo * pEI;
    BSTR         bstrDescription = NULL;
    BSTR         bstrSolution = NULL;
    BSTR         bstrHelpFile = NULL;
    BSTR         bstrSource = NULL;
    TCHAR *      pchSolution = NULL;
    TCHAR *      pchDescription = NULL;
    DWORD        dwHelpContext = 0;
    TCHAR *      pch = NULL;
    TCHAR *      pchText = NULL;   

    hr = THR(::GetErrorInfo(0, &pErrorInfo));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pErrorInfo->QueryInterface(
            CLSID_CErrorInfo,
            (void **)&pEI));
    if (OK(hr))
    {
        hr = THR(pEI->GetDescriptionEx(
                &bstrDescription,
                &bstrSolution));
        if (hr)
            goto Cleanup;

        if (bstrSolution)
        {
            hr = THR(Format(FMT_OUT_ALLOC, &pchSolution, 0,
                    MAKEINTRESOURCE(IDS_ERROR_SOLUTION), bstrSolution));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        hr = THR(pErrorInfo->GetDescription(&bstrDescription));
        if (hr)
            goto Cleanup;
    }

    if (!bstrDescription)
    {
        // we got no default text, therefore we will use
        // a standard error text, passed in iIDSDefault
        // that fixes the allpage
        // propertysettings (bug6525).
        Assert(idsDefault);

        hr = THR(Format(FMT_OUT_ALLOC, &pchDescription, 0,
                    MAKEINTRESOURCE(idsDefault), bstrSolution));

        if (hr)
            goto Cleanup;
    }


    THR_NOTRACE(pErrorInfo->GetHelpFile(&bstrHelpFile));
    THR_NOTRACE(pErrorInfo->GetHelpContext(&dwHelpContext));
    THR_NOTRACE(pErrorInfo->GetSource(&bstrSource));

    // Glue header and body together for message box.

    pchText = bstrDescription? bstrDescription : pchDescription;

    if (pchSolution)
    {
        hr = THR(Format(FMT_OUT_ALLOC,
                &pch,
                0,
                TEXT("<0s>\n\n<1s>"), pchText, pchSolution));
        if (hr)
            goto Cleanup;
        pchText = pch;
    }

    hr = THR(ShowMessageEx(NULL,
            MB_ICONEXCLAMATION | MB_OK,
            bstrHelpFile,
            dwHelpContext,
            pchText));            

Cleanup:

    delete pchSolution;
    delete pchDescription;
    delete pch;

    FormsFreeString(bstrSolution);
    FormsFreeString(bstrDescription);
    FormsFreeString(bstrHelpFile);
    FormsFreeString(bstrSource);

    ReleaseInterface(pErrorInfo);

    RRETURN1(hr, S_FALSE);   
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ShowHelp
//
//  Synopsis:   Show help to the user.
//
//--------------------------------------------------------------------------

HRESULT
CDoc::ShowHelp(TCHAR * szHelpFile, DWORD dwData, UINT uCmd, POINT pt)
{
    HRESULT             hr;
    IDispatch *         pDispatch = NULL;

    if (InPlace() && InPlace()->_pHostShowUI)
    {
        hr = QueryInterface(
                IID_IDispatch,
                (void **) &pDispatch);
        if (hr)
            goto Cleanup;

        hr = InPlace()->_pHostShowUI->ShowHelp(
                _pInPlace->_hwnd,
                szHelpFile,
                uCmd,
                dwData,
                pt,
                pDispatch);

        // If host displayed help, we will not display our own.
        if (!hr)
            goto Cleanup;
    }

    if (szHelpFile)
    {
        // BUGBUG: Temporary fix for beta1 to append window style.
        // Append ">LangRef"
        _tcscat(szHelpFile, _T(">LangRef"));
        if (WinHelp(TLS(gwnd.hwndGlobalWindow), szHelpFile, uCmd, dwData))
            hr = S_OK;                        
        else
            hr = E_FAIL;               
    }
    else
        hr = E_NOTIMPL;

Cleanup:
    ReleaseInterface(pDispatch);
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Function:   InvalidateBorder
//
//  Synopsis:   Invalidates the 1 pixel border which is drawn
//              around frames
//
//---------------------------------------------------------------

static void
InvalidateBorder(CDoc *pDoc)
{
    if ((pDoc->_dwFrameOptions & FRAMEOPTIONS_NO3DBORDER) == 0 &&
        (pDoc->_dwFlagsHostInfo  & DOCHOSTUIFLAG_NO3DBORDER) == 0)
    {
        // invalidate a 4 pixel-wide area at the perimeter
        // of the rect

        long    cBorder =   CFrameSetSite::iPixelFrameHighlightWidth  +
                            CFrameSetSite::iPixelFrameHighlightBuffer + 1;

        if (cBorder > 1)
        {
            pDoc->GetView()->InvalidateBorder(cBorder);
        }
    }
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::InstallFrameUI, CServer
//
//  Synopsis:   Installs the U.I. elements on the frame window.
//              This function assumes the server has does not
//              have any UI.  Derived classes should override
//              to provide their own UI.
//
//---------------------------------------------------------------

#ifndef NO_OLEUI
HRESULT
CDoc::InstallFrameUI()
{
    HRESULT hr = S_OK;
    IOleCommandTarget * pCommandTarget = NULL;
    IOleInPlaceActiveObject * pInPlaceActiveObject = NULL;

    Assert(InPlace());
    if(_pHostUIHandler)
    {
        hr = THR(PrivateQueryInterface(IID_IOleCommandTarget, (void **) &pCommandTarget));
        if (hr)
            goto Cleanup;

        hr = THR(PrivateQueryInterface(IID_IOleInPlaceActiveObject, (void **)&pInPlaceActiveObject));
        if (hr)
            goto Cleanup;

        hr = _pHostUIHandler->ShowUI(
                _fDesignMode ? DOCHOSTUITYPE_AUTHOR : DOCHOSTUITYPE_BROWSE,
                pInPlaceActiveObject,
                pCommandTarget,
                InPlace()->_pFrame,
                InPlace()->_pDoc);

        InPlace()->_fHostShowUI = hr != S_FALSE;
        if (hr == S_FALSE)
            hr = S_OK;
    }
    else
    {
        InPlace()->_fHostShowUI = FALSE;
    }

#if DBG == 1
    if (DbgExIsFullDebug())
    {
        if (!InPlace()->_fHostShowUI)
        {
            hr = THR(InstallMenuUI());
            if (hr)
                goto Cleanup;

            DeferUpdateUI();
            InvalidateBorder(this);
        }
    }
#endif // DBG == 1

Cleanup:
    ReleaseInterface(pCommandTarget);
    ReleaseInterface(pInPlaceActiveObject);
    RRETURN(hr);
}
#endif // NO_OLEUI

//+---------------------------------------------------------------
//
//  Member:     CDoc::RemoveFrameUI, CServer
//
//  Synopsis:   Removes the U.I. elements on the frame window
//
//  Notes:      This method "undoes" everything that was done in
//              InstallFrameUI -- it removes the shared menu from
//              the frame.
//
//---------------------------------------------------------------

#ifndef NO_OLEUI
void
CDoc::RemoveFrameUI()
{
    if (!InPlace()->_fHostShowUI)
    {
#if DBG == 1
        if (DbgExIsFullDebug())
        {
            super::RemoveFrameUI();

            InPlace()->_fMenusMerged = 0;

            DestroyMenuUI();    // Must clear menus cached in the doc's inplace
                                // object.
        }
#endif // DBG == 1
    }
    else
    {
        Assert(_pHostUIHandler);
        _pHostUIHandler->HideUI();
        InPlace()->_fHostShowUI = FALSE;
    }

    InvalidateBorder(this);
}
#endif // NO_OLEUI

//+-------------------------------------------------------------------
//
//  Member:     CDoc::DetachWin
//
//  Synopsis:   Our window is going down. Cleanup our UI.
//
//--------------------------------------------------------------------

void
CDoc::DetachWin()
{
    THREADSTATE *   pts;

#if DBG == 1
    if (DbgExIsFullDebug())
    {
        DestroyMenuUI();
    }
#endif // DBG == 1

#ifndef NO_IME
    // if a DIMM is installed, shut down it's ui
    DeactivateDIMM();
#endif // ndef NO_IME

    pts = GetThreadState();

    if (_pInPlace->_hwnd)
    {
        LRESULT lResult;
        
        // A bug in Win9x sometimes causes a stack overflow fault if our inplace window has
        // the focus and set attempt to hide the window (or SetParent).  By throwing the
        // focus away from our window, we work around the bug in the OS.

        if (_fInHTMLPropPage && ::GetFocus() == _pInPlace->_hwnd)
            ::SetFocus(NULL);

        ShowWindow (_pInPlace->_hwnd, SW_HIDE);
        OnWindowMessage (WM_KILLFOCUS, 0, 0, &lResult);
        if (SetParent (_pInPlace->_hwnd, pts->gwnd.hwndGlobalWindow))
        {
            // BUGBUG: can killing of all timers be done better?
            KillTimer (_pInPlace->_hwnd, 1);                    // mouse move timer
            KillTimer (_pInPlace->_hwnd, TIMER_DEFERUPDATEUI);  // defer update UI timer
            _fUpdateUIPending = FALSE;                          // clear update UI pending flag
            _fNeedUpdateUI = FALSE;
            _fNeedUpdateTitle = FALSE;
            SetWindowLongPtr(_pInPlace->_hwnd, GWLP_USERDATA, 0);  // disconnect the window from this CDoc
            OnDestroy();    // this call is necessary to balance all ref-counting and init-/deinitialization;
                            // normally in CServer this happens upon WM_DESTROY message; however, as we reuse
                            // the window, we don't get WM_DESTROY in this codepath so we explicitly call OnDestroy

            PrivateRelease();
            _hwndCached = _pInPlace->_hwnd;
            _pInPlace->_hwnd = NULL;
        }
        else // SetParent failed
        {
            super::DetachWin();
        }
    }
}


HRESULT
CDoc::HostTranslateAccelerator ( LPMSG lpmsg )
{
    HRESULT     hr = S_FALSE;

    // Give host a change to handle first
    if(_pHostUIHandler)
    {
        DWORD cmdID;
        const GUID *pcmdSetGuid = &CGID_MSHTML;

        Assert(_pElemCurrent);

        if (lpmsg->message < WM_KEYFIRST || lpmsg->message > WM_KEYLAST)
        {
            cmdID = 0;
            pcmdSetGuid = NULL;
        }
        else
            cmdID = _pElemCurrent->GetCommandID(lpmsg);

        hr = THR(_pHostUIHandler->TranslateAccelerator(
                    lpmsg,
                    pcmdSetGuid,
                    cmdID));
    }
    RRETURN1(hr, S_FALSE);
}


HRESULT
CDoc::TranslateAccelerator ( LPMSG lpmsg )
{
    HRESULT hr = S_FALSE;    
    
    //  Give tooltips a chance to dismiss.
    {
        // Ignore spurious WM_ERASEBACKGROUNDs generated by tooltips
        CServer::CLock Lock(this, SERVERLOCK_IGNOREERASEBKGND);

        FormsTooltipMessage(lpmsg->message, lpmsg->wParam, lpmsg->lParam);
    }

    // No_File_Menu restriction enabled   
    if (g_fNoFileMenu)
    {             
        DWORD nCmdID;

        Assert(_pElemCurrent);
        nCmdID = _pElemCurrent->GetCommandID(lpmsg);
        if (nCmdID==IDM_OPEN || nCmdID==IDM_SAVE || nCmdID==IDM_PRINT)
        {
            hr = S_OK;
            goto Cleanup;
        }
    }

    hr = THR(HostTranslateAccelerator(lpmsg));
    if (hr == S_OK)
        goto Cleanup;

    if (lpmsg->message < WM_KEYFIRST || lpmsg->message > WM_KEYLAST)
        return S_FALSE;

    hr = THR(super::TranslateAccelerator (lpmsg));

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
// Member:   CDoc::SetDataObjectSecurity
//
// Synopsis: Set Security Domain
//
//----------------------------------------------------------------------------
HRESULT
CDoc::SetDataObjectSecurity(IDataObject * pDataObj)
{
    HRESULT      hr;
    CVariant     var(VT_BSTR);
    BYTE         abSID[MAX_SIZE_SECURITY_ID];
    DWORD        cbSID = ARRAY_SIZE(abSID);

    memset(abSID, 0, cbSID);
    hr = THR(GetSecurityID(abSID, &cbSID));
    if (hr)
        goto Cleanup;

    hr = FormsAllocStringLen(NULL, MAX_SIZE_SECURITY_ID, &V_BSTR(&var));
    if (hr)
        goto Cleanup;

    memcpy(V_BSTR(&var), abSID, MAX_SIZE_SECURITY_ID);
    IGNORE_HR(CTExec(
            pDataObj,
            &CGID_DATAOBJECTEXEC,
            IDM_SETSECURITYDOMAIN,
            0,
            &var,
            NULL));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// Member:   CDoc::CheckDataObjectSecurity
//
// Synopsis: Check Security Domain
//
//----------------------------------------------------------------------------
HRESULT
CDoc::CheckDataObjectSecurity(IDataObject * pDataObj)
{
    HRESULT     hr;
    CVariant    var(VT_BSTR);
    BYTE        abSID[MAX_SIZE_SECURITY_ID];
    DWORD       cbSID = ARRAY_SIZE(abSID);

    memset(abSID, 0, cbSID);

    hr = THR(GetSecurityID(abSID, &cbSID));
    if (hr)
        goto Cleanup;

    hr = FormsAllocStringLen(NULL, MAX_SIZE_SECURITY_ID, &V_BSTR(&var));
    if (hr)
        goto Cleanup;

    memcpy(V_BSTR(&var), abSID, MAX_SIZE_SECURITY_ID);
    hr = THR_NOTRACE(CTExec(
            pDataObj,
            &CGID_DATAOBJECTEXEC,
            IDM_CHECKSECURITYDOMAIN,
            0,
            &var,
            NULL));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// Member:   CDoc::SetClipboard
//
// Synopsis: Set Security Domain before actual FormSetClipboard
//
//----------------------------------------------------------------------------
HRESULT
CDoc::SetClipboard(IDataObject * pDO)
{
    IGNORE_HR(SetDataObjectSecurity(pDO));

    RRETURN(FormSetClipboard(pDO));
}

//+---------------------------------------------------------------------------
//
// Member:   CDoc::AllowPaste
//
// Synopsis: Check if Paste is allowed to be executed.
//           Trident allows Paste execution if Paste is from
//           1) UI (menu command, toolbar, ...)
//           2) Script, after user confirms
//
//-----------------------------------------------------------------------------
HRESULT
CDoc::AllowPaste(IDataObject * pDO)
{
    HRESULT hr = S_OK;

    if (IsInScript())
    {
        hr = CheckDataObjectSecurity(pDO);

        if (hr)
            goto Cleanup;
#if 0
        int fYesNo;

        hr = ShowMessageEx(
                        &fYesNo,
                        MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2,
                        NULL,
                        0,
                        _T("Scripting code is trying to PASTE data from clipboard to one of the controls, the data is from the same security domain (that is, from the same server).\n\n Do you allow this to happen?"));
        hr = (fYesNo == IDYES) ? (S_OK) : (OLECMDERR_E_DISABLED);
#endif
    }

Cleanup:
    if (hr)
    {
        // cannot programatic paste accross application or security
        // domain.
        //
        hr = OLECMDERR_E_DISABLED;
    }
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
// Member: CheckDoc3DBorder
//
// Synopsis: Based on which 3D border edges are needed for this CDoc,
//           determine which extra 3D border edges are needed for pDoc, which
//           should be a child CDoc for this CDoc.
//           the function is called in CBodyElement::GetBorderInfo() as
//               Doc()->_pDocParent->CheckDoc3DBorder(Doc());
//----------------------------------------------------------------------------
void
CDoc::CheckDoc3DBorder(CDoc * pDoc)
{
    Assert(pDoc->_pDocParent == this);

    CElement * pElement = GetPrimaryElementClient();

    if (!pElement)
        return;
    
    if (pElement->Tag() == ETAG_BODY)
    {
        // If _pElemClient is a CBodyElement, pDoc is a CDoc defined inside
        // one <iframe> tag, need all 3D borders.
        //
        pDoc->_b3DBorder = NEED3DBORDER_TOP | NEED3DBORDER_LEFT
                         | NEED3DBORDER_BOTTOM | NEED3DBORDER_RIGHT;
    }
    else
    {
        // Since the request is from inner CDoc, _pElemClient must be frameset.
        //
        Assert(pElement->Tag() == ETAG_FRAMESET);
        CFrameSetSite * pFrameSet =
            DYNCAST( CFrameSetSite, pElement );

        // determine which 3D border edges are needed for this CDoc.
        // If this is the root CDoc (_pDocParent == NULL), we begin with that
        // we do not need to draw any 3D border edges for potential
        // CBodyElement in pDoc, since _pElemClient (top level CFrameSetSite)
        // will draw the 3D border for us.
        // Otherwise, let _pDocParent decide which 3D border edges this CDoc
        // need to draw (assume that there is a CBodyElement inside this).
        //
        if (_pDocParent)
        {
            _pDocParent->CheckDoc3DBorder(this);
        }
        else
        {
            // This must be the top-level frameset case, assume no 3D border at
            // first since top-level frameset (_pElemClient) already draw the
            // outmost 3D borders.
            //
            _b3DBorder = NEED3DBORDER_NO;
        }

        // calculate which extra 3D border edges are needed for pDoc
        //
        if (_b3DBorder == (NEED3DBORDER_TOP | NEED3DBORDER_LEFT
                                | NEED3DBORDER_BOTTOM | NEED3DBORDER_RIGHT))
        {
            // If this CDoc already needs all 3D border edges, child CDoc
            // should need also.
            //
            pDoc->_b3DBorder = NEED3DBORDER_TOP | NEED3DBORDER_LEFT
                             | NEED3DBORDER_BOTTOM | NEED3DBORDER_RIGHT;
        }
        else
        {
            // let CFrameSetSite decides whether more 3D border edges are
            // needed.
            //
            pFrameSet->Layout()->CheckFrame3DBorder(pDoc, _b3DBorder);
        }
    }
}
