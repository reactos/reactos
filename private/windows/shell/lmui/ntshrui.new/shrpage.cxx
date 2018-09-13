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
#include "cache.hxx"
#include "share.hxx"
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
        BOOL bDialog = FALSE;
        SHARINGPROPSHEETPAGE* pspsp = (SHARINGPROPSHEETPAGE*)lParam;
        if (pspsp->psp.dwSize == sizeof(SHARINGPROPSHEETPAGE)
            && pspsp->bDialog
            )
        {
            bDialog = TRUE;
        }

        PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
        pThis = new CSharingPropertyPage(hwnd, (SHARE_PAGE_INFO*)psp->lParam, bDialog);
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
	IN SHARE_PAGE_INFO* pInfo,
    IN BOOL bDialog     // called as a dialog, not a property page?
    )
    :
    _hwndPage(hwndPage),
	_pInfo(pInfo), 			// take ownership!
	_pShareCache(&g_ShareCache),
    _bDirty(FALSE),
    _bItemDirty(FALSE),
    _bShareNameChanged(FALSE),
    _bCommentChanged(FALSE),
    _bUserLimitChanged(FALSE),
    _bSecDescChanged(FALSE),
    _wMaxUsers(DEFAULT_MAX_USERS),
    _pInfoList(NULL),
    _pReplaceList(NULL),
    _pCurInfo(NULL),
    _cShares(0),
    _bNewShare(TRUE),
    _pfnAllowProc(NULL),
    _bDialog(bDialog)
{
    INIT_SIG(CSharingPropertyPage);
    appAssert(NULL != _pInfo->pszPath);
	_pszStoragePath = _pInfo->bRemote ? _pInfo->pszRemotePath : _pInfo->pszPath;
    appDebugOut((DEB_TRACE, "Storage path: %ws\n", _pszStoragePath));
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

    // delete the the list of shares
    appAssert(NULL != _pInfoList);
    DeleteShareInfoList(_pInfoList, TRUE);
    _pInfoList = NULL;
    _pCurInfo = NULL;

    // delete the "replacement" list
    appAssert(NULL != _pReplaceList);
    DeleteShareInfoList(_pReplaceList, TRUE);
    _pReplaceList = NULL;

    delete[] (BYTE*)_pInfo;
	_pInfo = NULL;

	if (_pShareCache != &g_ShareCache)
	{
		delete _pShareCache;
		_pShareCache = NULL;
	}
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

    _pInfoList = new CShareInfo();  // dummy head node
    if (NULL == _pInfoList)
    {
        return E_OUTOFMEMORY;
    }

    _pReplaceList = new CShareInfo();  // dummy head node
    if (NULL == _pReplaceList)
    {
        return E_OUTOFMEMORY;
    }

	if (_pInfo->bRemote)
	{
		// create a cache of shares on the remote machine.
		_pShareCache = new CShareCache();
		if (NULL == _pShareCache)
		{
			return E_OUTOFMEMORY;
		}
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

        IDC_SHARE_NOTSHARED,        HC_SHARE_NOTSHARED,
        IDC_SHARE_SHAREDAS,         HC_SHARE_SHAREDAS,
        IDC_SHARE_SHARENAME_COMBO,  HC_SHARE_SHARENAME_COMBO,
        IDC_SHARE_REMOVE,           HC_SHARE_REMOVE,
        IDC_SHARE_NEWSHARE,         HC_SHARE_NEWSHARE,

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

    // Subclass allow edit control to disallow non-positive numbers
    _pfnAllowProc = (WNDPROC)SetWindowLong(
                                    GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE),
                                    GWL_WNDPROC,
                                    (LONG)&SizeWndProc);

    // use LanMan API constants to set maximum share name & comment lengths
    SendDlgItemMessage(hwnd, IDC_SHARE_SHARENAME, EM_LIMITTEXT, NNLEN, 0L);
    SendDlgItemMessage(hwnd, IDC_SHARE_COMMENT,   EM_LIMITTEXT, MAXCOMMENTSZ, 0L);

    if (_bDialog)
    {
        SetWindowText(hwnd, _pInfo->pszPath);
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
        ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);

        EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
        ShowWindow(GetDlgItem(hwnd, IDCANCEL), SW_HIDE);
    }

    // We know this drive is allowed to be shared. However, someone may
    // have changed the number or characteristics of the share using the
    // command line or winfile or server manager or the new file server
    // tool, and the cache may not have been refreshed. Force a refresh
    // now, to pick up any new shares for this path.
    //
    // Note that for the SharingDialog() API, this is the first time the
    // cache gets loaded. If the server is not running, we nuke the dialog
    // and return an error code.
    _pShareCache->Refresh(_pInfo->pszServer);
    if (_bDialog && !g_fSharingEnabled)
    {
        EndDialog(hwnd, -1);
    }

#if DBG == 1
	_pShareCache->Dump();
#endif // DBG == 1

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

    case IDC_SHARE_NOTSHARED:
    {
        if (BN_CLICKED == wNotifyCode)
        {
            if (!_fInitializingPage)
            {
                _ReadControls(hwnd);

                // Delete all shares from the combo box; convert the combo box
                // to the edit control (default state), and then disable
                // all the controls.  Cache whatever shares we had, so if
                // the user hits "Shared As", we can put them back.

                if (_cShares > 1)
                {
                    DWORD id = MyConfirmationDialog(
                                    hwnd,
                                    MSG_MULTIDEL,
                                    MB_YESNO | MB_ICONQUESTION);
                    if (IDNO == id)
                    {
                        _fInitializingPage = TRUE;
                        CheckRadioButton(
                                hwnd,
                                IDC_SHARE_NOTSHARED,
                                IDC_SHARE_SHAREDAS,
                                IDC_SHARE_SHAREDAS);
                        _fInitializingPage = FALSE;
                        return TRUE;
                    }
                }

                //
                // Next, regenerate the UI
                //

                _SetControlsFromData(hwnd, NULL);

                _MarkPageDirty();
            }
        }

        return TRUE;
    }

    case IDC_SHARE_SHAREDAS:
    {
        if (BN_CLICKED == wNotifyCode)
        {
            if (!_fInitializingPage)
            {
                // if there were shares there that we just hid when the user
                // pressed "Not Shared", then put them back.

                //
                // Regenerate the UI
                //

                _SetControlsFromData(hwnd, NULL);

                _MarkPageDirty();
            }
        }

        return TRUE;
    }

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

    case IDC_SHARE_SHARENAME_COMBO:
    {
        if (CBN_SELCHANGE == wNotifyCode)
        {
            _ReadControls(hwnd);

            HWND hwndCombo = GetDlgItem(hwnd, IDC_SHARE_SHARENAME_COMBO);
            int item = ComboBox_GetCurSel(hwndCombo);
            _pCurInfo = (CShareInfo*)ComboBox_GetItemData(hwndCombo, item);
            appAssert(NULL != _pCurInfo);

            _SetFieldsFromCurrent(hwnd);
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

    case IDC_SHARE_REMOVE:
        return _OnRemove(hwnd);

    case IDC_SHARE_NEWSHARE:
        return _OnNewShare(hwnd);

//
// Commands only used when this page is invoked as a dialog box, via the
// SharingDialog() API:
//

    case IDCANCEL:
    {
        if (!_DoCancel(hwnd))
        {
            // We might consider not going away. But instead, go away anyway.
        }

        EndDialog(hwnd, FALSE);
        return TRUE;
    }

    case IDOK:    // change to another page
    {
        if (!_ValidatePage(hwnd))
        {
            // don't go away
            return TRUE;
        }

        if (!_DoApply(hwnd))
        {
            // don't go away
            return TRUE;
        }

        EndDialog(hwnd, TRUE);
        return TRUE;
    }

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
    if (_cShares > 0)
    {
        wcscpy(szShareName, _pCurInfo->GetNetname());
    }
    else
    {
        GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));
    }

    PSECURITY_DESCRIPTOR pNewSecDesc = NULL;
    PSECURITY_DESCRIPTOR pSecDesc = _pCurInfo->GetSecurityDescriptor();
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
        _bSecDescChanged = TRUE;

        appAssert(IsValidSecurityDescriptor(pNewSecDesc));
        _pCurInfo->TransferSecurityDescriptor(pNewSecDesc);
        _MarkPageDirty();
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnRemove, private
//
//  Synopsis:   WM_COMMAND handler: the Remove button
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnRemove(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appAssert(_cShares > 1);
    appAssert(_pCurInfo != NULL);

    //
    // Alter the data structures
    //

    if (_pCurInfo->GetFlag() == SHARE_FLAG_ADDED)
    {
        // Something that was added this session is being removed: get rid of
        // it from our list.

        _pCurInfo->Unlink();
        delete _pCurInfo;
        // the _SetControlsFromData call resets the _pCurInfo pointer
    }
    else
    {
        // if the state was MODIFY or no state, then set it to REMOVE
        _pCurInfo->SetFlag(SHARE_FLAG_REMOVE);
    }

    --_cShares;

    //
    // Next, regenerate the UI
    //

    _SetControlsFromData(hwnd, NULL);

    _MarkPageDirty();

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnNewShare, private
//
//  Synopsis:   WM_COMMAND handler: the New Share button
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnNewShare(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    //
    // First, create an object to put the new share into.
    // BUGBUG: for out of memory errors, should we pop up an error box?
    //

    HRESULT hr;

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

    hr = pNewInfo->SetPath(_pszStoragePath);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
    {
        delete pNewInfo;
        return FALSE;
    }

    pNewInfo->SetFlag(SHARE_FLAG_ADDED);

    CDlgNewShare dlg(hwnd);
    dlg.m_pInfoList = _pInfoList;
    dlg.m_pReplaceList = _pReplaceList;
    dlg.m_pShareInfo = pNewInfo;
    if (dlg.DoModal())
    {
        _ReadControls(hwnd);    // read current stuff

        //
        // Add the new one to the list
        //

        pNewInfo->InsertBefore(_pInfoList); // add to end of list

        ++_cShares; // one more share in the list...

        //
        // Next, regenerate the UI
        //

        _SetControlsFromData(hwnd, pNewInfo->GetNetname());

        _MarkPageDirty();
    }
    else
    {
        // user hit "cancel"
        delete pNewInfo;
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
    //
    // Set defaults first
    //

    _SetControlsToDefaults(hwnd);

    //
    // From the cached list of shares, construct a list of shares that
    // corresponds to the current path.
    //

    HRESULT hr = _ConstructShareList();
    CHECK_HRESULT(hr);
    // BUGBUG: do anything on error?

    _fInitializingPage = TRUE;
    CheckRadioButton(
            hwnd,
            IDC_SHARE_NOTSHARED,
            IDC_SHARE_SHAREDAS,
            (_cShares > 0) ? IDC_SHARE_SHAREDAS : IDC_SHARE_NOTSHARED);
    _fInitializingPage = FALSE;

    _SetControlsFromData(hwnd, NULL);
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

    _HideControls(hwnd, 0);

    HWND hwndCombo = GetDlgItem(hwnd, IDC_SHARE_SHARENAME_COMBO);
    ComboBox_ResetContent(hwndCombo);

    SetDlgItemText(hwnd, IDC_SHARE_SHARENAME,   L"");
    SetDlgItemText(hwnd, IDC_SHARE_COMMENT,     L"");
    SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");

    _fInitializingPage = FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_ConnstructShareList, private
//
//  Synopsis:   Construct the list of shares for the current path by
//              iterating through the entire cache of shares and adding an
//              element for every path that matches.
//
//--------------------------------------------------------------------------

HRESULT
CSharingPropertyPage::_ConstructShareList(
    VOID
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appDebugOut((DEB_ITRACE, "_ConstructShareList\n"));

    // This routine sets _cShares, and _bNewShare, and adds to
    // the _pInfoList list

    HRESULT hr;

    DeleteShareInfoList(_pInfoList);
    _pCurInfo = NULL;

    appAssert(_pInfoList->IsSingle());
    appAssert(_pCurInfo == NULL);
    appAssert(_pszStoragePath != NULL);

    hr = _pShareCache->ConstructList(_pszStoragePath, _pInfoList, &_cShares);
    if (FAILED(hr))
    {
        return hr;
    }

    if (_cShares == 0)
    {
        // There are no existing shares. Construct an element to be used
        // by the UI to stash the new share's data

        _bNewShare = TRUE;

        CShareInfo* pNewInfo = new CShareInfo();
        if (NULL == pNewInfo)
        {
            return E_OUTOFMEMORY;
        }

        hr = pNewInfo->InitInstance();
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            delete pNewInfo;
            return hr;
        }

        hr = pNewInfo->SetPath(_pszStoragePath);
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            delete pNewInfo;
            return hr;
        }

        pNewInfo->SetFlag(SHARE_FLAG_ADDED);
        pNewInfo->InsertBefore(_pInfoList);

        //NOTE: leave the count of shares to be zero. The count of shares only
        // reflects the number of committed shares (?)

        // Give the share a default name. For paths like "X:\", use the default
        // "X", for paths like "X:\foo\bar\baz", use the default "baz".
        // For everything else, juse leave it blank. Also, check that the
        // computed default is not already a share name. If it is, leave it
        // blank.

        appAssert(NULL != _pInfo->pszPath);

        WCHAR szDefault[2] = L"";
        PWSTR pszDefault = szDefault;

        int len = wcslen(_pInfo->pszPath);
        if (len == 3 && _pInfo->pszPath[1] == L':' && _pInfo->pszPath[2] == L'\\')
        {
            szDefault[0] = _pInfo->pszPath[0];
        }
        else
        {
            PWSTR pszTmp = wcsrchr(_pInfo->pszPath, L'\\');
            if (pszTmp != NULL)
            {
                pszDefault = pszTmp + 1;
            }
        }

        if (_pShareCache->IsShareNameUsed(pszDefault))
        {
            pszDefault = szDefault;
            szDefault[0] = L'\0';
        }

        hr = pNewInfo->SetNetname(pszDefault);
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            // BUGBUG: error handling?
            delete pNewInfo;
            return hr;
        }
    }
    else
    {
        _bNewShare = FALSE;
    }

    return S_OK;
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
    IN HWND hwnd,
    IN PWSTR pszPreferredSelection
    )
{
    BOOL bIsShared = (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_SHAREDAS));

    _EnableControls(hwnd, bIsShared);

    if (bIsShared)
    {
        appDebugOut((DEB_ITRACE, "_SetControlsFromData: path is shared\n"));

        _HideControls(hwnd, _cShares);

        //
        // Now, set controls based on actual data
        //

        _fInitializingPage = TRUE;

        // if there is a new share, we only show the edit control for the
        // share name, not the combo box.

        if (_cShares == 0)
        {
            _pCurInfo = (CShareInfo*)_pInfoList->Next();
            appAssert(NULL != _pCurInfo);

            SetDlgItemText(hwnd, IDC_SHARE_SHARENAME, _pCurInfo->GetNetname());
        }
        else // (_cShares > 0)
        {
            // in the combo box, the "item data" is the CShareInfo pointer of
            // the item.

            // fill the combo.

            HWND hwndCombo = GetDlgItem(hwnd, IDC_SHARE_SHARENAME_COMBO);
            ComboBox_ResetContent(hwndCombo);

            for (CShareInfo* p = (CShareInfo*) _pInfoList->Next();
                 p != _pInfoList;
                 p = (CShareInfo*) p->Next())
            {
                if (p->GetFlag() != SHARE_FLAG_REMOVE)
                {
                    int item = ComboBox_AddString(hwndCombo, p->GetNetname());
                    if (CB_ERR == item || CB_ERRSPACE == item)
                    {
                        // BUGBUG: how to recover here?
                        break;
                    }

                    if (CB_ERR == ComboBox_SetItemData(hwndCombo, item, p))
                    {
                        // BUGBUG: how to recover here?
                    }
                }
            }

            int sel = 0;
            if (NULL != pszPreferredSelection)
            {
                sel = ComboBox_FindStringExact(hwndCombo, -1, pszPreferredSelection);
                if (CB_ERR == sel)
                {
                    sel = 0;
                }
            }
            ComboBox_SetCurSel(hwndCombo, sel);

            _pCurInfo = (CShareInfo*)ComboBox_GetItemData(hwndCombo, sel);
            appAssert(NULL != _pCurInfo);
        }

        _fInitializingPage = FALSE;

        // From the current item, set the rest of the fields

        _SetFieldsFromCurrent(hwnd);
    }
    else
    {
        appDebugOut((DEB_ITRACE, "_SetControlsFromData: path is not shared\n"));
        _pCurInfo = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_EnableControls, private
//
//  Synopsis:   Enable/disable controls
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_EnableControls(
    IN HWND hwnd,
    IN BOOL bEnable
    )
{
    CHECK_SIG(CSharingPropertyPage);

    int idControls[] =
    {
        IDC_SHARE_SHARENAME_TEXT,
        IDC_SHARE_SHARENAME,
        IDC_SHARE_SHARENAME_COMBO,
        IDC_SHARE_COMMENT_TEXT,
        IDC_SHARE_COMMENT,
        IDC_SHARE_LIMIT,
        IDC_SHARE_MAXIMUM,
        IDC_SHARE_ALLOW,
        IDC_SHARE_ALLOW_SPIN,
        IDC_SHARE_ALLOW_VALUE,
        IDC_SHARE_USERS_TEXT,
        IDC_SHARE_REMOVE,
        IDC_SHARE_NEWSHARE,
        IDC_SHARE_PERMISSIONS
    };

    for (int i = 0; i < ARRAYLEN(idControls); i++)
    {
        EnableWindow(GetDlgItem(hwnd, idControls[i]), bEnable);
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_HideControls, private
//
//  Synopsis:   Hide or show the controls based on the current number
//              of shares.
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_HideControls(
    IN HWND hwnd,
    IN int cShares
    )
{
    CHECK_SIG(CSharingPropertyPage);

    if (cShares <= 1)
    {
        ShowWindow(GetDlgItem(hwnd, IDC_SHARE_REMOVE),          SW_HIDE);
    }
    else
    {
        ShowWindow(GetDlgItem(hwnd, IDC_SHARE_REMOVE),          SW_SHOW);
    }

    if (cShares < 1)
    {
        ShowWindow(GetDlgItem(hwnd, IDC_SHARE_NEWSHARE),        SW_HIDE);

        ShowWindow(GetDlgItem(hwnd, IDC_SHARE_SHARENAME),       SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, IDC_SHARE_SHARENAME_COMBO), SW_HIDE);
    }
    else
    {
        ShowWindow(GetDlgItem(hwnd, IDC_SHARE_NEWSHARE),        SW_SHOW);

        ShowWindow(GetDlgItem(hwnd, IDC_SHARE_SHARENAME),       SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_SHARE_SHARENAME_COMBO), SW_SHOW);
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_SetFieldsFromCurrent, private
//
//  Synopsis:   From the currently selected share, set the property page
//              controls.
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_SetFieldsFromCurrent(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appAssert(NULL != _pCurInfo);

    _fInitializingPage = TRUE;

    SetDlgItemText(hwnd, IDC_SHARE_COMMENT, _pCurInfo->GetRemark());

    DWORD dwLimit = _pCurInfo->GetMaxUses();
    if (dwLimit == SHI_USES_UNLIMITED)
    {
        _wMaxUsers = DEFAULT_MAX_USERS;

        appDebugOut((DEB_ITRACE, "_SetFieldsFromCurrent: unlimited users\n"));

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
            "_SetFieldsFromCurrent: max users = %d\n",
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
//  Method:     CSharingPropertyPage::_ReadControls, private
//
//  Synopsis:   Load the data in the controls into internal data structures.
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_ReadControls(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    if (!_bItemDirty)
    {
        // nothing to read
        appDebugOut((DEB_ITRACE, "_ReadControls: item not dirty\n"));
        return TRUE;
    }

    appDebugOut((DEB_ITRACE, "_ReadControls: item dirty\n"));
    appAssert(NULL != _pCurInfo);

    if (_bShareNameChanged)
    {
        appDebugOut((DEB_ITRACE, "_ReadControls: share name changed\n"));

        WCHAR szShareName[NNLEN + 1];
        GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));
        _pCurInfo->SetNetname(szShareName);
        _bShareNameChanged = FALSE;
    }

    if (_bCommentChanged)
    {
        appDebugOut((DEB_ITRACE, "_ReadControls: comment changed\n"));

        WCHAR szComment[MAXCOMMENTSZ + 1];
        GetDlgItemText(hwnd, IDC_SHARE_COMMENT, szComment, ARRAYLEN(szComment));
        _pCurInfo->SetRemark(szComment);
        _bCommentChanged = FALSE;
    }

    if (_bUserLimitChanged)
    {
        appDebugOut((DEB_ITRACE, "_ReadControls: user limit changed\n"));

        if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_MAXIMUM))
        {
            _pCurInfo->SetMaxUses(SHI_USES_UNLIMITED);
        }
        else if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_ALLOW))
        {
            _CacheMaxUses(hwnd);
            _pCurInfo->SetMaxUses(_wMaxUsers);
        }
        _bUserLimitChanged = FALSE;
    }

    _bItemDirty = FALSE;

    return TRUE;
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
        if (!_bItemDirty)
        {
            appDebugOut((DEB_ITRACE, "Marking Sharing page---current item---dirty\n"));
            _bItemDirty = TRUE;
        }

        _MarkPageDirty();
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_MarkPageDirty, private
//
//  Synopsis:   A change has made such that the page is now dirty
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_MarkPageDirty(
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

    _ReadControls(hwnd);    // read current stuff

    if (_bNewShare
        && (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_SHAREDAS))
        )
    {
        // If the user is creating a share on the property sheet (as
        // opposed to using the "new share" dialog), we must validate the
        // share.... Note that _bNewShare is still TRUE if the the user has
        // clicked on "Not Shared", so we must check that too.

        // Validate the share

        appAssert(NULL != _pCurInfo);
        PWSTR pszShareName = _pCurInfo->GetNetname();
        appAssert(NULL != pszShareName);

        if (0 == wcslen(pszShareName))
        {
            MyErrorDialog(hwnd, IERR_BlankShareName);
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return FALSE;
        }

        HRESULT uTemp;
        if (!IsValidShareName(pszShareName, &uTemp))
        {
            MyErrorDialog(hwnd, uTemp);
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return FALSE;
        }

        // Trying to create a reserved share?
        if (0 == _wcsicmp(g_szIpcShare, pszShareName))
        {
            MyErrorDialog(hwnd, IERR_SpecialShare);
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return FALSE;
        }

        if (0 == _wcsicmp(g_szAdminShare, pszShareName))
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

            if (0 != _wcsicmp(_pCurInfo->GetPath(), szWindowsDir))
            {
                MyErrorDialog(hwnd, IERR_SpecialShare);
                SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
                return FALSE;
            }

            // otherwise, it is the right directory. Let them create it.
        }

        // Check for downlevel accessibility
        ULONG nType;
        if (NERR_Success != NetpPathType(_pInfo->pszServer, pszShareName, &nType, INPT_FLAGS_OLDPATHS))
        {
            DWORD id = MyConfirmationDialog(
                            hwnd,
                            IERR_InaccessibleByDos,
                            MB_YESNO | MB_ICONEXCLAMATION,
                            pszShareName);
            if (id == IDNO)
            {
                SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
                return FALSE;
            }
        }

        WCHAR szOldPath[PATHLEN+1];

        if (_pShareCache->IsExistingShare(pszShareName, _pCurInfo->GetPath(), szOldPath))
        {
            DWORD id = ConfirmReplaceShare(hwnd, pszShareName, szOldPath, _pCurInfo->GetPath());
            if (id != IDYES)
            {
                SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
                return FALSE;
            }

            // User said to replace the old share. We need to add
            // a "delete" record for the old share.

            HRESULT hr;

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

            // only need net name for delete; ignore other fields
            hr = pNewInfo->SetNetname(pszShareName);
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
            pNewInfo->InsertBefore(_pReplaceList); // add to end of replace list
        }
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
        if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_NOTSHARED))
        {
            // When the user hits "Not Shared" after having had "Shared As"
            // selected, we don't actually delete anything (or even mark it
            // for deletion). This is so the user can hit "Shared As" again
            // and they haven't lost anything. However, if they subsequently
            // apply changes, we must go through and actually delete anything
            // that they wanted to delete. So, fix up our list to do this.
            // Delete SHARE_FLAG_ADDED nodes, and mark all others as
            // SHARE_FLAG_REMOVE.

            for (CShareInfo* p = (CShareInfo*) _pInfoList->Next();
                 p != _pInfoList;
                 )
            {
                CShareInfo* pNext = (CShareInfo*)p->Next();

                if (p->GetFlag() == SHARE_FLAG_ADDED)
                {
                    // get rid of p
                    p->Unlink();
                    delete p;
                }
                else
                {
                    p->SetFlag(SHARE_FLAG_REMOVE);
                }

                p = pNext;
            }
        }
        else
        {
            // if we're deleting everything, don't bother reading what's
            // changed!
            _ReadControls(hwnd);
        }

        //
        // Commit the changes!
        //

        HRESULT hr;
        CShareInfo* p;

        // Delete all "replace" shares first. These are all the shares whos
        // names are being reused to share a different directory.
        // These replace deletes have already been confirmed.

        for (p = (CShareInfo*) _pReplaceList->Next();
             p != _pReplaceList;
             p = (CShareInfo*) p->Next())
        {
            appAssert(p->GetFlag() == SHARE_FLAG_REMOVE);
            NET_API_STATUS ret = p->Commit(_pInfo->pszServer);
            if (ret != NERR_Success)
            {
                DisplayError(hwnd, IERR_CANT_DEL_SHARE, ret, p->GetNetname());
                // We've got a problem here because we couldn't delete a
                // share that will be added subsequently. Oh well...
            }

            // Note that we don't delete this list because we need to notify
            // the shell (*after* all changes) that all these guys have
            // changed (and get rid of those little hands)...
        }

        // Now, do all add/delete/modify of the current

        for (p = (CShareInfo*) _pInfoList->Next();
             p != _pInfoList;
             )
        {
            CShareInfo* pNext = (CShareInfo*)p->Next();

            if (0 != p->GetFlag())
            {
                if (SHARE_FLAG_REMOVE == p->GetFlag())
                {
                    // confirm the delete, if there are connections
                    DWORD id = ConfirmStopShare(hwnd, MB_YESNO, p->GetNetname());
                    if (id != IDYES)
                    {
                        p->SetFlag(0);  // don't do anything to it
                    }
                }

                NET_API_STATUS ret = p->Commit(_pInfo->pszServer);
                if (ret != NERR_Success)
                {
                    HRESULT hrMsg = 0;
                    switch (p->GetFlag())
                    {
                    case SHARE_FLAG_ADDED:  hrMsg = IERR_CANT_ADD_SHARE;    break;
                    case SHARE_FLAG_MODIFY: hrMsg = IERR_CANT_MODIFY_SHARE; break;
                    case SHARE_FLAG_REMOVE: hrMsg = IERR_CANT_DEL_SHARE;    break;
                    default:
                        appAssert(!"Illegal flag for a failed commit!");
                    }
                    DisplayError(hwnd, hrMsg, ret, p->GetNetname());
                }
                else
                {
                    if (p->GetFlag() == SHARE_FLAG_REMOVE)
                    {
                        // nuke it!
                        p->Unlink();
                        delete p;
                    }
                    else
                    {
                        p->SetFlag(0);  // clear flag on success
                    }
                }
            }

            p = pNext;
        }

        if (_bNewShare)
        {
            _bNewShare = FALSE;
            _cShares = 1;
        }

        _bSecDescChanged = FALSE;
        _bDirty = FALSE;
        PropSheet_UnChanged(_GetFrameWindow(),_hwndPage);

        // I refresh the cache, even though the shell comes back and asks
        // for a refresh after every SHChangeNotify. However, SHChangeNotify
        // is asynchronous, and I need the cache refreshed immediately so I
        // can display the new share information, if the "apply" button was
        // hit and the page didn't go away.

        _pShareCache->Refresh(_pInfo->pszServer);

        appDebugOut((DEB_TRACE,
            "Changed share for path %ws, notifying shell\n",
            _pInfo->pszPath));

        SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH, _pInfo->pszPath, NULL);
        // BUGBUG: might want to use SHCNE_NETUNSHARE, but the shell doesn't
        // distinguish them

        // Now, notify the shell about all the paths we got rid of to be able
        // to use their share names ...
        for (p = (CShareInfo*) _pReplaceList->Next();
             p != _pReplaceList;
             )
        {
            appAssert(p->GetFlag() == SHARE_FLAG_REMOVE);

            appDebugOut((DEB_TRACE,
                "Got rid of share on path %ws, notifying shell\n",
                p->GetPath()));

            // We're going to be asked by the shell to refresh the cache once
            // for every notify. But, seeing as how the average case is zero
            // of these notifies, don't worry about it.
            SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH, p->GetPath(), NULL);

            CShareInfo* pNext = (CShareInfo*) p->Next();
            p->Unlink();
            delete p;
            p = pNext;
        }

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
        _bItemDirty = FALSE;
        _bShareNameChanged = FALSE;
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
"\t            Info: 0x%08lx\n"
"\t            Page: 0x%08lx\n"
"\t   Initializing?: %ws\n"
"\t          Dirty?: %ws\n"
"\t     Item dirty?: %ws\n"
"\t  Share changed?: %ws\n"
"\tComment changed?: %ws\n"
"\tUsr Lim changed?: %ws\n"
"\t        Max uses: %d\n"
"\t      _pInfoList: 0x%08lx\n"
"\t       _pCurInfo: 0x%08lx\n"
"\t          Shares: %d\n"
,
this,
_pInfo,
_hwndPage,
_fInitializingPage ? L"yes" : L"no",
_bDirty ? L"yes" : L"no",
_bItemDirty ? L"yes" : L"no",
_bShareNameChanged ? L"yes" : L"no",
_bCommentChanged ? L"yes" : L"no",
_bUserLimitChanged ? L"yes" : L"no",
_wMaxUsers,
_pInfoList,
_pCurInfo,
_cShares
));

    CShareInfo* p;

    for (p = (CShareInfo*) _pInfoList->Next();
         p != _pInfoList;
         p = (CShareInfo*) p->Next())
    {
        p->Dump(L"Prop page list");
    }

    for (p = (CShareInfo*) _pReplaceList->Next();
         p != _pReplaceList;
         p = (CShareInfo*) p->Next())
    {
        p->Dump(L"Replace list");
    }
}

#endif // DBG == 1
