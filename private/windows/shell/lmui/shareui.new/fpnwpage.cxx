//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       fpnwpage.cxx
//
//  Contents:   "FPNW" shell property page extension
//
//  History:    9-Mar-96        BruceFo     Created from SMB share page
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "helpids.h"
#include "dlgnew.hxx"
#include "acl.hxx"
#include "shrinfo.hxx"
#include "fpnwpage.hxx"
#include "util.hxx"

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::DlgProcPage, static public
//
//  Synopsis:   Dialog Procedure for all CFpnwPropertyPage
//
//--------------------------------------------------------------------------

BOOL CALLBACK
CFpnwPropertyPage::DlgProcPage(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CFpnwPropertyPage* pThis = NULL;

    if (msg==WM_INITDIALOG)
    {
        SHARE_PROPSHEETPAGE* sprop = (SHARE_PROPSHEETPAGE*)lParam;
        pThis = new CFpnwPropertyPage(hwnd, sprop->pszMachine, sprop->pszShareName);
        if (NULL != pThis)
        {
            if (FAILED(pThis->InitInstance()))
            {
                delete pThis;
                pThis = NULL;
            }
        }

        SetWindowLong(hwnd,GWL_USERDATA,(LPARAM)pThis);
    }
    else
    {
        pThis = (CFpnwPropertyPage*) GetWindowLong(hwnd,GWL_USERDATA);
    }

    if (pThis != NULL)
    {
        return pThis->_PageProc(hwnd,msg,wParam,lParam);
    }
    else
    {
        return FALSE;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CFpnwPropertyPage::SizeWndProc, public
//
//  Synopsis:   "allow" edit window subclass proc to disallow non-numeric
//              characters.
//
//  History:    5-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

LRESULT CALLBACK
CFpnwPropertyPage::SizeWndProc(
    IN HWND hwnd,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (wMsg)
    {
    case WM_CHAR:
    {
        WCHAR chCharCode = (WCHAR)wParam;
        if (   (chCharCode == TEXT('\t'))
            || (chCharCode == TEXT('\b'))
            || (chCharCode == TEXT('\n'))
//          || (chCharCode == TEXT('\x1b')) // ESCAPE key
            )
        {
            break;
        }

        if (chCharCode < TEXT('0') || chCharCode > TEXT('9'))
        {
            // bad key: ignore it
            MessageBeep(0xffffffff);    // let user know it's an illegal char
            return FALSE;
        }

        break;
    }
    } // end of switch

    CFpnwPropertyPage* pThis = (CFpnwPropertyPage*)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
    appAssert(NULL != pThis);
    appAssert(NULL != pThis->_pfnAllowProc);
    return CallWindowProc(pThis->_pfnAllowProc, hwnd, wMsg, wParam, lParam);
}


//+--------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::CFpnwPropertyPage, public
//
//  Synopsis:   Constructor
//
//---------------------------------------------------------------------------

CFpnwPropertyPage::CFpnwPropertyPage(
    IN HWND hwndPage,
    IN PWSTR pszMachine,
    IN PWSTR pszShare
    )
    :
    _hwndPage(hwndPage),
    _pszMachine(pszMachine),
    _pszShare(pszShare),
    _bDirty(FALSE),
    _bShareNameChanged(FALSE),
    _bUserLimitChanged(FALSE),
    _bSecDescChanged(FALSE),
    _wMaxUsers(DEFAULT_MAX_USERS),
    _pCurInfo(NULL),
    _pfnAllowProc(NULL)
{
    INIT_SIG(CFpnwPropertyPage);
    appAssert(NULL != _pszShare);
}


//+--------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::~CFpnwPropertyPage, public
//
//  Synopsis:   Destructor
//
//---------------------------------------------------------------------------

CFpnwPropertyPage::~CFpnwPropertyPage()
{
    CHECK_SIG(CFpnwPropertyPage);

    delete _pCurInfo;
    _pCurInfo = NULL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::InitInstance, public
//
//  Synopsis:   Part II of the constuctor process
//
//  Notes:      We don't want to handle any errors in constuctor, so this
//              method is necessary for the second phase error detection.
//
//--------------------------------------------------------------------------

HRESULT
CFpnwPropertyPage::InitInstance(
    VOID
    )
{
    CHECK_SIG(CFpnwPropertyPage);
    appDebugOut((DEB_ITRACE, "CFpnwPropertyPage::InitInstance\n"));

    SHARE_INFO_502* info502;
    NET_API_STATUS ret = NetShareGetInfo(_pszMachine, _pszShare, 502, (LPBYTE*)&info502);
    if (ret == NERR_Success)
    {
        _pCurInfo = new CShareInfo(info502);
        if (NULL == _pCurInfo)
        {
            return E_OUTOFMEMORY;
        }

        if (!_pCurInfo->TakeOwn())
        {
            return E_OUTOFMEMORY;
        }

        NetApiBufferFree(info502);
    }
    else
    {
        appDebugOut((DEB_ERROR, "Couldn't get info\n"));
        return HRESULT_FROM_WIN32(ret);
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_PageProc, private
//
//  Synopsis:   Dialog Procedure for this object
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_PageProc(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    static DWORD aHelpIds[] =
    {
        IDC_SHARE_MAXIMUM,          HC_SHARE_MAXIMUM,
        IDC_SHARE_ALLOW,            HC_SHARE_ALLOW,
        IDC_SHARE_ALLOW_VALUE,      HC_SHARE_ALLOW_VALUE,
        IDC_SHARE_PERMISSIONS,      HC_SHARE_PERMISSIONS,

        0,0
    };

    switch (msg)
    {
    case WM_INITDIALOG:
        return _OnInitDialog(hwnd, (HWND)wParam, lParam);

    case WM_COMMAND:
        return _OnCommand(hwnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);

    case WM_NOTIFY:
        return _OnNotify(hwnd, (int)wParam, (LPNMHDR)lParam);

    case WM_VSCROLL:
        // The up/down control changed the edit control: select it again
        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);
        return TRUE;

    case WM_HELP:
    {
        LPHELPINFO lphi = (LPHELPINFO)lParam;

        if (lphi->iContextType == HELPINFO_WINDOW)  // a control
        {
            WCHAR szHelp[50];
            LoadString(g_hInstance, IDS_HELPFILENAME, szHelp, ARRAYLEN(szHelp));
            WinHelp(
                (HWND)lphi->hItemHandle,
                szHelp,
                HELP_WM_HELP,
                (DWORD)(LPVOID)aHelpIds);
        }
        break;
    }

    case WM_CONTEXTMENU:
    {
        WCHAR szHelp[50];
        LoadString(g_hInstance, IDS_HELPFILENAME, szHelp, ARRAYLEN(szHelp));
        WinHelp(
            (HWND)wParam,
            szHelp,
            HELP_CONTEXTMENU,
            (DWORD)(LPVOID)aHelpIds);
        break;
    }

    case WM_CLOSE:
        // BUGBUG: There is a bug where hitting "ESCAPE" with the focus on the
        // MLE for the "allow" text doesn't kill the property sheet unless we
        // forward the WM_CLOSE message on to the property sheet root dialog.
        return SendMessage(GetParent(hwnd), msg, wParam, lParam);

    case WM_DESTROY:
        // restore original subclass to window.
        appAssert(NULL != GetDlgItem(hwnd,IDC_SHARE_ALLOW_VALUE));
        SetWindowLong(GetDlgItem(hwnd,IDC_SHARE_ALLOW_VALUE), GWL_WNDPROC, (LONG)_pfnAllowProc);
        break;

    case WM_NCDESTROY:
        return _OnNcDestroy(hwnd);

    } // end switch (msg)

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_OnInitDialog, private
//
//  Synopsis:   WM_INITDIALOG handler
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_OnInitDialog(
    IN HWND hwnd,
    IN HWND hwndFocus,
    IN LPARAM lInitParam
    )
{
    CHECK_SIG(CFpnwPropertyPage);
    appDebugOut((DEB_ITRACE, "_OnInitDialog\n"));

    SetDialogIconBig(_GetFrameWindow(), IDI_SHARESFLD);

    // Subclass allow edit control to disallow non-positive numbers
    _pfnAllowProc = (WNDPROC)SetWindowLong(
                                    GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE),
                                    GWL_WNDPROC,
                                    (LONG)&SizeWndProc);

    // use LanMan API constants to set maximum share name & comment lengths
    SendDlgItemMessage(hwnd, IDC_SHARE_SHARENAME, EM_LIMITTEXT, NNLEN, 0L);

    _InitializeControls(hwnd);

// #if DBG == 1
//  Dump(L"_OnInitDialog finished");
// #endif // DBG == 1

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_OnCommand, private
//
//  Synopsis:   WM_COMMAND handler
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_OnCommand(
    IN HWND hwnd,
    IN WORD wNotifyCode,
    IN WORD wID,
    IN HWND hwndCtl
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    switch (wID)
    {

//
// Notifications
//

    case IDC_SHARE_MAXIMUM:
        if (BN_CLICKED == wNotifyCode)
        {
            // Take away WS_TABSTOP from the "allow users" edit control
            HWND hwndEdit = GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE);
            SetWindowLong(hwndEdit, GWL_STYLE, GetWindowLong(hwndEdit, GWL_STYLE) & ~WS_TABSTOP);

            _CacheMaxUses(hwnd);
            SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");

            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }
        return TRUE;

    case IDC_SHARE_ALLOW:
        if (BN_CLICKED == wNotifyCode)
        {
            // Give WS_TABSTOP to the "allow users" edit control
            HWND hwndEdit = GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE);
            SetWindowLong(hwndEdit, GWL_STYLE, GetWindowLong(hwndEdit, GWL_STYLE) | WS_TABSTOP);

            // let the spin control set the edit control
            SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_SETPOS, 0, MAKELONG(_wMaxUsers, 0));
            SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);

            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }
        return TRUE;

    case IDC_SHARE_ALLOW_VALUE:
    {
        if (EN_CHANGE == wNotifyCode)
        {
            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }

        if (EN_SETFOCUS == wNotifyCode)
        {
            if (1 != IsDlgButtonChecked(hwnd, IDC_SHARE_ALLOW))
            {
                CheckRadioButton(
                    hwnd,
                    IDC_SHARE_MAXIMUM,
                    IDC_SHARE_ALLOW,
                    IDC_SHARE_ALLOW);
            }

            // let the spin control set the edit control
            SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_SETPOS, 0, MAKELONG(_wMaxUsers, 0));
            SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);

            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }

        if (EN_KILLFOCUS == wNotifyCode)
        {
            _CacheMaxUses(hwnd);
        }

        return TRUE;
    }

    case IDC_SHARE_ALLOW_SPIN:
        if (UDN_DELTAPOS == wNotifyCode)
        {
            if (1 != IsDlgButtonChecked(hwnd, IDC_SHARE_ALLOW))
            {
                CheckRadioButton(
                    hwnd,
                    IDC_SHARE_MAXIMUM,
                    IDC_SHARE_ALLOW,
                    IDC_SHARE_ALLOW);
            }

            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }
        return TRUE;

//
// Commands
//

    case IDC_SHARE_PERMISSIONS:
        return _OnPermissions(hwnd);

    default:
        break;
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_OnNotify, private
//
//  Synopsis:   WM_NOTIFY handler
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_OnNotify(
    IN HWND hwnd,
    IN int idCtrl,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    // assume a property sheet notification
    return _OnPropertySheetNotify(hwnd, phdr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_OnPropertySheetNotify, private
//
//  Synopsis:   WM_NOTIFY handler for the property sheet notification
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_OnPropertySheetNotify(
    IN HWND hwnd,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    switch (phdr->code)
    {
    case PSN_RESET:         // cancel
        if (_DoCancel(hwnd))
        {
            SetWindowLong(hwnd, DWL_MSGRESULT, FALSE); // go away
        }
        else
        {
            SetWindowLong(hwnd, DWL_MSGRESULT, TRUE);
        }
        return TRUE;

    case PSN_KILLACTIVE:    // change to another page
        if (_ValidatePage(hwnd))
        {
            SetWindowLong(hwnd, DWL_MSGRESULT, PSNRET_NOERROR);
            return FALSE;
        }
        else
        {
            SetWindowLong(hwnd, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            return TRUE;
        }

    case PSN_APPLY:
        if (_DoApply(hwnd))
        {
            SetWindowLong(hwnd, DWL_MSGRESULT, FALSE); // go away
        }
        else
        {
            SetWindowLong(hwnd, DWL_MSGRESULT, TRUE);
        }
        return TRUE;

    } // end switch (phdr->code)

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_OnNcDestroy, private
//
//  Synopsis:   WM_NCDESTROY handler
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_OnNcDestroy(
    IN HWND hwnd
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    delete this;    // do this LAST!
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_OnPermissions, private
//
//  Synopsis:   WM_COMMAND handler: the permissions button
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_OnPermissions(
    IN HWND hwnd
    )
{
    CHECK_SIG(CFpnwPropertyPage);
    appAssert(NULL != _pCurInfo);

    if (STYPE_SPECIAL & _pCurInfo->GetType())
    {
        MyErrorDialog(hwnd, IERR_AdminShare);
        return TRUE;
    }

    WCHAR szShareName[NNLEN + 1];
    GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));

    PSECURITY_DESCRIPTOR pNewSecDesc = NULL;
    PSECURITY_DESCRIPTOR pSecDesc = _pCurInfo->GetSecurityDescriptor();
    appAssert(NULL == pSecDesc || IsValidSecurityDescriptor(pSecDesc));

    BOOL bSecDescChanged;
    LONG err = EditShareAcl(
                        hwnd,
                        _pszMachine,
                        szShareName,
                        pSecDesc,
                        &bSecDescChanged,
                        &pNewSecDesc);

    if (bSecDescChanged)
    {
        _bSecDescChanged = TRUE;

        appAssert(IsValidSecurityDescriptor(pNewSecDesc));
        _pCurInfo->TransferSecurityDescriptor(pNewSecDesc);
        _MarkItemDirty();
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_InitializeControls, private
//
//  Synopsis:   Initialize the controls from scratch
//
//--------------------------------------------------------------------------

VOID
CFpnwPropertyPage::_InitializeControls(
    IN HWND hwnd
    )
{
    _SetControlsToDefaults(hwnd);
    _SetControlsFromData(hwnd);
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_SetControlsToDefaults, private
//
//  Synopsis:   Set all the controls on the page to their default values
//
//--------------------------------------------------------------------------

VOID
CFpnwPropertyPage::_SetControlsToDefaults(
    IN HWND hwnd
    )
{
    _fInitializingPage = TRUE;

    // Make "Maximum" the default number of users, and clear the value field
    // (which the spin button set on creation?).

    CheckRadioButton(
            hwnd,
            IDC_SHARE_MAXIMUM,
            IDC_SHARE_ALLOW,
            IDC_SHARE_MAXIMUM);

    SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");

    // set the spin control range: 1 <--> large number
    SendDlgItemMessage(
            hwnd,
            IDC_SHARE_ALLOW_SPIN,
            UDM_SETRANGE,
            0,
            MAKELONG(g_uiMaxUsers, 1));

    SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");

    _fInitializingPage = FALSE;
}



//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::SetControlsFromData, private
//
//  Synopsis:   From the class variables and current state of the radio
//              buttons, set the enabled/disabled state of the buttons, as
//              well as filling the controls with the appropriate values.
//
//--------------------------------------------------------------------------

VOID
CFpnwPropertyPage::_SetControlsFromData(
    IN HWND hwnd
    )
{
    appAssert(NULL != _pCurInfo);

    _fInitializingPage = TRUE;

    DWORD dwLimit = _pCurInfo->GetMaxUses();
    if (dwLimit == SHI_USES_UNLIMITED)
    {
        _wMaxUsers = DEFAULT_MAX_USERS;

        appDebugOut((DEB_ITRACE, "_SetControlsFromData: unlimited users\n"));

        CheckRadioButton(
                hwnd,
                IDC_SHARE_MAXIMUM,
                IDC_SHARE_ALLOW,
                IDC_SHARE_MAXIMUM);

        SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");
    }
    else
    {
        _wMaxUsers = (WORD)dwLimit;

        appDebugOut((DEB_ITRACE,
            "_SetControlsFromData: max users = %d\n",
            _wMaxUsers));

        CheckRadioButton(
                hwnd,
                IDC_SHARE_MAXIMUM,
                IDC_SHARE_ALLOW,
                IDC_SHARE_ALLOW);

        // let the spin control set the edit control
        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_SETPOS, 0, MAKELONG(_wMaxUsers, 0));

        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);
    }

    _fInitializingPage = FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_MarkItemDirty, private
//
//  Synopsis:   A change has made such that the current item (and page)
//              is now dirty
//
//--------------------------------------------------------------------------

VOID
CFpnwPropertyPage::_MarkItemDirty(
    VOID
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    if (!_fInitializingPage)
    {
        if (!_bDirty)
        {
            appDebugOut((DEB_ITRACE, "Marking Sharing page dirty\n"));
            _bDirty = TRUE;
            PropSheet_Changed(_GetFrameWindow(),_hwndPage);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_ValidatePage, private
//
//  Synopsis:   Return TRUE if the current page is valid
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_ValidatePage(
    IN HWND hwnd
    )
{
    CHECK_SIG(CFpnwPropertyPage);
    appAssert(NULL != _pCurInfo);

    if (!_bDirty)
    {
        // nothing to validate
        return TRUE;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_DoApply, public
//
//  Synopsis:   If anything has changed, apply the data
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_DoApply(
    IN HWND hwnd
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    if (_bDirty)
    {
        appAssert(NULL != _pCurInfo);

        NET_API_STATUS ret;
        HRESULT hr;

        if (_bUserLimitChanged)
        {
            if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_MAXIMUM))
            {
                _pCurInfo->SetMaxUses(SHI_USES_UNLIMITED);
            }
            else if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_ALLOW))
            {
                _CacheMaxUses(hwnd);
                _pCurInfo->SetMaxUses(_wMaxUsers);
            }
        }

        //
        // Commit the changes!
        //

        ret = _pCurInfo->Commit(_pszMachine);
        if (ret != NERR_Success)
        {
            HRESULT hrMsg = 0;
            switch (_pCurInfo->GetFlag())
            {
            case SHARE_FLAG_ADDED:  hrMsg = IERR_CANT_ADD_SHARE;    break;
            case SHARE_FLAG_MODIFY: hrMsg = IERR_CANT_MODIFY_SHARE; break;
            default:
                appAssert(!"Illegal flag for a failed commit!");
            }
            DisplayError(hwnd, hrMsg, ret, _pCurInfo->GetNetname());
        }
        else
        {
            _pCurInfo->SetFlag(0);  // clear flag on success
        }

        _bDirty = FALSE;
        _bUserLimitChanged = FALSE;
        _bSecDescChanged = FALSE;
        PropSheet_UnChanged(_GetFrameWindow(),_hwndPage);

        _InitializeControls(hwnd);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_DoCancel, public
//
//  Synopsis:   Do whatever is necessary to cancel the changes
//
//--------------------------------------------------------------------------

BOOL
CFpnwPropertyPage::_DoCancel(
    IN HWND hwnd
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    if (_bDirty)
    {
        _bDirty = FALSE;
        _bUserLimitChanged = FALSE;
        _bSecDescChanged = FALSE;
        PropSheet_UnChanged(_GetFrameWindow(),_hwndPage);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::_CacheMaxUses, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------

VOID
CFpnwPropertyPage::_CacheMaxUses(
    IN HWND hwnd
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    DWORD dwRet = SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_GETPOS, 0, 0);
    if (HIWORD(dwRet) != 0)
    {
        _wMaxUsers = DEFAULT_MAX_USERS;

        // Reset the edit control to the new value
        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_SETPOS, 0, MAKELONG(_wMaxUsers, 0));
        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);
    }
    else
    {
        _wMaxUsers = LOWORD(dwRet);
    }
}


#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Method:     CFpnwPropertyPage::Dump, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------
VOID
CFpnwPropertyPage::Dump(
    IN PWSTR pszCaption
    )
{
    CHECK_SIG(CFpnwPropertyPage);

    appDebugOut((DEB_TRACE,
        "CFpnwPropertyPage::Dump, %ws\n",
        pszCaption));

    appDebugOut((DEB_TRACE | DEB_NOCOMPNAME,
"\t            This: 0x%08lx\n"
"\t            Page: 0x%08lx\n"
"\t   Initializing?: %ws\n"
"\t          Dirty?: %ws\n"
"\tUsr Lim changed?: %ws\n"
"\t        Max uses: %d\n"
"\t       _pCurInfo: 0x%08lx\n"
,
this,
_hwndPage,
_fInitializingPage ? L"yes" : L"no",
_bDirty            ? L"yes" : L"no",
_bUserLimitChanged ? L"yes" : L"no",
_wMaxUsers,
_pCurInfo
));

    _pCurInfo->Dump(L"Current");
}

#endif // DBG == 1
