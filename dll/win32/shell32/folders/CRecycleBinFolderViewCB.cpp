/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Recycle Bin virtual folder callback
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(CRecycleBin);

CRecycleBinFolderViewCB::CRecycleBinFolderViewCB()
    : m_pShellView(NULL)
    , m_pRecycleBin(NULL)
    , m_nChangeNotif(0)
{
}

CRecycleBinFolderViewCB::~CRecycleBinFolderViewCB()
{
    if (m_nChangeNotif)
        SHChangeNotifyDeregister(m_nChangeNotif);
}

void CRecycleBinFolderViewCB::Initialize(CRecycleBin *pRecycleBin, IShellView *psv, LPCITEMIDLIST pidlParent)
{
    m_pRecycleBin = pRecycleBin;
    m_pShellView = psv;
    m_pidlParent.Attach(ILClone(pidlParent));
}

HRESULT CRecycleBinFolderViewCB::RegisterChangeNotify(HWND hwndView)
{
    if (!hwndView)
    {
        WARN("!hwndView\n");
        return E_FAIL;
    }

    LPITEMIDLIST pidls[RECYCLEBINMAXDRIVECOUNT] = { NULL };
    SHChangeNotifyEntry entries[RECYCLEBINMAXDRIVECOUNT] = {};

    // Populate entries
    INT iEntry = 0;
    for (INT iDrive = 0; iDrive < RECYCLEBINMAXDRIVECOUNT; ++iDrive)
    {
        WCHAR szBinPath[MAX_PATH];
        HRESULT hr = GetRecycleBinPathFromDriveNumber(iDrive, szBinPath);
        if (FAILED(hr))
            continue;
        LPITEMIDLIST pidl = ILCreateFromPathW(szBinPath);
        if (!pidl)
            continue;
        entries[iEntry].pidl = pidls[iEntry] = pidl;
        entries[iEntry].fRecursive = FALSE;
        ++iEntry;
    }

    // Register
    const DWORD dwFlags = SHCNRF_InterruptLevel | SHCNRF_ShellLevel | SHCNRF_NewDelivery;
    const DWORD dwEvents =
        SHCNE_CREATE |
        SHCNE_DELETE |
        SHCNE_RENAMEITEM |
        SHCNE_UPDATEITEM |
        SHCNE_MKDIR |
        SHCNE_RMDIR |
        SHCNE_RENAMEFOLDER |
        SHCNE_UPDATEDIR |
        SHCNE_UPDATEIMAGE |
        SHCNE_ASSOCCHANGED;
    m_nChangeNotif = SHChangeNotifyRegister(hwndView, dwFlags, dwEvents, SHV_CHANGE_NOTIFY,
                                            iEntry, entries);
    if (!m_nChangeNotif)
        WARN("SHChangeNotifyRegister failed\n");

    // Clean up
    while (iEntry > 0)
        ILFree(pidls[--iEntry]);

    return m_nChangeNotif ? S_OK : E_FAIL;
}

HRESULT CRecycleBinFolderViewCB::TranslatePidl(LPITEMIDLIST *ppidlNew, LPCITEMIDLIST pidl)
{
    ATLASSERT(ppidlNew);
    *ppidlNew = NULL;

    WCHAR szPath[MAX_PATH];
    if (!SHGetPathFromIDListW(pidl, szPath))
    {
        ERR("!SHGetPathFromIDListW\n");
        return E_FAIL;
    }

    CComHeapPtr<ITEMIDLIST> pidlChild;
    ATLASSERT(m_pRecycleBin);
    m_pRecycleBin->ParseRecycleBinPath(szPath, NULL, &pidlChild, NULL);
    if (!pidlChild)
    {
        ERR("!pidlChild\n");
        return E_FAIL;
    }

    *ppidlNew = ILCombine(m_pidlParent, pidlChild);

    return *ppidlNew ? S_OK : E_OUTOFMEMORY;
}

void CRecycleBinFolderViewCB::TranslateTwoPIDLs(PIDLIST_ABSOLUTE* pidls)
{
    ATLASSERT(pidls);

    HRESULT hr;
    if (pidls[0])
    {
        m_pidls[0].Free();
        hr = TranslatePidl(&m_pidls[0], pidls[0]);
        if (!FAILED_UNEXPECTEDLY(hr))
            pidls[0] = m_pidls[0];
    }
    if (pidls[1])
    {
        m_pidls[1].Free();
        hr = TranslatePidl(&m_pidls[1], pidls[1]);
        if (!FAILED_UNEXPECTEDLY(hr))
            pidls[1] = m_pidls[1];
    }
}

STDMETHODIMP
CRecycleBinFolderViewCB::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case SFVM_QUERYFSNOTIFY: // Register change notification
        {
            // Now, we can get the view window
            ATLASSERT(m_pShellView);
            HWND hwndView;
            HRESULT hr = m_pShellView->GetWindow(&hwndView);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
            RegisterChangeNotify(hwndView);
            return S_OK;
        }
        case SFVM_FSNOTIFY: // Change notification
        {
            TranslateTwoPIDLs((PIDLIST_ABSOLUTE*)wParam);
            return S_OK;
        }
    }
    return E_NOTIMPL;
}
