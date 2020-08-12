#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static HRESULT
_GetCidlFromDataObject(IDataObject *pDataObject, CIDA** ppcida)
{
    static CLIPFORMAT s_cfHIDA = 0;
    if (s_cfHIDA == 0)
    {
        s_cfHIDA = (CLIPFORMAT)RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
    }

    FORMATETC fmt = { s_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium;

    HRESULT hr = pDataObject->GetData(&fmt, &medium);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    LPVOID lpSrc = GlobalLock(medium.hGlobal);
    SIZE_T cbSize = GlobalSize(medium.hGlobal);

    *ppcida = (CIDA *)::CoTaskMemAlloc(cbSize);
    if (*ppcida)
    {
        memcpy(*ppcida, lpSrc, cbSize);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }
    ReleaseStgMedium(&medium);
    return hr;
}

CCopyToMenu::CCopyToMenu() :
    m_idCmdFirst(0),
    m_idCmdLast(0),
    m_idCmdCopyTo(-1)
{
}

CCopyToMenu::~CCopyToMenu()
{
}

static int CALLBACK
BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    CCopyToMenu *this_ =
        reinterpret_cast<CCopyToMenu *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    LPCITEMIDLIST pidl;
    WCHAR szPath[MAX_PATH];

    switch (uMsg)
    {
        case BFFM_INITIALIZED:
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lpData);
            this_ = reinterpret_cast<CCopyToMenu *>(lpData);

            // Select initial directory
            SendMessageW(hwnd, BFFM_SETSELECTION, FALSE,
                reinterpret_cast<LPARAM>(static_cast<LPCITEMIDLIST>(this_->m_pidlFolder)));
            break;

        case BFFM_SELCHANGED:
            pidl = reinterpret_cast<LPCITEMIDLIST>(lParam);
            if (SHGetPathFromIDListW(pidl, szPath) && PathFileExistsW(szPath) &&
                !ILIsEqual(pidl, this_->m_pidlFolder))
            {
                SendMessageW(hwnd, BFFM_ENABLEOK, 0, TRUE);
            }
            else
            {
                SendMessageW(hwnd, BFFM_ENABLEOK, 0, FALSE);
            }
            break;
    }

    return FALSE;
}

HRESULT CCopyToMenu::DoRealCopy(LPCMINVOKECOMMANDINFO lpici, LPCITEMIDLIST pidl)
{
    CComHeapPtr<CIDA> pCIDA;
    HRESULT hr = _GetCidlFromDataObject(m_pDataObject, &pCIDA);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    PCUIDLIST_ABSOLUTE pidlParent = HIDA_GetPIDLFolder(pCIDA);
    if (!pidlParent)
        return E_FAIL;

    CStringW strFiles;
    WCHAR szPath[MAX_PATH];
    for (UINT n = 0; n < pCIDA->cidl; ++n)
    {
        PCUIDLIST_RELATIVE pidlRelative = HIDA_GetPIDLItem(pCIDA, n);
        if (!pidlRelative)
            continue;

        PIDLIST_ABSOLUTE pidlCombine = ILCombine(pidlParent, pidlRelative);
        if (!pidl)
            return E_FAIL;

        SHGetPathFromIDListW(pidlCombine, szPath);
        ILFree(pidlCombine);

        if (n > 0)
            strFiles += L'|';
        strFiles += szPath;
    }

    strFiles += L'|'; // double null-terminated
    strFiles.Replace(L'|', L'\0');

    SHGetPathFromIDListW(pidl, szPath);
    INT cchPath = lstrlenW(szPath);
    if (cchPath + 1 < MAX_PATH)
        szPath[cchPath + 1] = 0; // double null-terminated
    else
        return E_FAIL;

    SHFILEOPSTRUCTW op = { lpici->hwnd };
    op.wFunc = FO_COPY;
    op.pFrom = strFiles;
    op.pTo = szPath;
    op.fFlags = FOF_ALLOWUNDO;
    return ((SHFileOperation(&op) == 0) ? S_OK : E_FAIL);
}

HRESULT CCopyToMenu::DoCopyToFolder(LPCMINVOKECOMMANDINFO lpici)
{
    WCHAR wszPath[MAX_PATH];
    HRESULT hr;

    ERR("DoCopyToFolder(%p)\n", lpici);

    hr = SHGetPathFromIDListW(m_pidlFolder, wszPath);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CStringW strTitle(MAKEINTRESOURCEW(IDS_COPYTOTITLE));

    BROWSEINFOW info = { lpici->hwnd };
    info.pidlRoot = NULL;
    info.lpszTitle = strTitle;
    info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    info.lpfn = BrowseCallbackProc;
    info.lParam = (LPARAM)this;
    CComHeapPtr<ITEMIDLIST> pidl(SHBrowseForFolder(&info));
    if (pidl)
    {
        hr = DoRealCopy(lpici, pidl);
    }

    return E_FAIL;
}

HRESULT WINAPI
CCopyToMenu::QueryContextMenu(HMENU hMenu,
                              UINT indexMenu,
                              UINT idCmdFirst,
                              UINT idCmdLast,
                              UINT uFlags)
{
    MENUITEMINFOW mii;
    WCHAR wszBuf[200];
    UINT Pos = ::GetMenuItemCount(hMenu);

    ERR("CCopyToMenu::QueryContextMenu(%p, %u, %u, %u, %u)\n",
        hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    m_idCmdFirst = m_idCmdLast = idCmdFirst;

    if (!LoadStringW(shell32_hInstance, IDS_COPYTOMENU, wszBuf, _countof(wszBuf)))
        wszBuf[0] = 0;

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STRING | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.dwTypeData = wszBuf;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idCmdLast;
    mii.fState = MFS_ENABLED;
    if (InsertMenuItemW(hMenu, Pos++, TRUE, &mii))
    {
        m_idCmdCopyTo = m_idCmdLast++;
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, m_idCmdLast - m_idCmdFirst + 1);
}

HRESULT WINAPI
CCopyToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;
    ERR("CCopyToMenu::InvokeCommand(%p)\n", lpici);

    if (HIWORD(lpici->lpVerb) == 0)
    {
        ERR("%u, %u, %u\n", m_idCmdFirst, m_idCmdCopyTo, LOWORD(lpici->lpVerb));
        if (m_idCmdFirst + LOWORD(lpici->lpVerb) == m_idCmdCopyTo)
        {
            hr = DoCopyToFolder(lpici);
        }
    }
    else
    {
        ERR("'%s'\n", lpici->lpVerb);
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
