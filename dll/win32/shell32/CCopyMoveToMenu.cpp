/*
 * PROJECT:     ReactOS shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CopyTo and MoveTo implementation
 * COPYRIGHT:   Copyright 2020-2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

CCopyMoveToMenu::CCopyMoveToMenu() :
    m_idCmdFirst(0),
    m_idCmdLast(0),
    m_idCmdAction(-1),
    m_fnOldWndProc(NULL),
    m_bIgnoreTextBoxChange(FALSE)
{
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
    CCopyMoveToMenu *this_ =
        reinterpret_cast<CCopyMoveToMenu *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lpData);
            this_ = reinterpret_cast<CCopyMoveToMenu *>(lpData);

            // Select initial directory
            SendMessageW(hwnd, BFFM_SETSELECTION, FALSE,
                reinterpret_cast<LPARAM>(static_cast<LPCITEMIDLIST>(this_->m_pidlFolder)));

            // Set caption
            CString strCaption(MAKEINTRESOURCEW(this_->GetCaptionStringID()));
            SetWindowTextW(hwnd, strCaption);

            // Set OK button text
            CString strCopyOrMove(MAKEINTRESOURCEW(this_->GetButtonStringID()));
            SetDlgItemText(hwnd, IDOK, strCopyOrMove);

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
            if (!this_)
                break;

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

HRESULT
CCopyMoveToMenu::DoRealFileOp(LPCMINVOKECOMMANDINFO lpici, LPCITEMIDLIST pidl)
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
DoGetFileTitle(CStringW& strTitle, IDataObject *pDataObject)
{
    CDataObjectHIDA pCIDA(pDataObject);
    if (FAILED_UNEXPECTEDLY(pCIDA.hr()))
        return E_FAIL;

    PCUIDLIST_ABSOLUTE pidlParent = HIDA_GetPIDLFolder(pCIDA);
    if (!pidlParent)
    {
        ERR("HIDA_GetPIDLFolder failed\n");
        return E_FAIL;
    }

    WCHAR szPath[MAX_PATH];
    PCUIDLIST_RELATIVE pidlRelative = HIDA_GetPIDLItem(pCIDA, 0);
    if (!pidlRelative)
    {
        ERR("HIDA_GetPIDLItem failed\n");
        return E_FAIL;
    }

    CComHeapPtr<ITEMIDLIST> pidlCombine(ILCombine(pidlParent, pidlRelative));

    if (!SHGetPathFromIDListW(pidlCombine, szPath))
    {
        ERR("Cannot get path\n");
        return E_FAIL;
    }

    strTitle = PathFindFileNameW(szPath);
    if (strTitle.IsEmpty())
        return E_FAIL;

    if (pCIDA->cidl > 1)
        strTitle += L" ...";

    return S_OK;
}

HRESULT CCopyMoveToMenu::DoAction(LPCMINVOKECOMMANDINFO lpici)
{
    WCHAR wszPath[MAX_PATH];
    HRESULT hr = E_FAIL;

    TRACE("(%p)\n", lpici);

    if (!SHGetPathFromIDListW(m_pidlFolder, wszPath))
    {
        ERR("SHGetPathFromIDListW failed\n");
        return hr;
    }

    CStringW strFileTitle;
    hr = DoGetFileTitle(strFileTitle, m_pDataObject);
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
        hr = DoRealFileOp(lpici, pidl);

    return hr;
}

STDMETHODIMP
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

    if (uFlags & (CMF_NOVERBS | CMF_VERBSONLY))
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmdFirst);

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
        m_idCmdAction = m_idCmdLast++;
        ++indexMenu;
        ++Count;
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmdFirst + Count);
}

STDMETHODIMP
CMoveToMenu::QueryContextMenu(HMENU hMenu,
                              UINT indexMenu,
                              UINT idCmdFirst,
                              UINT idCmdLast,
                              UINT uFlags)
{
    MENUITEMINFOW mii;
    UINT Count = 0;

    TRACE("CMoveToMenu::QueryContextMenu(%p, %u, %u, %u, %u)\n",
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (uFlags & (CMF_NOVERBS | CMF_VERBSONLY))
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmdFirst);

    m_idCmdFirst = m_idCmdLast = idCmdFirst;

    // insert separator if necessary
    CStringW strCopyTo(MAKEINTRESOURCEW(IDS_COPYTOMENU));
    WCHAR szBuff[128];
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE;
    mii.dwTypeData = szBuff;
    mii.cch = _countof(szBuff);
    if (GetMenuItemInfoW(hMenu, indexMenu - 1, TRUE, &mii) &&
        mii.fType != MFT_SEPARATOR &&
        !(mii.fType == MFT_STRING && CStringW(szBuff) == strCopyTo))
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

    // insert "Move to folder..."
    CStringW strText(MAKEINTRESOURCEW(IDS_MOVETOMENU));
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.dwTypeData = strText.GetBuffer();
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = m_idCmdLast;
    if (InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
    {
        m_idCmdAction = m_idCmdLast++;
        ++indexMenu;
        ++Count;
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmdFirst + Count);
}

STDMETHODIMP
CCopyMoveToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;
    TRACE("CCopyMoveToMenu::InvokeCommand(%p)\n", lpici);

    if (IS_INTRESOURCE(lpici->lpVerb))
    {
        if (m_idCmdFirst + LOWORD(lpici->lpVerb) == m_idCmdAction)
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
    FIXME("%p %lu %u %p %p %u\n", this,
          idCmd, uType, pwReserved, pszName, cchMax);

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
    m_pidlFolder.Attach(ILClone(pidlFolder));
    m_pDataObject = pdtobj;
    return S_OK;
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

static DWORD
SetClipboard(UINT cf, const void* data, SIZE_T size)
{
    BOOL succ = FALSE;
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, size);
    if (handle)
    {
        LPVOID clipdata = GlobalLock(handle);
        if (clipdata)
        {
            CopyMemory(clipdata, data, size);
            GlobalUnlock(handle);
            if (OpenClipboard(NULL))
            {
                EmptyClipboard();
                succ = SetClipboardData(cf, handle) != NULL;
                CloseClipboard();
            }
        }
        if (!succ)
        {
            GlobalFree(handle);
        }
    }
    return succ ? ERROR_SUCCESS : GetLastError();
}

static DWORD
SetClipboardFromString(LPCWSTR str)
{
    SIZE_T cch = lstrlenW(str) + 1, size = cch * sizeof(WCHAR);
    if (size > cch)
        return SetClipboard(CF_UNICODETEXT, str, size);
    else
        return ERROR_BUFFER_OVERFLOW;
}

static void
AppendToPathList(CStringW &paths, LPCWSTR path, DWORD index)
{
    if (index)
        paths += L"\r\n";
    LPCWSTR quote = StrChrW(path, L' ');
    if (quote)
        paths += L'\"';
    paths += path;
    if (quote)
        paths += L'\"';
}

STDMETHODIMP
CCopyAsPathMenu::Drop(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    CStringW paths;
    DWORD i, count;
#if 0
    CComPtr<IShellItemArray> array;
    HRESULT hr = SHCreateShellItemArrayFromDataObject(pdto, IID_PPV_ARG(IShellItemArray, &array));
    if (SUCCEEDED(hr))
    {
        for (i = 0, array->GetCount(&count); i < count && SUCCEEDED(hr); ++i)
        {
            CComPtr<IShellItem> item;
            hr = array->GetItemAt(i, &item);
            if (SUCCEEDED(hr))
            {
                CComHeapPtr<WCHAR> path;
                hr = item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &path);
                if (SUCCEEDED(hr))
                {
                    AppendToPathList(paths, path, i);
                }
            }
        }
    }
#else
    FIXME("Implement and use SHCreateShellItemArrayFromDataObject\n");
    CDataObjectHIDA pCIDA(pdto);
    HRESULT hr = pCIDA.hr();
    if (SUCCEEDED(hr))
    {
        for (i = 0, count = pCIDA->cidl; i < count && SUCCEEDED(hr); ++i)
        {
            PCUIDLIST_ABSOLUTE folder = HIDA_GetPIDLFolder(pCIDA);
            PCUIDLIST_RELATIVE item = HIDA_GetPIDLItem(pCIDA, i);
            CComHeapPtr<ITEMIDLIST> full;
            hr = SHILCombine(folder, item, &full);
            if (SUCCEEDED(hr))
            {
                PCUITEMID_CHILD child;
                CComPtr<IShellFolder> sf;
                hr = SHBindToParent(full, IID_PPV_ARG(IShellFolder, &sf), &child);
                if (SUCCEEDED(hr))
                {
                    STRRET strret;
                    hr = sf->GetDisplayNameOf(child, SHGDN_FORPARSING, &strret);
                    if (SUCCEEDED(hr))
                    {
                        CComHeapPtr<WCHAR> path;
                        hr = StrRetToStrW(&strret, child, &path);
                        if (SUCCEEDED(hr))
                        {
                            AppendToPathList(paths, path, i);
                        }
                    }
                }
            }
        }
    }
    else
    {
        FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stgm;
        hr = pdto->GetData(&fmte, &stgm);
        if (SUCCEEDED(hr))
        {
            for (i = 0, count = DragQueryFileW((HDROP)stgm.hGlobal, -1, NULL, 0); i < count && SUCCEEDED(hr); ++i)
            {
                WCHAR path[MAX_PATH];
                if (DragQueryFileW((HDROP)stgm.hGlobal, i, path, _countof(path)))
                {
                    AppendToPathList(paths, path, i);
                }
            }
            ReleaseStgMedium(&stgm);
        }
    }
#endif

    if (SUCCEEDED(hr))
    {
        DWORD err = SetClipboardFromString(paths);
        hr = HRESULT_FROM_WIN32(err);
    }

    if (SUCCEEDED(hr))
        *pdwEffect &= DROPEFFECT_COPY;
    else
        *pdwEffect &= DROPEFFECT_NONE;
    return hr;
}
