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

#include "dsound_private.h"

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
    LPKSPROPERTYSET iface,
    REFIID riid,
    LPVOID *ppobj )
{
    IKsPrivatePropertySetImpl *This = impl_from_IKsPropertySet(iface);
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IKsPropertySet)) {
        *ppobj = iface;
        IUnknown_AddRef(iface);
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

static HRESULT DSPROPERTY_WaveDeviceMappingW(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    HRESULT hr = DSERR_INVALIDPARAM;
    PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA ppd;
    TRACE("(pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
	  pPropData,cbPropData,pcbReturned);

    ppd = pPropData;

    if (!ppd) {
	WARN("invalid parameter: pPropData\n");
	return DSERR_INVALIDPARAM;
    }

    if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER) {
        ULONG wod;
        unsigned int wodn;
        TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
        wodn = waveOutGetNumDevs();
        for (wod = 0; wod < wodn; wod++) {
            WAVEOUTCAPSW capsW;
            MMRESULT res;
            res = waveOutGetDevCapsW(wod, &capsW, sizeof(capsW));
            if (res == MMSYSERR_NOERROR) {
                if (lstrcmpW(capsW.szPname, ppd->DeviceName) == 0) {
                    ppd->DeviceId = DSOUND_renderer_guids[wod];
                    hr = DS_OK;
                    TRACE("found %s for %s\n", debugstr_guid(&ppd->DeviceId),
                          debugstr_w(ppd->DeviceName));
                    break;
                }
            }
        }
    } else if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE) {
        ULONG wid;
        unsigned int widn;
        TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
        widn = waveInGetNumDevs();
        for (wid = 0; wid < widn; wid++) {
            WAVEINCAPSW capsW;
            MMRESULT res;
            res = waveInGetDevCapsW(wid, &capsW, sizeof(capsW));
            if (res == MMSYSERR_NOERROR) {
                if (lstrcmpW(capsW.szPname, ppd->DeviceName) == 0) {
                    ppd->DeviceId = DSOUND_capture_guids[wid];
                    hr = DS_OK;
                    TRACE("found %s for %s\n", debugstr_guid(&ppd->DeviceId),
                          debugstr_w(ppd->DeviceName));
                    break;
                }
            }
        }
    }

    if (pcbReturned)
        *pcbReturned = cbPropData;

    return hr;
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
    HRESULT err;
    GUID dev_guid;
    ULONG wod, wid, wodn, widn;
    DSDRIVERDESC desc;

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

    wodn = waveOutGetNumDevs();
    widn = waveInGetNumDevs();
    wid = wod = dev_guid.Data4[7];
    if (!memcmp(&dev_guid, &DSOUND_renderer_guids[0], sizeof(GUID)-1)
        && wod < wodn)
    {
        ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
        ppd->WaveDeviceId = wod;
    }
    else if (!memcmp(&dev_guid, &DSOUND_capture_guids[0], sizeof(GUID)-1)
             && wid < widn)
    {
        ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
        ppd->WaveDeviceId = wid;
    }
    else
    {
        WARN("Device not found\n");
        return E_PROP_ID_UNSUPPORTED;
    }

    if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER)
        err = waveOutMessage(UlongToHandle(wod),DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,ds_hw_accel);
    else
        err = waveInMessage(UlongToHandle(wod),DRV_QUERYDSOUNDDESC,(DWORD_PTR)&desc,ds_hw_accel);

    if (err != MMSYSERR_NOERROR)
    {
        WARN("waveMessage(DRV_QUERYDSOUNDDESC) failed!\n");
        return E_PROP_ID_UNSUPPORTED;
    }
    else
    {
        /* FIXME: Still a memory leak.. */
        int desclen, modlen;
        static WCHAR wInterface[] = { 'I','n','t','e','r','f','a','c','e',0 };

        modlen = MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, NULL, 0 );
        desclen = MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, NULL, 0 );
        ppd->Module = HeapAlloc(GetProcessHeap(),0,modlen*sizeof(WCHAR));
        ppd->Description = HeapAlloc(GetProcessHeap(),0,desclen*sizeof(WCHAR));
        ppd->Interface = wInterface;
        if (!ppd->Description || !ppd->Module)
        {
            WARN("Out of memory\n");
            HeapFree(GetProcessHeap(), 0, ppd->Description);
            HeapFree(GetProcessHeap(), 0, ppd->Module);
            ppd->Description = ppd->Module = NULL;
            return E_OUTOFMEMORY;
        }

        MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, ppd->Module, modlen );
        MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, ppd->Description, desclen );
    }

    ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;

    if (pcbReturned) {
        *pcbReturned = sizeof(*ppd);
        TRACE("*pcbReturned=%d\n", *pcbReturned);
    }

    return S_OK;
}

static HRESULT DSPROPERTY_EnumerateW(
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA ppd = pPropData;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA data;
    BOOL ret;
    int widn, wodn, i;
    TRACE("(pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
          pPropData,cbPropData,pcbReturned);

    if (pcbReturned)
        *pcbReturned = 0;

    if (!ppd || !ppd->Callback)
    {
        WARN("Invalid ppd %p\n", ppd);
        return E_PROP_ID_UNSUPPORTED;
    }

    wodn = waveOutGetNumDevs();
    widn = waveInGetNumDevs();

    data.DeviceId = DSOUND_renderer_guids[0];
    for (i = 0; i < wodn; ++i)
    {
        HRESULT hr;
        data.DeviceId.Data4[7] = i;
        hr = DSPROPERTY_DescriptionW(&data, sizeof(data), NULL);
        if (FAILED(hr))
        {
            ERR("DescriptionW failed!\n");
            return S_OK;
        }
        ret = ppd->Callback(&data, ppd->Context);
        HeapFree(GetProcessHeap(), 0, data.Module);
        HeapFree(GetProcessHeap(), 0, data.Description);
        if (!ret)
            return S_OK;
    }

    data.DeviceId = DSOUND_capture_guids[0];
    for (i = 0; i < widn; ++i)
    {
        HRESULT hr;
        data.DeviceId.Data4[7] = i;
        hr = DSPROPERTY_DescriptionW(&data, sizeof(data), NULL);
        if (FAILED(hr))
        {
            ERR("DescriptionW failed!\n");
            return S_OK;
        }
        ret = ppd->Callback(&data, ppd->Context);
        HeapFree(GetProcessHeap(), 0, data.Module);
        HeapFree(GetProcessHeap(), 0, data.Description);
        if (!ret)
            return S_OK;
    }
    return S_OK;
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

HRESULT IKsPrivatePropertySetImpl_Create(
    REFIID riid,
    IKsPropertySet **piks)
{
    IKsPrivatePropertySetImpl *iks;
    TRACE("(%s, %p)\n", debugstr_guid(riid), piks);

    if (!IsEqualIID(riid, &IID_IUnknown) &&
        !IsEqualIID(riid, &IID_IKsPropertySet)) {
        *piks = 0;
        return E_NOINTERFACE;
    }

    iks = HeapAlloc(GetProcessHeap(),0,sizeof(*iks));
    iks->ref = 1;
    iks->IKsPropertySet_iface.lpVtbl = &ikspvt;

    *piks = &iks->IKsPropertySet_iface;
    return S_OK;
}
