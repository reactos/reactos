/*		DirectInput Mouse device
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#define MOUSE_HACK

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Wine mouse driver object instances */
#define WINE_MOUSE_X_AXIS_INSTANCE   0
#define WINE_MOUSE_Y_AXIS_INSTANCE   1
#define WINE_MOUSE_Z_AXIS_INSTANCE   2
#define WINE_MOUSE_L_BUTTON_INSTANCE 0
#define WINE_MOUSE_R_BUTTON_INSTANCE 1
#define WINE_MOUSE_M_BUTTON_INSTANCE 2
#define WINE_MOUSE_D_BUTTON_INSTANCE 3

/* ------------------------------- */
/* Wine mouse internal data format */
/* ------------------------------- */

/* Constants used to access the offset array */
#define WINE_MOUSE_X_POSITION 0
#define WINE_MOUSE_Y_POSITION 1
#define WINE_MOUSE_Z_POSITION 2
#define WINE_MOUSE_L_POSITION 3
#define WINE_MOUSE_R_POSITION 4
#define WINE_MOUSE_M_POSITION 5

typedef struct {
    LONG lX;
    LONG lY;
    LONG lZ;
    BYTE rgbButtons[4];
} Wine_InternalMouseData;

#define WINE_INTERNALMOUSE_NUM_OBJS 6

static const DIOBJECTDATAFORMAT Wine_InternalMouseObjectFormat[WINE_INTERNALMOUSE_NUM_OBJS] = {
    { &GUID_XAxis,   FIELD_OFFSET(Wine_InternalMouseData, lX),
	  DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS, 0 },
    { &GUID_YAxis,   FIELD_OFFSET(Wine_InternalMouseData, lY),
	  DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS, 0 },
    { &GUID_ZAxis,   FIELD_OFFSET(Wine_InternalMouseData, lZ),
	  DIDFT_MAKEINSTANCE(WINE_MOUSE_Z_AXIS_INSTANCE) | DIDFT_RELAXIS, 0 },
    { &GUID_Button, (FIELD_OFFSET(Wine_InternalMouseData, rgbButtons)) + 0,
	  DIDFT_MAKEINSTANCE(WINE_MOUSE_L_BUTTON_INSTANCE) | DIDFT_PSHBUTTON, 0 },
    { &GUID_Button, (FIELD_OFFSET(Wine_InternalMouseData, rgbButtons)) + 1,
	  DIDFT_MAKEINSTANCE(WINE_MOUSE_R_BUTTON_INSTANCE) | DIDFT_PSHBUTTON, 0 },
    { &GUID_Button, (FIELD_OFFSET(Wine_InternalMouseData, rgbButtons)) + 2,
	  DIDFT_MAKEINSTANCE(WINE_MOUSE_M_BUTTON_INSTANCE) | DIDFT_PSHBUTTON, 0 }
};

static const DIDATAFORMAT Wine_InternalMouseFormat = {
    0, /* dwSize - unused */
    0, /* dwObjsize - unused */
    0, /* dwFlags - unused */
    sizeof(Wine_InternalMouseData),
    WINE_INTERNALMOUSE_NUM_OBJS, /* dwNumObjs */
    (LPDIOBJECTDATAFORMAT) Wine_InternalMouseObjectFormat
};

static const IDirectInputDevice8AVtbl SysMouseAvt;
static const IDirectInputDevice8WVtbl SysMouseWvt;

typedef struct SysMouseImpl SysMouseImpl;

typedef enum {
    WARP_DONE,   /* Warping has been done */
    WARP_NEEDED, /* Warping is needed */
    WARP_STARTED /* Warping has been done, waiting for the warp event */
} WARP_STATUS;

struct SysMouseImpl
{
    const void                     *lpVtbl;
    LONG                            ref;
    GUID                            guid;
    
    IDirectInputImpl               *dinput;
    
    /* The current data format and the conversion between internal
       and external data formats */
    DIDATAFORMAT	           *df;
    DataFormat                     *wine_df;
    int                             offset_array[WINE_INTERNALMOUSE_NUM_OBJS];
    
    /* SysMouseAImpl */
    BYTE                            absolute;
    /* Previous position for relative moves */
    LONG			    prevX, prevY;
    /* These are used in case of relative -> absolute transitions */
    POINT                           org_coords;
    HHOOK                           hook;
    HWND			    win;
    DWORD			    dwCoopLevel;
    POINT      			    mapped_center;
    DWORD			    win_centerX, win_centerY;
    LPDIDEVICEOBJECTDATA 	    data_queue;
    int				    queue_head, queue_tail, queue_len;
    BOOL			    overflow;
    /* warping: whether we need to move mouse back to middle once we
     * reach window borders (for e.g. shooters, "surface movement" games) */
    WARP_STATUS		            need_warp;
    int				    acquired;
    HANDLE			    hEvent;
    CRITICAL_SECTION		    crit;
    
    /* This is for mouse reporting. */
    Wine_InternalMouseData          m_state;
};

/* FIXME: This is ugly and not thread safe :/ */
static IDirectInputDevice8A* current_lock = NULL;

/* FIXME: This is ugly but needed on Windows */
static int mouse_set = 0;
static GUID DInput_Wine_Mouse_GUID = { /* 9e573ed8-7734-11d2-8d4a-23903fb6bdf7 */
    0x9e573ed8,
    0x7734,
    0x11d2,
    {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

static void fill_mouse_dideviceinstanceA(LPDIDEVICEINSTANCEA lpddi, DWORD version) {
    DWORD dwSize;
    DIDEVICEINSTANCEA ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%ld %p\n", dwSize, lpddi);
    
    memset(lpddi, 0, dwSize);
    memset(&ddi, 0, sizeof(ddi));

    ddi.dwSize = dwSize;
    ddi.guidInstance = GUID_SysMouse;/* DInput's GUID */
    ddi.guidProduct = DInput_Wine_Mouse_GUID; /* Vendor's GUID */
    if (version >= 0x0800)
        ddi.dwDevType = DI8DEVTYPE_MOUSE | (DI8DEVTYPEMOUSE_TRADITIONAL << 8);
    else
        ddi.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_TRADITIONAL << 8);
    strcpy(ddi.tszInstanceName, "Mouse");
    strcpy(ddi.tszProductName, "Wine Mouse");

    memcpy(lpddi, &ddi, (dwSize < sizeof(ddi) ? dwSize : sizeof(ddi)));
}

static void fill_mouse_dideviceinstanceW(LPDIDEVICEINSTANCEW lpddi, DWORD version) {
    DWORD dwSize;
    DIDEVICEINSTANCEW ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%ld %p\n", dwSize, lpddi);
    
    memset(lpddi, 0, dwSize);
    memset(&ddi, 0, sizeof(ddi));

    ddi.dwSize = dwSize;
    ddi.guidInstance = GUID_SysMouse;/* DInput's GUID */
    ddi.guidProduct = DInput_Wine_Mouse_GUID; /* Vendor's GUID */
    if (version >= 0x0800)
        ddi.dwDevType = DI8DEVTYPE_MOUSE | (DI8DEVTYPEMOUSE_TRADITIONAL << 8);
    else
        ddi.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_TRADITIONAL << 8);
    MultiByteToWideChar(CP_ACP, 0, "Mouse", -1, ddi.tszInstanceName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, "Wine Mouse", -1, ddi.tszProductName, MAX_PATH);

    memcpy(lpddi, &ddi, (dwSize < sizeof(ddi) ? dwSize : sizeof(ddi)));
}

static BOOL mousedev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    if (id != 0)
        return FALSE;

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_MOUSE) && (version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_POINTER) || (dwDevType == DI8DEVTYPE_MOUSE)) && (version >= 0x0800))) {
	TRACE("Enumerating the mouse device\n");
	
	fill_mouse_dideviceinstanceA(lpddi, version);
	
	return TRUE;
    }
    
    return FALSE;
}

static BOOL mousedev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    if (id != 0)
        return FALSE;

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_MOUSE) && (version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_POINTER) || (dwDevType == DI8DEVTYPE_MOUSE)) && (version >= 0x0800))) {
	TRACE("Enumerating the mouse device\n");
	
	fill_mouse_dideviceinstanceW(lpddi, version);
	
	return TRUE;
    }
    
    return FALSE;
}

static SysMouseImpl *alloc_device(REFGUID rguid, const void *mvt, IDirectInputImpl *dinput)
{
    int offset_array[WINE_INTERNALMOUSE_NUM_OBJS] = {
	FIELD_OFFSET(Wine_InternalMouseData, lX),
	FIELD_OFFSET(Wine_InternalMouseData, lY),
	FIELD_OFFSET(Wine_InternalMouseData, lZ),
	FIELD_OFFSET(Wine_InternalMouseData, rgbButtons) + 0,
	FIELD_OFFSET(Wine_InternalMouseData, rgbButtons) + 1,
	FIELD_OFFSET(Wine_InternalMouseData, rgbButtons) + 2
    };
    SysMouseImpl* newDevice;
    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysMouseImpl));
    newDevice->ref = 1;
    newDevice->lpVtbl = mvt;
    InitializeCriticalSection(&(newDevice->crit));
    memcpy(&(newDevice->guid),rguid,sizeof(*rguid));

    /* Per default, Wine uses its internal data format */
    newDevice->df = (DIDATAFORMAT *) &Wine_InternalMouseFormat;
    memcpy(newDevice->offset_array, offset_array, WINE_INTERNALMOUSE_NUM_OBJS * sizeof(int));
    newDevice->wine_df = HeapAlloc(GetProcessHeap(), 0, sizeof(DataFormat));
    newDevice->wine_df->size = 0;
    newDevice->wine_df->internal_format_size = Wine_InternalMouseFormat.dwDataSize;
    newDevice->wine_df->dt = NULL;
    newDevice->dinput = dinput;

    return newDevice;
}

static HRESULT mousedev_create_deviceA(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
    if ((IsEqualGUID(&GUID_SysMouse,rguid)) ||             /* Generic Mouse */
	(IsEqualGUID(&DInput_Wine_Mouse_GUID,rguid))) { /* Wine Mouse */
	if ((riid == NULL) ||
	    IsEqualGUID(&IID_IDirectInputDeviceA,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice2A,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice7A,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice8A,riid)) {
	    *pdev = (IDirectInputDeviceA*) alloc_device(rguid, &SysMouseAvt, dinput);
	    TRACE("Creating a Mouse device (%p)\n", *pdev);
	    return DI_OK;
	} else
	    return DIERR_NOINTERFACE;
    }
    
    return DIERR_DEVICENOTREG;
}

static HRESULT mousedev_create_deviceW(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEW* pdev)
{
    if ((IsEqualGUID(&GUID_SysMouse,rguid)) ||             /* Generic Mouse */
	(IsEqualGUID(&DInput_Wine_Mouse_GUID,rguid))) { /* Wine Mouse */
	if ((riid == NULL) ||
	    IsEqualGUID(&IID_IDirectInputDeviceW,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice2W,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice7W,riid) ||
	    IsEqualGUID(&IID_IDirectInputDevice8W,riid)) {
	    *pdev = (IDirectInputDeviceW*) alloc_device(rguid, &SysMouseWvt, dinput);
	    TRACE("Creating a Mouse device (%p)\n", *pdev);
	    return DI_OK;
	} else
	    return DIERR_NOINTERFACE;
    }
    
    return DIERR_DEVICENOTREG;
}

const struct dinput_device mouse_device = {
    "Wine mouse driver",
    mousedev_enum_deviceA,
    mousedev_enum_deviceW,
    mousedev_create_deviceA,
    mousedev_create_deviceW
};

/******************************************************************************
 *	SysMouseA (DInput Mouse support)
 */

/******************************************************************************
  *     Release : release the mouse buffer.
  */
static ULONG WINAPI SysMouseAImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    ULONG ref;
 
    ref = InterlockedDecrement(&(This->ref));
    if (ref)
	return ref;
    
    /* Free the data queue */
    HeapFree(GetProcessHeap(),0,This->data_queue);
    
    if (This->hook) {
	UnhookWindowsHookEx( This->hook );
	if (This->dwCoopLevel & DISCL_EXCLUSIVE)
            ShowCursor(TRUE); /* show cursor */
    }
    DeleteCriticalSection(&(This->crit));
    
    /* Free the DataFormat */
    if (This->df != &(Wine_InternalMouseFormat)) {
	HeapFree(GetProcessHeap(), 0, This->df->rgodf);
	HeapFree(GetProcessHeap(), 0, This->df);
    }
    
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}


/******************************************************************************
  *     SetCooperativeLevel : store the window in which we will do our
  *   grabbing.
  */
static HRESULT WINAPI SysMouseAImpl_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE8A iface,HWND hwnd,DWORD dwflags
)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    TRACE("(this=%p,%p,0x%08lx)\n",This,hwnd,dwflags);
    
    if (TRACE_ON(dinput)) {
	TRACE(" cooperative level : ");
	_dump_cooperativelevel_DI(dwflags);
    }
    
    /* Store the window which asks for the mouse */
    if (!hwnd)
	hwnd = GetDesktopWindow();
    This->win = hwnd;
    This->dwCoopLevel = dwflags;
    
    return DI_OK;
}


/******************************************************************************
  *     SetDataFormat : the application can choose the format of the data
  *   the device driver sends back with GetDeviceState.
  *
  *   For the moment, only the "standard" configuration (c_dfDIMouse) is supported
  *   in absolute and relative mode.
  */
static HRESULT WINAPI SysMouseAImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE8A iface,LPCDIDATAFORMAT df
)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    TRACE("(this=%p,%p)\n",This,df);
    
    _dump_DIDATAFORMAT(df);
    
    /* Tests under windows show that a call to SetDataFormat always sets the mouse
       in relative mode whatever the dwFlags value (DIDF_ABSAXIS/DIDF_RELAXIS).
       To switch in absolute mode, SetProperty must be used. */
    This->absolute = 0;
    
    /* Store the new data format */
    This->df = HeapAlloc(GetProcessHeap(),0,df->dwSize);
    memcpy(This->df, df, df->dwSize);
    This->df->rgodf = HeapAlloc(GetProcessHeap(),0,df->dwNumObjs*df->dwObjSize);
    memcpy(This->df->rgodf,df->rgodf,df->dwNumObjs*df->dwObjSize);
    
    /* Prepare all the data-conversion filters */
    This->wine_df = create_DataFormat(&(Wine_InternalMouseFormat), df, This->offset_array);
    
    return DI_OK;
}

/* low-level mouse hook */
static LRESULT CALLBACK dinput_mouse_hook( int code, WPARAM wparam, LPARAM lparam )
{
    LRESULT ret;
    MSLLHOOKSTRUCT *hook = (MSLLHOOKSTRUCT *)lparam;
    SysMouseImpl* This = (SysMouseImpl*) current_lock;
    DWORD dwCoop;
    static long last_event = 0;
    int wdata;
    long lasttime = 0; 

    if (code != HC_ACTION) return CallNextHookEx( This->hook, code, wparam, lparam );

    EnterCriticalSection(&(This->crit));
    dwCoop = This->dwCoopLevel;

    /* Only allow mouse events every 10 ms.
     * This is to allow the cursor to start acceleration before
     * the warps happen. But if it involves a mouse button event we
     * allow it since we don't want to lose the clicks.
     */
#ifndef __REACTOS__     
    if (((GetCurrentTime() - last_event) < 10)
        && wparam == WM_MOUSEMOVE)
	goto end;
    else last_event = GetCurrentTime();
#else
    lasttime = GetCurrentTime() - last_event;
    
    if ((lasttime) < 1)        
	    goto end;
    else if ((lasttime) >= 10)        
	    last_event = GetCurrentTime();    
#endif    
    
    /* Mouse moved -> send event if asked */
    if (This->hEvent)
        SetEvent(This->hEvent);
    
    if (wparam == WM_MOUSEMOVE) {
	if (This->absolute) {
	    if (hook->pt.x != This->prevX)
		GEN_EVENT(This->offset_array[WINE_MOUSE_X_POSITION], hook->pt.x, hook->time, 0);
	    if (hook->pt.y != This->prevY)
		GEN_EVENT(This->offset_array[WINE_MOUSE_Y_POSITION], hook->pt.y, hook->time, 0);
	} else {
	    /* Now, warp handling */
	    if ((This->need_warp == WARP_STARTED) &&
		(hook->pt.x == This->mapped_center.x) && (hook->pt.y == This->mapped_center.y)) {
		/* Warp has been done... */
		This->need_warp = WARP_DONE;
		goto end;
	    }
	    
	    /* Relative mouse input with absolute mouse event : the real fun starts here... */
	    if ((This->need_warp == WARP_NEEDED) ||
		(This->need_warp == WARP_STARTED)) {
		if (hook->pt.x != This->prevX)
		    GEN_EVENT(This->offset_array[WINE_MOUSE_X_POSITION], hook->pt.x - This->prevX,
			      hook->time, (This->dinput->evsequence)++);
		if (hook->pt.y != This->prevY)
		    GEN_EVENT(This->offset_array[WINE_MOUSE_Y_POSITION], hook->pt.y - This->prevY,
			      hook->time, (This->dinput->evsequence)++);
	    } else {
		/* This is the first time the event handler has been called after a
		   GetDeviceData or GetDeviceState. */
		if (hook->pt.x != This->mapped_center.x) {
		    GEN_EVENT(This->offset_array[WINE_MOUSE_X_POSITION], hook->pt.x - This->mapped_center.x,
			      hook->time, (This->dinput->evsequence)++);
		    This->need_warp = WARP_NEEDED;
		}
		
		if (hook->pt.y != This->mapped_center.y) {
		    GEN_EVENT(This->offset_array[WINE_MOUSE_Y_POSITION], hook->pt.y - This->mapped_center.y,
			      hook->time, (This->dinput->evsequence)++);
		    This->need_warp = WARP_NEEDED;
		}
	    }
	}
	
	This->prevX = hook->pt.x;
	This->prevY = hook->pt.y;
	
	if (This->absolute) {
	    This->m_state.lX = hook->pt.x;
	    This->m_state.lY = hook->pt.y;
	} else {
	    This->m_state.lX = hook->pt.x - This->mapped_center.x;
	    This->m_state.lY = hook->pt.y - This->mapped_center.y;
	}
    }
    
    TRACE(" msg %x pt %ld %ld (W=%d)\n",
          wparam, hook->pt.x, hook->pt.y, (!This->absolute) && This->need_warp );
    
    switch(wparam) {
        case WM_LBUTTONDOWN:
	    GEN_EVENT(This->offset_array[WINE_MOUSE_L_POSITION], 0x80,
		      hook->time, This->dinput->evsequence++);
	    This->m_state.rgbButtons[0] = 0x80;
	    break;
	case WM_LBUTTONUP:
	    GEN_EVENT(This->offset_array[WINE_MOUSE_L_POSITION], 0x00,
		      hook->time, This->dinput->evsequence++);
	    This->m_state.rgbButtons[0] = 0x00;
	    break;
	case WM_RBUTTONDOWN:
	    GEN_EVENT(This->offset_array[WINE_MOUSE_R_POSITION], 0x80,
		      hook->time, This->dinput->evsequence++);
	    This->m_state.rgbButtons[1] = 0x80;
	    break;
	case WM_RBUTTONUP:
	    GEN_EVENT(This->offset_array[WINE_MOUSE_R_POSITION], 0x00,
		      hook->time, This->dinput->evsequence++);
	    This->m_state.rgbButtons[1] = 0x00;
	    break;
	case WM_MBUTTONDOWN:
	    GEN_EVENT(This->offset_array[WINE_MOUSE_M_POSITION], 0x80,
		      hook->time, This->dinput->evsequence++);
	    This->m_state.rgbButtons[2] = 0x80;
	    break;
	case WM_MBUTTONUP:
	    GEN_EVENT(This->offset_array[WINE_MOUSE_M_POSITION], 0x00,
		      hook->time, This->dinput->evsequence++);
	    This->m_state.rgbButtons[2] = 0x00;
	    break;
	case WM_MOUSEWHEEL:
	    wdata = (short)HIWORD(hook->mouseData);
	    GEN_EVENT(This->offset_array[WINE_MOUSE_Z_POSITION], wdata,
		      hook->time, This->dinput->evsequence++);
	    This->m_state.lZ += wdata;
	    break;
    }
    
    TRACE("(X: %ld - Y: %ld   L: %02x M: %02x R: %02x)\n",
	  This->m_state.lX, This->m_state.lY,
	  This->m_state.rgbButtons[0], This->m_state.rgbButtons[2], This->m_state.rgbButtons[1]);
    
  end:
    LeaveCriticalSection(&(This->crit));
    
    if (dwCoop & DISCL_NONEXCLUSIVE) {
	/* Pass the events down to previous handlers (e.g. win32 input) */
	ret = CallNextHookEx( This->hook, code, wparam, lparam );
    } else {
	/* Ignore message */
	ret = 1;
    }
    return ret;
}


static void dinput_window_check(SysMouseImpl* This) {
    RECT rect;
    DWORD centerX, centerY;
    
    /* make sure the window hasn't moved */
    GetWindowRect(This->win, &rect);
    centerX = (rect.right  - rect.left) / 2;
    centerY = (rect.bottom - rect.top ) / 2;
    if (This->win_centerX != centerX || This->win_centerY != centerY) {
	This->win_centerX = centerX;
	This->win_centerY = centerY;
    }
    This->mapped_center.x = This->win_centerX;
    This->mapped_center.y = This->win_centerY;
    MapWindowPoints(This->win, HWND_DESKTOP, &This->mapped_center, 1);
}


/******************************************************************************
  *     Acquire : gets exclusive control of the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    RECT  rect;
    POINT point;
    
    TRACE("(this=%p)\n",This);
    
    if (This->acquired)
      return S_FALSE;
    
    This->acquired = 1;

    /* Store (in a global variable) the current lock */
    current_lock = (IDirectInputDevice8A*)This;
    
    /* Init the mouse state */
    GetCursorPos( &point );
    if (This->absolute) {
      This->m_state.lX = point.x;
      This->m_state.lY = point.y;
      This->prevX = point.x;
      This->prevY = point.y;
    } else {
      This->m_state.lX = 0;
      This->m_state.lY = 0;
      This->org_coords = point;
    }
    This->m_state.lZ = 0;
    This->m_state.rgbButtons[0] = GetKeyState(VK_LBUTTON) & 0x80;
    This->m_state.rgbButtons[1] = GetKeyState(VK_RBUTTON) & 0x80;
    This->m_state.rgbButtons[2] = GetKeyState(VK_MBUTTON) & 0x80;
    
    /* Install our mouse hook */
    if (This->dwCoopLevel & DISCL_EXCLUSIVE)
      ShowCursor(FALSE); /* hide cursor */
    This->hook = SetWindowsHookExA( WH_MOUSE_LL, dinput_mouse_hook, DINPUT_instance, 0 );
    
    /* Get the window dimension and find the center */
    GetWindowRect(This->win, &rect);
    This->win_centerX = (rect.right  - rect.left) / 2;
    This->win_centerY = (rect.bottom - rect.top ) / 2;
    
    /* Warp the mouse to the center of the window */
    if (This->absolute == 0) {
      This->mapped_center.x = This->win_centerX;
      This->mapped_center.y = This->win_centerY;
      MapWindowPoints(This->win, HWND_DESKTOP, &This->mapped_center, 1);
      TRACE("Warping mouse to %ld - %ld\n", This->mapped_center.x, This->mapped_center.y);
      SetCursorPos( This->mapped_center.x, This->mapped_center.y );
#ifdef MOUSE_HACK
      This->need_warp = WARP_DONE;
#else
      This->need_warp = WARP_STARTED;
#endif
    }
	
    return DI_OK;
}

/******************************************************************************
  *     Unacquire : frees the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    TRACE("(this=%p)\n",This);
    
    if (0 == This->acquired) {
	return DI_NOEFFECT;
    }
	
    /* Reinstall previous mouse event handler */
    if (This->hook) {
      UnhookWindowsHookEx( This->hook );
      This->hook = 0;
      
      if (This->dwCoopLevel & DISCL_EXCLUSIVE)
	ShowCursor(TRUE); /* show cursor */
    }
	
    /* No more locks */
    if (current_lock == (IDirectInputDevice8A*) This)
      current_lock = NULL;
    else
      ERR("this(%p) != current_lock(%p)\n", This, current_lock);

    /* Unacquire device */
    This->acquired = 0;
    
    /* And put the mouse cursor back where it was at acquire time */
    if (This->absolute == 0) {
      TRACE(" warping mouse back to (%ld , %ld)\n", This->org_coords.x, This->org_coords.y);
      SetCursorPos(This->org_coords.x, This->org_coords.y);
    }
	
    return DI_OK;
}

// if you call poll then to getdevicestate
// it did not send back right value in windows 
int poll_mouse=0;


static HRESULT WINAPI SysMouseAImpl_Poll(LPDIRECTINPUTDEVICE8A iface)
{
 int retValue = DI_OK;
 
 if (poll_mouse==0) {
	                 retValue=SysMouseAImpl_Acquire(iface);	
                     poll_mouse=1; 
					 if (retValue!=DI_OK) retValue=DIERR_NOTACQUIRED;
                     else retValue = DI_OK;
                     }

 return retValue;
}
	

/******************************************************************************
  *     GetDeviceState : returns the "state" of the mouse.
  *
  *   For the moment, only the "standard" return structure (DIMOUSESTATE) is
  *   supported.
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE8A iface,DWORD len,LPVOID ptr
) {
    SysMouseImpl *This = (SysMouseImpl *)iface;

    if(This->acquired == 0) return DIERR_NOTACQUIRED;

    EnterCriticalSection(&(This->crit));
    TRACE("(this=%p,0x%08lx,%p):\n", This, len, ptr);
    TRACE("(X: %ld - Y: %ld - Z: %ld  L: %02x M: %02x R: %02x)\n",
	  This->m_state.lX, This->m_state.lY, This->m_state.lZ,
	  This->m_state.rgbButtons[0], This->m_state.rgbButtons[2], This->m_state.rgbButtons[1]);
    
    /* Copy the current mouse state */
    fill_DataFormat(ptr, &(This->m_state), This->wine_df);
    
#ifdef __REACTOS__
	// this fix windows bugs when 
	// some program calling on mouse poll
	if (poll_mouse==1) poll_mouse=0;		 
	else {
    if (This->absolute == 0) {
	This->m_state.lX = 0;
	This->m_state.lY = 0;
	This->m_state.lZ = 0;
    }
	     }
#endif	
    
    /* Check if we need to do a mouse warping */
    if (This->need_warp == WARP_NEEDED) {
	dinput_window_check(This);
	TRACE("Warping mouse to %ld - %ld\n", This->mapped_center.x, This->mapped_center.y);
        if (mouse_set==0){
	SetCursorPos( This->mapped_center.x, This->mapped_center.y );
           mouse_set++;
           }
	
#ifdef MOUSE_HACK
	This->need_warp = WARP_DONE;
#else
	This->need_warp = WARP_STARTED;
#endif
    }
    
    LeaveCriticalSection(&(This->crit));
    
    return DI_OK;
}

/******************************************************************************
  *     GetDeviceData : gets buffered input data.
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceData(LPDIRECTINPUTDEVICE8A iface,
						  DWORD dodsize,
						  LPDIDEVICEOBJECTDATA dod,
						  LPDWORD entries,
						  DWORD flags
) {
    SysMouseImpl *This = (SysMouseImpl *)iface;
    DWORD len;
    int nqtail = 0;
    
    TRACE("(%p)->(dods=%ld,dod=%p,entries=%p (%ld)%s,fl=0x%08lx%s)\n",This,dodsize,dod,
	  entries, *entries,*entries == INFINITE ? " (INFINITE)" : "",
	  flags, (flags & DIGDD_PEEK) ? " (DIGDD_PEEK)": "" );
    
    if (This->acquired == 0) {
	WARN(" application tries to get data from an unacquired device !\n");
	//return DIERR_NOTACQUIRED;

	// windows does not get any data if 
	// we do not call manual to mouse Acquire
	// this is only need if some apps calling on getdevice data direcly
	// in windows GetdeviceData does always update first the data
	// then return it.
	 SysMouseAImpl_Acquire(iface);
    }
    
    EnterCriticalSection(&(This->crit));

    len = ((This->queue_head < This->queue_tail) ? This->queue_len : 0)
	+ (This->queue_head - This->queue_tail);
    if ((*entries != INFINITE) && (len > *entries)) len = *entries;
    
    if (dod == NULL) {
	*entries = len;
	
	if (!(flags & DIGDD_PEEK)) {
	    if (len)
		TRACE("Application discarding %ld event(s).\n", len);
	    
	    nqtail = This->queue_tail + len;
	    while (nqtail >= This->queue_len) nqtail -= This->queue_len;
	} else {
	    TRACE("Telling application that %ld event(s) are in the queue.\n", len);
	}
    } else {
	if (dodsize < sizeof(DIDEVICEOBJECTDATA_DX3)) {
	    ERR("Wrong structure size !\n");
	    LeaveCriticalSection(&(This->crit));
	    return DIERR_INVALIDPARAM;
	}
	
	if (len)
	    TRACE("Application retrieving %ld event(s):\n", len);
	
	*entries = 0;
	nqtail = This->queue_tail;
	while (len) {
	    /* Copy the buffered data into the application queue */
	    TRACE(" - queuing Offs:%2ld Data:%5ld TS:%8ld Seq:%8ld at address %p from queue tail %4d\n",
		  (This->data_queue)->dwOfs,
		  (This->data_queue)->dwData,
		  (This->data_queue)->dwTimeStamp,
		  (This->data_queue)->dwSequence,
		  (char *)dod + *entries * dodsize,
		  nqtail);
	    memcpy((char *)dod + *entries * dodsize, This->data_queue + nqtail, dodsize);
	    /* Advance position */
	    nqtail++;
	    if (nqtail >= This->queue_len)
                nqtail -= This->queue_len;
	    (*entries)++;
	    len--;
	}
    }
    if (!(flags & DIGDD_PEEK))
	This->queue_tail = nqtail;
    
    LeaveCriticalSection(&(This->crit));
    
    /* Check if we need to do a mouse warping */
    if (This->need_warp == WARP_NEEDED) {
	dinput_window_check(This);
	TRACE("Warping mouse to %ld - %ld\n", This->mapped_center.x, This->mapped_center.y);
        if (mouse_set==0){
	SetCursorPos( This->mapped_center.x, This->mapped_center.y );
           mouse_set++;
           }
	
#ifdef MOUSE_HACK
	This->need_warp = WARP_DONE;
#else
	This->need_warp = WARP_STARTED;
#endif
    }
    return DI_OK;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI SysMouseAImpl_SetProperty(LPDIRECTINPUTDEVICE8A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
    
    if (!HIWORD(rguid)) {
	switch (LOWORD(rguid)) {
	    case (DWORD) DIPROP_BUFFERSIZE: {
		LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
		
		TRACE("buffersize = %ld\n",pd->dwData);
		
		This->data_queue = HeapAlloc(GetProcessHeap(),0, pd->dwData * sizeof(DIDEVICEOBJECTDATA));
		This->queue_head = 0;
		This->queue_tail = 0;
		This->queue_len  = pd->dwData;
		break;
	    }
	    case (DWORD) DIPROP_AXISMODE: {
		LPCDIPROPDWORD    pd = (LPCDIPROPDWORD)ph;
		This->absolute = !(pd->dwData);
		TRACE("Using %s coordinates mode now\n", This->absolute ? "absolute" : "relative");
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
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI SysMouseAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface,
						REFGUID rguid,
						LPDIPROPHEADER pdiph)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    TRACE("(this=%p,%s,%p)\n",
	  iface, debugstr_guid(rguid), pdiph);
    
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
	      
	    case (DWORD) DIPROP_GRANULARITY: {
		LPDIPROPDWORD pr = (LPDIPROPDWORD) pdiph;
		
		/* We'll just assume that the app asks about the Z axis */
		pr->dwData = WHEEL_DELTA;
		
		break;
	    }
	      
	    case (DWORD) DIPROP_RANGE: {
		LPDIPROPRANGE pr = (LPDIPROPRANGE) pdiph;
		
		if ((pdiph->dwHow == DIPH_BYID) &&
		    ((pdiph->dwObj == (DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS)) ||
		     (pdiph->dwObj == (DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS)))) {
		    /* Querying the range of either the X or the Y axis.  As I do
		       not know the range, do as if the range were
		       unrestricted...*/
		    pr->lMin = DIPROPRANGE_NOMIN;
		    pr->lMax = DIPROPRANGE_NOMAX;
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
static HRESULT WINAPI SysMouseAImpl_SetEventNotification(LPDIRECTINPUTDEVICE8A iface,
							 HANDLE hnd) {
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    TRACE("(this=%p,%p)\n",This,hnd);
    
    This->hEvent = hnd;
    
    return DI_OK;
}

/******************************************************************************
  *     GetCapabilities : get the device capablitites
  */
static HRESULT WINAPI SysMouseAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    DIDEVCAPS devcaps;

    TRACE("(this=%p,%p)\n",This,lpDIDevCaps);

    if ((lpDIDevCaps->dwSize != sizeof(DIDEVCAPS)) && (lpDIDevCaps->dwSize != sizeof(DIDEVCAPS_DX3))) {
        WARN("invalid parameter\n");
        return DIERR_INVALIDPARAM;
    }

    devcaps.dwSize = lpDIDevCaps->dwSize;
    devcaps.dwFlags = DIDC_ATTACHED;
    if (This->dinput->dwVersion >= 0x0800)
	devcaps.dwDevType = DI8DEVTYPE_MOUSE | (DI8DEVTYPEMOUSE_TRADITIONAL << 8);
    else
	devcaps.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_TRADITIONAL << 8);
    devcaps.dwAxes = 3;
    devcaps.dwButtons = 3;
    devcaps.dwPOVs = 0;
    devcaps.dwFFSamplePeriod = 0;
    devcaps.dwFFMinTimeResolution = 0;
    devcaps.dwFirmwareRevision = 100;
    devcaps.dwHardwareRevision = 100;
    devcaps.dwFFDriverVersion = 0;

    memcpy(lpDIDevCaps, &devcaps, lpDIDevCaps->dwSize);
    
    return DI_OK;
}


/******************************************************************************
  *     EnumObjects : enumerate the different buttons and axis...
  */
static HRESULT WINAPI SysMouseAImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    DIDEVICEOBJECTINSTANCEA ddoi;
    
    TRACE("(this=%p,%p,%p,%08lx)\n", This, lpCallback, lpvRef, dwFlags);
    if (TRACE_ON(dinput)) {
	TRACE("  - flags = ");
	_dump_EnumObjects_flags(dwFlags);
	TRACE("\n");
    }
    
    /* Only the fields till dwFFMaxForce are relevant */
    memset(&ddoi, 0, sizeof(ddoi));
    ddoi.dwSize = FIELD_OFFSET(DIDEVICEOBJECTINSTANCEA, dwFFMaxForce);
    
    /* In a mouse, we have : two relative axis and three buttons */
    if ((dwFlags == DIDFT_ALL) ||
	(dwFlags & DIDFT_AXIS)) {
	/* X axis */
	ddoi.guidType = GUID_XAxis;
	ddoi.dwOfs = This->offset_array[WINE_MOUSE_X_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS;
	strcpy(ddoi.tszName, "X-Axis");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
	
	/* Y axis */
	ddoi.guidType = GUID_YAxis;
	ddoi.dwOfs = This->offset_array[WINE_MOUSE_Y_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS;
	strcpy(ddoi.tszName, "Y-Axis");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
	
	/* Z axis */
	ddoi.guidType = GUID_ZAxis;
	ddoi.dwOfs = This->offset_array[WINE_MOUSE_Z_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_Z_AXIS_INSTANCE) | DIDFT_RELAXIS;
	strcpy(ddoi.tszName, "Z-Axis");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
    }

    if ((dwFlags == DIDFT_ALL) ||
	(dwFlags & DIDFT_BUTTON)) {
	ddoi.guidType = GUID_Button;
	
	/* Left button */
	ddoi.dwOfs = This->offset_array[WINE_MOUSE_L_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_L_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
	strcpy(ddoi.tszName, "Left-Button");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
	
	/* Right button */
	ddoi.dwOfs = This->offset_array[WINE_MOUSE_R_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_R_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
	strcpy(ddoi.tszName, "Right-Button");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
	
	/* Middle button */
	ddoi.dwOfs = This->offset_array[WINE_MOUSE_M_POSITION];
	ddoi.dwType = DIDFT_MAKEINSTANCE(WINE_MOUSE_M_BUTTON_INSTANCE) | DIDFT_PSHBUTTON;
	strcpy(ddoi.tszName, "Middle-Button");
	_dump_OBJECTINSTANCEA(&ddoi);
	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) return DI_OK;
    }
    
    return DI_OK;
}

static HRESULT WINAPI SysMouseWImpl_EnumObjects(LPDIRECTINPUTDEVICE8W iface, LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback,	LPVOID lpvRef,DWORD dwFlags)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    
    device_enumobjects_AtoWcb_data data;
    
    data.lpCallBack = lpCallback;
    data.lpvRef = lpvRef;
    
    return SysMouseAImpl_EnumObjects((LPDIRECTINPUTDEVICE8A) This, (LPDIENUMDEVICEOBJECTSCALLBACKA) DIEnumDevicesCallbackAtoW, (LPVOID) &data, dwFlags);
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEA)) {
        WARN(" dinput3 not supporte yet...\n");
	return DI_OK;
    }

    fill_mouse_dideviceinstanceA(pdidi, This->dinput->dwVersion);
    
    return DI_OK;
}

static HRESULT WINAPI SysMouseWImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8W iface, LPDIDEVICEINSTANCEW pdidi)
{
    SysMouseImpl *This = (SysMouseImpl *)iface;
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW)) {
        WARN(" dinput3 not supporte yet...\n");
	return DI_OK;
    }

    fill_mouse_dideviceinstanceW(pdidi, This->dinput->dwVersion);
    
    return DI_OK;
}


static const IDirectInputDevice8AVtbl SysMouseAvt =
{
    IDirectInputDevice2AImpl_QueryInterface,
    IDirectInputDevice2AImpl_AddRef,
    SysMouseAImpl_Release,
    SysMouseAImpl_GetCapabilities,
    SysMouseAImpl_EnumObjects,
    SysMouseAImpl_GetProperty,
    SysMouseAImpl_SetProperty,
    SysMouseAImpl_Acquire,
    SysMouseAImpl_Unacquire,
    SysMouseAImpl_GetDeviceState,
    SysMouseAImpl_GetDeviceData,
    SysMouseAImpl_SetDataFormat,
    SysMouseAImpl_SetEventNotification,
    SysMouseAImpl_SetCooperativeLevel,
    IDirectInputDevice2AImpl_GetObjectInfo,
    SysMouseAImpl_GetDeviceInfo,
    IDirectInputDevice2AImpl_RunControlPanel,
    IDirectInputDevice2AImpl_Initialize,
    IDirectInputDevice2AImpl_CreateEffect,
    IDirectInputDevice2AImpl_EnumEffects,
    IDirectInputDevice2AImpl_GetEffectInfo,
    IDirectInputDevice2AImpl_GetForceFeedbackState,
    IDirectInputDevice2AImpl_SendForceFeedbackCommand,
    IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
    IDirectInputDevice2AImpl_Escape,
    SysMouseAImpl_Poll,
    IDirectInputDevice2AImpl_SendDeviceData,
    IDirectInputDevice7AImpl_EnumEffectsInFile,
    IDirectInputDevice7AImpl_WriteEffectToFile,
    IDirectInputDevice8AImpl_BuildActionMap,
    IDirectInputDevice8AImpl_SetActionMap,
    IDirectInputDevice8AImpl_GetImageInfo
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(SysMouseWvt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static const IDirectInputDevice8WVtbl SysMouseWvt =
{
    IDirectInputDevice2WImpl_QueryInterface,
    XCAST(AddRef)IDirectInputDevice2AImpl_AddRef,
    XCAST(Release)SysMouseAImpl_Release,
    XCAST(GetCapabilities)SysMouseAImpl_GetCapabilities,
    SysMouseWImpl_EnumObjects,
    XCAST(GetProperty)SysMouseAImpl_GetProperty,
    XCAST(SetProperty)SysMouseAImpl_SetProperty,
    XCAST(Acquire)SysMouseAImpl_Acquire,
    XCAST(Unacquire)SysMouseAImpl_Unacquire,
    XCAST(GetDeviceState)SysMouseAImpl_GetDeviceState,
    XCAST(GetDeviceData)SysMouseAImpl_GetDeviceData,
    XCAST(SetDataFormat)SysMouseAImpl_SetDataFormat,
    XCAST(SetEventNotification)SysMouseAImpl_SetEventNotification,
    XCAST(SetCooperativeLevel)SysMouseAImpl_SetCooperativeLevel,
    IDirectInputDevice2WImpl_GetObjectInfo,
    SysMouseWImpl_GetDeviceInfo,
    XCAST(RunControlPanel)IDirectInputDevice2AImpl_RunControlPanel,
    XCAST(Initialize)IDirectInputDevice2AImpl_Initialize,
    XCAST(CreateEffect)IDirectInputDevice2AImpl_CreateEffect,
    IDirectInputDevice2WImpl_EnumEffects,
    IDirectInputDevice2WImpl_GetEffectInfo,
    XCAST(GetForceFeedbackState)IDirectInputDevice2AImpl_GetForceFeedbackState,
    XCAST(SendForceFeedbackCommand)IDirectInputDevice2AImpl_SendForceFeedbackCommand,
    XCAST(EnumCreatedEffectObjects)IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
    XCAST(Escape)IDirectInputDevice2AImpl_Escape,
    XCAST(Poll)SysMouseAImpl_Poll,
    XCAST(SendDeviceData)IDirectInputDevice2AImpl_SendDeviceData,
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    IDirectInputDevice8WImpl_BuildActionMap,
    IDirectInputDevice8WImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};
#undef XCAST
