/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     ITfRangeACP implementation
 * COPYRIGHT:   Copyright 2009 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <initguid.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <msctf.h>
#include <msctf_undoc.h>

// Cicero
#include <cicbase.h>
#include <cicreg.h>
#include <cicutb.h>

#include "inputcontext.h"
#include "msctf_internal.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

STDMETHODIMP CInputContext::QueryInterface(REFIID riid, void **ppvObj)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CInputContext::AddRef()
{
    return 2;
}

STDMETHODIMP_(ULONG) CInputContext::Release()
{
    return 2;
}

STDMETHODIMP CInputContext::RequestEditSession(
    _In_ TfClientId tid,
    _In_ ITfEditSession *pes,
    _In_ DWORD dwFlags,
    _Out_ HRESULT *phrSession)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::InWriteSession(
    _In_ TfClientId tid,
    _Out_ BOOL *pfWriteSession)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::GetSelection(
    _In_ TfEditCookie ec,
    _In_ ULONG ulIndex,
    _In_ ULONG ulCount,
    _Out_ TF_SELECTION *pSelection,
    _Out_ ULONG *pcFetched)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::SetSelection(
    _In_ TfEditCookie ec,
    _In_ ULONG ulCount,
    _In_ const TF_SELECTION *pSelection)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::GetStart(
    _In_ TfEditCookie ec,
    _Out_ ITfRange **ppStart)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::GetEnd(
    _In_ TfEditCookie ec,
    _Out_ ITfRange **ppEnd)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::GetActiveView(_Out_ ITfContextView **ppView)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::EnumViews(_Out_ IEnumTfContextViews **ppEnum)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::GetStatus(_Out_ TF_STATUS *pdcs)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::GetProperty(
    _In_ REFGUID guidProp,
    _Out_ ITfProperty **ppProp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::GetAppProperty(
    _In_ REFGUID guidProp,
    _Out_ ITfReadOnlyProperty **ppProp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::TrackProperties(
    _In_ const GUID **prgProp,
    _In_ ULONG cProp,
    _In_ const GUID **prgAppProp,
    _In_ ULONG cAppProp,
    _Out_ ITfReadOnlyProperty **ppProperty)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::EnumProperties(
    _Out_ IEnumTfProperties **ppEnum)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::GetDocumentMgr(
    _Out_ ITfDocumentMgr **ppDm)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CInputContext::CreateRangeBackup(
    _In_ TfEditCookie ec,
    _In_ ITfRange *pRange,
    _Out_ ITfRangeBackup **ppBackup)
{
    FIXME("\n");
    return E_NOTIMPL;
}
