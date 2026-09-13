/*
 * PROJECT:     ReactOS shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CopyTo and MoveTo implementation
 * COPYRIGHT:   Copyright 2020-2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

enum { IDC_ACTION = 0 };

CCopyMoveToMenu::CCopyMoveToMenu() :
    m_fnOldWndProc(NULL),
    m_bIgnoreTextBoxChange(FALSE)
{
}

static BOOL
IsValidTarget(CCopyMoveToMenu *pThis, LPCITEMIDLIST pidl)
{
    WCHAR szPath[MAX_PATH];

    if (ILIsEqual(pidl, pThis->m_pidlFolder))
        return pThis->GetFileOp() == FO_COPY;

    szPath[0] = UNICODE_NULL;
    SHGetPathFromIDListW(pidl, szPath);
    return _ILIsDesktop(pidl) || PathFileExistsW(szPath);
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR szPath[MAX_PATH];
    CCopyMoveToMenu *this_ =
        reinterpret_cast<CCopyMoveToMenu *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        if (!this_->m_bIgnoreTextBoxChange)
                        {
                            // get the text
                            GetDlgItemTextW(hwnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT, szPath, _countof(szPath));
                            StrTrimW(szPath, L" \t");

                            // update OK button
                            BOOL bValid = !PathIsRelative(szPath) && PathIsDirectoryW(szPath);
                            SendMessageW(hwnd, BFFM_ENABLEOK, 0, bValid);

                            return 0;
                        }

                        // reset flag
                        this_->m_bIgnoreTextBoxChange = FALSE;
                    }
                    break;
                }
            }
            break;
        }
    }
    return CallWindowProcW(this_->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);
}

static INT CALLBACK
BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    CCopyMoveToMenu *this_ = reinterpret_cast<CCopyMoveToMenu*>(lpData);

    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            // Set caption
            CString strCaption(MAKEINTRESOURCEW(this_->GetCaptionStringID()));
            SetWindowTextW(hwnd, strCaption);

            // Set OK button text
            CString strCopyOrMove(MAKEINTRESOURCEW(this_->GetButtonStringID()));
            SetDlgItemText(hwnd, IDOK, strCopyOrMove);

            // Subclassing
            // TODO: Replace this with BIF_VALIDATE
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lpData);
            this_->m_fnOldWndProc =
                reinterpret_cast<WNDPROC>(
                    SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));

            // Expand "My Computer"
            CComHeapPtr<ITEMIDLIST> drivesPidl(SHCloneSpecialIDList(hwnd, CSIDL_DRIVES, TRUE));
            if (drivesPidl)
                SendMessageW(hwnd, BFFM_SETEXPANDED, FALSE, (LPARAM)(LPITEMIDLIST)drivesPidl);

            // Select initial directory
            // TODO: Remember the last directory used by the user
            CComHeapPtr<ITEMIDLIST> pidl(SHCloneSpecialIDList(hwnd, CSIDL_PERSONAL, TRUE));
            if (pidl)
                SendMessageW(hwnd, BFFM_SETSELECTION, FALSE, (LPARAM)(LPITEMIDLIST)pidl);

            SendMessageW(hwnd, BFFM_ENABLEOK, 0, pidl && IsValidTarget(this_, pidl));
            break;
        }
        case BFFM_SELCHANGED:
        {
            LPCITEMIDLIST pidl = reinterpret_cast<LPCITEMIDLIST>(lParam);
            PostMessageW(hwnd, BFFM_ENABLEOK, 0, IsValidTarget(this_, pidl));

            // the text box will be updated later soon, ignore it
            this_->m_bIgnoreTextBoxChange = TRUE;
            break;
        }
    }

    return FALSE;
}

HRESULT
CCopyMoveToMenu::DoRealFileOp(const CIDA *pCIDA, LPCMINVOKECOMMANDINFO lpici, PCUIDLIST_ABSOLUTE pidlDestination)
{
    // TODO: Use SH32_SimulateDropWithSite instead
    CStringW strFiles;
    WCHAR szPath[MAX_PATH];
    for (UINT n = 0; n < pCIDA->cidl; ++n)
    {
        CComHeapPtr<ITEMIDLIST> pidlCombine(SHELL_CIDA_ILCloneFull(pCIDA, n));
        if (!pidlCombine)
            return E_FAIL;

        if (!SHGetPathFromIDListW(pidlCombine, szPath))
            return E_FAIL;

        if (n > 0)
            strFiles += L'|';
        strFiles += szPath;
    }

    strFiles += L'|'; // double null-terminated
    strFiles.Replace(L'|', L'\0');

    if (_ILIsDesktop(pidlDestination))
        SHGetSpecialFolderPathW(lpici->hwnd, szPath, CSIDL_DESKTOPDIRECTORY, TRUE);
    else
        SHGetPathFromIDListW(pidlDestination, szPath);
    INT cchPath = lstrlenW(szPath);
    if (cchPath + 1 < MAX_PATH)
    {
        szPath[cchPath + 1] = 0; // double null-terminated
    }
    else
    {
        ERR("Too long path\n");
        return E_FAIL;
    }

    SHFILEOPSTRUCTW op = { lpici->hwnd, GetFileOp(), strFiles, szPath };
    op.fFlags = FOF_ALLOWUNDO;
    int res = SHFileOperationW(&op);
    if (res)
    {
        ERR("SHFileOperationW failed with 0x%x\n", res);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT
DoGetFileTitle(const CIDA *pCIDA, CStringW& strTitle)
{
    CComHeapPtr<ITEMIDLIST> pidlCombine(SHELL_CIDA_ILCloneFull(pCIDA, 0));
    if (!pidlCombine)
        return E_OUTOFMEMORY;

    WCHAR szPath[MAX_PATH];
    HRESULT hr = SHGetNameAndFlagsW(pidlCombine, SHGDN_INFOLDER, szPath, _countof(szPath), NULL);
    strTitle = SUCCEEDED(hr) ? szPath : L"";
    if (strTitle.IsEmpty())
        return E_FAIL;

    if (pCIDA->cidl > 1)
        strTitle += L" ...";
    return S_OK;
}

HRESULT CCopyMoveToMenu::DoAction(LPCMINVOKECOMMANDINFO lpici)
{
    WCHAR wszPath[MAX_PATH];
    TRACE("(%p)\n", lpici);

    CDataObjectHIDA pCIDA(m_pDataObject);
    HRESULT hr = pCIDA.hr();
    if (FAILED_UNEXPECTEDLY(hr))
    {
        ERR("Failed to get CIDA, %#x\n", hr);
        return hr;
    }

    if (!SHGetPathFromIDListW(m_pidlFolder, wszPath))
    {
        ERR("SHGetPathFromIDListW failed\n");
        return E_FAIL;
    }

    CStringW strFileTitle;
    hr = DoGetFileTitle(pCIDA, strFileTitle);
    if (FAILED(hr))
        return hr;

    CStringW strTitle;
    strTitle.Format(GetActionTitleStringID(), static_cast<LPCWSTR>(strFileTitle));

    BROWSEINFOW info = { lpici->hwnd };
    info.pidlRoot = NULL;
    info.lpszTitle = strTitle;
    info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    info.lpfn = BrowseCallbackProc;
    info.lParam = reinterpret_cast<LPARAM>(this);
    CComHeapPtr<ITEMIDLIST> pidl(SHBrowseForFolder(&info));
    if (pidl)
        hr = DoRealFileOp(pCIDA, lpici, pidl);

    return hr;
}

static BOOL
GetPreviousMenuItemInfo(HMENU hMenu, UINT iItem, LPMENUITEMINFOW lpmii)
{
    BOOL bSuccess = FALSE;

    while (iItem > 0)
    {
        bSuccess = GetMenuItemInfoW(hMenu, --iItem, TRUE, lpmii);
        if (bSuccess || (!bSuccess && GetLastError() != ERROR_MENU_ITEM_NOT_FOUND))
            break;
    }

    return bSuccess;
}

STDMETHODIMP
CCopyToMenu::QueryContextMenu(HMENU hMenu,
                              UINT indexMenu,
                              UINT idCmdFirst,
                              UINT idCmdLast,
                              UINT uFlags)
{
    TRACE("CCopyToMenu::QueryContextMenu(%p, %u, %u, %u, %u)\n",
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    return QueryContextMenuImpl(TRUE, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
}

STDMETHODIMP
CMoveToMenu::QueryContextMenu(HMENU hMenu,
                              UINT indexMenu,
                              UINT idCmdFirst,
                              UINT idCmdLast,
                              UINT uFlags)
{
    TRACE("CMoveToMenu::QueryContextMenu(%p, %u, %u, %u, %u)\n",
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    return QueryContextMenuImpl(FALSE, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
}

STDMETHODIMP
CCopyMoveToMenu::QueryContextMenuImpl(BOOL IsCopyOp, HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (uFlags & (CMF_NOVERBS | CMF_VERBSONLY))
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);

    UINT idHighest = 0;
    CStringW strCopyTo(MAKEINTRESOURCEW(IDS_COPYTOMENU)), strMoveTo;
    LPWSTR itemText = strCopyTo.GetBuffer();
    if (!IsCopyOp)
    {
        strMoveTo.LoadString(IDS_MOVETOMENU);
        itemText = strMoveTo.GetBuffer();
    }

    // Insert separator if necessary
    WCHAR szBuff[128];
    MENUITEMINFOW mii = { sizeof(mii), MIIM_TYPE };
    mii.dwTypeData = szBuff;
    mii.cch = _countof(szBuff);
    if (GetPreviousMenuItemInfo(hMenu, indexMenu, &mii) &&
        mii.fType != MFT_SEPARATOR &&
        (IsCopyOp || !(mii.fType == MFT_STRING && !wcscmp(szBuff, strCopyTo.GetBuffer()))))
    {
        mii.fMask = MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        mii.dwTypeData = NULL;
        mii.cch = 0;
        if (InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
        {
            ++indexMenu;
        }
    }

    // Insert the menu item
    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.dwTypeData = itemText;
    mii.wID = idCmdFirst + IDC_ACTION;
    if (InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
    {
        idHighest = max(idHighest, mii.wID);
        ++indexMenu;
    }
    return idHighest >= idCmdFirst ? MAKE_HRESULT(SEVERITY_SUCCESS, 0, idHighest - idCmdFirst + 1) : E_FAIL;
}

STDMETHODIMP
CCopyMoveToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;
    TRACE("CCopyMoveToMenu::InvokeCommand(%p)\n", lpici);

    if (IS_INTRESOURCE(lpici->lpVerb))
    {
        if (LOWORD(lpici->lpVerb) == IDC_ACTION)
            hr = DoAction(lpici);
    }
    else
    {
        if (::lstrcmpiA(lpici->lpVerb, GetVerb()) == 0)
            hr = DoAction(lpici);
    }

    return hr;
}

STDMETHODIMP
CCopyMoveToMenu::GetCommandString(
    UINT_PTR idCmd,
    UINT uType,
    UINT *pwReserved,
    LPSTR pszName,
    UINT cchMax)
{
#if 1 // Windows does not implement any of these but we will continue to do so as long as we (incorrectly) show them in the context menu
    if ((uType | GCS_UNICODE) == GCS_VALIDATEW)
        return idCmd == IDC_ACTION ? S_OK : S_FALSE;

    if (uType == GCS_VERBW && idCmd == IDC_ACTION)
        return SHAnsiToUnicode(GetVerb(), (LPWSTR)pszName, cchMax);
#endif
    return E_NOTIMPL;
}

STDMETHODIMP
CCopyMoveToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("This %p uMsg %x\n", this, uMsg);
    return E_NOTIMPL;
}

STDMETHODIMP
CCopyMoveToMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    m_pDataObject = pdtobj;
    HRESULT hr = E_FAIL;
    if (pidlFolder)
    {
        hr = SHILClone(pidlFolder, const_cast<PIDLIST_ABSOLUTE*>(&pidlFolder));
    }
    else
    {
        pidlFolder = SHELL_DataObject_ILCloneFullItem(pdtobj, 0);
        if (pidlFolder)
        {
            ILRemoveLastID(const_cast<PIDLIST_ABSOLUTE>(pidlFolder));
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
        m_pidlFolder.Attach(const_cast<PIDLIST_ABSOLUTE>(pidlFolder));
    return hr;
}

STDMETHODIMP
CCopyMoveToMenu::SetSite(IUnknown *pUnkSite)
{
    m_pSite = pUnkSite;
    return S_OK;
}

STDMETHODIMP
CCopyMoveToMenu::GetSite(REFIID riid, void **ppvSite)
{
    if (!m_pSite)
        return E_FAIL;

    return m_pSite->QueryInterface(riid, ppvSite);
}

HRESULT
CCopyMoveToMenu::DoCopyMoveToFolder(BOOL Copy, HWND hWnd, IUnknown *pSite,
                                    IShellFolder *pSF, UINT cidl, PCUITEMID_CHILD_ARRAY pidls)
{
    SFGAOF attr = SFGAO_CANCOPY | SFGAO_CANMOVE;
    HRESULT hr = pSF->GetAttributesOf(cidl, pidls, &attr);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    if (!(attr & (Copy ? SFGAO_CANCOPY : SFGAO_CANMOVE)))
        return E_INVALIDARG;

    CComPtr<IDataObject> pDO;
    hr = pSF->GetUIObjectOf(hWnd, cidl, pidls, IID_NULL_PPV_ARG(IDataObject, &pDO));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    REFCLSID clsid = Copy ? CLSID_CopyToMenu : CLSID_MoveToMenu;
    CComPtr<IContextMenu> pCM;
    hr = SHELL_InitializeExtension(clsid, NULL, pDO, NULL, IID_PPV_ARG(IContextMenu, &pCM));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    IUnknown_SetSite(pCM, pSite);

    CMINVOKECOMMANDINFO ici = { sizeof(ici), 0, hWnd, MAKEINTRESOURCEA(IDC_ACTION), NULL, NULL, SW_SHOW };
    return pCM->InvokeCommand(&ici);
}
