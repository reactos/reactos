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
#include "cache.hxx"
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
    IN HWND hwndParent
    )
    :
    CDialog(hwndParent, MAKEINTRESOURCE(IDD_NEW_SHARE)),
    _bShareNameChanged(FALSE),
    _bCommentChanged(FALSE),
    _wMaxUsers(DEFAULT_MAX_USERS),
    _fSecDescModified(FALSE),
    _pStoredSecDesc(NULL),
    _pfnAllowProc(NULL)
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
        IDOK,                       HC_OK,
        IDCANCEL,                   HC_CANCEL,
        IDC_SHARE_SHARENAME,        HC_SHARE_SHARENAME,
        IDC_SHARE_SHARENAME_TEXT,   HC_SHARE_SHARENAME,
        IDC_SHARE_COMMENT,          HC_SHARE_COMMENT,
        IDC_SHARE_COMMENT_TEXT,     HC_SHARE_COMMENT,
        IDC_SHARE_MAXIMUM,          HC_SHARE_MAXIMUM,
        IDC_SHARE_ALLOW,            HC_SHARE_ALLOW,
        IDC_SHARE_ALLOW_VALUE,      HC_SHARE_ALLOW_VALUE,
        IDC_SHARE_USERS_TEXT,       HC_SHARE_ALLOW_VALUE,
        IDC_SHARE_PERMISSIONS,      HC_SHARE_PERMISSIONS,
        IDC_SHARE_LIMIT,            HC_SHARE_LIMIT,
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

    // Subclass allow edit control to disallow non-positive numbers
    _pfnAllowProc = (WNDPROC)SetWindowLong(
                                    GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE),
                                    GWL_WNDPROC,
                                    (LONG)&SizeWndProc);

    // use LanMan API constants to set maximum share name & comment lengths
    SendDlgItemMessage(hwnd, IDC_SHARE_SHARENAME, EM_LIMITTEXT, NNLEN, 0L);
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

    // Validate the share

    WCHAR szShareName[NNLEN + 1];

    if (0 == GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName)))
    {
        MyErrorDialog(hwnd, IERR_BlankShareName);
        SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
        return TRUE;
    }

    HRESULT uTemp;
    if (!IsValidShareName(szShareName, &uTemp))
    {
        MyErrorDialog(hwnd, uTemp);
        SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
        return TRUE;
    }

    // Trying to create a reserved share?
    if (0 == _wcsicmp(g_szIpcShare, szShareName))
    {
        MyErrorDialog(hwnd, IERR_SpecialShare);
        SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
        return TRUE;
    }

    if (0 == _wcsicmp(g_szAdminShare, szShareName))
    {
        // We will let the admin create the admin$ share if they create
        // it in the directory specified by GetWindowsDirectory().
        WCHAR szWindowsDir[MAX_PATH];
        UINT err = GetWindowsDirectory(szWindowsDir, ARRAYLEN(szWindowsDir));
        if (err == 0)
        {
            // oh well, give them this error
            MyErrorDialog(hwnd, IERR_SpecialShare);
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return FALSE;
        }

        if (0 != _wcsicmp(m_pShareInfo->GetPath(), szWindowsDir))
        {
            MyErrorDialog(hwnd, IERR_SpecialShare);
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return FALSE;
        }

        // otherwise, it is the right directory. Let them create it.
    }

    // Check to see that the same share doesn't already exist. We don't allow
    // the user to create a share with the same name as a marked-for-delete
    // share, because it's easier!
    for (CShareInfo* p = (CShareInfo*) m_pInfoList->Next();
         p != m_pInfoList;
         p = (CShareInfo*) p->Next())
    {
        if (0 == _wcsicmp(p->GetNetname(), szShareName))
        {
            MyErrorDialog(hwnd, IERR_AlreadyExists, szShareName);
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return TRUE;
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
            return TRUE;
        }
    }

    WCHAR szOldPath[PATHLEN + 1];

    if (g_ShareCache.IsExistingShare(szShareName, m_pShareInfo->GetPath(), szOldPath))
    {
        DWORD id = ConfirmReplaceShare(hwnd, szShareName, szOldPath, m_pShareInfo->GetPath());
        if (id != IDYES)
        {
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return TRUE;
        }

        // User said to replace the old share. We need to add
        // a "delete" record for the old share.

        CShareInfo* pNewInfo = new CShareInfo();
        if (NULL == pNewInfo)
        {
            return FALSE;
        }

        hr = pNewInfo->InitInstance();
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            delete pNewInfo;
            return FALSE;
        }

        hr = pNewInfo->SetNetname(szShareName);
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            delete pNewInfo;
            return FALSE;
        }

        hr = pNewInfo->SetPath(szOldPath);
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            delete pNewInfo;
            return FALSE;
        }

        pNewInfo->SetFlag(SHARE_FLAG_REMOVE);
        pNewInfo->InsertBefore(m_pReplaceList); // add to end of replace list
    }

    // Everything OK, save away the data

    if (_bShareNameChanged)
    {
        hr = m_pShareInfo->SetNetname(szShareName);
        CHECK_HRESULT(hr);
    }

    if (_bCommentChanged)
    {
        WCHAR szComment[MAXCOMMENTSZ + 1];
        GetDlgItemText(hwnd, IDC_SHARE_COMMENT, szComment, ARRAYLEN(szComment));
        hr = m_pShareInfo->SetRemark(szComment);
        CHECK_HRESULT(hr);
    }

    if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_MAXIMUM))
    {
        hr = m_pShareInfo->SetMaxUses(SHI_USES_UNLIMITED);
        CHECK_HRESULT(hr);
    }
    else if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_ALLOW))
    {
        _CacheMaxUses(hwnd);
        hr = m_pShareInfo->SetMaxUses(_wMaxUsers);
        CHECK_HRESULT(hr);
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
    PSECURITY_DESCRIPTOR pSecDesc = m_pShareInfo->GetSecurityDescriptor();
    appAssert(NULL == pSecDesc || IsValidSecurityDescriptor(pSecDesc));

    BOOL bSecDescChanged;
    LONG err = EditShareAcl(
                        hwnd,
                        NULL,
                        szShareName,
                        pSecDesc,
                        &bSecDescChanged,
                        &pNewSecDesc);

    if (bSecDescChanged)
    {
        _fSecDescModified = TRUE;

        appAssert(IsValidSecurityDescriptor(pNewSecDesc));
        m_pShareInfo->TransferSecurityDescriptor(pNewSecDesc);
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
