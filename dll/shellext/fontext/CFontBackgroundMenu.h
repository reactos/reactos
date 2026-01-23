/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fonts folder shell extension background menu
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class CFontBackgroundMenu
    : public CComObjectRootEx<CComMultiThreadModelNoCS>
    , public IContextMenu3
{
    HWND m_hwnd = nullptr;
    CFontExt* m_pFontExt = nullptr;
    CComPtr<IShellFolder> m_psf;
    CComPtr<IContextMenuCB> m_pmcb;
    LPFNDFMCALLBACK m_pfnmcb = nullptr;

public:
    CFontBackgroundMenu();
    virtual ~CFontBackgroundMenu();
    HRESULT WINAPI Initialize(CFontExt* pFontExt, const DEFCONTEXTMENU *pdcm);

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) override;
    STDMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen) override;

    // IContextMenu2
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // IContextMenu3
    STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) override;

    BEGIN_COM_MAP(CFontBackgroundMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu3, IContextMenu3)
    END_COM_MAP()
};
