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

#include "dinput_private.h"

#include <stdio.h>

#include "joystick_private.h"

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

DWORD typeFromGUID(REFGUID guid)
{
    if (IsEqualGUID(guid, &GUID_ConstantForce)) {
        return DIEFT_CONSTANTFORCE;
    } else if (IsEqualGUID(guid, &GUID_Square)
            || IsEqualGUID(guid, &GUID_Sine)
            || IsEqualGUID(guid, &GUID_Triangle)
            || IsEqualGUID(guid, &GUID_SawtoothUp)
            || IsEqualGUID(guid, &GUID_SawtoothDown)) {
        return DIEFT_PERIODIC;
    } else if (IsEqualGUID(guid, &GUID_RampForce)) {
        return DIEFT_RAMPFORCE;
    } else if (IsEqualGUID(guid, &GUID_Spring)
            || IsEqualGUID(guid, &GUID_Damper)
            || IsEqualGUID(guid, &GUID_Inertia)
            || IsEqualGUID(guid, &GUID_Friction)) {
        return DIEFT_CONDITION;
    } else if (IsEqualGUID(guid, &GUID_CustomForce)) {
        return DIEFT_CUSTOMFORCE;
    } else {
        WARN("GUID (%s) is not a known force type\n", _dump_dinput_GUID(guid));
        return 0;
    }
}

static void _dump_DIEFFECT_flags(DWORD dwFlags)
{
    if (TRACE_ON(dinput)) {
        unsigned int   i;
        static const struct {
            DWORD       mask;
            const char  *name;
        } flags[] = {
#define FE(x) { x, #x}
            FE(DIEFF_CARTESIAN),
            FE(DIEFF_OBJECTIDS),
            FE(DIEFF_OBJECTOFFSETS),
            FE(DIEFF_POLAR),
            FE(DIEFF_SPHERICAL)
#undef FE
        };
        for (i = 0; i < (sizeof(flags) / sizeof(flags[0])); i++)
            if (flags[i].mask & dwFlags)
                TRACE("%s ", flags[i].name);
        TRACE("\n");
    }
}

static void _dump_DIENVELOPE(LPCDIENVELOPE env)
{
    if (env->dwSize != sizeof(DIENVELOPE)) {
        WARN("Non-standard DIENVELOPE structure size %d.\n", env->dwSize);
    }
    TRACE("Envelope has attack (level: %d time: %d), fade (level: %d time: %d)\n",
          env->dwAttackLevel, env->dwAttackTime, env->dwFadeLevel, env->dwFadeTime);
}

static void _dump_DICONSTANTFORCE(LPCDICONSTANTFORCE frc)
{
    TRACE("Constant force has magnitude %d\n", frc->lMagnitude);
}

static void _dump_DIPERIODIC(LPCDIPERIODIC frc)
{
    TRACE("Periodic force has magnitude %d, offset %d, phase %d, period %d\n",
          frc->dwMagnitude, frc->lOffset, frc->dwPhase, frc->dwPeriod);
}

static void _dump_DIRAMPFORCE(LPCDIRAMPFORCE frc)
{
    TRACE("Ramp force has start %d, end %d\n",
          frc->lStart, frc->lEnd);
}

static void _dump_DICONDITION(LPCDICONDITION frc)
{
    TRACE("Condition has offset %d, pos/neg coefficients %d and %d, pos/neg saturations %d and %d, deadband %d\n",
          frc->lOffset, frc->lPositiveCoefficient, frc->lNegativeCoefficient,
          frc->dwPositiveSaturation, frc->dwNegativeSaturation, frc->lDeadBand);
}

static void _dump_DICUSTOMFORCE(LPCDICUSTOMFORCE frc)
{
    unsigned int i;
    TRACE("Custom force uses %d channels, sample period %d.  Has %d samples at %p.\n",
          frc->cChannels, frc->dwSamplePeriod, frc->cSamples, frc->rglForceData);
    if (frc->cSamples % frc->cChannels != 0)
        WARN("Custom force has a non-integral samples-per-channel count!\n");
    if (TRACE_ON(dinput)) {
        TRACE("Custom force data (time aligned, axes in order):\n");
        for (i = 1; i <= frc->cSamples; ++i) {
            TRACE("%d ", frc->rglForceData[i]);
            if (i % frc->cChannels == 0)
                TRACE("\n");
        }
    }
}

void dump_DIEFFECT(LPCDIEFFECT eff, REFGUID guid, DWORD dwFlags)
{
    DWORD type = typeFromGUID(guid);
    unsigned int i;

    TRACE("Dumping DIEFFECT structure:\n");
    TRACE("  - dwSize: %d\n", eff->dwSize);
    if ((eff->dwSize != sizeof(DIEFFECT)) && (eff->dwSize != sizeof(DIEFFECT_DX5))) {
        WARN("Non-standard DIEFFECT structure size %d\n", eff->dwSize);
    }
    TRACE("  - dwFlags: %d\n", eff->dwFlags);
    TRACE("    ");
    _dump_DIEFFECT_flags(eff->dwFlags);
    TRACE("  - dwDuration: %d\n", eff->dwDuration);
    TRACE("  - dwGain: %d\n", eff->dwGain);

    if (eff->dwGain > 10000)
        WARN("dwGain is out of range (>10,000)\n");

    TRACE("  - dwTriggerButton: %d\n", eff->dwTriggerButton);
    TRACE("  - dwTriggerRepeatInterval: %d\n", eff->dwTriggerRepeatInterval);
    TRACE("  - rglDirection: %p\n", eff->rglDirection);
    TRACE("  - cbTypeSpecificParams: %d\n", eff->cbTypeSpecificParams);
    TRACE("  - lpvTypeSpecificParams: %p\n", eff->lpvTypeSpecificParams);

    /* Only trace some members if dwFlags indicates they have data */
    if (dwFlags & DIEP_AXES) {
        TRACE("  - cAxes: %d\n", eff->cAxes);
        TRACE("  - rgdwAxes: %p\n", eff->rgdwAxes);

        if (TRACE_ON(dinput) && eff->rgdwAxes) {
            TRACE("    ");
            for (i = 0; i < eff->cAxes; ++i)
                TRACE("%d ", eff->rgdwAxes[i]);
            TRACE("\n");
        }
    }

    if (dwFlags & DIEP_ENVELOPE) {
        TRACE("  - lpEnvelope: %p\n", eff->lpEnvelope);
        if (eff->lpEnvelope != NULL)
            _dump_DIENVELOPE(eff->lpEnvelope);
    }

    if (eff->dwSize > sizeof(DIEFFECT_DX5))
        TRACE("  - dwStartDelay: %d\n", eff->dwStartDelay);

    if (type == DIEFT_CONSTANTFORCE) {
        if (eff->cbTypeSpecificParams != sizeof(DICONSTANTFORCE)) {
            WARN("Effect claims to be a constant force but the type-specific params are the wrong size!\n");
        } else {
            _dump_DICONSTANTFORCE(eff->lpvTypeSpecificParams);
        }
    } else if (type == DIEFT_PERIODIC) {
        if (eff->cbTypeSpecificParams != sizeof(DIPERIODIC)) {
            WARN("Effect claims to be a periodic force but the type-specific params are the wrong size!\n");
        } else {
            _dump_DIPERIODIC(eff->lpvTypeSpecificParams);
        }
    } else if (type == DIEFT_RAMPFORCE) {
        if (eff->cbTypeSpecificParams != sizeof(DIRAMPFORCE)) {
            WARN("Effect claims to be a ramp force but the type-specific params are the wrong size!\n");
        } else {
            _dump_DIRAMPFORCE(eff->lpvTypeSpecificParams);
        }
    } else if (type == DIEFT_CONDITION) {
        if (eff->cbTypeSpecificParams != sizeof(DICONDITION)) {
            WARN("Effect claims to be a condition but the type-specific params are the wrong size!\n");
        } else {
            _dump_DICONDITION(eff->lpvTypeSpecificParams);
        }
    } else if (type == DIEFT_CUSTOMFORCE) {
        if (eff->cbTypeSpecificParams != sizeof(DICUSTOMFORCE)) {
            WARN("Effect claims to be a custom force but the type-specific params are the wrong size!\n");
        } else {
            _dump_DICUSTOMFORCE(eff->lpvTypeSpecificParams);
        }
    }
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
    ObjProps remap_props;

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

                    remap_props.lDevMin = This->props[i].lMin;
                    remap_props.lDevMax = This->props[i].lMax;

                    remap_props.lDeadZone = This->props[i].lDeadZone;
                    remap_props.lSaturation = This->props[i].lSaturation;

                    remap_props.lMin = pr->lMin;
                    remap_props.lMax = pr->lMax;

                    switch (This->base.data_format.wine_df->rgodf[i].dwOfs) {
                    case DIJOFS_X        : This->js.lX  = joystick_map_axis(&remap_props, This->js.lX); break;
                    case DIJOFS_Y        : This->js.lY  = joystick_map_axis(&remap_props, This->js.lY); break;
                    case DIJOFS_Z        : This->js.lZ  = joystick_map_axis(&remap_props, This->js.lZ); break;
                    case DIJOFS_RX       : This->js.lRx = joystick_map_axis(&remap_props, This->js.lRx); break;
                    case DIJOFS_RY       : This->js.lRy = joystick_map_axis(&remap_props, This->js.lRy); break;
                    case DIJOFS_RZ       : This->js.lRz = joystick_map_axis(&remap_props, This->js.lRz); break;
                    case DIJOFS_SLIDER(0): This->js.rglSlider[0] = joystick_map_axis(&remap_props, This->js.rglSlider[0]); break;
                    case DIJOFS_SLIDER(1): This->js.rglSlider[1] = joystick_map_axis(&remap_props, This->js.rglSlider[1]); break;
	            default: break;
                    }

                    This->props[i].lMin = pr->lMin;
                    This->props[i].lMax = pr->lMax;
                }
            } else {
                int obj = find_property(&This->base.data_format, ph);

                TRACE("proprange(%d,%d) obj=%d\n", pr->lMin, pr->lMax, obj);
                if (obj >= 0) {

                    /*ePSXe polls the joystick immediately after setting the range for calibration purposes, so the old values need to be remapped to the new range before it does so*/

                    remap_props.lDevMin = This->props[obj].lMin;
                    remap_props.lDevMax = This->props[obj].lMax;

                    remap_props.lDeadZone = This->props[obj].lDeadZone;
                    remap_props.lSaturation = This->props[obj].lSaturation;

                    remap_props.lMin = pr->lMin;
                    remap_props.lMax = pr->lMax;

                    switch (ph->dwObj) {
                    case DIJOFS_X        : This->js.lX  = joystick_map_axis(&remap_props, This->js.lX); break;
                    case DIJOFS_Y        : This->js.lY  = joystick_map_axis(&remap_props, This->js.lY); break;
                    case DIJOFS_Z        : This->js.lZ  = joystick_map_axis(&remap_props, This->js.lZ); break;
                    case DIJOFS_RX       : This->js.lRx = joystick_map_axis(&remap_props, This->js.lRx); break;
                    case DIJOFS_RY       : This->js.lRy = joystick_map_axis(&remap_props, This->js.lRy); break;
                    case DIJOFS_RZ       : This->js.lRz = joystick_map_axis(&remap_props, This->js.lRz); break;
                    case DIJOFS_SLIDER(0): This->js.rglSlider[0] = joystick_map_axis(&remap_props, This->js.rglSlider[0]); break;
                    case DIJOFS_SLIDER(1): This->js.rglSlider[1] = joystick_map_axis(&remap_props, This->js.rglSlider[1]); break;
		    default: break;
                    }

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

#define DEBUG_TYPE(x) case (x): str = #x; break
void _dump_DIDEVCAPS(const DIDEVCAPS *lpDIDevCaps)
{
    int type = GET_DIDEVICE_TYPE(lpDIDevCaps->dwDevType);
    const char *str;
    TRACE("dwSize: %d\n", lpDIDevCaps->dwSize);
    TRACE("dwFlags: %08x\n", lpDIDevCaps->dwFlags);
    switch(type)
    {
        /* Direct X <= 7 definitions */
        DEBUG_TYPE(DIDEVTYPE_DEVICE);
        DEBUG_TYPE(DIDEVTYPE_MOUSE);
        DEBUG_TYPE(DIDEVTYPE_KEYBOARD);
        DEBUG_TYPE(DIDEVTYPE_JOYSTICK);
        DEBUG_TYPE(DIDEVTYPE_HID);
        /* Direct X >= 8 definitions */
        DEBUG_TYPE(DI8DEVTYPE_DEVICE);
        DEBUG_TYPE(DI8DEVTYPE_MOUSE);
        DEBUG_TYPE(DI8DEVTYPE_KEYBOARD);
        DEBUG_TYPE(DI8DEVTYPE_JOYSTICK);
        DEBUG_TYPE(DI8DEVTYPE_GAMEPAD);
        DEBUG_TYPE(DI8DEVTYPE_DRIVING);
        DEBUG_TYPE(DI8DEVTYPE_FLIGHT);
        DEBUG_TYPE(DI8DEVTYPE_1STPERSON);
        DEBUG_TYPE(DI8DEVTYPE_DEVICECTRL);
        DEBUG_TYPE(DI8DEVTYPE_SCREENPOINTER);
        DEBUG_TYPE(DI8DEVTYPE_REMOTE);
        DEBUG_TYPE(DI8DEVTYPE_SUPPLEMENTAL);
        default: str = "UNKNOWN";
    }

    TRACE("dwDevType: %08x %s\n", lpDIDevCaps->dwDevType, str);
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
#undef DEBUG_TYPE

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
    DIPROPDWORD pd;
    DWORD index = 0;

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

    /* Try to get joystick index */
    pd.diph.dwSize = sizeof(pd);
    pd.diph.dwHeaderSize = sizeof(pd.diph);
    pd.diph.dwObj = 0;
    pd.diph.dwHow = DIPH_DEVICE;
    if (SUCCEEDED(IDirectInputDevice2_GetProperty(iface, DIPROP_JOYSTICKID, &pd.diph)))
        index = pd.dwData;

    /* Return joystick */
    pdidi->guidInstance = This->guidInstance;
    pdidi->guidProduct = This->guidProduct;
    /* we only support traditional joysticks for now */
    pdidi->dwDevType = This->devcaps.dwDevType;
    snprintf(pdidi->tszInstanceName, MAX_PATH, "Joystick %d", index);
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
    CHAR buffer[MAX_PATH];
    DIPROPDWORD pd;
    DWORD index = 0;

    TRACE("(%p,%p)\n", iface, pdidi);

    if ((pdidi->dwSize != sizeof(DIDEVICEINSTANCE_DX3W)) &&
        (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW))) {
        WARN("invalid parameter: pdidi->dwSize = %d\n", pdidi->dwSize);
        return DIERR_INVALIDPARAM;
    }

    /* Try to get joystick index */
    pd.diph.dwSize = sizeof(pd);
    pd.diph.dwHeaderSize = sizeof(pd.diph);
    pd.diph.dwObj = 0;
    pd.diph.dwHow = DIPH_DEVICE;
    if (SUCCEEDED(IDirectInputDevice2_GetProperty(iface, DIPROP_JOYSTICKID, &pd.diph)))
        index = pd.dwData;

    /* Return joystick */
    pdidi->guidInstance = This->guidInstance;
    pdidi->guidProduct = This->guidProduct;
    /* we only support traditional joysticks for now */
    pdidi->dwDevType = This->devcaps.dwDevType;
    snprintf(buffer, sizeof(buffer), "Joystick %d", index);
    MultiByteToWideChar(CP_ACP, 0, buffer, -1, pdidi->tszInstanceName, MAX_PATH);
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
    BOOL has_actions = FALSE;
    DWORD object_types[] = { DIDFT_AXIS, DIDFT_BUTTON };
    DWORD type_map[] = { DIDFT_RELAXIS, DIDFT_PSHBUTTON };

    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    for (i=0; i < lpdiaf->dwNumActions; i++)
    {
        DWORD inst = (0x000000ff & (lpdiaf->rgoAction[i].dwSemantic)) - 1;
        DWORD type = 0x000000ff & (lpdiaf->rgoAction[i].dwSemantic >> 8);
        DWORD genre = 0xff000000 & lpdiaf->rgoAction[i].dwSemantic;

        /* Don't touch a user configured action */
        if (lpdiaf->rgoAction[i].dwHow == DIAH_USERCONFIG) continue;

        /* Only consider actions of the right genre */
        if (lpdiaf->dwGenre != genre && genre != DIGENRE_ANY) continue;

        for (j=0; j < sizeof(object_types)/sizeof(object_types[0]); j++)
        {
            if (type & object_types[j])
            {
                /* Ensure that the object exists */
                LPDIOBJECTDATAFORMAT odf = dataformat_to_odf_by_type(This->base.data_format.wine_df, inst, object_types[j]);

                if (odf != NULL)
                {
                    lpdiaf->rgoAction[i].dwObjID = type_map[j] | (0x0000ff00 & (inst << 8));
                    lpdiaf->rgoAction[i].guidInstance = This->base.guid;
                    lpdiaf->rgoAction[i].dwHow = DIAH_DEFAULT;

                    has_actions = TRUE;

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
