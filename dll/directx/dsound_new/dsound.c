/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/dsound.c
 * PURPOSE:         Handles DSound initialization
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

HINSTANCE dsound_hInstance;
LPFILTERINFO RootInfo = NULL;

static INTERFACE_TABLE InterfaceTable[] =
{
    {
        &CLSID_DirectSoundPrivate,
        NewKsPropertySet
    },
    {
        &CLSID_DirectSoundCapture,
        NewDirectSoundCapture
    },
    {
        &CLSID_DirectSoundCapture8,
        NewDirectSoundCapture
    },
    {
        &CLSID_DirectSound,
        NewDirectSound
    },
    {
        &CLSID_DirectSound8,
        NewDirectSound
    },
    {
        NULL,
        NULL
    }
};


HRESULT
WINAPI
DllCanUnloadNow()
{
    return S_FALSE;
}

HRESULT
WINAPI
GetDeviceID(LPCGUID pGuidSrc, LPGUID pGuidDest)
{
    ULONG DeviceID = ULONG_MAX, Flags;
    MMRESULT Result;
    LPFILTERINFO Filter;

    if (!pGuidSrc || !pGuidDest)
    {
        /* invalid param */
        return DSERR_INVALIDPARAM;
    }

    /* sanity check */
    ASSERT(!IsEqualGUID(pGuidSrc, &GUID_NULL));

    if (IsEqualGUID(&DSDEVID_DefaultPlayback, pGuidSrc) ||
        IsEqualGUID(&DSDEVID_DefaultVoicePlayback, pGuidSrc))
    {
        Result = waveOutMessage(UlongToHandle(WAVE_MAPPER), DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&DeviceID, (DWORD_PTR)&Flags);
        if (Result != MMSYSERR_NOERROR || DeviceID == ULONG_MAX)
        {
            /* hack */
            DPRINT1("Failed to get DRVM_MAPPER_PREFERRED_GET, using device 0\n");
            DeviceID = 0;
        }

        if (!FindDeviceByMappedId(DeviceID, &Filter, TRUE))
        {
            /* device not found */
            return DSERR_INVALIDPARAM;
        }

        /* copy device guid */
        RtlMoveMemory(pGuidDest, &Filter->DeviceGuid[1], sizeof(GUID));
        return DS_OK;
    }
    else if (IsEqualGUID(&DSDEVID_DefaultCapture, pGuidSrc) ||
             IsEqualGUID(&DSDEVID_DefaultVoiceCapture, pGuidSrc))
    {
        Result = waveInMessage(UlongToHandle(WAVE_MAPPER), DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&DeviceID, (DWORD_PTR)&Flags);
        if (Result != MMSYSERR_NOERROR || DeviceID == ULONG_MAX)
        {
            /* hack */
            DPRINT1("Failed to get DRVM_MAPPER_PREFERRED_GET, for record using device 0\n");
            DeviceID = 0;
        }

        if (!FindDeviceByMappedId(DeviceID, &Filter, FALSE))
        {
            /* device not found */
            return DSERR_INVALIDPARAM;
        }

        /* copy device guid */
        RtlMoveMemory(pGuidDest, &Filter->DeviceGuid[0], sizeof(GUID));
        return DS_OK;
    }

    if (!FindDeviceByGuid(pGuidSrc, &Filter))
    {
        /* unknown guid */
        return DSERR_INVALIDPARAM;
    }

    /* done */
    return DS_OK;
}


HRESULT
WINAPI
DllGetClassObject(
  REFCLSID rclsid,
  REFIID riid,
  LPVOID* ppv)
{
    LPOLESTR pStr, pStr2;
    UINT i;
    HRESULT	hres = E_OUTOFMEMORY;
    IClassFactory * pcf = NULL;

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    for (i = 0; InterfaceTable[i].riid; i++)
    {
        if (IsEqualIID(InterfaceTable[i].riid, rclsid))
        {
            pcf = IClassFactory_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
            break;
        }
    }

    if (!pcf)
    {
        StringFromIID(rclsid, &pStr);
        StringFromIID(riid, &pStr2);
        DPRINT("No Class Available for %ws IID %ws\n", pStr, pStr2);
        CoTaskMemFree(pStr);
        CoTaskMemFree(pStr2);
        //ASSERT(0);
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    hres = IClassFactory_QueryInterface(pcf, riid, ppv);
    IClassFactory_Release(pcf);

    return hres;
}



BOOL
WINAPI
DllMain(
    HINSTANCE hInstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            dsound_hInstance = hInstDLL;
#if 1
            DPRINT("NumDevs %u\n", waveOutGetNumDevs());
            if (EnumAudioDeviceInterfaces(&RootInfo) != S_OK)
            {
                DPRINT("EnumAudioDeviceInterfaces failed\n");
                RootInfo = NULL;
            }
DPRINT1("EnumAudioDeviceInterfaces %p %u\n", RootInfo, waveOutGetNumDevs());
#endif
            DisableThreadLibraryCalls(dsound_hInstance);
            break;
    default:
        break;
    }

    return TRUE;
}

