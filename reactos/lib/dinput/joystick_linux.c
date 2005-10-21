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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/fcntl.h>
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

#define JOYDEV "/dev/js"

typedef struct {
    LONG lMin;
    LONG lMax;
    LONG lDeadZone;
    LONG lSaturation;
} ObjProps;

typedef struct {
    LONG lX;
    LONG lY;
} POV;

typedef struct JoystickImpl JoystickImpl;
static const IDirectInputDevice8AVtbl JoystickAvt;
static const IDirectInputDevice8WVtbl JoystickWvt;
struct JoystickImpl
{
        const void                     *lpVtbl;
        LONG                            ref;
        GUID                            guid;
	char				dev[32];

	/* The 'parent' DInput */
	IDirectInputImpl               *dinput;

	/* joystick private */
	int				joyfd;
	DIJOYSTATE2			js;		/* wine data */
	LPDIDATAFORMAT			user_df;	/* user defined format */
	DataFormat			*transform;	/* wine to user format converter */
	int				*offsets;	/* object offsets */
	ObjProps			*props;
        HANDLE				hEvent;
        LPDIDEVICEOBJECTDATA 		data_queue;
        int				queue_head, queue_tail, queue_len;
	BOOL				acquired;
	char				*name;
	DIDEVCAPS			devcaps;
	LONG				deadzone;
	int				*axis_map;
	int				axes;
	int				buttons;
	POV				povs[4];
	CRITICAL_SECTION		crit;
	BOOL				overflow;
};

static GUID DInput_Wine_Joystick_GUID = { /* 9e573ed9-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed9,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

static void _dump_DIDEVCAPS(LPDIDEVCAPS lpDIDevCaps)
{
    TRACE("dwSize: %ld\n", lpDIDevCaps->dwSize);
    TRACE("dwFlags: %08lx\n",lpDIDevCaps->dwFlags);
    TRACE("dwDevType: %08lx %s\n", lpDIDevCaps->dwDevType,
          lpDIDevCaps->dwDevType == DIDEVTYPE_DEVICE ? "DIDEVTYPE_DEVICE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_DEVICE ? "DIDEVTYPE_DEVICE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_MOUSE ? "DIDEVTYPE_MOUSE" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_KEYBOARD ? "DIDEVTYPE_KEYBOARD" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_JOYSTICK ? "DIDEVTYPE_JOYSTICK" :
          lpDIDevCaps->dwDevType == DIDEVTYPE_HID ? "DIDEVTYPE_HID" : "UNKNOWN");
    TRACE("dwAxes: %ld\n",lpDIDevCaps->dwAxes);
    TRACE("dwButtons: %ld\n",lpDIDevCaps->dwButtons);
    TRACE("dwPOVs: %ld\n",lpDIDevCaps->dwPOVs);
    if (lpDIDevCaps->dwSize > sizeof(DIDEVCAPS_DX3)) {
        TRACE("dwFFSamplePeriod: %ld\n",lpDIDevCaps->dwFFSamplePeriod);
        TRACE("dwFFMinTimeResolution: %ld\n",lpDIDevCaps->dwFFMinTimeResolution);
        TRACE("dwFirmwareRevision: %ld\n",lpDIDevCaps->dwFirmwareRevision);
        TRACE("dwHardwareRevision: %ld\n",lpDIDevCaps->dwHardwareRevision);
        TRACE("dwFFDriverVersion: %ld\n",lpDIDevCaps->dwFFDriverVersion);
    }
}

static BOOL joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    int fd = -1;
    char dev[32];

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return FALSE;
    }

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {
        /* check whether we have a joystick */
        sprintf(dev, "%s%d", JOYDEV, id);
        if ((fd = open(dev,O_RDONLY)) < 0) {
            WARN("open(%s,O_RDONLY) failed: %s\n", dev, strerror(errno));
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
            WARN("ioctl(%s,JSIOCGNAME) failed: %s\n", dev, strerror(errno));
            strcpy(lpddi->tszProductName, "Wine Joystick");
        }
#else
        strcpy(lpddi->tszProductName, "Wine Joystick");
#endif

        lpddi->guidFFDriver = GUID_NULL;
        close(fd);
        TRACE("Enumerating the linux Joystick device: %s (%s)\n", dev, lpddi->tszProductName);
        return TRUE;
    }

    return FALSE;
}

static BOOL joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    int fd = -1;
    char name[MAX_PATH];
    char dev[32];
    char friendly[32];

    if (dwFlags & DIEDFL_FORCEFEEDBACK) {
        WARN("force feedback not supported\n");
        return FALSE;
    }

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))) {
        /* check whether we have a joystick */
        sprintf(dev, "%s%d", JOYDEV, id);
        if ((fd = open(dev,O_RDONLY)) < 0) {
            WARN("open(%s,O_RDONLY) failed: %s\n", dev, strerror(errno));
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
            WARN("ioctl(%s,JSIOCGNAME) failed: %s\n", dev, strerror(errno));
            strcpy(name, "Wine Joystick");
        }
#else
        strcpy(name, "Wine Joystick");
#endif
        MultiByteToWideChar(CP_ACP, 0, name, -1, lpddi->tszProductName, MAX_PATH);
        lpddi->guidFFDriver = GUID_NULL;
        close(fd);
        TRACE("Enumerating the linux Joystick device: %s (%s)\n",dev,name);
        return TRUE;
    }

    return FALSE;
}

/*
 * Get a config key from either the app-specific or the default config
 */

inline static DWORD get_config_key( HKEY defkey, HKEY appkey, const char *name,
                                    char *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE)buffer, &size ))
        return 0;

    if (defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE)buffer, &size ))
        return 0;

    return ERROR_FILE_NOT_FOUND;
}

/*
 * Setup the dinput options.
 */

static HRESULT setup_dinput_options(JoystickImpl * device)
{
    char buffer[MAX_PATH+16];
    HKEY hkey, appkey = 0;
    DWORD len;

    buffer[MAX_PATH]='\0';

    /* @@ Wine registry key: HKCU\Software\Wine\DirectInput */
    if (RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\DirectInput", &hkey)) hkey = 0;

    len = GetModuleFileNameA( 0, buffer, MAX_PATH );
    if (len && len < MAX_PATH) {
        HKEY tmpkey;
        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\DirectInput */
        if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey ))
        {
            char *p, *appname = buffer;
            if ((p = strrchr( appname, '/' ))) appname = p + 1;
            if ((p = strrchr( appname, '\\' ))) appname = p + 1;
            strcat( appname, "\\DirectInput" );
            if (RegOpenKeyA( tmpkey, appname, &appkey )) appkey = 0;
            RegCloseKey( tmpkey );
        }
    }

    /* get options */

    if (!get_config_key( hkey, appkey, "DefaultDeadZone", buffer, MAX_PATH )) {
        device->deadzone = atoi(buffer);
        TRACE("setting default deadzone to: \"%s\" %ld\n", buffer, device->deadzone);
    }

    if (!get_config_key( hkey, appkey, device->name, buffer, MAX_PATH )) {
        int tokens = 0;
        int axis = 0;
        int pov = 0;
        const char *delim = ",";
        char * ptr;
        TRACE("\"%s\" = \"%s\"\n", device->name, buffer);

        device->axis_map = HeapAlloc(GetProcessHeap(), 0, device->axes * sizeof(int));
        if (device->axis_map == 0)
            return DIERR_OUTOFMEMORY;

        if ((ptr = strtok(buffer, delim)) != NULL) {
            do {
                if (strcmp(ptr, "X") == 0) {
                    device->axis_map[tokens] = 0;
                    axis++;
                } else if (strcmp(ptr, "Y") == 0) {
                    device->axis_map[tokens] = 1;
                    axis++;
                } else if (strcmp(ptr, "Z") == 0) {
                    device->axis_map[tokens] = 2;
                    axis++;
                } else if (strcmp(ptr, "Rx") == 0) {
                    device->axis_map[tokens] = 3;
                    axis++;
                } else if (strcmp(ptr, "Ry") == 0) {
                    device->axis_map[tokens] = 4;
                    axis++;
                } else if (strcmp(ptr, "Rz") == 0) {
                    device->axis_map[tokens] = 5;
                    axis++;
                } else if (strcmp(ptr, "Slider1") == 0) {
                    device->axis_map[tokens] = 6;
                    axis++;
                } else if (strcmp(ptr, "Slider2") == 0) {
                    device->axis_map[tokens] = 7;
                    axis++;
                } else if (strcmp(ptr, "POV1") == 0) {
                    device->axis_map[tokens++] = 8;
                    device->axis_map[tokens] = 8;
                    pov++;
                } else if (strcmp(ptr, "POV2") == 0) {
                    device->axis_map[tokens++] = 9;
                    device->axis_map[tokens] = 9;
                    pov++;
                } else if (strcmp(ptr, "POV3") == 0) {
                    device->axis_map[tokens++] = 10;
                    device->axis_map[tokens] = 10;
                    pov++;
                } else if (strcmp(ptr, "POV4") == 0) {
                    device->axis_map[tokens++] = 11;
                    device->axis_map[tokens] = 11;
                    pov++;
                } else {
                    ERR("invalid joystick axis type: %s\n", ptr);
                    device->axis_map[tokens] = tokens;
                    axis++;
                }

                tokens++;
            } while ((ptr = strtok(NULL, delim)) != NULL);

            if (tokens != device->devcaps.dwAxes) {
                ERR("not all joystick axes mapped: %d axes(%d,%d), %d arguments\n", device->axes, axis, pov,tokens);
                while (tokens < device->axes) {
                    device->axis_map[tokens] = tokens;
                    tokens++;
                }
            }
        }

        device->devcaps.dwAxes = axis;
        device->devcaps.dwPOVs = pov;
    }

    if (appkey)
        RegCloseKey( appkey );

    if (hkey)
        RegCloseKey( hkey );

    return DI_OK;
}

static void calculate_ids(JoystickImpl* device)
{
    int i;
    int axis = 0;
    int button = 0;
    int pov = 0;
    int axis_base;
    int pov_base;
    int button_base;

    /* Make two passes over the format. The first counts the number
     * for each type and the second sets the id */
    for (i = 0; i < device->user_df->dwNumObjs; i++) {
        if (DIDFT_GETTYPE(device->user_df->rgodf[i].dwType) & DIDFT_AXIS)
            axis++;
        else if (DIDFT_GETTYPE(device->user_df->rgodf[i].dwType) & DIDFT_POV)
            pov++;
        else if (DIDFT_GETTYPE(device->user_df->rgodf[i].dwType) & DIDFT_BUTTON)
            button++;
    }

    axis_base = 0;
    pov_base = axis;
    button_base = axis + pov;

    axis = 0;
    button = 0;
    pov = 0;

    for (i = 0; i < device->user_df->dwNumObjs; i++) {
        DWORD type = 0;
        if (DIDFT_GETTYPE(device->user_df->rgodf[i].dwType) & DIDFT_AXIS) {
            axis++;
            type = DIDFT_GETTYPE(device->user_df->rgodf[i].dwType) |
                DIDFT_MAKEINSTANCE(axis + axis_base);
            TRACE("axis type = 0x%08lx\n", type);
        } else if (DIDFT_GETTYPE(device->user_df->rgodf[i].dwType) & DIDFT_POV) {
            pov++;
            type = DIDFT_GETTYPE(device->user_df->rgodf[i].dwType) |
                DIDFT_MAKEINSTANCE(pov + pov_base);
            TRACE("POV type = 0x%08lx\n", type);
        } else if (DIDFT_GETTYPE(device->user_df->rgodf[i].dwType) & DIDFT_BUTTON) {
            button++;
            type = DIDFT_GETTYPE(device->user_df->rgodf[i].dwType) |
                DIDFT_MAKEINSTANCE(button + button_base);
            TRACE("button type = 0x%08lx\n", type);
        }
        device->user_df->rgodf[i].dwType = type;
    }
}

static HRESULT alloc_device(REFGUID rguid, const void *jvt, IDirectInputImpl *dinput, LPDIRECTINPUTDEVICEA* pdev)
{
    DWORD i;
    JoystickImpl* newDevice;
    char name[MAX_PATH];
    HRESULT hr;

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickImpl));
    if (newDevice == 0) {
        WARN("out of memory\n");
        *pdev = 0;
        return DIERR_OUTOFMEMORY;
    }

    sprintf(newDevice->dev, "%s%d", JOYDEV, rguid->Data3);

    if ((newDevice->joyfd = open(newDevice->dev,O_RDONLY)) < 0) {
        WARN("open(%s,O_RDONLY) failed: %s\n", newDevice->dev, strerror(errno));
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
    if (ioctl(newDevice->joyfd,JSIOCGBUTTONS,&newDevice->buttons) < 0) {
        WARN("ioctl(%s,JSIOCGBUTTONS) failed: %s, defauting to 2\n", newDevice->dev, strerror(errno));
        newDevice->buttons = 2;
    }
#endif

    newDevice->lpVtbl = jvt;
    newDevice->ref = 1;
    newDevice->dinput = dinput;
    newDevice->acquired = FALSE;
    newDevice->overflow = FALSE;
    CopyMemory(&(newDevice->guid),rguid,sizeof(*rguid));

    /* setup_dinput_options may change these */
    newDevice->deadzone = 5000;
    newDevice->devcaps.dwButtons = newDevice->buttons;
    newDevice->devcaps.dwAxes = newDevice->axes;
    newDevice->devcaps.dwPOVs = 0;

    /* do any user specified configuration */
    hr = setup_dinput_options(newDevice);
    if (hr != DI_OK)
        goto FAILED1;

    if (newDevice->axis_map == 0) {
        newDevice->axis_map = HeapAlloc(GetProcessHeap(), 0, newDevice->axes * sizeof(int));
        if (newDevice->axis_map == 0)
            goto FAILED;

        for (i = 0; i < newDevice->axes; i++)
            newDevice->axis_map[i] = i;
    }

    /* wine uses DIJOYSTATE2 as it's internal format so copy
     * the already defined format c_dfDIJoystick2 */
    newDevice->user_df = HeapAlloc(GetProcessHeap(),0,c_dfDIJoystick2.dwSize);
    if (newDevice->user_df == 0)
        goto FAILED;

    CopyMemory(newDevice->user_df, &c_dfDIJoystick2, c_dfDIJoystick2.dwSize);

    /* copy default objects */
    newDevice->user_df->rgodf = HeapAlloc(GetProcessHeap(),0,c_dfDIJoystick2.dwNumObjs*c_dfDIJoystick2.dwObjSize);
    if (newDevice->user_df->rgodf == 0)
        goto FAILED;

    CopyMemory(newDevice->user_df->rgodf,c_dfDIJoystick2.rgodf,c_dfDIJoystick2.dwNumObjs*c_dfDIJoystick2.dwObjSize);

    /* create default properties */
    newDevice->props = HeapAlloc(GetProcessHeap(),0,c_dfDIJoystick2.dwNumObjs*sizeof(ObjProps));
    if (newDevice->props == 0)
        goto FAILED;

    /* initialize default properties */
    for (i = 0; i < c_dfDIJoystick2.dwNumObjs; i++) {
        newDevice->props[i].lMin = 0;
        newDevice->props[i].lMax = 0xffff;
        newDevice->props[i].lDeadZone = newDevice->deadzone;	/* % * 1000 */
        newDevice->props[i].lSaturation = 0;
    }

    /* create an offsets array */
    newDevice->offsets = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,c_dfDIJoystick2.dwNumObjs*sizeof(int));
    if (newDevice->offsets == 0)
        goto FAILED;

    /* create the default transform filter */
    newDevice->transform = create_DataFormat(&c_dfDIJoystick2, newDevice->user_df, newDevice->offsets);

    calculate_ids(newDevice);

    IDirectInputDevice_AddRef((LPDIRECTINPUTDEVICE8A)newDevice->dinput);
    InitializeCriticalSection(&(newDevice->crit));
    newDevice->crit.DebugInfo->Spare[0] = (DWORD_PTR)"DINPUT_Mouse";

    newDevice->devcaps.dwSize = sizeof(newDevice->devcaps);
    newDevice->devcaps.dwFlags = DIDC_ATTACHED;
    if (newDevice->dinput->dwVersion >= 0x0800)
        newDevice->devcaps.dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
        newDevice->devcaps.dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);
    newDevice->devcaps.dwFFSamplePeriod = 0;
    newDevice->devcaps.dwFFMinTimeResolution = 0;
    newDevice->devcaps.dwFirmwareRevision = 0;
    newDevice->devcaps.dwHardwareRevision = 0;
    newDevice->devcaps.dwFFDriverVersion = 0;

    if (TRACE_ON(dinput)) {
        _dump_DIDATAFORMAT(newDevice->user_df);
       for (i = 0; i < (newDevice->axes); i++)
           TRACE("axis_map[%ld] = %d\n", i, newDevice->axis_map[i]);
        _dump_DIDEVCAPS(&newDevice->devcaps);
    }

    *pdev = (LPDIRECTINPUTDEVICEA)newDevice;

    return DI_OK;

FAILED:
    hr = DIERR_OUTOFMEMORY;
FAILED1:
    HeapFree(GetProcessHeap(),0,newDevice->axis_map);
    HeapFree(GetProcessHeap(),0,newDevice->name);
    HeapFree(GetProcessHeap(),0,newDevice->props);
    HeapFree(GetProcessHeap(),0,newDevice->user_df->rgodf);
    HeapFree(GetProcessHeap(),0,newDevice->user_df);
    HeapFree(GetProcessHeap(),0,newDevice);
    *pdev = 0;

    return hr;
}

static BOOL IsJoystickGUID(REFGUID guid)
{
    GUID wine_joystick = DInput_Wine_Joystick_GUID;
    GUID dev_guid = *guid;

    wine_joystick.Data3 = 0;
    dev_guid.Data3 = 0;

    return IsEqualGUID(&wine_joystick, &dev_guid);
}

static HRESULT joydev_create_deviceA(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
  if ((IsEqualGUID(&GUID_Joystick,rguid)) ||
      (IsJoystickGUID(rguid))) {
    if ((riid == NULL) ||
	IsEqualGUID(&IID_IDirectInputDeviceA,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice2A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice7A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice8A,riid)) {
      return alloc_device(rguid, &JoystickAvt, dinput, pdev);
    } else {
      WARN("no interface\n");
      *pdev = 0;
      return DIERR_NOINTERFACE;
    }
  }

  WARN("invalid device GUID\n");
  *pdev = 0;
  return DIERR_DEVICENOTREG;
}

static HRESULT joydev_create_deviceW(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev)
{
  if ((IsEqualGUID(&GUID_Joystick,rguid)) ||
      (IsJoystickGUID(rguid))) {
    if ((riid == NULL) ||
	IsEqualGUID(&IID_IDirectInputDeviceW,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice2W,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice7W,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice8W,riid)) {
      return alloc_device(rguid, &JoystickWvt, dinput, (LPDIRECTINPUTDEVICEA *)pdev);
    } else {
      WARN("no interface\n");
      *pdev = 0;
      return DIERR_NOINTERFACE;
    }
  }

  WARN("invalid device GUID\n");
  *pdev = 0;
  return DIERR_DEVICENOTREG;
}

const struct dinput_device joystick_linux_device = {
  "Wine Linux joystick driver",
  joydev_enum_deviceA,
  joydev_enum_deviceW,
  joydev_create_deviceA,
  joydev_create_deviceW
};

/******************************************************************************
 *	Joystick
 */
static ULONG WINAPI JoystickAImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    ULONG ref;

    ref = InterlockedDecrement((&This->ref));
    if (ref)
        return ref;

    /* Free the device name */
    HeapFree(GetProcessHeap(),0,This->name);

    /* Free the axis map */
    HeapFree(GetProcessHeap(),0,This->axis_map);

    /* Free the data queue */
    HeapFree(GetProcessHeap(),0,This->data_queue);

    /* Free the DataFormat */
    HeapFree(GetProcessHeap(), 0, This->user_df->rgodf);
    HeapFree(GetProcessHeap(), 0, This->user_df);

    /* Free the properties */
    HeapFree(GetProcessHeap(), 0, This->props);

    /* Free the offsets array */
    HeapFree(GetProcessHeap(),0,This->offsets);

    /* release the data transform filter */
    release_DataFormat(This->transform);

    This->crit.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&(This->crit));
    IDirectInputDevice_Release((LPDIRECTINPUTDEVICE8A)This->dinput);

    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

/******************************************************************************
  *   SetDataFormat : the application can choose the format of the data
  *   the device driver sends back with GetDeviceState.
  */
static HRESULT WINAPI JoystickAImpl_SetDataFormat(
    LPDIRECTINPUTDEVICE8A iface,
    LPCDIDATAFORMAT df)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    unsigned int i;
    LPDIDATAFORMAT new_df = 0;
    LPDIOBJECTDATAFORMAT new_rgodf = 0;
    ObjProps * new_props = 0;

    TRACE("(%p,%p)\n",This,df);

    if (df == NULL) {
        WARN("invalid pointer\n");
        return E_POINTER;
    }

    if (df->dwSize != sizeof(*df)) {
        WARN("invalid argument\n");
        return DIERR_INVALIDPARAM;
    }

    if (This->acquired) {
        WARN("acquired\n");
        return DIERR_ACQUIRED;
    }

    if (TRACE_ON(dinput))
        _dump_DIDATAFORMAT(df);

    /* Store the new data format */
    new_df = HeapAlloc(GetProcessHeap(),0,df->dwSize);
    if (new_df == 0)
        goto FAILED;

    new_rgodf = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*df->dwObjSize);
    if (new_rgodf == 0)
        goto FAILED;

    new_props = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*sizeof(ObjProps));
    if (new_props == 0)
        goto FAILED;

    HeapFree(GetProcessHeap(),0,This->user_df);
    HeapFree(GetProcessHeap(),0,This->user_df->rgodf);
    HeapFree(GetProcessHeap(),0,This->props);
    release_DataFormat(This->transform);

    This->user_df = new_df;
    CopyMemory(This->user_df, df, df->dwSize);
    This->user_df->rgodf = new_rgodf;
    CopyMemory(This->user_df->rgodf,df->rgodf,df->dwNumObjs*df->dwObjSize);
    This->props = new_props;
    for (i = 0; i < df->dwNumObjs; i++) {
        This->props[i].lMin = 0;
        This->props[i].lMax = 0xffff;
        This->props[i].lDeadZone = 1000;
        This->props[i].lSaturation = 0;
    }
    This->transform = create_DataFormat(&c_dfDIJoystick2, This->user_df, This->offsets);

    calculate_ids(This);

    return DI_OK;

FAILED:
    WARN("out of memory\n");
    HeapFree(GetProcessHeap(),0,new_props);
    HeapFree(GetProcessHeap(),0,new_rgodf);
    HeapFree(GetProcessHeap(),0,new_df);
    return DIERR_OUTOFMEMORY;
}

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p)\n",This);

    if (This->acquired) {
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

    This->acquired = TRUE;

    return DI_OK;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p)\n",This);

    if (!This->acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    if (This->joyfd!=-1) {
        TRACE("closing joystick device\n");
        close(This->joyfd);
        This->joyfd = -1;
        This->acquired = FALSE;
        return DI_OK;
    }

    This->acquired = FALSE;

    return DI_NOEFFECT;
}

static LONG map_axis(JoystickImpl * This, short val, short index)
{
    double    fval = val;
    double    fmin = This->props[index].lMin;
    double    fmax = This->props[index].lMax;
    double    fret;

    fret = (((fval + 32767.0) * (fmax - fmin)) / (32767.0*2.0)) + fmin;

    if (fret >= 0.0)
        fret += 0.5;
    else
        fret -= 0.5;

    return fret;
}

/* convert wine format offset to user format object index */
static int offset_to_object(JoystickImpl *This, int offset)
{
    int i;

    for (i = 0; i < This->user_df->dwNumObjs; i++) {
        if (This->user_df->rgodf[i].dwOfs == offset)
            return i;
    }

    return -1;
}

static LONG calculate_pov(JoystickImpl *This, int index)
{
    if (This->povs[index].lX < 16384) {
        if (This->povs[index].lY < 16384)
            This->js.rgdwPOV[index] = 31500;
        else if (This->povs[index].lY > 49150)
            This->js.rgdwPOV[index] = 22500;
        else
            This->js.rgdwPOV[index] = 27000;
    } else if (This->povs[index].lX > 49150) {
        if (This->povs[index].lY < 16384)
            This->js.rgdwPOV[index] = 4500;
        else if (This->povs[index].lY > 49150)
            This->js.rgdwPOV[index] = 13500;
        else
            This->js.rgdwPOV[index] = 9000;
    } else {
        if (This->povs[index].lY < 16384)
            This->js.rgdwPOV[index] = 0;
        else if (This->povs[index].lY > 49150)
            This->js.rgdwPOV[index] = 18000;
        else
            This->js.rgdwPOV[index] = -1;
    }

    return This->js.rgdwPOV[index];
}

static void joy_polldev(JoystickImpl *This) {
    struct timeval tv;
    fd_set	readfds;
    struct	js_event jse;
    TRACE("(%p)\n", This);

    if (This->joyfd==-1) {
        WARN("no device\n");
        return;
    }
    while (1) {
	memset(&tv,0,sizeof(tv));
	FD_ZERO(&readfds);FD_SET(This->joyfd,&readfds);
	if (1>select(This->joyfd+1,&readfds,NULL,NULL,&tv))
	    return;
	/* we have one event, so we can read */
	if (sizeof(jse)!=read(This->joyfd,&jse,sizeof(jse))) {
	    return;
	}
        TRACE("js_event: type 0x%x, number %d, value %d\n",
              jse.type,jse.number,jse.value);
        if (jse.type & JS_EVENT_BUTTON) {
            int offset = This->offsets[jse.number + 12];
            int value = jse.value?0x80:0x00;

            This->js.rgbButtons[jse.number] = value;
            GEN_EVENT(offset,value,jse.time,(This->dinput->evsequence)++);
        } else if (jse.type & JS_EVENT_AXIS) {
            int number = This->axis_map[jse.number];	/* wine format object index */
            if (number < 12) {
                int offset = This->offsets[number];
                int index = offset_to_object(This, offset);
                LONG value = map_axis(This, jse.value, index);

                /* FIXME do deadzone and saturation here */

                TRACE("changing axis %d => %d\n", jse.number, number);
                switch (number) {
                case 0:
                    This->js.lX = value;
                    break;
                case 1:
                    This->js.lY = value;
                    break;
                case 2:
                    This->js.lZ = value;
                    break;
                case 3:
                    This->js.lRx = value;
                    break;
                case 4:
                    This->js.lRy = value;
                    break;
                case 5:
                    This->js.lRz = value;
                    break;
                case 6:
                    This->js.rglSlider[0] = value;
                    break;
                case 7:
                    This->js.rglSlider[1] = value;
                    break;
                case 8:
                    /* FIXME don't go off array */
                    if (This->axis_map[jse.number + 1] == number)
                        This->povs[0].lX = value;
                    else if (This->axis_map[jse.number - 1] == number)
                        This->povs[0].lY = value;
                    value = calculate_pov(This, 0);
                    break;
                case 9:
                    if (This->axis_map[jse.number + 1] == number)
                        This->povs[1].lX = value;
                    else if (This->axis_map[jse.number - 1] == number)
                        This->povs[1].lY = value;
                    value = calculate_pov(This, 1);
                    break;
                case 10:
                    if (This->axis_map[jse.number + 1] == number)
                        This->povs[2].lX = value;
                    else if (This->axis_map[jse.number - 1] == number)
                        This->povs[2].lY = value;
                    value = calculate_pov(This, 2);
                    break;
                case 11:
                    if (This->axis_map[jse.number + 1] == number)
                        This->povs[3].lX = value;
                    else if (This->axis_map[jse.number - 1] == number)
                        This->povs[3].lY = value;
                    value = calculate_pov(This, 3);
                    break;
                }

                GEN_EVENT(offset,value,jse.time,(This->dinput->evsequence)++);
            } else
                WARN("axis %d not supported\n", number);
        }
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

    TRACE("(%p,0x%08lx,%p)\n",This,len,ptr);

    if (!This->acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    /* update joystick state */
    joy_polldev(This);

    /* convert and copy data to user supplied buffer */
    fill_DataFormat(ptr, &This->js, This->transform);

    return DI_OK;
}

/******************************************************************************
  *     GetDeviceData : gets buffered input data.
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceData(
    LPDIRECTINPUTDEVICE8A iface,
    DWORD dodsize,
    LPDIDEVICEOBJECTDATA dod,
    LPDWORD entries,
    DWORD flags)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    DWORD len;
    int nqtail;
    HRESULT hr = DI_OK;

    TRACE("(%p)->(dods=%ld,entries=%ld,fl=0x%08lx)\n",This,dodsize,*entries,flags);

    if (!This->acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    EnterCriticalSection(&(This->crit));

    joy_polldev(This);

    len = ((This->queue_head < This->queue_tail) ? This->queue_len : 0)
        + (This->queue_head - This->queue_tail);
    if (len > *entries)
        len = *entries;

    if (dod == NULL) {
        if (len)
            TRACE("Application discarding %ld event(s).\n", len);

        *entries = len;
        nqtail = This->queue_tail + len;
        while (nqtail >= This->queue_len)
            nqtail -= This->queue_len;
    } else {
        if (dodsize < sizeof(DIDEVICEOBJECTDATA_DX3)) {
            ERR("Wrong structure size !\n");
            LeaveCriticalSection(&(This->crit));
            return DIERR_INVALIDPARAM;
        }

        if (len)
            TRACE("Application retrieving %ld event(s).\n", len);

        *entries = 0;
        nqtail = This->queue_tail;
        while (len) {
            /* Copy the buffered data into the application queue */
            memcpy((char *)dod + *entries * dodsize, This->data_queue + nqtail, dodsize);
            /* Advance position */
            nqtail++;
            if (nqtail >= This->queue_len)
                nqtail -= This->queue_len;
            (*entries)++;
            len--;
        }
    }

    if (This->overflow) {
        hr = DI_BUFFEROVERFLOW;
        if (!(flags & DIGDD_PEEK)) {
            This->overflow = FALSE;
        }
    }

    if (!(flags & DIGDD_PEEK))
        This->queue_tail = nqtail;

    LeaveCriticalSection(&(This->crit));

    return hr;
}

static int find_property(JoystickImpl * This, LPCDIPROPHEADER ph)
{
    int i;
    if (ph->dwHow == DIPH_BYOFFSET) {
        return offset_to_object(This, ph->dwObj);
    } else if (ph->dwHow == DIPH_BYID) {
        for (i = 0; i < This->user_df->dwNumObjs; i++) {
            if ((This->user_df->rgodf[i].dwType & 0x00ffffff) == (ph->dwObj & 0x00ffffff)) {
                return i;
            }
        }
    }

    return -1;
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
    int i;

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(rguid),ph);

    if (ph == NULL) {
        WARN("invalid parameter: ph == NULL\n");
        return DIERR_INVALIDPARAM;
    }

    if (TRACE_ON(dinput))
        _dump_DIPROPHEADER(ph);

    if (!HIWORD(rguid)) {
        switch (LOWORD(rguid)) {
        case (DWORD) DIPROP_BUFFERSIZE: {
            LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
            TRACE("buffersize = %ld\n",pd->dwData);
            if (This->data_queue)
                This->data_queue = HeapReAlloc(GetProcessHeap(),0, This->data_queue, pd->dwData * sizeof(DIDEVICEOBJECTDATA));
            else
                This->data_queue = HeapAlloc(GetProcessHeap(),0, pd->dwData * sizeof(DIDEVICEOBJECTDATA));
            This->queue_head = 0;
            This->queue_tail = 0;
            This->queue_len  = pd->dwData;
            break;
        }
        case (DWORD)DIPROP_RANGE: {
            LPCDIPROPRANGE	pr = (LPCDIPROPRANGE)ph;
            if (ph->dwHow == DIPH_DEVICE) {
                TRACE("proprange(%ld,%ld) all\n",pr->lMin,pr->lMax);
                for (i = 0; i < This->user_df->dwNumObjs; i++) {
                    This->props[i].lMin = pr->lMin;
                    This->props[i].lMax = pr->lMax;
                }
            } else {
                int obj = find_property(This, ph);
                TRACE("proprange(%ld,%ld) obj=%d\n",pr->lMin,pr->lMax,obj);
                if (obj >= 0) {
                    This->props[obj].lMin = pr->lMin;
                    This->props[obj].lMax = pr->lMax;
                    return DI_OK;
                }
            }
            break;
        }
        case (DWORD)DIPROP_DEADZONE: {
            LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
            if (ph->dwHow == DIPH_DEVICE) {
                TRACE("deadzone(%ld) all\n",pd->dwData);
                for (i = 0; i < This->user_df->dwNumObjs; i++)
                    This->props[i].lDeadZone  = pd->dwData;
            } else {
                int obj = find_property(This, ph);
                TRACE("deadzone(%ld) obj=%d\n",pd->dwData,obj);
                if (obj >= 0) {
                    This->props[obj].lDeadZone  = pd->dwData;
                    return DI_OK;
                }
            }
            break;
        }
        case (DWORD)DIPROP_SATURATION: {
            LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
            if (ph->dwHow == DIPH_DEVICE) {
                TRACE("saturation(%ld) all\n",pd->dwData);
                for (i = 0; i < This->user_df->dwNumObjs; i++)
                    This->props[i].lSaturation = pd->dwData;
            } else {
                int obj = find_property(This, ph);
                TRACE("saturation(%ld) obj=%d\n",pd->dwData,obj);
                if (obj >= 0) {
                    This->props[obj].lSaturation = pd->dwData;
                    return DI_OK;
                }
            }
            break;
        }
        default:
            FIXME("Unknown type %p (%s)\n",rguid,debugstr_guid(rguid));
            break;
        }
    }

    return DI_OK;
}

/******************************************************************************
  *     SetEventNotification : specifies event to be sent on state change
  */
static HRESULT WINAPI JoystickAImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE8A iface, HANDLE hnd
) {
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(this=%p,%p)\n",This,hnd);
    This->hEvent = hnd;
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

    if (!This->acquired) {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    joy_polldev(This);
    return DI_OK;
}

/******************************************************************************
  *     EnumObjects : enumerate the different buttons and axis...
  */
static HRESULT WINAPI JoystickAImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
  JoystickImpl *This = (JoystickImpl *)iface;
  DIDEVICEOBJECTINSTANCEA ddoi;
  BYTE i;
  int user_offset;
  int user_object;

  TRACE("(this=%p,%p,%p,%08lx)\n", This, lpCallback, lpvRef, dwFlags);
  if (TRACE_ON(dinput)) {
    TRACE("  - flags = ");
    _dump_EnumObjects_flags(dwFlags);
    TRACE("\n");
  }

  /* Only the fields till dwFFMaxForce are relevant */
  ddoi.dwSize = FIELD_OFFSET(DIDEVICEOBJECTINSTANCEA, dwFFMaxForce);

  /* For the joystick, do as is done in the GetCapabilities function */
  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_AXIS) ||
      (dwFlags & DIDFT_POV)) {
    int	pov[4] = { 0, 0, 0, 0 };
    int axes = 0;
    int povs = 0;

    for (i = 0; i < This->axes; i++) {
      int wine_obj = This->axis_map[i];
      BOOL skip = FALSE;

      switch (wine_obj) {
      case 0:
	ddoi.guidType = GUID_XAxis;
	break;
      case 1:
	ddoi.guidType = GUID_YAxis;
	break;
      case 2:
	ddoi.guidType = GUID_ZAxis;
	break;
      case 3:
	ddoi.guidType = GUID_RxAxis;
	break;
      case 4:
	ddoi.guidType = GUID_RyAxis;
	break;
      case 5:
	ddoi.guidType = GUID_RzAxis;
	break;
      case 6:
	ddoi.guidType = GUID_Slider;
	break;
      case 7:
	ddoi.guidType = GUID_Slider;
	break;
      case 8:
        pov[0]++;
	ddoi.guidType = GUID_POV;
	break;
      case 9:
        pov[1]++;
	ddoi.guidType = GUID_POV;
	break;
      case 10:
        pov[2]++;
	ddoi.guidType = GUID_POV;
	break;
      case 11:
        pov[3]++;
	ddoi.guidType = GUID_POV;
	break;
      default:
	ddoi.guidType = GUID_Unknown;
      }
      if (wine_obj < 8) {
          user_offset = This->offsets[wine_obj];	/* get user offset from wine index */
          user_object = offset_to_object(This, user_offset);

          ddoi.dwType = This->user_df->rgodf[user_object].dwType & 0x00ffffff;
          ddoi.dwOfs =  This->user_df->rgodf[user_object].dwOfs;
          sprintf(ddoi.tszName, "Axis %d", axes);
          axes++;
      } else {
          if (pov[wine_obj - 8] < 2) {
              user_offset = This->offsets[wine_obj];	/* get user offset from wine index */
              user_object = offset_to_object(This, user_offset);

              ddoi.dwType = This->user_df->rgodf[user_object].dwType & 0x00ffffff;
              ddoi.dwOfs =  This->user_df->rgodf[user_object].dwOfs;
              sprintf(ddoi.tszName, "POV %d", povs);
              povs++;
          } else
              skip = TRUE;
      }
      if (!skip) {
          _dump_OBJECTINSTANCEA(&ddoi);
          if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE)
              return DI_OK;
      }
    }
  }

  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_BUTTON)) {

    /* The DInput SDK says that GUID_Button is only for mouse buttons but well... */
    ddoi.guidType = GUID_Button;

    for (i = 0; i < This->buttons; i++) {
      user_offset = This->offsets[i + 12];	/* get user offset from wine index */
      user_object = offset_to_object(This, user_offset);
      ddoi.guidType = GUID_Button;
      ddoi.dwType = This->user_df->rgodf[user_object].dwType & 0x00ffffff;
      ddoi.dwOfs =  This->user_df->rgodf[user_object].dwOfs;
      sprintf(ddoi.tszName, "Button %d", i);
      _dump_OBJECTINSTANCEA(&ddoi);
      if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
    }
  }

  return DI_OK;
}

/******************************************************************************
  *     EnumObjects : enumerate the different buttons and axis...
  */
static HRESULT WINAPI JoystickWImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
  JoystickImpl *This = (JoystickImpl *)iface;

  device_enumobjects_AtoWcb_data data;

  data.lpCallBack = lpCallback;
  data.lpvRef = lpvRef;

  return JoystickAImpl_EnumObjects((LPDIRECTINPUTDEVICE8A) This, (LPDIENUMDEVICEOBJECTSCALLBACKA) DIEnumDevicesCallbackAtoW, (LPVOID) &data, dwFlags);
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
        case (DWORD) DIPROP_BUFFERSIZE: {
            LPDIPROPDWORD	pd = (LPDIPROPDWORD)pdiph;
            TRACE(" return buffersize = %d\n",This->queue_len);
            pd->dwData = This->queue_len;
            break;
        }
        case (DWORD) DIPROP_RANGE: {
            LPDIPROPRANGE pr = (LPDIPROPRANGE) pdiph;
            int obj = find_property(This, pdiph);
            /* The app is querying the current range of the axis
             * return the lMin and lMax values */
            if (obj >= 0) {
                pr->lMin = This->props[obj].lMin;
                pr->lMax = This->props[obj].lMax;
                TRACE("range(%ld, %ld) obj=%d\n", pr->lMin, pr->lMax, obj);
                return DI_OK;
            }
            break;
        }
        case (DWORD) DIPROP_DEADZONE: {
            LPDIPROPDWORD	pd = (LPDIPROPDWORD)pdiph;
            int obj = find_property(This, pdiph);
            if (obj >= 0) {
                pd->dwData = This->props[obj].lDeadZone;
                TRACE("deadzone(%ld) obj=%d\n", pd->dwData, obj);
                return DI_OK;
            }
            break;
        }
        case (DWORD) DIPROP_SATURATION: {
            LPDIPROPDWORD	pd = (LPDIPROPDWORD)pdiph;
            int obj = find_property(This, pdiph);
            if (obj >= 0) {
                pd->dwData = This->props[obj].lSaturation;
                TRACE("saturation(%ld) obj=%d\n", pd->dwData, obj);
                return DI_OK;
            }
            break;
        }
        default:
            FIXME("Unknown type %p (%s)\n",rguid,debugstr_guid(rguid));
            break;
        }
    }

    return DI_OK;
}

/******************************************************************************
  *     GetObjectInfo : get object info
  */
HRESULT WINAPI JoystickAImpl_GetObjectInfo(
        LPDIRECTINPUTDEVICE8A iface,
        LPDIDEVICEOBJECTINSTANCEA pdidoi,
        DWORD dwObj,
        DWORD dwHow)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    DIDEVICEOBJECTINSTANCEA didoiA;
    unsigned int i;

    TRACE("(%p,%p,%ld,0x%08lx(%s))\n",
          iface, pdidoi, dwObj, dwHow,
          dwHow == DIPH_BYOFFSET ? "DIPH_BYOFFSET" :
          dwHow == DIPH_BYID ? "DIPH_BYID" :
          dwHow == DIPH_BYUSAGE ? "DIPH_BYUSAGE" :
          "UNKNOWN");

    if (pdidoi == NULL) {
        WARN("invalid parameter: pdidoi = NULL\n");
        return DIERR_INVALIDPARAM;
    }

    if ((pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCEA)) &&
        (pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3A))) {
        WARN("invalid parameter: pdidoi->dwSize = %ld != %d or %d\n",
             pdidoi->dwSize, sizeof(DIDEVICEOBJECTINSTANCEA),
             sizeof(DIDEVICEOBJECTINSTANCE_DX3A));
        return DIERR_INVALIDPARAM;
    }

    ZeroMemory(&didoiA, sizeof(didoiA));
    didoiA.dwSize = pdidoi->dwSize;

    switch (dwHow) {
    case DIPH_BYOFFSET: {
        int axis = 0;
        int pov = 0;
        int button = 0;
        for (i = 0; i < This->user_df->dwNumObjs; i++) {
            if (This->user_df->rgodf[i].dwOfs == dwObj) {
                if (This->user_df->rgodf[i].pguid)
                    didoiA.guidType = *This->user_df->rgodf[i].pguid;
                else
                    didoiA.guidType = GUID_NULL;

                didoiA.dwOfs = dwObj;
                didoiA.dwType = This->user_df->rgodf[i].dwType;
                didoiA.dwFlags = This->user_df->rgodf[i].dwFlags;

                if (DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) & DIDFT_AXIS)
                    sprintf(didoiA.tszName, "Axis %d", axis);
                else if (DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) & DIDFT_POV)
                    sprintf(didoiA.tszName, "POV %d", pov);
                else if (DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) & DIDFT_BUTTON)
                    sprintf(didoiA.tszName, "Button %d", button);

                CopyMemory(pdidoi, &didoiA, pdidoi->dwSize);
                return DI_OK;
            }

            if (DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) & DIDFT_AXIS)
                axis++;
            else if (DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) & DIDFT_POV)
                pov++;
            else if (DIDFT_GETTYPE(This->user_df->rgodf[i].dwType) & DIDFT_BUTTON)
                button++;
        }
        break;
    }
    case DIPH_BYID:
        FIXME("dwHow = DIPH_BYID not implemented\n");
        break;
    case DIPH_BYUSAGE:
        FIXME("dwHow = DIPH_BYUSAGE not implemented\n");
        break;
    default:
        WARN("invalid parameter: dwHow = %08lx\n", dwHow);
        return DIERR_INVALIDPARAM;
    }

    CopyMemory(pdidoi, &didoiA, pdidoi->dwSize);

    return DI_OK;
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
HRESULT WINAPI JoystickAImpl_GetDeviceInfo(
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
        WARN("invalid parameter: pdidi->dwSize = %ld != %d or %d\n",
             pdidi->dwSize, sizeof(DIDEVICEINSTANCE_DX3A),
             sizeof(DIDEVICEINSTANCEA));
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
HRESULT WINAPI JoystickWImpl_GetDeviceInfo(
    LPDIRECTINPUTDEVICE8W iface,
    LPDIDEVICEINSTANCEW pdidi)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p,%p)\n", iface, pdidi);

    if ((pdidi->dwSize != sizeof(DIDEVICEINSTANCE_DX3W)) &&
        (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW))) {
        WARN("invalid parameter: pdidi->dwSize = %ld != %d or %d\n",
             pdidi->dwSize, sizeof(DIDEVICEINSTANCE_DX3W),
             sizeof(DIDEVICEINSTANCEW));
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
	JoystickAImpl_Release,
	JoystickAImpl_GetCapabilities,
	JoystickAImpl_EnumObjects,
	JoystickAImpl_GetProperty,
	JoystickAImpl_SetProperty,
	JoystickAImpl_Acquire,
	JoystickAImpl_Unacquire,
	JoystickAImpl_GetDeviceState,
	JoystickAImpl_GetDeviceData,
	JoystickAImpl_SetDataFormat,
	JoystickAImpl_SetEventNotification,
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
# define XCAST(fun)	(typeof(SysJoystickWvt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static const IDirectInputDevice8WVtbl SysJoystickWvt =
{
	IDirectInputDevice2WImpl_QueryInterface,
	XCAST(AddRef)IDirectInputDevice2AImpl_AddRef,
	XCAST(Release)JoystickAImpl_Release,
	XCAST(GetCapabilities)JoystickAImpl_GetCapabilities,
	JoystickWImpl_EnumObjects,
	XCAST(GetProperty)JoystickAImpl_GetProperty,
	XCAST(SetProperty)JoystickAImpl_SetProperty,
	XCAST(Acquire)JoystickAImpl_Acquire,
	XCAST(Unacquire)JoystickAImpl_Unacquire,
	XCAST(GetDeviceState)JoystickAImpl_GetDeviceState,
	XCAST(GetDeviceData)JoystickAImpl_GetDeviceData,
	XCAST(SetDataFormat)JoystickAImpl_SetDataFormat,
	XCAST(SetEventNotification)JoystickAImpl_SetEventNotification,
	XCAST(SetCooperativeLevel)IDirectInputDevice2AImpl_SetCooperativeLevel,
	IDirectInputDevice2WImpl_GetObjectInfo,
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
