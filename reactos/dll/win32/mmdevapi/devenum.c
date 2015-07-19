/*
 * Copyright 2009 Maarten Lankhorst
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

#include "mmdevapi.h"

#include <wine/list.h>

#include <oleauto.h>
#include <initguid.h>
#define _WINDOWS_H
#include <dshow.h>
#include <devpkey.h>

static const WCHAR software_mmdevapi[] =
    { 'S','o','f','t','w','a','r','e','\\',
      'M','i','c','r','o','s','o','f','t','\\',
      'W','i','n','d','o','w','s','\\',
      'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
      'M','M','D','e','v','i','c','e','s','\\',
      'A','u','d','i','o',0};
static const WCHAR reg_render[] =
    { 'R','e','n','d','e','r',0 };
static const WCHAR reg_capture[] =
    { 'C','a','p','t','u','r','e',0 };
static const WCHAR reg_devicestate[] =
    { 'D','e','v','i','c','e','S','t','a','t','e',0 };
static const WCHAR reg_properties[] =
    { 'P','r','o','p','e','r','t','i','e','s',0 };
static const WCHAR slashW[] = {'\\',0};
static const WCHAR reg_out_nameW[] = {'D','e','f','a','u','l','t','O','u','t','p','u','t',0};
static const WCHAR reg_vout_nameW[] = {'D','e','f','a','u','l','t','V','o','i','c','e','O','u','t','p','u','t',0};
static const WCHAR reg_in_nameW[] = {'D','e','f','a','u','l','t','I','n','p','u','t',0};
static const WCHAR reg_vin_nameW[] = {'D','e','f','a','u','l','t','V','o','i','c','e','I','n','p','u','t',0};

static HKEY key_render;
static HKEY key_capture;

typedef struct MMDevPropStoreImpl
{
    IPropertyStore IPropertyStore_iface;
    LONG ref;
    MMDevice *parent;
    DWORD access;
} MMDevPropStore;

typedef struct MMDevEnumImpl
{
    IMMDeviceEnumerator IMMDeviceEnumerator_iface;
    LONG ref;
} MMDevEnumImpl;

static MMDevEnumImpl *MMDevEnumerator;
static MMDevice **MMDevice_head;
static MMDevice *MMDevice_def_rec, *MMDevice_def_play;
static DWORD MMDevice_count;
static const IMMDeviceEnumeratorVtbl MMDevEnumVtbl;
static const IMMDeviceCollectionVtbl MMDevColVtbl;
static const IMMDeviceVtbl MMDeviceVtbl;
static const IPropertyStoreVtbl MMDevPropVtbl;
static const IMMEndpointVtbl MMEndpointVtbl;

static IMMDevice info_device;

typedef struct MMDevColImpl
{
    IMMDeviceCollection IMMDeviceCollection_iface;
    LONG ref;
    EDataFlow flow;
    DWORD state;
} MMDevColImpl;

typedef struct IPropertyBagImpl {
    IPropertyBag IPropertyBag_iface;
    GUID devguid;
} IPropertyBagImpl;

static const IPropertyBagVtbl PB_Vtbl;

static HRESULT MMDevPropStore_Create(MMDevice *This, DWORD access, IPropertyStore **ppv);

static inline MMDevPropStore *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, MMDevPropStore, IPropertyStore_iface);
}

static inline MMDevEnumImpl *impl_from_IMMDeviceEnumerator(IMMDeviceEnumerator *iface)
{
    return CONTAINING_RECORD(iface, MMDevEnumImpl, IMMDeviceEnumerator_iface);
}

static inline MMDevColImpl *impl_from_IMMDeviceCollection(IMMDeviceCollection *iface)
{
    return CONTAINING_RECORD(iface, MMDevColImpl, IMMDeviceCollection_iface);
}

static inline IPropertyBagImpl *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, IPropertyBagImpl, IPropertyBag_iface);
}

static const WCHAR propkey_formatW[] = {
    '{','%','0','8','X','-','%','0','4','X','-',
    '%','0','4','X','-','%','0','2','X','%','0','2','X','-',
    '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',
    '%','0','2','X','%','0','2','X','}',',','%','d',0 };

static HRESULT MMDevPropStore_OpenPropKey(const GUID *guid, DWORD flow, HKEY *propkey)
{
    WCHAR buffer[39];
    LONG ret;
    HKEY key;
    StringFromGUID2(guid, buffer, 39);
    if ((ret = RegOpenKeyExW(flow == eRender ? key_render : key_capture, buffer, 0, KEY_READ|KEY_WRITE|KEY_WOW64_64KEY, &key)) != ERROR_SUCCESS)
    {
        WARN("Opening key %s failed with %u\n", debugstr_w(buffer), ret);
        return E_FAIL;
    }
    ret = RegOpenKeyExW(key, reg_properties, 0, KEY_READ|KEY_WRITE|KEY_WOW64_64KEY, propkey);
    RegCloseKey(key);
    if (ret != ERROR_SUCCESS)
    {
        WARN("Opening key %s failed with %u\n", debugstr_w(reg_properties), ret);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT MMDevice_GetPropValue(const GUID *devguid, DWORD flow, REFPROPERTYKEY key, PROPVARIANT *pv)
{
    WCHAR buffer[80];
    const GUID *id = &key->fmtid;
    DWORD type, size;
    HRESULT hr = S_OK;
    HKEY regkey;
    LONG ret;

    hr = MMDevPropStore_OpenPropKey(devguid, flow, &regkey);
    if (FAILED(hr))
        return hr;
    wsprintfW( buffer, propkey_formatW, id->Data1, id->Data2, id->Data3,
               id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
               id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7], key->pid );
    ret = RegGetValueW(regkey, NULL, buffer, RRF_RT_ANY, &type, NULL, &size);
    if (ret != ERROR_SUCCESS)
    {
        WARN("Reading %s returned %d\n", debugstr_w(buffer), ret);
        RegCloseKey(regkey);
        PropVariantClear(pv);
        return S_OK;
    }

    switch (type)
    {
        case REG_SZ:
        {
            pv->vt = VT_LPWSTR;
            pv->u.pwszVal = CoTaskMemAlloc(size);
            if (!pv->u.pwszVal)
                hr = E_OUTOFMEMORY;
            else
                RegGetValueW(regkey, NULL, buffer, RRF_RT_REG_SZ, NULL, (BYTE*)pv->u.pwszVal, &size);
            break;
        }
        case REG_DWORD:
        {
            pv->vt = VT_UI4;
            RegGetValueW(regkey, NULL, buffer, RRF_RT_REG_DWORD, NULL, (BYTE*)&pv->u.ulVal, &size);
            break;
        }
        case REG_BINARY:
        {
            pv->vt = VT_BLOB;
            pv->u.blob.cbSize = size;
            pv->u.blob.pBlobData = CoTaskMemAlloc(size);
            if (!pv->u.blob.pBlobData)
                hr = E_OUTOFMEMORY;
            else
                RegGetValueW(regkey, NULL, buffer, RRF_RT_REG_BINARY, NULL, (BYTE*)pv->u.blob.pBlobData, &size);
            break;
        }
        default:
            ERR("Unknown/unhandled type: %u\n", type);
            PropVariantClear(pv);
            break;
    }
    RegCloseKey(regkey);
    return hr;
}

static HRESULT MMDevice_SetPropValue(const GUID *devguid, DWORD flow, REFPROPERTYKEY key, REFPROPVARIANT pv)
{
    WCHAR buffer[80];
    const GUID *id = &key->fmtid;
    HRESULT hr;
    HKEY regkey;
    LONG ret;

    hr = MMDevPropStore_OpenPropKey(devguid, flow, &regkey);
    if (FAILED(hr))
        return hr;
    wsprintfW( buffer, propkey_formatW, id->Data1, id->Data2, id->Data3,
               id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
               id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7], key->pid );
    switch (pv->vt)
    {
        case VT_UI4:
        {
            ret = RegSetValueExW(regkey, buffer, 0, REG_DWORD, (const BYTE*)&pv->u.ulVal, sizeof(DWORD));
            break;
        }
        case VT_BLOB:
        {
            ret = RegSetValueExW(regkey, buffer, 0, REG_BINARY, pv->u.blob.pBlobData, pv->u.blob.cbSize);
            TRACE("Blob %p %u\n", pv->u.blob.pBlobData, pv->u.blob.cbSize);

            break;
        }
        case VT_LPWSTR:
        {
            ret = RegSetValueExW(regkey, buffer, 0, REG_SZ, (const BYTE*)pv->u.pwszVal, sizeof(WCHAR)*(1+lstrlenW(pv->u.pwszVal)));
            break;
        }
        default:
            ret = 0;
            FIXME("Unhandled type %u\n", pv->vt);
            hr = E_INVALIDARG;
            break;
    }
    RegCloseKey(regkey);
    TRACE("Writing %s returned %u\n", debugstr_w(buffer), ret);
    return hr;
}

static HRESULT set_driver_prop_value(GUID *id, const EDataFlow flow, const PROPERTYKEY *prop)
{
    HRESULT hr;
    PROPVARIANT pv;

    if (!drvs.pGetPropValue)
        return E_NOTIMPL;

    hr = drvs.pGetPropValue(id, prop, &pv);

    if (SUCCEEDED(hr))
    {
        MMDevice_SetPropValue(id, flow, prop, &pv);
        PropVariantClear(&pv);
    }

    return hr;
}

/* Creates or updates the state of a device
 * If GUID is null, a random guid will be assigned
 * and the device will be created
 */
static MMDevice *MMDevice_Create(WCHAR *name, GUID *id, EDataFlow flow, DWORD state, BOOL setdefault)
{
    HKEY key, root;
    MMDevice *cur = NULL;
    WCHAR guidstr[39];
    DWORD i;

    static const PROPERTYKEY deviceinterface_key = {
        {0x233164c8, 0x1b2c, 0x4c7d, {0xbc, 0x68, 0xb6, 0x71, 0x68, 0x7a, 0x25, 0x67}}, 1
    };

    static const PROPERTYKEY devicepath_key = {
        {0xb3f8fa53, 0x0004, 0x438e, {0x90, 0x03, 0x51, 0xa4, 0x6e, 0x13, 0x9b, 0xfc}}, 2
    };

    for (i = 0; i < MMDevice_count; ++i)
    {
        MMDevice *device = MMDevice_head[i];
        if (device->flow == flow && IsEqualGUID(&device->devguid, id)){
            cur = device;
            break;
        }
    }

    if(!cur){
        /* No device found, allocate new one */
        cur = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*cur));
        if (!cur)
            return NULL;

        cur->IMMDevice_iface.lpVtbl = &MMDeviceVtbl;
        cur->IMMEndpoint_iface.lpVtbl = &MMEndpointVtbl;

        InitializeCriticalSection(&cur->crst);
        cur->crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": MMDevice.crst");

        if (!MMDevice_head)
            MMDevice_head = HeapAlloc(GetProcessHeap(), 0, sizeof(*MMDevice_head));
        else
            MMDevice_head = HeapReAlloc(GetProcessHeap(), 0, MMDevice_head, sizeof(*MMDevice_head)*(1+MMDevice_count));
        MMDevice_head[MMDevice_count++] = cur;
    }else if(cur->ref > 0)
        WARN("Modifying an MMDevice with postitive reference count!\n");

    HeapFree(GetProcessHeap(), 0, cur->drv_id);
    cur->drv_id = name;

    cur->flow = flow;
    cur->state = state;
    cur->devguid = *id;

    StringFromGUID2(&cur->devguid, guidstr, sizeof(guidstr)/sizeof(*guidstr));

    if (flow == eRender)
        root = key_render;
    else
        root = key_capture;

    if (RegCreateKeyExW(root, guidstr, 0, NULL, 0, KEY_WRITE|KEY_READ|KEY_WOW64_64KEY, NULL, &key, NULL) == ERROR_SUCCESS)
    {
        HKEY keyprop;
        RegSetValueExW(key, reg_devicestate, 0, REG_DWORD, (const BYTE*)&state, sizeof(DWORD));
        if (!RegCreateKeyExW(key, reg_properties, 0, NULL, 0, KEY_WRITE|KEY_READ|KEY_WOW64_64KEY, NULL, &keyprop, NULL))
        {
            PROPVARIANT pv;

            pv.vt = VT_LPWSTR;
            pv.u.pwszVal = name;
            MMDevice_SetPropValue(id, flow, (const PROPERTYKEY*)&DEVPKEY_Device_FriendlyName, &pv);
            MMDevice_SetPropValue(id, flow, (const PROPERTYKEY*)&DEVPKEY_Device_DeviceDesc, &pv);

            pv.u.pwszVal = guidstr;
            MMDevice_SetPropValue(id, flow, &deviceinterface_key, &pv);

            set_driver_prop_value(id, flow, &devicepath_key);

            if (FAILED(set_driver_prop_value(id, flow, &PKEY_AudioEndpoint_FormFactor)))
            {
                pv.vt = VT_UI4;
                pv.u.ulVal = (flow == eCapture) ? Microphone : Speakers;

                MMDevice_SetPropValue(id, flow, &PKEY_AudioEndpoint_FormFactor, &pv);
            }

            if (flow != eCapture)
            {
                PROPVARIANT pv2;

                PropVariantInit(&pv2);

                /* make read-write by not overwriting if already set */
                if (FAILED(MMDevice_GetPropValue(id, flow, &PKEY_AudioEndpoint_PhysicalSpeakers, &pv2)) || pv2.vt != VT_UI4)
                    set_driver_prop_value(id, flow, &PKEY_AudioEndpoint_PhysicalSpeakers);

                PropVariantClear(&pv2);
            }

            RegCloseKey(keyprop);
        }
        RegCloseKey(key);
    }

    if (setdefault)
    {
        if (flow == eRender)
            MMDevice_def_play = cur;
        else
            MMDevice_def_rec = cur;
    }
    return cur;
}

static HRESULT load_devices_from_reg(void)
{
    DWORD i = 0;
    HKEY root, cur;
    LONG ret;
    DWORD curflow;

    ret = RegCreateKeyExW(HKEY_LOCAL_MACHINE, software_mmdevapi, 0, NULL, 0, KEY_WRITE|KEY_READ|KEY_WOW64_64KEY, NULL, &root, NULL);
    if (ret == ERROR_SUCCESS)
        ret = RegCreateKeyExW(root, reg_capture, 0, NULL, 0, KEY_READ|KEY_WRITE|KEY_WOW64_64KEY, NULL, &key_capture, NULL);
    if (ret == ERROR_SUCCESS)
        ret = RegCreateKeyExW(root, reg_render, 0, NULL, 0, KEY_READ|KEY_WRITE|KEY_WOW64_64KEY, NULL, &key_render, NULL);
    RegCloseKey(root);
    cur = key_capture;
    curflow = eCapture;
    if (ret != ERROR_SUCCESS)
    {
        RegCloseKey(key_capture);
        key_render = key_capture = NULL;
        WARN("Couldn't create key: %u\n", ret);
        return E_FAIL;
    }

    do {
        WCHAR guidvalue[39];
        GUID guid;
        DWORD len;
        PROPVARIANT pv = { VT_EMPTY };

        len = sizeof(guidvalue)/sizeof(guidvalue[0]);
        ret = RegEnumKeyExW(cur, i++, guidvalue, &len, NULL, NULL, NULL, NULL);
        if (ret == ERROR_NO_MORE_ITEMS)
        {
            if (cur == key_capture)
            {
                cur = key_render;
                curflow = eRender;
                i = 0;
                continue;
            }
            break;
        }
        if (ret != ERROR_SUCCESS)
            continue;
        if (SUCCEEDED(CLSIDFromString(guidvalue, &guid))
            && SUCCEEDED(MMDevice_GetPropValue(&guid, curflow, (const PROPERTYKEY*)&DEVPKEY_Device_FriendlyName, &pv))
            && pv.vt == VT_LPWSTR)
        {
            DWORD size_bytes = (strlenW(pv.u.pwszVal) + 1) * sizeof(WCHAR);
            WCHAR *name = HeapAlloc(GetProcessHeap(), 0, size_bytes);
            memcpy(name, pv.u.pwszVal, size_bytes);
            MMDevice_Create(name, &guid, curflow,
                    DEVICE_STATE_NOTPRESENT, FALSE);
            CoTaskMemFree(pv.u.pwszVal);
        }
    } while (1);

    return S_OK;
}

static HRESULT set_format(MMDevice *dev)
{
    HRESULT hr;
    IAudioClient *client;
    WAVEFORMATEX *fmt;
    PROPVARIANT pv = { VT_EMPTY };

    hr = drvs.pGetAudioEndpoint(&dev->devguid, &dev->IMMDevice_iface, &client);
    if(FAILED(hr))
        return hr;

    hr = IAudioClient_GetMixFormat(client, &fmt);
    if(FAILED(hr)){
        IAudioClient_Release(client);
        return hr;
    }

    IAudioClient_Release(client);

    pv.vt = VT_BLOB;
    pv.u.blob.cbSize = sizeof(WAVEFORMATEX) + fmt->cbSize;
    pv.u.blob.pBlobData = (BYTE*)fmt;
    MMDevice_SetPropValue(&dev->devguid, dev->flow,
            &PKEY_AudioEngine_DeviceFormat, &pv);
    MMDevice_SetPropValue(&dev->devguid, dev->flow,
            &PKEY_AudioEngine_OEMFormat, &pv);
    CoTaskMemFree(fmt);

    return S_OK;
}

static HRESULT load_driver_devices(EDataFlow flow)
{
    WCHAR **ids;
    GUID *guids;
    UINT num, def, i;
    HRESULT hr;

    if(!drvs.pGetEndpointIDs)
        return S_OK;

    hr = drvs.pGetEndpointIDs(flow, &ids, &guids, &num, &def);
    if(FAILED(hr))
        return hr;

    for(i = 0; i < num; ++i){
        MMDevice *dev;
        dev = MMDevice_Create(ids[i], &guids[i], flow, DEVICE_STATE_ACTIVE,
                def == i);
        set_format(dev);
    }

    HeapFree(GetProcessHeap(), 0, guids);
    HeapFree(GetProcessHeap(), 0, ids);

    return S_OK;
}

static void MMDevice_Destroy(MMDevice *This)
{
    DWORD i;
    TRACE("Freeing %s\n", debugstr_w(This->drv_id));
    /* Since this function is called at destruction time, reordering of the list is unimportant */
    for (i = 0; i < MMDevice_count; ++i)
    {
        if (MMDevice_head[i] == This)
        {
            MMDevice_head[i] = MMDevice_head[--MMDevice_count];
            break;
        }
    }
    This->crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->crst);
    HeapFree(GetProcessHeap(), 0, This->drv_id);
    HeapFree(GetProcessHeap(), 0, This);
}

static inline MMDevice *impl_from_IMMDevice(IMMDevice *iface)
{
    return CONTAINING_RECORD(iface, MMDevice, IMMDevice_iface);
}

static HRESULT WINAPI MMDevice_QueryInterface(IMMDevice *iface, REFIID riid, void **ppv)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IMMDevice))
        *ppv = &This->IMMDevice_iface;
    else if (IsEqualIID(riid, &IID_IMMEndpoint))
        *ppv = &This->IMMEndpoint_iface;
    if (*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MMDevice_AddRef(IMMDevice *iface)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    LONG ref;

    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI MMDevice_Release(IMMDevice *iface)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    LONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static HRESULT WINAPI MMDevice_Activate(IMMDevice *iface, REFIID riid, DWORD clsctx, PROPVARIANT *params, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    MMDevice *This = impl_from_IMMDevice(iface);

    TRACE("(%p)->(%p,%x,%p,%p)\n", iface, riid, clsctx, params, ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IAudioClient)){
        hr = drvs.pGetAudioEndpoint(&This->devguid, iface, (IAudioClient**)ppv);
    }else if (IsEqualIID(riid, &IID_IAudioEndpointVolume))
        hr = AudioEndpointVolume_Create(This, (IAudioEndpointVolume**)ppv);
    else if (IsEqualIID(riid, &IID_IAudioSessionManager)
             || IsEqualIID(riid, &IID_IAudioSessionManager2))
    {
        hr = drvs.pGetAudioSessionManager(iface, (IAudioSessionManager2**)ppv);
    }
    else if (IsEqualIID(riid, &IID_IBaseFilter))
    {
        if (This->flow == eRender)
            hr = CoCreateInstance(&CLSID_DSoundRender, NULL, clsctx, riid, ppv);
        else
            ERR("Not supported for recording?\n");
        if (SUCCEEDED(hr))
        {
            IPersistPropertyBag *ppb;
            hr = IUnknown_QueryInterface((IUnknown*)*ppv, &IID_IPersistPropertyBag, (void*)&ppb);
            if (SUCCEEDED(hr))
            {
                /* ::Load cannot assume the interface stays alive after the function returns,
                 * so just create the interface on the stack, saves a lot of complicated code */
                IPropertyBagImpl bag = { { &PB_Vtbl } };
                bag.devguid = This->devguid;
                hr = IPersistPropertyBag_Load(ppb, &bag.IPropertyBag_iface, NULL);
                IPersistPropertyBag_Release(ppb);
                if (FAILED(hr))
                    IBaseFilter_Release((IBaseFilter*)*ppv);
            }
            else
            {
                FIXME("Wine doesn't support IPersistPropertyBag on DSoundRender yet, ignoring..\n");
                hr = S_OK;
            }
        }
    }
    else if (IsEqualIID(riid, &IID_IDeviceTopology))
    {
        FIXME("IID_IDeviceTopology unsupported\n");
    }
    else if (IsEqualIID(riid, &IID_IDirectSound)
             || IsEqualIID(riid, &IID_IDirectSound8))
    {
        if (This->flow == eRender)
            hr = CoCreateInstance(&CLSID_DirectSound8, NULL, clsctx, riid, ppv);
        if (SUCCEEDED(hr))
        {
            hr = IDirectSound_Initialize((IDirectSound*)*ppv, &This->devguid);
            if (FAILED(hr))
                IDirectSound_Release((IDirectSound*)*ppv);
        }
    }
    else if (IsEqualIID(riid, &IID_IDirectSoundCapture))
    {
        if (This->flow == eCapture)
            hr = CoCreateInstance(&CLSID_DirectSoundCapture8, NULL, clsctx, riid, ppv);
        if (SUCCEEDED(hr))
        {
            hr = IDirectSoundCapture_Initialize((IDirectSoundCapture*)*ppv, &This->devguid);
            if (FAILED(hr))
                IDirectSoundCapture_Release((IDirectSoundCapture*)*ppv);
        }
    }
    else
        ERR("Invalid/unknown iid %s\n", debugstr_guid(riid));

    if (FAILED(hr))
        *ppv = NULL;

    TRACE("Returning %08x\n", hr);
    return hr;
}

static HRESULT WINAPI MMDevice_OpenPropertyStore(IMMDevice *iface, DWORD access, IPropertyStore **ppv)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    TRACE("(%p)->(%x,%p)\n", This, access, ppv);

    if (!ppv)
        return E_POINTER;
    return MMDevPropStore_Create(This, access, ppv);
}

static HRESULT WINAPI MMDevice_GetId(IMMDevice *iface, WCHAR **itemid)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    WCHAR *str;
    GUID *id = &This->devguid;
    static const WCHAR formatW[] = { '{','0','.','0','.','%','u','.','0','0','0','0','0','0','0','0','}','.',
                                     '{','%','0','8','X','-','%','0','4','X','-',
                                     '%','0','4','X','-','%','0','2','X','%','0','2','X','-',
                                     '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',
                                     '%','0','2','X','%','0','2','X','}',0 };

    TRACE("(%p)->(%p)\n", This, itemid);
    if (!itemid)
        return E_POINTER;
    *itemid = str = CoTaskMemAlloc(56 * sizeof(WCHAR));
    if (!str)
        return E_OUTOFMEMORY;
    wsprintfW( str, formatW, This->flow, id->Data1, id->Data2, id->Data3,
               id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
               id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    TRACE("returning %s\n", wine_dbgstr_w(str));
    return S_OK;
}

static HRESULT WINAPI MMDevice_GetState(IMMDevice *iface, DWORD *state)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    TRACE("(%p)->(%p)\n", iface, state);

    if (!state)
        return E_POINTER;
    *state = This->state;
    return S_OK;
}

static const IMMDeviceVtbl MMDeviceVtbl =
{
    MMDevice_QueryInterface,
    MMDevice_AddRef,
    MMDevice_Release,
    MMDevice_Activate,
    MMDevice_OpenPropertyStore,
    MMDevice_GetId,
    MMDevice_GetState
};

static inline MMDevice *impl_from_IMMEndpoint(IMMEndpoint *iface)
{
    return CONTAINING_RECORD(iface, MMDevice, IMMEndpoint_iface);
}

static HRESULT WINAPI MMEndpoint_QueryInterface(IMMEndpoint *iface, REFIID riid, void **ppv)
{
    MMDevice *This = impl_from_IMMEndpoint(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);
    return IMMDevice_QueryInterface(&This->IMMDevice_iface, riid, ppv);
}

static ULONG WINAPI MMEndpoint_AddRef(IMMEndpoint *iface)
{
    MMDevice *This = impl_from_IMMEndpoint(iface);
    TRACE("(%p)\n", This);
    return IMMDevice_AddRef(&This->IMMDevice_iface);
}

static ULONG WINAPI MMEndpoint_Release(IMMEndpoint *iface)
{
    MMDevice *This = impl_from_IMMEndpoint(iface);
    TRACE("(%p)\n", This);
    return IMMDevice_Release(&This->IMMDevice_iface);
}

static HRESULT WINAPI MMEndpoint_GetDataFlow(IMMEndpoint *iface, EDataFlow *flow)
{
    MMDevice *This = impl_from_IMMEndpoint(iface);
    TRACE("(%p)->(%p)\n", This, flow);
    if (!flow)
        return E_POINTER;
    *flow = This->flow;
    return S_OK;
}

static const IMMEndpointVtbl MMEndpointVtbl =
{
    MMEndpoint_QueryInterface,
    MMEndpoint_AddRef,
    MMEndpoint_Release,
    MMEndpoint_GetDataFlow
};

static HRESULT MMDevCol_Create(IMMDeviceCollection **ppv, EDataFlow flow, DWORD state)
{
    MMDevColImpl *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    *ppv = NULL;
    if (!This)
        return E_OUTOFMEMORY;
    This->IMMDeviceCollection_iface.lpVtbl = &MMDevColVtbl;
    This->ref = 1;
    This->flow = flow;
    This->state = state;
    *ppv = &This->IMMDeviceCollection_iface;
    return S_OK;
}

static void MMDevCol_Destroy(MMDevColImpl *This)
{
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI MMDevCol_QueryInterface(IMMDeviceCollection *iface, REFIID riid, void **ppv)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IMMDeviceCollection))
        *ppv = &This->IMMDeviceCollection_iface;
    else
        *ppv = NULL;
    if (!*ppv)
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MMDevCol_AddRef(IMMDeviceCollection *iface)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI MMDevCol_Release(IMMDeviceCollection *iface)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        MMDevCol_Destroy(This);
    return ref;
}

static HRESULT WINAPI MMDevCol_GetCount(IMMDeviceCollection *iface, UINT *numdevs)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);
    DWORD i;

    TRACE("(%p)->(%p)\n", This, numdevs);
    if (!numdevs)
        return E_POINTER;

    *numdevs = 0;
    for (i = 0; i < MMDevice_count; ++i)
    {
        MMDevice *cur = MMDevice_head[i];
        if ((cur->flow == This->flow || This->flow == eAll)
            && (cur->state & This->state))
            ++(*numdevs);
    }
    return S_OK;
}

static HRESULT WINAPI MMDevCol_Item(IMMDeviceCollection *iface, UINT n, IMMDevice **dev)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);
    DWORD i = 0, j = 0;

    TRACE("(%p)->(%u, %p)\n", This, n, dev);
    if (!dev)
        return E_POINTER;

    for (j = 0; j < MMDevice_count; ++j)
    {
        MMDevice *cur = MMDevice_head[j];
        if ((cur->flow == This->flow || This->flow == eAll)
            && (cur->state & This->state)
            && i++ == n)
        {
            *dev = &cur->IMMDevice_iface;
            IMMDevice_AddRef(*dev);
            return S_OK;
        }
    }
    WARN("Could not obtain item %u\n", n);
    *dev = NULL;
    return E_INVALIDARG;
}

static const IMMDeviceCollectionVtbl MMDevColVtbl =
{
    MMDevCol_QueryInterface,
    MMDevCol_AddRef,
    MMDevCol_Release,
    MMDevCol_GetCount,
    MMDevCol_Item
};

HRESULT MMDevEnum_Create(REFIID riid, void **ppv)
{
    MMDevEnumImpl *This = MMDevEnumerator;

    if (!This)
    {
        This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
        *ppv = NULL;
        if (!This)
            return E_OUTOFMEMORY;
        This->ref = 1;
        This->IMMDeviceEnumerator_iface.lpVtbl = &MMDevEnumVtbl;
        MMDevEnumerator = This;

        load_devices_from_reg();
        load_driver_devices(eRender);
        load_driver_devices(eCapture);
    }
    return IMMDeviceEnumerator_QueryInterface(&This->IMMDeviceEnumerator_iface, riid, ppv);
}

void MMDevEnum_Free(void)
{
    while (MMDevice_count)
        MMDevice_Destroy(MMDevice_head[0]);
    RegCloseKey(key_render);
    RegCloseKey(key_capture);
    key_render = key_capture = NULL;
    HeapFree(GetProcessHeap(), 0, MMDevEnumerator);
    MMDevEnumerator = NULL;
}

static HRESULT WINAPI MMDevEnum_QueryInterface(IMMDeviceEnumerator *iface, REFIID riid, void **ppv)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IMMDeviceEnumerator))
        *ppv = &This->IMMDeviceEnumerator_iface;
    else
        *ppv = NULL;
    if (!*ppv)
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MMDevEnum_AddRef(IMMDeviceEnumerator *iface)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI MMDevEnum_Release(IMMDeviceEnumerator *iface)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    if (!ref)
        MMDevEnum_Free();
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static HRESULT WINAPI MMDevEnum_EnumAudioEndpoints(IMMDeviceEnumerator *iface, EDataFlow flow, DWORD mask, IMMDeviceCollection **devices)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    TRACE("(%p)->(%u,%u,%p)\n", This, flow, mask, devices);
    if (!devices)
        return E_POINTER;
    *devices = NULL;
    if (flow >= EDataFlow_enum_count)
        return E_INVALIDARG;
    if (mask & ~DEVICE_STATEMASK_ALL)
        return E_INVALIDARG;
    return MMDevCol_Create(devices, flow, mask);
}

static HRESULT WINAPI MMDevEnum_GetDefaultAudioEndpoint(IMMDeviceEnumerator *iface, EDataFlow flow, ERole role, IMMDevice **device)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    WCHAR reg_key[256];
    HKEY key;
    HRESULT hr;

    TRACE("(%p)->(%u,%u,%p)\n", This, flow, role, device);

    if (!device)
        return E_POINTER;

    if((flow != eRender && flow != eCapture) ||
            (role != eConsole && role != eMultimedia && role != eCommunications)){
        WARN("Unknown flow (%u) or role (%u)\n", flow, role);
        return E_INVALIDARG;
    }

    *device = NULL;

    if(!drvs.module_name[0])
        return E_NOTFOUND;

    lstrcpyW(reg_key, drv_keyW);
    lstrcatW(reg_key, slashW);
    lstrcatW(reg_key, drvs.module_name);

    if(RegOpenKeyW(HKEY_CURRENT_USER, reg_key, &key) == ERROR_SUCCESS){
        const WCHAR *reg_x_name, *reg_vx_name;
        WCHAR def_id[256];
        DWORD size = sizeof(def_id), state;

        if(flow == eRender){
            reg_x_name = reg_out_nameW;
            reg_vx_name = reg_vout_nameW;
        }else{
            reg_x_name = reg_in_nameW;
            reg_vx_name = reg_vin_nameW;
        }

        if(role == eCommunications &&
                RegQueryValueExW(key, reg_vx_name, 0, NULL,
                    (BYTE*)def_id, &size) == ERROR_SUCCESS){
            hr = IMMDeviceEnumerator_GetDevice(iface, def_id, device);
            if(SUCCEEDED(hr)){
                if(SUCCEEDED(IMMDevice_GetState(*device, &state)) &&
                        state == DEVICE_STATE_ACTIVE){
                    RegCloseKey(key);
                    return S_OK;
                }
            }

            TRACE("Unable to find voice device %s\n", wine_dbgstr_w(def_id));
        }

        if(RegQueryValueExW(key, reg_x_name, 0, NULL,
                    (BYTE*)def_id, &size) == ERROR_SUCCESS){
            hr = IMMDeviceEnumerator_GetDevice(iface, def_id, device);
            if(SUCCEEDED(hr)){
                if(SUCCEEDED(IMMDevice_GetState(*device, &state)) &&
                        state == DEVICE_STATE_ACTIVE){
                    RegCloseKey(key);
                    return S_OK;
                }
            }

            TRACE("Unable to find device %s\n", wine_dbgstr_w(def_id));
        }

        RegCloseKey(key);
    }

    if (flow == eRender)
        *device = &MMDevice_def_play->IMMDevice_iface;
    else
        *device = &MMDevice_def_rec->IMMDevice_iface;

    if (!*device)
        return E_NOTFOUND;
    IMMDevice_AddRef(*device);
    return S_OK;
}

static HRESULT WINAPI MMDevEnum_GetDevice(IMMDeviceEnumerator *iface, const WCHAR *name, IMMDevice **device)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    DWORD i=0;
    IMMDevice *dev = NULL;

    static const WCHAR wine_info_deviceW[] = {'W','i','n','e',' ',
        'i','n','f','o',' ','d','e','v','i','c','e',0};

    TRACE("(%p)->(%s,%p)\n", This, debugstr_w(name), device);

    if(!name || !device)
        return E_POINTER;

    if(!lstrcmpW(name, wine_info_deviceW)){
        *device = &info_device;
        return S_OK;
    }

    for (i = 0; i < MMDevice_count; ++i)
    {
        WCHAR *str;
        dev = &MMDevice_head[i]->IMMDevice_iface;
        IMMDevice_GetId(dev, &str);

        if (str && !lstrcmpW(str, name))
        {
            CoTaskMemFree(str);
            IMMDevice_AddRef(dev);
            *device = dev;
            return S_OK;
        }
        CoTaskMemFree(str);
    }
    TRACE("Could not find device %s\n", debugstr_w(name));
    return E_INVALIDARG;
}

struct NotificationClientWrapper {
    IMMNotificationClient *client;
    struct list entry;
};

static struct list g_notif_clients = LIST_INIT(g_notif_clients);
static HANDLE g_notif_thread;

static CRITICAL_SECTION g_notif_lock;
static CRITICAL_SECTION_DEBUG g_notif_lock_debug =
{
    0, 0, &g_notif_lock,
    { &g_notif_lock_debug.ProcessLocksList, &g_notif_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": g_notif_lock") }
};
static CRITICAL_SECTION g_notif_lock = { &g_notif_lock_debug, -1, 0, 0, 0, 0 };

static void notify_clients(EDataFlow flow, ERole role, const WCHAR *id)
{
    struct NotificationClientWrapper *wrapper;
    LIST_FOR_EACH_ENTRY(wrapper, &g_notif_clients,
            struct NotificationClientWrapper, entry)
        IMMNotificationClient_OnDefaultDeviceChanged(wrapper->client, flow,
                role, id);

    /* Windows 7 treats changes to eConsole as changes to eMultimedia */
    if(role == eConsole)
        notify_clients(flow, eMultimedia, id);
}

static BOOL notify_if_changed(EDataFlow flow, ERole role, HKEY key,
                              const WCHAR *val_name, WCHAR *old_val, IMMDevice *def_dev)
{
    WCHAR new_val[64], *id;
    DWORD size;
    HRESULT hr;

    size = sizeof(new_val);
    if(RegQueryValueExW(key, val_name, 0, NULL,
                (BYTE*)new_val, &size) != ERROR_SUCCESS){
        if(old_val[0] != 0){
            /* set by user -> system default */
            if(def_dev){
                hr = IMMDevice_GetId(def_dev, &id);
                if(FAILED(hr)){
                    ERR("GetId failed: %08x\n", hr);
                    return FALSE;
                }
            }else
                id = NULL;

            notify_clients(flow, role, id);
            old_val[0] = 0;
            CoTaskMemFree(id);

            return TRUE;
        }

        /* system default -> system default, noop */
        return FALSE;
    }

    if(!lstrcmpW(old_val, new_val)){
        /* set by user -> same value */
        return FALSE;
    }

    if(new_val[0] != 0){
        /* set by user -> different value */
        notify_clients(flow, role, new_val);
        memcpy(old_val, new_val, sizeof(new_val));
        return TRUE;
    }

    /* set by user -> system default */
    if(def_dev){
        hr = IMMDevice_GetId(def_dev, &id);
        if(FAILED(hr)){
            ERR("GetId failed: %08x\n", hr);
            return FALSE;
        }
    }else
        id = NULL;

    notify_clients(flow, role, id);
    old_val[0] = 0;
    CoTaskMemFree(id);

    return TRUE;
}

static DWORD WINAPI notif_thread_proc(void *user)
{
    HKEY key;
    WCHAR reg_key[256];
    WCHAR out_name[64], vout_name[64], in_name[64], vin_name[64];
    DWORD size;

    lstrcpyW(reg_key, drv_keyW);
    lstrcatW(reg_key, slashW);
    lstrcatW(reg_key, drvs.module_name);

    if(RegCreateKeyExW(HKEY_CURRENT_USER, reg_key, 0, NULL, 0,
                MAXIMUM_ALLOWED, NULL, &key, NULL) != ERROR_SUCCESS){
        ERR("RegCreateKeyEx failed: %u\n", GetLastError());
        return 1;
    }

    size = sizeof(out_name);
    if(RegQueryValueExW(key, reg_out_nameW, 0, NULL,
                (BYTE*)out_name, &size) != ERROR_SUCCESS)
        out_name[0] = 0;

    size = sizeof(vout_name);
    if(RegQueryValueExW(key, reg_vout_nameW, 0, NULL,
                (BYTE*)vout_name, &size) != ERROR_SUCCESS)
        vout_name[0] = 0;

    size = sizeof(in_name);
    if(RegQueryValueExW(key, reg_in_nameW, 0, NULL,
                (BYTE*)in_name, &size) != ERROR_SUCCESS)
        in_name[0] = 0;

    size = sizeof(vin_name);
    if(RegQueryValueExW(key, reg_vin_nameW, 0, NULL,
                (BYTE*)vin_name, &size) != ERROR_SUCCESS)
        vin_name[0] = 0;

    while(1){
        if(RegNotifyChangeKeyValue(key, FALSE, REG_NOTIFY_CHANGE_LAST_SET,
                    NULL, FALSE) != ERROR_SUCCESS){
            ERR("RegNotifyChangeKeyValue failed: %u\n", GetLastError());
            RegCloseKey(key);
            g_notif_thread = NULL;
            return 1;
        }

        EnterCriticalSection(&g_notif_lock);

        notify_if_changed(eRender, eConsole, key, reg_out_nameW,
                out_name, &MMDevice_def_play->IMMDevice_iface);
        notify_if_changed(eRender, eCommunications, key, reg_vout_nameW,
                vout_name, &MMDevice_def_play->IMMDevice_iface);
        notify_if_changed(eCapture, eConsole, key, reg_in_nameW,
                in_name, &MMDevice_def_rec->IMMDevice_iface);
        notify_if_changed(eCapture, eCommunications, key, reg_vin_nameW,
                vin_name, &MMDevice_def_rec->IMMDevice_iface);

        LeaveCriticalSection(&g_notif_lock);
    }

    RegCloseKey(key);

    g_notif_thread = NULL;

    return 0;
}

static HRESULT WINAPI MMDevEnum_RegisterEndpointNotificationCallback(IMMDeviceEnumerator *iface, IMMNotificationClient *client)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    struct NotificationClientWrapper *wrapper;

    TRACE("(%p)->(%p)\n", This, client);

    if(!client)
        return E_POINTER;

    wrapper = HeapAlloc(GetProcessHeap(), 0, sizeof(*wrapper));
    if(!wrapper)
        return E_OUTOFMEMORY;

    wrapper->client = client;

    EnterCriticalSection(&g_notif_lock);

    list_add_tail(&g_notif_clients, &wrapper->entry);

    if(!g_notif_thread){
        g_notif_thread = CreateThread(NULL, 0, notif_thread_proc, NULL, 0, NULL);
        if(!g_notif_thread)
            ERR("CreateThread failed: %u\n", GetLastError());
    }

    LeaveCriticalSection(&g_notif_lock);

    return S_OK;
}

static HRESULT WINAPI MMDevEnum_UnregisterEndpointNotificationCallback(IMMDeviceEnumerator *iface, IMMNotificationClient *client)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    struct NotificationClientWrapper *wrapper, *wrapper2;

    TRACE("(%p)->(%p)\n", This, client);

    if(!client)
        return E_POINTER;

    EnterCriticalSection(&g_notif_lock);

    LIST_FOR_EACH_ENTRY_SAFE(wrapper, wrapper2, &g_notif_clients,
            struct NotificationClientWrapper, entry){
        if(wrapper->client == client){
            list_remove(&wrapper->entry);
            HeapFree(GetProcessHeap(), 0, wrapper);
            LeaveCriticalSection(&g_notif_lock);
            return S_OK;
        }
    }

    LeaveCriticalSection(&g_notif_lock);

    return E_NOTFOUND;
}

static const IMMDeviceEnumeratorVtbl MMDevEnumVtbl =
{
    MMDevEnum_QueryInterface,
    MMDevEnum_AddRef,
    MMDevEnum_Release,
    MMDevEnum_EnumAudioEndpoints,
    MMDevEnum_GetDefaultAudioEndpoint,
    MMDevEnum_GetDevice,
    MMDevEnum_RegisterEndpointNotificationCallback,
    MMDevEnum_UnregisterEndpointNotificationCallback
};

static HRESULT MMDevPropStore_Create(MMDevice *parent, DWORD access, IPropertyStore **ppv)
{
    MMDevPropStore *This;
    if (access != STGM_READ
        && access != STGM_WRITE
        && access != STGM_READWRITE)
    {
        WARN("Invalid access %08x\n", access);
        return E_INVALIDARG;
    }
    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    *ppv = &This->IPropertyStore_iface;
    if (!This)
        return E_OUTOFMEMORY;
    This->IPropertyStore_iface.lpVtbl = &MMDevPropVtbl;
    This->ref = 1;
    This->parent = parent;
    This->access = access;
    return S_OK;
}

static void MMDevPropStore_Destroy(MMDevPropStore *This)
{
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI MMDevPropStore_QueryInterface(IPropertyStore *iface, REFIID riid, void **ppv)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IPropertyStore))
        *ppv = &This->IPropertyStore_iface;
    else
        *ppv = NULL;
    if (!*ppv)
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MMDevPropStore_AddRef(IPropertyStore *iface)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI MMDevPropStore_Release(IPropertyStore *iface)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        MMDevPropStore_Destroy(This);
    return ref;
}

static HRESULT WINAPI MMDevPropStore_GetCount(IPropertyStore *iface, DWORD *nprops)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    WCHAR buffer[50];
    DWORD i = 0;
    HKEY propkey;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, nprops);
    if (!nprops)
        return E_POINTER;
    hr = MMDevPropStore_OpenPropKey(&This->parent->devguid, This->parent->flow, &propkey);
    if (FAILED(hr))
        return hr;
    *nprops = 0;
    do {
        DWORD len = sizeof(buffer)/sizeof(*buffer);
        if (RegEnumValueW(propkey, i, buffer, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;
        i++;
    } while (1);
    RegCloseKey(propkey);
    TRACE("Returning %i\n", i);
    *nprops = i;
    return S_OK;
}

static HRESULT WINAPI MMDevPropStore_GetAt(IPropertyStore *iface, DWORD prop, PROPERTYKEY *key)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    WCHAR buffer[50];
    DWORD len = sizeof(buffer)/sizeof(*buffer);
    HRESULT hr;
    HKEY propkey;

    TRACE("(%p)->(%u,%p)\n", iface, prop, key);
    if (!key)
        return E_POINTER;

    hr = MMDevPropStore_OpenPropKey(&This->parent->devguid, This->parent->flow, &propkey);
    if (FAILED(hr))
        return hr;

    if (RegEnumValueW(propkey, prop, buffer, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS
        || len <= 39)
    {
        WARN("GetAt %u failed\n", prop);
        return E_INVALIDARG;
    }
    RegCloseKey(propkey);
    buffer[38] = 0;
    CLSIDFromString(buffer, &key->fmtid);
    key->pid = atoiW(&buffer[39]);
    return S_OK;
}

static HRESULT WINAPI MMDevPropStore_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *pv)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    TRACE("(%p)->(\"%s,%u\", %p)\n", This, key ? debugstr_guid(&key->fmtid) : NULL, key ? key->pid : 0, pv);

    if (!key || !pv)
        return E_POINTER;
    if (This->access != STGM_READ
        && This->access != STGM_READWRITE)
        return STG_E_ACCESSDENIED;

    /* Special case */
    if (IsEqualPropertyKey(*key, PKEY_AudioEndpoint_GUID))
    {
        pv->vt = VT_LPWSTR;
        pv->u.pwszVal = CoTaskMemAlloc(39 * sizeof(WCHAR));
        if (!pv->u.pwszVal)
            return E_OUTOFMEMORY;
        StringFromGUID2(&This->parent->devguid, pv->u.pwszVal, 39);
        return S_OK;
    }

    return MMDevice_GetPropValue(&This->parent->devguid, This->parent->flow, key, pv);
}

static HRESULT WINAPI MMDevPropStore_SetValue(IPropertyStore *iface, REFPROPERTYKEY key, REFPROPVARIANT pv)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    TRACE("(%p)->(\"%s,%u\", %p)\n", This, key ? debugstr_guid(&key->fmtid) : NULL, key ? key->pid : 0, pv);

    if (!key || !pv)
        return E_POINTER;

    if (This->access != STGM_WRITE
        && This->access != STGM_READWRITE)
        return STG_E_ACCESSDENIED;
    return MMDevice_SetPropValue(&This->parent->devguid, This->parent->flow, key, pv);
}

static HRESULT WINAPI MMDevPropStore_Commit(IPropertyStore *iface)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    TRACE("(%p)\n", iface);

    if (This->access != STGM_WRITE
        && This->access != STGM_READWRITE)
        return STG_E_ACCESSDENIED;

    /* Does nothing - for mmdevapi, the propstore values are written on SetValue,
     * not on Commit. */

    return S_OK;
}

static const IPropertyStoreVtbl MMDevPropVtbl =
{
    MMDevPropStore_QueryInterface,
    MMDevPropStore_AddRef,
    MMDevPropStore_Release,
    MMDevPropStore_GetCount,
    MMDevPropStore_GetAt,
    MMDevPropStore_GetValue,
    MMDevPropStore_SetValue,
    MMDevPropStore_Commit
};


/* Property bag for IBaseFilter activation */
static HRESULT WINAPI PB_QueryInterface(IPropertyBag *iface, REFIID riid, void **ppv)
{
    ERR("Should not be called\n");
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI PB_AddRef(IPropertyBag *iface)
{
    ERR("Should not be called\n");
    return 2;
}

static ULONG WINAPI PB_Release(IPropertyBag *iface)
{
    ERR("Should not be called\n");
    return 1;
}

static HRESULT WINAPI PB_Read(IPropertyBag *iface, LPCOLESTR name, VARIANT *var, IErrorLog *log)
{
    static const WCHAR dsguid[] = { 'D','S','G','u','i','d', 0 };
    IPropertyBagImpl *This = impl_from_IPropertyBag(iface);
    TRACE("Trying to read %s, type %u\n", debugstr_w(name), var->n1.n2.vt);
    if (!lstrcmpW(name, dsguid))
    {
        WCHAR guidstr[39];
        StringFromGUID2(&This->devguid, guidstr,sizeof(guidstr)/sizeof(*guidstr));
        var->n1.n2.vt = VT_BSTR;
        var->n1.n2.n3.bstrVal = SysAllocString(guidstr);
        return S_OK;
    }
    ERR("Unknown property '%s' queried\n", debugstr_w(name));
    return E_FAIL;
}

static HRESULT WINAPI PB_Write(IPropertyBag *iface, LPCOLESTR name, VARIANT *var)
{
    ERR("Should not be called\n");
    return E_FAIL;
}

static const IPropertyBagVtbl PB_Vtbl =
{
    PB_QueryInterface,
    PB_AddRef,
    PB_Release,
    PB_Read,
    PB_Write
};

static ULONG WINAPI info_device_ps_AddRef(IPropertyStore *iface)
{
    return 2;
}

static ULONG WINAPI info_device_ps_Release(IPropertyStore *iface)
{
    return 1;
}

static HRESULT WINAPI info_device_ps_GetValue(IPropertyStore *iface,
        REFPROPERTYKEY key, PROPVARIANT *pv)
{
    TRACE("(static)->(\"%s,%u\", %p)\n", debugstr_guid(&key->fmtid), key ? key->pid : 0, pv);

    if (!key || !pv)
        return E_POINTER;

    if (IsEqualPropertyKey(*key, DEVPKEY_Device_Driver))
    {
        INT size = (lstrlenW(drvs.module_name) + 1) * sizeof(WCHAR);
        pv->vt = VT_LPWSTR;
        pv->u.pwszVal = CoTaskMemAlloc(size);
        if (!pv->u.pwszVal)
            return E_OUTOFMEMORY;
        memcpy(pv->u.pwszVal, drvs.module_name, size);
        return S_OK;
    }

    return E_INVALIDARG;
}

static const IPropertyStoreVtbl info_device_ps_Vtbl =
{
    NULL,
    info_device_ps_AddRef,
    info_device_ps_Release,
    NULL,
    NULL,
    info_device_ps_GetValue,
    NULL,
    NULL
};

static IPropertyStore info_device_ps = {
    &info_device_ps_Vtbl
};

static ULONG WINAPI info_device_AddRef(IMMDevice *iface)
{
    return 2;
}

static ULONG WINAPI info_device_Release(IMMDevice *iface)
{
    return 1;
}

static HRESULT WINAPI info_device_OpenPropertyStore(IMMDevice *iface,
        DWORD access, IPropertyStore **ppv)
{
    TRACE("(static)->(%x, %p)\n", access, ppv);
    *ppv = &info_device_ps;
    return S_OK;
}

static const IMMDeviceVtbl info_device_Vtbl =
{
    NULL,
    info_device_AddRef,
    info_device_Release,
    NULL,
    info_device_OpenPropertyStore,
    NULL,
    NULL
};

static IMMDevice info_device = {
    &info_device_Vtbl
};
