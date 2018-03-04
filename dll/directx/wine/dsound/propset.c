/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winternl.h"
#include "winnls.h"
#include "vfwmsgs.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsound_private.h"
#include "dsconf.h"

#include "ksmedia.h"
#include "propkey.h"
#include "devpkey.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

static WCHAR wInterface[] = { 'I','n','t','e','r','f','a','c','e',0 };

typedef struct IKsPrivatePropertySetImpl
{
    IKsPropertySet IKsPropertySet_iface;
    LONG ref;
} IKsPrivatePropertySetImpl;

static IKsPrivatePropertySetImpl *impl_from_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, IKsPrivatePropertySetImpl, IKsPropertySet_iface);
}

/*******************************************************************************
 *              IKsPrivatePropertySet
 */

/* IUnknown methods */
static HRESULT WINAPI IKsPrivatePropertySetImpl_QueryInterface(
        IKsPropertySet *iface, REFIID riid, void **ppobj)
{
    IKsPrivatePropertySetImpl *This = impl_from_IKsPropertySet(iface);
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IKsPropertySet)) {
        *ppobj = iface;
        IKsPropertySet_AddRef(iface);
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IKsPrivatePropertySetImpl_AddRef(LPKSPROPERTYSET iface)
{
    IKsPrivatePropertySetImpl *This = impl_from_IKsPropertySet(iface);
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IKsPrivatePropertySetImpl_Release(LPKSPROPERTYSET iface)
{
    IKsPrivatePropertySetImpl *This = impl_from_IKsPropertySet(iface);
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
	TRACE("(%p) released\n", This);
    }
    return ref;
}

struct search_data {
    const WCHAR *tgt_name;
    GUID *found_guid;
};

static BOOL CALLBACK search_callback(GUID *guid, const WCHAR *desc,
        const WCHAR *module, void *user)
{
    struct search_data *search = user;

    if(!lstrcmpW(desc, search->tgt_name)){
        *search->found_guid = *guid;
        return FALSE;
    }

    return TRUE;
}

static HRESULT DSPROPERTY_WaveDeviceMappingW(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    HRESULT hr;
    PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA ppd = pPropData;
    struct search_data search;

    TRACE("(pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
          pPropData,cbPropData,pcbReturned);

    if (!ppd) {
        WARN("invalid parameter: pPropData\n");
        return DSERR_INVALIDPARAM;
    }

    search.tgt_name = ppd->DeviceName;
    search.found_guid = &ppd->DeviceId;

    if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER)
        hr = enumerate_mmdevices(eRender, DSOUND_renderer_guids,
                search_callback, &search);
    else if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE)
        hr = enumerate_mmdevices(eCapture, DSOUND_capture_guids,
                search_callback, &search);
    else
        return DSERR_INVALIDPARAM;

    if(hr != S_FALSE)
        /* device was not found */
        return DSERR_INVALIDPARAM;

    if (pcbReturned)
        *pcbReturned = cbPropData;

    return DS_OK;
}

static HRESULT DSPROPERTY_WaveDeviceMappingA(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA *ppd = pPropData;
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA data;
    DWORD len;
    HRESULT hr;

    TRACE("(pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
      pPropData,cbPropData,pcbReturned);

    if (!ppd || !ppd->DeviceName) {
        WARN("invalid parameter: ppd=%p\n", ppd);
        return DSERR_INVALIDPARAM;
    }

    data.DataFlow = ppd->DataFlow;
    len = MultiByteToWideChar(CP_ACP, 0, ppd->DeviceName, -1, NULL, 0);
    data.DeviceName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!data.DeviceName)
        return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, ppd->DeviceName, -1, data.DeviceName, len);

    hr = DSPROPERTY_WaveDeviceMappingW(&data, cbPropData, pcbReturned);
    HeapFree(GetProcessHeap(), 0, data.DeviceName);
    ppd->DeviceId = data.DeviceId;

    if (pcbReturned)
        *pcbReturned = cbPropData;

    return hr;
}

static HRESULT DSPROPERTY_DescriptionW(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA ppd = pPropData;
    GUID dev_guid;
    IMMDevice *mmdevice;
    IPropertyStore *ps;
    PROPVARIANT pv;
    HRESULT hr;

    TRACE("pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
          pPropData,cbPropData,pcbReturned);

    TRACE("DeviceId=%s\n",debugstr_guid(&ppd->DeviceId));
    if ( IsEqualGUID( &ppd->DeviceId , &GUID_NULL) ) {
        /* default device of type specified by ppd->DataFlow */
        if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE) {
            TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
            ppd->DeviceId = DSDEVID_DefaultCapture;
        } else if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER) {
            TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
            ppd->DeviceId = DSDEVID_DefaultPlayback;
        } else {
            WARN("DataFlow=Unknown(%d)\n", ppd->DataFlow);
            return E_PROP_ID_UNSUPPORTED;
        }
    }

    setup_dsound_options();

    GetDeviceID(&ppd->DeviceId, &dev_guid);

    hr = get_mmdevice(eRender, &dev_guid, &mmdevice);
    if(FAILED(hr)){
        hr = get_mmdevice(eCapture, &dev_guid, &mmdevice);
        if(FAILED(hr))
            return hr;
    }

    hr = IMMDevice_OpenPropertyStore(mmdevice, STGM_READ, &ps);
    if(FAILED(hr)){
        IMMDevice_Release(mmdevice);
        WARN("OpenPropertyStore failed: %08x\n", hr);
        return hr;
    }

    hr = IPropertyStore_GetValue(ps,
            (const PROPERTYKEY *)&DEVPKEY_Device_FriendlyName, &pv);
    if(FAILED(hr)){
        IPropertyStore_Release(ps);
        IMMDevice_Release(mmdevice);
        WARN("GetValue(FriendlyName) failed: %08x\n", hr);
        return hr;
    }

    ppd->Description = strdupW(pv.u.pwszVal);
    ppd->Module = strdupW(wine_vxd_drv);
    ppd->Interface = strdupW(wInterface);
    ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;

    PropVariantClear(&pv);
    IPropertyStore_Release(ps);
    IMMDevice_Release(mmdevice);

    if (pcbReturned) {
        *pcbReturned = sizeof(*ppd);
        TRACE("*pcbReturned=%d\n", *pcbReturned);
    }

    return S_OK;
}

static
BOOL CALLBACK enum_callback(GUID *guid, const WCHAR *desc, const WCHAR *module,
        void *user)
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA ppd = user;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA data;
    DWORD len;
    BOOL ret;

    TRACE("%s %s %s %p\n", wine_dbgstr_guid(guid), wine_dbgstr_w(desc),
            wine_dbgstr_w(module), user);

    if(!guid)
        return TRUE;

    data.DeviceId = *guid;

    len = lstrlenW(module) + 1;
    data.Module = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    memcpy(data.Module, module, len * sizeof(WCHAR));

    len = lstrlenW(desc) + 1;
    data.Description = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    memcpy(data.Description, desc, len * sizeof(WCHAR));

    data.Interface = wInterface;

    ret = ppd->Callback(&data, ppd->Context);

    HeapFree(GetProcessHeap(), 0, data.Module);
    HeapFree(GetProcessHeap(), 0, data.Description);

    return ret;
}

static HRESULT DSPROPERTY_EnumerateW(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA ppd = pPropData;
    HRESULT hr;

    TRACE("(pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
          pPropData,cbPropData,pcbReturned);

    if (pcbReturned)
        *pcbReturned = 0;

    if (!ppd || !ppd->Callback)
    {
        WARN("Invalid ppd %p\n", ppd);
        return E_PROP_ID_UNSUPPORTED;
    }

    hr = enumerate_mmdevices(eRender, DSOUND_renderer_guids,
            enum_callback, ppd);

    if(hr == S_OK)
        hr = enumerate_mmdevices(eCapture, DSOUND_capture_guids,
                enum_callback, ppd);

    return SUCCEEDED(hr) ? DS_OK : hr;
}

static BOOL DSPROPERTY_descWtoA(const DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA *dataW,
                                DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA *dataA)
{
    DWORD modlen, desclen;
    static char Interface[] = "Interface";

    modlen = WideCharToMultiByte(CP_ACP, 0, dataW->Module, -1, NULL, 0, NULL, NULL);
    desclen = WideCharToMultiByte(CP_ACP, 0, dataW->Description, -1, NULL, 0, NULL, NULL);
    dataA->Type = dataW->Type;
    dataA->DataFlow = dataW->DataFlow;
    dataA->DeviceId = dataW->DeviceId;
    dataA->WaveDeviceId = dataW->WaveDeviceId;
    dataA->Interface = Interface;
    dataA->Module = HeapAlloc(GetProcessHeap(), 0, modlen);
    dataA->Description = HeapAlloc(GetProcessHeap(), 0, desclen);
    if (!dataA->Module || !dataA->Description)
    {
        HeapFree(GetProcessHeap(), 0, dataA->Module);
        HeapFree(GetProcessHeap(), 0, dataA->Description);
        dataA->Module = dataA->Description = NULL;
        return FALSE;
    }

    WideCharToMultiByte(CP_ACP, 0, dataW->Module, -1, dataA->Module, modlen, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, dataW->Description, -1, dataA->Description, desclen, NULL, NULL);
    return TRUE;
}

static void DSPROPERTY_descWto1(const DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA *dataW,
                                DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA *data1)
{
    data1->DeviceId = dataW->DeviceId;
    lstrcpynW(data1->ModuleW, dataW->Module, sizeof(data1->ModuleW)/sizeof(*data1->ModuleW));
    lstrcpynW(data1->DescriptionW, dataW->Description, sizeof(data1->DescriptionW)/sizeof(*data1->DescriptionW));
    WideCharToMultiByte(CP_ACP, 0, data1->DescriptionW, -1, data1->DescriptionA, sizeof(data1->DescriptionA)-1, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, data1->ModuleW, -1, data1->ModuleA, sizeof(data1->ModuleA)-1, NULL, NULL);
    data1->DescriptionA[sizeof(data1->DescriptionA)-1] = 0;
    data1->ModuleA[sizeof(data1->ModuleA)-1] = 0;
    data1->Type = dataW->Type;
    data1->DataFlow = dataW->DataFlow;
    data1->WaveDeviceId = data1->Devnode = dataW->WaveDeviceId;
}

static BOOL CALLBACK DSPROPERTY_enumWtoA(DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA *descW, void *data)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA descA;
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA *ppd = data;
    BOOL ret;

    ret = DSPROPERTY_descWtoA(descW, &descA);
    if (!ret)
        return FALSE;
    ret = ppd->Callback(&descA, ppd->Context);
    HeapFree(GetProcessHeap(), 0, descA.Module);
    HeapFree(GetProcessHeap(), 0, descA.Description);
    return ret;
}

static HRESULT DSPROPERTY_EnumerateA(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA *ppd = pPropData;
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA data;

    if (!ppd || !ppd->Callback)
    {
        WARN("Invalid ppd %p\n", ppd);
        return E_PROP_ID_UNSUPPORTED;
    }

    data.Callback = DSPROPERTY_enumWtoA;
    data.Context = ppd;

    return DSPROPERTY_EnumerateW(&data, cbPropData, pcbReturned);
}

static BOOL CALLBACK DSPROPERTY_enumWto1(DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA *descW, void *data)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA desc1;
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1_DATA *ppd = data;
    BOOL ret;

    DSPROPERTY_descWto1(descW, &desc1);
    ret = ppd->Callback(&desc1, ppd->Context);
    return ret;
}

static HRESULT DSPROPERTY_Enumerate1(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1_DATA *ppd = pPropData;
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA data;

    if (!ppd || !ppd->Callback)
    {
        WARN("Invalid ppd %p\n", ppd);
        return E_PROP_ID_UNSUPPORTED;
    }

    data.Callback = DSPROPERTY_enumWto1;
    data.Context = ppd;

    return DSPROPERTY_EnumerateW(&data, cbPropData, pcbReturned);
}

static HRESULT DSPROPERTY_DescriptionA(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA data;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA *ppd = pPropData;
    HRESULT hr;

    if (pcbReturned)
        *pcbReturned = sizeof(*ppd);
    if (!pPropData)
        return S_OK;

    data.DeviceId = ppd->DeviceId;
    data.DataFlow = ppd->DataFlow;
    hr = DSPROPERTY_DescriptionW(&data, sizeof(data), NULL);
    if (FAILED(hr))
        return hr;
    if (!DSPROPERTY_descWtoA(&data, ppd))
        hr = E_OUTOFMEMORY;
    HeapFree(GetProcessHeap(), 0, data.Description);
    HeapFree(GetProcessHeap(), 0, data.Module);
    HeapFree(GetProcessHeap(), 0, data.Interface);
    return hr;
}

static HRESULT DSPROPERTY_Description1(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA data;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA *ppd = pPropData;
    HRESULT hr;

    if (pcbReturned)
        *pcbReturned = sizeof(*ppd);
    if (!pPropData)
        return S_OK;

    data.DeviceId = ppd->DeviceId;
    data.DataFlow = ppd->DataFlow;
    hr = DSPROPERTY_DescriptionW(&data, sizeof(data), NULL);
    if (FAILED(hr))
        return hr;
    DSPROPERTY_descWto1(&data, ppd);
    HeapFree(GetProcessHeap(), 0, data.Description);
    HeapFree(GetProcessHeap(), 0, data.Module);
    HeapFree(GetProcessHeap(), 0, data.Interface);
    return hr;
}

static HRESULT WINAPI IKsPrivatePropertySetImpl_Get(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    IKsPrivatePropertySetImpl *This = impl_from_IKsPropertySet(iface);
    TRACE("(iface=%p,guidPropSet=%s,dwPropID=%d,pInstanceData=%p,cbInstanceData=%d,pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
          This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData,pcbReturned);

    if ( IsEqualGUID( &DSPROPSETID_DirectSoundDevice, guidPropSet) ) {
        switch (dwPropID) {
        case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A:
            return DSPROPERTY_WaveDeviceMappingA(pPropData,cbPropData,pcbReturned);
        case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1:
            return DSPROPERTY_Description1(pPropData,cbPropData,pcbReturned);
        case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1:
            return DSPROPERTY_Enumerate1(pPropData,cbPropData,pcbReturned);
        case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W:
            return DSPROPERTY_WaveDeviceMappingW(pPropData,cbPropData,pcbReturned);
        case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A:
            return DSPROPERTY_DescriptionA(pPropData,cbPropData,pcbReturned);
        case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W:
            return DSPROPERTY_DescriptionW(pPropData,cbPropData,pcbReturned);
        case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A:
            return DSPROPERTY_EnumerateA(pPropData,cbPropData,pcbReturned);
        case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W:
            return DSPROPERTY_EnumerateW(pPropData,cbPropData,pcbReturned);
        default:
            FIXME("unsupported ID: %d\n",dwPropID);
            break;
        }
    } else {
        FIXME("unsupported property: %s\n",debugstr_guid(guidPropSet));
    }

    if (pcbReturned) {
        *pcbReturned = 0;
        FIXME("*pcbReturned=%d\n", *pcbReturned);
    }

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPrivatePropertySetImpl_Set(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData )
{
    IKsPrivatePropertySetImpl *This = impl_from_IKsPropertySet(iface);

    FIXME("(%p,%s,%d,%p,%d,%p,%d), stub!\n",This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData);
    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPrivatePropertySetImpl_QuerySupport(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    PULONG pTypeSupport )
{
    IKsPrivatePropertySetImpl *This = impl_from_IKsPropertySet(iface);
    TRACE("(%p,%s,%d,%p)\n",This,debugstr_guid(guidPropSet),dwPropID,pTypeSupport);

    if ( IsEqualGUID( &DSPROPSETID_DirectSoundDevice, guidPropSet) ) {
	switch (dwPropID) {
	case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	default:
            FIXME("unsupported ID: %d\n",dwPropID);
	    break;
	}
    } else {
	FIXME("unsupported property: %s\n",debugstr_guid(guidPropSet));
    }

    return E_PROP_ID_UNSUPPORTED;
}

static const IKsPropertySetVtbl ikspvt = {
    IKsPrivatePropertySetImpl_QueryInterface,
    IKsPrivatePropertySetImpl_AddRef,
    IKsPrivatePropertySetImpl_Release,
    IKsPrivatePropertySetImpl_Get,
    IKsPrivatePropertySetImpl_Set,
    IKsPrivatePropertySetImpl_QuerySupport
};

HRESULT IKsPrivatePropertySetImpl_Create(REFIID riid, void **ppv)
{
    IKsPrivatePropertySetImpl *iks;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    iks = HeapAlloc(GetProcessHeap(), 0, sizeof(*iks));
    if (!iks) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    iks->ref = 1;
    iks->IKsPropertySet_iface.lpVtbl = &ikspvt;

    hr = IKsPropertySet_QueryInterface(&iks->IKsPropertySet_iface, riid, ppv);
    IKsPropertySet_Release(&iks->IKsPropertySet_iface);

    return hr;
}
