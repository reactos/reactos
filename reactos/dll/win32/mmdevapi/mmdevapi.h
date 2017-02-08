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

#ifndef _MMDEVAPI_H_
#define _MMDEVAPI_H_

#include <wine/config.h>
#include <wine/port.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <objbase.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>

#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

extern HRESULT MMDevEnum_Create(REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern void MMDevEnum_Free(void) DECLSPEC_HIDDEN;


/* Changes to this enum must be synced in drivers. */
enum _DriverPriority {
    Priority_Unavailable = 0, /* driver won't work */
    Priority_Low, /* driver may work, but unlikely */
    Priority_Neutral, /* driver makes no judgment */
    Priority_Preferred /* driver thinks it's correct */
};

typedef struct _DriverFuncs {
    HMODULE module;
    WCHAR module_name[64];
    int priority;

    /* Returns a "priority" value for the driver. Highest priority wins.
     * If multiple drivers think they are valid, they will return a
     * priority value reflecting the likelihood that they are actually
     * valid. See enum _DriverPriority. */
    int (WINAPI *pGetPriority)(void);

    /* ids gets an array of human-friendly endpoint names
     * keys gets an array of driver-specific stuff that is used
     *   in GetAudioEndpoint to identify the endpoint
     * it is the caller's responsibility to free both arrays, and
     *   all of the elements in both arrays with HeapFree() */
    HRESULT (WINAPI *pGetEndpointIDs)(EDataFlow flow, WCHAR ***ids,
            GUID **guids, UINT *num, UINT *default_index);
    HRESULT (WINAPI *pGetAudioEndpoint)(void *key, IMMDevice *dev,
            IAudioClient **out);
    HRESULT (WINAPI *pGetAudioSessionManager)(IMMDevice *device,
            IAudioSessionManager2 **out);
    HRESULT (WINAPI *pGetPropValue)(GUID *guid,
            const PROPERTYKEY *prop, PROPVARIANT *out);
} DriverFuncs;

extern DriverFuncs drvs DECLSPEC_HIDDEN;

typedef struct MMDevice {
    IMMDevice IMMDevice_iface;
    IMMEndpoint IMMEndpoint_iface;
    LONG ref;

    CRITICAL_SECTION crst;

    EDataFlow flow;
    DWORD state;
    GUID devguid;
    WCHAR *drv_id;
} MMDevice;

extern HRESULT AudioClient_Create(MMDevice *parent, IAudioClient **ppv) DECLSPEC_HIDDEN;
extern HRESULT AudioEndpointVolume_Create(MMDevice *parent, IAudioEndpointVolume **ppv) DECLSPEC_HIDDEN;

extern const WCHAR drv_keyW[] DECLSPEC_HIDDEN;

#endif /* _MMDEVAPI_H_ */
