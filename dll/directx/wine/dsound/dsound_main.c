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
 *
 * Most thread locking is complete. There may be a few race
 * conditions still lurking.
 *
 * TODO:
 *	Implement SetCooperativeLevel properly (need to address focus issues)
 *	Implement DirectSound3DBuffers (stubs in place)
 *	Use hardware 3D support if available
 *      Add critical section locking inside Release and AddRef methods
 *      Handle static buffers - put those in hardware, non-static not in hardware
 *      Hardware DuplicateSoundBuffer
 *      Proper volume calculation for 3d buffers
 *      Remove DS_HEL_FRAGS and use mixer fragment length for it
 */

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsconf.h"
#include "ks.h"
#include "rpcproxy.h"
#include "rpc.h"
#include "rpcndr.h"
#include "unknwn.h"
#include "oleidl.h"
#include "shobjidl.h"
#include "strmif.h"

#include "initguid.h"
#include "ksmedia.h"
#include "propkey.h"
#include "devpkey.h"

#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

struct list DSOUND_renderers = LIST_INIT(DSOUND_renderers);
CRITICAL_SECTION DSOUND_renderers_lock;
static CRITICAL_SECTION_DEBUG DSOUND_renderers_lock_debug =
{
    0, 0, &DSOUND_renderers_lock,
    { &DSOUND_renderers_lock_debug.ProcessLocksList, &DSOUND_renderers_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": DSOUND_renderers_lock") }
};
CRITICAL_SECTION DSOUND_renderers_lock = { &DSOUND_renderers_lock_debug, -1, 0, 0, 0, 0 };

struct list DSOUND_capturers = LIST_INIT(DSOUND_capturers);
CRITICAL_SECTION DSOUND_capturers_lock;
static CRITICAL_SECTION_DEBUG DSOUND_capturers_lock_debug =
{
    0, 0, &DSOUND_capturers_lock,
    { &DSOUND_capturers_lock_debug.ProcessLocksList, &DSOUND_capturers_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": DSOUND_capturers_lock") }
};
CRITICAL_SECTION DSOUND_capturers_lock = { &DSOUND_capturers_lock_debug, -1, 0, 0, 0, 0 };

GUID                    DSOUND_renderer_guids[MAXWAVEDRIVERS];
GUID                    DSOUND_capture_guids[MAXWAVEDRIVERS];

const WCHAR wine_vxd_drv[] = { 'w','i','n','e','m','m','.','v','x','d', 0 };

/* All default settings, you most likely don't want to touch these, see wiki on UsefulRegistryKeys */
int ds_hel_buflen = 32768 * 2;
int ds_hq_buffers_max = 4;
BOOL ds_eax_enabled = FALSE;
static HINSTANCE instance;

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

/*
 * Get a config key from either the app-specific or the default config
 */

static inline DWORD get_config_key( HKEY defkey, HKEY appkey, const char *name,
                                    char *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;
    if (defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;
    return ERROR_FILE_NOT_FOUND;
}

/*
 * Setup the dsound options.
 */

void setup_dsound_options(void)
{
    char buffer[MAX_PATH+16];
    HKEY hkey, appkey = 0;
    DWORD len;

    buffer[MAX_PATH]='\0';

    /* @@ Wine registry key: HKCU\Software\Wine\DirectSound */
    if (RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\DirectSound", &hkey )) hkey = 0;

    len = GetModuleFileNameA( 0, buffer, MAX_PATH );
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;
        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\DirectSound */
        if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey ))
        {
            char *p, *appname = buffer;
            if ((p = strrchr( appname, '/' ))) appname = p + 1;
            if ((p = strrchr( appname, '\\' ))) appname = p + 1;
            strcat( appname, "\\DirectSound" );
            TRACE("appname = [%s]\n", appname);
            if (RegOpenKeyA( tmpkey, appname, &appkey )) appkey = 0;
            RegCloseKey( tmpkey );
        }
    }

    /* get options */

    if (!get_config_key( hkey, appkey, "HelBuflen", buffer, MAX_PATH ))
        ds_hel_buflen = atoi(buffer);

    if (!get_config_key( hkey, appkey, "HQBuffersMax", buffer, MAX_PATH ))
        ds_hq_buffers_max = atoi(buffer);

    if (!get_config_key( hkey, appkey, "EAXEnabled", buffer, MAX_PATH ))
        ds_eax_enabled = IS_OPTION_TRUE( buffer[0] );

    if (appkey) RegCloseKey( appkey );
    if (hkey) RegCloseKey( hkey );

    TRACE("ds_hel_buflen = %d\n", ds_hel_buflen);
    TRACE("ds_hq_buffers_max = %d\n", ds_hq_buffers_max);
    TRACE("ds_eax_enabled = %u\n", ds_eax_enabled);
}

static const char * get_device_id(LPCGUID pGuid)
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

static HRESULT get_mmdevenum(IMMDeviceEnumerator **devenum)
{
    HRESULT hr, init_hr;

    init_hr = CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
            CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)devenum);
    if(FAILED(hr)){
        if(SUCCEEDED(init_hr))
            CoUninitialize();
        *devenum = NULL;
        ERR("CoCreateInstance failed: %08x\n", hr);
        return hr;
    }

    return init_hr;
}

static void release_mmdevenum(IMMDeviceEnumerator *devenum, HRESULT init_hr)
{
    IMMDeviceEnumerator_Release(devenum);
    if(SUCCEEDED(init_hr))
        CoUninitialize();
}

static HRESULT get_mmdevice_guid(IMMDevice *device, IPropertyStore *ps,
        GUID *guid)
{
    PROPVARIANT pv;
    HRESULT hr;

    if(!ps){
        hr = IMMDevice_OpenPropertyStore(device, STGM_READ, &ps);
        if(FAILED(hr)){
            WARN("OpenPropertyStore failed: %08x\n", hr);
            return hr;
        }
    }else
        IPropertyStore_AddRef(ps);

    PropVariantInit(&pv);

    hr = IPropertyStore_GetValue(ps, &PKEY_AudioEndpoint_GUID, &pv);
    if(FAILED(hr)){
        IPropertyStore_Release(ps);
        WARN("GetValue(GUID) failed: %08x\n", hr);
        return hr;
    }

    CLSIDFromString(pv.u.pwszVal, guid);

    PropVariantClear(&pv);
    IPropertyStore_Release(ps);

    return S_OK;
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
    IMMDeviceEnumerator *devenum;
    EDataFlow flow = (EDataFlow)-1;
    ERole role = (ERole)-1;
    HRESULT hr, init_hr;

    TRACE("(%s,%p)\n", get_device_id(pGuidSrc),pGuidDest);

    if(!pGuidSrc || !pGuidDest)
        return DSERR_INVALIDPARAM;

    init_hr = get_mmdevenum(&devenum);
    if(!devenum)
        return init_hr;

    if(IsEqualGUID(&DSDEVID_DefaultPlayback, pGuidSrc)){
        role = eMultimedia;
        flow = eRender;
    }else if(IsEqualGUID(&DSDEVID_DefaultVoicePlayback, pGuidSrc)){
        role = eCommunications;
        flow = eRender;
    }else if(IsEqualGUID(&DSDEVID_DefaultCapture, pGuidSrc)){
        role = eMultimedia;
        flow = eCapture;
    }else if(IsEqualGUID(&DSDEVID_DefaultVoiceCapture, pGuidSrc)){
        role = eCommunications;
        flow = eCapture;
    }

    if(role != (ERole)-1 && flow != (EDataFlow)-1){
        IMMDevice *device;

        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(devenum,
                flow, role, &device);
        if(FAILED(hr)){
            WARN("GetDefaultAudioEndpoint failed: %08x\n", hr);
            release_mmdevenum(devenum, init_hr);
            return DSERR_NODRIVER;
        }

        hr = get_mmdevice_guid(device, NULL, pGuidDest);
        IMMDevice_Release(device);

        release_mmdevenum(devenum, init_hr);

        return (hr == S_OK) ? DS_OK : hr;
    }

    release_mmdevenum(devenum, init_hr);

    *pGuidDest = *pGuidSrc;

    return DS_OK;
}

struct morecontext
{
    LPDSENUMCALLBACKA callA;
    LPVOID data;
};

static BOOL CALLBACK a_to_w_callback(LPGUID guid, LPCWSTR descW, LPCWSTR modW, LPVOID data)
{
    struct morecontext *context = data;
    char descA[MAXPNAMELEN], modA[MAXPNAMELEN];

    WideCharToMultiByte(CP_ACP, 0, descW, -1, descA, sizeof(descA), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, modW, -1, modA, sizeof(modA), NULL, NULL);

    return context->callA(guid, descA, modA, context->data);
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
    struct morecontext context;

    if (lpDSEnumCallback == NULL) {
        WARN("invalid parameter: lpDSEnumCallback == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    context.callA = lpDSEnumCallback;
    context.data = lpContext;

    return DirectSoundEnumerateW(a_to_w_callback, &context);
}

HRESULT get_mmdevice(EDataFlow flow, const GUID *tgt, IMMDevice **device)
{
    IMMDeviceEnumerator *devenum;
    IMMDeviceCollection *coll;
    UINT count, i;
    HRESULT hr, init_hr;

    init_hr = get_mmdevenum(&devenum);
    if(!devenum)
        return init_hr;

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(devenum, flow,
            DEVICE_STATE_ACTIVE, &coll);
    if(FAILED(hr)){
        WARN("EnumAudioEndpoints failed: %08x\n", hr);
        release_mmdevenum(devenum, init_hr);
        return hr;
    }

    hr = IMMDeviceCollection_GetCount(coll, &count);
    if(FAILED(hr)){
        IMMDeviceCollection_Release(coll);
        release_mmdevenum(devenum, init_hr);
        WARN("GetCount failed: %08x\n", hr);
        return hr;
    }

    for(i = 0; i < count; ++i){
        GUID guid;

        hr = IMMDeviceCollection_Item(coll, i, device);
        if(FAILED(hr))
            continue;

        hr = get_mmdevice_guid(*device, NULL, &guid);
        if(FAILED(hr)){
            IMMDevice_Release(*device);
            continue;
        }

        if(IsEqualGUID(&guid, tgt)){
            IMMDeviceCollection_Release(coll);
            release_mmdevenum(devenum, init_hr);
            return DS_OK;
        }

        IMMDevice_Release(*device);
    }

    WARN("No device with GUID %s found!\n", wine_dbgstr_guid(tgt));

    IMMDeviceCollection_Release(coll);
    release_mmdevenum(devenum, init_hr);

    return DSERR_INVALIDPARAM;
}

static BOOL send_device(IMMDevice *device, GUID *guid,
        LPDSENUMCALLBACKW cb, void *user)
{
    IPropertyStore *ps;
    PROPVARIANT pv;
    BOOL keep_going;
    HRESULT hr;

    PropVariantInit(&pv);

    hr = IMMDevice_OpenPropertyStore(device, STGM_READ, &ps);
    if(FAILED(hr)){
        WARN("OpenPropertyStore failed: %08x\n", hr);
        return TRUE;
    }

    hr = get_mmdevice_guid(device, ps, guid);
    if(FAILED(hr)){
        IPropertyStore_Release(ps);
        return TRUE;
    }

    hr = IPropertyStore_GetValue(ps,
            (const PROPERTYKEY *)&DEVPKEY_Device_FriendlyName, &pv);
    if(FAILED(hr)){
        IPropertyStore_Release(ps);
        WARN("GetValue(FriendlyName) failed: %08x\n", hr);
        return TRUE;
    }

    TRACE("Calling back with %s (%s)\n", wine_dbgstr_guid(guid),
            wine_dbgstr_w(pv.u.pwszVal));

    keep_going = cb(guid, pv.u.pwszVal, wine_vxd_drv, user);

    PropVariantClear(&pv);
    IPropertyStore_Release(ps);

    return keep_going;
}

/* S_FALSE means the callback returned FALSE at some point
 * S_OK means the callback always returned TRUE */
HRESULT enumerate_mmdevices(EDataFlow flow, GUID *guids,
        LPDSENUMCALLBACKW cb, void *user)
{
    IMMDeviceEnumerator *devenum;
    IMMDeviceCollection *coll;
    IMMDevice *defdev = NULL;
    UINT count, i, n;
    BOOL keep_going;
    HRESULT hr, init_hr;

    static const WCHAR primary_desc[] = {'P','r','i','m','a','r','y',' ',
        'S','o','u','n','d',' ','D','r','i','v','e','r',0};
    static const WCHAR empty_drv[] = {0};

    init_hr = get_mmdevenum(&devenum);
    if(!devenum)
        return init_hr;

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(devenum, flow,
            DEVICE_STATE_ACTIVE, &coll);
    if(FAILED(hr)){
        release_mmdevenum(devenum, init_hr);
        WARN("EnumAudioEndpoints failed: %08x\n", hr);
        return DS_OK;
    }

    hr = IMMDeviceCollection_GetCount(coll, &count);
    if(FAILED(hr)){
        IMMDeviceCollection_Release(coll);
        release_mmdevenum(devenum, init_hr);
        WARN("GetCount failed: %08x\n", hr);
        return DS_OK;
    }

    if(count == 0){
        release_mmdevenum(devenum, init_hr);
        return DS_OK;
    }

    TRACE("Calling back with NULL (%s)\n", wine_dbgstr_w(primary_desc));
    keep_going = cb(NULL, primary_desc, empty_drv, user);

    /* always send the default device first */
    if(keep_going){
        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(devenum, flow,
                eMultimedia, &defdev);
        if(FAILED(hr)){
            defdev = NULL;
            n = 0;
        }else{
            keep_going = send_device(defdev, &guids[0], cb, user);
            n = 1;
        }
    }

    for(i = 0; keep_going && i < count; ++i){
        IMMDevice *device;

        hr = IMMDeviceCollection_Item(coll, i, &device);
        if(FAILED(hr)){
            WARN("Item failed: %08x\n", hr);
            continue;
        }

        if(device != defdev){
            keep_going = send_device(device, &guids[n], cb, user);
            ++n;
        }

        IMMDevice_Release(device);
    }

    if(defdev)
        IMMDevice_Release(defdev);
    IMMDeviceCollection_Release(coll);

    release_mmdevenum(devenum, init_hr);

    return keep_going ? S_OK : S_FALSE;
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
    HRESULT hr;

    TRACE("(%p,%p)\n", lpDSEnumCallback, lpContext);

    if (lpDSEnumCallback == NULL) {
        WARN("invalid parameter: lpDSEnumCallback == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    setup_dsound_options();

    hr = enumerate_mmdevices(eRender, DSOUND_renderer_guids,
            lpDSEnumCallback, lpContext);
    return SUCCEEDED(hr) ? DS_OK : hr;
}

/***************************************************************************
 * DirectSoundCaptureEnumerateA [DSOUND.7]
 *
 * Enumerate all DirectSound drivers installed in the system.
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI DirectSoundCaptureEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    struct morecontext context;

    if (lpDSEnumCallback == NULL) {
        WARN("invalid parameter: lpDSEnumCallback == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    context.callA = lpDSEnumCallback;
    context.data = lpContext;

    return DirectSoundCaptureEnumerateW(a_to_w_callback, &context);
}

/***************************************************************************
 * DirectSoundCaptureEnumerateW [DSOUND.8]
 *
 * Enumerate all DirectSound drivers installed in the system.
 *
 * PARAMS
 *    lpDSEnumCallback  [I] Address of callback function.
 *    lpContext         [I] Address of user defined context passed to callback function.
 *
 * RETURNS
 *    Success: DS_OK
 *    Failure: DSERR_INVALIDPARAM
 */
HRESULT WINAPI
DirectSoundCaptureEnumerateW(
    LPDSENUMCALLBACKW lpDSEnumCallback,
    LPVOID lpContext)
{
    HRESULT hr;

    TRACE("(%p,%p)\n", lpDSEnumCallback, lpContext );

    if (lpDSEnumCallback == NULL) {
        WARN("invalid parameter: lpDSEnumCallback == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    setup_dsound_options();

    hr = enumerate_mmdevices(eCapture, DSOUND_capture_guids,
            lpDSEnumCallback, lpContext);
    return SUCCEEDED(hr) ? DS_OK : hr;
}

/*******************************************************************************
 * DirectSound ClassFactory
 */

typedef  HRESULT (*FnCreateInstance)(REFIID riid, LPVOID *ppobj);
 
typedef struct {
    IClassFactory IClassFactory_iface;
    REFCLSID rclsid;
    FnCreateInstance pfnCreateInstance;
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI
DSCF_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);
    if (ppobj == NULL)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppobj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI DSCF_AddRef(LPCLASSFACTORY iface)
{
    return 2;
}

static ULONG WINAPI DSCF_Release(LPCLASSFACTORY iface)
{
    /* static class, won't be freed */
    return 1;
}

static HRESULT WINAPI DSCF_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pOuter,
    REFIID riid,
    LPVOID *ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    TRACE("(%p, %p, %s, %p)\n", This, pOuter, debugstr_guid(riid), ppobj);

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    if (ppobj == NULL) {
        WARN("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }
    *ppobj = NULL;
    return This->pfnCreateInstance(riid, ppobj);
}
 
static HRESULT WINAPI DSCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    FIXME("(%p, %d) stub!\n", This, dolock);
    return S_OK;
}

static const IClassFactoryVtbl DSCF_Vtbl = {
    DSCF_QueryInterface,
    DSCF_AddRef,
    DSCF_Release,
    DSCF_CreateInstance,
    DSCF_LockServer
};

static IClassFactoryImpl DSOUND_CF[] = {
    { { &DSCF_Vtbl }, &CLSID_DirectSound, DSOUND_Create },
    { { &DSCF_Vtbl }, &CLSID_DirectSound8, DSOUND_Create8 },
    { { &DSCF_Vtbl }, &CLSID_DirectSoundCapture, DSOUND_CaptureCreate },
    { { &DSCF_Vtbl }, &CLSID_DirectSoundCapture8, DSOUND_CaptureCreate8 },
    { { &DSCF_Vtbl }, &CLSID_DirectSoundFullDuplex, DSOUND_FullDuplexCreate },
    { { &DSCF_Vtbl }, &CLSID_DirectSoundPrivate, IKsPrivatePropertySetImpl_Create },
    { { NULL }, NULL, NULL }
};

/*******************************************************************************
 * DllGetClassObject [DSOUND.@]
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
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    int i = 0;
    TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (ppv == NULL) {
        WARN("invalid parameter\n");
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (!IsEqualIID(riid, &IID_IClassFactory) &&
        !IsEqualIID(riid, &IID_IUnknown)) {
        WARN("no interface for %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    while (NULL != DSOUND_CF[i].rclsid) {
        if (IsEqualGUID(rclsid, DSOUND_CF[i].rclsid)) {
            DSCF_AddRef(&DSOUND_CF[i].IClassFactory_iface);
            *ppv = &DSOUND_CF[i];
            return S_OK;
        }
        i++;
    }

    WARN("(%s, %s, %p): no class found.\n", debugstr_guid(rclsid),
         debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}


/*******************************************************************************
 * DllCanUnloadNow [DSOUND.4]
 * Determines whether the DLL is in use.
 *
 * RETURNS
 *    Can unload now: S_OK
 *    Cannot unload now (the DLL is still active): S_FALSE
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
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
    TRACE("(%p %d %p)\n", hInstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        instance = hInstDLL;
        DisableThreadLibraryCalls(hInstDLL);
        /* Increase refcount on dsound by 1 */
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)hInstDLL, &hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        if (lpvReserved) break;
        DeleteCriticalSection(&DSOUND_renderers_lock);
        DeleteCriticalSection(&DSOUND_capturers_lock);
        break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllRegisterServer (DSOUND.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (DSOUND.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
