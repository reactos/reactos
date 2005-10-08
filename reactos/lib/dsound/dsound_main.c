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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * Most thread locking is complete. There may be a few race
 * conditions still lurking.
 *
 * Tested with a Soundblaster clone, a Gravis UltraSound Classic,
 * and a Turtle Beach Tropez+.
 *
 * TODO:
 *	Implement SetCooperativeLevel properly (need to address focus issues)
 *	Implement DirectSound3DBuffers (stubs in place)
 *	Use hardware 3D support if available
 *      Add critical section locking inside Release and AddRef methods
 *      Handle static buffers - put those in hardware, non-static not in hardware
 *      Hardware DuplicateSoundBuffer
 *      Proper volume calculation, and setting volume in HEL primary buffer
 *      Optimize WINMM and negotiate fragment size, decrease DS_HEL_MARGIN
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"
#include "dsconf.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/* these are eligible for tuning... they must be high on slow machines... */
/* some stuff may get more responsive with lower values though... */
#define DS_EMULDRIVER 0 /* some games (Quake 2, UT) refuse to accept
				emulated dsound devices. set to 0 ! */
#define DS_HEL_MARGIN 5 /* HEL only: number of waveOut fragments ahead to mix in new buffers
			 * (keep this close or equal to DS_HEL_QUEUE for best results) */
#define DS_HEL_QUEUE  5 /* HEL only: number of waveOut fragments ahead to queue to driver
			 * (this will affect HEL sound reliability and latency) */

#define DS_SND_QUEUE_MAX 28 /* max number of fragments to prebuffer */
#define DS_SND_QUEUE_MIN 12 /* min number of fragments to prebuffer */

IDirectSoundImpl*	dsound = NULL;
GUID                    renderer_guids[MAXWAVEDRIVERS];
GUID                    capture_guids[MAXWAVEDRIVERS];

HRESULT mmErr(UINT err)
{
	switch(err) {
	case MMSYSERR_NOERROR:
		return DS_OK;
	case MMSYSERR_ALLOCATED:
		return DSERR_ALLOCATED;
	case MMSYSERR_ERROR:
	case MMSYSERR_INVALHANDLE:
	case WAVERR_STILLPLAYING:
		return DSERR_GENERIC; /* FIXME */
	case MMSYSERR_NODRIVER:
		return DSERR_NODRIVER;
	case MMSYSERR_NOMEM:
		return DSERR_OUTOFMEMORY;
	case MMSYSERR_INVALPARAM:
	case WAVERR_BADFORMAT:
	case WAVERR_UNPREPARED:
		return DSERR_INVALIDPARAM;
	case MMSYSERR_NOTSUPPORTED:
		return DSERR_UNSUPPORTED;
	default:
		FIXME("Unknown MMSYS error %d\n",err);
		return DSERR_GENERIC;
	}
}

int ds_emuldriver = DS_EMULDRIVER;
int ds_hel_margin = DS_HEL_MARGIN;
int ds_hel_queue = DS_HEL_QUEUE;
int ds_snd_queue_max = DS_SND_QUEUE_MAX;
int ds_snd_queue_min = DS_SND_QUEUE_MIN;
int ds_hw_accel = DS_HW_ACCEL_FULL;
int ds_default_playback = 0;
int ds_default_capture = 0;

/*
 * Get a config key from either the app-specific or the default config
 */

inline static DWORD get_config_key( HKEY defkey, HKEY appkey, const char *name,
                                    char *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExA( appkey, name, 0, NULL, buffer, &size )) return 0;
    return RegQueryValueExA( defkey, name, 0, NULL, buffer, &size );
}


/*
 * Setup the dsound options.
 */

void setup_dsound_options(void)
{
    char buffer[MAX_PATH+1];
    HKEY hkey, appkey = 0;
    DWORD len;

    buffer[MAX_PATH]='\0';

    if (RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\dsound", 0, NULL,
                         REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        ERR("Cannot create config registry key\n" );
        ExitProcess(1);
    }

    len = GetModuleFileNameA( 0, buffer, MAX_PATH );
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;

        if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\AppDefaults", &tmpkey ))
        {
           char appname[MAX_PATH+16];
           char *p = strrchr( buffer, '\\' );
           if (p!=NULL) {
                   appname[MAX_PATH]='\0';
                   strncpy(appname,p+1,MAX_PATH);
                   strcat(appname,"\\dsound");
                   TRACE("appname = [%s] \n",appname);
                   if (RegOpenKeyA( tmpkey, appname, &appkey )) appkey = 0;
                       RegCloseKey( tmpkey );
           }
        }
    }

    /* get options */

    if (!get_config_key( hkey, appkey, "EmulDriver", buffer, MAX_PATH ))
        ds_emuldriver = strcmp(buffer, "N");

    if (!get_config_key( hkey, appkey, "HELmargin", buffer, MAX_PATH ))
        ds_hel_margin = atoi(buffer);

    if (!get_config_key( hkey, appkey, "HELqueue", buffer, MAX_PATH ))
        ds_hel_queue = atoi(buffer);

    if (!get_config_key( hkey, appkey, "SndQueueMax", buffer, MAX_PATH ))
        ds_snd_queue_max = atoi(buffer);

    if (!get_config_key( hkey, appkey, "SndQueueMin", buffer, MAX_PATH ))
        ds_snd_queue_min = atoi(buffer);

    if (!get_config_key( hkey, appkey, "HardwareAcceleration", buffer, MAX_PATH )) {
	if (strcmp(buffer, "Full") == 0)
	    ds_hw_accel = DS_HW_ACCEL_FULL;
	else if (strcmp(buffer, "Standard") == 0)
	    ds_hw_accel = DS_HW_ACCEL_STANDARD;
	else if (strcmp(buffer, "Basic") == 0)
	    ds_hw_accel = DS_HW_ACCEL_BASIC;
	else if (strcmp(buffer, "Emulation") == 0)
	    ds_hw_accel = DS_HW_ACCEL_EMULATION;
    }
	FIXME("dsound are Hardcode to software emulation until we fix it in ros");
    ds_hw_accel = DS_HW_ACCEL_EMULATION;

    if (!get_config_key( hkey, appkey, "DefaultPlayback", buffer, MAX_PATH ))
	    ds_default_playback = atoi(buffer);

    if (!get_config_key( hkey, appkey, "DefaultCapture", buffer, MAX_PATH ))
	    ds_default_capture = atoi(buffer);

    if (appkey) RegCloseKey( appkey );
    RegCloseKey( hkey );

    if (ds_emuldriver != DS_EMULDRIVER )
       WARN("ds_emuldriver = %d (default=%d)\n",ds_emuldriver, DS_EMULDRIVER);
    if (ds_hel_margin != DS_HEL_MARGIN )
       WARN("ds_hel_margin = %d (default=%d)\n",ds_hel_margin, DS_HEL_MARGIN );
    if (ds_hel_queue != DS_HEL_QUEUE )
       WARN("ds_hel_queue = %d (default=%d)\n",ds_hel_queue, DS_HEL_QUEUE );
    if (ds_snd_queue_max != DS_SND_QUEUE_MAX)
       WARN("ds_snd_queue_max = %d (default=%d)\n",ds_snd_queue_max ,DS_SND_QUEUE_MAX);
    if (ds_snd_queue_min != DS_SND_QUEUE_MIN)
       WARN("ds_snd_queue_min = %d (default=%d)\n",ds_snd_queue_min ,DS_SND_QUEUE_MIN);
    if (ds_hw_accel != DS_HW_ACCEL_FULL)
	WARN("ds_hw_accel = %s (default=Full)\n",
	    ds_hw_accel==DS_HW_ACCEL_FULL ? "Full" :
	    ds_hw_accel==DS_HW_ACCEL_STANDARD ? "Standard" :
	    ds_hw_accel==DS_HW_ACCEL_BASIC ? "Basic" :
	    ds_hw_accel==DS_HW_ACCEL_EMULATION ? "Emulation" :
	    "Unknown");
    if (ds_default_playback != 0)
	WARN("ds_default_playback = %d (default=0)\n",ds_default_playback);
    if (ds_default_capture != 0)
	WARN("ds_default_capture = %d (default=0)\n",ds_default_playback);
}

const char * get_device_id(LPCGUID pGuid)
{
    if (IsEqualGUID(&DSDEVID_DefaultPlayback, pGuid))
        return "DSDEVID_DefaultPlayback";
    else if (IsEqualGUID(&DSDEVID_DefaultVoicePlayback, pGuid))
        return "DSDEVID_DefaultVoicePlayback";
    else if (IsEqualGUID(&DSDEVID_DefaultCapture, pGuid))
        return "DSDEVID_DefaultCapture";
    else if (IsEqualGUID(&DSDEVID_DefaultVoiceCapture, pGuid))
        return "DSDEVID_DefaultVoiceCapture";
    return debugstr_guid(pGuid);
}

/***************************************************************************
 * GetDeviceID	[DSOUND.9]
 *
 * Retrieves unique identifier of default device specified
 *
 * PARAMS
 *    pGuidSrc  [I] Address of device GUID.
 *    pGuidDest [O] Address to receive unique device GUID.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 *
 * NOTES
 *    pGuidSrc is a valid device GUID or DSDEVID_DefaultPlayback,
 *    DSDEVID_DefaultCapture, DSDEVID_DefaultVoicePlayback, or
 *    DSDEVID_DefaultVoiceCapture.
 *    Returns pGuidSrc if pGuidSrc is a valid device or the device
 *    GUID for the specified constants.
 */
HRESULT WINAPI GetDeviceID(LPCGUID pGuidSrc, LPGUID pGuidDest)
{
    TRACE("(%s,%p)\n", get_device_id(pGuidSrc),pGuidDest);

    if ( pGuidSrc == NULL) {
	WARN("invalid parameter: pGuidSrc == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if ( pGuidDest == NULL ) {
	WARN("invalid parameter: pGuidDest == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    if ( IsEqualGUID( &DSDEVID_DefaultPlayback, pGuidSrc ) ||
    	 IsEqualGUID( &DSDEVID_DefaultVoicePlayback, pGuidSrc ) ) {
	memcpy(pGuidDest, &renderer_guids[ds_default_playback], sizeof(GUID));
        TRACE("returns %s\n", get_device_id(pGuidDest));
	return DS_OK;
    }

    if ( IsEqualGUID( &DSDEVID_DefaultCapture, pGuidSrc ) ||
    	 IsEqualGUID( &DSDEVID_DefaultVoiceCapture, pGuidSrc ) ) {
	memcpy(pGuidDest, &capture_guids[ds_default_capture], sizeof(GUID));
        TRACE("returns %s\n", get_device_id(pGuidDest));
	return DS_OK;
    }

    memcpy(pGuidDest, pGuidSrc, sizeof(GUID));
    TRACE("returns %s\n", get_device_id(pGuidDest));

    return DS_OK;
}


/***************************************************************************
 * DirectSoundEnumerateA [DSOUND.2]
 *
 * Enumerate all DirectSound drivers installed in the system
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI DirectSoundEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    unsigned devs, wod;
    DSDRIVERDESC desc;
    GUID guid;
    int err;

    TRACE("lpDSEnumCallback = %p, lpContext = %p\n",
	lpDSEnumCallback, lpContext);

    if (lpDSEnumCallback == NULL) {
	WARN("invalid parameter: lpDSEnumCallback == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    devs = waveOutGetNumDevs();
    if (devs > 0) {
	if (GetDeviceID(&DSDEVID_DefaultPlayback, &guid) == DS_OK) {
	    for (wod = 0; wod < devs; ++wod) {
                if (IsEqualGUID( &guid, &renderer_guids[wod]) ) {
                    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
                    if (err == DS_OK) {
                        TRACE("calling lpDSEnumCallback(NULL,\"%s\",\"%s\",%p)\n",
                              "Primary Sound Driver",desc.szDrvname,lpContext);
                        if (lpDSEnumCallback(NULL, "Primary Sound Driver", desc.szDrvname, lpContext) == FALSE)
                            return DS_OK;
		    }
		}
	    }
	}
    }

    for (wod = 0; wod < devs; ++wod) {
	err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
	if (err == DS_OK) {
            TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
                  debugstr_guid(&renderer_guids[wod]),desc.szDesc,desc.szDrvname,lpContext);
            if (lpDSEnumCallback(&renderer_guids[wod], desc.szDesc, desc.szDrvname, lpContext) == FALSE)
                return DS_OK;
	}
    }
    return DS_OK;
}

/***************************************************************************
 * DirectSoundEnumerateW [DSOUND.3]
 *
 * Enumerate all DirectSound drivers installed in the system
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI DirectSoundEnumerateW(
	LPDSENUMCALLBACKW lpDSEnumCallback,
	LPVOID lpContext )
{
    unsigned devs, wod;
    DSDRIVERDESC desc;
    GUID guid;
    int err;
    WCHAR wDesc[MAXPNAMELEN];
    WCHAR wName[MAXPNAMELEN];

    TRACE("lpDSEnumCallback = %p, lpContext = %p\n",
	lpDSEnumCallback, lpContext);

    if (lpDSEnumCallback == NULL) {
	WARN("invalid parameter: lpDSEnumCallback == NULL\n");
	return DSERR_INVALIDPARAM;
    }

    devs = waveOutGetNumDevs();
    if (devs > 0) {
	if (GetDeviceID(&DSDEVID_DefaultPlayback, &guid) == DS_OK) {
	    for (wod = 0; wod < devs; ++wod) {
                if (IsEqualGUID( &guid, &renderer_guids[wod] ) ) {
                    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
                    if (err == DS_OK) {
                        TRACE("calling lpDSEnumCallback(NULL,\"%s\",\"%s\",%p)\n",
                              "Primary Sound Driver",desc.szDrvname,lpContext);
                        MultiByteToWideChar( CP_ACP, 0, "Primary Sound Driver", -1,
                                             wDesc, sizeof(wDesc)/sizeof(WCHAR) );
                        MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1,
                                             wName, sizeof(wName)/sizeof(WCHAR) );
                        if (lpDSEnumCallback(NULL, wDesc, wName, lpContext) == FALSE)
                            return DS_OK;
		    }
		}
	    }
	}
    }

    for (wod = 0; wod < devs; ++wod) {
	err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
	if (err == DS_OK) {
            TRACE("calling lpDSEnumCallback(%s,\"%s\",\"%s\",%p)\n",
                  debugstr_guid(&renderer_guids[wod]),desc.szDesc,desc.szDrvname,lpContext);
            MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1,
                                 wDesc, sizeof(wDesc)/sizeof(WCHAR) );
            MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1,
                                 wName, sizeof(wName)/sizeof(WCHAR) );
            if (lpDSEnumCallback(&renderer_guids[wod], wDesc, wName, lpContext) == FALSE)
                return DS_OK;
	}
    }
    return DS_OK;
}

/*******************************************************************************
 * DirectSound ClassFactory
 */

static HRESULT WINAPI
DSCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
DSCF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	TRACE("(%p) ref was %ld\n", This, This->ref);
	return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI DSCF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	/* static class, won't be  freed */
	TRACE("(%p) ref was %ld\n", This, This->ref);
	return InterlockedDecrement(&(This->ref));
}

static HRESULT WINAPI DSCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return DSERR_INVALIDPARAM;
	}

	*ppobj = NULL;

	if ( IsEqualIID( &IID_IDirectSound, riid ) )
		return DSOUND_Create(0,(LPDIRECTSOUND*)ppobj,pOuter);

	if ( IsEqualIID( &IID_IDirectSound8, riid ) )
		return DSOUND_Create8(0,(LPDIRECTSOUND8*)ppobj,pOuter);

	WARN("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
	return E_NOINTERFACE;
}

static HRESULT WINAPI DSCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static IClassFactoryVtbl DSCF_Vtbl = {
	DSCF_QueryInterface,
	DSCF_AddRef,
	DSCF_Release,
	DSCF_CreateInstance,
	DSCF_LockServer
};

static IClassFactoryImpl DSOUND_CF = { &DSCF_Vtbl, 1 };

/*******************************************************************************
 * DirectSoundPrivate ClassFactory
 */

static HRESULT WINAPI
DSPCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
DSPCF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	TRACE("(%p) ref was %ld\n", This, This->ref);
	return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI
DSPCF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	/* static class, won't be  freed */
	TRACE("(%p) ref was %ld\n", This, This->ref);
	return InterlockedDecrement(&(This->ref));
}

static HRESULT WINAPI
DSPCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return DSERR_INVALIDPARAM;
	}

	*ppobj = NULL;

	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
		return IKsPrivatePropertySetImpl_Create((IKsPrivatePropertySetImpl**)ppobj);
	}

	WARN("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
	return E_NOINTERFACE;
}

static HRESULT WINAPI
DSPCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static IClassFactoryVtbl DSPCF_Vtbl = {
	DSPCF_QueryInterface,
	DSPCF_AddRef,
	DSPCF_Release,
	DSPCF_CreateInstance,
	DSPCF_LockServer
};

static IClassFactoryImpl DSOUND_PRIVATE_CF = { &DSPCF_Vtbl, 1 };

/*******************************************************************************
 * DllGetClassObject [DSOUND.5]
 * Retrieves class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
DWORD WINAPI DSOUND_DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID *ppv)
{
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (ppv == NULL) {
	WARN("invalid parameter\n");
	return E_INVALIDARG;
    }

    *ppv = NULL;

    if ( IsEqualCLSID( &CLSID_DirectSound, rclsid ) ||
	 IsEqualCLSID( &CLSID_DirectSound8, rclsid ) ) {
	if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
	    *ppv = (LPVOID)&DSOUND_CF;
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    	WARN("(%s,%s,%p): no interface found.\n",
	    debugstr_guid(rclsid), debugstr_guid(riid), ppv);
	return S_FALSE;
    }

    if ( IsEqualCLSID( &CLSID_DirectSoundCapture, rclsid ) ||
	 IsEqualCLSID( &CLSID_DirectSoundCapture8, rclsid ) ) {
	if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
	    *ppv = (LPVOID)&DSOUND_CAPTURE_CF;
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    	WARN("(%s,%s,%p): no interface found.\n",
	    debugstr_guid(rclsid), debugstr_guid(riid), ppv);
	return S_FALSE;
    }

    if ( IsEqualCLSID( &CLSID_DirectSoundFullDuplex, rclsid ) ) {
	if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
	    *ppv = (LPVOID)&DSOUND_FULLDUPLEX_CF;
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    	WARN("(%s,%s,%p): no interface found.\n",
	    debugstr_guid(rclsid), debugstr_guid(riid), ppv);
	return S_FALSE;
    }

    if ( IsEqualCLSID( &CLSID_DirectSoundPrivate, rclsid ) ) {
	if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
	    *ppv = (LPVOID)&DSOUND_PRIVATE_CF;
	    IClassFactory_AddRef((IClassFactory*)*ppv);
	    return S_OK;
	}
    	WARN("(%s,%s,%p): no interface found.\n",
	    debugstr_guid(rclsid), debugstr_guid(riid), ppv);
	return S_FALSE;
    }

    WARN("(%s,%s,%p): no class found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}


/*******************************************************************************
 * DllCanUnloadNow [DSOUND.4]
 * Determines whether the DLL is in use.
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
DWORD WINAPI DSOUND_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");
    return S_FALSE;
}

#define INIT_GUID(guid, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)      \
        guid.Data1 = l; guid.Data2 = w1; guid.Data3 = w2;               \
        guid.Data4[0] = b1; guid.Data4[1] = b2; guid.Data4[2] = b3;     \
        guid.Data4[3] = b4; guid.Data4[4] = b5; guid.Data4[5] = b6;     \
        guid.Data4[6] = b7; guid.Data4[7] = b8;

/***********************************************************************
 *           DllMain (DSOUND.init)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    int i;
    TRACE("(%p 0x%lx %p)\n", hInstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        TRACE("DLL_PROCESS_ATTACH\n");
        for (i = 0; i < MAXWAVEDRIVERS; i++) {
            INIT_GUID(renderer_guids[i], 0xbd6dd71a, 0x3deb, 0x11d1, 0xb1, 0x71, 0x00, 0xc0, 0x4f, 0xc2, 0x00, 0x00 + i);			
            INIT_GUID(capture_guids[i],  0xbd6dd71b, 0x3deb, 0x11d1, 0xb1, 0x71, 0x00, 0xc0, 0x4f, 0xc2, 0x00, 0x00 + i);
        }
        break;
    case DLL_PROCESS_DETACH:
        TRACE("DLL_PROCESS_DETACH\n");
        break;
    case DLL_THREAD_ATTACH:
        TRACE("DLL_THREAD_ATTACH\n");
        break;
    case DLL_THREAD_DETACH:
        TRACE("DLL_THREAD_DETACH\n");
        break;
    default:
        TRACE("UNKNOWN REASON\n");
        break;
    }
    return TRUE;
}
