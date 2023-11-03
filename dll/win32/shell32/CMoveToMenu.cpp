/*
 * PROJECT:     shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CopyTo implementation
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

CMoveToMenu::CMoveToMenu()
{
}

HRESULT CMoveToMenu::DoRealMove(LPCMINVOKECOMMANDINFO lpici, LPCITEMIDLIST pidl)
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
    op.wFunc = FO_MOVE;
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

HRESULT CMoveToMenu::DoMoveToFolder(LPCMINVOKECOMMANDINFO lpici)
{
    WCHAR wszPath[MAX_PATH];
    HRESULT hr = E_FAIL;

    TRACE("DoMoveToFolder(%p)\n", lpici);

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
    strTitle.Format(IDS_MOVETOTITLE, static_cast<LPCWSTR>(strFileTitle));

    BROWSEINFOW info = { lpici->hwnd };
    info.pidlRoot = NULL;
    info.lpszTitle = strTitle;
    info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    info.lpfn = BrowseCallbackProc;
    info.lParam = reinterpret_cast<LPARAM>(this);
    CComHeapPtr<ITEMIDLIST> pidl(SHBrowseForFolder(&info));
    if (pidl)
    {
        hr = DoRealMove(lpici, pidl);
    }

    return hr;
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
        m_idCmdDoTo = m_idCmdLast++;
        ++indexMenu;
        ++Count;
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmdFirst + Count);
}

STDMETHODIMP
CMoveToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;
    TRACE("CMoveToMenu::InvokeCommand(%p)\n", lpici);

    if (IS_INTRESOURCE(lpici->lpVerb))
    {
        if (m_idCmdFirst + LOWORD(lpici->lpVerb) == m_idCmdDoTo)
        {
            hr = DoMoveToFolder(lpici);
        }
    }
    else
    {
        if (::lstrcmpiA(lpici->lpVerb, "moveto") == 0)
        {
            hr = DoMoveToFolder(lpici);
        }
    }

    return hr;
}
