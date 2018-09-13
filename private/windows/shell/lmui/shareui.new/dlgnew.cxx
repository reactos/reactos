//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dlgnew.cxx
//
//  Contents:   "New Share" dialog
//
//  History:    21-Feb-95 BruceFo Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "helpids.h"
#include "dlgnew.hxx"
#include "acl.hxx"
#include "util.hxx"
#include "shrinfo.hxx"

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Member:     CDlgNewShare::SizeWndProc, public
//
//  Synopsis:   "allow" edit window subclass proc to disallow non-numeric
//              characters.
//
//  History:    5-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

LRESULT CALLBACK
CDlgNewShare::SizeWndProc(
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

    CDlgNewShare* pThis = (CDlgNewShare*)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
    appAssert(NULL != pThis);
    return CallWindowProc(pThis->_pfnAllowProc, hwnd, wMsg, wParam, lParam);
}


//+-------------------------------------------------------------------------
//
//  Method:     CDlgNewShare::CDlgNewShare, private
//
//  Synopsis:   constructor
//
//--------------------------------------------------------------------------
CDlgNewShare::CDlgNewShare(
    IN HWND hwndParent,
    IN PWSTR pszMachine
    )
    :
    CDialog(hwndParent, MAKEINTRESOURCE(IDD_NEW_SHARE)),
    _pszMachine(pszMachine),
    _bShareNameChanged(FALSE),
    _bPathChanged(FALSE),
    _bCommentChanged(FALSE),
    _wMaxUsers(DEFAULT_MAX_USERS),
    _fSecDescModified(FALSE),
    _pfnAllowProc(NULL),
    _pShareInfo(NULL)
{
    INIT_SIG(CDlgNewShare);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDlgNewShare::~CDlgNewShare, private
//
//  Synopsis:   destructor
//
//--------------------------------------------------------------------------
CDlgNewShare::~CDlgNewShare()
{
    CHECK_SIG(CDlgNewShare);

    delete _pShareInfo;
    _pShareInfo = NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDlgNewShare::DlgProc, private
//
//  Synopsis:   Dialog Procedure for this object
//
//--------------------------------------------------------------------------
BOOL
CDlgNewShare::DlgProc(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CHECK_SIG(CDlgNewShare);

    static DWORD aHelpIds[] =
    {
        IDOK,                   HC_OK,
        IDCANCEL,               HC_CANCEL,
        IDC_SHARE_SHARENAME,    HC_SHARE_SHARENAME,
        IDC_SHARE_PATH,         HC_SHARE_PATH,
        IDC_SHARE_COMMENT,      HC_SHARE_COMMENT,
        IDC_SHARE_MAXIMUM,      HC_SHARE_MAXIMUM,
        IDC_SHARE_ALLOW,        HC_SHARE_ALLOW,
        IDC_SHARE_ALLOW_VALUE,  HC_SHARE_ALLOW_VALUE,
        IDC_SHARE_PERMISSIONS,  HC_SHARE_PERMISSIONS,
        0,0
    };

    switch (msg)
    {
    case WM_INITDIALOG:
        return _OnInitDialog(hwnd);

    case WM_COMMAND:
        return _OnCommand(hwnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);

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

    case WM_DESTROY:
    {
        // restore original subclass to window.
        appAssert(NULL != GetDlgItem(hwnd,IDC_SHARE_ALLOW_VALUE));
        SetWindowLong(GetDlgItem(hwnd,IDC_SHARE_ALLOW_VALUE), GWL_WNDPROC, (LONG)_pfnAllowProc);
        return FALSE;
    }

    } // end of switch

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDlgNewShare::_OnInitDialog, private
//
//  Synopsis:   WM_INITDIALOG handler
//
//--------------------------------------------------------------------------

BOOL
CDlgNewShare::_OnInitDialog(
    IN HWND hwnd
    )
{
    CHECK_SIG(CDlgNewShare);

    HRESULT hr;

    // for some reason, this dialog comes up on the bottom!
//  SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
//  SetActiveWindow(hwnd);
    SetForegroundWindow(hwnd);

    // Use a trick from the property sheet code to properly place the dialog.
    // Basically, we want it to go wherever a new window would have gone, not
    // always in the upper-left corner of the screen. This avoids the problem
    // of multiple dialogs showing up in the same place on the screen,
    // overlapping each other.

    const TCHAR c_szStatic[] = TEXT("Static");

    HWND hwndT = CreateWindowEx(0, c_szStatic, NULL,
                    WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT,
                    0, 0, NULL, NULL, g_hInstance, NULL);
    if (hwndT)
    {
        RECT rc;
        GetWindowRect(hwndT, &rc);
        DestroyWindow(hwndT);
        SetWindowPos(hwnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    SetDialogIconBig(hwnd, IDI_SHARESFLD);
    SetDialogIconSmall(hwnd, IDI_SHARESFLD);

    // storage for security descriptor

    _pShareInfo = new CShareInfo();
    if (NULL == _pShareInfo)
    {
        return FALSE;
    }

    hr = _pShareInfo->InitInstance();
    CHECK_HRESULT(hr);
    if (FAILED(hr))
    {
        delete _pShareInfo;
        return FALSE;
    }

    // Subclass allow edit control to disallow non-positive numbers
    _pfnAllowProc = (WNDPROC)SetWindowLong(
                                    GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE),
                                    GWL_WNDPROC,
                                    (LONG)&SizeWndProc);

    // use LanMan API constants to set maximum share name & comment lengths
    SendDlgItemMessage(hwnd, IDC_SHARE_SHARENAME, EM_LIMITTEXT, NNLEN, 0L);
    SendDlgItemMessage(hwnd, IDC_SHARE_PATH,      EM_LIMITTEXT, MAX_PATH-1, 0L);
    SendDlgItemMessage(hwnd, IDC_SHARE_COMMENT,   EM_LIMITTEXT, MAXCOMMENTSZ, 0L);

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

    SetFocus(GetDlgItem(hwnd, IDC_SHARE_SHARENAME));

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDlgNewShare::_OnCommand, private
//
//  Synopsis:   WM_COMMAND handler
//
//  History:    21-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
CDlgNewShare::_OnCommand(
    IN HWND hwnd,
    IN WORD wNotifyCode,
    IN WORD wID,
    IN HWND hwndCtl
    )
{
    CHECK_SIG(CDlgNewShare);

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
        }
        return TRUE;

    case IDC_SHARE_ALLOW_VALUE:
    {
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
        }
        return TRUE;

    case IDC_SHARE_SHARENAME:
    {
        if (wNotifyCode == EN_CHANGE)
        {
            _bShareNameChanged = TRUE;
        }
        return TRUE;
    }

    case IDC_SHARE_PATH:
    {
        if (wNotifyCode == EN_CHANGE)
        {
            _bPathChanged = TRUE;
        }
        return TRUE;
    }

    case IDC_SHARE_COMMENT:
    {
        if (wNotifyCode == EN_CHANGE)
        {
            _bCommentChanged = TRUE;
        }
        return TRUE;
    }


//
// Commands
//

    case IDOK:
        return _OnOK(hwnd);

    case IDCANCEL:
        EndDialog(hwnd, FALSE);
        return TRUE;

    case IDC_SHARE_PERMISSIONS:
        return _OnPermissions(hwnd);

    } // end of switch (wID)

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDlgNewShare::_OnOK, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------

BOOL
CDlgNewShare::_OnOK(
    IN HWND hwnd
    )
{
    CHECK_SIG(CDlgNewShare);

    HRESULT hr;
    HRESULT uTemp;
    BOOL    fSpecial = FALSE;   // IPC$ or ADMIN$

    // Validate the share

    WCHAR szShareName[NNLEN + 1];

    if (0 == GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName)))
    {
        MyErrorDialog(hwnd, IERR_BlankShareName);
        SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
        return TRUE;
    }

    WCHAR szPath[MAX_PATH];
    GetDlgItemText(hwnd, IDC_SHARE_PATH, szPath, ARRAYLEN(szPath));

    // Trying to create a reserved share?
    if (   (0 == _wcsicmp(g_szIpcShare,   szShareName))
        || (0 == _wcsicmp(g_szAdminShare, szShareName)))
    {
        // We will let you add IPC$ and ADMIN$ as long as there is no
        // path specified.
        if (szPath[0] != TEXT('\0'))
        {
            MyErrorDialog(hwnd, MSG_ADDSPECIAL);
            SetErrorFocus(hwnd, IDC_SHARE_PATH);
            return TRUE;
        }

        fSpecial = TRUE;
    }
    else if (!IsValidShareName(szShareName, &uTemp))
    {
        MyErrorDialog(hwnd, uTemp);
        SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
        return TRUE;
    }

    // If the user entered some ACL, warn them that we're going to nuke
    // it and let the system use its default (since special shares can't
    // have their security set).
    if (fSpecial || DriveLetterShare(szShareName))
    {
        if (_fSecDescModified)
        {
            DWORD id = MyConfirmationDialog(
                            hwnd,
                            MSG_NOSECURITYONSPECIAL,
                            MB_YESNO | MB_ICONEXCLAMATION);
            if (id == IDNO)
            {
                SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
                return TRUE;
            }
            _pShareInfo->TransferSecurityDescriptor(NULL);
        }
    }

    // Check to see that the same share isn't already used, for either the
    // same path or for another path.

    SHARE_INFO_2* info2;
    NET_API_STATUS ret = NetShareGetInfo(_pszMachine, szShareName, 2, (LPBYTE*)&info2);
    if (ret == NERR_Success)
    {
        // It is already shared. Trying to re-share IPC$ or ADMIN$?

        if (fSpecial)
        {
            MyErrorDialog(hwnd, IERR_AlreadyExistsSpecial, szShareName);
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            NetApiBufferFree(info2);
            return TRUE;
        }

        // Is it already shared for the same path?
        if (0 == _wcsicmp(info2->shi2_path, szPath))
        {
            MyErrorDialog(hwnd, IERR_AlreadyExists, szShareName);
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            NetApiBufferFree(info2);
            return TRUE;
        }

        // Shared for a different path. Ask the user if they wish to delete
        // the old share and create the new one using the name.

        DWORD id = ConfirmReplaceShare(hwnd, szShareName, info2->shi2_path, szPath);
        if (id == IDNO)
        {
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            NetApiBufferFree(info2);
            return TRUE;
        }
        else if (id == IDCANCEL)
        {
            EndDialog(hwnd, FALSE);
            NetApiBufferFree(info2);
            return TRUE;
        }

        // User said to replace the old share. Do it.
        ret = NetShareDel(_pszMachine, szShareName, 0);
        if (ret != NERR_Success)
        {
            DisplayError(hwnd, IERR_CANT_DEL_SHARE, ret, szShareName);
            NetApiBufferFree(info2);
            return FALSE;
        }
        else
        {
            SHChangeNotify(SHCNE_NETUNSHARE, SHCNF_PATH, info2->shi2_path, NULL);
        }

        NetApiBufferFree(info2);
    }

    if (!fSpecial)
    {
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
                return TRUE;
            }
        }
    }

    // Everything OK, save away the data

    if (_bShareNameChanged)
    {
        hr = _pShareInfo->SetNetname(szShareName);
        CHECK_HRESULT(hr);
    }

    if (_bPathChanged)
    {
        hr = _pShareInfo->SetPath(szPath);
        CHECK_HRESULT(hr);
    }

    if (_bCommentChanged)
    {
        WCHAR szComment[MAXCOMMENTSZ + 1];
        GetDlgItemText(hwnd, IDC_SHARE_COMMENT, szComment, ARRAYLEN(szComment));
        hr = _pShareInfo->SetRemark(szComment);
        CHECK_HRESULT(hr);
    }

    if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_MAXIMUM))
    {
        hr = _pShareInfo->SetMaxUses(SHI_USES_UNLIMITED);
        CHECK_HRESULT(hr);
    }
    else if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_ALLOW))
    {
        _CacheMaxUses(hwnd);
        hr = _pShareInfo->SetMaxUses(_wMaxUsers);
        CHECK_HRESULT(hr);
    }

    _pShareInfo->SetFlag(SHARE_FLAG_ADDED);
    ret = _pShareInfo->Commit(_pszMachine);
    if (ret != NERR_Success)
    {
        DisplayError(hwnd, IERR_CANT_ADD_SHARE, ret, _pShareInfo->GetNetname());
    }

    if (fSpecial)
    {
        // IPC$ doesn't have a path. ADMIN$ does, but we need to get it: we
        // create the share passing in no path, as required by the API, but
        // then we ask the server what it decided to share it as.

        if (0 == _wcsicmp(g_szAdminShare, szShareName))
        {
            SHARE_INFO_2* info2;
            NET_API_STATUS ret = NetShareGetInfo(_pszMachine, szShareName, 2, (LPBYTE*)&info2);
            if (ret == NERR_Success)
            {
                SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH, info2->shi2_path, NULL);
                NetApiBufferFree(info2);
            }
            // else... oh well. No notification to the shell.
        }
    }
    else
    {
        SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH, _pShareInfo->GetPath(), NULL);
    }

    EndDialog(hwnd, TRUE);
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDlgNewShare::_OnPermissions, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------
BOOL
CDlgNewShare::_OnPermissions(
    IN HWND hwnd
    )
{
    CHECK_SIG(CDlgNewShare);

    WCHAR szShareName[NNLEN + 1];
    GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));

    PSECURITY_DESCRIPTOR pNewSecDesc = NULL;
    PSECURITY_DESCRIPTOR pSecDesc = _pShareInfo->GetSecurityDescriptor();
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
        _fSecDescModified = TRUE;

        appAssert(IsValidSecurityDescriptor(pNewSecDesc));
        _pShareInfo->TransferSecurityDescriptor(pNewSecDesc);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDlgNewShare::_CacheMaxUses, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------
VOID
CDlgNewShare::_CacheMaxUses(
    IN HWND hwnd
    )
{
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
