/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/property.c
 * PURPOSE:         Secondary IDirectSoundBuffer8 implementation
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */


#include "precomp.h"

typedef struct
{
    IKsPropertySetVtbl *lpVtbl;
    LONG ref;

}CKsPropertySetImpl, *LPCKsPropertySetImpl;

HRESULT
WINAPI
KSPropertySetImpl_fnQueryInterface(
    LPKSPROPERTYSET iface,
    REFIID riid,
    LPVOID * ppobj)
{
    LPOLESTR pStr;
    LPCKsPropertySetImpl This = (LPCKsPropertySetImpl)CONTAINING_RECORD(iface, CKsPropertySetImpl, lpVtbl);


    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IKsPropertySet))
    {
        *ppobj = (LPVOID)&This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }

    if (SUCCEEDED(StringFromIID(riid, &pStr)))
    {
        DPRINT("No Interface for riid %s\n", pStr);
        CoTaskMemFree(pStr);
    }
    return E_NOINTERFACE;
}

ULONG
WINAPI
KSPropertySetImpl_fnAddRef(
    LPKSPROPERTYSET iface)
{
    ULONG ref;
    LPCKsPropertySetImpl This = (LPCKsPropertySetImpl)CONTAINING_RECORD(iface, CKsPropertySetImpl, lpVtbl);

    ref = InterlockedIncrement(&This->ref);

    return ref;
}

ULONG
WINAPI
KSPropertySetImpl_fnRelease(
    LPKSPROPERTYSET iface)
{
    ULONG ref;
    LPCKsPropertySetImpl This = (LPCKsPropertySetImpl)CONTAINING_RECORD(iface, CKsPropertySetImpl, lpVtbl);

    ref = InterlockedDecrement(&(This->ref));

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}


HRESULT
WINAPI
KSPropertySetImpl_Get(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    LPOLESTR pStr;
    //HRESULT hr;
    MMRESULT Result;
    WAVEINCAPSW CapsIn;
    WAVEOUTCAPSW CapsOut;

    GUID DeviceGuid;
    LPFILTERINFO Filter = NULL;
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA Desc;

    StringFromIID(guidPropSet, &pStr);

    //DPRINT("Requested Property %ws dwPropID %u pInstanceData %p cbInstanceData %u pPropData %p cbPropData %u pcbReturned %p\n",
    //       pStr, dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned);
    CoTaskMemFree(pStr);

    if (!IsEqualGUID(guidPropSet, &DSPROPSETID_DirectSoundDevice))
    {
        /* unsupported property set */
        return E_PROP_ID_UNSUPPORTED;
    }

    if (dwPropID == DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1)
    {
        if (cbPropData < sizeof(DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA))
        {
            /* invalid parameter */
            return DSERR_INVALIDPARAM;
        }

        Desc = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA)pPropData;

        if (IsEqualGUID(&Desc->DeviceId, &GUID_NULL))
        {
            if (Desc->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE)
            {
                DPRINT("Using default capture guid\n");
                CopyMemory(&DeviceGuid, &DSDEVID_DefaultCapture, sizeof(GUID));
            }
            else
            {
                DPRINT("Using default playback guid\n");
                CopyMemory(&DeviceGuid, &DSDEVID_DefaultPlayback, sizeof(GUID));
            }
        }
        else
        {
            /* use provided guid */
            CopyMemory(&DeviceGuid, &Desc->DeviceId, sizeof(GUID));
        }

        if (GetDeviceID(&DeviceGuid, &Desc->DeviceId) != DS_OK)
        {
            DPRINT("Unknown device guid\n");
            return DSERR_INVALIDPARAM;
        }

         /* sanity check */
         ASSERT(FindDeviceByGuid(&Desc->DeviceId, &Filter));
         ASSERT(Filter != NULL);

         /* fill in description */
         if (IsEqualGUID(&Desc->DeviceId, &Filter->DeviceGuid[0]))
         {
             /* capture device */
             Desc->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
             Desc->WaveDeviceId = Filter->MappedId[0];

             Result = waveInGetDevCapsW(Filter->MappedId[0], &CapsIn, sizeof(WAVEINCAPSW));
             if (Result != MMSYSERR_NOERROR)
             {
                 CapsIn.szPname[0] = 0;
                 DPRINT("waveInGetDevCaps Device %u failed with %x\n", Filter->MappedId[0], Result);
             }

             CapsIn.szPname[MAXPNAMELEN-1] = 0;
             wcscpy(Desc->DescriptionW, CapsIn.szPname);
             WideCharToMultiByte(CP_ACP, 0, CapsIn.szPname, -1, Desc->DescriptionA, sizeof(Desc->DescriptionA), NULL, NULL);

         }
         else
         {
             /* render device */
             Desc->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
             Desc->WaveDeviceId = Filter->MappedId[1];

             Result = waveOutGetDevCapsW(Filter->MappedId[1], &CapsOut, sizeof(WAVEOUTCAPSW));
             if (Result != MMSYSERR_NOERROR)
             {
                 CapsOut.szPname[0] = 0;
                 DPRINT("waveOutGetDevCaps Device %u failed with %x\n", Filter->MappedId[1], Result);
             }

             CapsOut.szPname[MAXPNAMELEN-1] = 0;
             wcscpy(Desc->DescriptionW, CapsOut.szPname);
             WideCharToMultiByte(CP_ACP, 0, CapsOut.szPname, -1, Desc->DescriptionA, sizeof(Desc->DescriptionA), NULL, NULL);
         }

          /* ReactOS doesnt support vxd or emulated */
          Desc->Type = DIRECTSOUNDDEVICE_TYPE_WDM;
          Desc->ModuleA[0] = 0;
          Desc->ModuleW[0] = 0;

          *pcbReturned = sizeof(DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA);
          return S_OK;
    }


    UNIMPLEMENTED
    return E_PROP_ID_UNSUPPORTED;
}

HRESULT
WINAPI
KSPropertySetImpl_Set(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData )
{
    UNIMPLEMENTED
    return E_PROP_ID_UNSUPPORTED;
}

HRESULT
WINAPI
KSPropertySetImpl_QuerySupport(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    PULONG pTypeSupport )
{
    UNIMPLEMENTED
    return E_PROP_ID_UNSUPPORTED;
}

static IKsPropertySetVtbl vt_KsPropertySet =
{
    /* IUnknown methods */
    KSPropertySetImpl_fnQueryInterface,
    KSPropertySetImpl_fnAddRef,
    KSPropertySetImpl_fnRelease,
    /* IKsPropertySet methods */
    KSPropertySetImpl_Get,
    KSPropertySetImpl_Set,
    KSPropertySetImpl_QuerySupport
};

HRESULT
CALLBACK
NewKsPropertySet(
    IUnknown* pUnkOuter,
    REFIID riid,
    LPVOID* ppvObject)
{
    LPOLESTR pStr;
    LPCKsPropertySetImpl This;

    /* check requested interface */
    if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IKsPropertySet))
    {
        *ppvObject = 0;
        StringFromIID(riid, &pStr);
        DPRINT("KsPropertySet does not support Interface %ws\n", pStr);
        CoTaskMemFree(pStr);
        return E_NOINTERFACE;
    }

    /* allocate object */
    This = HeapAlloc(GetProcessHeap(), 0, sizeof(CKsPropertySetImpl));
    if (!This)
        return E_OUTOFMEMORY;

    /* initialize object */
    This->ref = 1;
    This->lpVtbl = &vt_KsPropertySet;
    *ppvObject = (LPVOID)&This->lpVtbl;

    return S_OK;
}
