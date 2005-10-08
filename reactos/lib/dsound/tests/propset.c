/*
 * Unit tests for CLSID_DirectSoundPrivate property set functions
 * (used by dxdiag)
 *
 * Copyright (c) 2003 Robert Reif
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include <windows.h>

#include <math.h>
#include <stdlib.h>

#include "wine/test.h"
#include "windef.h"
#include "wingdi.h"
#include "dsound.h"
#include "initguid.h"
#include "dsconf.h"
#include "dxerr8.h"

#include "dsound_test.h"

#ifndef DSBCAPS_CTRLDEFAULT
#define DSBCAPS_CTRLDEFAULT \
        DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLPAN|DSBCAPS_CTRLVOLUME
#endif

DEFINE_GUID(DSPROPSETID_VoiceManager, \
            0x62A69BAE,0xDF9D,0x11D1,0x99,0xA6,0x00,0xC0,0x4F,0xC9,0x9D,0x46);
DEFINE_GUID(DSPROPSETID_EAX20_ListenerProperties, \
            0x306a6a8,0xb224,0x11d2,0x99,0xe5,0x0,0x0,0xe8,0xd8,0xc7,0x22);
DEFINE_GUID(DSPROPSETID_EAX20_BufferProperties, \
            0x306a6a7,0xb224,0x11d2,0x99,0xe5,0x0,0x0,0xe8,0xd8,0xc7,0x22);
DEFINE_GUID(DSPROPSETID_I3DL2_ListenerProperties, \
            0xDA0F0520,0x300A,0x11D3,0x8A,0x2B,0x00,0x60,0x97,0x0D,0xB0,0x11);
DEFINE_GUID(DSPROPSETID_I3DL2_BufferProperties, \
            0xDA0F0521,0x300A,0x11D3,0x8A,0x2B,0x00,0x60,0x97,0x0D,0xB0,0x11);
DEFINE_GUID(DSPROPSETID_ZOOMFX_BufferProperties, \
            0xCD5368E0,0x3450,0x11D3,0x8B,0x6E,0x00,0x10,0x5A,0x9B,0x7B,0xBC);

typedef HRESULT  (CALLBACK * MYPROC)(REFCLSID, REFIID, LPVOID *);

BOOL CALLBACK callback(PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA data,
                       LPVOID context)
{
    trace("found device:\n");
    trace("\tType: %s\n",
          data->Type == DIRECTSOUNDDEVICE_TYPE_EMULATED ? "Emulated" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_VXD ? "VxD" :
          data->Type == DIRECTSOUNDDEVICE_TYPE_WDM ? "WDM" : "Unknown");
    trace("\tDataFlow: %s\n",
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "Render" :
          data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ?
          "Capture" : "Unknown");
    trace("\tDeviceId: {%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
          data->DeviceId.Data1,data->DeviceId.Data2,data->DeviceId.Data3,
          data->DeviceId.Data4[0],data->DeviceId.Data4[1],
          data->DeviceId.Data4[2],data->DeviceId.Data4[3],
          data->DeviceId.Data4[4],data->DeviceId.Data4[5],
          data->DeviceId.Data4[6],data->DeviceId.Data4[7]);
    trace("\tDescription: %s\n", data->Description);
    trace("\tModule: %s\n", data->Module);
    trace("\tInterface: %s\n", data->Interface);
    trace("\tWaveDeviceId: %ld\n", data->WaveDeviceId);

    return TRUE;
}

static void propset_private_tests()
{
    HMODULE hDsound;
    HRESULT rc;
    IClassFactory * pcf;
    IKsPropertySet * pps;
    MYPROC fProc;
    ULONG support;

    hDsound = LoadLibrary("dsound.dll");
    ok(hDsound!=0,"LoadLibrary(dsound.dll) failed\n");
    if (hDsound==0)
        return;

    fProc = (MYPROC)GetProcAddress(hDsound, "DllGetClassObject");

    /* try direct sound first */
    rc = (fProc)(&CLSID_DirectSound, &IID_IClassFactory, (void **)0);
    ok(rc==DSERR_INVALIDPARAM,"DllGetClassObject(CLSID_DirectSound, "
       "IID_IClassFactory) should have returned DSERR_INVALIDPARAM, "
       "returned: %s\n",DXGetErrorString8(rc));

    rc = (fProc)(&CLSID_DirectSound, &IID_IClassFactory, (void **)(&pcf));
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSound, IID_IClassFactory) "
       "failed: %s\n",DXGetErrorString8(rc));
    if (pcf==0)
        goto error;

    /* direct sound doesn't have an IKsPropertySet */
    rc = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)0);
    ok(rc==DSERR_INVALIDPARAM, "CreateInstance(IID_IKsPropertySet) should have "
       "returned DSERR_INVALIDPARAM, returned: %s\n",DXGetErrorString8(rc));

    rc = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned E_NOINTERFACE, returned: %s\n",DXGetErrorString8(rc));

    /* and the direct sound 8 version */
    rc = (fProc)(&CLSID_DirectSound8, &IID_IClassFactory, (void **)(&pcf));
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSound8, IID_IClassFactory) "
       "failed: %s\n",DXGetErrorString8(rc));
    if (pcf==0)
        goto error;

    /* direct sound 8 doesn't have an IKsPropertySet */
    rc = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned E_NOINTERFACE, returned: %s\n",DXGetErrorString8(rc));

    /* try direct sound capture next */
    rc = (fProc)(&CLSID_DirectSoundCapture, &IID_IClassFactory,
                 (void **)(&pcf));
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundCapture, IID_IClassFactory) "
       "failed: %s\n",DXGetErrorString8(rc));
    if (pcf==0)
        goto error;

    /* direct sound capture doesn't have an IKsPropertySet */
    rc = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned E_NOINTERFACE,returned: %s\n",DXGetErrorString8(rc));

    /* and the direct sound capture 8 version */
    rc = (fProc)(&CLSID_DirectSoundCapture8, &IID_IClassFactory,
                 (void **)(&pcf));
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundCapture8, "
       "IID_IClassFactory) failed: %s\n",DXGetErrorString8(rc));
    if (pcf==0)
        goto error;

    /* direct sound capture 8 doesn't have an IKsPropertySet */
    rc = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned E_NOINTERFACE, returned: %s\n",DXGetErrorString8(rc));

    /* try direct sound full duplex next */
    rc = (fProc)(&CLSID_DirectSoundFullDuplex, &IID_IClassFactory,
                 (void **)(&pcf));
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundFullDuplex, "
       "IID_IClassFactory) failed: %s\n",DXGetErrorString8(rc));
    if (pcf==0)
        goto error;

    /* direct sound full duplex doesn't have an IKsPropertySet */
    rc = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==E_NOINTERFACE, "CreateInstance(IID_IKsPropertySet) should have "
       "returned NOINTERFACE, returned: %s\n",DXGetErrorString8(rc));

    /* try direct sound private last */
    rc = (fProc)(&CLSID_DirectSoundPrivate, &IID_IClassFactory,
                 (void **)(&pcf));
    ok(pcf!=0, "DllGetClassObject(CLSID_DirectSoundPrivate, "
       "IID_IClassFactory) failed: %s\n",DXGetErrorString8(rc));
    if (pcf==0)
        goto error;

    /* direct sound private does have an IKsPropertySet */
    rc = pcf->lpVtbl->CreateInstance(pcf, NULL, &IID_IKsPropertySet,
                                     (void **)(&pps));
    ok(rc==DS_OK, "CreateInstance(IID_IKsPropertySet) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto error;

    rc = pps->lpVtbl->QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
                                   &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto error;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),
       "Shouldn't be able to set DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION: "
       "support = 0x%lx\n",support);

    rc = pps->lpVtbl->QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
        DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING, &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto error;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET), "Shouldn't be able to set "
       "DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING: support = "
       "0x%lx\n",support);

    rc = pps->lpVtbl->QuerySupport(pps, &DSPROPSETID_DirectSoundDevice,
                                   DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE,
                                   &support);
    ok(rc==DS_OK, "QuerySupport(DSPROPSETID_DirectSoundDevice, "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE) failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto error;

    ok(support & KSPROPERTY_SUPPORT_GET,
       "Couldn't get DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE: "
       "support = 0x%lx\n",support);
    ok(!(support & KSPROPERTY_SUPPORT_SET),"Shouldn't be able to set "
       "DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE: support = 0x%lx\n",support);

    if (support & KSPROPERTY_SUPPORT_GET) {
        DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA data;
        ULONG bytes;

        data.Callback = callback;
        data.Context = 0;

        rc = pps->lpVtbl->Get(pps, &DSPROPSETID_DirectSoundDevice,
                              DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE,
                              NULL, 0, &data, sizeof(data), &bytes);
        ok(rc==DS_OK, "Couldn't enumerate: 0x%lx\n",rc);
    }

error:
    FreeLibrary(hDsound);
}

static BOOL WINAPI dsenum_callback(LPGUID lpGuid, LPCSTR lpcstrDescription,
                                   LPCSTR lpcstrModule, LPVOID lpContext)
{
    HRESULT rc;
    LPDIRECTSOUND dso=NULL;
    LPDIRECTSOUNDBUFFER primary=NULL,secondary=NULL;
    DSBUFFERDESC bufdesc;
    WAVEFORMATEX wfx;
    int ref;

    trace("*** Testing %s - %s ***\n",lpcstrDescription,lpcstrModule);

    rc=DirectSoundCreate(lpGuid,&dso,NULL);
    ok(rc==DS_OK||rc==DSERR_NODRIVER||rc==DSERR_ALLOCATED,
       "DirectSoundCreate() failed: %s\n",DXGetErrorString8(rc));
    if (rc!=DS_OK) {
        if (rc==DSERR_NODRIVER)
            trace("  No Driver\n");
        else if (rc == DSERR_ALLOCATED)
            trace("  Already In Use\n");
        goto EXIT;
    }

    /* We must call SetCooperativeLevel before calling CreateSoundBuffer */
    /* DSOUND: Setting DirectSound cooperative level to DSSCL_PRIORITY */
    rc=IDirectSound_SetCooperativeLevel(dso,get_hwnd(),DSSCL_PRIORITY);
    ok(rc==DS_OK,"IDirectSound_SetCooperativeLevel() failed: %s\n",
       DXGetErrorString8(rc));
    if (rc!=DS_OK)
        goto EXIT;

    /* Testing 3D buffers */
    primary=NULL;
    ZeroMemory(&bufdesc, sizeof(bufdesc));
    bufdesc.dwSize=sizeof(bufdesc);
    bufdesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_LOCHARDWARE|DSBCAPS_CTRL3D;
    rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&primary,NULL);
    ok(rc==DS_OK&&primary!=NULL,"IDirectSound_CreateSoundBuffer() failed to "
       "create a hardware 3D primary buffer: %s\n",DXGetErrorString8(rc));
    if (rc==DS_OK&&primary!=NULL) {
        ZeroMemory(&wfx, sizeof(wfx));
        wfx.wFormatTag=WAVE_FORMAT_PCM;
        wfx.nChannels=1;
        wfx.wBitsPerSample=16;
        wfx.nSamplesPerSec=44100;
        wfx.nBlockAlign=wfx.nChannels*wfx.wBitsPerSample/8;
        wfx.nAvgBytesPerSec=wfx.nSamplesPerSec*wfx.nBlockAlign;
        ZeroMemory(&bufdesc, sizeof(bufdesc));
        bufdesc.dwSize=sizeof(bufdesc);
        bufdesc.dwFlags=DSBCAPS_CTRLDEFAULT|DSBCAPS_GETCURRENTPOSITION2;
        bufdesc.dwBufferBytes=wfx.nAvgBytesPerSec;
        bufdesc.lpwfxFormat=&wfx;
        trace("  Testing a secondary buffer at %ldx%dx%d\n",
            wfx.nSamplesPerSec,wfx.wBitsPerSample,wfx.nChannels);
        rc=IDirectSound_CreateSoundBuffer(dso,&bufdesc,&secondary,NULL);
        ok(rc==DS_OK&&secondary!=NULL,"IDirectSound_CreateSoundBuffer() "
           "failed to create a secondary buffer: %s\n",DXGetErrorString8(rc));
        if (rc==DS_OK&&secondary!=NULL) {
            IKsPropertySet * pPropertySet=NULL;
            rc=IDirectSoundBuffer_QueryInterface(secondary,
                                                 &IID_IKsPropertySet,
                                                 (void **)&pPropertySet);
            /* it's not an error for this to fail */
            if(rc==DS_OK) {
                ULONG ulTypeSupport;
                trace("  Supports property sets\n");
                /* it's not an error for these to fail */
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                                               &DSPROPSETID_VoiceManager,
                                               0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                                                KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_VoiceManager supported\n");
                else
                    trace("    DSPROPSETID_VoiceManager not supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_EAX20_ListenerProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_EAX20_ListenerProperties "
                          "supported\n");
                else
                    trace("    DSPROPSETID_EAX20_ListenerProperties not "
                          "supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_EAX20_BufferProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_EAX20_BufferProperties supported\n");
                else
                    trace("    DSPROPSETID_EAX20_BufferProperties not "
                          "supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_I3DL2_ListenerProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_I3DL2_ListenerProperties "
                          "supported\n");
                else
                    trace("    DSPROPSETID_I3DL2_ListenerProperties not "
                          "supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_I3DL2_BufferProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_I3DL2_BufferProperties supported\n");
                else
                    trace("    DSPROPSETID_I3DL2_BufferProperties not "
                          "supported\n");
                rc=IKsPropertySet_QuerySupport(pPropertySet,
                    &DSPROPSETID_ZOOMFX_BufferProperties,0,&ulTypeSupport);
                if((rc==DS_OK)&&(ulTypeSupport&(KSPROPERTY_SUPPORT_GET|
                    KSPROPERTY_SUPPORT_SET)))
                    trace("    DSPROPSETID_ZOOMFX_BufferProperties "
                          "supported\n");
                else
                    trace("    DSPROPSETID_ZOOMFX_BufferProperties not "
                          "supported\n");
                ref=IKsPropertySet_Release(pPropertySet);
                /* try a few common ones */
                ok(ref==0,"IKsPropertySet_Release() secondary has %d "
                   "references, should have 0\n",ref);
            } else
                trace("  Doesn't support property sets\n");

            ref=IDirectSoundBuffer_Release(secondary);
            ok(ref==0,"IDirectSoundBuffer_Release() secondary has %d "
               "references, should have 0\n",ref);
        }

        ref=IDirectSoundBuffer_Release(primary);
        ok(ref==0,"IDirectSoundBuffer_Release() primary has %d references, "
           "should have 0\n",ref);
    }

EXIT:
    if (dso!=NULL) {
        ref=IDirectSound_Release(dso);
        ok(ref==0,"IDirectSound_Release() has %d references, should have 0\n",
           ref);
    }
    return 1;
}

static void propset_buffer_tests()
{
    HRESULT rc;
    rc=DirectSoundEnumerateA(&dsenum_callback,NULL);
    ok(rc==DS_OK,"DirectSoundEnumerateA() failed: %s\n",DXGetErrorString8(rc));
}

START_TEST(propset)
{
    propset_private_tests();
    propset_buffer_tests();
}
