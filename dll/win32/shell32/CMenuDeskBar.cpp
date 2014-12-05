#include "precomp.h"

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Popup(
    POINTL *ppt,
    RECTL *prcExclude,
    MP_POPUPFLAGS dwFlags)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnSelect(
    DWORD dwSelectType)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSubMenu(
    IMenuPopup *pmp,
    BOOL fSet)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetClient(
    IUnknown *punkClient)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetClient(
    IUnknown **ppunkClient)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnPosRectChangeDB(
    RECT *prc)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetWindow(
    HWND *phwnd)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::ContextSensitiveHelp(
    BOOL fEnterMode)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSite(
    IUnknown *pUnkSite)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetSite(
    REFIID riid,
    PVOID *ppvSite)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetIconSize(THIS_ DWORD iIcon)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetIconSize(THIS_ DWORD* piIcon)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetBitmap(THIS_ HBITMAP hBitmap)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetBitmap(THIS_ HBITMAP* phBitmap)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Initialize(THIS)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryStatus(
    const GUID *pguidCmdGroup,
    ULONG cCmds,
    OLECMD prgCmds[],
    OLECMDTEXT *pCmdText)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Exec(
    const GUID *pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANT *pvaIn,
    VARIANT *pvaOut)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryService(
    REFGUID guidService,
    REFIID riid,
    void **ppvObject)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnFocusChangeIS(THIS_ IUnknown * lpUnknown, BOOL bFocus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::UIActivateIO(THIS_ BOOL bActivating, LPMSG lpMsg)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::HasFocusIO(THIS) 
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::TranslateAcceleratorIO(THIS_ LPMSG lpMsg) 
{
    return S_OK;
}
