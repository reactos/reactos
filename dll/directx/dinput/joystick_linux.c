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
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif
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
#include "winreg.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

#ifdef HAVE_LINUX_22_JOYSTICK_API

#define JOYDEV_NEW "/dev/input/js"
#define JOYDEV_OLD "/dev/js"

typedef struct JoystickImpl JoystickImpl;
static const IDirectInputDevice8AVtbl JoystickAvt;
static const IDirectInputDevice8WVtbl JoystickWvt;
struct JoystickImpl
{
        struct IDirectInputDevice2AImpl base;

	char				dev[32];

	/* joystick private */
	int				joyfd;
	DIJOYSTATE2			js;		/* wine data */
	ObjProps			*props;
	char				*name;
	DIDEVCAPS			devcaps;
	LONG				deadzone;
	int				*axis_map;
	int				axes;
        POINTL                          povs[4];
};

static const GUID DInput_Wine_Joystick_GUID = { /* 9e573ed9-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed9,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

static void _dump_DIDEVCAPS(const DIDEVCAPS *lpDIDevCaps)
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

#define MAX_JOYSTICKS 64
static INT joystick_devices_count = -1;
static LPSTR joystick_devices[MAX_JOYSTICKS];

static INT find_joystick_devices(void)
{
    INT i;

    if (joystick_devices_count != -1) return joystick_devices_count;

    joystick_devices_count = 0;
    for (i = 0; i < MAX_JOYSTICKS; i++)
    {
        CHAR device_name[MAX_PATH], *str;
        INT len;
        int fd;

        len = sprintf(device_name, "%s%d", JOYDEV_NEW, i) + 1;
        if ((fd = open(device_name, O_RDONLY)) < 0)
        {
            len = sprintf(device_name, "%s%d", JOYDEV_OLD, i) + 1;
            if ((fd = open(device_name, O_RDONLY)) < 0) continue;
        }

        close(fd);

        if (!(str = HeapAlloc(GetProcessHeap(), 0, len))) break;
        memcpy(str, device_name, len);

        joystick_devices[joystick_devices_count++] = str;
    }

    return joystick_devices_count;
}

static BOOL joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    int fd = -1;

    if (id >= find_joystick_devices()) return FALSE;

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return FALSE;
    }

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {
        /* check whether we have a joystick */
        if ((fd = open(joystick_devices[id], O_RDONLY)) < 0)
        {
            WARN("open(%s, O_RDONLY) failed: %s\n", joystick_devices[id], strerror(errno));
            return FALSE;
        }

        /* Return joystick */
        lpddi->guidInstance = DInput_Wine_Joystick_GUID;
        lpddi->guidInstance.Data3 = id;
        lpddi->guidProduct = DInput_Wine_Joystick_GUID;
        /* we only support traditional joysticks for now */
        if (version >= 0x0800)
            lpddi->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
        else
            lpddi->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
        sprintf(lpddi->tszInstanceName, "Joystick %d", id);
#if defined(JSIOCGNAME)
        if (ioctl(fd,JSIOCGNAME(sizeof(lpddi->tszProductName)),lpddi->tszProductName) < 0) {
            WARN("ioctl(%s,JSIOCGNAME) failed: %s\n", joystick_devices[id], strerror(errno));
            strcpy(lpddi->tszProductName, "Wine Joystick");
        }
#else
        strcpy(lpddi->tszProductName, "Wine Joystick");
#endif

        lpddi->guidFFDriver = GUID_NULL;
        close(fd);
        TRACE("Enumerating the linux Joystick device: %s (%s)\n", joystick_devices[id], lpddi->tszProductName);
        return TRUE;
    }

    return FALSE;
}

static BOOL joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    int fd = -1;
    char name[MAX_PATH];
    char friendly[32];

    if (id >= find_joystick_devices()) return FALSE;

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return FALSE;
    }

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {
        /* check whether we have a joystick */
        if ((fd = open(joystick_devices[id], O_RDONLY)) < 0)
        {
            WARN("open(%s,O_RDONLY) failed: %s\n", joystick_devices[id], strerror(errno));
            return FALSE;
        }

        /* Return joystick */
        lpddi->guidInstance = DInput_Wine_Joystick_GUID;
        lpddi->guidInstance.Data3 = id;
        lpddi->guidProduct = DInput_Wine_Joystick_GUID;
        /* we only support traditional joysticks for now */
        if (version >= 0x0800)
            lpddi->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
        else
            lpddi->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
        sprintf(friendly, "Joystick %d", id);
        MultiByteToWideChar(CP_ACP, 0, friendly, -1, lpddi->tszInstanceName, MAX_PATH);
#if defined(JSIOCGNAME)
        if (ioctl(fd,JSIOCGNAME(sizeof(name)),name) < 0) {
            WARN("ioctl(%s, JSIOCGNAME) failed: %s\n", joystick_devices[id], strerror(errno));
            strcpy(name, "Wine Joystick");
        }
#else
        strcpy(name, "Wine Joystick");
#endif
        MultiByteToWideChar(CP_ACP, 0, name, -1, lpddi->tszProductName, MAX_PATH);
        lpddi->guidFFDriver = GUID_NULL;
        close(fd);
        TRACE("Enumerating the linux Joystick device: %s (%s)\n", joystick_devices[id], name);
        return TRUE;
    }

    return FALSE;
}

/*
 * Setup the dinput options.
 */

static HRESULT setup_dinput_options(JoystickImpl * device)
{
    char buffer[MAX_PATH+16];
    HKEY hkey, appkey;
    int tokens = 0;
    int axis = 0;
    int pov = 0;

    buffer[MAX_PATH]='\0';

    get_app_key(&hkey, &appkey);

    /* get options */

    if (!get_config_key( hkey, appkey, "DefaultDeadZone", buffer, MAX_PATH )) {
        device->deadzone = atoi(buffer);
        TRACE("setting default deadzone to: \"%s\" %d\n", buffer, device->deadzone);
    }

    device->axis_map = HeapAlloc(GetProcessHeap(), 0, device->axes * sizeof(int));
    if (!device->axis_map) return DIERR_OUTOFMEMORY;

    if (!get_config_key( hkey, appkey, device->name, buffer, MAX_PATH )) {
        static const char *axis_names[] = {"X", "Y", "Z", "Rx", "Ry", "Rz",
                                           "Slider1", "Slider2",
                                           "POV1", "POV2", "POV3", "POV4"};
        const char *delim = ",";
        char * ptr;
        TRACE("\"%s\" = \"%s\"\n", device->name, buffer);

        if ((ptr = strtok(buffer, delim)) != NULL) {
            do {
                int i;

                for (i = 0; i < sizeof(axis_names) / sizeof(axis_names[0]); i++)
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
                                device->axis_map[tokens++] = i;
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

                if (i == sizeof(axis_names) / sizeof(axis_names[0]))
                {
                    ERR("invalid joystick axis type: \"%s\"\n", ptr);
                    i = -1;
                }

                device->axis_map[tokens] = i;
                tokens++;
            } while ((ptr = strtok(NULL, delim)) != NULL);

            if (tokens != device->axes) {
                ERR("not all joystick axes mapped: %d axes(%d,%d), %d arguments\n", device->axes, axis, pov,tokens);
                while (tokens < device->axes) {
                    device->axis_map[tokens] = -1;
                    tokens++;
                }
            }
        }
    }
    else
    {
        for (tokens = 0; tokens < device->axes; tokens++)
        {
            if (tokens < 8)
                device->axis_map[tokens] = axis++;
            else if (tokens < 16)
            {
                device->axis_map[tokens++] = 8 + pov;
                device->axis_map[tokens  ] = 8 + pov++;
            }
            else
                device->axis_map[tokens] = -1;
        }
    }
    device->devcaps.dwAxes = axis;
    device->devcaps.dwPOVs = pov;

    if (appkey)
        RegCloseKey( appkey );

    if (hkey)
        RegCloseKey( hkey );

    return DI_OK;
}

static HRESULT alloc_device(REFGUID rguid, const void *jvt, IDirectInputImpl *dinput,
    LPDIRECTINPUTDEVICEA* pdev, unsigned short index)
{
    DWORD i;
    JoystickImpl* newDevice;
    char name[MAX_PATH];
    HRESULT hr;
    LPDIDATAFORMAT df = NULL;
    int idx = 0;

    TRACE("%s %p %p %p %hu\n", debugstr_guid(rguid), jvt, dinput, pdev, index);

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickImpl));
    if (newDevice == 0) {
        WARN("out of memory\n");
        *pdev = 0;
        return DIERR_OUTOFMEMORY;
    }

    if (!lstrcpynA(newDevice->dev, joystick_devices[index], sizeof(newDevice->dev)) ||
        (newDevice->joyfd = open(newDevice->dev, O_RDONLY)) < 0)
    {
        WARN("open(%s, O_RDONLY) failed: %s\n", newDevice->dev, strerror(errno));
        HeapFree(GetProcessHeap(), 0, newDevice);
        return DIERR_DEVICENOTREG;
    }

    /* get the device name */
#if defined(JSIOCGNAME)
    if (ioctl(newDevice->joyfd,JSIOCGNAME(MAX_PATH),name) < 0) {
        WARN("ioctl(%s,JSIOCGNAME) failed: %s\n", newDevice->dev, strerror(errno));
        strcpy(name, "Wine Joystick");
    }
#else
    strcpy(name, "Wine Joystick");
#endif

    /* copy the device name */
    newDevice->name = HeapAlloc(GetProcessHeap(),0,strlen(name) + 1);
    strcpy(newDevice->name, name);

#ifdef JSIOCGAXES
    if (ioctl(newDevice->joyfd,JSIOCGAXES,&newDevice->axes) < 0) {
        WARN("ioctl(%s,JSIOCGAXES) failed: %s, defauting to 2\n", newDevice->dev, strerror(errno));
        newDevice->axes = 2;
    }
#endif
#ifdef JSIOCGBUTTONS
    if (ioctl(newDevice->joyfd, JSIOCGBUTTONS, &newDevice->devcaps.dwButtons) < 0) {
        WARN("ioctl(%s,JSIOCGBUTTONS) failed: %s, defauting to 2\n", newDevice->dev, strerror(errno));
        newDevice->devcaps.dwButtons = 2;
    }
#endif

    if (newDevice->devcaps.dwButtons > 128)
    {
        WARN("Can't support %d buttons. Clamping down to 128\n", newDevice->devcaps.dwButtons);
        newDevice->devcaps.dwButtons = 128;
    }

    newDevice->base.lpVtbl = jvt;
    newDevice->base.ref = 1;
    newDevice->base.dinput = dinput;
    newDevice->base.guid = *rguid;
    InitializeCriticalSection(&newDevice->base.crit);
    newDevice->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JoystickImpl*->base.crit");

    /* setup_dinput_options may change these */
    newDevice->deadzone = 0;

    /* do any user specified configuration */
    hr = setup_dinput_options(newDevice);
    if (hr != DI_OK)
        goto FAILED1;

    /* Create copy of default data format */
    if (!(df = HeapAlloc(GetProcessHeap(), 0, c_dfDIJoystick2.dwSize))) goto FAILED;
    memcpy(df, &c_dfDIJoystick2, c_dfDIJoystick2.dwSize);

    df->dwNumObjs = newDevice->devcaps.dwAxes + newDevice->devcaps.dwPOVs + newDevice->devcaps.dwButtons;
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto FAILED;

    for (i = 0; i < newDevice->axes; i++)
    {
        int wine_obj = newDevice->axis_map[i];

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
    for (i = 0; i < newDevice->devcaps.dwButtons; i++)
    {
        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[i + 12], df->dwObjSize);
        df->rgodf[idx  ].pguid = &GUID_Button;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(i) | DIDFT_PSHBUTTON;
    }
    newDevice->base.data_format.wine_df = df;

    /* create default properties */
    newDevice->props = HeapAlloc(GetProcessHeap(),0,c_dfDIJoystick2.dwNumObjs*sizeof(ObjProps));
    if (newDevice->props == 0)
        goto FAILED;

    /* initialize default properties */
    for (i = 0; i < c_dfDIJoystick2.dwNumObjs; i++) {
        newDevice->props[i].lDevMin = -32767;
        newDevice->props[i].lDevMax = +32767;
        newDevice->props[i].lMin = 0;
        newDevice->props[i].lMax = 0xffff;
        newDevice->props[i].lDeadZone = newDevice->deadzone;	/* % * 1000 */
        newDevice->props[i].lSaturation = 0;
    }

    IDirectInput_AddRef((LPDIRECTINPUTDEVICE8A)newDevice->base.dinput);

    newDevice->devcaps.dwSize = sizeof(newDevice->devcaps);
    newDevice->devcaps.dwFlags = DIDC_ATTACHED;
    if (newDevice->base.dinput->dwVersion >= 0x0800)
        newDevice->devcaps.dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
        newDevice->devcaps.dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
    newDevice->devcaps.dwFFSamplePeriod = 0;
    newDevice->devcaps.dwFFMinTimeResolution = 0;
    newDevice->devcaps.dwFirmwareRevision = 0;
    newDevice->devcaps.dwHardwareRevision = 0;
    newDevice->devcaps.dwFFDriverVersion = 0;

    if (TRACE_ON(dinput)) {
        _dump_DIDATAFORMAT(newDevice->base.data_format.wine_df);
       for (i = 0; i < (newDevice->axes); i++)
           TRACE("axis_map[%d] = %d\n", i, newDevice->axis_map[i]);
        _dump_DIDEVCAPS(&newDevice->devcaps);
    }

    *pdev = (LPDIRECTINPUTDEVICEA)newDevice;

    return DI_OK;

FAILED:
    hr = DIERR_OUTOFMEMORY;
FAILED1:
    if (df) HeapFree(GetProcessHeap(), 0, df->rgodf);
    HeapFree(GetProcessHeap(), 0, df);
    release_DataFormat(&newDevice->base.data_format);
    HeapFree(GetProcessHeap(),0,newDevice->axis_map);
    HeapFree(GetProcessHeap(),0,newDevice->name);
    HeapFree(GetProcessHeap(),0,newDevice->props);
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

static HRESULT joydev_create_deviceA(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
    unsigned short index;

    TRACE("%p %s %p %p\n",dinput, debugstr_guid(rguid), riid, pdev);
    find_joystick_devices();
    *pdev = NULL;

    if ((index = get_joystick_index(rguid)) < MAX_JOYSTICKS &&
        joystick_devices_count && index < joystick_devices_count)
    {
        if ((riid == NULL) ||
	    IsEqualGUID(&IID_IDirectInputDeviceA,  riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice2A, riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice7A, riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice8A, riid))
        {
            return alloc_device(rguid, &JoystickAvt, dinput, pdev, index);
        }

        WARN("no interface\n");
        return DIERR_NOINTERFACE;
    }

    return DIERR_DEVICENOTREG;
}

static HRESULT joydev_create_deviceW(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev)
{
    unsigned short index;

    TRACE("%p %s %p %p\n",dinput, debugstr_guid(rguid), riid, pdev);
    find_joystick_devices();
    *pdev = NULL;

    if ((index = get_joystick_index(rguid)) < MAX_JOYSTICKS &&
        joystick_devices_count && index < joystick_devices_count)
    {
        if ((riid == NULL) ||
	    IsEqualGUID(&IID_IDirectInputDeviceW,  riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice2W, riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice7W, riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice8W, riid))
        {
            return alloc_device(rguid, &JoystickWvt, dinput, (LPDIRECTINPUTDEVICEA *)pdev, index);
        }
        WARN("no interface\n");
        return DIERR_NOINTERFACE;
    }

    WARN("invalid device GUID %s\n",debugstr_guid(rguid));
    return DIERR_DEVICENOTREG;
}

#undef MAX_JOYSTICKS

const struct dinput_device joystick_linux_device = {
  "Wine Linux joystick driver",
  joydev_enum_deviceA,
  joydev_enum_deviceW,
  joydev_create_deviceA,
  joydev_create_deviceW
};

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p)\n",This);

    if (This->base.acquired) {
        WARN("already acquired\n");
        return S_FALSE;
    }

    /* open the joystick device */
    if (This->joyfd==-1) {
        TRACE("opening joystick device %s\n", This->dev);

        This->joyfd=open(This->dev,O_RDONLY);
        if (This->joyfd==-1) {
            ERR("open(%s) failed: %s\n", This->dev, strerror(errno));
            return DIERR_NOTFOUND;
        }
    }

    This->base.acquired = 1;

    return DI_OK;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    HRESULT res;

    TRACE("(%p)\n",This);

    if ((res = IDirectInputDevice2AImpl_Unacquire(iface)) != DI_OK) return res;

    if (This->joyfd!=-1) {
        TRACE("closing joystick device\n");
        close(This->joyfd);
        This->joyfd = -1;
        return DI_OK;
    }

    return DI_NOEFFECT;
}

static void joy_polldev(JoystickImpl *This) {
    struct pollfd plfd;
    struct	js_event jse;
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
            if (jse.number >= This->devcaps.dwButtons) return;

            inst_id = DIDFT_MAKEINSTANCE(jse.number) | DIDFT_PSHBUTTON;
            This->js.rgbButtons[jse.number] = value = jse.value ? 0x80 : 0x00;
        }
        else if (jse.type & JS_EVENT_AXIS)
        {
            int number = This->axis_map[jse.number];	/* wine format object index */

            if (number < 0) return;
            inst_id = DIDFT_MAKEINSTANCE(number) | (number < 8 ? DIDFT_ABSAXIS : DIDFT_POV);
            value = joystick_map_axis(&This->props[id_to_object(This->base.data_format.wine_df, inst_id)], jse.value);

            TRACE("changing axis %d => %d\n", jse.number, number);
            switch (number)
            {
                case 0: This->js.lX  = value; break;
                case 1: This->js.lY  = value; break;
                case 2: This->js.lZ  = value; break;
                case 3: This->js.lRx = value; break;
                case 4: This->js.lRy = value; break;
                case 5: This->js.lRz = value; break;
                case 6: This->js.rglSlider[0] = value; break;
                case 7: This->js.rglSlider[1] = value; break;
                case 8: case 9: case 10: case 11:
                {
                    int idx = number - 8;

                    if (jse.number % 2)
                        This->povs[idx].y = jse.value;
                    else
                        This->povs[idx].x = jse.value;

                    This->js.rgdwPOV[idx] = value = joystick_map_pov(&This->povs[idx]);
                    break;
                }
                default:
                    WARN("axis %d not supported\n", number);
            }
        }
        if (inst_id >= 0)
            queue_event((LPDIRECTINPUTDEVICE8A)This,
                        id_to_offset(&This->base.data_format, inst_id),
                        value, jse.time, This->base.dinput->evsequence++);
    }
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the joystick.
  *
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceState(
    LPDIRECTINPUTDEVICE8A iface,
    DWORD len,
    LPVOID ptr)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p,0x%08x,%p)\n", This, len, ptr);

    if (!This->base.acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    /* update joystick state */
    joy_polldev(This);

    /* convert and copy data to user supplied buffer */
    fill_DataFormat(ptr, len, &This->js, &This->base.data_format);

    return DI_OK;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI JoystickAImpl_SetProperty(
    LPDIRECTINPUTDEVICE8A iface,
    REFGUID rguid,
    LPCDIPROPHEADER ph)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    DWORD i;

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(rguid),ph);

    if (ph == NULL) {
        WARN("invalid parameter: ph == NULL\n");
        return DIERR_INVALIDPARAM;
    }

    if (TRACE_ON(dinput))
        _dump_DIPROPHEADER(ph);

    if (!HIWORD(rguid)) {
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
            return IDirectInputDevice2AImpl_SetProperty(iface, rguid, ph);
        }
    }

    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
    JoystickImpl *This = (JoystickImpl *)iface;
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

static HRESULT WINAPI JoystickAImpl_Poll(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p)\n",This);

    if (!This->base.acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    joy_polldev(This);
    return DI_OK;
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI JoystickAImpl_GetProperty(
    LPDIRECTINPUTDEVICE8A iface,
    REFGUID rguid,
    LPDIPROPHEADER pdiph)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(rguid), pdiph);

    if (TRACE_ON(dinput))
        _dump_DIPROPHEADER(pdiph);

    if (!HIWORD(rguid)) {
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
        default:
            return IDirectInputDevice2AImpl_GetProperty(iface, rguid, pdiph);
        }
    }

    return DI_OK;
}

/******************************************************************************
  *     GetObjectInfo : get object info
  */
static HRESULT WINAPI JoystickWImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface,
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

static HRESULT WINAPI JoystickAImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8A iface,
        LPDIDEVICEOBJECTINSTANCEA pdidoi, DWORD dwObj, DWORD dwHow)
{
    HRESULT res;
    DIDEVICEOBJECTINSTANCEW didoiW;
    DWORD dwSize = pdidoi->dwSize;

    didoiW.dwSize = sizeof(didoiW);
    res = JoystickWImpl_GetObjectInfo((LPDIRECTINPUTDEVICE8W)iface, &didoiW, dwObj, dwHow);
    if (res != DI_OK) return res;

    memset(pdidoi, 0, pdidoi->dwSize);
    memcpy(pdidoi, &didoiW, FIELD_OFFSET(DIDEVICEOBJECTINSTANCEW, tszName));
    pdidoi->dwSize = dwSize;
    WideCharToMultiByte(CP_ACP, 0, didoiW.tszName, -1, pdidoi->tszName,
                        sizeof(pdidoi->tszName), NULL, NULL);

    return res;
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceInfo(
    LPDIRECTINPUTDEVICE8A iface,
    LPDIDEVICEINSTANCEA pdidi)
{
    JoystickImpl *This = (JoystickImpl *)iface;

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
    pdidi->guidInstance = GUID_Joystick;
    pdidi->guidProduct = DInput_Wine_Joystick_GUID;
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
static HRESULT WINAPI JoystickWImpl_GetDeviceInfo(
    LPDIRECTINPUTDEVICE8W iface,
    LPDIDEVICEINSTANCEW pdidi)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p,%p)\n", iface, pdidi);

    if ((pdidi->dwSize != sizeof(DIDEVICEINSTANCE_DX3W)) &&
        (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW))) {
        WARN("invalid parameter: pdidi->dwSize = %d\n", pdidi->dwSize);
        return DIERR_INVALIDPARAM;
    }

    /* Return joystick */
    pdidi->guidInstance = GUID_Joystick;
    pdidi->guidProduct = DInput_Wine_Joystick_GUID;
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

static const IDirectInputDevice8AVtbl JoystickAvt =
{
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
        IDirectInputDevice2AImpl_Release,
	JoystickAImpl_GetCapabilities,
        IDirectInputDevice2AImpl_EnumObjects,
	JoystickAImpl_GetProperty,
	JoystickAImpl_SetProperty,
	JoystickAImpl_Acquire,
	JoystickAImpl_Unacquire,
	JoystickAImpl_GetDeviceState,
	IDirectInputDevice2AImpl_GetDeviceData,
	IDirectInputDevice2AImpl_SetDataFormat,
	IDirectInputDevice2AImpl_SetEventNotification,
	IDirectInputDevice2AImpl_SetCooperativeLevel,
	JoystickAImpl_GetObjectInfo,
	JoystickAImpl_GetDeviceInfo,
	IDirectInputDevice2AImpl_RunControlPanel,
	IDirectInputDevice2AImpl_Initialize,
	IDirectInputDevice2AImpl_CreateEffect,
	IDirectInputDevice2AImpl_EnumEffects,
	IDirectInputDevice2AImpl_GetEffectInfo,
	IDirectInputDevice2AImpl_GetForceFeedbackState,
	IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	IDirectInputDevice2AImpl_Escape,
	JoystickAImpl_Poll,
	IDirectInputDevice2AImpl_SendDeviceData,
	IDirectInputDevice7AImpl_EnumEffectsInFile,
	IDirectInputDevice7AImpl_WriteEffectToFile,
	IDirectInputDevice8AImpl_BuildActionMap,
	IDirectInputDevice8AImpl_SetActionMap,
	IDirectInputDevice8AImpl_GetImageInfo
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(JoystickWvt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static const IDirectInputDevice8WVtbl JoystickWvt =
{
	IDirectInputDevice2WImpl_QueryInterface,
	XCAST(AddRef)IDirectInputDevice2AImpl_AddRef,
        XCAST(Release)IDirectInputDevice2AImpl_Release,
	XCAST(GetCapabilities)JoystickAImpl_GetCapabilities,
        IDirectInputDevice2WImpl_EnumObjects,
	XCAST(GetProperty)JoystickAImpl_GetProperty,
	XCAST(SetProperty)JoystickAImpl_SetProperty,
	XCAST(Acquire)JoystickAImpl_Acquire,
	XCAST(Unacquire)JoystickAImpl_Unacquire,
	XCAST(GetDeviceState)JoystickAImpl_GetDeviceState,
	XCAST(GetDeviceData)IDirectInputDevice2AImpl_GetDeviceData,
	XCAST(SetDataFormat)IDirectInputDevice2AImpl_SetDataFormat,
	XCAST(SetEventNotification)IDirectInputDevice2AImpl_SetEventNotification,
	XCAST(SetCooperativeLevel)IDirectInputDevice2AImpl_SetCooperativeLevel,
        JoystickWImpl_GetObjectInfo,
	JoystickWImpl_GetDeviceInfo,
	XCAST(RunControlPanel)IDirectInputDevice2AImpl_RunControlPanel,
	XCAST(Initialize)IDirectInputDevice2AImpl_Initialize,
	XCAST(CreateEffect)IDirectInputDevice2AImpl_CreateEffect,
	IDirectInputDevice2WImpl_EnumEffects,
	IDirectInputDevice2WImpl_GetEffectInfo,
	XCAST(GetForceFeedbackState)IDirectInputDevice2AImpl_GetForceFeedbackState,
	XCAST(SendForceFeedbackCommand)IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	XCAST(EnumCreatedEffectObjects)IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	XCAST(Escape)IDirectInputDevice2AImpl_Escape,
	XCAST(Poll)JoystickAImpl_Poll,
	XCAST(SendDeviceData)IDirectInputDevice2AImpl_SendDeviceData,
        IDirectInputDevice7WImpl_EnumEffectsInFile,
        IDirectInputDevice7WImpl_WriteEffectToFile,
        IDirectInputDevice8WImpl_BuildActionMap,
        IDirectInputDevice8WImpl_SetActionMap,
        IDirectInputDevice8WImpl_GetImageInfo
};
#undef XCAST

#else  /* HAVE_LINUX_22_JOYSTICK_API */

const struct dinput_device joystick_linux_device = {
  "Wine Linux joystick driver",
  NULL,
  NULL,
  NULL,
  NULL
};

#endif  /* HAVE_LINUX_22_JOYSTICK_API */
