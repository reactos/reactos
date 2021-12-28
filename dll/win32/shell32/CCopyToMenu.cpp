/*
 * PROJECT:     shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CopyTo implementation
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


CCopyToMenu::CCopyToMenu() :
    m_idCmdFirst(0),
    m_idCmdLast(0),
    m_idCmdCopyTo(-1),
    m_fnOldWndProc(NULL),
    m_bIgnoreTextBoxChange(FALSE)
{
}

CCopyToMenu::~CCopyToMenu()
{
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR szPath[MAX_PATH];
    CCopyToMenu *this_ =
        reinterpret_cast<CCopyToMenu *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

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

static int CALLBACK
BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    CCopyToMenu *this_ =
        reinterpret_cast<CCopyToMenu *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lpData);
            this_ = reinterpret_cast<CCopyToMenu *>(lpData);

            // Select initial directory
            SendMessageW(hwnd, BFFM_SETSELECTION, FALSE,
                reinterpret_cast<LPARAM>(static_cast<LPCITEMIDLIST>(this_->m_pidlFolder)));

            // Set caption
            CString strCaption(MAKEINTRESOURCEW(IDS_COPYITEMS));
            SetWindowTextW(hwnd, strCaption);

            // Set OK button text
            CString strCopy(MAKEINTRESOURCEW(IDS_COPYBUTTON));
            SetDlgItemText(hwnd, IDOK, strCopy);

            // Subclassing
            this_->m_fnOldWndProc =
                reinterpret_cast<WNDPROC>(
                    SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));

            // Disable OK
            PostMessageW(hwnd, BFFM_ENABLEOK, 0, FALSE);
            break;
        }
        case BFFM_SELCHANGED:
        {
            WCHAR szPath[MAX_PATH];
            LPCITEMIDLIST pidl = reinterpret_cast<LPCITEMIDLIST>(lParam);

            szPath[0] = 0;
            SHGetPathFromIDListW(pidl, szPath);

            if (ILIsEqual(pidl, this_->m_pidlFolder))
                PostMessageW(hwnd, BFFM_ENABLEOK, 0, FALSE);
            else if (PathFileExistsW(szPath) || _ILIsDesktop(pidl))
                PostMessageW(hwnd, BFFM_ENABLEOK, 0, TRUE);
            else
                PostMessageW(hwnd, BFFM_ENABLEOK, 0, FALSE);

            // the text box will be updated later soon, ignore it
            this_->m_bIgnoreTextBoxChange = TRUE;
            break;
        }
    }

    return FALSE;
}

HRESULT CCopyToMenu::DoRealCopy(LPCMINVOKECOMMANDINFO lpici, LPCITEMIDLIST pidl)
{
    CDataObjectHIDA pCIDA(m_pDataObject);
    if (FAILED_UNEXPECTEDLY(pCIDA.hr()))
        return pCIDA.hr();

    PCUIDLIST_ABSOLUTE pidlParent = HIDA_GetPIDLFolder(pCIDA);
    if (!pidlParent)
    {
        ERR("HIDA_GetPIDLFolder failed\n");
        return E_FAIL;
    }

    CStringW strFiles;
    WCHAR szPath[MAX_PATH];
    for (UINT n = 0; n < pCIDA->cidl; ++n)
    {
        PCUIDLIST_RELATIVE pidlRelative = HIDA_GetPIDLItem(pCIDA, n);
        if (!pidlRelative)
            continue;

        CComHeapPtr<ITEMIDLIST> pidlCombine(ILCombine(pidlParent, pidlRelative));
        if (!pidl)
            return E_FAIL;

        SHGetPathFromIDListW(pidlCombine, szPath);

        if (n > 0)
            strFiles += L'|';
        strFiles += szPath;
    }

    strFiles += L'|'; // double null-terminated
    strFiles.Replace(L'|', L'\0');

    if (_ILIsDesktop(pidl))
        SHGetSpecialFolderPathW(NULL, szPath, CSIDL_DESKTOPDIRECTORY, FALSE);
    else
        SHGetPathFromIDListW(pidl, szPath);
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

    SHFILEOPSTRUCTW op = { lpici->hwnd };
    op.wFunc = FO_COPY;
    op.pFrom = strFiles;
    op.pTo = szPath;
    op.fFlags = FOF_ALLOWUNDO;
    int res = SHFileOperationW(&op);
    if (res)
    {
        ERR("SHFileOperationW failed with 0x%x\n", res);
        return E_FAIL;
    }
    return S_OK;
}

CStringW CCopyToMenu::DoGetFileTitle()
{
    CStringW ret = L"(file)";

    CDataObjectHIDA pCIDA(m_pDataObject);
    if (FAILED_UNEXPECTEDLY(pCIDA.hr()))
        return ret;

    PCUIDLIST_ABSOLUTE pidlParent = HIDA_GetPIDLFolder(pCIDA);
    if (!pidlParent)
    {
        ERR("HIDA_GetPIDLFolder failed\n");
        return ret;
    }

    WCHAR szPath[MAX_PATH];
    PCUIDLIST_RELATIVE pidlRelative = HIDA_GetPIDLItem(pCIDA, 0);
    if (!pidlRelative)
    {
        ERR("HIDA_GetPIDLItem failed\n");
        return ret;
    }

    CComHeapPtr<ITEMIDLIST> pidlCombine(ILCombine(pidlParent, pidlRelative));

    if (SHGetPathFromIDListW(pidlCombine, szPath))
        ret = PathFindFileNameW(szPath);
    else
        ERR("Cannot get path\n");

    if (pCIDA->cidl > 1)
        ret += L" ...";

    return ret;
}

HRESULT CCopyToMenu::DoCopyToFolder(LPCMINVOKECOMMANDINFO lpici)
{
    WCHAR wszPath[MAX_PATH];
    HRESULT hr = E_FAIL;

    TRACE("DoCopyToFolder(%p)\n", lpici);

    if (!SHGetPathFromIDListW(m_pidlFolder, wszPath))
    {
        ERR("SHGetPathFromIDListW failed\n");
        return hr;
    }

    CStringW strFileTitle = DoGetFileTitle();
    CStringW strTitle;
    strTitle.Format(IDS_COPYTOTITLE, static_cast<LPCWSTR>(strFileTitle));

    BROWSEINFOW info = { lpici->hwnd };
    info.pidlRoot = NULL;
    info.lpszTitle = strTitle;
    info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    info.lpfn = BrowseCallbackProc;
    info.lParam = reinterpret_cast<LPARAM>(this);
    CComHeapPtr<ITEMIDLIST> pidl(SHBrowseForFolder(&info));
    if (pidl)
    {
        hr = DoRealCopy(lpici, pidl);
    }

    return hr;
}

HRESULT WINAPI
CCopyToMenu::QueryContextMenu(HMENU hMenu,
                              UINT indexMenu,
                              UINT idCmdFirst,
                              UINT idCmdLast,
                              UINT uFlags)
{
    MENUITEMINFOW mii;
    UINT Count = 0;

    TRACE("CCopyToMenu::QueryContextMenu(%p, %u, %u, %u, %u)\n",
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    m_idCmdFirst = m_idCmdLast = idCmdFirst;

    // insert separator if necessary
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE;
    if (GetMenuItemInfoW(hMenu, indexMenu - 1, TRUE, &mii) &&
        mii.fType != MFT_SEPARATOR)
    {
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        if (InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
        {
            ++indexMenu;
            ++Count;
        }
    }

    // insert "Copy to folder..."
    CStringW strText(MAKEINTRESOURCEW(IDS_COPYTOMENU));
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.dwTypeData = strText.GetBuffer();
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = m_idCmdLast;
    if (InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
    {
        m_idCmdCopyTo = m_idCmdLast++;
        ++indexMenu;
        ++Count;
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, Count);
}

HRESULT WINAPI
CCopyToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;
    TRACE("CCopyToMenu::InvokeCommand(%p)\n", lpici);

    if (HIWORD(lpici->lpVerb) == 0)
    {
        if (m_idCmdFirst + LOWORD(lpici->lpVerb) == m_idCmdCopyTo)
        {
            hr = DoCopyToFolder(lpici);
        }
    }
    else
    {
        if (::lstrcmpiA(lpici->lpVerb, "copyto") == 0)
        {
            hr = DoCopyToFolder(lpici);
        }
    }

    return hr;
}

HRESULT WINAPI
CCopyToMenu::GetCommandString(UINT_PTR idCmd,
                              UINT uType,
                              UINT *pwReserved,
                              LPSTR pszName,
                              UINT cchMax)
{
    FIXME("%p %lu %u %p %p %u\n", this,
          idCmd, uType, pwReserved, pszName, cchMax);

    return E_NOTIMPL;
}

HRESULT WINAPI
CCopyToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("This %p uMsg %x\n", this, uMsg);
    return E_NOTIMPL;
}

HRESULT WINAPI
CCopyToMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder,
                        IDataObject *pdtobj, HKEY hkeyProgID)
{
    m_pidlFolder.Attach(ILClone(pidlFolder));
    m_pDataObject = pdtobj;
    return S_OK;
}

HRESULT WINAPI CCopyToMenu::SetSite(IUnknown *pUnkSite)
{
    m_pSite = pUnkSite;
    return S_OK;
}

HRESULT WINAPI CCopyToMenu::GetSite(REFIID riid, void **ppvSite)
{
    if (!m_pSite)
        return E_FAIL;

    return m_pSite->QueryInterface(riid, ppvSite);
}
