/*		DirectInput Joystick device
 *
 * Copyright 1998,2000 Marcus Meissner
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

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_LINUX_INPUT_H

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
#include <sys/fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif

#ifdef HAVE_CORRECT_LINUXINPUT_H

#ifdef HAVE_LINUX_INPUT_H
# include <linux/input.h>
#endif


#define EVDEVPREFIX	"/dev/input/event"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Wine joystick driver object instances */
#define WINE_JOYSTICK_AXIS_BASE   0
#define WINE_JOYSTICK_BUTTON_BASE 8

typedef struct JoystickImpl JoystickImpl;
static IDirectInputDevice8AVtbl JoystickAvt;
static IDirectInputDevice8WVtbl JoystickWvt;
struct JoystickImpl
{
        LPVOID                          lpVtbl;
        DWORD                           ref;
        GUID                            guid;


	/* The 'parent' DInput */
	IDirectInputImpl               *dinput;

	/* joystick private */
	/* what range and deadzone the game wants */
	LONG				wantmin[ABS_MAX];
	LONG				wantmax[ABS_MAX];
	LONG				deadz[ABS_MAX];

	/* autodetecting ranges per axe by following movement */
	LONG				havemax[ABS_MAX];
	LONG				havemin[ABS_MAX];

	int				joyfd;

	LPDIDATAFORMAT			df;
        HANDLE				hEvent;
        LPDIDEVICEOBJECTDATA 		data_queue;
        int				queue_head, queue_tail, queue_len;
	BOOL				overflow;
	DIJOYSTATE2			js;

	/* data returned by the EVIOCGABS() ioctl */
	int				axes[ABS_MAX+1][5];

#define AXE_ABS		0
#define AXE_ABSMIN	1
#define AXE_ABSMAX	2
#define AXE_ABSFUZZ	3
#define AXE_ABSFLAT	4


	/* data returned by EVIOCGBIT for EV_ABS and EV_KEY */
	BYTE				absbits[(ABS_MAX+7)/8];
	BYTE				keybits[(KEY_MAX+7)/8];
};

/* This GUID is slightly different from the linux joystick one. Take note. */
static GUID DInput_Wine_Joystick_GUID = { /* 9e573eda-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573eda,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

static void fake_current_js_state(JoystickImpl *ji);

#define test_bit(arr,bit) (((BYTE*)arr)[bit>>3]&(1<<(bit&7)))

static int joydev_have(void)
{
  int i, fd;
  int havejoy = 0;

  for (i=0;i<64;i++) {
      char	buf[200];
      BYTE	absbits[(ABS_MAX+7)/8],keybits[(KEY_MAX+7)/8];

      sprintf(buf,EVDEVPREFIX"%d",i);
      if (-1!=(fd=open(buf,O_RDONLY))) {
	  if (-1==ioctl(fd,EVIOCGBIT(EV_ABS,sizeof(absbits)),absbits)) {
	      perror("EVIOCGBIT EV_ABS");
	      close(fd);
	      continue;
	  }
	  if (-1==ioctl(fd,EVIOCGBIT(EV_KEY,sizeof(keybits)),keybits)) {
	      perror("EVIOCGBIT EV_KEY");
	      close(fd);
	      continue;
	  }
	  /* A true joystick has at least axis X and Y, and at least 1
	   * button. copied from linux/drivers/input/joydev.c */
	  if (test_bit(absbits,ABS_X) && test_bit(absbits,ABS_Y) &&
	      (   test_bit(keybits,BTN_TRIGGER)	||
		  test_bit(keybits,BTN_A) 	||
		  test_bit(keybits,BTN_1)
	      )
	  ) {
	      FIXME("found a joystick at %s!\n",buf);
	      havejoy = 1;
	  }
	  close(fd);
      }
      if (havejoy || (errno==ENODEV))
	  break;
  }
  return havejoy;
}

static BOOL joydev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, int version, int id)
{
  int havejoy = 0;

  if (id != 0)
      return FALSE;

  if ((dwDevType != 0) && (GET_DIDEVICE_TYPE(dwDevType) != DIDEVTYPE_JOYSTICK))
      return FALSE;

  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return FALSE;

  havejoy = joydev_have();

  if (!havejoy)
      return FALSE;

  TRACE("Enumerating the linuxinput Joystick device\n");

  /* Return joystick */
  lpddi->guidInstance	= GUID_Joystick;
  lpddi->guidProduct	= DInput_Wine_Joystick_GUID;

  lpddi->guidFFDriver = GUID_NULL;
  if (version >= 8)
    lpddi->dwDevType    = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_STANDARD << 8);
  else
    lpddi->dwDevType    = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8);

  strcpy(lpddi->tszInstanceName, "Joystick");
  /* ioctl JSIOCGNAME(len) */
  strcpy(lpddi->tszProductName,	"Wine Joystick");
  return TRUE;
}

static BOOL joydev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, int version, int id)
{
  int havejoy = 0;

  if (id != 0)
      return FALSE;

  if ((dwDevType != 0) && (GET_DIDEVICE_TYPE(dwDevType) != DIDEVTYPE_JOYSTICK))
      return FALSE;

  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return FALSE;

  havejoy = joydev_have();

  if (!havejoy)
      return FALSE;

  TRACE("Enumerating the linuxinput Joystick device\n");

  /* Return joystick */
  lpddi->guidInstance	= GUID_Joystick;
  lpddi->guidProduct	= DInput_Wine_Joystick_GUID;

  lpddi->guidFFDriver = GUID_NULL;
  lpddi->dwDevType    = DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL<<8);

  MultiByteToWideChar(CP_ACP, 0, "Joystick", -1, lpddi->tszInstanceName, MAX_PATH);
  /* ioctl JSIOCGNAME(len) */
  MultiByteToWideChar(CP_ACP, 0, "Wine Joystick", -1, lpddi->tszProductName, MAX_PATH);
  return TRUE;
}

static JoystickImpl *alloc_device(REFGUID rguid, LPVOID jvt, IDirectInputImpl *dinput)
{
  JoystickImpl* newDevice;
  int i;

  newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickImpl));
  newDevice->lpVtbl = jvt;
  newDevice->ref = 1;
  newDevice->joyfd = -1;
  newDevice->dinput = dinput;
  memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
  for (i=0;i<ABS_MAX;i++) {
    newDevice->wantmin[i] = -32768;
    newDevice->wantmax[i] =  32767;
    /* TODO: 
     * direct input defines a default for the deadzone somewhere; but as long
     * as in map_axis the code for the dead zone is commented out its no
     * problem
     */
    newDevice->deadz[i]   =  0;
  }
  fake_current_js_state(newDevice);
  return newDevice;
}

static HRESULT joydev_create_deviceA(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
  int havejoy = 0;

  havejoy = joydev_have();

  if (!havejoy)
      return DIERR_DEVICENOTREG;

  if ((IsEqualGUID(&GUID_Joystick,rguid)) ||
      (IsEqualGUID(&DInput_Wine_Joystick_GUID,rguid))) {
    if ((riid == NULL) ||
	IsEqualGUID(&IID_IDirectInputDeviceA,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice2A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice7A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice8A,riid)) {
      *pdev = (IDirectInputDeviceA*) alloc_device(rguid, &JoystickAvt, dinput);
      TRACE("Creating a Joystick device (%p)\n", *pdev);
      return DI_OK;
    } else
      return DIERR_NOINTERFACE;
  }

  return DIERR_DEVICENOTREG;
}


static HRESULT joydev_create_deviceW(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev)
{
  int havejoy = 0;

  havejoy = joydev_have();

  if (!havejoy)
      return DIERR_DEVICENOTREG;

  if ((IsEqualGUID(&GUID_Joystick,rguid)) ||
      (IsEqualGUID(&DInput_Wine_Joystick_GUID,rguid))) {
    if ((riid == NULL) ||
	IsEqualGUID(&IID_IDirectInputDeviceW,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice2W,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice7W,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice8W,riid)) {
      *pdev = (IDirectInputDeviceW*) alloc_device(rguid, &JoystickWvt, dinput);
      TRACE("Creating a Joystick device (%p)\n", *pdev);
      return DI_OK;
    } else
      return DIERR_NOINTERFACE;
  }

  return DIERR_DEVICENOTREG;
}

static dinput_device joydev = {
  20,
  "Wine Linux-input joystick driver",
  joydev_enum_deviceA,
  joydev_enum_deviceW,
  joydev_create_deviceA,
  joydev_create_deviceW
};

DECL_GLOBAL_CONSTRUCTOR(joydev_register) { dinput_register_device(&joydev); }

/******************************************************************************
 *	Joystick
 */
static ULONG WINAPI JoystickAImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
	JoystickImpl *This = (JoystickImpl *)iface;
	ULONG ref;

	ref = InterlockedDecrement(&(This->ref));
	if (ref)
		return ref;

	/* Free the data queue */
	if (This->data_queue != NULL)
	  HeapFree(GetProcessHeap(),0,This->data_queue);

	/* Free the DataFormat */
	HeapFree(GetProcessHeap(), 0, This->df);

	HeapFree(GetProcessHeap(),0,This);
	return 0;
}

/******************************************************************************
  *   SetDataFormat : the application can choose the format of the data
  *   the device driver sends back with GetDeviceState.
  */
static HRESULT WINAPI JoystickAImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE8A iface,LPCDIDATAFORMAT df
)
{
  JoystickImpl *This = (JoystickImpl *)iface;

  TRACE("(this=%p,%p)\n",This,df);

  _dump_DIDATAFORMAT(df);
  
  /* Store the new data format */
  This->df = HeapAlloc(GetProcessHeap(),0,df->dwSize);
  memcpy(This->df, df, df->dwSize);
  This->df->rgodf = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*df->dwObjSize);
  memcpy(This->df->rgodf,df->rgodf,df->dwNumObjs*df->dwObjSize);

  return 0;
}

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    int		i;
    JoystickImpl *This = (JoystickImpl *)iface;
    char	buf[200];

    TRACE("(this=%p)\n",This);
    if (This->joyfd!=-1)
    	return 0;
    for (i=0;i<64;i++) {
      sprintf(buf,EVDEVPREFIX"%d",i);
      if (-1==(This->joyfd=open(buf,O_RDONLY))) {
	if (errno==ENODEV)
	  return DIERR_NOTFOUND;
	perror(buf);
	continue;
      }
      if ((-1!=ioctl(This->joyfd,EVIOCGBIT(EV_ABS,sizeof(This->absbits)),This->absbits)) &&
          (-1!=ioctl(This->joyfd,EVIOCGBIT(EV_KEY,sizeof(This->keybits)),This->keybits)) &&
          (test_bit(This->absbits,ABS_X) && test_bit(This->absbits,ABS_Y) &&
	   (test_bit(This->keybits,BTN_TRIGGER)||
	    test_bit(This->keybits,BTN_A)	 ||
	    test_bit(This->keybits,BTN_1)
	  )
	 )
      )
	break;
      close(This->joyfd);
      This->joyfd = -1;
    }
    if (This->joyfd==-1)
    	return DIERR_NOTFOUND;

    for (i=0;i<ABS_MAX;i++) {
	if (test_bit(This->absbits,i)) {
	  if (-1==ioctl(This->joyfd,EVIOCGABS(i),&(This->axes[i])))
	    continue;
	  FIXME("axe %d: cur=%d, min=%d, max=%d, fuzz=%d, flat=%d\n",
	      i,
	      This->axes[i][AXE_ABS],
	      This->axes[i][AXE_ABSMIN],
	      This->axes[i][AXE_ABSMAX],
	      This->axes[i][AXE_ABSFUZZ],
	      This->axes[i][AXE_ABSFLAT]
	  );
	  This->havemin[i] = This->axes[i][AXE_ABSMIN];
	  This->havemax[i] = This->axes[i][AXE_ABSMAX];
	}
    }
    MESSAGE("\n");

	fake_current_js_state(This);

    return 0;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(this=%p)\n",This);
    if (This->joyfd!=-1) {
  	close(This->joyfd);
	This->joyfd = -1;
	return DI_OK;
    }
    else 
    	return DI_NOEFFECT;
}

/*
 * This maps the read value (from the input event) to a value in the
 * 'wanted' range. It also autodetects the possible range of the axe and
 * adapts values accordingly.
 */
static int
map_axis(JoystickImpl* This, int axis, int val) {
    int	xmin = This->axes[axis][AXE_ABSMIN];
    int	xmax = This->axes[axis][AXE_ABSMAX];
    int hmax = This->havemax[axis];
    int hmin = This->havemin[axis];
    int	wmin = This->wantmin[axis];
    int	wmax = This->wantmax[axis];
    int ret;

    if (val > hmax) This->havemax[axis] = hmax = val;
    if (val < hmin) This->havemin[axis] = hmin = val;

    if (xmin == xmax) return val;

    /* map the value from the hmin-hmax range into the wmin-wmax range */
    ret = (val * (wmax-wmin)) / (hmax-hmin) + wmin;

#if 0
    /* deadzone doesn't work comfortably enough right now. needs more testing*/
    if ((ret > -deadz/2 ) && (ret < deadz/2)) {
        FIXME("%d in deadzone, return mid.\n",val);
	return (wmax-wmin)/2+wmin;
    }
#endif
    return ret;
}

/* 
 * set the current state of the js device as it would be with the middle
 * values on the axes
 */
static void fake_current_js_state(JoystickImpl *ji)
{
	ji->js.lX  = map_axis(ji, ABS_X,  ji->axes[ABS_X ][AXE_ABS]);
	ji->js.lY  = map_axis(ji, ABS_Y,  ji->axes[ABS_Y ][AXE_ABS]);
	ji->js.lZ  = map_axis(ji, ABS_Z,  ji->axes[ABS_Z ][AXE_ABS]);
	ji->js.lRx = map_axis(ji, ABS_RX, ji->axes[ABS_RX][AXE_ABS]);
	ji->js.lRy = map_axis(ji, ABS_RY, ji->axes[ABS_RY][AXE_ABS]);
	ji->js.lRz = map_axis(ji, ABS_RZ, ji->axes[ABS_RZ][AXE_ABS]);
}

static void joy_polldev(JoystickImpl *This) {
    struct timeval tv;
    fd_set	readfds;
    struct	input_event ie;

    if (This->joyfd==-1)
	return;

    while (1) {
	memset(&tv,0,sizeof(tv));
	FD_ZERO(&readfds);
	FD_SET(This->joyfd,&readfds);

	if (1>select(This->joyfd+1,&readfds,NULL,NULL,&tv))
	    return;

	/* we have one event, so we can read */
	if (sizeof(ie)!=read(This->joyfd,&ie,sizeof(ie)))
	    return;

	TRACE("input_event: type %d, code %d, value %d\n",ie.type,ie.code,ie.value);
	switch (ie.type) {
	case EV_KEY:	/* button */
	    switch (ie.code) {
	    case BTN_TRIGGER:	/* normal flight stick */
	    case BTN_A:		/* gamepad */
	    case BTN_1:		/* generic */
		This->js.rgbButtons[0] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(0),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_THUMB:
	    case BTN_B:
	    case BTN_2:
		This->js.rgbButtons[1] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(1),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_THUMB2:
	    case BTN_C:
	    case BTN_3:
		This->js.rgbButtons[2] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(2),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_TOP:
	    case BTN_X:
	    case BTN_4:
		This->js.rgbButtons[3] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(3),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_TOP2:
	    case BTN_Y:
	    case BTN_5:
		This->js.rgbButtons[4] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(4),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_PINKIE:
	    case BTN_Z:
	    case BTN_6:
		This->js.rgbButtons[5] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(5),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_BASE:
	    case BTN_TL:
	    case BTN_7:
		This->js.rgbButtons[6] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(6),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_BASE2:
	    case BTN_TR:
	    case BTN_8:
		This->js.rgbButtons[7] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(7),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_BASE3:
	    case BTN_TL2:
	    case BTN_9:
		This->js.rgbButtons[8] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(8),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_BASE4:
	    case BTN_TR2:
		This->js.rgbButtons[9] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(9),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case BTN_BASE5:
	    case BTN_SELECT:
		This->js.rgbButtons[10] = ie.value?0x80:0x00;
		GEN_EVENT(DIJOFS_BUTTON(10),ie.value?0x80:0x0,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    default:
		FIXME("unhandled joystick button %x, value %d\n",ie.code,ie.value);
		break;
	    }
	    break;
	case EV_ABS:
	    switch (ie.code) {
	    case ABS_X:
		This->js.lX = map_axis(This,ABS_X,ie.value);
		GEN_EVENT(DIJOFS_X,This->js.lX,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case ABS_Y:
		This->js.lY = map_axis(This,ABS_Y,ie.value);
		GEN_EVENT(DIJOFS_Y,This->js.lY,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case ABS_Z:
		This->js.lZ = map_axis(This,ABS_Z,ie.value);
		GEN_EVENT(DIJOFS_Z,This->js.lZ,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case ABS_RX:
		This->js.lRx = map_axis(This,ABS_RX,ie.value);
		GEN_EVENT(DIJOFS_RX,This->js.lRx,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case ABS_RY:
		This->js.lRy = map_axis(This,ABS_RY,ie.value);
		GEN_EVENT(DIJOFS_RY,This->js.lRy,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    case ABS_RZ:
		This->js.lRz = map_axis(This,ABS_RZ,ie.value);
		GEN_EVENT(DIJOFS_RZ,This->js.lRz,ie.time.tv_usec,(This->dinput->evsequence)++);
		break;
	    default:
		FIXME("unhandled joystick axe event (code %d, value %d)\n",ie.code,ie.value);
		break;
	    }
	    break;
	default:
	    FIXME("joystick cannot handle type %d event (code %d)\n",ie.type,ie.code);
	    break;
	}
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

    joy_polldev(This);

    TRACE("(this=%p,0x%08lx,%p)\n",This,len,ptr);
    if ((len != sizeof(DIJOYSTATE)) && (len != sizeof(DIJOYSTATE2))) {
    	FIXME("len %ld is not sizeof(DIJOYSTATE) or DIJOYSTATE2, unsupported format.\n",len);
	return E_FAIL;
    }
    memcpy(ptr,&(This->js),len);
    This->queue_head = 0;
    This->queue_tail = 0;
    return 0;
}

/******************************************************************************
  *     GetDeviceData : gets buffered input data.
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceData(LPDIRECTINPUTDEVICE8A iface,
					      DWORD dodsize,
					      LPDIDEVICEOBJECTDATA dod,
					      LPDWORD entries,
					      DWORD flags
) {
  JoystickImpl *This = (JoystickImpl *)iface;

  FIXME("(%p)->(dods=%ld,entries=%ld,fl=0x%08lx),STUB!\n",This,dodsize,*entries,flags);

  joy_polldev(This);
  if (flags & DIGDD_PEEK)
    FIXME("DIGDD_PEEK\n");

  if (dod == NULL) {
  } else {
  }
  return 0;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI JoystickAImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
  JoystickImpl *This = (JoystickImpl *)iface;

  FIXME("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
  FIXME("ph.dwSize = %ld, ph.dwHeaderSize =%ld, ph.dwObj = %ld, ph.dwHow= %ld\n",ph->dwSize, ph->dwHeaderSize,ph->dwObj,ph->dwHow);

  if (!HIWORD(rguid)) {
    switch ((DWORD)rguid) {
    case (DWORD) DIPROP_BUFFERSIZE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

      FIXME("buffersize = %ld\n",pd->dwData);
      break;
    }
    case (DWORD)DIPROP_RANGE: {
      LPCDIPROPRANGE	pr = (LPCDIPROPRANGE)ph;

      FIXME("proprange(%ld,%ld)\n",pr->lMin,pr->lMax);
      switch (ph->dwObj) {
      case 0:	/* X */
      case 4:	/* Y */
      case 8:	/* Z */
      case 12:  /* Rx */
      case 16:  /* Ry */
      case 20:  /* Rz */
	  This->wantmin[ph->dwObj/4] = pr->lMin;
	  This->wantmax[ph->dwObj/4] = pr->lMax;
	  break;
      default:
	  FIXME("setting proprange %ld - %ld for dwObj %ld\n",pr->lMin,pr->lMax,ph->dwObj);
      }
      break;
    }
    case (DWORD)DIPROP_DEADZONE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

      FIXME("setting deadzone(%ld)\n",pd->dwData);
      This->deadz[ph->dwObj/4] = pd->dwData;
      break;
    }
    default:
      FIXME("Unknown type %ld (%s)\n",(DWORD)rguid,debugstr_guid(rguid));
      break;
    }
  }
  fake_current_js_state(This);
  return 0;
}

/******************************************************************************
  *     SetEventNotification : specifies event to be sent on state change
  */
static HRESULT WINAPI JoystickAImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE8A iface, HANDLE hnd
) {
    JoystickImpl *This = (JoystickImpl *)iface;

    TRACE("(this=%p,0x%08lx)\n",This,(DWORD)hnd);
    This->hEvent = hnd;
    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
    JoystickImpl *This = (JoystickImpl *)iface;
    int		xfd = This->joyfd;
    int		i,axes,buttons;
    int		wasacquired = 1;

    TRACE("%p->(%p)\n",iface,lpDIDevCaps);
    if (xfd==-1) {
	/* yes, games assume we return something, even if unacquired */
	JoystickAImpl_Acquire(iface);
	xfd = This->joyfd;
	wasacquired = 0;
    }
    lpDIDevCaps->dwFlags	= DIDC_ATTACHED;
    lpDIDevCaps->dwDevType	= DIDEVTYPE_JOYSTICK;

    axes=0;
    for (i=0;i<ABS_MAX;i++) if (test_bit(This->absbits,i)) axes++;
    buttons=0;
    for (i=0;i<KEY_MAX;i++) if (test_bit(This->keybits,i)) buttons++;

    lpDIDevCaps->dwAxes = axes;
    lpDIDevCaps->dwButtons = buttons;

    if (!wasacquired)
	JoystickAImpl_Unacquire(iface);

    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_Poll(LPDIRECTINPUTDEVICE8A iface) {
    JoystickImpl *This = (JoystickImpl *)iface;
    TRACE("(),stub!\n");

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
  int xfd = This->joyfd;


  TRACE("(this=%p,%p,%p,%08lx)\n", This, lpCallback, lpvRef, dwFlags);
  if (TRACE_ON(dinput)) {
    TRACE("  - flags = ");
    _dump_EnumObjects_flags(dwFlags);
    TRACE("\n");
  }

  if (xfd == -1) return DIERR_NOTACQUIRED;

  /* Only the fields till dwFFMaxForce are relevant */
  ddoi.dwSize = FIELD_OFFSET(DIDEVICEOBJECTINSTANCEA, dwFFMaxForce);

  /* For the joystick, do as is done in the GetCapabilities function */
  /* FIXME: needs more items */
  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_AXIS)) {
    BYTE i;

    for (i = 0; i < ABS_MAX; i++) {
      if (!test_bit(This->absbits,i)) continue;

      switch (i) {
      case ABS_X:
	ddoi.guidType = GUID_XAxis;
	ddoi.dwOfs = DIJOFS_X;
	break;
      case ABS_Y:
	ddoi.guidType = GUID_YAxis;
	ddoi.dwOfs = DIJOFS_Y;
	break;
      case ABS_Z:
	ddoi.guidType = GUID_ZAxis;
	ddoi.dwOfs = DIJOFS_Z;
	break;
      case ABS_RX:
	ddoi.guidType = GUID_RxAxis;
	ddoi.dwOfs = DIJOFS_RX;
	break;
      case ABS_RY:
	ddoi.guidType = GUID_RyAxis;
	ddoi.dwOfs = DIJOFS_RY;
	break;
      case ABS_RZ:
	ddoi.guidType = GUID_RzAxis;
	ddoi.dwOfs = DIJOFS_RZ;
	break;
      case ABS_THROTTLE:
	ddoi.guidType = GUID_Slider;
	ddoi.dwOfs = DIJOFS_SLIDER(0);
	break;
      default:
	FIXME("unhandled abs axis %d, ignoring!\n",i);
      }
      ddoi.dwType = DIDFT_MAKEINSTANCE((1<<i) << WINE_JOYSTICK_AXIS_BASE) | DIDFT_ABSAXIS;
      sprintf(ddoi.tszName, "%d-Axis", i);
      _dump_OBJECTINSTANCEA(&ddoi);
      if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE)
	  return DI_OK;
    }
  }

  if ((dwFlags == DIDFT_ALL) ||
      (dwFlags & DIDFT_BUTTON)) {
    int i;

    /*The DInput SDK says that GUID_Button is only for mouse buttons but well*/

    ddoi.guidType = GUID_Button;

    for (i = 0; i < KEY_MAX; i++) {
      if (!test_bit(This->keybits,i)) continue;

      switch (i) {
      case BTN_TRIGGER:
      case BTN_A:
      case BTN_1:
	  ddoi.dwOfs = DIJOFS_BUTTON(0);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 0) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_THUMB:
	case BTN_B:
	case BTN_2:
	  ddoi.dwOfs = DIJOFS_BUTTON(1);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 1) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_THUMB2:
	case BTN_C:
	case BTN_3:
	  ddoi.dwOfs = DIJOFS_BUTTON(2);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 2) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_TOP:
	case BTN_X:
	case BTN_4:
	  ddoi.dwOfs = DIJOFS_BUTTON(3);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 3) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_TOP2:
	case BTN_Y:
	case BTN_5:
	  ddoi.dwOfs = DIJOFS_BUTTON(4);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 4) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_PINKIE:
	case BTN_Z:
	case BTN_6:
	  ddoi.dwOfs = DIJOFS_BUTTON(5);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 5) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_BASE:
	case BTN_TL:
	case BTN_7:
	  ddoi.dwOfs = DIJOFS_BUTTON(6);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 6) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_BASE2:
	case BTN_TR:
	case BTN_8:
	  ddoi.dwOfs = DIJOFS_BUTTON(7);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 7) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_BASE3:
	case BTN_TL2:
	case BTN_9:
	  ddoi.dwOfs = DIJOFS_BUTTON(8);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 8) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_BASE4:
	case BTN_TR2:
	  ddoi.dwOfs = DIJOFS_BUTTON(9);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 9) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
	case BTN_BASE5:
	case BTN_SELECT:
	  ddoi.dwOfs = DIJOFS_BUTTON(10);
	  ddoi.dwType = DIDFT_MAKEINSTANCE((0x0001 << 10) << WINE_JOYSTICK_BUTTON_BASE) | DIDFT_PSHBUTTON;
	  break;
      }
      sprintf(ddoi.tszName, "%d-Button", i);
      _dump_OBJECTINSTANCEA(&ddoi);
      if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE)
	  return DI_OK;
    }
  }

  if (xfd!=This->joyfd)
    close(xfd);

  return DI_OK;
}

static HRESULT WINAPI JoystickWImpl_EnumObjects(LPDIRECTINPUTDEVICE8W iface,
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
static HRESULT WINAPI JoystickAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface,
						REFGUID rguid,
						LPDIPROPHEADER pdiph)
{
  JoystickImpl *This = (JoystickImpl *)iface;

  TRACE("(this=%p,%s,%p): stub!\n",
	iface, debugstr_guid(rguid), pdiph);

  if (TRACE_ON(dinput))
    _dump_DIPROPHEADER(pdiph);

  if (!HIWORD(rguid)) {
    switch ((DWORD)rguid) {
    case (DWORD) DIPROP_BUFFERSIZE: {
      LPDIPROPDWORD	pd = (LPDIPROPDWORD)pdiph;

      TRACE(" return buffersize = %d\n",This->queue_len);
      pd->dwData = This->queue_len;
      break;
    }

    case (DWORD) DIPROP_RANGE: {
      /* LPDIPROPRANGE pr = (LPDIPROPRANGE) pdiph; */
      if ((pdiph->dwHow == DIPH_BYID) &&
	  (pdiph->dwObj & DIDFT_ABSAXIS)) {
	/* The app is querying the current range of the axis : return the lMin and lMax values */
	FIXME("unimplemented axis range query.\n");
      }

      break;
    }

    default:
      FIXME("Unknown type %ld (%s)\n",(DWORD)rguid,debugstr_guid(rguid));
      break;
    }
  }


  return DI_OK;
}

static IDirectInputDevice8AVtbl JoystickAvt =
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
	IDirectInputDevice2AImpl_GetObjectInfo,
	IDirectInputDevice2AImpl_GetDeviceInfo,
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

static IDirectInputDevice8WVtbl JoystickWvt =
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
	IDirectInputDevice2WImpl_GetDeviceInfo,
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

#endif  /* HAVE_LINUX_INPUT_H */

#endif
