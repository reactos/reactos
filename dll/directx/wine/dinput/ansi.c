/*
 * Direct Input ANSI interface wrappers
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"

#include "device_private.h"
#include "dinput_private.h"

#include "wine/debug.h"

static struct dinput_device *impl_from_IDirectInputDevice8A( IDirectInputDevice8A *iface )
{
    return CONTAINING_RECORD( iface, struct dinput_device, IDirectInputDevice8A_iface );
}

static IDirectInputDevice8W *IDirectInputDevice8W_from_impl( struct dinput_device *impl )
{
    return &impl->IDirectInputDevice8W_iface;
}

static inline IDirectInputDevice8A *IDirectInputDevice8A_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    if (!iface) return NULL;
    return &CONTAINING_RECORD( iface, struct dinput_device, IDirectInputDevice8W_iface )->IDirectInputDevice8A_iface;
}

static inline IDirectInputDeviceA *IDirectInputDeviceA_from_IDirectInputDeviceW( IDirectInputDeviceW *iface )
{
    return (IDirectInputDeviceA *)IDirectInputDevice8A_from_IDirectInputDevice8W( (IDirectInputDevice8W *)iface );
}

static struct dinput *impl_from_IDirectInput7A( IDirectInput7A *iface )
{
    return CONTAINING_RECORD( iface, struct dinput, IDirectInput7A_iface );
}

static IDirectInput7W *IDirectInput7W_from_impl( struct dinput *impl )
{
    return &impl->IDirectInput7W_iface;
}

static struct dinput *impl_from_IDirectInput8A( IDirectInput8A *iface )
{
    return CONTAINING_RECORD( iface, struct dinput, IDirectInput8A_iface );
}

static IDirectInput8W *IDirectInput8W_from_impl( struct dinput *impl )
{
    return &impl->IDirectInput8W_iface;
}

static void dideviceobjectinstance_wtoa( const DIDEVICEOBJECTINSTANCEW *in, DIDEVICEOBJECTINSTANCEA *out )
{
    out->guidType = in->guidType;
    out->dwOfs = in->dwOfs;
    out->dwType = in->dwType;
    out->dwFlags = in->dwFlags;
    WideCharToMultiByte( CP_ACP, 0, in->tszName, -1, out->tszName, sizeof(out->tszName), NULL, NULL );

    if (out->dwSize <= FIELD_OFFSET( DIDEVICEOBJECTINSTANCEA, dwFFMaxForce )) return;

    out->dwFFMaxForce = in->dwFFMaxForce;
    out->dwFFForceResolution = in->dwFFForceResolution;
    out->wCollectionNumber = in->wCollectionNumber;
    out->wDesignatorIndex = in->wDesignatorIndex;
    out->wUsagePage = in->wUsagePage;
    out->wUsage = in->wUsage;
    out->dwDimension = in->dwDimension;
    out->wExponent = in->wExponent;
    out->wReportId = in->wReportId;
}

static void dideviceinstance_wtoa( const DIDEVICEINSTANCEW *in, DIDEVICEINSTANCEA *out )
{
    out->guidInstance = in->guidInstance;
    out->guidProduct = in->guidProduct;
    out->dwDevType = in->dwDevType;
    WideCharToMultiByte( CP_ACP, 0, in->tszInstanceName, -1, out->tszInstanceName,
                         sizeof(out->tszInstanceName), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, in->tszProductName, -1, out->tszProductName,
                         sizeof(out->tszProductName), NULL, NULL );

    if (out->dwSize <= FIELD_OFFSET( DIDEVICEINSTANCEA, guidFFDriver )) return;

    out->guidFFDriver = in->guidFFDriver;
    out->wUsagePage = in->wUsagePage;
    out->wUsage = in->wUsage;
}

static void dieffectinfo_wtoa( const DIEFFECTINFOW *in, DIEFFECTINFOA *out )
{
    out->guid = in->guid;
    out->dwEffType = in->dwEffType;
    out->dwStaticParams = in->dwStaticParams;
    out->dwDynamicParams = in->dwDynamicParams;
    WideCharToMultiByte( CP_ACP, 0, in->tszName, -1, out->tszName, sizeof(out->tszName), NULL, NULL );
}

static HRESULT string_atow( const char *in, WCHAR **out )
{
    int len;

    *out = NULL;
    if (!in) return DI_OK;

    len = MultiByteToWideChar( CP_ACP, 0, in, -1, NULL, 0 );
    if (!(*out = malloc( len * sizeof(WCHAR) ))) return DIERR_OUTOFMEMORY;

    MultiByteToWideChar( CP_ACP, 0, in, -1, *out, len );
    return DI_OK;
}

static void diactionformat_wtoa( const DIACTIONFORMATW *in, DIACTIONFORMATA *out )
{
    DWORD i;

    out->dwDataSize = in->dwDataSize;
    out->dwNumActions = in->dwNumActions;

    for (i = 0; i < in->dwNumActions; ++i)
    {
        out->rgoAction[i].uAppData = in->rgoAction[i].uAppData;
        out->rgoAction[i].dwSemantic = in->rgoAction[i].dwSemantic;
        out->rgoAction[i].dwFlags = in->rgoAction[i].dwFlags;
        out->rgoAction[i].guidInstance = in->rgoAction[i].guidInstance;
        out->rgoAction[i].dwObjID = in->rgoAction[i].dwObjID;
        out->rgoAction[i].dwHow = in->rgoAction[i].dwHow;
    }

    out->guidActionMap = in->guidActionMap;
    out->dwGenre = in->dwGenre;
    out->dwBufferSize = in->dwBufferSize;
    out->lAxisMin = in->lAxisMin;
    out->lAxisMax = in->lAxisMax;
    out->hInstString = in->hInstString;
    out->ftTimeStamp = in->ftTimeStamp;
    out->dwCRC = in->dwCRC;

    WideCharToMultiByte( CP_ACP, 0, in->tszActionMap, -1, out->tszActionMap,
                         sizeof(out->tszActionMap), NULL, NULL );
}

static HRESULT diactionformat_atow( const DIACTIONFORMATA *in, DIACTIONFORMATW *out, BOOL convert_names )
{
    HRESULT hr = DI_OK;
    DWORD i;

    out->dwDataSize = in->dwDataSize;
    out->dwNumActions = in->dwNumActions;

    for (i = 0; i < out->dwNumActions && SUCCEEDED(hr); ++i)
    {
        out->rgoAction[i].uAppData = in->rgoAction[i].uAppData;
        out->rgoAction[i].dwSemantic = in->rgoAction[i].dwSemantic;
        out->rgoAction[i].dwFlags = in->rgoAction[i].dwFlags;
        out->rgoAction[i].guidInstance = in->rgoAction[i].guidInstance;
        out->rgoAction[i].dwObjID = in->rgoAction[i].dwObjID;
        out->rgoAction[i].dwHow = in->rgoAction[i].dwHow;
        if (!convert_names) out->rgoAction[i].lptszActionName = 0;
        else if (in->hInstString) out->rgoAction[i].uResIdString = in->rgoAction[i].uResIdString;
        else hr = string_atow( in->rgoAction[i].lptszActionName, (WCHAR **)&out->rgoAction[i].lptszActionName );
    }

    for (; i < out->dwNumActions; ++i) out->rgoAction[i].lptszActionName = 0;

    out->guidActionMap = in->guidActionMap;
    out->dwGenre = in->dwGenre;
    out->dwBufferSize = in->dwBufferSize;
    out->lAxisMin = in->lAxisMin;
    out->lAxisMax = in->lAxisMax;
    out->hInstString = in->hInstString;
    out->ftTimeStamp = in->ftTimeStamp;
    out->dwCRC = in->dwCRC;

    MultiByteToWideChar( CP_ACP, 0, in->tszActionMap, -1, out->tszActionMap,
                         sizeof(out->tszActionMap) / sizeof(WCHAR) );

    return hr;
}

static void dideviceimageinfo_wtoa( const DIDEVICEIMAGEINFOW *in, DIDEVICEIMAGEINFOA *out )
{
    WideCharToMultiByte( CP_ACP, 0, in->tszImagePath, -1, out->tszImagePath,
                         sizeof(out->tszImagePath), NULL, NULL );

    out->dwFlags = in->dwFlags;
    out->dwViewID = in->dwViewID;
    out->rcOverlay = in->rcOverlay;
    out->dwObjID = in->dwObjID;
    out->dwcValidPts = in->dwcValidPts;
    out->rgptCalloutLine[0] = in->rgptCalloutLine[0];
    out->rgptCalloutLine[1] = in->rgptCalloutLine[1];
    out->rgptCalloutLine[2] = in->rgptCalloutLine[2];
    out->rgptCalloutLine[3] = in->rgptCalloutLine[3];
    out->rgptCalloutLine[4] = in->rgptCalloutLine[4];
    out->rcCalloutRect = in->rcCalloutRect;
    out->dwTextAlign = in->dwTextAlign;
}

static void dideviceimageinfoheader_wtoa( const DIDEVICEIMAGEINFOHEADERW *in, DIDEVICEIMAGEINFOHEADERA *out )
{
    DWORD i;

    out->dwcViews = in->dwcViews;
    out->dwcButtons = in->dwcButtons;
    out->dwcAxes = in->dwcAxes;
    out->dwcPOVs = in->dwcPOVs;
    out->dwBufferUsed = 0;

    for (i = 0; i < in->dwBufferUsed / sizeof(DIDEVICEIMAGEINFOW); ++i)
    {
        dideviceimageinfo_wtoa( &in->lprgImageInfoArray[i], &out->lprgImageInfoArray[i] );
        out->dwBufferUsed += sizeof(DIDEVICEIMAGEINFOA);
    }
}

static HRESULT diconfiguredevicesparams_atow( const DICONFIGUREDEVICESPARAMSA *in, DICONFIGUREDEVICESPARAMSW *out )
{
    const char *name_a = in->lptszUserNames;
    DWORD len_w, len_a;

    if (!in->lptszUserNames) out->lptszUserNames = NULL;
    else
    {
        while (name_a[0] && name_a[1]) ++name_a;
        len_a = name_a - in->lptszUserNames + 1;
        len_w = MultiByteToWideChar( CP_ACP, 0, in->lptszUserNames, len_a, NULL, 0 );

        out->lptszUserNames = calloc( len_w, sizeof(WCHAR) );
        if (!out->lptszUserNames) return DIERR_OUTOFMEMORY;

        MultiByteToWideChar( CP_ACP, 0, in->lptszUserNames, len_a, out->lptszUserNames, len_w );
    }

    out->dwcUsers = in->dwcUsers;
    out->dwcFormats = in->dwcFormats;
    out->hwnd = in->hwnd;
    out->dics = in->dics;
    out->lpUnkDDSTarget = in->lpUnkDDSTarget;

    return DI_OK;
}

static HRESULT WINAPI dinput_device_a_QueryInterface( IDirectInputDevice8A *iface_a, REFIID iid, void **out )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_QueryInterface( iface_w, iid, out );
}

static ULONG WINAPI dinput_device_a_AddRef( IDirectInputDevice8A *iface_a )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_AddRef( iface_w );
}

static ULONG WINAPI dinput_device_a_Release( IDirectInputDevice8A *iface_a )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Release( iface_w );
}

static HRESULT WINAPI dinput_device_a_GetCapabilities( IDirectInputDevice8A *iface_a, DIDEVCAPS *caps )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_GetCapabilities( iface_w, caps );
}

struct enum_objects_wtoa_params
{
    LPDIENUMDEVICEOBJECTSCALLBACKA callback;
    void *ref;
};

static BOOL CALLBACK enum_objects_wtoa_callback( const DIDEVICEOBJECTINSTANCEW *instance_w, void *ref )
{
    struct enum_objects_wtoa_params *params = ref;
    DIDEVICEOBJECTINSTANCEA instance_a = {sizeof(instance_a)};

    dideviceobjectinstance_wtoa( instance_w, &instance_a );
    return params->callback( &instance_a, params->ref );
}

static HRESULT WINAPI dinput_device_a_EnumObjects( IDirectInputDevice8A *iface_a, LPDIENUMDEVICEOBJECTSCALLBACKA callback,
                                                   void *ref, DWORD flags )
{
    struct enum_objects_wtoa_params params = {callback, ref};
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );

    if (!callback) return DIERR_INVALIDPARAM;

    return IDirectInputDevice8_EnumObjects( iface_w, enum_objects_wtoa_callback, &params, flags );
}

static HRESULT WINAPI dinput_device_a_GetProperty( IDirectInputDevice8A *iface_a, REFGUID guid, DIPROPHEADER *header )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_GetProperty( iface_w, guid, header );
}

static HRESULT WINAPI dinput_device_a_SetProperty( IDirectInputDevice8A *iface_a, REFGUID guid, const DIPROPHEADER *header )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SetProperty( iface_w, guid, header );
}

static HRESULT WINAPI dinput_device_a_Acquire( IDirectInputDevice8A *iface_a )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Acquire( iface_w );
}

static HRESULT WINAPI dinput_device_a_Unacquire( IDirectInputDevice8A *iface_a )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Unacquire( iface_w );
}

static HRESULT WINAPI dinput_device_a_GetDeviceState( IDirectInputDevice8A *iface_a, DWORD count, void *data )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_GetDeviceState( iface_w, count, data );
}

static HRESULT WINAPI dinput_device_a_GetDeviceData( IDirectInputDevice8A *iface_a, DWORD data_size, DIDEVICEOBJECTDATA *data,
                                                     DWORD *entries, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_GetDeviceData( iface_w, data_size, data, entries, flags );
}

static HRESULT WINAPI dinput_device_a_SetDataFormat( IDirectInputDevice8A *iface_a, const DIDATAFORMAT *format )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SetDataFormat( iface_w, format );
}

static HRESULT WINAPI dinput_device_a_SetEventNotification( IDirectInputDevice8A *iface_a, HANDLE event )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SetEventNotification( iface_w, event );
}

static HRESULT WINAPI dinput_device_a_SetCooperativeLevel( IDirectInputDevice8A *iface_a, HWND window, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SetCooperativeLevel( iface_w, window, flags );
}

static HRESULT WINAPI dinput_device_a_GetObjectInfo( IDirectInputDevice8A *iface_a, DIDEVICEOBJECTINSTANCEA *instance_a,
                                                     DWORD obj, DWORD how )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    DIDEVICEOBJECTINSTANCEW instance_w = {sizeof(instance_w)};
    HRESULT hr;

    if (!instance_a) return E_POINTER;
    if (instance_a->dwSize != sizeof(DIDEVICEOBJECTINSTANCEA) &&
        instance_a->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3A))
        return DIERR_INVALIDPARAM;

    hr = IDirectInputDevice8_GetObjectInfo( iface_w, &instance_w, obj, how );
    dideviceobjectinstance_wtoa( &instance_w, instance_a );

    return hr;
}

static HRESULT WINAPI dinput_device_a_GetDeviceInfo( IDirectInputDevice8A *iface_a, DIDEVICEINSTANCEA *instance_a )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    DIDEVICEINSTANCEW instance_w = {sizeof(instance_w)};
    HRESULT hr;

    if (!instance_a) return E_POINTER;
    if (instance_a->dwSize != sizeof(DIDEVICEINSTANCEA) && instance_a->dwSize != sizeof(DIDEVICEINSTANCE_DX3A))
        return DIERR_INVALIDPARAM;

    hr = IDirectInputDevice8_GetDeviceInfo( iface_w, &instance_w );
    dideviceinstance_wtoa( &instance_w, instance_a );

    return hr;
}

static HRESULT WINAPI dinput_device_a_RunControlPanel( IDirectInputDevice8A *iface_a, HWND owner, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_RunControlPanel( iface_w, owner, flags );
}

static HRESULT WINAPI dinput_device_a_Initialize( IDirectInputDevice8A *iface_a, HINSTANCE instance, DWORD version, REFGUID guid )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Initialize( iface_w, instance, version, guid );
}

static HRESULT WINAPI dinput_device_a_CreateEffect( IDirectInputDevice8A *iface_a, REFGUID guid, const DIEFFECT *effect,
                                                    IDirectInputEffect **out, IUnknown *outer )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_CreateEffect( iface_w, guid, effect, out, outer );
}

struct enum_effects_wtoa_params
{
    LPDIENUMEFFECTSCALLBACKA callback;
    void *ref;
};

static BOOL CALLBACK enum_effects_wtoa_callback( const DIEFFECTINFOW *info_w, void *ref )
{
    struct enum_effects_wtoa_params *params = ref;
    DIEFFECTINFOA info_a = {sizeof(info_a)};

    dieffectinfo_wtoa( info_w, &info_a );
    return params->callback( &info_a, params->ref );
}

static HRESULT WINAPI dinput_device_a_EnumEffects( IDirectInputDevice8A *iface_a, LPDIENUMEFFECTSCALLBACKA callback,
                                                   void *ref, DWORD type )
{
    struct enum_effects_wtoa_params params = {callback, ref};
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );

    if (!callback) return DIERR_INVALIDPARAM;

    return IDirectInputDevice8_EnumEffects( iface_w, enum_effects_wtoa_callback, &params, type );
}

static HRESULT WINAPI dinput_device_a_GetEffectInfo( IDirectInputDevice8A *iface_a, DIEFFECTINFOA *info_a, REFGUID guid )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    DIEFFECTINFOW info_w = {sizeof(info_w)};
    HRESULT hr;

    if (!info_a) return E_POINTER;
    if (info_a->dwSize != sizeof(DIEFFECTINFOA)) return DIERR_INVALIDPARAM;

    hr = IDirectInputDevice8_GetEffectInfo( iface_w, &info_w, guid );
    dieffectinfo_wtoa( &info_w, info_a );

    return hr;
}

static HRESULT WINAPI dinput_device_a_GetForceFeedbackState( IDirectInputDevice8A *iface_a, DWORD *state )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_GetForceFeedbackState( iface_w, state );
}

static HRESULT WINAPI dinput_device_a_SendForceFeedbackCommand( IDirectInputDevice8A *iface_a, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SendForceFeedbackCommand( iface_w, flags );
}

static HRESULT WINAPI dinput_device_a_EnumCreatedEffectObjects( IDirectInputDevice8A *iface_a, LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                                                void *ref, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_EnumCreatedEffectObjects( iface_w, callback, ref, flags );
}

static HRESULT WINAPI dinput_device_a_Escape( IDirectInputDevice8A *iface_a, DIEFFESCAPE *escape )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Escape( iface_w, escape );
}

static HRESULT WINAPI dinput_device_a_Poll( IDirectInputDevice8A *iface_a )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Poll( iface_w );
}

static HRESULT WINAPI dinput_device_a_SendDeviceData( IDirectInputDevice8A *iface_a, DWORD count, const DIDEVICEOBJECTDATA *data,
                                                      DWORD *inout, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SendDeviceData( iface_w, count, data, inout, flags );
}

static HRESULT WINAPI dinput_device_a_EnumEffectsInFile( IDirectInputDevice8A *iface_a, const char *filename_a, LPDIENUMEFFECTSINFILECALLBACK callback,
                                                         void *ref, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    WCHAR buffer[MAX_PATH], *filename_w = buffer;

    if (!filename_a) filename_w = NULL;
    else MultiByteToWideChar( CP_ACP, 0, filename_a, -1, buffer, MAX_PATH );

    return IDirectInputDevice8_EnumEffectsInFile( iface_w, filename_w, callback, ref, flags );
}

static HRESULT WINAPI dinput_device_a_WriteEffectToFile( IDirectInputDevice8A *iface_a, const char *filename_a, DWORD entries,
                                                         DIFILEEFFECT *file_effect, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    WCHAR buffer[MAX_PATH], *filename_w = buffer;

    if (!filename_a) filename_w = NULL;
    else MultiByteToWideChar( CP_ACP, 0, filename_a, -1, buffer, MAX_PATH );

    return IDirectInputDevice8_WriteEffectToFile( iface_w, filename_w, entries, file_effect, flags );
}

static HRESULT WINAPI dinput_device_a_BuildActionMap( IDirectInputDevice8A *iface_a, DIACTIONFORMATA *format_a,
                                                      const char *username_a, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    DIACTIONFORMATW format_w = {sizeof(format_w), sizeof(DIACTIONW)};
    HRESULT hr;
    WCHAR *username_w;

    if (!format_a) return E_POINTER;
    if (format_a->dwSize != sizeof(DIACTIONFORMATA)) return DIERR_INVALIDPARAM;
    if (format_a->dwActionSize != sizeof(DIACTIONA)) return DIERR_INVALIDPARAM;
    if (FAILED(hr = string_atow( username_a, &username_w ))) return hr;

    format_w.dwNumActions = format_a->dwNumActions;
    format_w.rgoAction = calloc( format_a->dwNumActions, sizeof(DIACTIONW) );
    if (!format_w.rgoAction) hr = DIERR_OUTOFMEMORY;
    else
    {
        diactionformat_atow( format_a, &format_w, FALSE );
        hr = IDirectInputDevice8_BuildActionMap( iface_w, &format_w, username_w, flags );
        diactionformat_wtoa( &format_w, format_a );
        free( format_w.rgoAction );
    }

    free( username_w );
    return hr;
}

static HRESULT WINAPI dinput_device_a_SetActionMap( IDirectInputDevice8A *iface_a, DIACTIONFORMATA *format_a,
                                                    const char *username_a, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    DIACTIONFORMATW format_w = {sizeof(format_w), sizeof(DIACTIONW)};
    HRESULT hr;
    WCHAR *username_w;

    if (!format_a) return E_POINTER;
    if (format_a->dwSize != sizeof(DIACTIONFORMATA)) return DIERR_INVALIDPARAM;
    if (format_a->dwActionSize != sizeof(DIACTIONA)) return DIERR_INVALIDPARAM;
    if (FAILED(hr = string_atow( username_a, &username_w ))) return hr;

    format_w.dwNumActions = format_a->dwNumActions;
    format_w.rgoAction = calloc( format_a->dwNumActions, sizeof(DIACTIONW) );
    if (!format_w.rgoAction) hr = DIERR_OUTOFMEMORY;
    else
    {
        diactionformat_atow( format_a, &format_w, FALSE );
        hr = IDirectInputDevice8_SetActionMap( iface_w, &format_w, username_w, flags );
        diactionformat_wtoa( &format_w, format_a );
        free( format_w.rgoAction );
    }

    free( username_w );
    return hr;
}

static HRESULT WINAPI dinput_device_a_GetImageInfo( IDirectInputDevice8A *iface_a, DIDEVICEIMAGEINFOHEADERA *header_a )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    DIDEVICEIMAGEINFOHEADERW header_w = {sizeof(header_w), sizeof(DIDEVICEIMAGEINFOW)};
    HRESULT hr;

    if (!header_a) return E_POINTER;
    if (header_a->dwSize != sizeof(DIDEVICEIMAGEINFOHEADERA)) return DIERR_INVALIDPARAM;
    if (header_a->dwSizeImageInfo != sizeof(DIDEVICEIMAGEINFOA)) return DIERR_INVALIDPARAM;

    header_w.dwBufferSize = (header_a->dwBufferSize / sizeof(DIDEVICEIMAGEINFOA)) * sizeof(DIDEVICEIMAGEINFOW);
    header_w.lprgImageInfoArray = calloc( header_w.dwBufferSize, 1 );
    if (!header_w.lprgImageInfoArray) return DIERR_OUTOFMEMORY;

    hr = IDirectInputDevice8_GetImageInfo( iface_w, &header_w );
    dideviceimageinfoheader_wtoa( &header_w, header_a );
    free( header_w.lprgImageInfoArray );
    return hr;
}

const IDirectInputDevice8AVtbl dinput_device_a_vtbl =
{
    /*** IUnknown methods ***/
    dinput_device_a_QueryInterface,
    dinput_device_a_AddRef,
    dinput_device_a_Release,
    /*** IDirectInputDeviceA methods ***/
    dinput_device_a_GetCapabilities,
    dinput_device_a_EnumObjects,
    dinput_device_a_GetProperty,
    dinput_device_a_SetProperty,
    dinput_device_a_Acquire,
    dinput_device_a_Unacquire,
    dinput_device_a_GetDeviceState,
    dinput_device_a_GetDeviceData,
    dinput_device_a_SetDataFormat,
    dinput_device_a_SetEventNotification,
    dinput_device_a_SetCooperativeLevel,
    dinput_device_a_GetObjectInfo,
    dinput_device_a_GetDeviceInfo,
    dinput_device_a_RunControlPanel,
    dinput_device_a_Initialize,
    /*** IDirectInputDevice2A methods ***/
    dinput_device_a_CreateEffect,
    dinput_device_a_EnumEffects,
    dinput_device_a_GetEffectInfo,
    dinput_device_a_GetForceFeedbackState,
    dinput_device_a_SendForceFeedbackCommand,
    dinput_device_a_EnumCreatedEffectObjects,
    dinput_device_a_Escape,
    dinput_device_a_Poll,
    dinput_device_a_SendDeviceData,
    /*** IDirectInputDevice7A methods ***/
    dinput_device_a_EnumEffectsInFile,
    dinput_device_a_WriteEffectToFile,
    /*** IDirectInputDevice8A methods ***/
    dinput_device_a_BuildActionMap,
    dinput_device_a_SetActionMap,
    dinput_device_a_GetImageInfo,
};

static HRESULT WINAPI dinput8_a_QueryInterface( IDirectInput8A *iface_a, REFIID iid, void **out )
{
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    return IDirectInput8_QueryInterface( iface_w, iid, out );
}

static ULONG WINAPI dinput8_a_AddRef( IDirectInput8A *iface_a )
{
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    return IDirectInput8_AddRef( iface_w );
}

static ULONG WINAPI dinput8_a_Release( IDirectInput8A *iface_a )
{
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    return IDirectInput8_Release( iface_w );
}

static HRESULT WINAPI dinput8_a_CreateDevice( IDirectInput8A *iface_a, REFGUID guid, IDirectInputDevice8A **out, IUnknown *outer )
{
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    IDirectInputDevice8W *outw;
    HRESULT hr;

    if (!out) return E_POINTER;

    hr = IDirectInput8_CreateDevice( iface_w, guid, &outw, outer );
    *out = IDirectInputDevice8A_from_IDirectInputDevice8W( outw );
    return hr;
}

struct enum_devices_wtoa_params
{
    LPDIENUMDEVICESCALLBACKA callback;
    void *ref;
};

static BOOL CALLBACK enum_devices_wtoa_callback( const DIDEVICEINSTANCEW *instance_w, void *data )
{
    struct enum_devices_wtoa_params *params = data;
    DIDEVICEINSTANCEA instance_a = {sizeof(instance_a)};

    dideviceinstance_wtoa( instance_w, &instance_a );
    return params->callback( &instance_a, params->ref );
}

static HRESULT WINAPI dinput8_a_EnumDevices( IDirectInput8A *iface_a, DWORD type, LPDIENUMDEVICESCALLBACKA callback,
                                            void *ref, DWORD flags )
{
    struct enum_devices_wtoa_params params = {callback, ref};
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );

    if (!callback) return DIERR_INVALIDPARAM;

    return IDirectInput8_EnumDevices( iface_w, type, enum_devices_wtoa_callback, &params, flags );
}

static HRESULT WINAPI dinput8_a_GetDeviceStatus( IDirectInput8A *iface_a, REFGUID instance_guid )
{
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    return IDirectInput8_GetDeviceStatus( iface_w, instance_guid );
}

static HRESULT WINAPI dinput8_a_RunControlPanel( IDirectInput8A *iface_a, HWND owner, DWORD flags )
{
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    return IDirectInput8_RunControlPanel( iface_w, owner, flags );
}

static HRESULT WINAPI dinput8_a_Initialize( IDirectInput8A *iface_a, HINSTANCE instance, DWORD version )
{
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    return IDirectInput8_Initialize( iface_w, instance, version );
}

static HRESULT WINAPI dinput8_a_FindDevice( IDirectInput8A *iface_a, REFGUID guid, const char *name_a, GUID *instance_guid )
{
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    HRESULT hr;
    WCHAR *name_w;

    if (FAILED(hr = string_atow( name_a, &name_w ))) return hr;

    hr = IDirectInput8_FindDevice( iface_w, guid, name_w, instance_guid );
    free( name_w );
    return hr;
}

struct enum_devices_by_semantics_wtoa_params
{
    LPDIENUMDEVICESBYSEMANTICSCBA callback;
    void *ref;
};

static BOOL CALLBACK enum_devices_by_semantics_wtoa_callback( const DIDEVICEINSTANCEW *instance_w, IDirectInputDevice8W *iface_w,
                                                              DWORD flags, DWORD remaining, void *data )
{
    struct enum_devices_by_semantics_wtoa_params *params = data;
    IDirectInputDevice8A *iface_a = IDirectInputDevice8A_from_IDirectInputDevice8W( iface_w );
    DIDEVICEINSTANCEA instance_a = {sizeof(instance_a)};

    dideviceinstance_wtoa( instance_w, &instance_a );
    return params->callback( &instance_a, iface_a, flags, remaining, params->ref );
}

static HRESULT WINAPI dinput8_a_EnumDevicesBySemantics( IDirectInput8A *iface_a, const char *username_a, DIACTIONFORMATA *format_a,
                                                       LPDIENUMDEVICESBYSEMANTICSCBA callback, void *ref, DWORD flags )
{
    struct enum_devices_by_semantics_wtoa_params params = {callback, ref};
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    DIACTIONFORMATW format_w = {sizeof(format_w), sizeof(DIACTIONW)};
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    HRESULT hr;
    WCHAR *username_w;

    if (!callback) return DIERR_INVALIDPARAM;
    if (FAILED(hr = string_atow( username_a, &username_w ))) return hr;

    format_w.dwNumActions = format_a->dwNumActions;
    format_w.rgoAction = calloc( format_a->dwNumActions, sizeof(DIACTIONW) );
    if (!format_w.rgoAction) hr = DIERR_OUTOFMEMORY;
    else
    {
        diactionformat_atow( format_a, &format_w, FALSE );
        hr = IDirectInput8_EnumDevicesBySemantics( iface_w, username_w, &format_w, enum_devices_by_semantics_wtoa_callback,
                                                   &params, flags );
        free( format_w.rgoAction );
    }

    free( username_w );
    return hr;
}

static HRESULT WINAPI dinput8_a_ConfigureDevices( IDirectInput8A *iface_a, LPDICONFIGUREDEVICESCALLBACK callback,
                                                 DICONFIGUREDEVICESPARAMSA *params_a, DWORD flags, void *ref )
{
    struct dinput *impl = impl_from_IDirectInput8A( iface_a );
    IDirectInput8W *iface_w = IDirectInput8W_from_impl( impl );
    DICONFIGUREDEVICESPARAMSW params_w = {sizeof(params_w)};
    DIACTIONFORMATA *format_a = params_a->lprgFormats;
    DIACTIONFORMATW format_w = {sizeof(format_w), sizeof(DIACTIONW)};
    HRESULT hr;
    DWORD i;

    if (FAILED(hr = diconfiguredevicesparams_atow( params_a, &params_w ))) return hr;

    format_w.dwNumActions = format_a->dwNumActions;
    format_w.rgoAction = calloc( format_a->dwNumActions, sizeof(DIACTIONW) );
    if (!format_w.rgoAction) hr = DIERR_OUTOFMEMORY;
    else
    {
        hr = diactionformat_atow( format_a, &format_w, TRUE );
        params_w.lprgFormats = &format_w;

        if (SUCCEEDED(hr)) hr = IDirectInput8_ConfigureDevices( iface_w, callback, &params_w, flags, ref );
        if (SUCCEEDED(hr)) diactionformat_wtoa( &format_w, format_a );

        if (!format_w.hInstString)
        {
            for (i = 0; i < format_w.dwNumActions; ++i)
                free( (void *)format_w.rgoAction[i].lptszActionName );
        }
        free( format_w.rgoAction );
    }

    free( params_w.lptszUserNames );
    return hr;
}

const IDirectInput8AVtbl dinput8_a_vtbl =
{
    /*** IUnknown methods ***/
    dinput8_a_QueryInterface,
    dinput8_a_AddRef,
    dinput8_a_Release,
    /*** IDirectInput8A methods ***/
    dinput8_a_CreateDevice,
    dinput8_a_EnumDevices,
    dinput8_a_GetDeviceStatus,
    dinput8_a_RunControlPanel,
    dinput8_a_Initialize,
    dinput8_a_FindDevice,
    dinput8_a_EnumDevicesBySemantics,
    dinput8_a_ConfigureDevices,
};

static HRESULT WINAPI dinput7_a_QueryInterface( IDirectInput7A *iface_a, REFIID iid, void **out )
{
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );
    return IDirectInput7_QueryInterface( iface_w, iid, out );
}

static ULONG WINAPI dinput7_a_AddRef( IDirectInput7A *iface_a )
{
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );
    return IDirectInput7_AddRef( iface_w );
}

static ULONG WINAPI dinput7_a_Release( IDirectInput7A *iface_a )
{
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );
    return IDirectInput7_Release( iface_w );
}

static HRESULT WINAPI dinput7_a_CreateDevice( IDirectInput7A *iface_a, REFGUID guid, IDirectInputDeviceA **out_a, IUnknown *outer )
{
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );
    IDirectInputDeviceW *out_w;
    HRESULT hr;

    if (!out_a) return E_POINTER;

    hr = IDirectInput7_CreateDevice( iface_w, guid, &out_w, outer );
    *out_a = IDirectInputDeviceA_from_IDirectInputDeviceW( out_w );
    return hr;
}

static HRESULT WINAPI dinput7_a_EnumDevices( IDirectInput7A *iface_a, DWORD type, LPDIENUMDEVICESCALLBACKA callback,
                                             void *ref, DWORD flags )
{
    struct enum_devices_wtoa_params params = {callback, ref};
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );

    if (!callback) return DIERR_INVALIDPARAM;

    return IDirectInput7_EnumDevices( iface_w, type, enum_devices_wtoa_callback, &params, flags );
}

static HRESULT WINAPI dinput7_a_GetDeviceStatus( IDirectInput7A *iface_a, REFGUID instance_guid )
{
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );
    return IDirectInput7_GetDeviceStatus( iface_w, instance_guid );
}

static HRESULT WINAPI dinput7_a_RunControlPanel( IDirectInput7A *iface_a, HWND owner, DWORD flags )
{
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );
    return IDirectInput7_RunControlPanel( iface_w, owner, flags );
}

static HRESULT WINAPI dinput7_a_Initialize( IDirectInput7A *iface_a, HINSTANCE instance, DWORD version )
{
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );
    return IDirectInput7_Initialize( iface_w, instance, version );
}

static HRESULT WINAPI dinput7_a_FindDevice( IDirectInput7A *iface_a, REFGUID guid, const char *name_a, GUID *instance_guid )
{
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );
    HRESULT hr;
    WCHAR *name_w;

    if (FAILED(hr = string_atow( name_a, &name_w ))) return hr;

    hr = IDirectInput7_FindDevice( iface_w, guid, name_w, instance_guid );
    free( name_w );
    return hr;
}

static HRESULT WINAPI dinput7_a_CreateDeviceEx( IDirectInput7A *iface_a, REFGUID guid, REFIID iid, void **out, IUnknown *outer )
{
    struct dinput *impl = impl_from_IDirectInput7A( iface_a );
    IDirectInput7W *iface_w = IDirectInput7W_from_impl( impl );
    return IDirectInput7_CreateDeviceEx( iface_w, guid, iid, out, outer );
}

const IDirectInput7AVtbl dinput7_a_vtbl =
{
    /*** IUnknown methods ***/
    dinput7_a_QueryInterface,
    dinput7_a_AddRef,
    dinput7_a_Release,
    /*** IDirectInputA methods ***/
    dinput7_a_CreateDevice,
    dinput7_a_EnumDevices,
    dinput7_a_GetDeviceStatus,
    dinput7_a_RunControlPanel,
    dinput7_a_Initialize,
    /*** IDirectInput2A methods ***/
    dinput7_a_FindDevice,
    /*** IDirectInput7A methods ***/
    dinput7_a_CreateDeviceEx,
};
