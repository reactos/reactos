/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fonts folder view callback implementation
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(fontext);

void CFontFolderViewCB::Initialize(CFontExt* pFontExt, IShellView *psv, LPCITEMIDLIST pidlParent)
{
    ATLASSERT(pFontExt);
    ATLASSERT(psv);
    ATLASSERT(pidlParent);
    m_pFontExt = pFontExt;
    m_pShellView = psv;
    m_pidlParent.Attach(ILClone(pidlParent));
    if (!m_pidlParent)
        ERR("m_pidlParent was null\n");
}

BOOL CFontFolderViewCB::FilterEvent(PIDLIST_ABSOLUTE* apidls, LONG lEvent) const
{
    lEvent &= ~SHCNE_INTERRUPT;

    switch (lEvent)
    {
        case SHCNE_CREATE:
        case SHCNE_RENAMEITEM:
            break;
        case SHCNE_DELETE:
        {
            const FontPidlEntry* pEntry = _FontFromIL(apidls[0]);
            if (pEntry)
            {
                CStringW strFontName = pEntry->Name();

                HKEY hKey;
                LSTATUS error = RegOpenKeyExW(FONT_HIVE, FONT_KEY, 0, KEY_WRITE, &hKey);
                if (error == ERROR_SUCCESS)
                {
                    RegDeleteValueW(hKey, strFontName);
                    RegCloseKey(hKey);
                }
            }
            break;
        }
        case SHCNE_UPDATEDIR:
            // Refresh font cache and notify the system about the font change
            if (g_FontCache)
                g_FontCache->Read();
            break;
        default:
            return TRUE; // We don't want this event
    }

    return FALSE;
}

STDMETHODIMP
CFontFolderViewCB::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case SFVM_QUERYFSNOTIFY: // Registering change notification
        {
            if (!m_pShellView || !m_pFontExt)
                return E_FAIL;
            // Now, we can get the view window
            m_pShellView->GetWindow(&m_hwndView);
            m_pFontExt->SetViewWindow(m_hwndView);
            return S_OK;
        }
        case SFVM_FSNOTIFY: // Change notification
            if (FilterEvent((PIDLIST_ABSOLUTE*)wParam, (LONG)lParam))
                return S_FALSE; // Don't process
            return S_OK;
    }
    return E_NOTIMPL;
}
