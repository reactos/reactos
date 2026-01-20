/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fonts folder view callback implementation
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell_ad);

void CFontFolderViewCB::Initialize(CFontExt* pFontExt, IShellView *psv, LPCITEMIDLIST pidlParent)
{
    ATLASSERT(pFontExt);
    ATLASSERT(psv);
    ATLASSERT(pidlParent);
    m_pFontExt = pFontExt;
    m_pShellView = psv;
    m_pidlParent.Attach(ILClone(pidlParent));
    if (!m_pidlParent)
        ERR("!m_pidlParent\n");
}

HRESULT CFontFolderViewCB::TranslatePidl(LPITEMIDLIST* ppidlNew, LPCITEMIDLIST pidl)
{
    ATLASSERT(ppidlNew);

    *ppidlNew = NULL;

    WCHAR szFontFile[MAX_PATH];
    if (!SHGetPathFromIDListW(pidl, szFontFile))
        return E_FAIL;

    CStringW strFontName;
    HRESULT hr = DoGetFontTitle(szFontFile, strFontName);
    if (FAILED_UNEXPECTEDLY(hr))
        return E_FAIL;

    LPITEMIDLIST pidlChild = _ILCreate(strFontName);
    if (!pidlChild)
    {
        ERR("!pidlChild\n");
        return E_OUTOFMEMORY;
    }

    *ppidlNew = ILCombine(m_pidlParent, pidlChild);
    ILFree(pidlChild);

    return *ppidlNew ? S_OK : E_OUTOFMEMORY;
}

void CFontFolderViewCB::TranslateTwoPIDLs(PIDLIST_ABSOLUTE* pidls)
{
    ATLASSERT(pidls);

    HRESULT hr;
    if (pidls[0])
    {
        m_pidl0.Free();
        hr = TranslatePidl(&m_pidl0, pidls[0]);
        if (!FAILED_UNEXPECTEDLY(hr))
            pidls[0] = m_pidl0;
    }
    if (pidls[1])
    {
        m_pidl1.Free();
        hr = TranslatePidl(&m_pidl1, pidls[1]);
        if (!FAILED_UNEXPECTEDLY(hr))
            pidls[1] = m_pidl1;
    }
}

BOOL CFontFolderViewCB::FilterEvent(LONG lEvent) const
{
    switch (lEvent & ~SHCNE_INTERRUPT)
    {
        case SHCNE_CREATE:
        case SHCNE_DELETE:
        case SHCNE_RENAMEITEM:
        case SHCNE_UPDATEDIR:
            return FALSE; // OK
        default:
            return TRUE; // We don't want this event
    }
}

STDMETHODIMP
CFontFolderViewCB::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case SFVM_QUERYFSNOTIFY: // Registering change notification
        {
            // Now, we can get the view window
            ATLASSERT(m_pShellView);
            ATLASSERT(m_pFontExt);
            m_pShellView->GetWindow(&m_hwndView);
            m_pFontExt->SetViewWindow(m_hwndView);
            return S_OK;
        }
        case SFVM_FSNOTIFY: // Change notification
        {
            if (FilterEvent((LONG)lParam))
                return S_FALSE; // Don't process

            TranslateTwoPIDLs((PIDLIST_ABSOLUTE*)wParam);
            return S_OK;
        }
    }
    return E_NOTIMPL;
}
