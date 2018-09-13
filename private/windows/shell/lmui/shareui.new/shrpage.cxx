//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       shrpage.cxx
//
//  Contents:   "Sharing" shell property page extension
//
//  History:    6-Apr-95        BruceFo     Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "helpids.h"
#include "dlgnew.hxx"
#include "acl.hxx"
#include "shrinfo.hxx"
#include "shrpage.hxx"
#include "util.hxx"

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::DlgProcPage, static public
//
//  Synopsis:   Dialog Procedure for all CSharingPropertyPage
//
//--------------------------------------------------------------------------

BOOL CALLBACK
CSharingPropertyPage::DlgProcPage(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CSharingPropertyPage* pThis = NULL;

    if (msg==WM_INITDIALOG)
    {
        SHARE_PROPSHEETPAGE* sprop = (SHARE_PROPSHEETPAGE*)lParam;
        pThis = new CSharingPropertyPage(hwnd, sprop->pszMachine, sprop->pszShareName);
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
        pThis = (CSharingPropertyPage*) GetWindowLong(hwnd,GWL_USERDATA);
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
//  Member:     CSharingPropertyPage::SizeWndProc, public
//
//  Synopsis:   "allow" edit window subclass proc to disallow non-numeric
//              characters.
//
//  History:    5-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

LRESULT CALLBACK
CSharingPropertyPage::SizeWndProc(
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

    CSharingPropertyPage* pThis = (CSharingPropertyPage*)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
    appAssert(NULL != pThis);
    appAssert(NULL != pThis->_pfnAllowProc);
    return CallWindowProc(pThis->_pfnAllowProc, hwnd, wMsg, wParam, lParam);
}


//+--------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::CSharingPropertyPage, public
//
//  Synopsis:   Constructor
//
//---------------------------------------------------------------------------

CSharingPropertyPage::CSharingPropertyPage(
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
    _bPathChanged(FALSE),
    _bCommentChanged(FALSE),
    _bUserLimitChanged(FALSE),
    _bSecDescChanged(FALSE),
    _wMaxUsers(DEFAULT_MAX_USERS),
    _pCurInfo(NULL),
    _pszReplacePath(NULL),
    _pfnAllowProc(NULL)
{
    INIT_SIG(CSharingPropertyPage);
    appAssert(NULL != _pszShare);
}


//+--------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::~CSharingPropertyPage, public
//
//  Synopsis:   Destructor
//
//---------------------------------------------------------------------------

CSharingPropertyPage::~CSharingPropertyPage()
{
    CHECK_SIG(CSharingPropertyPage);

    delete _pCurInfo;
    _pCurInfo = NULL;

    delete[] _pszReplacePath;
    _pszReplacePath = NULL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::InitInstance, public
//
//  Synopsis:   Part II of the constuctor process
//
//  Notes:      We don't want to handle any errors in constuctor, so this
//              method is necessary for the second phase error detection.
//
//--------------------------------------------------------------------------

HRESULT
CSharingPropertyPage::InitInstance(
    VOID
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appDebugOut((DEB_ITRACE, "CSharingPropertyPage::InitInstance\n"));

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
//  Method:     CSharingPropertyPage::_PageProc, private
//
//  Synopsis:   Dialog Procedure for this object
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_PageProc(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CHECK_SIG(CSharingPropertyPage);

    static DWORD aHelpIds[] =
    {
        IDC_SHARE_SHARENAME,        HC_SHARE_SHARENAME,
        IDC_SHARE_PATH,             HC_SHARE_PATH,
        IDC_SHARE_COMMENT,          HC_SHARE_COMMENT,
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
//  Method:     CSharingPropertyPage::_OnInitDialog, private
//
//  Synopsis:   WM_INITDIALOG handler
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnInitDialog(
    IN HWND hwnd,
    IN HWND hwndFocus,
    IN LPARAM lInitParam
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appDebugOut((DEB_ITRACE, "_OnInitDialog\n"));

    SetDialogIconBig(_GetFrameWindow(), IDI_SHARESFLD);

    // Subclass allow edit control to disallow non-positive numbers
    _pfnAllowProc = (WNDPROC)SetWindowLong(
                                    GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE),
                                    GWL_WNDPROC,
                                    (LONG)&SizeWndProc);

    // use LanMan API constants to set maximum share name & comment lengths
    SendDlgItemMessage(hwnd, IDC_SHARE_SHARENAME, EM_LIMITTEXT, NNLEN, 0L);
    SendDlgItemMessage(hwnd, IDC_SHARE_PATH,      EM_LIMITTEXT, MAX_PATH-1, 0L);
    SendDlgItemMessage(hwnd, IDC_SHARE_COMMENT,   EM_LIMITTEXT, MAXCOMMENTSZ, 0L);

    _InitializeControls(hwnd);

// #if DBG == 1
//  Dump(L"_OnInitDialog finished");
// #endif // DBG == 1

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnCommand, private
//
//  Synopsis:   WM_COMMAND handler
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnCommand(
    IN HWND hwnd,
    IN WORD wNotifyCode,
    IN WORD wID,
    IN HWND hwndCtl
    )
{
    CHECK_SIG(CSharingPropertyPage);

    switch (wID)
    {

//
// Notifications
//

    case IDC_SHARE_SHARENAME:
    {
        if (EN_CHANGE == wNotifyCode)
        {
            if (!_fInitializingPage)
            {
                _bShareNameChanged = TRUE;
                _MarkItemDirty();
            }
        }
        return TRUE;
    }

    case IDC_SHARE_PATH:
    {
        if (EN_CHANGE == wNotifyCode)
        {
            if (!_fInitializingPage)
            {
                _bPathChanged = TRUE;
                _MarkItemDirty();
            }
        }
        return TRUE;
    }

    case IDC_SHARE_COMMENT:
    {
        if (EN_CHANGE == wNotifyCode)
        {
            if (!_fInitializingPage)
            {
                _bCommentChanged = TRUE;
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

    case IDC_SHARE_PERMISSIONS:
        return _OnPermissions(hwnd);

    default:
        break;
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnNotify, private
//
//  Synopsis:   WM_NOTIFY handler
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnNotify(
    IN HWND hwnd,
    IN int idCtrl,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CSharingPropertyPage);

    // assume a property sheet notification
    return _OnPropertySheetNotify(hwnd, phdr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnPropertySheetNotify, private
//
//  Synopsis:   WM_NOTIFY handler for the property sheet notification
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnPropertySheetNotify(
    IN HWND hwnd,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CSharingPropertyPage);

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
//  Method:     CSharingPropertyPage::_OnNcDestroy, private
//
//  Synopsis:   WM_NCDESTROY handler
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnNcDestroy(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    delete this;    // do this LAST!
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnPermissions, private
//
//  Synopsis:   WM_COMMAND handler: the permissions button
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnPermissions(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);
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
//  Method:     CSharingPropertyPage::_InitializeControls, private
//
//  Synopsis:   Initialize the controls from scratch
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_InitializeControls(
    IN HWND hwnd
    )
{
    _SetControlsToDefaults(hwnd);
    _SetControlsFromData(hwnd);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_SetControlsToDefaults, private
//
//  Synopsis:   Set all the controls on the page to their default values
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_SetControlsToDefaults(
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

    SetDlgItemText(hwnd, IDC_SHARE_SHARENAME,   L"");
    SetDlgItemText(hwnd, IDC_SHARE_PATH,        L"");
    SetDlgItemText(hwnd, IDC_SHARE_COMMENT,     L"");
    SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");

    _fInitializingPage = FALSE;
}



//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::SetControlsFromData, private
//
//  Synopsis:   From the class variables and current state of the radio
//              buttons, set the enabled/disabled state of the buttons, as
//              well as filling the controls with the appropriate values.
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_SetControlsFromData(
    IN HWND hwnd
    )
{
    appAssert(NULL != _pCurInfo);

    _fInitializingPage = TRUE;

    SetDlgItemText(hwnd, IDC_SHARE_SHARENAME, _pCurInfo->GetNetname());
    SetDlgItemText(hwnd, IDC_SHARE_PATH,      _pCurInfo->GetPath());
    SetDlgItemText(hwnd, IDC_SHARE_COMMENT,   _pCurInfo->GetRemark());

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

    // Managine a special share?
    if (_pCurInfo->GetType() & STYPE_SPECIAL)
    {
        // Can't change the path or the name
        // Making it read-only makes it easier to read than if it were disabled
        SendDlgItemMessage(hwnd, IDC_SHARE_SHARENAME, EM_SETREADONLY, (WPARAM)(BOOL)TRUE, 0);
        SendDlgItemMessage(hwnd, IDC_SHARE_PATH,      EM_SETREADONLY, (WPARAM)(BOOL)TRUE, 0);
    }

    _fInitializingPage = FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_MarkItemDirty, private
//
//  Synopsis:   A change has made such that the current item (and page)
//              is now dirty
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_MarkItemDirty(
    VOID
    )
{
    CHECK_SIG(CSharingPropertyPage);

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
//  Method:     CSharingPropertyPage::_ValidatePage, private
//
//  Synopsis:   Return TRUE if the current page is valid
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_ValidatePage(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appAssert(NULL != _pCurInfo);

    if (!_bDirty)
    {
        // nothing to validate
        return TRUE;
    }

    if (!_bShareNameChanged)
    {
        appDebugOut((DEB_ITRACE, "_ValidatePage: share name not changed!\n"));

        return TRUE;
    }

    delete[] _pszReplacePath;
    _pszReplacePath = NULL;

    WCHAR szShareName[MAX_PATH];
    GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));

    WCHAR szPath[MAX_PATH];
    GetDlgItemText(hwnd, IDC_SHARE_PATH, szPath, ARRAYLEN(szPath));

    // Validate the share

    if (0 == wcslen(szShareName))
    {
        MyErrorDialog(hwnd, IERR_BlankShareName);
        SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
        return FALSE;
    }

    HRESULT uTemp;
    if (!IsValidShareName(szShareName, &uTemp))
    {
        MyErrorDialog(hwnd, uTemp);
        SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
        return FALSE;
    }

    // Trying to create a reserved share?
    if (   (0 == _wcsicmp(g_szIpcShare,   szShareName))
        || (0 == _wcsicmp(g_szAdminShare, szShareName)))
    {
        MyErrorDialog(hwnd, IERR_SpecialShare);
        SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
        return FALSE;
    }

    // If the user entered some ACL, warn them that we're going to nuke
    // it and let the system use its default (since special shares can't
    // have their security set).
    if (DriveLetterShare(szShareName))
    {
        if (_bSecDescChanged)
        {
            DWORD id = MyConfirmationDialog(
                            hwnd,
                            MSG_NOSECURITYONSPECIAL,
                            MB_YESNO | MB_ICONEXCLAMATION);
            if (id == IDNO)
            {
                SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
                return FALSE;
            }
            _pCurInfo->TransferSecurityDescriptor(NULL);
        }
    }

    // Check for downlevel accessibility
    ULONG nType;
    if (NERR_Success != NetpPathType(NULL, szShareName, &nType, INPT_FLAGS_OLDPATHS))
    {
        DWORD id = MyConfirmationDialog(
                        hwnd,
                        IERR_InaccessibleByDos,
                        MB_YESNO | MB_ICONEXCLAMATION,
                        szShareName);
        if (id == IDNO)
        {
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return FALSE;
        }
    }

    // Check to see that the same share isn't already used, for either the
    // same path or for another path.

    SHARE_INFO_502* info502;
    NET_API_STATUS ret = NetShareGetInfo(_pszMachine, szShareName, 502, (LPBYTE*)&info502);
    if (ret == NERR_Success)
    {
        // It is already shared. Same path?

        if (0 == _wcsicmp(info502->shi502_path, szPath))
        {
            MyErrorDialog(hwnd, IERR_AlreadyExists, szShareName);
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            NetApiBufferFree(info502);
            return FALSE;
        }

        // Shared for a different path. Ask the user if they wish to delete
        // the old share and create the new one using the name.

        DWORD id = ConfirmReplaceShare(hwnd, szShareName, info502->shi502_path, szPath);
        if (id == IDNO)
        {
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            NetApiBufferFree(info502);
            return FALSE;
        }
        else if (id == IDCANCEL)
        {
            // EndDialog(hwnd, FALSE);  // BUGBUG: nuke page
            NetApiBufferFree(info502);
            return TRUE;
        }
        else
        {
            _pszReplacePath = NewDup(info502->shi502_path);
            if (NULL == _pszReplacePath)
            {
                NetApiBufferFree(info502);
                MyErrorDialog(hwnd, E_OUTOFMEMORY);
                return FALSE;
            }
        }

        NetApiBufferFree(info502);
    }
    else
    {
        // NetShareGetInfo failed. This is probably because there
        // is no share by this name, in which case it is a pure rename.

        appDebugOut((DEB_TRACE,
            "NetShareGetInfo failed, 0x%08lx\n",
            ret));
    }

#if DBG == 1
    Dump(L"_ValidatePage finished");
#endif // DBG == 1

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_DoApply, public
//
//  Synopsis:   If anything has changed, apply the data
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_DoApply(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    if (_bDirty)
    {
        appAssert(NULL != _pCurInfo);

        NET_API_STATUS ret;
        HRESULT hr;

        // If either the share name changed or the path changed, we need
        // to delete the old share. If the share name changed, it is
        // because the user is renaming the share. If the path changed,
        // it is because the LanMan APIs don't allow a NetShareSetInfo
        // to change the shared path.

        if (_bShareNameChanged)
        {
            // The share name was changed. This is either a pure rename, in
            // which case the old share should be deleted and a share with
            // the new name created, or the user changed the share name to
            // the name of an already existing share, in which case both the
            // current share name as well as the share to be replaced must be
            // deleted.

            if (NULL != _pszReplacePath)
            {
                // user said to replace an existing share
                WCHAR szShareName[NNLEN + 1];
                GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));

                ret = NetShareDel(_pszMachine, szShareName, 0);
                if (ret != NERR_Success)
                {
                    DisplayError(hwnd, IERR_CANT_DEL_SHARE, ret, szShareName);
                    delete[] _pszReplacePath;
                    _pszReplacePath = NULL;
                    return FALSE;
                }
                else
                {
                    SHChangeNotify(SHCNE_NETUNSHARE, SHCNF_PATH, _pszReplacePath, NULL);
                }

                delete[] _pszReplacePath;
                _pszReplacePath = NULL;
            }
        }

        if (_bShareNameChanged || _bPathChanged)
        {
            // delete the existing share
            ret = NetShareDel(_pszMachine, _pCurInfo->GetNetname(), 0);
            if (ret != NERR_Success)
            {
                DisplayError(hwnd, IERR_CANT_DEL_SHARE, ret, _pCurInfo->GetNetname());
                return FALSE;
            }
            else
            {
                // Only if the path changed in the rename should the shell
                // be notified
                if (_bPathChanged)
                {
                    SHChangeNotify(SHCNE_NETUNSHARE, SHCNF_PATH, _pCurInfo->GetPath(), NULL);
                }
            }
        }

        if (_bShareNameChanged)
        {
            // User wants to rename the share.
            WCHAR szShareName[NNLEN + 1];
            _pCurInfo->SetFlag(SHARE_FLAG_ADDED);   // special case
            GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));
            _pCurInfo->SetNetname(szShareName);
        }

        if (_bPathChanged)
        {
            WCHAR szPath[MAX_PATH];
            _pCurInfo->SetFlag(SHARE_FLAG_ADDED);   // special case
            GetDlgItemText(hwnd, IDC_SHARE_PATH, szPath, ARRAYLEN(szPath));
            _pCurInfo->SetPath(szPath);
        }

        if (_bCommentChanged)
        {
            WCHAR szComment[MAXCOMMENTSZ + 1];
            GetDlgItemText(hwnd, IDC_SHARE_COMMENT, szComment, ARRAYLEN(szComment));
            _pCurInfo->SetRemark(szComment);
        }

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
        _bShareNameChanged = FALSE;
        _bPathChanged = FALSE;
        _bCommentChanged = FALSE;
        _bUserLimitChanged = FALSE;
        _bSecDescChanged = FALSE;
        PropSheet_UnChanged(_GetFrameWindow(),_hwndPage);

        SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH, _pCurInfo->GetPath(), NULL);

        _InitializeControls(hwnd);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_DoCancel, public
//
//  Synopsis:   Do whatever is necessary to cancel the changes
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_DoCancel(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    if (_bDirty)
    {
        _bDirty = FALSE;
        _bShareNameChanged = FALSE;
        _bPathChanged = FALSE;
        _bCommentChanged = FALSE;
        _bUserLimitChanged = FALSE;
        _bSecDescChanged = FALSE;
        PropSheet_UnChanged(_GetFrameWindow(),_hwndPage);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_CacheMaxUses, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_CacheMaxUses(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

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
//  Method:     CSharingPropertyPage::Dump, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------
VOID
CSharingPropertyPage::Dump(
    IN PWSTR pszCaption
    )
{
    CHECK_SIG(CSharingPropertyPage);

    appDebugOut((DEB_TRACE,
        "CSharingPropertyPage::Dump, %ws\n",
        pszCaption));

    appDebugOut((DEB_TRACE | DEB_NOCOMPNAME,
"\t            This: 0x%08lx\n"
"\t            Page: 0x%08lx\n"
"\t   Initializing?: %ws\n"
"\t          Dirty?: %ws\n"
"\t  Share changed?: %ws\n"
"\t   Path changed?: %ws\n"
"\tComment changed?: %ws\n"
"\tUsr Lim changed?: %ws\n"
"\t        Max uses: %d\n"
"\t       _pCurInfo: 0x%08lx\n"
,
this,
_hwndPage,
_fInitializingPage ? L"yes" : L"no",
_bDirty            ? L"yes" : L"no",
_bShareNameChanged ? L"yes" : L"no",
_bPathChanged      ? L"yes" : L"no",
_bCommentChanged   ? L"yes" : L"no",
_bUserLimitChanged ? L"yes" : L"no",
_wMaxUsers,
_pCurInfo
));

    _pCurInfo->Dump(L"Current");
}

#endif // DBG == 1
