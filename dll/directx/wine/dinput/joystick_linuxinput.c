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

#include "dinput_private.h"

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
#ifdef HAVE_LINUX_INPUT_H
# include <linux/input.h>
# undef SW_MAX
# if defined(EVIOCGBIT) && defined(EV_ABS) && defined(BTN_PINKIE)
#  define HAS_PROPER_HEADER
# endif
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "device_private.h"

#ifdef HAS_PROPER_HEADER

#define EVDEVPREFIX "/dev/input/event"
#define EVDEVDRIVER " (event)"

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

        BOOL has_ff;
        int num_effects;

	/* data returned by EVIOCGBIT for caps, EV_ABS, EV_KEY, and EV_FF */
	BYTE				evbits[(EV_MAX+7)/8];
	BYTE				absbits[(ABS_MAX+7)/8];
	BYTE				keybits[(KEY_MAX+7)/8];
	BYTE				ffbits[(FF_MAX+7)/8];	

	/* data returned by the EVIOCGABS() ioctl */
        struct wine_input_absinfo       axes[ABS_MAX];

        WORD vendor_id, product_id;
};

struct JoystickImpl
{
        struct JoystickGenericImpl      generic;
        struct JoyDev                  *joydev;

	/* joystick private */
	int				joyfd;

	int                             dev_axes_to_di[ABS_MAX];
        POINTL                          povs[4];

	/* LUT for KEY_ to offset in rgbButtons */
	BYTE				buttons[KEY_MAX];

	/* Force feedback variables */
        struct list                     ff_effects;
	int				ff_state;
	int				ff_autocenter;
	int				ff_gain;
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

static void fake_current_js_state(JoystickImpl *ji);
static void find_joydevs(void);
static void joy_polldev(LPDIRECTINPUTDEVICE8A iface);

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
        BOOL no_ff_check = FALSE;
        int j;
        struct JoyDev *new_joydevs;
        struct input_id device_id = {0};

        snprintf(buf, sizeof(buf), EVDEVPREFIX"%d", i);

        if ((fd = open(buf, O_RDWR)) == -1)
        {
            fd = open(buf, O_RDONLY);
            no_ff_check = TRUE;
        }

        if (fd == -1)
        {
            WARN("Failed to open \"%s\": %d %s\n", buf, errno, strerror(errno));
            continue;
        }

        if (ioctl(fd, EVIOCGBIT(0, sizeof(joydev.evbits)), joydev.evbits) == -1)
        {
            WARN("ioctl(EVIOCGBIT, 0) failed: %d %s\n", errno, strerror(errno));
            close(fd);
            continue;
        }
        if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(joydev.absbits)), joydev.absbits) == -1)
        {
            WARN("ioctl(EVIOCGBIT, EV_ABS) failed: %d %s\n", errno, strerror(errno));
            close(fd);
            continue;
        }
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(joydev.keybits)), joydev.keybits) == -1)
        {
            WARN("ioctl(EVIOCGBIT, EV_KEY) failed: %d %s\n", errno, strerror(errno));
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
            (joydev.name = HeapAlloc(GetProcessHeap(), 0, strlen(buf) + strlen(EVDEVDRIVER) + 1)))
        {
            strcpy(joydev.name, buf);
            /* Append driver name */
            strcat(joydev.name, EVDEVDRIVER);
        }
        else
            joydev.name = joydev.device;

        if (device_disabled_registry(joydev.name)) {
            close(fd);
            HeapFree(GetProcessHeap(), 0, joydev.name);
            if (joydev.name != joydev.device)
                HeapFree(GetProcessHeap(), 0, joydev.device);
            continue;
        }

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
            joydev.has_ff = TRUE;
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

        if (ioctl(fd, EVIOCGID, &device_id) == -1)
            WARN("ioctl(EVIOCGID) failed: %d %s\n", errno, strerror(errno));
        else
        {
            joydev.vendor_id = device_id.vendor;
            joydev.product_id = device_id.product;
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
        joydevs[have_joydevs] = joydev;
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
    strcpy(lpddi->tszProductName, joydevs[id].name);
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
    MultiByteToWideChar(CP_ACP, 0, joydevs[id].name, -1, lpddi->tszProductName, MAX_PATH);
}

static HRESULT joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
  find_joydevs();

  if (id >= have_joydevs) {
    return E_FAIL;
  }

  if (!((dwDevType == 0) ||
        ((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
        (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))))
    return S_FALSE;

#ifndef HAVE_STRUCT_FF_EFFECT_DIRECTION
  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return S_FALSE;
#endif

  if (!(dwFlags & DIEDFL_FORCEFEEDBACK) || joydevs[id].has_ff) {
    fill_joystick_dideviceinstanceA(lpddi, version, id);
    return S_OK;
  }
  return S_FALSE;
}

static HRESULT joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
  find_joydevs();

  if (id >= have_joydevs) {
    return E_FAIL;
  }

  if (!((dwDevType == 0) ||
        ((dwDevType == DIDEVTYPE_JOYSTICK) && (version > 0x0300 && version < 0x0800)) ||
        (((dwDevType == DI8DEVCLASS_GAMECTRL) || (dwDevType == DI8DEVTYPE_JOYSTICK)) && (version >= 0x0800))))
    return S_FALSE;

#ifndef HAVE_STRUCT_FF_EFFECT_DIRECTION
  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return S_FALSE;
#endif

  if (!(dwFlags & DIEDFL_FORCEFEEDBACK) || joydevs[id].has_ff) {
    fill_joystick_dideviceinstanceW(lpddi, version, id);
    return S_OK;
  }
  return S_FALSE;
}

static JoystickImpl *alloc_device(REFGUID rguid, IDirectInputImpl *dinput, unsigned short index)
{
    JoystickImpl* newDevice;
    LPDIDATAFORMAT df = NULL;
    int i, idx = 0;
    int default_axis_map[WINE_JOYSTICK_MAX_AXES + WINE_JOYSTICK_MAX_POVS*2];

    newDevice = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(JoystickImpl));
    if (!newDevice) return NULL;

    newDevice->generic.base.IDirectInputDevice8A_iface.lpVtbl = &JoystickAvt;
    newDevice->generic.base.IDirectInputDevice8W_iface.lpVtbl = &JoystickWvt;
    newDevice->generic.base.ref    = 1;
    newDevice->generic.base.guid   = *rguid;
    newDevice->generic.base.dinput = dinput;
    newDevice->generic.joy_polldev = joy_polldev;
    newDevice->joyfd       = -1;
    newDevice->joydev      = &joydevs[index];
    newDevice->generic.name        = newDevice->joydev->name;
    list_init(&newDevice->ff_effects);
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    newDevice->ff_state    = FF_STATUS_STOPPED;
#endif
    /* There is no way in linux to query force feedback autocenter status.
       Instead, track it with ff_autocenter, and assume it's initially
       enabled. */
    newDevice->ff_autocenter = 1;
    newDevice->ff_gain = 0xFFFF;
    InitializeCriticalSection(&newDevice->generic.base.crit);
    newDevice->generic.base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JoystickImpl*->base.crit");

    /* Count number of available axes - supported Axis & POVs */
    for (i = 0; i < ABS_MAX; i++)
    {
        if (i < WINE_JOYSTICK_MAX_AXES &&
            test_bit(newDevice->joydev->absbits, i))
        {
            newDevice->generic.device_axis_count++;
            newDevice->dev_axes_to_di[i] = idx;
            newDevice->generic.props[idx].lDevMin = newDevice->joydev->axes[i].minimum;
            newDevice->generic.props[idx].lDevMax = newDevice->joydev->axes[i].maximum;
            default_axis_map[idx] = i;
            idx++;
        }
        else
            newDevice->dev_axes_to_di[i] = -1;
    }

    for (i = 0; i < WINE_JOYSTICK_MAX_POVS; i++)
    {
        if (test_bit(newDevice->joydev->absbits, ABS_HAT0X + i * 2) &&
            test_bit(newDevice->joydev->absbits, ABS_HAT0Y + i * 2))
        {
            newDevice->generic.device_axis_count += 2;
            newDevice->generic.props[idx  ].lDevMin = newDevice->joydev->axes[ABS_HAT0X + i * 2].minimum;
            newDevice->generic.props[idx  ].lDevMax = newDevice->joydev->axes[ABS_HAT0X + i * 2].maximum;
            newDevice->dev_axes_to_di[ABS_HAT0X + i * 2] = idx;
            newDevice->generic.props[idx+1].lDevMin = newDevice->joydev->axes[ABS_HAT0Y + i * 2].minimum;
            newDevice->generic.props[idx+1].lDevMax = newDevice->joydev->axes[ABS_HAT0Y + i * 2].maximum;
            newDevice->dev_axes_to_di[ABS_HAT0Y + i * 2] = idx + 1;

            default_axis_map[idx] = default_axis_map[idx + 1] = WINE_JOYSTICK_MAX_AXES + i;
            idx += 2;
        }
        else
            newDevice->dev_axes_to_di[ABS_HAT0X + i * 2] = newDevice->dev_axes_to_di[ABS_HAT0Y + i * 2] = -1;
    }

    /* do any user specified configuration */
    if (setup_dinput_options(&newDevice->generic, default_axis_map) != DI_OK) goto failed;

    /* Create copy of default data format */
    if (!(df = HeapAlloc(GetProcessHeap(), 0, c_dfDIJoystick2.dwSize))) goto failed;
    memcpy(df, &c_dfDIJoystick2, c_dfDIJoystick2.dwSize);
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto failed;


    /* Construct internal data format */

    /* Supported Axis & POVs */
    for (i = 0, idx = 0; i < newDevice->generic.device_axis_count; i++)
    {
        int wine_obj = newDevice->generic.axis_map[i];

        if (wine_obj < 0) continue;

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[wine_obj], df->dwObjSize);
        if (wine_obj < 8)
            df->rgodf[idx].dwType = DIDFT_MAKEINSTANCE(wine_obj) | DIDFT_ABSAXIS;
        else
        {
            df->rgodf[idx].dwType = DIDFT_MAKEINSTANCE(wine_obj - 8) | DIDFT_POV;
            i++; /* POV takes 2 axes */
        }

        newDevice->generic.props[idx].lMin        = 0;
        newDevice->generic.props[idx].lMax        = 0xffff;
        newDevice->generic.props[idx].lSaturation = 0;
        newDevice->generic.props[idx].lDeadZone   = newDevice->generic.deadzone;

        /* Linux supports force-feedback on X & Y axes only */
        if (newDevice->joydev->has_ff && (i == 0 || i == 1))
            df->rgodf[idx].dwFlags |= DIDOI_FFACTUATOR;

        idx++;
    }

    /* Buttons can be anywhere, so check all */
    for (i = 0; i < KEY_MAX && newDevice->generic.devcaps.dwButtons < WINE_JOYSTICK_MAX_BUTTONS; i++)
    {
        if (!test_bit(newDevice->joydev->keybits, i)) continue;

        memcpy(&df->rgodf[idx], &c_dfDIJoystick2.rgodf[newDevice->generic.devcaps.dwButtons + 12], df->dwObjSize);
        newDevice->buttons[i] = 0x80 | newDevice->generic.devcaps.dwButtons;
        df->rgodf[idx  ].pguid = &GUID_Button;
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(newDevice->generic.devcaps.dwButtons++) | DIDFT_PSHBUTTON;
    }
    df->dwNumObjs = idx;
    newDevice->generic.base.data_format.wine_df = df;

    fake_current_js_state(newDevice);

    /* Fill the caps */
    newDevice->generic.devcaps.dwSize = sizeof(newDevice->generic.devcaps);
    newDevice->generic.devcaps.dwFlags = DIDC_ATTACHED;
    if (newDevice->generic.base.dinput->dwVersion >= 0x0800)
        newDevice->generic.devcaps.dwDevType = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
    else
        newDevice->generic.devcaps.dwDevType = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);

    if (newDevice->joydev->has_ff)
        newDevice->generic.devcaps.dwFlags |= DIDC_FORCEFEEDBACK;

    IDirectInput_AddRef(&newDevice->generic.base.dinput->IDirectInput7A_iface);

    EnterCriticalSection(&dinput->crit);
    list_add_tail(&dinput->devices_list, &newDevice->generic.base.entry);
    LeaveCriticalSection(&dinput->crit);

    return newDevice;

failed:
    if (df) HeapFree(GetProcessHeap(), 0, df->rgodf);
    HeapFree(GetProcessHeap(), 0, df);
    HeapFree(GetProcessHeap(), 0, newDevice->generic.axis_map);
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

static HRESULT joydev_create_device(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPVOID *pdev, int unicode)
{
    unsigned short index;

    TRACE("%p %s %s %p %i\n", dinput, debugstr_guid(rguid), debugstr_guid(riid), pdev, unicode);
    find_joydevs();
    *pdev = NULL;

    if ((index = get_joystick_index(rguid)) < MAX_JOYDEV &&
        have_joydevs && index < have_joydevs)
    {
        JoystickImpl *This;

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

        This = alloc_device(rguid, dinput, index);
        TRACE("Created a Joystick device (%p)\n", This);

        if (!This) return DIERR_OUTOFMEMORY;

        if (unicode)
            *pdev = &This->generic.base.IDirectInputDevice8W_iface;
        else
            *pdev = &This->generic.base.IDirectInputDevice8A_iface;

        return DI_OK;
    }

    return DIERR_DEVICENOTREG;
}


const struct dinput_device joystick_linuxinput_device = {
  "Wine Linux-input joystick driver",
  joydev_enum_deviceA,
  joydev_enum_deviceW,
  joydev_create_device
};

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickWImpl_Acquire(LPDIRECTINPUTDEVICE8W iface)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;

    TRACE("(this=%p)\n",This);

    if ((res = IDirectInputDevice2WImpl_Acquire(iface)) != DI_OK)
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
            IDirectInputDevice2WImpl_Unacquire(iface);
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
        struct input_event event;

        event.type = EV_FF;
        event.code = FF_GAIN;
        event.value = This->ff_gain;
        if (write(This->joyfd, &event, sizeof(event)) == -1)
            ERR("Failed to set gain (%i): %d %s\n", This->ff_gain, errno, strerror(errno));
        if (!This->ff_autocenter)
        {
            /* Disable autocenter. */
            event.code = FF_AUTOCENTER;
            event.value = 0;
            if (write(This->joyfd, &event, sizeof(event)) == -1)
                ERR("Failed disabling autocenter: %d %s\n", errno, strerror(errno));
        }
    }

    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWImpl_Acquire(IDirectInputDevice8W_from_impl(This));
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickWImpl_Unacquire(LPDIRECTINPUTDEVICE8W iface)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;

    TRACE("(this=%p)\n",This);
    res = IDirectInputDevice2WImpl_Unacquire(iface);
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
      /* TODO: Read autocenter strength before disabling it, and use it here
       * instead of 0xFFFF (maximum strength).
       */
      event.value = 0xFFFF;
      if (write(This->joyfd, &event, sizeof(event)) == -1)
        ERR("Failed to set autocenter to %04x: %d %s\n", event.value, errno, strerror(errno));

      close(This->joyfd);
      This->joyfd = -1;
    }
    return res;
}

static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWImpl_Unacquire(IDirectInputDevice8W_from_impl(This));
}

/* 
 * set the current state of the js device as it would be with the middle
 * values on the axes
 */
#define CENTER_AXIS(a) \
    (ji->dev_axes_to_di[a] == -1 ? 0 : joystick_map_axis( &ji->generic.props[ji->dev_axes_to_di[a]], \
                                                          ji->joydev->axes[a].value ))
static void fake_current_js_state(JoystickImpl *ji)
{
    int i;

    /* center the axes */
    ji->generic.js.lX           = CENTER_AXIS(ABS_X);
    ji->generic.js.lY           = CENTER_AXIS(ABS_Y);
    ji->generic.js.lZ           = CENTER_AXIS(ABS_Z);
    ji->generic.js.lRx          = CENTER_AXIS(ABS_RX);
    ji->generic.js.lRy          = CENTER_AXIS(ABS_RY);
    ji->generic.js.lRz          = CENTER_AXIS(ABS_RZ);
    ji->generic.js.rglSlider[0] = CENTER_AXIS(ABS_THROTTLE);
    ji->generic.js.rglSlider[1] = CENTER_AXIS(ABS_RUDDER);

    /* POV center is -1 */
    for (i = 0; i < 4; i++)
        ji->generic.js.rgdwPOV[i] = -1;
}
#undef CENTER_AXIS

/* convert wine format offset to user format object index */
static void joy_polldev(LPDIRECTINPUTDEVICE8A iface)
{
    struct pollfd plfd;
    struct input_event ie;
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);

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
                This->generic.js.rgbButtons[btn] = value = ie.value ? 0x80 : 0x00;
            }
            break;
        }
	case EV_ABS:
        {
            int axis = This->dev_axes_to_di[ie.code];

            /* User axis remapping */
            if (axis < 0) break;
            axis = This->generic.axis_map[axis];
            if (axis < 0) break;

            inst_id = axis < 8 ?  DIDFT_MAKEINSTANCE(axis) | DIDFT_ABSAXIS :
                                  DIDFT_MAKEINSTANCE(axis - 8) | DIDFT_POV;
            value = joystick_map_axis(&This->generic.props[id_to_object(This->generic.base.data_format.wine_df, inst_id)], ie.value);

	    switch (axis) {
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
                int idx = axis - 8;

                if (ie.code % 2)
                    This->povs[idx].y = ie.value;
                else
                    This->povs[idx].x = ie.value;

                This->generic.js.rgdwPOV[idx] = value = joystick_map_pov(&This->povs[idx]);
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
#ifdef EV_MSC
        case EV_MSC:
            /* Ignore */
            break;
#endif
	default:
	    FIXME("joystick cannot handle type %d event (code %d)\n",ie.type,ie.code);
	    break;
	}
        if (inst_id >= 0)
            queue_event(iface, inst_id,
                        value, GetCurrentTime(), This->generic.base.dinput->evsequence++);
    }
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI JoystickWImpl_SetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPCDIPROPHEADER ph)
{
  JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);

  if (!ph) {
    WARN("invalid argument\n");
    return DIERR_INVALIDPARAM;
  }

  TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
  TRACE("ph.dwSize = %d, ph.dwHeaderSize =%d, ph.dwObj = %d, ph.dwHow= %d\n",
        ph->dwSize, ph->dwHeaderSize, ph->dwObj, ph->dwHow);

  if (IS_DIPROP(rguid)) {
    switch (LOWORD(rguid)) {
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
    case (DWORD_PTR)DIPROP_FFGAIN: {
      LPCDIPROPDWORD pd = (LPCDIPROPDWORD)ph;

      TRACE("DIPROP_FFGAIN(%d)\n", pd->dwData);
      This->ff_gain = MulDiv(pd->dwData, 0xFFFF, 10000);
      if (This->generic.base.acquired) {
        /* Update immediately. */
        struct input_event event;

        event.type = EV_FF;
        event.code = FF_GAIN;
        event.value = This->ff_gain;
        if (write(This->joyfd, &event, sizeof(event)) == -1)
          ERR("Failed to set gain (%i): %d %s\n", This->ff_gain, errno, strerror(errno));
      }
      break;
    }
    default:
      return JoystickWGenericImpl_SetProperty(iface, rguid, ph);
    }
  }
  return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPCDIPROPHEADER ph)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWImpl_SetProperty(IDirectInputDevice8W_from_impl(This), rguid, ph);
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI JoystickWImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(this=%p,%s,%p)\n", iface, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (!IS_DIPROP(rguid)) return DI_OK;

    switch (LOWORD(rguid)) {
    case (DWORD_PTR) DIPROP_AUTOCENTER:
    {
        LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;

        pd->dwData = This->ff_autocenter ? DIPROPAUTOCENTER_ON : DIPROPAUTOCENTER_OFF;
        TRACE("autocenter(%d)\n", pd->dwData);
        break;
    }
    case (DWORD_PTR) DIPROP_FFGAIN:
    {
        LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;

        pd->dwData = MulDiv(This->ff_gain, 10000, 0xFFFF);
        TRACE("DIPROP_FFGAIN(%d)\n", pd->dwData);
        break;
    }

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

    default:
        return JoystickWGenericImpl_GetProperty(iface, rguid, pdiph);
    }

    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWImpl_GetProperty(IDirectInputDevice8W_from_impl(This), rguid, pdiph);
}

/****************************************************************************** 
  *	CreateEffect - Create a new FF effect with the specified params
  */
static HRESULT WINAPI JoystickWImpl_CreateEffect(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid,
                                                 LPCDIEFFECT lpeff, LPDIRECTINPUTEFFECT *ppdef,
                                                 LPUNKNOWN pUnkOuter)
{
#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    effect_list_item* new_effect = NULL;
    HRESULT retval = DI_OK;
#endif

    JoystickImpl* This = impl_from_IDirectInputDevice8W(iface);
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

static HRESULT WINAPI JoystickAImpl_CreateEffect(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid,
                                                 LPCDIEFFECT lpeff, LPDIRECTINPUTEFFECT *ppdef,
                                                 LPUNKNOWN pUnkOuter)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWImpl_CreateEffect(IDirectInputDevice8W_from_impl(This), rguid, lpeff, ppdef, pUnkOuter);
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
    JoystickImpl* This = impl_from_IDirectInputDevice8A(iface);

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
    JoystickImpl* This = impl_from_IDirectInputDevice8W(iface);
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
    JoystickImpl* This = impl_from_IDirectInputDevice8A(iface);

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
    JoystickImpl* This = impl_from_IDirectInputDevice8W(iface);

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
static HRESULT WINAPI JoystickWImpl_GetForceFeedbackState(LPDIRECTINPUTDEVICE8W iface, LPDWORD pdwOut)
{
    JoystickImpl* This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(this=%p,%p)\n", This, pdwOut);

    (*pdwOut) = 0;

#ifdef HAVE_STRUCT_FF_EFFECT_DIRECTION
    /* DIGFFS_STOPPED is the only mandatory flag to report */
    if (This->ff_state == FF_STATUS_STOPPED)
	(*pdwOut) |= DIGFFS_STOPPED;
#endif

    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_GetForceFeedbackState(LPDIRECTINPUTDEVICE8A iface, LPDWORD pdwOut)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWImpl_GetForceFeedbackState(IDirectInputDevice8W_from_impl(This), pdwOut);
}

/*******************************************************************************
 *      SendForceFeedbackCommand - Send a command to the device's FF system
 */
static HRESULT WINAPI JoystickWImpl_SendForceFeedbackCommand(LPDIRECTINPUTDEVICE8W iface, DWORD dwFlags)
{
    JoystickImpl* This = impl_from_IDirectInputDevice8W(iface);
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

static HRESULT WINAPI JoystickAImpl_SendForceFeedbackCommand(LPDIRECTINPUTDEVICE8A iface, DWORD dwFlags)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWImpl_SendForceFeedbackCommand(IDirectInputDevice8W_from_impl(This), dwFlags);
}

/*******************************************************************************
 *      EnumCreatedEffectObjects - Enumerate all the effects that have been
 *		created for this device.
 */
static HRESULT WINAPI JoystickWImpl_EnumCreatedEffectObjects(LPDIRECTINPUTDEVICE8W iface,
                                                             LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
                                                             LPVOID pvRef, DWORD dwFlags)
{
    /* this function is safe to call on non-ff-enabled builds */
    JoystickImpl* This = impl_from_IDirectInputDevice8W(iface);
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

static HRESULT WINAPI JoystickAImpl_EnumCreatedEffectObjects(LPDIRECTINPUTDEVICE8A iface,
                                                             LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
                                                             LPVOID pvRef, DWORD dwFlags)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);
    return JoystickWImpl_EnumCreatedEffectObjects(IDirectInputDevice8W_from_impl(This), lpCallback, pvRef, dwFlags);
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8A iface,
                                                  LPDIDEVICEINSTANCEA pdidi)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8A(iface);

    TRACE("(%p) %p\n", This, pdidi);

    if (pdidi == NULL) return E_POINTER;
    if ((pdidi->dwSize != sizeof(DIDEVICEINSTANCE_DX3A)) &&
        (pdidi->dwSize != sizeof(DIDEVICEINSTANCEA)))
        return DIERR_INVALIDPARAM;

    fill_joystick_dideviceinstanceA(pdidi, This->generic.base.dinput->dwVersion,
                                    get_joystick_index(&This->generic.base.guid));
    return DI_OK;
}

static HRESULT WINAPI JoystickWImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8W iface,
                                                  LPDIDEVICEINSTANCEW pdidi)
{
    JoystickImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p) %p\n", This, pdidi);

    if (pdidi == NULL) return E_POINTER;
    if ((pdidi->dwSize != sizeof(DIDEVICEINSTANCE_DX3W)) &&
        (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW)))
        return DIERR_INVALIDPARAM;

    fill_joystick_dideviceinstanceW(pdidi, This->generic.base.dinput->dwVersion,
                                    get_joystick_index(&This->generic.base.guid));
    return DI_OK;
}

static const IDirectInputDevice8AVtbl JoystickAvt =
{
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
        IDirectInputDevice2AImpl_Release,
        JoystickAGenericImpl_GetCapabilities,
        IDirectInputDevice2AImpl_EnumObjects,
	JoystickAImpl_GetProperty,
	JoystickAImpl_SetProperty,
	JoystickAImpl_Acquire,
	JoystickAImpl_Unacquire,
        JoystickAGenericImpl_GetDeviceState,
	IDirectInputDevice2AImpl_GetDeviceData,
        IDirectInputDevice2AImpl_SetDataFormat,
	IDirectInputDevice2AImpl_SetEventNotification,
	IDirectInputDevice2AImpl_SetCooperativeLevel,
        JoystickAGenericImpl_GetObjectInfo,
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
    JoystickWImpl_GetProperty,
    JoystickWImpl_SetProperty,
    JoystickWImpl_Acquire,
    JoystickWImpl_Unacquire,
    JoystickWGenericImpl_GetDeviceState,
    IDirectInputDevice2WImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    JoystickWGenericImpl_GetObjectInfo,
    JoystickWImpl_GetDeviceInfo,
    IDirectInputDevice2WImpl_RunControlPanel,
    IDirectInputDevice2WImpl_Initialize,
    JoystickWImpl_CreateEffect,
    JoystickWImpl_EnumEffects,
    JoystickWImpl_GetEffectInfo,
    JoystickWImpl_GetForceFeedbackState,
    JoystickWImpl_SendForceFeedbackCommand,
    JoystickWImpl_EnumCreatedEffectObjects,
    IDirectInputDevice2WImpl_Escape,
    JoystickWGenericImpl_Poll,
    IDirectInputDevice2WImpl_SendDeviceData,
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    JoystickWGenericImpl_BuildActionMap,
    JoystickWGenericImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};

#else  /* HAS_PROPER_HEADER */

const struct dinput_device joystick_linuxinput_device = {
  "Wine Linux-input joystick driver",
  NULL,
  NULL,
  NULL
};

#endif  /* HAS_PROPER_HEADER */
