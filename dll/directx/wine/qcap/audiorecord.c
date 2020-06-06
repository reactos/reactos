/* Implementation of the Audio Capture Filter (CLSID_AudioRecord)
 *
 * Copyright 2015 Damjan Jovanovic
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

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"

#include "qcap_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct {
    IUnknown IUnknown_iface;
    IUnknown *outerUnknown;
    BaseFilter filter;
    IPersistPropertyBag IPersistPropertyBag_iface;
    BaseOutputPin *output;
} AudioRecord;

static inline AudioRecord *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, AudioRecord, IUnknown_iface);
}

static inline AudioRecord *impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, AudioRecord, filter);
}

static inline AudioRecord *impl_from_IBaseFilter(IBaseFilter *iface)
{
    BaseFilter *filter = CONTAINING_RECORD(iface, BaseFilter, IBaseFilter_iface);
    return impl_from_BaseFilter(filter);
}

static inline AudioRecord *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, AudioRecord, IPersistPropertyBag_iface);
}

static HRESULT WINAPI Unknown_QueryInterface(IUnknown *iface, REFIID riid, LPVOID *ppv)
{
    AudioRecord *This = impl_from_IUnknown(iface);
    if (IsEqualIID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown, %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    } else if (IsEqualIID(riid, &IID_IPersist)) {
        TRACE("(%p)->(IID_IPersist, %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    } else if (IsEqualIID(riid, &IID_IMediaFilter)) {
        TRACE("(%p)->(IID_IMediaFilter, %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    } else if (IsEqualIID(riid, &IID_IBaseFilter)) {
        TRACE("(%p)->(IID_IBaseFilter, %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    } else if (IsEqualIID(riid, &IID_IPersistPropertyBag)) {
        TRACE("(%p)->(IID_IPersistPropertyBag, %p)\n", This, ppv);
        *ppv = &This->IPersistPropertyBag_iface;
    } else {
        FIXME("(%p): no interface for %s\n", This, debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Unknown_AddRef(IUnknown *iface)
{
    AudioRecord *This = impl_from_IUnknown(iface);
    return BaseFilterImpl_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI Unknown_Release(IUnknown *iface)
{
    AudioRecord *This = impl_from_IUnknown(iface);
    ULONG ref = BaseFilterImpl_Release(&This->filter.IBaseFilter_iface);
    TRACE("(%p/%p)->() ref=%d\n", iface, This, ref);
    if (!ref) {
        CoTaskMemFree(This);
    }
    return ref;
}

static const IUnknownVtbl UnknownVtbl = {
    Unknown_QueryInterface,
    Unknown_AddRef,
    Unknown_Release
};

static HRESULT WINAPI AudioRecord_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    return IUnknown_QueryInterface(This->outerUnknown, riid, ppv);
}

static ULONG WINAPI AudioRecord_AddRef(IBaseFilter *iface)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    return IUnknown_AddRef(This->outerUnknown);
}

static ULONG WINAPI AudioRecord_Release(IBaseFilter *iface)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    return IUnknown_Release(This->outerUnknown);
}

static HRESULT WINAPI AudioRecord_Stop(IBaseFilter *iface)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioRecord_Pause(IBaseFilter *iface)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioRecord_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    FIXME("(%p, %s): stub\n", This, wine_dbgstr_longlong(tStart));
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioRecord_FindPin(IBaseFilter *iface, LPCWSTR Id, IPin **ppPin)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%s, %p): stub\n", This, debugstr_w(Id), ppPin);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl AudioRecordVtbl = {
    AudioRecord_QueryInterface,
    AudioRecord_AddRef,
    AudioRecord_Release,
    BaseFilterImpl_GetClassID,
    AudioRecord_Stop,
    AudioRecord_Pause,
    AudioRecord_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    AudioRecord_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static IPin* WINAPI AudioRecord_GetPin(BaseFilter *iface, int pos)
{
    AudioRecord *This = impl_from_BaseFilter(iface);
    FIXME("(%p, %d): stub\n", This, pos);
    return NULL;
}

static LONG WINAPI AudioRecord_GetPinCount(BaseFilter *iface)
{
    AudioRecord *This = impl_from_BaseFilter(iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

static const BaseFilterFuncTable AudioRecordFuncs = {
    AudioRecord_GetPin,
    AudioRecord_GetPinCount
};

static HRESULT WINAPI PPB_QueryInterface(IPersistPropertyBag *iface, REFIID riid, LPVOID *ppv)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    return IUnknown_QueryInterface(This->outerUnknown, riid, ppv);
}

static ULONG WINAPI PPB_AddRef(IPersistPropertyBag *iface)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    return IUnknown_AddRef(This->outerUnknown);
}

static ULONG WINAPI PPB_Release(IPersistPropertyBag *iface)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    return IUnknown_Release(This->outerUnknown);
}

static HRESULT WINAPI PPB_GetClassID(IPersistPropertyBag *iface, CLSID *pClassID)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    TRACE("(%p/%p)->(%p)\n", iface, This, pClassID);
    return IBaseFilter_GetClassID(&This->filter.IBaseFilter_iface, pClassID);
}

static HRESULT WINAPI PPB_InitNew(IPersistPropertyBag *iface)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p/%p)->(): stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PPB_Load(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        IErrorLog *pErrorLog)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    HRESULT hr;
    VARIANT var;
    static const WCHAR WaveInIDW[] = {'W','a','v','e','I','n','I','D',0};

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, pPropBag, pErrorLog);

    V_VT(&var) = VT_I4;
    hr = IPropertyBag_Read(pPropBag, WaveInIDW, &var, pErrorLog);
    if (SUCCEEDED(hr))
    {
        FIXME("FIXME: implement opening waveIn device %d\n", V_I4(&var));
    }

    return hr;
}

static HRESULT WINAPI PPB_Save(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        BOOL fClearDirty, BOOL fSaveAllProperties)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p/%p)->(%p, %u, %u): stub\n", iface, This, pPropBag, fClearDirty, fSaveAllProperties);
    return E_NOTIMPL;
}

static const IPersistPropertyBagVtbl PersistPropertyBagVtbl =
{
    PPB_QueryInterface,
    PPB_AddRef,
    PPB_Release,
    PPB_GetClassID,
    PPB_InitNew,
    PPB_Load,
    PPB_Save
};

IUnknown* WINAPI QCAP_createAudioCaptureFilter(IUnknown *outer, HRESULT *phr)
{
    HRESULT hr;
    AudioRecord *This = NULL;

    FIXME("(%p, %p): the entire CLSID_AudioRecord implementation is just stubs\n", outer, phr);

    This = CoTaskMemAlloc(sizeof(*This));
    if (This == NULL) {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    memset(This, 0, sizeof(*This));
    This->IUnknown_iface.lpVtbl = &UnknownVtbl;
    This->IPersistPropertyBag_iface.lpVtbl = &PersistPropertyBagVtbl;
    if (outer)
        This->outerUnknown = outer;
    else
        This->outerUnknown = &This->IUnknown_iface;

    hr = BaseFilter_Init(&This->filter, &AudioRecordVtbl, &CLSID_AudioRecord,
            (DWORD_PTR)(__FILE__ ": AudioRecord.csFilter"), &AudioRecordFuncs);

end:
    *phr = hr;
    if (SUCCEEDED(hr)) {
        return (IUnknown*)&This->filter.IBaseFilter_iface;
    } else {
        if (This)
            IBaseFilter_Release(&This->filter.IBaseFilter_iface);
        return NULL;
    }
}
