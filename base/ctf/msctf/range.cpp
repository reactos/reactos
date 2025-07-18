/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     ITfRangeACP implementation
 * COPYRIGHT:   Copyright 2009 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "range.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////

CRange::CRange(
    _In_ ITfContext *context,
    _In_ TfAnchor anchorStart,
    _In_ TfAnchor anchorEnd
)
    : m_cRefs(1)
    , m_context(context)
    , m_anchorStart(anchorStart)
    , m_anchorEnd(anchorEnd)
{
    if (context)
        context->AddRef();
}

CRange::~CRange()
{
    TRACE("destroying %p\n", this);
    if (m_context)
        m_context->Release();
}

CRange *CRange::_Clone()
{
    CRange *pRange = new(cicNoThrow) CRange(m_context, m_anchorStart, m_anchorEnd);
    if (!pRange)
        return NULL;
    return pRange;
}

HRESULT CRange::_IsEqualX(TfEditCookie ec, BOOL bEnd, ITfRange *pWith, TfAnchor aPos, BOOL *pfEqual)
{
    return E_NOTIMPL;
}

HRESULT CRange::_CompareX(
    TfEditCookie ec,
    BOOL bEnd,
    ITfRange *pWidth,
    TfAnchor aPos,
    LONG *plResult)
{
    return E_NOTIMPL;
}

HRESULT CRange::TF_SELECTION_to_TS_SELECTION_ACP(const TF_SELECTION *tf, TS_SELECTION_ACP *tsAcp)
{
    if (!tf || !tsAcp || !tf->range)
    {
        ERR("E_INVALIDARG: %p, %p, %p\n", tf, tsAcp, tf->range);
        return E_INVALIDARG;
    }

    CRange *This = static_cast<CRange *>(static_cast<ITfRangeACP *>(tf->range));
    tsAcp->acpStart = This->m_anchorStart;
    tsAcp->acpEnd = This->m_anchorEnd;
    tsAcp->style.ase = static_cast<TsActiveSelEnd>(tf->style.ase);
    tsAcp->style.fInterimChar = tf->style.fInterimChar;
    return S_OK;
}

STDMETHODIMP CRange::QueryInterface(REFIID riid, void **ppvObj)
{
    if (riid == IID_PRIV_CRANGE)
    {
        *ppvObj = this;
        return S_OK; // No AddRef
    }

    if (riid == IID_IUnknown || riid == IID_ITfRange || riid == IID_ITfRangeACP)
        *ppvObj = static_cast<ITfRangeACP *>(this);
    else if (riid == IID_ITfRangeAnchor)
        *ppvObj = static_cast<ITfRangeAnchor *>(this);
    else if (riid == IID_ITfSource)
        *ppvObj = static_cast<ITfSource *>(this);
    else
        *ppvObj = NULL;
 
    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CRange::AddRef()
{
    TRACE("%p -> ()\n", this);
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CRange::Release()
{
    TRACE("%p -> ()\n", this);
    ULONG ret = InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CRange::GetText(
    _In_ TfEditCookie ec,
    _In_ DWORD dwFlags,
    _Out_ WCHAR *pchText,
    _In_ ULONG cchMax,
    _Out_ ULONG *pcch)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::SetText(
    _In_ TfEditCookie ec,
    _In_ DWORD dwFlags,
    _In_ const WCHAR *pchText,
    _In_ LONG cch)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::GetFormattedText(
    _In_ TfEditCookie ec,
    _Out_ IDataObject **ppDataObject)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::GetEmbedded(
    _In_ TfEditCookie ec,
    _In_ REFGUID rguidService,
    _In_ REFIID riid,
    _Out_ IUnknown **ppunk)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::InsertEmbedded(
    _In_ TfEditCookie ec,
    _In_ DWORD dwFlags,
    _In_ IDataObject *pDataObject)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::ShiftStart(
    _In_ TfEditCookie ec,
    _In_ LONG cchReq,
    _Out_ LONG *pcch,
    _In_ const TF_HALTCOND *pHalt)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::ShiftEnd(
    _In_ TfEditCookie ec,
    _In_ LONG cchReq,
    _Out_ LONG *pcch,
    _In_ const TF_HALTCOND *pHalt)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::ShiftStartToRange(
    _In_ TfEditCookie ec,
    _In_ ITfRange *pRange,
    _In_ TfAnchor aPos)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::ShiftEndToRange(
    _In_ TfEditCookie ec,
    _In_ ITfRange *pRange,
    _In_ TfAnchor aPos)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::ShiftStartRegion(
    _In_ TfEditCookie ec,
    _In_ TfShiftDir dir,
    _Out_ BOOL *pfNoRegion)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::ShiftEndRegion(
    _In_ TfEditCookie ec,
    _In_ TfShiftDir dir,
    _Out_ BOOL *pfNoRegion)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::IsEmpty(
    _In_ TfEditCookie ec,
    _Out_ BOOL *pfEmpty)
{
    TRACE("(%d, %p)\n", ec, pfEmpty);
    return IsEqualStart(ec, static_cast<ITfRangeACP *>(this), TF_ANCHOR_END, pfEmpty);
}

STDMETHODIMP CRange::Collapse(
    _In_ TfEditCookie ec,
    _In_ TfAnchor aPos)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::IsEqualStart(
    _In_ TfEditCookie ec,
    _In_ ITfRange *pWith,
    _In_ TfAnchor aPos,
    _Out_ BOOL *pfEqual)
{
    FIXME("\n");
    return _IsEqualX(ec, FALSE, pWith, aPos, pfEqual);
}

STDMETHODIMP CRange::IsEqualEnd(
    _In_ TfEditCookie ec,
    _In_ ITfRange *pWith,
    _In_ TfAnchor aPos,
    _Out_ BOOL *pfEqual)
{
    FIXME("\n");
    return _IsEqualX(ec, TRUE, pWith, aPos, pfEqual);
}

STDMETHODIMP CRange::CompareStart(
    _In_ TfEditCookie ec,
    _In_ ITfRange *pWith,
    _In_ TfAnchor aPos,
    _Out_ LONG *plResult)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::CompareEnd(
    _In_ TfEditCookie ec,
    _In_ ITfRange *pWith,
    _In_ TfAnchor aPos,
    _Out_ LONG *plResult)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::AdjustForInsert(
    _In_ TfEditCookie ec,
    _In_ ULONG cchInsert,
    _Out_ BOOL *pfInsertOk)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::GetGravity(
    _Out_ TfGravity *pgStart,
    _Out_ TfGravity *pgEnd)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::SetGravity(
    _In_ TfEditCookie ec,
    _In_ TfGravity gStart,
    _In_ TfGravity gEnd)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::Clone(
    _Out_ ITfRange **ppClone)
{
    TRACE("%p\n", ppClone);

    if (!ppClone)
        return E_INVALIDARG;

    CRange *pCloned = _Clone();
    if (!pCloned)
        return E_OUTOFMEMORY;

    *ppClone = static_cast<ITfRangeACP *>(pCloned);
    return S_OK;
}

STDMETHODIMP CRange::GetContext(
    _Out_ ITfContext **ppContext)
{
    FIXME("%p\n", ppContext);
    return E_NOTIMPL;
}

STDMETHODIMP CRange::GetExtent(_Out_ LONG *pacpAnchor, _Out_ LONG *pcch)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::SetExtent(_In_ LONG acpAnchor, _In_ LONG cch)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::GetExtent(_Out_ IAnchor **ppStart, _Out_ IAnchor **ppEnd)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::SetExtent(_In_ IAnchor *pAnchorStart, _In_ IAnchor *pAnchorEnd)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::AdviseSink(
    _In_ REFIID riid,
    _In_ IUnknown *punk,
    _Out_ DWORD *pdwCookie)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP CRange::UnadviseSink(
    _In_ DWORD dwCookie)
{
    FIXME("\n");
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////

EXTERN_C
HRESULT
Range_Constructor(ITfContext *context, DWORD anchorStart, DWORD anchorEnd, ITfRange **ppOut)
{
    CRange *This = new(cicNoThrow) CRange(context, (TfAnchor)anchorStart, (TfAnchor)anchorEnd);
    if (!This)
        return E_OUTOFMEMORY;

    *ppOut = static_cast<ITfRangeACP *>(This);
    return S_OK;
}

/* Internal conversion functions */

EXTERN_C
HRESULT TF_SELECTION_to_TS_SELECTION_ACP(const TF_SELECTION *tf, TS_SELECTION_ACP *tsAcp)
{
    return CRange::TF_SELECTION_to_TS_SELECTION_ACP(tf, tsAcp);
}
