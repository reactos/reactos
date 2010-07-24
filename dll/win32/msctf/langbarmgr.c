/*
 *  ITfLangBarMgr implementation
 *
 *  Copyright 2010 Justin Chevrier
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

#define COBJMACROS

#include "wine/debug.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagLangBarMgr {
    const ITfLangBarMgrVtbl *LangBarMgrVtbl;

    LONG refCount;

} LangBarMgr;

static void LangBarMgr_Destructor(LangBarMgr *This)
{
    TRACE("destroying %p\n", This);

    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI LangBarMgr_QueryInterface(ITfLangBarMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    LangBarMgr *This = (LangBarMgr *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfLangBarMgr))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI LangBarMgr_AddRef(ITfLangBarMgr *iface)
{
    LangBarMgr *This = (LangBarMgr *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI LangBarMgr_Release(ITfLangBarMgr *iface)
{
    LangBarMgr *This = (LangBarMgr *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        LangBarMgr_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfLangBarMgr functions
 *****************************************************/

static HRESULT WINAPI LangBarMgr_AdviseEventSink( ITfLangBarMgr* iface, ITfLangBarEventSink *pSink, HWND hwnd, DWORD dwflags, DWORD *pdwCookie)
{
    LangBarMgr *This = (LangBarMgr *)iface;

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI LangBarMgr_UnAdviseEventSink( ITfLangBarMgr* iface, DWORD dwCookie)
{
    LangBarMgr *This = (LangBarMgr *)iface;

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI LangBarMgr_GetThreadMarshalInterface( ITfLangBarMgr* iface, DWORD dwThreadId, DWORD dwType, REFIID riid, IUnknown **ppunk)
{
    LangBarMgr *This = (LangBarMgr *)iface;

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI LangBarMgr_GetThreadLangBarItemMgr( ITfLangBarMgr* iface, DWORD dwThreadId, ITfLangBarItemMgr **pplbi, DWORD *pdwThreadid)
{
    LangBarMgr *This = (LangBarMgr *)iface;

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI LangBarMgr_GetInputProcessorProfiles( ITfLangBarMgr* iface, DWORD dwThreadId, ITfInputProcessorProfiles **ppaip, DWORD *pdwThreadid)
{
    LangBarMgr *This = (LangBarMgr *)iface;

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI LangBarMgr_RestoreLastFocus( ITfLangBarMgr* iface, DWORD *dwThreadId, BOOL fPrev)
{
    LangBarMgr *This = (LangBarMgr *)iface;

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI LangBarMgr_SetModalInput( ITfLangBarMgr* iface, ITfLangBarEventSink *pSink, DWORD dwThreadId, DWORD dwFlags)
{
    LangBarMgr *This = (LangBarMgr *)iface;

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI LangBarMgr_ShowFloating( ITfLangBarMgr* iface, DWORD dwFlags)
{
    LangBarMgr *This = (LangBarMgr *)iface;

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI LangBarMgr_GetShowFloatingStatus( ITfLangBarMgr* iface, DWORD *pdwFlags)
{
    LangBarMgr *This = (LangBarMgr *)iface;

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static const ITfLangBarMgrVtbl LangBarMgr_LangBarMgrVtbl =
{
    LangBarMgr_QueryInterface,
    LangBarMgr_AddRef,
    LangBarMgr_Release,

    LangBarMgr_AdviseEventSink,
    LangBarMgr_UnAdviseEventSink,
    LangBarMgr_GetThreadMarshalInterface,
    LangBarMgr_GetThreadLangBarItemMgr,
    LangBarMgr_GetInputProcessorProfiles,
    LangBarMgr_RestoreLastFocus,
    LangBarMgr_SetModalInput,
    LangBarMgr_ShowFloating,
    LangBarMgr_GetShowFloatingStatus
};

HRESULT LangBarMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    LangBarMgr *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),0,sizeof(LangBarMgr));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->LangBarMgrVtbl= &LangBarMgr_LangBarMgrVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    return S_OK;
}
