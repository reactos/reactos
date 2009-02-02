/*		DirectInput Joystick device
 *
 * Copyright 1998,2000 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2005 Daniel Remenak
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
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
#ifdef HAVE_LINUX_INPUT_H
# include <linux/input.h>
# undef SW_MAX
# if defined(EVIOCGBIT) && defined(EV_ABS) && defined(BTN_PINKIE)
#  define HAVE_CORRECT_LINUXINPUT_H
# endif
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);


/*
 * Maps POV x & y event values to a DX "clock" position:
 *         0
 *   31500    4500
 * 27000  -1    9000
 *   22500   13500
 *       18000
 */
DWORD joystick_map_pov(POINTL *p)
{
    if (p->x > 0)
        return p->y < 0 ?  4500 : !p->y ?  9000 : 13500;
    else if (p->x < 0)
        return p->y < 0 ? 31500 : !p->y ? 27000 : 22500;
    else
        return p->y < 0 ?     0 : !p->y ?    -1 : 18000;
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

#ifdef HAVE_CORRECT_LINUXINPUT_H

#define EVDEVPREFIX	"/dev/input/event"

/* Wine joystick driver object instances */
#define WINE_JOYSTICK_MAX_AXES    8
#define WINE_JOYSTICK_MAX_POVS    4
#define WINE_JOYSTICK_MAX_BUTTONS 128

struct wine_input_absinfo {
    LONG value;
    LONG minimum;
    LONG maximum;
    LONG fuzz;
    LONG flat;
};

/* implemented in effect_linuxinput.c */
HRESULT linuxinput_create_effect(int* fd, REFGUID rguid, struct list *parent_list_entry, LPDIRECTINPUTEFFECT* peff);
HRESULT linuxinput_get_info_A(int fd, REFGUID rguid, LPDIEFFECTINFOA info);
HRESULT linuxinput_get_info_W(int fd, REFGUID rguid, LPDIEFFECTINFOW info);

typedef struct JoystickImpl JoystickImpl;
static const IDirectInputDevice8AVtbl JoystickAvt;
static const IDirectInputDevice8WVtbl JoystickWvt;

struct JoyDev {
	char *device;
	char *name;
	GUID guid;

	int has_ff;
        int num_effects;

	/* data returned by EVIOCGBIT for caps, EV_ABS, EV_KEY, and EV_FF */
	BYTE				evbits[(EV_MAX+7)/8];
	BYTE				absbits[(ABS_MAX+7)/8];
	BYTE				keybits[(KEY_MAX+7)/8];
	BYTE				ffbits[(FF_MAX+7)/8];	

	/* data returned by the EVIOCGABS() ioctl */
        struct wine_input_absinfo       axes[ABS_MAX];
};

struct JoystickImpl
{
        struct IDirectInputDevice2AImpl base;

        struct JoyDev                  *joydev;

	/* joystick private */
	int				joyfd;

	DIJOYSTATE2			js;

	ObjProps                        props[ABS_MAX];

	int                             axes[ABS_MAX];
        POINTL                          povs[4];

	/* LUT for KEY_ to offset in rgbButtons */
	BYTE				buttons[KEY_MAX];

	DWORD                           numAxes;
	DWORD                           numPOVs;
	DWORD                           numButtons;

	/* Force feedback variables */
        struct list                     ff_effects;
	int				ff_state;
	int				ff_autocenter;
};

static void fake_current_js_state(JoystickImpl *ji);
static void find_joydevs(void);

/* This GUID is slightly different from the linux joystick one. Take note. */
static const GUID DInput_Wine_Joystick_Base_GUID = { /* 9e573eda-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573eda,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

#define test_bit(arr,bit) (((BYTE*)(arr))[(bit)>>3]&(1<<((bit)&7)))

#define MAX_JOYDEV 64

static int have_joydevs = -1;
static struct JoyDev *joydevs = NULL;

static void find_joydevs(void)
{
    int i;

    if (InterlockedCompareExchange(&have_joydevs, 0, -1) != -1)
        /* Someone beat us to it */
        return;

    for (i = 0; i < MAX_JOYDEV; i++)
    {
        char buf[MAX_PATH];
        struct JoyDev joydev = {0};
        int fd;
        int no_ff_check = 0;
        int j;
        struct JoyDev *new_joydevs;

        snprintf(buf, sizeof(buf), EVDEVPREFIX"%d", i);

        if ((fd = open(buf, O_RDWR)) == -1)
        {
            fd = open(buf, O_RDONLY);
            no_ff_check = 1;
        }

        if (fd == -1)
        {
            WARN("Failed to open \"%s\": %d %s\n", buf, errno, strerror(errno));
            continue;
        }

        if (ioctl(fd, EVIOCGBIT(0, sizeof(joydev.evbits)), joydev.evbits) == -1)
        {
            WARN("ioct(EVIOCGBIT, 0) failed: %d %s\n", errno, strerror(errno));
            close(fd);
            continue;
        }
        if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(joydev.absbits)), joydev.absbits) == -1)
        {
            WARN("ioct(EVIOCGBIT, EV_ABS) failed: %d %s\n", errno, strerror(errno));
            close(fd);
            continue;
        }
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(joydev.keybits)), joydev.keybits) == -1)
        {
            WARN("ioct(EVIOCGBIT, EV_KEY) failed: %d %s\n", errno, strerror(errno));
            close(fd);
            continue;
        }

        /* A true joystick has at least axis X and Y, and at least 1
         * button. copied from linux/drivers/input/joydev.c */
        if (!test_bit(joydev.absbits, ABS_X) || !test_bit(joydev.absbits, ABS_Y) ||
            !(test_bit(joydev.keybits, BTN_TRIGGER) ||
              test_bit(joydev.keybits, BTN_A) ||
              test_bit(joydev.keybits, BTN_1)))
        {
            close(fd);
            continue;
        }

        if (!(joydev.device = HeapAlloc(GetProcessHeap(), 0, strlen(buf) + 1)))
        {
            close(fd);
            continue;
        }
        strcpy(joydev.device, buf);

        buf[MAX_PATH - 1] = 0;
        if (ioctl(fd, EVIOCGNAME(MAX_PATH - 1), buf) != -1 &&
            (joydev.name = HeapAlloc(GetProcessHeap(), 0, strlen(buf) + 1)))
            strcpy(joydev.name, buf);
        else
            joydev.name = joydev.device;

	joydev.guid = DInput_Wine_Joystick_Base_GUID;
	joydev.guid.Data3 += have_joydevs;

        TRACE("Found a joystick on %s: %s (%s)\n", 
            joydev.device, joydev.name, 
            debugstr_guid(&joydev.guid)
            );

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
        if (!no_ff_check &&
            test_bit(joydev.evbits, EV_FF) &&
            ioctl(fd, EVIOCGBIT(EV_FF, sizeof(joydev.ffbits)), joydev.ffbits) != -1 &&
            ioctl(fd, EVIOCGEFFECTS, &joydev.num_effects) != -1 &&
            joydev.num_effects > 0)
        {
	    TRACE(" ... with force feedback\n");
	    joydev.has_ff = 1;
        }
#endif

        for (j = 0; j < ABS_MAX;j ++)
        {
            if (!test_bit(joydev.absbits, j)) continue;
            if (ioctl(fd, EVIOCGABS(j), &(joydev.axes[j])) != -1)
            {
	      TRACE(" ... with axis %d: cur=%d, min=%d, max=%d, fuzz=%d, flat=%d\n",
		  j,
		  joydev.axes[j].value,
		  joydev.axes[j].minimum,
		  joydev.axes[j].maximum,
		  joydev.axes[j].fuzz,
		  joydev.axes[j].flat
		  );
	    }
	}

        if (!have_joydevs)
            new_joydevs = HeapAlloc(GetProcessHeap(), 0, sizeof(struct JoyDev));
        else
            new_joydevs = HeapReAlloc(GetProcessHeap(), 0, joydevs, (1 + have_joydevs) * sizeof(struct JoyDev));

        if (!new_joydevs)
        {
            close(fd);
            continue;
        }
        joydevs = new_joydevs;
        memcpy(joydevs + have_joydevs, &joydev, sizeof(joydev));
        have_joydevs++;

        close(fd);
    }
}

static void fill_joystick_dideviceinstanceA(LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    DWORD dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    memset(lpddi, 0, dwSize);

    lpddi->dwSize       = dwSize;
    lpddi->guidInstance = joydevs[id].guid;
    lpddi->guidProduct  = DInput_Wine_Joystick_Base_GUID;
    lpddi->guidFFDriver = GUID_NULL;

    if (version >= 0x0800)
        lpddi->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
        lpddi->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);

    strcpy(lpddi->tszInstanceName, joydevs[id].name);
    strcpy(lpddi->tszProductName, joydevs[id].device);
}

static void fill_joystick_dideviceinstanceW(LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    DWORD dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    memset(lpddi, 0, dwSize);

    lpddi->dwSize       = dwSize;
    lpddi->guidInstance = joydevs[id].guid;
    lpddi->guidProduct  = DInput_Wine_Joystick_Base_GUID;
    lpddi->guidFFDriver = GUID_NULL;

    if (version >= 0x0800)
        lpddi->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
        lpddi->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);

    MultiByteToWideChar(CP_ACP, 0, joydevs[id].name, -1, lpddi->tszInstanceName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, joydevs[id].device, -1, lpddi->tszProductName, MAX_PATH);
}

static BOOL joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
  find_joydevs();

  if (id >= have_joydevs) {
    return FALSE;
  }

  if (!((dwDevType == 0) ||
        ((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
        (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))))
    return FALSE;

#ifndef HAVE_STRUCT_FF_EFFECT_DIRECTION
  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return FALSE;
#endif

  if (!(dwFlags & DIEDFL_FORCEFEEDBACK) || joydevs[id].has_ff) {
    fill_joystick_dideviceinstanceA(lpddi, version, id);
    return TRUE;
  }
  return FALSE;
}

static BOOL joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
  find_joydevs();

  if (id >= have_joydevs) {
    return FALSE;
  }

  if (!((dwDevType == 0) ||
        ((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
        (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))))
    return FALSE;

#ifndef HAVE_STRUCT_FF_EFFECT_DIRECTION
  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return FALSE;
#endif

  if (!(dwFlags & DIEDFL_FORCEFEEDBACK) || joydevs[id].has_ff) {
    fill_joystick_dideviceinstanceW(lpddi, version, id);
    return TRUE;
  }
  return FALSE;
}

static JoystickImpl *alloc_device(REFGUID rguid, const void *jvt, IDirectInputImpl *dinput, unsigned short index)
{
    JoystickImpl* newDevice;
    LPDIDATAFORMAT df = NULL;
    int i, idx = 0;
    char buffer[MAX_PATH+16];
    HKEY hkey, appkey;
    LONG def_deadzone = 0;

    newDevice = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(JoystickImpl));
    if (!newDevice) return NULL;

    newDevice->base.lpVtbl = jvt;
    newDevice->base.ref    = 1;
    newDevice->base.guid   = *rguid;
    newDevice->base.dinput = dinput;
    newDevice->joyfd       = -1;
    newDevice->joydev      = &joydevs[index];
    list_init(&newDevice->ff_effects);
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    newDevice->ff_state    = FF_STATUS_STOPPED;
#endif
    /* There is no way in linux to query force feedback autocenter status.
       Instead, track it with ff_autocenter, and assume it's initialy
       enabled. */
    newDevice->ff_autocenter = 1;
    InitializeCriticalSection(&newDevice->base.crit);
    newDevice->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JoystickImpl*->base.crit");

    /* get options */
    get_app_key(&hkey, &appkey);

    if (!get_config_key(hkey, appkey, "DefaultDeadZone", buffer, MAX_PATH))
    {
        def_deadzone = atoi(buffer);
        TRACE("setting default deadzone to: %d\n", def_deadzone);
    }
    if (appkey) RegCloseKey(appkey);
    if (hkey) RegCloseKey(hkey);

    /* Create copy of default data format */
    if (!(df = HeapAlloc(GetProcessHeap(), 0, c_dfDIJoystick2.dwSize))) goto failed;
    memcpy(df, &c_dfDIJoystick2, c_dfDIJoystick2.dwSize);
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto failed;

    /* Supported Axis & POVs should map 1-to-1 */
    for (i = 0; i < WINE_JOYSTICK_MAX_AXES; i++)
    {
        if (!test_bit(newDevice->joydev->absbits, i)) {
            newDevice->axes[i] = -1;
            continue;
        }

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[i], df->dwObjSize);
        newDevice->axes[i] = idx;
        newDevice->props[idx].lDevMin = newDevice->joydev->axes[i].minimum;
        newDevice->props[idx].lDevMax = newDevice->joydev->axes[i].maximum;
        newDevice->props[idx].lMin    = 0;
        newDevice->props[idx].lMax    = 0xffff;
        newDevice->props[idx].lSaturation = 0;
        newDevice->props[idx].lDeadZone = def_deadzone;

        /* Linux supports force-feedback on X & Y axes only */
        if (newDevice->joydev->has_ff && (i == 0 || i == 1))
            df->rgodf[idx].dwFlags |= DIDOI_FFACTUATOR;

        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(newDevice->numAxes++) | DIDFT_ABSAXIS;
    }

    for (i = 0; i < WINE_JOYSTICK_MAX_POVS; i++)
    {
        if (!test_bit(newDevice->joydev->absbits, ABS_HAT0X + i * 2) ||
            !test_bit(newDevice->joydev->absbits, ABS_HAT0Y + i * 2)) {
            newDevice->axes[ABS_HAT0X + i * 2] = newDevice->axes[ABS_HAT0Y + i * 2] = -1;
            continue;
        }

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[i + WINE_JOYSTICK_MAX_AXES], df->dwObjSize);
        newDevice->axes[ABS_HAT0X + i * 2] = newDevice->axes[ABS_HAT0Y + i * 2] = i;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(newDevice->numPOVs++) | DIDFT_POV;
    }

    /* Buttons can be anywhere, so check all */
    for (i = 0; i < KEY_MAX && newDevice->numButtons < WINE_JOYSTICK_MAX_BUTTONS; i++)
    {
        if (!test_bit(newDevice->joydev->keybits, i)) continue;

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[newDevice->numButtons + WINE_JOYSTICK_MAX_AXES + WINE_JOYSTICK_MAX_POVS], df->dwObjSize);
	newDevice->buttons[i] = 0x80 | newDevice->numButtons;
        df->rgodf[idx  ].pguid = &GUID_Button;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(newDevice->numButtons++) | DIDFT_PSHBUTTON;
    }
    df->dwNumObjs = idx;

    fake_current_js_state(newDevice);

    newDevice->base.data_format.wine_df = df;
    IDirectInput_AddRef((LPDIRECTINPUTDEVICE8A)newDevice->base.dinput);
    return newDevice;

failed:
    if (df) HeapFree(GetProcessHeap(), 0, df->rgodf);
    HeapFree(GetProcessHeap(), 0, df);
    HeapFree(GetProcessHeap(), 0, newDevice);
    return NULL;
}

/******************************************************************************
  *     get_joystick_index : Get the joystick index from a given GUID
  */
static unsigned short get_joystick_index(REFGUID guid)
{
    GUID wine_joystick = DInput_Wine_Joystick_Base_GUID;
    GUID dev_guid = *guid;

    wine_joystick.Data3 = 0;
    dev_guid.Data3 = 0;

    /* for the standard joystick GUID use index 0 */
    if(IsEqualGUID(&GUID_Joystick,guid)) return 0;

    /* for the wine joystick GUIDs use the index stored in Data3 */
    if(IsEqualGUID(&wine_joystick, &dev_guid)) return guid->Data3 - DInput_Wine_Joystick_Base_GUID.Data3;

    return MAX_JOYDEV;
}

static HRESULT joydev_create_deviceA(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
    unsigned short index;

    find_joydevs();

    if ((index = get_joystick_index(rguid)) < MAX_JOYDEV &&
        have_joydevs && index < have_joydevs)
    {
        if ((riid == NULL) ||
            IsEqualGUID(&IID_IDirectInputDeviceA,  riid) ||
            IsEqualGUID(&IID_IDirectInputDevice2A, riid) ||
            IsEqualGUID(&IID_IDirectInputDevice7A, riid) ||
            IsEqualGUID(&IID_IDirectInputDevice8A, riid))
        {
            *pdev = (IDirectInputDeviceA*) alloc_device(rguid, &JoystickAvt, dinput, index);
            TRACE("Created a Joystick device (%p)\n", *pdev);

            if (*pdev == NULL)
            {
                ERR("out of memory\n");
                return DIERR_OUTOFMEMORY;
            }
            return DI_OK;
        }

        WARN("no interface\n");
        return DIERR_NOINTERFACE;
    }

    return DIERR_DEVICENOTREG;
}


static HRESULT joydev_create_deviceW(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev)
{
    unsigned short index;

    find_joydevs();

    if ((index = get_joystick_index(rguid)) < MAX_JOYDEV &&
        have_joydevs && index < have_joydevs)
    {
        if ((riid == NULL) ||
            IsEqualGUID(&IID_IDirectInputDeviceW,  riid) ||
            IsEqualGUID(&IID_IDirectInputDevice2W, riid) ||
            IsEqualGUID(&IID_IDirectInputDevice7W, riid) ||
            IsEqualGUID(&IID_IDirectInputDevice8W, riid))
        {
            *pdev = (IDirectInputDeviceW*) alloc_device(rguid, &JoystickWvt, dinput, index);
            TRACE("Created a Joystick device (%p)\n", *pdev);

            if (*pdev == NULL)
            {
                ERR("out of memory\n");
                return DIERR_OUTOFMEMORY;
            }
            return DI_OK;
        }
        WARN("no interface\n");
        return DIERR_NOINTERFACE;
    }

    WARN("invalid device GUID\n");
    return DIERR_DEVICENOTREG;
}

const struct dinput_device joystick_linuxinput_device = {
  "Wine Linux-input joystick driver",
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
    HRESULT res;

    TRACE("(this=%p)\n",This);

    if ((res = IDirectInputDevice2AImpl_Acquire(iface)) != DI_OK)
    {
        WARN("Failed to acquire: %x\n", res);
        return res;
    }

    if ((This->joyfd = open(This->joydev->device, O_RDWR)) == -1)
    {
        if ((This->joyfd = open(This->joydev->device, O_RDONLY)) == -1)
        {
            /* Couldn't open the device at all */
            ERR("Failed to open device %s: %d %s\n", This->joydev->device, errno, strerror(errno));
            IDirectInputDevice2AImpl_Unacquire(iface);
            return DIERR_NOTFOUND;
        }
        else
        {
            /* Couldn't open in r/w but opened in read-only. */
            WARN("Could not open %s in read-write mode.  Force feedback will be disabled.\n", This->joydev->device);
        }
    }
    else
    {
        if (!This->ff_autocenter)
        {
            struct input_event event;

            /* Disable autocenter. */
            event.type = EV_FF;
            event.code = FF_AUTOCENTER;
            event.value = 0;
            if (write(This->joyfd, &event, sizeof(event)) == -1)
                ERR("Failed disabling autocenter: %d %s\n", errno, strerror(errno));
        }
    }

    return DI_OK;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    HRESULT res;

    TRACE("(this=%p)\n",This);
    res = IDirectInputDevice2AImpl_Unacquire(iface);
    if (res==DI_OK && This->joyfd!=-1) {
      effect_list_item *itr;
      struct input_event event;

      /* For each known effect:
       * - stop it
       * - unload it
       * But, unlike DISFFC_RESET, do not release the effect.
       */
      LIST_FOR_EACH_ENTRY(itr, &This->ff_effects, effect_list_item, entry) {
          IDirectInputEffect_Stop(itr->ref);
          IDirectInputEffect_Unload(itr->ref);
      }

      /* Enable autocenter. */
      event.type = EV_FF;
      event.code = FF_AUTOCENTER;
      /* TODO: Read autocenter strengh before disabling it, and use it here
       * instead of 0xFFFF (maximum strengh).
       */
      event.value = 0xFFFF;
      if (write(This->joyfd, &event, sizeof(event)) == -1)
        ERR("Failed to set autocenter to %04x: %d %s\n", event.value, errno, strerror(errno));

      close(This->joyfd);
      This->joyfd = -1;
    }
    return res;
}

/* 
 * set the current state of the js device as it would be with the middle
 * values on the axes
 */
#define CENTER_AXIS(a) \
    (ji->axes[a] == -1 ? 0 : joystick_map_axis( &ji->props[ji->axes[a]], \
                                                ji->joydev->axes[a].value ))
static void fake_current_js_state(JoystickImpl *ji)
{
    int i;

    /* center the axes */
    ji->js.lX           = CENTER_AXIS(ABS_X);
    ji->js.lY           = CENTER_AXIS(ABS_Y);
    ji->js.lZ           = CENTER_AXIS(ABS_Z);
    ji->js.lRx          = CENTER_AXIS(ABS_RX);
    ji->js.lRy          = CENTER_AXIS(ABS_RY);
    ji->js.lRz          = CENTER_AXIS(ABS_RZ);
    ji->js.rglSlider[0] = CENTER_AXIS(ABS_THROTTLE);
    ji->js.rglSlider[1] = CENTER_AXIS(ABS_RUDDER);

    /* POV center is -1 */
    for (i = 0; i < 4; i++)
        ji->js.rgdwPOV[i] = -1;
}
#undef CENTER_AXIS

/* convert wine format offset to user format object index */
static void joy_polldev(JoystickImpl *This)
{
    struct pollfd plfd;
    struct input_event ie;

    if (This->joyfd==-1)
	return;

    while (1)
    {
        LONG value = 0;
        int inst_id = -1;

	plfd.fd = This->joyfd;
	plfd.events = POLLIN;

	if (poll(&plfd,1,0) != 1)
	    return;

	/* we have one event, so we can read */
	if (sizeof(ie)!=read(This->joyfd,&ie,sizeof(ie)))
	    return;

	TRACE("input_event: type %d, code %d, value %d\n",ie.type,ie.code,ie.value);
	switch (ie.type) {
	case EV_KEY:	/* button */
        {
            int btn = This->buttons[ie.code];

            TRACE("(%p) %d -> %d\n", This, ie.code, btn);
            if (btn & 0x80)
            {
                btn &= 0x7F;
                inst_id = DIDFT_MAKEINSTANCE(btn) | DIDFT_PSHBUTTON;
                This->js.rgbButtons[btn] = value = ie.value ? 0x80 : 0x00;
            }
            break;
        }
	case EV_ABS:
        {
            int axis = This->axes[ie.code];
            if (axis==-1) {
                break;
            }
            inst_id = DIDFT_MAKEINSTANCE(axis) | (ie.code < ABS_HAT0X ? DIDFT_ABSAXIS : DIDFT_POV);
            value = joystick_map_axis(&This->props[id_to_object(This->base.data_format.wine_df, inst_id)], ie.value);

	    switch (ie.code) {
            case ABS_X:         This->js.lX  = value; break;
            case ABS_Y:         This->js.lY  = value; break;
            case ABS_Z:         This->js.lZ  = value; break;
            case ABS_RX:        This->js.lRx = value; break;
            case ABS_RY:        This->js.lRy = value; break;
            case ABS_RZ:        This->js.lRz = value; break;
            case ABS_THROTTLE:  This->js.rglSlider[0] = value; break;
            case ABS_RUDDER:    This->js.rglSlider[1] = value; break;
            case ABS_HAT0X: case ABS_HAT0Y: case ABS_HAT1X: case ABS_HAT1Y:
            case ABS_HAT2X: case ABS_HAT2Y: case ABS_HAT3X: case ABS_HAT3Y:
            {
                int idx = (ie.code - ABS_HAT0X) / 2;

                if (ie.code % 2)
                    This->povs[idx].y = ie.value;
                else
                    This->povs[idx].x = ie.value;

                This->js.rgdwPOV[idx] = value = joystick_map_pov(&This->povs[idx]);
                break;
            }
	    default:
		FIXME("unhandled joystick axis event (code %d, value %d)\n",ie.code,ie.value);
	    }
	    break;
        }
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
	case EV_FF_STATUS:
	    This->ff_state = ie.value;
	    break;
#endif
#ifdef EV_SYN
	case EV_SYN:
	    /* there is nothing to do */
	    break;
#endif
	default:
	    FIXME("joystick cannot handle type %d event (code %d)\n",ie.type,ie.code);
	    break;
	}
        if (inst_id >= 0)
            queue_event((LPDIRECTINPUTDEVICE8A)This,
                        id_to_offset(&This->base.data_format, inst_id),
                        value, ie.time.tv_usec, This->base.dinput->evsequence++);
    }
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the joystick.
  *
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE8A iface,DWORD len,LPVOID ptr
) {
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(this=%p,0x%08x,%p)\n", This, len, ptr);

    if (!This->base.acquired)
    {
        WARN("not acquired\n");
        return DIERR_NOTACQUIRED;
    }

    joy_polldev(This);

    /* convert and copy data to user supplied buffer */
    fill_DataFormat(ptr, len, &This->js, &This->base.data_format);

    return DI_OK;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI JoystickAImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
  JoystickImpl *This = (JoystickImpl *)iface;

  if (!ph) {
    WARN("invalid argument\n");
    return DIERR_INVALIDPARAM;
  }

  TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
  TRACE("ph.dwSize = %d, ph.dwHeaderSize =%d, ph.dwObj = %d, ph.dwHow= %d\n",
        ph->dwSize, ph->dwHeaderSize, ph->dwObj, ph->dwHow);

  if (!HIWORD(rguid)) {
    switch (LOWORD(rguid)) {
    case (DWORD_PTR)DIPROP_RANGE: {
      LPCDIPROPRANGE pr = (LPCDIPROPRANGE)ph;

      if (ph->dwHow == DIPH_DEVICE) {
        DWORD i;
        TRACE("proprange(%d,%d) all\n", pr->lMin, pr->lMax);
        for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++) {
          /* Scale dead-zone */
          This->props[i].lDeadZone = MulDiv(This->props[i].lDeadZone, pr->lMax - pr->lMin,
                                            This->props[i].lMax - This->props[i].lMin);
          This->props[i].lMin = pr->lMin;
          This->props[i].lMax = pr->lMax;
        }
      } else {
        int obj = find_property(&This->base.data_format, ph);

        TRACE("proprange(%d,%d) obj=%d\n", pr->lMin, pr->lMax, obj);
        if (obj >= 0) {
          /* Scale dead-zone */
          This->props[obj].lDeadZone = MulDiv(This->props[obj].lDeadZone, pr->lMax - pr->lMin,
                                              This->props[obj].lMax - This->props[obj].lMin);
          This->props[obj].lMin = pr->lMin;
          This->props[obj].lMax = pr->lMax;
        }
      }
      fake_current_js_state(This);
      break;
    }
    case (DWORD_PTR)DIPROP_DEADZONE: {
      LPCDIPROPDWORD pd = (LPCDIPROPDWORD)ph;
      if (ph->dwHow == DIPH_DEVICE) {
        DWORD i;
        TRACE("deadzone(%d) all\n", pd->dwData);
        for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++) {
          This->props[i].lDeadZone = pd->dwData;
        }
      } else {
        int obj = find_property(&This->base.data_format, ph);

        TRACE("deadzone(%d) obj=%d\n", pd->dwData, obj);
        if (obj >= 0) {
          This->props[obj].lDeadZone = pd->dwData;
        }
      }
      fake_current_js_state(This);
      break;
    }
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
      FIXME("DIPROP_CALIBRATIONMODE(%d)\n", pd->dwData);
      break;
    }
    case (DWORD_PTR)DIPROP_AUTOCENTER: {
      LPCDIPROPDWORD pd = (LPCDIPROPDWORD)ph;

      TRACE("autocenter(%d)\n", pd->dwData);
      This->ff_autocenter = pd->dwData == DIPROPAUTOCENTER_ON;

      break;
    }
    case (DWORD_PTR)DIPROP_SATURATION: {
      LPCDIPROPDWORD pd = (LPCDIPROPDWORD)ph;

      if (ph->dwHow == DIPH_DEVICE) {
        DWORD i;

        TRACE("saturation(%d) all\n", pd->dwData);
        for (i = 0; i < This->base.data_format.wine_df->dwNumObjs; i++)
          This->props[i].lSaturation = pd->dwData;
      } else {
          int obj = find_property(&This->base.data_format, ph);

          if (obj < 0) return DIERR_OBJECTNOTFOUND;

          TRACE("saturation(%d) obj=%d\n", pd->dwData, obj);
          This->props[obj].lSaturation = pd->dwData;
      }
      fake_current_js_state(This);
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

    TRACE("%p->(%p)\n",iface,lpDIDevCaps);

    if (!lpDIDevCaps) {
	WARN("invalid pointer\n");
	return E_POINTER;
    }

    if (lpDIDevCaps->dwSize != sizeof(DIDEVCAPS)) {
        WARN("invalid argument\n");
        return DIERR_INVALIDPARAM;
    }

    lpDIDevCaps->dwFlags	= DIDC_ATTACHED;
    if (This->base.dinput->dwVersion >= 0x0800)
        lpDIDevCaps->dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
        lpDIDevCaps->dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);

    if (This->joydev->has_ff) 
	 lpDIDevCaps->dwFlags |= DIDC_FORCEFEEDBACK;

    lpDIDevCaps->dwAxes = This->numAxes;
    lpDIDevCaps->dwButtons = This->numButtons;
    lpDIDevCaps->dwPOVs = This->numPOVs;

    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_Poll(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p)\n",This);

    if (!This->base.acquired)
        return DIERR_NOTACQUIRED;

    joy_polldev(This);
    return DI_OK;
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI JoystickAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface,
						REFGUID rguid,
						LPDIPROPHEADER pdiph)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(this=%p,%s,%p)\n", iface, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (HIWORD(rguid)) return DI_OK;

    switch (LOWORD(rguid)) {
    case (DWORD_PTR) DIPROP_RANGE:
    {
        LPDIPROPRANGE pr = (LPDIPROPRANGE) pdiph;
        int obj = find_property(&This->base.data_format, pdiph);

        if (obj < 0) return DIERR_OBJECTNOTFOUND;

        pr->lMin = This->props[obj].lMin;
        pr->lMax = This->props[obj].lMax;
	TRACE("range(%d, %d) obj=%d\n", pr->lMin, pr->lMax, obj);
        break;
    }
    case (DWORD_PTR) DIPROP_DEADZONE:
    {
        LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;
        int obj = find_property(&This->base.data_format, pdiph);

        if (obj < 0) return DIERR_OBJECTNOTFOUND;

        pd->dwData = This->props[obj].lDeadZone;
        TRACE("deadzone(%d) obj=%d\n", pd->dwData, obj);
        break;
    }
    case (DWORD_PTR) DIPROP_SATURATION:
    {
        LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;
        int obj = find_property(&This->base.data_format, pdiph);

        if (obj < 0) return DIERR_OBJECTNOTFOUND;

        pd->dwData = This->props[obj].lSaturation;
        TRACE("saturation(%d) obj=%d\n", pd->dwData, obj);
        break;
    }
    case (DWORD_PTR) DIPROP_AUTOCENTER:
    {
        LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;

        pd->dwData = This->ff_autocenter ? DIPROPAUTOCENTER_ON : DIPROPAUTOCENTER_OFF;
        TRACE("autocenter(%d)\n", pd->dwData);
        break;
    }

    default:
        return IDirectInputDevice2AImpl_GetProperty(iface, rguid, pdiph);
    }

    return DI_OK;
}

/******************************************************************************
  *     GetObjectInfo : get information about a device object such as a button
  *                     or axis
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
  *	CreateEffect - Create a new FF effect with the specified params
  */
static HRESULT WINAPI JoystickAImpl_CreateEffect(LPDIRECTINPUTDEVICE8A iface,
						 REFGUID rguid,
						 LPCDIEFFECT lpeff,
						 LPDIRECTINPUTEFFECT *ppdef,
						 LPUNKNOWN pUnkOuter)
{
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    effect_list_item* new_effect = NULL;
    HRESULT retval = DI_OK;
#endif

    JoystickImpl* This = (JoystickImpl*)iface;
    TRACE("(this=%p,%p,%p,%p,%p)\n", This, rguid, lpeff, ppdef, pUnkOuter);

#ifndef HAVE_STRUCT_FF_EFFECT_DIRECTION
    TRACE("not available (compiled w/o ff support)\n");
    *ppdef = NULL;
    return DI_OK;
#else

    if (!(new_effect = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_effect))))
        return DIERR_OUTOFMEMORY;

    retval = linuxinput_create_effect(&This->joyfd, rguid, &new_effect->entry, &new_effect->ref);
    if (retval != DI_OK)
    {
        HeapFree(GetProcessHeap(), 0, new_effect);
        return retval;
    }

    if (lpeff != NULL)
    {
        retval = IDirectInputEffect_SetParameters(new_effect->ref, lpeff, 0);

        if (retval != DI_OK && retval != DI_DOWNLOADSKIPPED)
        {
            HeapFree(GetProcessHeap(), 0, new_effect);
            return retval;
        }
    }

    list_add_tail(&This->ff_effects, &new_effect->entry);
    *ppdef = new_effect->ref;

    if (pUnkOuter != NULL)
	FIXME("Interface aggregation not implemented.\n");

    return DI_OK;

#endif /* HAVE_STRUCT_FF_EFFECT_DIRECTION */
} 

/*******************************************************************************
 *	EnumEffects - Enumerate available FF effects
 */
static HRESULT WINAPI JoystickAImpl_EnumEffects(LPDIRECTINPUTDEVICE8A iface,
						LPDIENUMEFFECTSCALLBACKA lpCallback,
						LPVOID pvRef,
						DWORD dwEffType)
{
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    DIEFFECTINFOA dei; /* feif */
    DWORD type = DIEFT_GETTYPE(dwEffType);
    JoystickImpl* This = (JoystickImpl*)iface;

    TRACE("(this=%p,%p,%d) type=%d\n", This, pvRef, dwEffType, type);

    dei.dwSize = sizeof(DIEFFECTINFOA);          

    if ((type == DIEFT_ALL || type == DIEFT_CONSTANTFORCE)
	&& test_bit(This->joydev->ffbits, FF_CONSTANT)) {
	IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_ConstantForce);
	(*lpCallback)(&dei, pvRef);
    }

    if ((type == DIEFT_ALL || type == DIEFT_PERIODIC)
	&& test_bit(This->joydev->ffbits, FF_PERIODIC)) {
	if (test_bit(This->joydev->ffbits, FF_SQUARE)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Square);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SINE)) {
            IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Sine);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_TRIANGLE)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Triangle);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SAW_UP)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_SawtoothUp);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SAW_DOWN)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_SawtoothDown);
	    (*lpCallback)(&dei, pvRef);
	}
    } 

    if ((type == DIEFT_ALL || type == DIEFT_RAMPFORCE)
	&& test_bit(This->joydev->ffbits, FF_RAMP)) {
        IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_RampForce);
        (*lpCallback)(&dei, pvRef);
    }

    if (type == DIEFT_ALL || type == DIEFT_CONDITION) {
	if (test_bit(This->joydev->ffbits, FF_SPRING)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Spring);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_DAMPER)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Damper);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_INERTIA)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Inertia);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_FRICTION)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Friction);
	    (*lpCallback)(&dei, pvRef);
	}
    }

#endif

    return DI_OK;
}

static HRESULT WINAPI JoystickWImpl_EnumEffects(LPDIRECTINPUTDEVICE8W iface,
                                                LPDIENUMEFFECTSCALLBACKW lpCallback,
                                                LPVOID pvRef,
                                                DWORD dwEffType)
{
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    /* seems silly to duplicate all this code but all the structures and functions
     * are actually different (A/W) */
    DIEFFECTINFOW dei; /* feif */
    DWORD type = DIEFT_GETTYPE(dwEffType);
    JoystickImpl* This = (JoystickImpl*)iface;
    int xfd = This->joyfd;

    TRACE("(this=%p,%p,%d) type=%d fd=%d\n", This, pvRef, dwEffType, type, xfd);

    dei.dwSize = sizeof(DIEFFECTINFOW);          

    if ((type == DIEFT_ALL || type == DIEFT_CONSTANTFORCE)
	&& test_bit(This->joydev->ffbits, FF_CONSTANT)) {
	IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_ConstantForce);
	(*lpCallback)(&dei, pvRef);
    }

    if ((type == DIEFT_ALL || type == DIEFT_PERIODIC)
	&& test_bit(This->joydev->ffbits, FF_PERIODIC)) {
	if (test_bit(This->joydev->ffbits, FF_SQUARE)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Square);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SINE)) {
            IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Sine);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_TRIANGLE)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Triangle);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SAW_UP)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_SawtoothUp);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_SAW_DOWN)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_SawtoothDown);
	    (*lpCallback)(&dei, pvRef);
	}
    } 

    if ((type == DIEFT_ALL || type == DIEFT_RAMPFORCE)
	&& test_bit(This->joydev->ffbits, FF_RAMP)) {
        IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_RampForce);
        (*lpCallback)(&dei, pvRef);
    }

    if (type == DIEFT_ALL || type == DIEFT_CONDITION) {
	if (test_bit(This->joydev->ffbits, FF_SPRING)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Spring);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_DAMPER)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Damper);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_INERTIA)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Inertia);
	    (*lpCallback)(&dei, pvRef);
	}
	if (test_bit(This->joydev->ffbits, FF_FRICTION)) {
	    IDirectInputDevice8_GetEffectInfo(iface, &dei, &GUID_Friction);
	    (*lpCallback)(&dei, pvRef);
	}
    }

    /* return to unacquired state if that's where it was */
    if (xfd == -1)
	IDirectInputDevice8_Unacquire(iface);
#endif

    return DI_OK;
}

/*******************************************************************************
 *      GetEffectInfo - Get information about a particular effect 
 */
static HRESULT WINAPI JoystickAImpl_GetEffectInfo(LPDIRECTINPUTDEVICE8A iface,
						  LPDIEFFECTINFOA pdei,
						  REFGUID guid)
{
    JoystickImpl* This = (JoystickImpl*)iface;

    TRACE("(this=%p,%p,%s)\n", This, pdei, _dump_dinput_GUID(guid));

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    return linuxinput_get_info_A(This->joyfd, guid, pdei); 
#else
    return DI_OK;
#endif
}

static HRESULT WINAPI JoystickWImpl_GetEffectInfo(LPDIRECTINPUTDEVICE8W iface,
                                                  LPDIEFFECTINFOW pdei,
                                                  REFGUID guid)
{
    JoystickImpl* This = (JoystickImpl*)iface;
            
    TRACE("(this=%p,%p,%s)\n", This, pdei, _dump_dinput_GUID(guid));
        
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    return linuxinput_get_info_W(This->joyfd, guid, pdei);
#else
    return DI_OK;
#endif
}

/*******************************************************************************
 *      GetForceFeedbackState - Get information about the device's FF state 
 */
static HRESULT WINAPI JoystickAImpl_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE8A iface,
	LPDWORD pdwOut)
{
    JoystickImpl* This = (JoystickImpl*)iface;

    TRACE("(this=%p,%p)\n", This, pdwOut);

    (*pdwOut) = 0;

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    /* DIGFFS_STOPPED is the only mandatory flag to report */
    if (This->ff_state == FF_STATUS_STOPPED)
	(*pdwOut) |= DIGFFS_STOPPED;
#endif

    return DI_OK;
}

/*******************************************************************************
 *      SendForceFeedbackCommand - Send a command to the device's FF system
 */
static HRESULT WINAPI JoystickAImpl_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE8A iface,
	DWORD dwFlags)
{
    JoystickImpl* This = (JoystickImpl*)iface;
    TRACE("(this=%p,%d)\n", This, dwFlags);

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    switch (dwFlags)
    {
    case DISFFC_STOPALL:
    {
	/* Stop all effects */
        effect_list_item *itr;

        LIST_FOR_EACH_ENTRY(itr, &This->ff_effects, effect_list_item, entry)
            IDirectInputEffect_Stop(itr->ref);
        break;
    }

    case DISFFC_RESET:
    {
        effect_list_item *itr, *ptr;

	/* Stop, unload, release and free all effects */
	/* This returns the device to its "bare" state */
        LIST_FOR_EACH_ENTRY_SAFE(itr, ptr, &This->ff_effects, effect_list_item, entry)
            IDirectInputEffect_Release(itr->ref);
        break;
    }
    case DISFFC_PAUSE:
    case DISFFC_CONTINUE:
	FIXME("No support for Pause or Continue in linux\n");
        break;

    case DISFFC_SETACTUATORSOFF:
    case DISFFC_SETACTUATORSON:
        FIXME("No direct actuator control in linux\n");
        break;

    default:
	FIXME("Unknown Force Feedback Command!\n");
	return DIERR_INVALIDPARAM;
    }
    return DI_OK;
#else
    return DIERR_UNSUPPORTED;
#endif
}

/*******************************************************************************
 *      EnumCreatedEffectObjects - Enumerate all the effects that have been
 *		created for this device.
 */
static HRESULT WINAPI JoystickAImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID pvRef,
	DWORD dwFlags)
{
    /* this function is safe to call on non-ff-enabled builds */
    JoystickImpl* This = (JoystickImpl*)iface;
    effect_list_item *itr, *ptr;

    TRACE("(this=%p,%p,%p,%d)\n", This, lpCallback, pvRef, dwFlags);

    if (!lpCallback)
	return DIERR_INVALIDPARAM;

    if (dwFlags != 0)
	FIXME("Flags specified, but no flags exist yet (DX9)!\n");

    LIST_FOR_EACH_ENTRY_SAFE(itr, ptr, &This->ff_effects, effect_list_item, entry)
        (*lpCallback)(itr->ref, pvRef);

    return DI_OK;
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8A iface,
                                                  LPDIDEVICEINSTANCEA pdidi)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p) %p\n", This, pdidi);

    if (pdidi == NULL) return E_POINTER;
    if ((pdidi->dwSize != sizeof(DIDEVICEINSTANCE_DX3A)) &&
        (pdidi->dwSize != sizeof(DIDEVICEINSTANCEA)))
        return DIERR_INVALIDPARAM;

    fill_joystick_dideviceinstanceA(pdidi, This->base.dinput->dwVersion,
                                    get_joystick_index(&This->base.guid));
    return DI_OK;
}

static HRESULT WINAPI JoystickWImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8W iface,
                                                  LPDIDEVICEINSTANCEW pdidi)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(%p) %p\n", This, pdidi);

    if (pdidi == NULL) return E_POINTER;
    if ((pdidi->dwSize != sizeof(DIDEVICEINSTANCE_DX3W)) &&
        (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW)))
        return DIERR_INVALIDPARAM;

    fill_joystick_dideviceinstanceW(pdidi, This->base.dinput->dwVersion,
                                    get_joystick_index(&This->base.guid));
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
	JoystickAImpl_CreateEffect,
	JoystickAImpl_EnumEffects,
	JoystickAImpl_GetEffectInfo,
	JoystickAImpl_GetForceFeedbackState,
	JoystickAImpl_SendForceFeedbackCommand,
	JoystickAImpl_EnumCreatedEffectObjects,
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
	XCAST(CreateEffect)JoystickAImpl_CreateEffect,
	JoystickWImpl_EnumEffects,
	JoystickWImpl_GetEffectInfo,
	XCAST(GetForceFeedbackState)JoystickAImpl_GetForceFeedbackState,
	XCAST(SendForceFeedbackCommand)JoystickAImpl_SendForceFeedbackCommand,
	XCAST(EnumCreatedEffectObjects)JoystickAImpl_EnumCreatedEffectObjects,
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

#else  /* HAVE_CORRECT_LINUXINPUT_H */

const struct dinput_device joystick_linuxinput_device = {
  "Wine Linux-input joystick driver",
  NULL,
  NULL,
  NULL,
  NULL
};

#endif  /* HAVE_CORRECT_LINUXINPUT_H */
