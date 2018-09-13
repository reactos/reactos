//--------------------------------------------------------------------------;
//
//  File: dslevel.cpp
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved 
//
//--------------------------------------------------------------------------;


#include "mmcpl.h"

#include <windowsx.h>
#ifdef DEBUG
#undef DEBUG
#include <mmsystem.h>
#define DEBUG
#else
#include <mmsystem.h>
#endif
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>

#include "utils.h"
#include "medhelp.h"
#include "dslevel.h"
#include "perfpage.h"
#include "speakers.h"

#include <initguid.h>
#include <dsound.h>
#include <dsprv.h>

#define REG_KEY_SPEAKERTYPE TEXT("Speaker Type")

typedef HRESULT (STDAPICALLTYPE *LPFNDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID *);
typedef HRESULT (STDAPICALLTYPE *LPFNDIRECTSOUNDCREATE)(LPGUID, LPDIRECTSOUND*, IUnknown FAR *);
typedef HRESULT (STDAPICALLTYPE *LPFNDIRECTSOUNDCAPTURECREATE)(LPGUID, LPDIRECTSOUNDCAPTURE*, IUnknown FAR *);


HRESULT 
DirectSoundPrivateCreate
(
    OUT LPKSPROPERTYSET *   ppKsPropertySet
)
{
    HINSTANCE               hLibDsound              = NULL;
    LPFNDLLGETCLASSOBJECT   pfnDllGetClassObject    = NULL;
    LPCLASSFACTORY          pClassFactory           = NULL;
    LPKSPROPERTYSET         pKsPropertySet          = NULL;
    HRESULT                 hr                      = DS_OK;
    
    // Load dsound.dll
    hLibDsound = 
        GetModuleHandle
        (
            TEXT("dsound.dll")
        );

    if(!hLibDsound)
    {
        hr = DSERR_GENERIC;
    }

    // Find DllGetClassObject
    if(SUCCEEDED(hr))
    {
        pfnDllGetClassObject = 
            (LPFNDLLGETCLASSOBJECT)GetProcAddress
            (
                hLibDsound, 
                "DllGetClassObject"
            );

        if(!pfnDllGetClassObject)
        {
            hr = DSERR_GENERIC;
        }
    }

    // Create a class factory object    
    if(SUCCEEDED(hr))
    {
        hr = 
            pfnDllGetClassObject
            (
                CLSID_DirectSoundPrivate, 
                IID_IClassFactory, 
                (LPVOID *)&pClassFactory
            );
    }

    // Create the DirectSoundPrivate object and query for an IKsPropertySet
    // interface
    if(SUCCEEDED(hr))
    {
        hr = 
            pClassFactory->CreateInstance
            (
                NULL, 
                IID_IKsPropertySet, 
                (LPVOID *)&pKsPropertySet
            );
    }

    // Release the class factory
    if(pClassFactory)
    {
        pClassFactory->Release();
    }

    // Handle final success or failure
    if(SUCCEEDED(hr))
    {
        *ppKsPropertySet = pKsPropertySet;
    }
    else if(pKsPropertySet)
    {
        pKsPropertySet->Release();
    }

    return hr;
}


HRESULT 
DSGetGuidFromName
(
    IN  LPTSTR              szName, 
    IN  BOOL                fRecord, 
    OUT LPGUID              pGuid
)
{
    LPKSPROPERTYSET         pKsPropertySet  = NULL;
    HRESULT                 hr;

    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA 
        WaveDeviceMap;

    // Create the DirectSoundPrivate object
    hr = 
        DirectSoundPrivateCreate
        (
            &pKsPropertySet
        );

    // Attempt to map the waveIn/waveOut device string to a DirectSound device
    // GUID.
    if(SUCCEEDED(hr))
    {
        WaveDeviceMap.DeviceName = szName;
        WaveDeviceMap.DataFlow = fRecord ? DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE : DIRECTSOUNDDEVICE_DATAFLOW_RENDER;

        hr = 
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundDevice, 
                DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING, 
                NULL, 
                0, 
                &WaveDeviceMap, 
                sizeof(WaveDeviceMap), 
                NULL
            );
    }

    // Clean up
    if(pKsPropertySet)
    {
        pKsPropertySet->Release();
    }

    if(SUCCEEDED(hr))
    {
        *pGuid = WaveDeviceMap.DeviceId;
    }

    return hr;
}

HRESULT
DSSetupFunctions
(
    LPFNDIRECTSOUNDCREATE* pfnDSCreate,
    LPFNDIRECTSOUNDCAPTURECREATE* pfnDSCaptureCreate
)
{
    HINSTANCE               hLibDsound              = NULL;
    HRESULT                 hr                      = DS_OK;
    
    // Load dsound.dll
    hLibDsound = 
        GetModuleHandle
        (
            TEXT("dsound.dll")
        );

    if(!hLibDsound)
    {
        hr = DSERR_GENERIC;
    }

    // Find DirectSoundCreate
    if(SUCCEEDED(hr))
    {
        *pfnDSCreate = 
            (LPFNDIRECTSOUNDCREATE)GetProcAddress
            (
                hLibDsound, 
                "DirectSoundCreate"
            );

        if(!(*pfnDSCreate))
        {
            hr = DSERR_GENERIC;
        }
    } //end DirectSoundCreate

    // Find DirectSoundCaptureCreate
    if(SUCCEEDED(hr))
    {
        *pfnDSCaptureCreate = 
            (LPFNDIRECTSOUNDCAPTURECREATE)GetProcAddress
            (
                hLibDsound, 
                "DirectSoundCaptureCreate"
            );

        if(!(*pfnDSCaptureCreate))
        {
            hr = DSERR_GENERIC;
        }
    } //end DirectSoundCaptureCreate

    return (hr);
}

HRESULT 
DSGetCplValues
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    OUT LPCPLDATA           pData
)
{
    LPKSPROPERTYSET         pKsPropertySet      = NULL;
    LPDIRECTSOUND           pDirectSound        = NULL;
    LPDIRECTSOUNDCAPTURE    pDirectSoundCapture = NULL;
    HRESULT                 hr;

    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA   
        BasicAcceleration;
    
    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA                 
        SrcQuality;

    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA
        SpeakerType;

    LPFNDIRECTSOUNDCREATE           pfnDirectSoundCreate = NULL;
    LPFNDIRECTSOUNDCAPTURECREATE    pfnDirectSoundCaptureCreate = NULL;

    // Find the necessary DirectSound functions
    hr = DSSetupFunctions(&pfnDirectSoundCreate, &pfnDirectSoundCaptureCreate);

    if (FAILED(hr))
    {
        return (hr);
    }

    // Create the DirectSound object
    if(fRecord)
    {
        hr = 
            pfnDirectSoundCaptureCreate
            (
                &guid, 
                &pDirectSoundCapture, 
                NULL
            );
    }
    else
    {
        hr = 
            pfnDirectSoundCreate
            (
                &guid, 
                &pDirectSound, 
                NULL
            );
    }
    
    // Create the DirectSoundPrivate object
    if(SUCCEEDED(hr))
    {
        hr = 
            DirectSoundPrivateCreate
            (
                &pKsPropertySet
            );
    }

    // Get properties for this device
    if(SUCCEEDED(hr))
    {
        // Get the basic HW acceleration level.  This property will return
        // S_FALSE if no error occurred, but the registry value did not exist.
        BasicAcceleration.DeviceId = guid;
        
        hr = 
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundBasicAcceleration, 
                DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION, 
                NULL, 
                0, 
                &BasicAcceleration, 
                sizeof(BasicAcceleration), 
                NULL
            );

        if(DS_OK == hr)
        {
            pData->dwHWLevel = BasicAcceleration.Level;
        }
        else
        {
            pData->dwHWLevel = DEFAULT_HW_LEVEL;
        }

        // Get the mixer SRC quality.  This property will return S_FALSE 
        // if no error occurred, but the registry value did not exist.
        SrcQuality.DeviceId = guid;
        
        hr = 
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundMixer, 
                DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY, 
                NULL, 
                0, 
                &SrcQuality, 
                sizeof(SrcQuality), 
                NULL
            );

        if(DS_OK == hr)
        {
            // The CPL only uses the 3 highest of 4 possible SRC values
            pData->dwSRCLevel = SrcQuality.Quality;

            if(pData->dwSRCLevel > 0)
            {
                pData->dwSRCLevel--;
            }
        }
        else
        {
            pData->dwSRCLevel = DEFAULT_SRC_LEVEL;
        }

        // Get playback-specific settings
        if(!fRecord)
        {
            // Get the speaker config
            hr = 
                pDirectSound->GetSpeakerConfig
                (
                    &pData->dwSpeakerConfig
                );

            if(DS_OK != hr)
            {
                pData->dwSpeakerConfig = DSSPEAKER_STEREO;
            }

            // Get the speaker type.  This property will return failure
            // if the registry value doesn't exist.
            SpeakerType.DeviceId = guid;
            SpeakerType.SubKeyName = REG_KEY_SPEAKERTYPE;
            SpeakerType.ValueName = REG_KEY_SPEAKERTYPE;
            SpeakerType.RegistryDataType = REG_DWORD;
            SpeakerType.Data = &pData->dwSpeakerType;
            SpeakerType.DataSize = sizeof(pData->dwSpeakerType);

            hr = 
                pKsPropertySet->Get
                (
                    DSPROPSETID_DirectSoundPersistentData, 
                    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA, 
                    NULL, 
                    0, 
                    &SpeakerType, 
                    sizeof(SpeakerType), 
                    NULL
                );

            if(DS_OK != hr)
            {
                pData->dwSpeakerType = SPEAKERS_DEFAULT_TYPE;
            }
        }
    }

    // Clean up
    if(pKsPropertySet)
    {
        pKsPropertySet->Release();
    }

    if(pDirectSound)
    {
        pDirectSound->Release();
    }

    if(pDirectSoundCapture)
    {
        pDirectSoundCapture->Release();
    }

    return DS_OK;
}


HRESULT 
DSSetCplValues
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    IN  const LPCPLDATA     pData
)
{
    LPKSPROPERTYSET         pKsPropertySet      = NULL;
    LPDIRECTSOUND           pDirectSound        = NULL;
    LPDIRECTSOUNDCAPTURE    pDirectSoundCapture = NULL;
    HRESULT                 hr;

    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA   
        BasicAcceleration;

    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA                 
        SrcQuality;

    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA
        SpeakerType;

    LPFNDIRECTSOUNDCREATE           pfnDirectSoundCreate = NULL;
    LPFNDIRECTSOUNDCAPTURECREATE    pfnDirectSoundCaptureCreate = NULL;

    // Find the necessary DirectSound functions
    hr = DSSetupFunctions(&pfnDirectSoundCreate, &pfnDirectSoundCaptureCreate);

    if (FAILED(hr))
    {
        return (hr);
    }

    // Create the DirectSound object
    if(fRecord)
    {
        hr = 
            pfnDirectSoundCaptureCreate
            (
                &guid, 
                &pDirectSoundCapture, 
                NULL
            );
    }
    else
    {
        hr = 
            pfnDirectSoundCreate
            (
                &guid, 
                &pDirectSound, 
                NULL
            );
    }
    
    // Create the DirectSoundPrivate object
    if(SUCCEEDED(hr))
    {
        hr = 
            DirectSoundPrivateCreate
            (
                &pKsPropertySet
            );
    }

    // Set the basic HW acceleration level
    if(SUCCEEDED(hr))
    {
        BasicAcceleration.DeviceId = guid;
        BasicAcceleration.Level = (DIRECTSOUNDBASICACCELERATION_LEVEL)pData->dwHWLevel;

        hr = 
            pKsPropertySet->Set
            (
                DSPROPSETID_DirectSoundBasicAcceleration, 
                DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION, 
                NULL, 
                0, 
                &BasicAcceleration, 
                sizeof(BasicAcceleration)
            );
    }

    // Set the mixer SRC quality
    if(SUCCEEDED(hr))
    {
        SrcQuality.DeviceId = guid;

        // The CPL only uses the 3 highest of 4 possible SRC values
        SrcQuality.Quality = (DIRECTSOUNDMIXER_SRCQUALITY)(pData->dwSRCLevel + 1);
        
        hr = 
            pKsPropertySet->Set
            (
                DSPROPSETID_DirectSoundMixer, 
                DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY, 
                NULL, 
                0, 
                &SrcQuality, 
                sizeof(SrcQuality)
            );
    }

    // Set the speaker config
    if(SUCCEEDED(hr) && !fRecord)
    {
        hr = 
            pDirectSound->SetSpeakerConfig
            (
                pData->dwSpeakerConfig
            );
    }

    // Set the speaker type
    if(SUCCEEDED(hr) && !fRecord)
    {
        SpeakerType.DeviceId = guid;
        SpeakerType.SubKeyName = REG_KEY_SPEAKERTYPE;
        SpeakerType.ValueName = REG_KEY_SPEAKERTYPE;
        SpeakerType.RegistryDataType = REG_DWORD;
        SpeakerType.Data = &pData->dwSpeakerType;
        SpeakerType.DataSize = sizeof(pData->dwSpeakerType);

        hr = 
            pKsPropertySet->Set
            (
                DSPROPSETID_DirectSoundPersistentData, 
                DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA, 
                NULL, 
                0, 
                &SpeakerType, 
                sizeof(SpeakerType)
            );
    }

    // Clean up
    if(pKsPropertySet)
    {
        pKsPropertySet->Release();
    }

    if(pDirectSound)
    {
        pDirectSound->Release();
    }

    if(pDirectSoundCapture)
    {
        pDirectSoundCapture->Release();
    }

    return hr;
}


