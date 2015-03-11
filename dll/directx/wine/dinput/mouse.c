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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dinput_private.h"

/* Wine mouse driver object instances */
#define WINE_MOUSE_X_AXIS_INSTANCE   0
#define WINE_MOUSE_Y_AXIS_INSTANCE   1
#define WINE_MOUSE_Z_AXIS_INSTANCE   2
#define WINE_MOUSE_BUTTONS_INSTANCE  3

static const IDirectInputDevice8AVtbl SysMouseAvt;
static const IDirectInputDevice8WVtbl SysMouseWvt;

typedef struct SysMouseImpl SysMouseImpl;

typedef enum
{
    WARP_DEFAULT,
    WARP_DISABLE,
    WARP_FORCE_ON
} WARP_MOUSE;

struct SysMouseImpl
{
    struct IDirectInputDeviceImpl   base;

    /* SysMouseAImpl */
    /* These are used in case of relative -> absolute transitions */
    POINT                           org_coords;
    BOOL                            clipped;
    /* warping: whether we need to move mouse back to middle once we
     * reach window borders (for e.g. shooters, "surface movement" games) */
    BOOL                            need_warp;
    DWORD                           last_warped;

    /* This is for mouse reporting. */
    DIMOUSESTATE2                   m_state;

    WARP_MOUSE                      warp_override;
};

static inline SysMouseImpl *impl_from_IDirectInputDevice8A(IDirectInputDevice8A *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8A_iface), SysMouseImpl, base);
}
static inline SysMouseImpl *impl_from_IDirectInputDevice8W(IDirectInputDevice8W *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface), SysMouseImpl, base);
}

static inline IDirectInputDevice8W *IDirectInputDevice8W_from_impl(SysMouseImpl *This)
{
    return &This->base.IDirectInputDevice8W_iface;
}

static int dinput_mouse_hook( LPDIRECTINPUTDEVICE8A iface, WPARAM wparam, LPARAM lparam );

const GUID DInput_Wine_Mouse_GUID = { /* 9e573ed8-7734-11d2-8d4a-23903fb6bdf7 */
    0x9e573ed8, 0x7734, 0x11d2, {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};

static void _dump_mouse_state(const DIMOUSESTATE2 *m_state)
{
    int i;

    if (!TRACE_ON(dinput)) return;

    TRACE("(X: %d Y: %d Z: %d", m_state->lX, m_state->lY, m_state->lZ);
    for (i = 0; i < 5; i++) TRACE(" B%d: %02x", i, m_state->rgbButtons[i]);
    TRACE(")\n");
}

static void fill_mouse_dideviceinstanceA(LPDIDEVICEINSTANCEA lpddi, DWORD version) {
    DWORD dwSize;
    DIDEVICEINSTANCEA ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    
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

    TRACE("%d %p\n", dwSize, lpddi);
    
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

static HRESULT mousedev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
    if (id != 0)
        return E_FAIL;

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_MOUSE) && (version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_POINTER) || (dwDevType == DI8DEVTYPE_MOUSE)) && (version >= 0x0800))) {
	TRACE("Enumerating the mouse device\n");
	
	fill_mouse_dideviceinstanceA(lpddi, version);
	
	return S_OK;
    }
    
    return S_FALSE;
}

static HRESULT mousedev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    if (id != 0)
        return E_FAIL;

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_MOUSE) && (version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_POINTER) || (dwDevType == DI8DEVTYPE_MOUSE)) && (version >= 0x0800))) {
	TRACE("Enumerating the mouse device\n");
	
	fill_mouse_dideviceinstanceW(lpddi, version);
	
	return S_OK;
    }
    
    return S_FALSE;
}

static SysMouseImpl *alloc_device(REFGUID rguid, IDirectInputImpl *dinput)
{
    SysMouseImpl* newDevice;
    LPDIDATAFORMAT df = NULL;
    unsigned i;
    char buffer[20];
    HKEY hkey, appkey;

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysMouseImpl));
    if (!newDevice) return NULL;
    newDevice->base.IDirectInputDevice8A_iface.lpVtbl = &SysMouseAvt;
    newDevice->base.IDirectInputDevice8W_iface.lpVtbl = &SysMouseWvt;
    newDevice->base.ref = 1;
    newDevice->base.dwCoopLevel = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
    newDevice->base.guid = *rguid;
    InitializeCriticalSection(&newDevice->base.crit);
    newDevice->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": SysMouseImpl*->base.crit");
    newDevice->base.dinput = dinput;
    newDevice->base.event_proc = dinput_mouse_hook;

    get_app_key(&hkey, &appkey);
    if (!get_config_key(hkey, appkey, "MouseWarpOverride", buffer, sizeof(buffer)))
    {
        if (!strcasecmp(buffer, "disable"))
            newDevice->warp_override = WARP_DISABLE;
        else if (!strcasecmp(buffer, "force"))
            newDevice->warp_override = WARP_FORCE_ON;
    }
    if (appkey) RegCloseKey(appkey);
    if (hkey) RegCloseKey(hkey);

    /* Create copy of default data format */
    if (!(df = HeapAlloc(GetProcessHeap(), 0, c_dfDIMouse2.dwSize))) goto failed;
    memcpy(df, &c_dfDIMouse2, c_dfDIMouse2.dwSize);
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto failed;
    memcpy(df->rgodf, c_dfDIMouse2.rgodf, df->dwNumObjs * df->dwObjSize);

    /* Because we don't do any detection yet just modify instance and type */
    for (i = 0; i < df->dwNumObjs; i++)
        if (DIDFT_GETTYPE(df->rgodf[i].dwType) & DIDFT_AXIS)
            df->rgodf[i].dwType = DIDFT_MAKEINSTANCE(i) | DIDFT_RELAXIS;
        else
            df->rgodf[i].dwType = DIDFT_MAKEINSTANCE(i) | DIDFT_PSHBUTTON;

    newDevice->base.data_format.wine_df = df;
    IDirectInput_AddRef(&newDevice->base.dinput->IDirectInput7A_iface);

    EnterCriticalSection(&dinput->crit);
    list_add_tail(&dinput->devices_list, &newDevice->base.entry);
    LeaveCriticalSection(&dinput->crit);

    return newDevice;

failed:
    if (df) HeapFree(GetProcessHeap(), 0, df->rgodf);
    HeapFree(GetProcessHeap(), 0, df);
    HeapFree(GetProcessHeap(), 0, newDevice);
    return NULL;
}

static HRESULT mousedev_create_device(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPVOID *pdev, int unicode)
{
    TRACE("%p %s %s %p %i\n", dinput, debugstr_guid(rguid), debugstr_guid(riid), pdev, unicode);
    *pdev = NULL;

    if (IsEqualGUID(&GUID_SysMouse, rguid) ||        /* Generic Mouse */
        IsEqualGUID(&DInput_Wine_Mouse_GUID, rguid)) /* Wine Mouse */
    {
        SysMouseImpl *This;

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

        This = alloc_device(rguid, dinput);
        TRACE("Created a Mouse device (%p)\n", This);

        if (!This) return DIERR_OUTOFMEMORY;

        if (unicode)
            *pdev = &This->base.IDirectInputDevice8W_iface;
        else
            *pdev = &This->base.IDirectInputDevice8A_iface;

        return DI_OK;
    }

    return DIERR_DEVICENOTREG;
}

const struct dinput_device mouse_device = {
    "Wine mouse driver",
    mousedev_enum_deviceA,
    mousedev_enum_deviceW,
    mousedev_create_device
};

/******************************************************************************
 *	SysMouseA (DInput Mouse support)
 */

/* low-level mouse hook */
static int dinput_mouse_hook( LPDIRECTINPUTDEVICE8A iface, WPARAM wparam, LPARAM lparam )
{
    MSLLHOOKSTRUCT *hook = (MSLLHOOKSTRUCT *)lparam;
    SysMouseImpl* This = impl_from_IDirectInputDevice8A(iface);
    int wdata = 0, inst_id = -1, ret = 0;

    TRACE("msg %lx @ (%d %d)\n", wparam, hook->pt.x, hook->pt.y);

    EnterCriticalSection(&This->base.crit);

    switch(wparam) {
        case WM_MOUSEMOVE:
        {
            POINT pt, pt1;

            GetCursorPos(&pt);
            This->m_state.lX += pt.x = hook->pt.x - pt.x;
            This->m_state.lY += pt.y = hook->pt.y - pt.y;

            if (This->base.data_format.user_df->dwFlags & DIDF_ABSAXIS)
            {
                pt1.x = This->m_state.lX;
                pt1.y = This->m_state.lY;
            } else
                pt1 = pt;

            if (pt.x)
            {
                inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS;
                wdata = pt1.x;
            }
            if (pt.y)
            {
                /* Already have X, need to queue it */
                if (inst_id != -1)
                    queue_event(iface, inst_id,
                                wdata, GetCurrentTime(), This->base.dinput->evsequence);
                inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS;
                wdata = pt1.y;
            }

            if (pt.x || pt.y)
            {
                if ((This->warp_override == WARP_FORCE_ON) ||
                    (This->warp_override != WARP_DISABLE && (This->base.dwCoopLevel & DISCL_EXCLUSIVE)))
                    This->need_warp = TRUE;
            }
            break;
        }
        case WM_MOUSEWHEEL:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_Z_AXIS_INSTANCE) | DIDFT_RELAXIS;
            This->m_state.lZ += wdata = (short)HIWORD(hook->mouseData);
            /* FarCry crashes if it gets a mouse wheel message */
            /* FIXME: should probably filter out other messages too */
            ret = This->clipped;
            break;
        case WM_LBUTTONDOWN:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 0) | DIDFT_PSHBUTTON;
            This->m_state.rgbButtons[0] = wdata = 0x80;
	    break;
	case WM_LBUTTONUP:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 0) | DIDFT_PSHBUTTON;
            This->m_state.rgbButtons[0] = wdata = 0x00;
	    break;
	case WM_RBUTTONDOWN:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 1) | DIDFT_PSHBUTTON;
            This->m_state.rgbButtons[1] = wdata = 0x80;
	    break;
	case WM_RBUTTONUP:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 1) | DIDFT_PSHBUTTON;
            This->m_state.rgbButtons[1] = wdata = 0x00;
	    break;
	case WM_MBUTTONDOWN:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 2) | DIDFT_PSHBUTTON;
            This->m_state.rgbButtons[2] = wdata = 0x80;
	    break;
	case WM_MBUTTONUP:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 2) | DIDFT_PSHBUTTON;
            This->m_state.rgbButtons[2] = wdata = 0x00;
	    break;
        case WM_XBUTTONDOWN:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 2 + HIWORD(hook->mouseData)) | DIDFT_PSHBUTTON;
            This->m_state.rgbButtons[2 + HIWORD(hook->mouseData)] = wdata = 0x80;
            break;
        case WM_XBUTTONUP:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 2 + HIWORD(hook->mouseData)) | DIDFT_PSHBUTTON;
            This->m_state.rgbButtons[2 + HIWORD(hook->mouseData)] = wdata = 0x00;
            break;
    }


    if (inst_id != -1)
    {
        _dump_mouse_state(&This->m_state);
        queue_event(iface, inst_id,
                    wdata, GetCurrentTime(), This->base.dinput->evsequence++);
    }

    LeaveCriticalSection(&This->base.crit);
    return ret;
}

static void warp_check( SysMouseImpl* This, BOOL force )
{
    DWORD now = GetCurrentTime();
    const DWORD interval = This->clipped ? 500 : 10;

    if (force || (This->need_warp && (now - This->last_warped > interval)))
    {
        RECT rect, new_rect;
        POINT mapped_center;

        This->last_warped = now;
        This->need_warp = FALSE;
        if (!GetClientRect(This->base.win, &rect)) return;
        MapWindowPoints( This->base.win, 0, (POINT *)&rect, 2 );
        if (!This->clipped)
        {
            mapped_center.x = (rect.left + rect.right) / 2;
            mapped_center.y = (rect.top + rect.bottom) / 2;
            TRACE("Warping mouse to %d - %d\n", mapped_center.x, mapped_center.y);
            SetCursorPos( mapped_center.x, mapped_center.y );
        }
        if (This->base.dwCoopLevel & DISCL_EXCLUSIVE)
        {
            /* make sure we clip even if the window covers the whole screen */
            rect.left = max( rect.left, GetSystemMetrics( SM_XVIRTUALSCREEN ) + 1 );
            rect.top = max( rect.top, GetSystemMetrics( SM_YVIRTUALSCREEN ) + 1 );
            rect.right = min( rect.right, rect.left + GetSystemMetrics( SM_CXVIRTUALSCREEN ) - 2 );
            rect.bottom = min( rect.bottom, rect.top + GetSystemMetrics( SM_CYVIRTUALSCREEN ) - 2 );
            TRACE("Clipping mouse to %s\n", wine_dbgstr_rect( &rect ));
            ClipCursor( &rect );
            This->clipped = GetClipCursor( &new_rect ) && EqualRect( &rect, &new_rect );
        }
    }
}


/******************************************************************************
  *     Acquire : gets exclusive control of the mouse
  */
static HRESULT WINAPI SysMouseWImpl_Acquire(LPDIRECTINPUTDEVICE8W iface)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8W(iface);
    POINT point;
    HRESULT res;

    TRACE("(this=%p)\n",This);

    if ((res = IDirectInputDevice2WImpl_Acquire(iface)) != DI_OK) return res;

    /* Init the mouse state */
    GetCursorPos( &point );
    if (This->base.data_format.user_df->dwFlags & DIDF_ABSAXIS)
    {
      This->m_state.lX = point.x;
      This->m_state.lY = point.y;
    } else {
      This->m_state.lX = 0;
      This->m_state.lY = 0;
      This->org_coords = point;
    }
    This->m_state.lZ = 0;
    This->m_state.rgbButtons[0] = GetKeyState(VK_LBUTTON) & 0x80;
    This->m_state.rgbButtons[1] = GetKeyState(VK_RBUTTON) & 0x80;
    This->m_state.rgbButtons[2] = GetKeyState(VK_MBUTTON) & 0x80;

    if (This->base.dwCoopLevel & DISCL_EXCLUSIVE)
    {
        ShowCursor(FALSE); /* hide cursor */
        warp_check( This, TRUE );
    }
    else if (This->warp_override == WARP_FORCE_ON)
    {
        /* Need a window to warp mouse in. */
        if (!This->base.win) This->base.win = GetDesktopWindow();
        warp_check( This, TRUE );
    }
    else if (This->clipped)
    {
        ClipCursor( NULL );
        This->clipped = FALSE;
    }

    return DI_OK;
}

static HRESULT WINAPI SysMouseAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
    return SysMouseWImpl_Acquire(IDirectInputDevice8W_from_impl(This));
}

/******************************************************************************
  *     Unacquire : frees the mouse
  */
static HRESULT WINAPI SysMouseWImpl_Unacquire(LPDIRECTINPUTDEVICE8W iface)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;

    TRACE("(this=%p)\n",This);

    if ((res = IDirectInputDevice2WImpl_Unacquire(iface)) != DI_OK) return res;

    if (This->base.dwCoopLevel & DISCL_EXCLUSIVE)
    {
        ClipCursor(NULL);
        ShowCursor(TRUE); /* show cursor */
        This->clipped = FALSE;
    }

    /* And put the mouse cursor back where it was at acquire time */
    if (This->base.dwCoopLevel & DISCL_EXCLUSIVE || This->warp_override == WARP_FORCE_ON)
    {
        TRACE("warping mouse back to %s\n", wine_dbgstr_point(&This->org_coords));
        SetCursorPos(This->org_coords.x, This->org_coords.y);
    }

    return DI_OK;
}

static HRESULT WINAPI SysMouseAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
    return SysMouseWImpl_Unacquire(IDirectInputDevice8W_from_impl(This));
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the mouse.
  *
  *   For the moment, only the "standard" return structure (DIMOUSESTATE) is
  *   supported.
  */
static HRESULT WINAPI SysMouseWImpl_GetDeviceState(LPDIRECTINPUTDEVICE8W iface, DWORD len, LPVOID ptr)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8W(iface);

    if(This->base.acquired == 0) return DIERR_NOTACQUIRED;

#ifndef __REACTOS__
    __wine_check_for_events( QS_ALLINPUT );
#endif

    TRACE("(this=%p,0x%08x,%p):\n", This, len, ptr);
    _dump_mouse_state(&This->m_state);

    EnterCriticalSection(&This->base.crit);
    /* Copy the current mouse state */
    fill_DataFormat(ptr, len, &This->m_state, &This->base.data_format);

    /* Initialize the buffer when in relative mode */
    if (!(This->base.data_format.user_df->dwFlags & DIDF_ABSAXIS))
    {
	This->m_state.lX = 0;
	This->m_state.lY = 0;
	This->m_state.lZ = 0;
    }
    LeaveCriticalSection(&This->base.crit);

    warp_check( This, FALSE );
    return DI_OK;
}

static HRESULT WINAPI SysMouseAImpl_GetDeviceState(LPDIRECTINPUTDEVICE8A iface, DWORD len, LPVOID ptr)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
    return SysMouseWImpl_GetDeviceState(IDirectInputDevice8W_from_impl(This), len, ptr);
}

/******************************************************************************
  *     GetDeviceData : gets buffered input data.
  */
static HRESULT WINAPI SysMouseWImpl_GetDeviceData(LPDIRECTINPUTDEVICE8W iface,
        DWORD dodsize, LPDIDEVICEOBJECTDATA dod, LPDWORD entries, DWORD flags)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;

    res = IDirectInputDevice2WImpl_GetDeviceData(iface, dodsize, dod, entries, flags);
    if (SUCCEEDED(res)) warp_check( This, FALSE );
    return res;
}

static HRESULT WINAPI SysMouseAImpl_GetDeviceData(LPDIRECTINPUTDEVICE8A iface,
        DWORD dodsize, LPDIDEVICEOBJECTDATA dod, LPDWORD entries, DWORD flags)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
    return SysMouseWImpl_GetDeviceData(IDirectInputDevice8W_from_impl(This), dodsize, dod, entries, flags);
}

/******************************************************************************
  *     GetProperty : get input device properties
  */
static HRESULT WINAPI SysMouseWImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p) %s,%p\n", This, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (IS_DIPROP(rguid)) {
	switch (LOWORD(rguid)) {
	    case (DWORD_PTR) DIPROP_GRANULARITY: {
		LPDIPROPDWORD pr = (LPDIPROPDWORD) pdiph;
		
		/* We'll just assume that the app asks about the Z axis */
		pr->dwData = WHEEL_DELTA;
		
		break;
	    }
	      
	    case (DWORD_PTR) DIPROP_RANGE: {
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
                return IDirectInputDevice2WImpl_GetProperty(iface, rguid, pdiph);
        }
    }

    return DI_OK;
}

static HRESULT WINAPI SysMouseAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
    return SysMouseWImpl_GetProperty(IDirectInputDevice8W_from_impl(This), rguid, pdiph);
}

/******************************************************************************
  *     GetCapabilities : get the device capabilities
  */
static HRESULT WINAPI SysMouseWImpl_GetCapabilities(LPDIRECTINPUTDEVICE8W iface, LPDIDEVCAPS lpDIDevCaps)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8W(iface);
    DIDEVCAPS devcaps;

    TRACE("(this=%p,%p)\n",This,lpDIDevCaps);

    if ((lpDIDevCaps->dwSize != sizeof(DIDEVCAPS)) && (lpDIDevCaps->dwSize != sizeof(DIDEVCAPS_DX3))) {
        WARN("invalid parameter\n");
        return DIERR_INVALIDPARAM;
    }

    devcaps.dwSize = lpDIDevCaps->dwSize;
    devcaps.dwFlags = DIDC_ATTACHED | DIDC_EMULATED;
    if (This->base.dinput->dwVersion >= 0x0800)
	devcaps.dwDevType = DI8DEVTYPE_MOUSE | (DI8DEVTYPEMOUSE_TRADITIONAL << 8);
    else
	devcaps.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_TRADITIONAL << 8);
    devcaps.dwAxes = 3;
    devcaps.dwButtons = 8;
    devcaps.dwPOVs = 0;
    devcaps.dwFFSamplePeriod = 0;
    devcaps.dwFFMinTimeResolution = 0;
    devcaps.dwFirmwareRevision = 100;
    devcaps.dwHardwareRevision = 100;
    devcaps.dwFFDriverVersion = 0;

    memcpy(lpDIDevCaps, &devcaps, lpDIDevCaps->dwSize);

    return DI_OK;
}

static HRESULT WINAPI SysMouseAImpl_GetCapabilities(LPDIRECTINPUTDEVICE8A iface, LPDIDEVCAPS lpDIDevCaps)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
    return SysMouseWImpl_GetCapabilities(IDirectInputDevice8W_from_impl(This), lpDIDevCaps);
}

/******************************************************************************
  *     GetObjectInfo : get information about a device object such as a button
  *                     or axis
  */
static HRESULT WINAPI SysMouseWImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface,
        LPDIDEVICEOBJECTINSTANCEW pdidoi, DWORD dwObj, DWORD dwHow)
{
    static const WCHAR x_axisW[] = {'X','-','A','x','i','s',0};
    static const WCHAR y_axisW[] = {'Y','-','A','x','i','s',0};
    static const WCHAR wheelW[] = {'W','h','e','e','l',0};
    static const WCHAR buttonW[] = {'B','u','t','t','o','n',' ','%','d',0};
    HRESULT res;

    res = IDirectInputDevice2WImpl_GetObjectInfo(iface, pdidoi, dwObj, dwHow);
    if (res != DI_OK) return res;

    if      (IsEqualGUID(&pdidoi->guidType, &GUID_XAxis)) strcpyW(pdidoi->tszName, x_axisW);
    else if (IsEqualGUID(&pdidoi->guidType, &GUID_YAxis)) strcpyW(pdidoi->tszName, y_axisW);
    else if (IsEqualGUID(&pdidoi->guidType, &GUID_ZAxis)) strcpyW(pdidoi->tszName, wheelW);
    else if (pdidoi->dwType & DIDFT_BUTTON)
        wsprintfW(pdidoi->tszName, buttonW, DIDFT_GETINSTANCE(pdidoi->dwType) - 3);

    _dump_OBJECTINSTANCEW(pdidoi);
    return res;
}

static HRESULT WINAPI SysMouseAImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8A iface,
        LPDIDEVICEOBJECTINSTANCEA pdidoi, DWORD dwObj, DWORD dwHow)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
    HRESULT res;
    DIDEVICEOBJECTINSTANCEW didoiW;
    DWORD dwSize = pdidoi->dwSize;

    didoiW.dwSize = sizeof(didoiW);
    res = SysMouseWImpl_GetObjectInfo(IDirectInputDevice8W_from_impl(This), &didoiW, dwObj, dwHow);
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
static HRESULT WINAPI SysMouseAImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEA)) {
        WARN(" dinput3 not supported yet...\n");
	return DI_OK;
    }

    fill_mouse_dideviceinstanceA(pdidi, This->base.dinput->dwVersion);
    
    return DI_OK;
}

static HRESULT WINAPI SysMouseWImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8W iface, LPDIDEVICEINSTANCEW pdidi)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8W(iface);
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW)) {
        WARN(" dinput3 not supported yet...\n");
	return DI_OK;
    }

    fill_mouse_dideviceinstanceW(pdidi, This->base.dinput->dwVersion);
    
    return DI_OK;
}

static HRESULT WINAPI SysMouseWImpl_BuildActionMap(LPDIRECTINPUTDEVICE8W iface,
                                                   LPDIACTIONFORMATW lpdiaf,
                                                   LPCWSTR lpszUserName,
                                                   DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    return _build_action_map(iface, lpdiaf, lpszUserName, dwFlags, DIMOUSE_MASK, &c_dfDIMouse2);
}

static HRESULT WINAPI SysMouseAImpl_BuildActionMap(LPDIRECTINPUTDEVICE8A iface,
                                                   LPDIACTIONFORMATA lpdiaf,
                                                   LPCSTR lpszUserName,
                                                   DWORD dwFlags)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
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

    hr = SysMouseWImpl_BuildActionMap(&This->base.IDirectInputDevice8W_iface, &diafW, lpszUserNameW, dwFlags);

    _copy_diactionformatWtoA(lpdiaf, &diafW);
    HeapFree(GetProcessHeap(), 0, diafW.rgoAction);
    HeapFree(GetProcessHeap(), 0, lpszUserNameW);

    return hr;
}

static HRESULT WINAPI SysMouseWImpl_SetActionMap(LPDIRECTINPUTDEVICE8W iface,
                                                 LPDIACTIONFORMATW lpdiaf,
                                                 LPCWSTR lpszUserName,
                                                 DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    return _set_action_map(iface, lpdiaf, lpszUserName, dwFlags, &c_dfDIMouse2);
}

static HRESULT WINAPI SysMouseAImpl_SetActionMap(LPDIRECTINPUTDEVICE8A iface,
                                                 LPDIACTIONFORMATA lpdiaf,
                                                 LPCSTR lpszUserName,
                                                 DWORD dwFlags)
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8A(iface);
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

    hr = SysMouseWImpl_SetActionMap(&This->base.IDirectInputDevice8W_iface, &diafW, lpszUserNameW, dwFlags);

    HeapFree(GetProcessHeap(), 0, diafW.rgoAction);
    HeapFree(GetProcessHeap(), 0, lpszUserNameW);

    return hr;
}

static const IDirectInputDevice8AVtbl SysMouseAvt =
{
    IDirectInputDevice2AImpl_QueryInterface,
    IDirectInputDevice2AImpl_AddRef,
    IDirectInputDevice2AImpl_Release,
    SysMouseAImpl_GetCapabilities,
    IDirectInputDevice2AImpl_EnumObjects,
    SysMouseAImpl_GetProperty,
    IDirectInputDevice2AImpl_SetProperty,
    SysMouseAImpl_Acquire,
    SysMouseAImpl_Unacquire,
    SysMouseAImpl_GetDeviceState,
    SysMouseAImpl_GetDeviceData,
    IDirectInputDevice2AImpl_SetDataFormat,
    IDirectInputDevice2AImpl_SetEventNotification,
    IDirectInputDevice2AImpl_SetCooperativeLevel,
    SysMouseAImpl_GetObjectInfo,
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
    IDirectInputDevice2AImpl_Poll,
    IDirectInputDevice2AImpl_SendDeviceData,
    IDirectInputDevice7AImpl_EnumEffectsInFile,
    IDirectInputDevice7AImpl_WriteEffectToFile,
    SysMouseAImpl_BuildActionMap,
    SysMouseAImpl_SetActionMap,
    IDirectInputDevice8AImpl_GetImageInfo
};

static const IDirectInputDevice8WVtbl SysMouseWvt =
{
    IDirectInputDevice2WImpl_QueryInterface,
    IDirectInputDevice2WImpl_AddRef,
    IDirectInputDevice2WImpl_Release,
    SysMouseWImpl_GetCapabilities,
    IDirectInputDevice2WImpl_EnumObjects,
    SysMouseWImpl_GetProperty,
    IDirectInputDevice2WImpl_SetProperty,
    SysMouseWImpl_Acquire,
    SysMouseWImpl_Unacquire,
    SysMouseWImpl_GetDeviceState,
    SysMouseWImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    SysMouseWImpl_GetObjectInfo,
    SysMouseWImpl_GetDeviceInfo,
    IDirectInputDevice2WImpl_RunControlPanel,
    IDirectInputDevice2WImpl_Initialize,
    IDirectInputDevice2WImpl_CreateEffect,
    IDirectInputDevice2WImpl_EnumEffects,
    IDirectInputDevice2WImpl_GetEffectInfo,
    IDirectInputDevice2WImpl_GetForceFeedbackState,
    IDirectInputDevice2WImpl_SendForceFeedbackCommand,
    IDirectInputDevice2WImpl_EnumCreatedEffectObjects,
    IDirectInputDevice2WImpl_Escape,
    IDirectInputDevice2WImpl_Poll,
    IDirectInputDevice2WImpl_SendDeviceData,
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    SysMouseWImpl_BuildActionMap,
    SysMouseWImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};
