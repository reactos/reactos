/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHShouldShowWizards
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <undocshell.h>
#include <versionhelpers.h>

class CDummyClass
    : public IServiceProvider
    , public IShellBrowser
{
public:
    CDummyClass() { }

    IUnknown *GetUnknown()
    {
        return static_cast<IServiceProvider *>(this);
    }

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, VOID **ppvObj) override
    {
        if (riid == IID_IUnknown || riid == IID_IServiceProvider)
        {
            AddRef();
            *ppvObj = static_cast<IServiceProvider *>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return 1;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        return 1;
    }

    // *** IOleWindow methods ***
    STDMETHODIMP GetWindow(HWND *phwnd) override { return E_NOTIMPL; }
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) override { return E_NOTIMPL; }

    // *** IShellBrowser methods ***
    STDMETHODIMP InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) override { return E_NOTIMPL; }
    STDMETHODIMP SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject) override { return E_NOTIMPL; }
    STDMETHODIMP RemoveMenusSB(HMENU hmenuShared) override { return E_NOTIMPL; }
    STDMETHODIMP SetStatusTextSB(LPCOLESTR pszStatusText) override { return E_NOTIMPL; }
    STDMETHODIMP EnableModelessSB(BOOL fEnable) override { return E_NOTIMPL; }
    STDMETHODIMP TranslateAcceleratorSB(MSG *pmsg, WORD wID) override { return E_NOTIMPL; }
    STDMETHODIMP BrowseObject(LPCITEMIDLIST pidl, UINT wFlags) override { return E_NOTIMPL; }
    STDMETHODIMP GetViewStateStream(DWORD grfMode, IStream **ppStrm) override { return E_NOTIMPL; }
    STDMETHODIMP GetControlWindow(UINT id, HWND *lphwnd) override { return E_NOTIMPL; }
    STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret) override { return E_NOTIMPL; }
    STDMETHODIMP QueryActiveShellView(struct IShellView **ppshv) override { return E_NOTIMPL; }
    STDMETHODIMP OnViewWindowActive(struct IShellView *ppshv) override { return E_NOTIMPL; }
    STDMETHODIMP SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags) override { return E_NOTIMPL; }

    // *** IServiceProvider methods ***
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObject) override
    {
        if (riid == IID_IShellBrowser)
        {
            AddRef();
            *ppvObject = static_cast<IShellBrowser *>(this);
            return S_OK;
        }
        return E_FAIL;
    }
};

static VOID SetShowWizardsTEST(BOOL bValue)
{
    DWORD dwValue = bValue;
    SHRegSetUSValueW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                     L"ShowWizardsTEST", REG_DWORD, &dwValue, sizeof(dwValue), SHREGSET_FORCE_HKCU);
}

START_TEST(SHShouldShowWizards)
{
    // Save old values
    SHELLSTATE state;
    SHGetSetSettings(&state, SSF_WEBVIEW, FALSE);
    BOOL bOldWebView = state.fWebView;
    BOOL bOldTestValue = SHRegGetBoolUSValueW(
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
        L"ShowWizardsTEST",
        FALSE,
        FALSE);

    CDummyClass dummy;
    HRESULT hr;
    const BOOL bVistaPlus = IsWindowsVistaOrGreater();

    state.fWebView = FALSE;
    SHGetSetSettings(&state, SSF_WEBVIEW, TRUE);
    SetShowWizardsTEST(FALSE);
    hr = SHShouldShowWizards(dummy.GetUnknown());
    ok_hex(hr, bVistaPlus ? S_FALSE : S_OK);

    SetShowWizardsTEST(TRUE);
    hr = SHShouldShowWizards(dummy.GetUnknown());
    ok_hex(hr, bVistaPlus ? S_FALSE : S_OK);

    state.fWebView = TRUE;
    SHGetSetSettings(&state, SSF_WEBVIEW, TRUE);
    SetShowWizardsTEST(FALSE);
    hr = SHShouldShowWizards(dummy.GetUnknown());
    ok_hex(hr, S_FALSE);

    SetShowWizardsTEST(TRUE);
    hr = SHShouldShowWizards(dummy.GetUnknown());
    ok_hex(hr, bVistaPlus ? S_FALSE : S_OK);

    // Restore old values
    state.fWebView = bOldWebView;
    SHGetSetSettings(&state, SSF_WEBVIEW, TRUE);
    SetShowWizardsTEST(bOldTestValue);
}
