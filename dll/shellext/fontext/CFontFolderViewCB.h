/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fonts folder view callback implementation
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

class CFontFolderViewCB
    : public CComObjectRootEx<CComMultiThreadModelNoCS>
    , public IShellFolderViewCB
{
    CFontExt* m_pFontExt;
    CComPtr<IShellView> m_pShellView;
    HWND m_hwndView = nullptr;
    CComHeapPtr<ITEMIDLIST> m_pidlParent;

    BOOL FilterEvent(PIDLIST_ABSOLUTE* apidls, LONG lEvent) const;

public:
    CFontFolderViewCB() { }
    void Initialize(CFontExt* pFontExt, IShellView *psv, LPCITEMIDLIST pidlParent);

    // IShellFolderViewCB
    STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CFontFolderViewCB)
    BEGIN_COM_MAP(CFontFolderViewCB)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
    END_COM_MAP()
};
