/*  DirectInput Generic Joystick device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2009 Aric Stewart, CodeWeavers
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

/*
 * To Do:
 *	dead zone
 *	force feedback
 */

#include "joystick_private.h"
#include <wine/debug.h>
#include <winreg.h>

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static inline JoystickGenericImpl *impl_from_IDirectInputDevice8A(IDirectInputDevice8A *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8A_iface), JoystickGenericImpl, base);
}
static inline JoystickGenericImpl *impl_from_IDirectInputDevice8W(IDirectInputDevice8W *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface), JoystickGenericImpl, base);
}
static inline IDirectInputDevice8A *IDirectInputDevice8A_from_impl(JoystickGenericImpl *This)
{
    return &This->base.IDirectInputDevice8A_iface;
}
static inline IDirectInputDevice8W *IDirectInputDevice8W_from_impl(JoystickGenericImpl *This)
{
    return &This->base.IDirectInputDevice8W_iface;
}

BOOL device_disabled_registry(const char* name)
{
    static const char disabled_str[] = "disabled";
    static const char joystick_key[] = "Joysticks";
    char buffer[MAX_PATH];
    HKEY hkey, appkey, temp;
    BOOL do_disable = FALSE;

    get_app_key(&hkey, &appkey);

    /* Joystick settings are in the 'joysticks' subkey */
    if (appkey)
    {
        if (RegOpenKeyA(appkey, joystick_key, &temp)) temp = 0;
        RegCloseKey(appkey);
        appkey = temp;
    }
    if (hkey)
    {
        if (RegOpenKeyA(hkey, joystick_key, &temp)) temp = 0;
        RegCloseKey(hkey);
        hkey = temp;
    }

    /* Look for the "controllername"="disabled" key */
    if (!get_config_key(hkey, appkey, name, buffer, sizeof(buffer)))
        if (!strcmp(disabled_str, buffer))
        {
            TRACE("Disabling joystick '%s' based on registry key.\n", name);
            do_disable = TRUE;
        }

    if (appkey) RegCloseKey(appkey);
    if (hkey)   RegCloseKey(hkey);

    return do_disable;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
HRESULT WINAPI JoystickWGenericImpl_SetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPCDIPROPHEADER ph)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8W(iface);
    DWORD i;

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(rguid),ph);

    if (ph == NULL) {
        WARN("invalid parameter: ph == NULL\n");
        return DIERR_INVALIDPARAM;
    }

    if (TRACE_ON(dinput))
        _dump_DIPROPHEADER(ph);

    if (IS_DIPROP(rguid)) {
        switch (LOWORD(rguid)) {
        case (DWORD_PTR)DIPROP_RANGE: {
            LPCDIPROPRANGE pr = (LPCDIPROPRANGE)ph;
            if (ph->dwHow == DIPH_DEVICE) {
                TRACE("proprange(%d,%d) all\n", pr->lMin, pr->lMax);
                for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++) {
                    This->props[i].lMin = pr->lMin;
                    This->props[i].lMax = pr->lMax;
                }
            } else {
                int obj = find_property(&This->base.data_format, ph);

                TRACE("proprange(%d,%d) obj=%d\n", pr->lMin, pr->lMax, obj);
                if (obj >= 0) {
                    This->props[obj].lMin = pr->lMin;
                    This->props[obj].lMax = pr->lMax;
                    return DI_OK;
                }
            }
            break;
        }
        case (DWORD_PTR)DIPROP_DEADZONE: {
            LPCDIPROPDWORD pd = (LPCDIPROPDWORD)ph;
            if (ph->dwHow == DIPH_DEVICE) {
                TRACE("deadzone(%d) all\n", pd->dwData);
                for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++)
                    This->props[i].lDeadZone  = pd->dwData;
            } else {
                int obj = find_property(&This->base.data_format, ph);

                TRACE("deadzone(%d) obj=%d\n", pd->dwData, obj);
                if (obj >= 0) {
                    This->props[obj].lDeadZone  = pd->dwData;
                    return DI_OK;
                }
            }
            break;
        }
        case (DWORD_PTR)DIPROP_SATURATION: {
            LPCDIPROPDWORD pd = (LPCDIPROPDWORD)ph;
            if (ph->dwHow == DIPH_DEVICE) {
                TRACE("saturation(%d) all\n", pd->dwData);
                for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++)
                    This->props[i].lSaturation = pd->dwData;
            } else {
                int obj = find_property(&This->base.data_format, ph);

                TRACE("saturation(%d) obj=%d\n", pd->dwData, obj);
                if (obj >= 0) {
                    This->props[obj].lSaturation = pd->dwData;
                    return DI_OK;
                }
            }
            break;
        }
        default:
            return IDirectInputDevice2WImpl_SetProperty(iface, rguid, ph);
        }
    }

    return DI_OK;
}

HRESULT WINAPI JoystickAGenericImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPCDIPROPHEADER ph)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWGenericImpl_SetProperty(IDirectInputDevice8W_from_impl(This), rguid, ph);
}

void _dump_DIDEVCAPS(const DIDEVCAPS *lpDIDevCaps)
{
    TRACE("dwSize: %d\n", lpDIDevCaps->dwSize);
    TRACE("dwFlags: %08x\n", lpDIDevCaps->dwFlags);
    TRACE("dwDevType: %08x %s\n", lpDIDevCaps->dwDevType,
          lpDIDevCaps->dwDevType == DIDEVTYPE_DEVICE ? "DIDEVTYPE_DEVICE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_DEVICE ? "DIDEVTYPE_DEVICE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_MOUSE ? "DIDEVTYPE_MOUSE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_KEYBOARD ? "DIDEVTYPE_KEYBOARD" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_JOYSTICK ? "DIDEVTYPE_JOYSTICK" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_HID ? "DIDEVTYPE_HID" : "UNKNOWN");
    TRACE("dwAxes: %d\n", lpDIDevCaps->dwAxes);
    TRACE("dwButtons: %d\n", lpDIDevCaps->dwButtons);
    TRACE("dwPOVs: %d\n", lpDIDevCaps->dwPOVs);
    if (lpDIDevCaps->dwSize > sizeof(DIDEVCAPS_DX3)) {
        TRACE("dwFFSamplePeriod: %d\n", lpDIDevCaps->dwFFSamplePeriod);
        TRACE("dwFFMinTimeResolution: %d\n", lpDIDevCaps->dwFFMinTimeResolution);
        TRACE("dwFirmwareRevision: %d\n", lpDIDevCaps->dwFirmwareRevision);
        TRACE("dwHardwareRevision: %d\n", lpDIDevCaps->dwHardwareRevision);
        TRACE("dwFFDriverVersion: %d\n", lpDIDevCaps->dwFFDriverVersion);
    }
}

HRESULT WINAPI JoystickWGenericImpl_GetCapabilities(LPDIRECTINPUTDEVICE8W iface, LPDIDEVCAPS lpDIDevCaps)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8W(iface);
    int size;

    TRACE("%p->(%p)\n",iface,lpDIDevCaps);

    if (lpDIDevCaps == NULL) {
        WARN("invalid pointer\n");
        return E_POINTER;
    }

    size = lpDIDevCaps->dwSize;

    if (!(size == sizeof(DIDEVCAPS) || size == sizeof(DIDEVCAPS_DX3))) {
        WARN("invalid parameter\n");
        return DIERR_INVALIDPARAM;
    }

    CopyMemory(lpDIDevCaps, &This->devcaps, size);
    lpDIDevCaps->dwSize = size;

    if (TRACE_ON(dinput))
        _dump_DIDEVCAPS(lpDIDevCaps);

    return DI_OK;
}

HRESULT WINAPI JoystickAGenericImpl_GetCapabilities(LPDIRECTINPUTDEVICE8A iface, LPDIDEVCAPS lpDIDevCaps)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWGenericImpl_GetCapabilities(IDirectInputDevice8W_from_impl(This), lpDIDevCaps);
}

/******************************************************************************
  *     GetObjectInfo : get object info
  */
HRESULT WINAPI JoystickWGenericImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface,
        LPDIDEVICEOBJECTINSTANCEW pdidoi, DWORD dwObj, DWORD dwHow)
{
    static const WCHAR axisW[] = {'A','x','i','s',' ','%','d',0};
    static const WCHAR povW[] = {'P','O','V',' ','%','d',0};
    static const WCHAR buttonW[] = {'B','u','t','t','o','n',' ','%','d',0};
    HRESULT res;

    res = IDirectInputDevice2WImpl_GetObjectInfo(iface, pdidoi, dwObj, dwHow);
    if (res != DI_OK) return res;

    if      (pdidoi->dwType & DIDFT_AXIS)
        sprintfW(pdidoi->tszName, axisW, DIDFT_GETINSTANCE(pdidoi->dwType));
    else if (pdidoi->dwType & DIDFT_POV)
        sprintfW(pdidoi->tszName, povW, DIDFT_GETINSTANCE(pdidoi->dwType));
    else if (pdidoi->dwType & DIDFT_BUTTON)
        sprintfW(pdidoi->tszName, buttonW, DIDFT_GETINSTANCE(pdidoi->dwType));

    _dump_OBJECTINSTANCEW(pdidoi);
    return res;
}

HRESULT WINAPI JoystickAGenericImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8A iface,
        LPDIDEVICEOBJECTINSTANCEA pdidoi, DWORD dwObj, DWORD dwHow)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8A(iface);
    HRESULT res;
    DIDEVICEOBJECTINSTANCEW didoiW;
    DWORD dwSize = pdidoi->dwSize;

    didoiW.dwSize = sizeof(didoiW);
    res = JoystickWGenericImpl_GetObjectInfo(IDirectInputDevice8W_from_impl(This), &didoiW, dwObj, dwHow);
    if (res != DI_OK) return res;

    memset(pdidoi, 0, pdidoi->dwSize);
    memcpy(pdidoi, &didoiW, FIELD_OFFSET(DIDEVICEOBJECTINSTANCEW, tszName));
    pdidoi->dwSize = dwSize;
    WideCharToMultiByte(CP_ACP, 0, didoiW.tszName, -1, pdidoi->tszName,
                        sizeof(pdidoi->tszName), NULL, NULL);

    return res;
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
HRESULT WINAPI JoystickWGenericImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(rguid), pdiph);

    if (TRACE_ON(dinput))
        _dump_DIPROPHEADER(pdiph);

    if (IS_DIPROP(rguid)) {
        switch (LOWORD(rguid)) {
        case (DWORD_PTR) DIPROP_RANGE: {
            LPDIPROPRANGE pr = (LPDIPROPRANGE)pdiph;
            int obj = find_property(&This->base.data_format, pdiph);

            /* The app is querying the current range of the axis
             * return the lMin and lMax values */
            if (obj >= 0) {
                pr->lMin = This->props[obj].lMin;
                pr->lMax = This->props[obj].lMax;
                TRACE("range(%d, %d) obj=%d\n", pr->lMin, pr->lMax, obj);
                return DI_OK;
            }
            break;
        }
        case (DWORD_PTR) DIPROP_DEADZONE: {
            LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;
            int obj = find_property(&This->base.data_format, pdiph);

            if (obj >= 0) {
                pd->dwData = This->props[obj].lDeadZone;
                TRACE("deadzone(%d) obj=%d\n", pd->dwData, obj);
                return DI_OK;
            }
            break;
        }
        case (DWORD_PTR) DIPROP_SATURATION: {
            LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;
            int obj = find_property(&This->base.data_format, pdiph);

            if (obj >= 0) {
                pd->dwData = This->props[obj].lSaturation;
                TRACE("saturation(%d) obj=%d\n", pd->dwData, obj);
                return DI_OK;
            }
            break;
        }
        case (DWORD_PTR) DIPROP_INSTANCENAME: {
            DIPROPSTRING *ps = (DIPROPSTRING*) pdiph;
            DIDEVICEINSTANCEW didev;

            didev.dwSize = sizeof(didev);

            IDirectInputDevice_GetDeviceInfo(iface, &didev);
            lstrcpynW(ps->wsz, didev.tszInstanceName, MAX_PATH);

            return DI_OK;
        }
        default:
            return IDirectInputDevice2WImpl_GetProperty(iface, rguid, pdiph);
        }
    }

    return DI_OK;
}

HRESULT WINAPI JoystickAGenericImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWGenericImpl_GetProperty(IDirectInputDevice8W_from_impl(This), rguid, pdiph);
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
HRESULT WINAPI JoystickAGenericImpl_GetDeviceInfo(
    LPDIRECTINPUTDEVICE8A iface,
    LPDIDEVICEINSTANCEA pdidi)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8A(iface);

    TRACE("(%p,%p)\n", iface, pdidi);

    if (pdidi == NULL) {
        WARN("invalid pointer\n");
        return E_POINTER;
    }

    if ((pdidi->dwSize != sizeof(DIDEVICEINSTANCE_DX3A)) &&
        (pdidi->dwSize != sizeof(DIDEVICEINSTANCEA))) {
        WARN("invalid parameter: pdidi->dwSize = %d\n", pdidi->dwSize);
        return DIERR_INVALIDPARAM;
    }

    /* Return joystick */
    pdidi->guidInstance = This->guidInstance;
    pdidi->guidProduct = This->guidProduct;
    /* we only support traditional joysticks for now */
    pdidi->dwDevType = This->devcaps.dwDevType;
    strcpy(pdidi->tszInstanceName, "Joystick");
    strcpy(pdidi->tszProductName, This->name);
    if (pdidi->dwSize > sizeof(DIDEVICEINSTANCE_DX3A)) {
        pdidi->guidFFDriver = GUID_NULL;
        pdidi->wUsagePage = 0;
        pdidi->wUsage = 0;
    }

    return DI_OK;
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
HRESULT WINAPI JoystickWGenericImpl_GetDeviceInfo(
    LPDIRECTINPUTDEVICE8W iface,
    LPDIDEVICEINSTANCEW pdidi)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p,%p)\n", iface, pdidi);

    if ((pdidi->dwSize != sizeof(DIDEVICEINSTANCE_DX3W)) &&
        (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW))) {
        WARN("invalid parameter: pdidi->dwSize = %d\n", pdidi->dwSize);
        return DIERR_INVALIDPARAM;
    }

    /* Return joystick */
    pdidi->guidInstance = This->guidInstance;
    pdidi->guidProduct = This->guidProduct;
    /* we only support traditional joysticks for now */
    pdidi->dwDevType = This->devcaps.dwDevType;
    MultiByteToWideChar(CP_ACP, 0, "Joystick", -1, pdidi->tszInstanceName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, This->name, -1, pdidi->tszProductName, MAX_PATH);
    if (pdidi->dwSize > sizeof(DIDEVICEINSTANCE_DX3W)) {
        pdidi->guidFFDriver = GUID_NULL;
        pdidi->wUsagePage = 0;
        pdidi->wUsage = 0;
    }

    return DI_OK;
}

HRESULT WINAPI JoystickWGenericImpl_Poll(LPDIRECTINPUTDEVICE8W iface)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p)\n",This);

    if (!This->base.acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    This->joy_polldev(IDirectInputDevice8A_from_impl(This));
    return DI_OK;
}

HRESULT WINAPI JoystickAGenericImpl_Poll(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWGenericImpl_Poll(IDirectInputDevice8W_from_impl(This));
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the joystick.
  *
  */
HRESULT WINAPI JoystickWGenericImpl_GetDeviceState(LPDIRECTINPUTDEVICE8W iface, DWORD len, LPVOID ptr)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p,0x%08x,%p)\n", This, len, ptr);

    if (!This->base.acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    /* update joystick state */
    This->joy_polldev(IDirectInputDevice8A_from_impl(This));

    /* convert and copy data to user supplied buffer */
    fill_DataFormat(ptr, len, &This->js, &This->base.data_format);

    return DI_OK;
}

HRESULT WINAPI JoystickAGenericImpl_GetDeviceState(LPDIRECTINPUTDEVICE8A iface, DWORD len, LPVOID ptr)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWGenericImpl_GetDeviceState(IDirectInputDevice8W_from_impl(This), len, ptr);
}


HRESULT WINAPI JoystickWGenericImpl_BuildActionMap(LPDIRECTINPUTDEVICE8W iface,
                                                   LPDIACTIONFORMATW lpdiaf,
                                                   LPCWSTR lpszUserName,
                                                   DWORD dwFlags)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8W(iface);
    unsigned int i, j;
    int has_actions = 0;
    DWORD object_types[] = { DIDFT_AXIS, DIDFT_BUTTON };
    DWORD type_map[] = { DIDFT_RELAXIS, DIDFT_PSHBUTTON };

    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    for (i=0; i < lpdiaf->dwNumActions; i++)
    {
        DWORD inst = (0x000000ff & (lpdiaf->rgoAction[i].dwSemantic)) - 1;
        DWORD type = 0x000000ff & (lpdiaf->rgoAction[i].dwSemantic >> 8);
        DWORD genre = 0xff000000 & lpdiaf->rgoAction[i].dwSemantic;

        /* Don't touch an user configured action */
        if (lpdiaf->rgoAction[i].dwHow == DIAH_USERCONFIG) continue;

        /* Only consider actions of the right genre */
        if (lpdiaf->dwGenre != genre && genre != DIGENRE_ANY) continue;

        for (j=0; j < sizeof(object_types)/sizeof(object_types[0]); j++)
        {
            if (type & object_types[j])
            {
                /* Assure that the object exists */
                LPDIOBJECTDATAFORMAT odf = dataformat_to_odf_by_type(This->base.data_format.wine_df, inst, object_types[j]);

                if (odf != NULL)
                {
                    lpdiaf->rgoAction[i].dwObjID = type_map[j] | (0x0000ff00 & (inst << 8));
                    lpdiaf->rgoAction[i].guidInstance = This->base.guid;
                    lpdiaf->rgoAction[i].dwHow = DIAH_DEFAULT;

                    has_actions = 1;

                    /* No need to try other types if the action was already mapped */
                    break;
                }
            }
        }
    }

    if (!has_actions) return DI_NOEFFECT;

    return IDirectInputDevice8WImpl_BuildActionMap(iface, lpdiaf, lpszUserName, dwFlags);
}

HRESULT WINAPI JoystickAGenericImpl_BuildActionMap(LPDIRECTINPUTDEVICE8A iface,
                                                   LPDIACTIONFORMATA lpdiaf,
                                                   LPCSTR lpszUserName,
                                                   DWORD dwFlags)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8A(iface);
    DIACTIONFORMATW diafW;
    HRESULT hr;
    WCHAR *lpszUserNameW = NULL;
    int username_size;

    diafW.rgoAction = HeapAlloc(GetProcessHeap(), 0, sizeof(DIACTIONW)*lpdiaf->dwNumActions);
    _copy_diactionformatAtoW(&diafW, lpdiaf);

    if (lpszUserName != NULL)
    {
        username_size = MultiByteToWideChar(CP_ACP, 0, lpszUserName, -1, NULL, 0);
        lpszUserNameW = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*username_size);
        MultiByteToWideChar(CP_ACP, 0, lpszUserName, -1, lpszUserNameW, username_size);
    }

    hr = JoystickWGenericImpl_BuildActionMap(&This->base.IDirectInputDevice8W_iface, &diafW, lpszUserNameW, dwFlags);

    _copy_diactionformatWtoA(lpdiaf, &diafW);
    HeapFree(GetProcessHeap(), 0, diafW.rgoAction);
    HeapFree(GetProcessHeap(), 0, lpszUserNameW);

    return hr;
}

HRESULT WINAPI JoystickWGenericImpl_SetActionMap(LPDIRECTINPUTDEVICE8W iface,
                                                 LPDIACTIONFORMATW lpdiaf,
                                                 LPCWSTR lpszUserName,
                                                 DWORD dwFlags)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8W(iface);

    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    return _set_action_map(iface, lpdiaf, lpszUserName, dwFlags, This->base.data_format.wine_df);
}

HRESULT WINAPI JoystickAGenericImpl_SetActionMap(LPDIRECTINPUTDEVICE8A iface,
                                                 LPDIACTIONFORMATA lpdiaf,
                                                 LPCSTR lpszUserName,
                                                 DWORD dwFlags)
{
    JoystickGenericImpl *This = impl_from_IDirectInputDevice8A(iface);
    DIACTIONFORMATW diafW;
    HRESULT hr;
    WCHAR *lpszUserNameW = NULL;
    int username_size;

    diafW.rgoAction = HeapAlloc(GetProcessHeap(), 0, sizeof(DIACTIONW)*lpdiaf->dwNumActions);
    _copy_diactionformatAtoW(&diafW, lpdiaf);

    if (lpszUserName != NULL)
    {
        username_size = MultiByteToWideChar(CP_ACP, 0, lpszUserName, -1, NULL, 0);
        lpszUserNameW = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*username_size);
        MultiByteToWideChar(CP_ACP, 0, lpszUserName, -1, lpszUserNameW, username_size);
    }

    hr = JoystickWGenericImpl_SetActionMap(&This->base.IDirectInputDevice8W_iface, &diafW, lpszUserNameW, dwFlags);

    HeapFree(GetProcessHeap(), 0, diafW.rgoAction);
    HeapFree(GetProcessHeap(), 0, lpszUserNameW);

    return hr;
}

/*
 * This maps the read value (from the input event) to a value in the
 * 'wanted' range.
 * Notes:
 *   Dead zone is in % multiplied by a 100 (range 0..10000)
 */
LONG joystick_map_axis(ObjProps *props, int val)
{
    LONG ret;
    LONG dead_zone = MulDiv( props->lDeadZone, props->lDevMax - props->lDevMin, 10000 );
    LONG dev_range = props->lDevMax - props->lDevMin - dead_zone;

    /* Center input */
    val -= (props->lDevMin + props->lDevMax) / 2;

    /* Remove dead zone */
    if (abs( val ) <= dead_zone / 2)
        val = 0;
    else
        val = val < 0 ? val + dead_zone / 2 : val - dead_zone / 2;

    /* Scale and map the value from the device range into the required range */
    ret = MulDiv( val, props->lMax - props->lMin, dev_range ) +
          (props->lMin + props->lMax) / 2;

    /* Clamp in case or rounding errors */
    if      (ret > props->lMax) ret = props->lMax;
    else if (ret < props->lMin) ret = props->lMin;

    TRACE( "(%d <%d> %d) -> (%d <%d> %d): val=%d ret=%d\n",
           props->lDevMin, dead_zone, props->lDevMax,
           props->lMin, props->lDeadZone, props->lMax,
           val, ret );

    return ret;
}

/*
 * Maps POV x & y event values to a DX "clock" position:
 *         0
 *   31500    4500
 * 27000  -1    9000
 *   22500   13500
 *       18000
 */
DWORD joystick_map_pov(const POINTL *p)
{
    if (p->x > 0)
        return p->y < 0 ?  4500 : !p->y ?  9000 : 13500;
    else if (p->x < 0)
        return p->y < 0 ? 31500 : !p->y ? 27000 : 22500;
    else
        return p->y < 0 ?     0 : !p->y ?    -1 : 18000;
}

/*
 * Setup the dinput options.
 */

HRESULT setup_dinput_options(JoystickGenericImpl *This, const int *default_axis_map)
{
    char buffer[MAX_PATH+16];
    HKEY hkey, appkey;
    int tokens = 0;
    int axis = 0;
    int pov = 0;

    get_app_key(&hkey, &appkey);

    /* get options */

    if (!get_config_key(hkey, appkey, "DefaultDeadZone", buffer, sizeof(buffer)))
    {
        This->deadzone = atoi(buffer);
        TRACE("setting default deadzone to: \"%s\" %d\n", buffer, This->deadzone);
    }

    This->axis_map = HeapAlloc(GetProcessHeap(), 0, This->device_axis_count * sizeof(int));
    if (!This->axis_map) return DIERR_OUTOFMEMORY;

    if (!get_config_key(hkey, appkey, This->name, buffer, sizeof(buffer)))
    {
        static const char *axis_names[] = {"X", "Y", "Z", "Rx", "Ry", "Rz",
                                           "Slider1", "Slider2",
                                           "POV1", "POV2", "POV3", "POV4"};
        const char *delim = ",";
        char * ptr;
        TRACE("\"%s\" = \"%s\"\n", This->name, buffer);

        if ((ptr = strtok(buffer, delim)) != NULL)
        {
            do
            {
                int i;

                for (i = 0; i < sizeof(axis_names) / sizeof(axis_names[0]); i++)
                {
                    if (!strcmp(ptr, axis_names[i]))
                    {
                        if (!strncmp(ptr, "POV", 3))
                        {
                            if (pov >= 4)
                            {
                                WARN("Only 4 POVs supported - ignoring extra\n");
                                i = -1;
                            }
                            else
                            {
                                /* Pov takes two axes */
                                This->axis_map[tokens++] = i;
                                pov++;
                            }
                        }
                        else
                        {
                            if (axis >= 8)
                            {
                                FIXME("Only 8 Axes supported - ignoring extra\n");
                                i = -1;
                            }
                            else
                                axis++;
                        }
                        break;
                    }
                }

                if (i == sizeof(axis_names) / sizeof(axis_names[0]))
                {
                    ERR("invalid joystick axis type: \"%s\"\n", ptr);
                    i = -1;
                }

                This->axis_map[tokens] = i;
                tokens++;
            } while ((ptr = strtok(NULL, delim)) != NULL);

            if (tokens != This->device_axis_count)
            {
                ERR("not all joystick axes mapped: %d axes(%d,%d), %d arguments\n",
                    This->device_axis_count, axis, pov, tokens);
                while (tokens < This->device_axis_count)
                {
                    This->axis_map[tokens] = -1;
                    tokens++;
                }
            }
        }
    }
    else
    {
        int i;

        if (default_axis_map)
        {
            /* Use default mapping from the driver */
            for (i = 0; i < This->device_axis_count; i++)
            {
                This->axis_map[i] = default_axis_map[i];
                tokens = default_axis_map[i];
                if (tokens < 0)
                    continue;
                if (tokens < 8)
                    axis++;
                else if (tokens < 15)
                {
                    i++;
                    pov++;
                    This->axis_map[i] = default_axis_map[i];
                }
            }
        }
        else
        {
            /* No config - set default mapping. */
            for (i = 0; i < This->device_axis_count; i++)
            {
                if (i < 8)
                    This->axis_map[i] = axis++;
                else if (i < 15)
                {
                    This->axis_map[i++] = 8 + pov;
                    This->axis_map[i  ] = 8 + pov++;
                }
                else
                    This->axis_map[i] = -1;
            }
        }
    }
    This->devcaps.dwAxes = axis;
    This->devcaps.dwPOVs = pov;

    if (appkey) RegCloseKey(appkey);
    if (hkey)   RegCloseKey(hkey);

    return DI_OK;
}
