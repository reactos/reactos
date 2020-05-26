/*
 *  ITfRange implementation
 *
 *  Copyright 2009 Aric Stewart, CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagRange {
    ITfRange ITfRange_iface;
    /* const ITfRangeACPVtb *RangeACPVtbl; */
    LONG refCount;

    ITextStoreACP   *pITextStoreACP;
    ITfContext      *pITfContext;

    DWORD lockType;
    TfGravity gravityStart, gravityEnd;
    DWORD anchorStart, anchorEnd;

} Range;

static inline Range *impl_from_ITfRange(ITfRange *iface)
{
    return CONTAINING_RECORD(iface, Range, ITfRange_iface);
}

static void Range_Destructor(Range *This)
{
    TRACE("destroying %p\n", This);
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI Range_QueryInterface(ITfRange *iface, REFIID iid, LPVOID *ppvOut)
{
    Range *This = impl_from_ITfRange(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfRange))
    {
        *ppvOut = &This->ITfRange_iface;
    }

    if (*ppvOut)
    {
        ITfRange_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Range_AddRef(ITfRange *iface)
{
    Range *This = impl_from_ITfRange(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI Range_Release(ITfRange *iface)
{
    Range *This = impl_from_ITfRange(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        Range_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfRange functions
 *****************************************************/

static HRESULT WINAPI Range_GetText(ITfRange *iface, TfEditCookie ec,
        DWORD dwFlags, WCHAR *pchText, ULONG cchMax, ULONG *pcch)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_SetText(ITfRange *iface, TfEditCookie ec,
         DWORD dwFlags, const WCHAR *pchText, LONG cch)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_GetFormattedText(ITfRange *iface, TfEditCookie ec,
        IDataObject **ppDataObject)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_GetEmbedded(ITfRange *iface, TfEditCookie ec,
        REFGUID rguidService, REFIID riid, IUnknown **ppunk)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_InsertEmbedded(ITfRange *iface, TfEditCookie ec,
        DWORD dwFlags, IDataObject *pDataObject)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftStart(ITfRange *iface, TfEditCookie ec,
        LONG cchReq, LONG *pcch, const TF_HALTCOND *pHalt)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftEnd(ITfRange *iface, TfEditCookie ec,
        LONG cchReq, LONG *pcch, const TF_HALTCOND *pHalt)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftStartToRange(ITfRange *iface, TfEditCookie ec,
        ITfRange *pRange, TfAnchor aPos)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftEndToRange(ITfRange *iface, TfEditCookie ec,
        ITfRange *pRange, TfAnchor aPos)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftStartRegion(ITfRange *iface, TfEditCookie ec,
        TfShiftDir dir, BOOL *pfNoRegion)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_ShiftEndRegion(ITfRange *iface, TfEditCookie ec,
        TfShiftDir dir, BOOL *pfNoRegion)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_IsEmpty(ITfRange *iface, TfEditCookie ec,
        BOOL *pfEmpty)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_Collapse(ITfRange *iface, TfEditCookie ec,
        TfAnchor aPos)
{
    Range *This = impl_from_ITfRange(iface);
    TRACE("(%p) %i %i\n",This,ec,aPos);

    switch (aPos)
    {
        case TF_ANCHOR_START:
            This->anchorEnd = This->anchorStart;
            break;
        case TF_ANCHOR_END:
            This->anchorStart = This->anchorEnd;
            break;
        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI Range_IsEqualStart(ITfRange *iface, TfEditCookie ec,
        ITfRange *pWith, TfAnchor aPos, BOOL *pfEqual)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_IsEqualEnd(ITfRange *iface, TfEditCookie ec,
        ITfRange *pWith, TfAnchor aPos, BOOL *pfEqual)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_CompareStart(ITfRange *iface, TfEditCookie ec,
        ITfRange *pWith, TfAnchor aPos, LONG *plResult)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_CompareEnd(ITfRange *iface, TfEditCookie ec,
        ITfRange *pWith, TfAnchor aPos, LONG *plResult)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_AdjustForInsert(ITfRange *iface, TfEditCookie ec,
        ULONG cchInsert, BOOL *pfInsertOk)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_GetGravity(ITfRange *iface,
        TfGravity *pgStart, TfGravity *pgEnd)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_SetGravity(ITfRange *iface, TfEditCookie ec,
         TfGravity gStart, TfGravity gEnd)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_Clone(ITfRange *iface, ITfRange **ppClone)
{
    Range *This = impl_from_ITfRange(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Range_GetContext(ITfRange *iface, ITfContext **ppContext)
{
    Range *This = impl_from_ITfRange(iface);
    TRACE("(%p)\n",This);
    if (!ppContext)
        return E_INVALIDARG;
    *ppContext = This->pITfContext;
    return S_OK;
}

static const ITfRangeVtbl Range_RangeVtbl =
{
    Range_QueryInterface,
    Range_AddRef,
    Range_Release,

    Range_GetText,
    Range_SetText,
    Range_GetFormattedText,
    Range_GetEmbedded,
    Range_InsertEmbedded,
    Range_ShiftStart,
    Range_ShiftEnd,
    Range_ShiftStartToRange,
    Range_ShiftEndToRange,
    Range_ShiftStartRegion,
    Range_ShiftEndRegion,
    Range_IsEmpty,
    Range_Collapse,
    Range_IsEqualStart,
    Range_IsEqualEnd,
    Range_CompareStart,
    Range_CompareEnd,
    Range_AdjustForInsert,
    Range_GetGravity,
    Range_SetGravity,
    Range_Clone,
    Range_GetContext
};

HRESULT Range_Constructor(ITfContext *context, ITextStoreACP *textstore, DWORD lockType, DWORD anchorStart, DWORD anchorEnd, ITfRange **ppOut)
{
    Range *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(Range));
    if (This == NULL)
        return E_OUTOFMEMORY;

    TRACE("(%p) %p %p\n",This, context, textstore);

    This->ITfRange_iface.lpVtbl = &Range_RangeVtbl;
    This->refCount = 1;
    This->pITfContext = context;
    This->pITextStoreACP = textstore;
    This->lockType = lockType;
    This->anchorStart = anchorStart;
    This->anchorEnd = anchorEnd;

    *ppOut = &This->ITfRange_iface;
    TRACE("returning %p\n", *ppOut);

    return S_OK;
}

/* Internal conversion functions */

HRESULT TF_SELECTION_to_TS_SELECTION_ACP(const TF_SELECTION *tf, TS_SELECTION_ACP *tsAcp)
{
    Range *This;

    if (!tf || !tsAcp || !tf->range)
        return E_INVALIDARG;

    This = impl_from_ITfRange(tf->range);

    tsAcp->acpStart = This->anchorStart;
    tsAcp->acpEnd = This->anchorEnd;
    tsAcp->style.ase = tf->style.ase;
    tsAcp->style.fInterimChar = tf->style.fInterimChar;
    return S_OK;
}
