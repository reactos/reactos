//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       sfmpage.cxx
//
//  Contents:   "SFM" shell property page extension
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
#include "sfmpage.hxx"
#include "util.hxx"

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::DlgProcPage, static public
//
//  Synopsis:   Dialog Procedure for all CSfmPropertyPage
//
//--------------------------------------------------------------------------

BOOL CALLBACK
CSfmPropertyPage::DlgProcPage(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CSfmPropertyPage* pThis = NULL;

    if (msg==WM_INITDIALOG)
    {
        SHARE_PROPSHEETPAGE* sprop = (SHARE_PROPSHEETPAGE*)lParam;
        pThis = new CSfmPropertyPage(hwnd, sprop->pszMachine, sprop->pszShareName);
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
        pThis = (CSfmPropertyPage*) GetWindowLong(hwnd,GWL_USERDATA);
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
//  Member:     CSfmPropertyPage::SizeWndProc, public
//
//  Synopsis:   "allow" edit window subclass proc to disallow non-numeric
//              characters.
//
//  History:    5-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

LRESULT CALLBACK
CSfmPropertyPage::SizeWndProc(
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

    CSfmPropertyPage* pThis = (CSfmPropertyPage*)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
    appAssert(NULL != pThis);
    appAssert(NULL != pThis->_pfnAllowProc);
    return CallWindowProc(pThis->_pfnAllowProc, hwnd, wMsg, wParam, lParam);
}


//+--------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::CSfmPropertyPage, public
//
//  Synopsis:   Constructor
//
//---------------------------------------------------------------------------

CSfmPropertyPage::CSfmPropertyPage(
    IN HWND hwndPage,
    IN PWSTR pszMachine,
    IN PWSTR pszShare
    )
    :
    _hwndPage(hwndPage),
    _pszMachine(pszMachine),
    _pszShare(pszShare),
    _bDirty(FALSE),
    _bPasswordChanged(FALSE),
    _bPasswordConfirmChanged(FALSE),
    _bUserLimitChanged(FALSE),
    _wMaxUsers(DEFAULT_MAX_USERS),
    _pCurInfo(NULL),
    _pszReplacePath(NULL),
    _pfnAllowProc(NULL)
{
    INIT_SIG(CSfmPropertyPage);
    appAssert(NULL != _pszShare);
}


//+--------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::~CSfmPropertyPage, public
//
//  Synopsis:   Destructor
//
//---------------------------------------------------------------------------

CSfmPropertyPage::~CSfmPropertyPage()
{
    CHECK_SIG(CSfmPropertyPage);

    delete _pCurInfo;
    _pCurInfo = NULL;

    delete[] _pszReplacePath;
    _pszReplacePath = NULL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::InitInstance, public
//
//  Synopsis:   Part II of the constuctor process
//
//  Notes:      We don't want to handle any errors in constuctor, so this
//              method is necessary for the second phase error detection.
//
//--------------------------------------------------------------------------

HRESULT
CSfmPropertyPage::InitInstance(
    VOID
    )
{
    CHECK_SIG(CSfmPropertyPage);
    appDebugOut((DEB_ITRACE, "CSfmPropertyPage::InitInstance\n"));

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
//  Method:     CSfmPropertyPage::_PageProc, private
//
//  Synopsis:   Dialog Procedure for this object
//
//--------------------------------------------------------------------------

BOOL
CSfmPropertyPage::_PageProc(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CHECK_SIG(CSfmPropertyPage);

    static DWORD aHelpIds[] =
    {
        IDC_SFM_PASSWORD,           HC_SHARE_PATH,		// BUGBUG
        IDC_SFM_PASSWORD_CONFIRM,   HC_SHARE_COMMENT,	// BUGBUG
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
//  Method:     CSfmPropertyPage::_OnInitDialog, private
//
//  Synopsis:   WM_INITDIALOG handler
//
//--------------------------------------------------------------------------

BOOL
CSfmPropertyPage::_OnInitDialog(
    IN HWND hwnd,
    IN HWND hwndFocus,
    IN LPARAM lInitParam
    )
{
    CHECK_SIG(CSfmPropertyPage);
    appDebugOut((DEB_ITRACE, "_OnInitDialog\n"));

    SetDialogIconBig(_GetFrameWindow(), IDI_SHARESFLD);

    // Subclass allow edit control to disallow non-positive numbers
    _pfnAllowProc = (WNDPROC)SetWindowLong(
                                    GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE),
                                    GWL_WNDPROC,
                                    (LONG)&SizeWndProc);

    // set maximum text lengths
    SendDlgItemMessage(hwnd, IDC_SFM_PASSWORD, EM_LIMITTEXT, AFP_VOLPASS_LEN, 0L);
    SendDlgItemMessage(hwnd, IDC_SFM_PASSWORD_CONFIRM, EM_LIMITTEXT, AFP_VOLPASS_LEN, 0L);

    _InitializeControls(hwnd);

// #if DBG == 1
//  Dump(L"_OnInitDialog finished");
// #endif // DBG == 1

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::_OnCommand, private
//
//  Synopsis:   WM_COMMAND handler
//
//--------------------------------------------------------------------------

BOOL
CSfmPropertyPage::_OnCommand(
    IN HWND hwnd,
    IN WORD wNotifyCode,
    IN WORD wID,
    IN HWND hwndCtl
    )
{
    CHECK_SIG(CSfmPropertyPage);

    switch (wID)
    {

//
// Notifications
//

    case IDC_SFM_PASSWORD:
    {
        if (EN_CHANGE == wNotifyCode)
        {
            if (!_fInitializingPage)
            {
                _bPasswordChanged = TRUE;
                _MarkItemDirty();
            }
        }
        return TRUE;
    }

    case IDC_SFM_PASSWORD_CONFIRM:
    {
        if (EN_CHANGE == wNotifyCode)
        {
            if (!_fInitializingPage)
            {
                _bPasswordConfirmChanged = TRUE;
                _MarkItemDirty();
            }
        }
        return TRUE;
    }

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

    default:
        break;
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::_OnNotify, private
//
//  Synopsis:   WM_NOTIFY handler
//
//--------------------------------------------------------------------------

BOOL
CSfmPropertyPage::_OnNotify(
    IN HWND hwnd,
    IN int idCtrl,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CSfmPropertyPage);

    // assume a property sheet notification
    return _OnPropertySheetNotify(hwnd, phdr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::_OnPropertySheetNotify, private
//
//  Synopsis:   WM_NOTIFY handler for the property sheet notification
//
//--------------------------------------------------------------------------

BOOL
CSfmPropertyPage::_OnPropertySheetNotify(
    IN HWND hwnd,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CSfmPropertyPage);

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
//  Method:     CSfmPropertyPage::_OnNcDestroy, private
//
//  Synopsis:   WM_NCDESTROY handler
//
//--------------------------------------------------------------------------

BOOL
CSfmPropertyPage::_OnNcDestroy(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSfmPropertyPage);

    delete this;    // do this LAST!
    return TRUE;
}



//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::_InitializeControls, private
//
//  Synopsis:   Initialize the controls from scratch
//
//--------------------------------------------------------------------------

VOID
CSfmPropertyPage::_InitializeControls(
    IN HWND hwnd
    )
{
    _SetControlsToDefaults(hwnd);
    _SetControlsFromData(hwnd);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::_SetControlsToDefaults, private
//
//  Synopsis:   Set all the controls on the page to their default values
//
//--------------------------------------------------------------------------

VOID
CSfmPropertyPage::_SetControlsToDefaults(
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

    SetDlgItemText(hwnd, IDC_SFM_PASSWORD,   L"");
    SetDlgItemText(hwnd, IDC_SFM_PASSWORD_CONFIRM,   L"");
    SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");

    _fInitializingPage = FALSE;
}



//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::SetControlsFromData, private
//
//  Synopsis:   From the class variables and current state of the radio
//              buttons, set the enabled/disabled state of the buttons, as
//              well as filling the controls with the appropriate values.
//
//--------------------------------------------------------------------------

VOID
CSfmPropertyPage::_SetControlsFromData(
    IN HWND hwnd
    )
{
    appAssert(NULL != _pCurInfo);

    _fInitializingPage = TRUE;

    SetDlgItemText(hwnd, IDC_SFM_PASSWORD, _pCurInfo->GetNetname());
    SetDlgItemText(hwnd, IDC_SFM_PASSWORD_CONFIRM, _pCurInfo->GetNetname());

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

	// BUGBUG:

	CheckDlgButton(hwnd, IDC_SFM_READONLY, BST_CHECKED);
	CheckDlgButton(hwnd, IDC_SFM_GUESTS, BST_CHECKED);

    _fInitializingPage = FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::_MarkItemDirty, private
//
//  Synopsis:   A change has made such that the current item (and page)
//              is now dirty
//
//--------------------------------------------------------------------------

VOID
CSfmPropertyPage::_MarkItemDirty(
    VOID
    )
{
    CHECK_SIG(CSfmPropertyPage);

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
//  Method:     CSfmPropertyPage::_ValidatePage, private
//
//  Synopsis:   Return TRUE if the current page is valid
//
//--------------------------------------------------------------------------

BOOL
CSfmPropertyPage::_ValidatePage(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSfmPropertyPage);
    appAssert(NULL != _pCurInfo);

    if (!_bDirty)
    {
        // nothing to validate
        return TRUE;
    }

	// BUGBUG: validate that the two passwords are the same

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::_DoApply, public
//
//  Synopsis:   If anything has changed, apply the data
//
//--------------------------------------------------------------------------

BOOL
CSfmPropertyPage::_DoApply(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSfmPropertyPage);

    if (_bDirty)
    {
        appAssert(NULL != _pCurInfo);

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

        NET_API_STATUS ret = _pCurInfo->Commit(_pszMachine);
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
        _bPasswordChanged = FALSE;
        _bPasswordConfirmChanged = FALSE;
        _bUserLimitChanged = FALSE;
        PropSheet_UnChanged(_GetFrameWindow(),_hwndPage);

        _InitializeControls(hwnd);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::_DoCancel, public
//
//  Synopsis:   Do whatever is necessary to cancel the changes
//
//--------------------------------------------------------------------------

BOOL
CSfmPropertyPage::_DoCancel(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSfmPropertyPage);

    if (_bDirty)
    {
        _bDirty = FALSE;
        _bPasswordChanged = FALSE;
        _bPasswordConfirmChanged = FALSE;
        _bUserLimitChanged = FALSE;
        PropSheet_UnChanged(_GetFrameWindow(),_hwndPage);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSfmPropertyPage::_CacheMaxUses, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------

VOID
CSfmPropertyPage::_CacheMaxUses(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSfmPropertyPage);

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
//  Method:     CSfmPropertyPage::Dump, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------
VOID
CSfmPropertyPage::Dump(
    IN PWSTR pszCaption
    )
{
    CHECK_SIG(CSfmPropertyPage);

    appDebugOut((DEB_TRACE,
        "CSfmPropertyPage::Dump, %ws\n",
        pszCaption));

    appDebugOut((DEB_TRACE | DEB_NOCOMPNAME,
"\t            This: 0x%08lx\n"
"\t            Page: 0x%08lx\n"
"\t   Initializing?: %ws\n"
"\t          Dirty?: %ws\n"
"\tPassword changed?: %ws\n"
"\tConfirm password changed?: %ws\n"
"\tUsr Lim changed?: %ws\n"
"\t        Max uses: %d\n"
"\t       _pCurInfo: 0x%08lx\n"
,
this,
_hwndPage,
_fInitializingPage ? L"yes" : L"no",
_bDirty            ? L"yes" : L"no",
_bPasswordChanged ? L"yes" : L"no",
_bPasswordConfirmChanged ? L"yes" : L"no",
_bUserLimitChanged ? L"yes" : L"no",
_wMaxUsers,
_pCurInfo
));

    _pCurInfo->Dump(L"Current");
}

#endif // DBG == 1
