/*		DirectInput Joystick device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef __REACTOS__
#include "dinput.h"
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <errno.h>
#ifdef HAVE_LINUX_IOCTL_H
# include <linux/ioctl.h>
#endif
#ifdef HAVE_LINUX_JOYSTICK_H
# include <linux/joystick.h>
# undef SW_MAX
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "devguid.h"
#ifndef __REACTOS__
#include "dinput.h"
#endif

#include "dinput_private.h"
#include "device_private.h"
#include "joystick_private.h"

#ifdef HAVE_LINUX_22_JOYSTICK_API

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

#define JOYDEV_NEW "/dev/input/js"
#define JOYDEV_OLD "/dev/js"
#define JOYDEVDRIVER " (js)"

struct JoyDev
{
    char device[MAX_PATH];
    char name[MAX_PATH];
    GUID guid_product;

    BYTE axis_count;
    BYTE button_count;
    int  *dev_axes_map;

    WORD vendor_id, product_id, bus_type;

    BOOL is_joystick;
};

typedef struct JoystickImpl JoystickImpl;
static const IDirectInputDevice8AVtbl JoystickAvt;
static const IDirectInputDevice8WVtbl JoystickWvt;
struct JoystickImpl
{
        struct JoystickGenericImpl generic;

        struct JoyDev                  *joydev;

	/* joystick private */
	int				joyfd;
        POINTL                          povs[4];
};

static inline JoystickImpl *impl_from_IDirectInputDevice8A(IDirectInputDevice8A *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8A_iface),
           JoystickGenericImpl, base), JoystickImpl, generic);
}
static inline JoystickImpl *impl_from_IDirectInputDevice8W(IDirectInputDevice8W *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface),
           JoystickGenericImpl, base), JoystickImpl, generic);
}

static inline IDirectInputDevice8W *IDirectInputDevice8W_from_impl(JoystickImpl *This)
{
    return &This->generic.base.IDirectInputDevice8W_iface;
}

static const GUID DInput_Wine_Joystick_GUID = { /* 9e573ed9-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed9,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

#define MAX_JOYSTICKS 64
static INT joystick_devices_count = -1;
static struct JoyDev *joystick_devices;

static void joy_polldev(LPDIRECTINPUTDEVICE8A iface);

#define SYS_PATH_FORMAT "/sys/class/input/js%d/device/id/%s"
static BOOL read_sys_id_variable(int index, const char *property, WORD *value)
{
    char sys_path[sizeof(SYS_PATH_FORMAT) + 16], id_str[5];
    int sys_fd;
    BOOL ret = FALSE;

    sprintf(sys_path, SYS_PATH_FORMAT, index, property);
    if ((sys_fd = open(sys_path, O_RDONLY)) != -1)
    {
        if (read(sys_fd, id_str, 4) == 4)
        {
            id_str[4] = '\0';
            *value = strtol(id_str, NULL, 16);
            ret = TRUE;
        }

        close(sys_fd);
    }
    return ret;
}
#undef SYS_PATH_FORMAT

static INT find_joystick_devices(void)
{
    INT i;

    if (joystick_devices_count != -1) return joystick_devices_count;

    joystick_devices_count = 0;
    for (i = 0; i < MAX_JOYSTICKS; i++)
    {
        int fd;
        struct JoyDev joydev, *new_joydevs;
        BYTE axes_map[ABS_MAX + 1];
        SHORT btn_map[KEY_MAX - BTN_MISC + 1];
        BOOL is_stylus = FALSE;

        snprintf(joydev.device, sizeof(joydev.device), "%s%d", JOYDEV_NEW, i);
        if ((fd = open(joydev.device, O_RDONLY)) == -1)
        {
            snprintf(joydev.device, sizeof(joydev.device), "%s%d", JOYDEV_OLD, i);
            if ((fd = open(joydev.device, O_RDONLY)) == -1) continue;
        }

        strcpy(joydev.name, "Wine Joystick");
#if defined(JSIOCGNAME)
        if (ioctl(fd, JSIOCGNAME(sizeof(joydev.name) - sizeof(JOYDEVDRIVER)), joydev.name) < 0)
            WARN("ioctl(%s,JSIOCGNAME) failed: %s\n", joydev.device, strerror(errno));
#endif

        /* Append driver name */
        strcat(joydev.name, JOYDEVDRIVER);

        if (device_disabled_registry(joydev.name)) {
            close(fd);
            continue;
        }

#ifdef JSIOCGAXES
        if (ioctl(fd, JSIOCGAXES, &joydev.axis_count) < 0)
        {
            WARN("ioctl(%s,JSIOCGAXES) failed: %s, defaulting to 2\n", joydev.device, strerror(errno));
            joydev.axis_count = 2;
        }
#else
        WARN("reading number of joystick axes unsupported in this platform, defaulting to 2\n");
        joydev.axis_count = 2;
#endif
#ifdef JSIOCGBUTTONS
        if (ioctl(fd, JSIOCGBUTTONS, &joydev.button_count) < 0)
        {
            WARN("ioctl(%s,JSIOCGBUTTONS) failed: %s, defaulting to 2\n", joydev.device, strerror(errno));
            joydev.button_count = 2;
        }
#else
        WARN("reading number of joystick buttons unsupported in this platform, defaulting to 2\n");
        joydev.button_count = 2;
#endif

        joydev.is_joystick = FALSE;
        if (ioctl(fd, JSIOCGBTNMAP, btn_map) < 0)
        {
            WARN("ioctl(%s,JSIOCGBTNMAP) failed: %s\n", joydev.device, strerror(errno));
        }
        else
        {
            INT j;
            /* in lieu of properly reporting HID usage, detect presence of
             * "joystick buttons" and report those devices as joysticks instead of
             * gamepads */
            for (j = 0; !joydev.is_joystick && j < joydev.button_count; j++)
            {
                switch (btn_map[j])
                {
                case BTN_TRIGGER:
                case BTN_THUMB:
                case BTN_THUMB2:
                case BTN_TOP:
                case BTN_TOP2:
                case BTN_PINKIE:
                case BTN_BASE:
                case BTN_BASE2:
                case BTN_BASE3:
                case BTN_BASE4:
                case BTN_BASE5:
                case BTN_BASE6:
                case BTN_DEAD:
                    joydev.is_joystick = TRUE;
                    break;
                case BTN_STYLUS:
                    is_stylus = TRUE;
                    break;
                default:
                    break;
                }
            }
        }

        if(is_stylus)
        {
            TRACE("Stylus detected. Skipping\n");
            close(fd);
            continue;
        }

        if (ioctl(fd, JSIOCGAXMAP, axes_map) < 0)
        {
            WARN("ioctl(%s,JSIOCGAXMAP) failed: %s\n", joydev.device, strerror(errno));
            joydev.dev_axes_map = NULL;
        }
        else
            if ((joydev.dev_axes_map = HeapAlloc(GetProcessHeap(), 0, joydev.axis_count * sizeof(int))))
            {
                INT j, found_axes = 0;

                /* Remap to DI numbers */
                for (j = 0; j < joydev.axis_count; j++)
                {
                    if (axes_map[j] < 8)
                    {
                        /* Axis match 1-to-1 */
                        joydev.dev_axes_map[j] = j;
                        found_axes++;
                    }
                    else if (axes_map[j] <= 10)
                    {
                        /* Axes 8 through 10 are Wheel, Gas and Brake,
                         * remap to 0, 1 and 2
                         */
                        joydev.dev_axes_map[j] = axes_map[j] - 8;
                        found_axes++;
                    }
                    else if (axes_map[j] == 16 ||
                             axes_map[j] == 17)
                    {
                        /* POV axis */
                        joydev.dev_axes_map[j] = 8;
                        found_axes++;
                    }
                    else
                        joydev.dev_axes_map[j] = -1;
                }

                /* If no axes were configured but there are axes assume a 1-to-1 (wii controller) */
                if (joydev.axis_count && !found_axes)
                {
                    int axes_limit = min(joydev.axis_count, 8); /* generic driver limit */

                    ERR("Incoherent joystick data, advertised %d axes, detected 0. Assuming 1-to-1.\n",
                        joydev.axis_count);
                    for (j = 0; j < axes_limit; j++)
                        joydev.dev_axes_map[j] = j;

                    joydev.axis_count = axes_limit;
                }
            }

        /* Find vendor_id and product_id in sysfs */
        joydev.vendor_id  = 0;
        joydev.product_id = 0;

        read_sys_id_variable(i, "vendor", &joydev.vendor_id);
        read_sys_id_variable(i, "product", &joydev.product_id);
        read_sys_id_variable(i, "bustype", &joydev.bus_type);

        if (joydev.vendor_id == 0 || joydev.product_id == 0)
        {
            joydev.guid_product = DInput_Wine_Joystick_GUID;
        }
        else
        {
            /* Concatenate product_id with vendor_id to mimic Windows behaviour */
            joydev.guid_product       = DInput_PIDVID_Product_GUID;
            joydev.guid_product.Data1 = MAKELONG(joydev.vendor_id, joydev.product_id);
        }

        close(fd);

        if (!joystick_devices_count)
            new_joydevs = HeapAlloc(GetProcessHeap(), 0, sizeof(struct JoyDev));
        else
            new_joydevs = HeapReAlloc(GetProcessHeap(), 0, joystick_devices,
                                      (joystick_devices_count + 1) * sizeof(struct JoyDev));
        if (!new_joydevs) continue;

        TRACE("Found a joystick on %s: %s\n  with %d axes and %d buttons\n", joydev.device,
              joydev.name, joydev.axis_count, joydev.button_count);

        joystick_devices = new_joydevs;
        joystick_devices[joystick_devices_count++] = joydev;
    }

    return joystick_devices_count;
}

static void fill_joystick_dideviceinstanceW(LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    DWORD dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    memset(lpddi, 0, dwSize);

    /* Return joystick */
    lpddi->dwSize = dwSize;
    lpddi->guidInstance = DInput_Wine_Joystick_GUID;
    lpddi->guidInstance.Data3 = id;
    lpddi->guidProduct = joystick_devices[id].guid_product;
    lpddi->dwDevType = get_device_type(version, joystick_devices[id].is_joystick);

    /* Assume the joystick as HID if it is attached to USB bus and has a valid VID/PID */
    if (joystick_devices[id].bus_type == BUS_USB &&
        joystick_devices[id].vendor_id && joystick_devices[id].product_id)
    {
        lpddi->dwDevType |= DIDEVTYPE_HID;
        lpddi->wUsagePage = 0x01; /* Desktop */
        if (joystick_devices[id].is_joystick)
            lpddi->wUsage = 0x04; /* Joystick */
        else
            lpddi->wUsage = 0x05; /* Game Pad */
    }

    MultiByteToWideChar(CP_ACP, 0, joystick_devices[id].name, -1, lpddi->tszInstanceName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, joystick_devices[id].name, -1, lpddi->tszProductName, MAX_PATH);
    lpddi->guidFFDriver = GUID_NULL;
}

static void fill_joystick_dideviceinstanceA(LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    DIDEVICEINSTANCEW lpddiW;
    DWORD dwSize = lpddi->dwSize;

    lpddiW.dwSize = sizeof(lpddiW);
    fill_joystick_dideviceinstanceW(&lpddiW, version, id);

    TRACE("%d %p\n", dwSize, lpddi);
    memset(lpddi, 0, dwSize);

    /* Convert W->A */
    lpddi->dwSize = dwSize;
    lpddi->guidInstance = lpddiW.guidInstance;
    lpddi->guidProduct = lpddiW.guidProduct;
    lpddi->dwDevType = lpddiW.dwDevType;
    strcpy(lpddi->tszInstanceName, joystick_devices[id].name);
    strcpy(lpddi->tszProductName,  joystick_devices[id].name);
    lpddi->guidFFDriver = lpddiW.guidFFDriver;
    lpddi->wUsagePage = lpddiW.wUsagePage;
    lpddi->wUsage = lpddiW.wUsage;
}

static HRESULT joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    int fd = -1;

    if (id >= find_joystick_devices()) return E_FAIL;

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return S_FALSE;
    }

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_JOYSTICK) && (version >= 0x0300 && version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {
        /* check whether we have a joystick */
        if ((fd = open(joystick_devices[id].device, O_RDONLY)) == -1)
        {
            WARN("open(%s, O_RDONLY) failed: %s\n", joystick_devices[id].device, strerror(errno));
            return S_FALSE;
        }
        fill_joystick_dideviceinstanceA( lpddi, version, id );
        close(fd);
        TRACE("Enumerating the linux Joystick device: %s (%s)\n", joystick_devices[id].device, joystick_devices[id].name);
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    int fd = -1;

    if (id >= find_joystick_devices()) return E_FAIL;

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return S_FALSE;
    }

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_JOYSTICK) && (version >= 0x0300 && version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {
        /* check whether we have a joystick */
        if ((fd = open(joystick_devices[id].device, O_RDONLY)) == -1)
        {
            WARN("open(%s, O_RDONLY) failed: %s\n", joystick_devices[id].device, strerror(errno));
            return S_FALSE;
        }
        fill_joystick_dideviceinstanceW( lpddi, version, id );
        close(fd);
        TRACE("Enumerating the linux Joystick device: %s (%s)\n", joystick_devices[id].device, joystick_devices[id].name);
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT alloc_device(REFGUID rguid, IDirectInputImpl *dinput,
                            JoystickImpl **pdev, unsigned short index)
{
    DWORD i;
    JoystickImpl* newDevice;
    HRESULT hr;
    LPDIDATAFORMAT df = NULL;
    int idx = 0;
    DIDEVICEINSTANCEW ddi;

    TRACE("%s %p %p %hu\n", debugstr_guid(rguid), dinput, pdev, index);

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickImpl));
    if (newDevice == 0) {
        WARN("out of memory\n");
        *pdev = 0;
        return DIERR_OUTOFMEMORY;
    }

    newDevice->joydev = &joystick_devices[index];
    newDevice->joyfd = -1;
    newDevice->generic.guidInstance = DInput_Wine_Joystick_GUID;
    newDevice->generic.guidInstance.Data3 = index;
    newDevice->generic.guidProduct = DInput_Wine_Joystick_GUID;
    newDevice->generic.joy_polldev = joy_polldev;
    newDevice->generic.name        = newDevice->joydev->name;
    newDevice->generic.device_axis_count = newDevice->joydev->axis_count;
    newDevice->generic.devcaps.dwButtons = newDevice->joydev->button_count;

    if (newDevice->generic.devcaps.dwButtons > 128)
    {
        WARN("Can't support %d buttons. Clamping down to 128\n", newDevice->generic.devcaps.dwButtons);
        newDevice->generic.devcaps.dwButtons = 128;
    }

    newDevice->generic.base.IDirectInputDevice8A_iface.lpVtbl = &JoystickAvt;
    newDevice->generic.base.IDirectInputDevice8W_iface.lpVtbl = &JoystickWvt;
    newDevice->generic.base.ref = 1;
    newDevice->generic.base.dinput = dinput;
    newDevice->generic.base.guid = *rguid;
    InitializeCriticalSection(&newDevice->generic.base.crit);
    newDevice->generic.base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JoystickImpl*->generic.base.crit");

    /* setup_dinput_options may change these */
    newDevice->generic.deadzone = 0;

    /* do any user specified configuration */
    hr = setup_dinput_options(&newDevice->generic, newDevice->joydev->dev_axes_map);
    if (hr != DI_OK)
        goto FAILED1;

    /* Create copy of default data format */
    if (!(df = HeapAlloc(GetProcessHeap(), 0, c_dfDIJoystick2.dwSize))) goto FAILED;
    memcpy(df, &c_dfDIJoystick2, c_dfDIJoystick2.dwSize);

    df->dwNumObjs = newDevice->generic.devcaps.dwAxes + newDevice->generic.devcaps.dwPOVs + newDevice->generic.devcaps.dwButtons;
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto FAILED;

    for (i = 0; i < newDevice->generic.device_axis_count; i++)
    {
        int wine_obj = newDevice->generic.axis_map[i];

        if (wine_obj < 0) continue;

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[wine_obj], df->dwObjSize);
        if (wine_obj < 8)
            df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(wine_obj) | DIDFT_ABSAXIS;
        else
        {
            df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(wine_obj - 8) | DIDFT_POV;
            i++; /* POV takes 2 axes */
        }
    }
    for (i = 0; i < newDevice->generic.devcaps.dwButtons; i++)
    {
        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[i + 12], df->dwObjSize);
        df->rgodf[idx  ].pguid = &GUID_Button;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(i) | DIDFT_PSHBUTTON;
    }
    newDevice->generic.base.data_format.wine_df = df;

    /* initialize default properties */
    for (i = 0; i < c_dfDIJoystick2.dwNumObjs; i++) {
        newDevice->generic.props[i].lDevMin = -32767;
        newDevice->generic.props[i].lDevMax = +32767;
        newDevice->generic.props[i].lMin = 0;
        newDevice->generic.props[i].lMax = 0xffff;
        newDevice->generic.props[i].lDeadZone = newDevice->generic.deadzone; /* % * 1000 */
        newDevice->generic.props[i].lSaturation = 0;
    }

    IDirectInput_AddRef(&newDevice->generic.base.dinput->IDirectInput7A_iface);

    EnterCriticalSection(&dinput->crit);
    list_add_tail(&dinput->devices_list, &newDevice->generic.base.entry);
    LeaveCriticalSection(&dinput->crit);

    newDevice->generic.devcaps.dwSize = sizeof(newDevice->generic.devcaps);
    newDevice->generic.devcaps.dwFlags = DIDC_ATTACHED;

    ddi.dwSize = sizeof(ddi);
    fill_joystick_dideviceinstanceW(&ddi, newDevice->generic.base.dinput->dwVersion, index);
    newDevice->generic.devcaps.dwDevType = ddi.dwDevType;

    newDevice->generic.devcaps.dwFFSamplePeriod = 0;
    newDevice->generic.devcaps.dwFFMinTimeResolution = 0;
    newDevice->generic.devcaps.dwFirmwareRevision = 0;
    newDevice->generic.devcaps.dwHardwareRevision = 0;
    newDevice->generic.devcaps.dwFFDriverVersion = 0;

    if (TRACE_ON(dinput)) {
        _dump_DIDATAFORMAT(newDevice->generic.base.data_format.wine_df);
       for (i = 0; i < (newDevice->generic.device_axis_count); i++)
           TRACE("axis_map[%d] = %d\n", i, newDevice->generic.axis_map[i]);
        _dump_DIDEVCAPS(&newDevice->generic.devcaps);
    }

    *pdev = newDevice;

    return DI_OK;

FAILED:
    hr = DIERR_OUTOFMEMORY;
FAILED1:
    if (df) HeapFree(GetProcessHeap(), 0, df->rgodf);
    HeapFree(GetProcessHeap(), 0, df);
    release_DataFormat(&newDevice->generic.base.data_format);
    HeapFree(GetProcessHeap(),0,newDevice->generic.axis_map);
    HeapFree(GetProcessHeap(),0,newDevice);
    *pdev = 0;

    return hr;
}

/******************************************************************************
  *     get_joystick_index : Get the joystick index from a given GUID
  */
static unsigned short get_joystick_index(REFGUID guid)
{
    GUID wine_joystick = DInput_Wine_Joystick_GUID;
    GUID dev_guid = *guid;

    wine_joystick.Data3 = 0;
    dev_guid.Data3 = 0;

    /* for the standard joystick GUID use index 0 */
    if(IsEqualGUID(&GUID_Joystick,guid)) return 0;

    /* for the wine joystick GUIDs use the index stored in Data3 */
    if(IsEqualGUID(&wine_joystick, &dev_guid)) return guid->Data3;

    return MAX_JOYSTICKS;
}

static HRESULT joydev_create_device(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPVOID *pdev, int unicode)
{
    unsigned short index;

    TRACE("%p %s %s %p %i\n", dinput, debugstr_guid(rguid), debugstr_guid(riid), pdev, unicode);
    find_joystick_devices();
    *pdev = NULL;

    if ((index = get_joystick_index(rguid)) < MAX_JOYSTICKS &&
        joystick_devices_count && index < joystick_devices_count)
    {
        JoystickImpl *This;
        HRESULT hr;

        if (riid == NULL)
            ;/* nothing */
        else if (IsEqualGUID(&IID_IDirectInputDeviceA,  riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice2A, riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice7A, riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice8A, riid))
        {
            unicode = 0;
        }
        else if (IsEqualGUID(&IID_IDirectInputDeviceW,  riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice2W, riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice7W, riid) ||
                 IsEqualGUID(&IID_IDirectInputDevice8W, riid))
        {
            unicode = 1;
        }
        else
        {
            WARN("no interface\n");
            return DIERR_NOINTERFACE;
        }

        hr = alloc_device(rguid, dinput, &This, index);
        if (!This) return hr;

        if (unicode)
            *pdev = &This->generic.base.IDirectInputDevice8W_iface;
        else
            *pdev = &This->generic.base.IDirectInputDevice8A_iface;

        return hr;
    }

    return DIERR_DEVICENOTREG;
}

#undef MAX_JOYSTICKS

const struct dinput_device joystick_linux_device = {
  "Wine Linux joystick driver",
  joydev_enum_deviceA,
  joydev_enum_deviceW,
  joydev_create_device
};

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickLinuxWImpl_Acquire(LPDIRECTINPUTDEVICE8W iface)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;

    TRACE("(%p)\n",This);

    res = IDirectInputDevice2WImpl_Acquire(iface);
    if (res != DI_OK)
        return res;

    /* open the joystick device */
    if (This->joyfd==-1) {
        TRACE("opening joystick device %s\n", This->joydev->device);

        This->joyfd = open(This->joydev->device, O_RDONLY);
        if (This->joyfd==-1) {
            ERR("open(%s) failed: %s\n", This->joydev->device, strerror(errno));
            IDirectInputDevice2WImpl_Unacquire(iface);
            return DIERR_NOTFOUND;
        }
    }

    return DI_OK;
}

static HRESULT WINAPI JoystickLinuxAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickLinuxWImpl_Acquire(IDirectInputDevice8W_from_impl(This));
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI JoystickLinuxWImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (!IS_DIPROP(rguid)) return DI_OK;

    switch (LOWORD(rguid)) {

        case (DWORD_PTR) DIPROP_VIDPID:
        {
            LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;

            if (!This->joydev->product_id || !This->joydev->vendor_id)
                return DIERR_UNSUPPORTED;
            pd->dwData = MAKELONG(This->joydev->vendor_id, This->joydev->product_id);
            TRACE("DIPROP_VIDPID(%08x)\n", pd->dwData);
            break;
        }
        case (DWORD_PTR) DIPROP_JOYSTICKID:
        {
            LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;

            pd->dwData = get_joystick_index(&This->generic.base.guid);
            TRACE("DIPROP_JOYSTICKID(%d)\n", pd->dwData);
            break;
        }

        case (DWORD_PTR) DIPROP_GUIDANDPATH:
        {
            static const WCHAR formatW[] = {'\\','\\','?','\\','h','i','d','#','v','i','d','_','%','0','4','x','&',
                                            'p','i','d','_','%','0','4','x','&','%','s','_','%','h','u',0};
            static const WCHAR miW[] = {'m','i',0};
            static const WCHAR igW[] = {'i','g',0};

            BOOL is_gamepad;
            LPDIPROPGUIDANDPATH pd = (LPDIPROPGUIDANDPATH)pdiph;
            WORD vid = This->joydev->vendor_id;
            WORD pid = This->joydev->product_id;

            if (!pid || !vid)
                return DIERR_UNSUPPORTED;

            is_gamepad = is_xinput_device(&This->generic.devcaps, vid, pid);
            pd->guidClass = GUID_DEVCLASS_HIDCLASS;
            sprintfW(pd->wszPath, formatW, vid, pid, is_gamepad ? igW : miW, get_joystick_index(&This->generic.base.guid));

            TRACE("DIPROP_GUIDANDPATH(%s, %s): returning fake path\n", debugstr_guid(&pd->guidClass), debugstr_w(pd->wszPath));
            break;
        }

    default:
        return JoystickWGenericImpl_GetProperty(iface, rguid, pdiph);
    }

    return DI_OK;
}

static HRESULT WINAPI JoystickLinuxAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickLinuxWImpl_GetProperty(IDirectInputDevice8W_from_impl(This), rguid, pdiph);
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
static HRESULT WINAPI JoystickLinuxAImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8A iface, LPDIDEVICEINSTANCEA ddi)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);

    TRACE("(%p) %p\n", This, ddi);

    if (ddi == NULL) return E_POINTER;
    if ((ddi->dwSize != sizeof(DIDEVICEINSTANCE_DX3A)) &&
        (ddi->dwSize != sizeof(DIDEVICEINSTANCEA)))
        return DIERR_INVALIDPARAM;

    fill_joystick_dideviceinstanceA( ddi, This->generic.base.dinput->dwVersion,
                                     get_joystick_index(&This->generic.base.guid) );
    return DI_OK;
}

static HRESULT WINAPI JoystickLinuxWImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8W iface, LPDIDEVICEINSTANCEW ddi)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p) %p\n", This, ddi);

    if (ddi == NULL) return E_POINTER;
    if ((ddi->dwSize != sizeof(DIDEVICEINSTANCE_DX3W)) &&
        (ddi->dwSize != sizeof(DIDEVICEINSTANCEW)))
        return DIERR_INVALIDPARAM;

    fill_joystick_dideviceinstanceW( ddi, This->generic.base.dinput->dwVersion,
                                     get_joystick_index(&This->generic.base.guid) );
    return DI_OK;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickLinuxWImpl_Unacquire(LPDIRECTINPUTDEVICE8W iface)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;

    TRACE("(%p)\n",This);

    res = IDirectInputDevice2WImpl_Unacquire(iface);

    if (res != DI_OK)
        return res;

    if (This->joyfd!=-1) {
        TRACE("closing joystick device\n");
        close(This->joyfd);
        This->joyfd = -1;
        return DI_OK;
    }

    return DI_NOEFFECT;
}

static HRESULT WINAPI JoystickLinuxAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickLinuxWImpl_Unacquire(IDirectInputDevice8W_from_impl(This));
}

static void joy_polldev(LPDIRECTINPUTDEVICE8A iface)
{
    struct pollfd plfd;
    struct js_event jse;
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);

    TRACE("(%p)\n", This);

    if (This->joyfd==-1) {
        WARN("no device\n");
        return;
    }
    while (1)
    {
        LONG value;
        int inst_id = -1;

	plfd.fd = This->joyfd;
	plfd.events = POLLIN;
	if (poll(&plfd,1,0) != 1)
	    return;
	/* we have one event, so we can read */
	if (sizeof(jse)!=read(This->joyfd,&jse,sizeof(jse))) {
	    return;
	}
        TRACE("js_event: type 0x%x, number %d, value %d\n",
              jse.type,jse.number,jse.value);
        if (jse.type & JS_EVENT_BUTTON)
        {
            int button;
            if (jse.number >= This->generic.devcaps.dwButtons) return;

            button = This->generic.button_map[jse.number];

            inst_id = DIDFT_MAKEINSTANCE(button) | DIDFT_PSHBUTTON;
            This->generic.js.rgbButtons[button] = value = jse.value ? 0x80 : 0x00;
        }
        else if (jse.type & JS_EVENT_AXIS)
        {
            int number = This->generic.axis_map[jse.number];	/* wine format object index */

            if (number < 0) return;
            inst_id = number < 8 ?  DIDFT_MAKEINSTANCE(number) | DIDFT_ABSAXIS :
                                    DIDFT_MAKEINSTANCE(number - 8) | DIDFT_POV;
            value = joystick_map_axis(&This->generic.props[id_to_object(This->generic.base.data_format.wine_df, inst_id)], jse.value);

            TRACE("changing axis %d => %d\n", jse.number, number);
            switch (number)
            {
                case 0: This->generic.js.lX  = value; break;
                case 1: This->generic.js.lY  = value; break;
                case 2: This->generic.js.lZ  = value; break;
                case 3: This->generic.js.lRx = value; break;
                case 4: This->generic.js.lRy = value; break;
                case 5: This->generic.js.lRz = value; break;
                case 6: This->generic.js.rglSlider[0] = value; break;
                case 7: This->generic.js.rglSlider[1] = value; break;
                case 8: case 9: case 10: case 11:
                {
                    int idx = number - 8;

                    if (jse.number % 2)
                        This->povs[idx].y = jse.value;
                    else
                        This->povs[idx].x = jse.value;

                    This->generic.js.rgdwPOV[idx] = value = joystick_map_pov(&This->povs[idx]);
                    break;
                }
                default:
                    WARN("axis %d not supported\n", number);
            }
        }
        if (inst_id >= 0)
            queue_event(iface, inst_id, value, GetCurrentTime(), This->generic.base.dinput->evsequence++);
    }
}

static const IDirectInputDevice8AVtbl JoystickAvt =
{
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
        IDirectInputDevice2AImpl_Release,
	JoystickAGenericImpl_GetCapabilities,
        IDirectInputDevice2AImpl_EnumObjects,
	JoystickLinuxAImpl_GetProperty,
	JoystickAGenericImpl_SetProperty,
	JoystickLinuxAImpl_Acquire,
	JoystickLinuxAImpl_Unacquire,
	JoystickAGenericImpl_GetDeviceState,
	IDirectInputDevice2AImpl_GetDeviceData,
	IDirectInputDevice2AImpl_SetDataFormat,
	IDirectInputDevice2AImpl_SetEventNotification,
	IDirectInputDevice2AImpl_SetCooperativeLevel,
	JoystickAGenericImpl_GetObjectInfo,
	JoystickLinuxAImpl_GetDeviceInfo,
	IDirectInputDevice2AImpl_RunControlPanel,
	IDirectInputDevice2AImpl_Initialize,
	IDirectInputDevice2AImpl_CreateEffect,
	IDirectInputDevice2AImpl_EnumEffects,
	IDirectInputDevice2AImpl_GetEffectInfo,
	IDirectInputDevice2AImpl_GetForceFeedbackState,
	IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	IDirectInputDevice2AImpl_Escape,
	JoystickAGenericImpl_Poll,
	IDirectInputDevice2AImpl_SendDeviceData,
	IDirectInputDevice7AImpl_EnumEffectsInFile,
	IDirectInputDevice7AImpl_WriteEffectToFile,
	JoystickAGenericImpl_BuildActionMap,
	JoystickAGenericImpl_SetActionMap,
	IDirectInputDevice8AImpl_GetImageInfo
};

static const IDirectInputDevice8WVtbl JoystickWvt =
{
    IDirectInputDevice2WImpl_QueryInterface,
    IDirectInputDevice2WImpl_AddRef,
    IDirectInputDevice2WImpl_Release,
    JoystickWGenericImpl_GetCapabilities,
    IDirectInputDevice2WImpl_EnumObjects,
    JoystickLinuxWImpl_GetProperty,
    JoystickWGenericImpl_SetProperty,
    JoystickLinuxWImpl_Acquire,
    JoystickLinuxWImpl_Unacquire,
    JoystickWGenericImpl_GetDeviceState,
    IDirectInputDevice2WImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    JoystickWGenericImpl_GetObjectInfo,
    JoystickLinuxWImpl_GetDeviceInfo,
    IDirectInputDevice2WImpl_RunControlPanel,
    IDirectInputDevice2WImpl_Initialize,
    IDirectInputDevice2WImpl_CreateEffect,
    IDirectInputDevice2WImpl_EnumEffects,
    IDirectInputDevice2WImpl_GetEffectInfo,
    IDirectInputDevice2WImpl_GetForceFeedbackState,
    IDirectInputDevice2WImpl_SendForceFeedbackCommand,
    IDirectInputDevice2WImpl_EnumCreatedEffectObjects,
    IDirectInputDevice2WImpl_Escape,
    JoystickWGenericImpl_Poll,
    IDirectInputDevice2WImpl_SendDeviceData,
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    JoystickWGenericImpl_BuildActionMap,
    JoystickWGenericImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};

#else  /* HAVE_LINUX_22_JOYSTICK_API */

const struct dinput_device joystick_linux_device = {
  "Wine Linux joystick driver",
  NULL,
  NULL,
  NULL
};

#endif  /* HAVE_LINUX_22_JOYSTICK_API */
