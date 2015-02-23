/*		DirectInput Keyboard device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2005 Raphael Junqueira
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

#define WINE_DINPUT_KEYBOARD_MAX_KEYS 256

static const IDirectInputDevice8AVtbl SysKeyboardAvt;
static const IDirectInputDevice8WVtbl SysKeyboardWvt;

typedef struct SysKeyboardImpl SysKeyboardImpl;
struct SysKeyboardImpl
{
    struct IDirectInputDeviceImpl base;
    BYTE DInputKeyState[WINE_DINPUT_KEYBOARD_MAX_KEYS];
};

static inline SysKeyboardImpl *impl_from_IDirectInputDevice8A(IDirectInputDevice8A *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8A_iface), SysKeyboardImpl, base);
}
static inline SysKeyboardImpl *impl_from_IDirectInputDevice8W(IDirectInputDevice8W *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface), SysKeyboardImpl, base);
}
static inline IDirectInputDevice8A *IDirectInputDevice8A_from_impl(SysKeyboardImpl *This)
{
    return &This->base.IDirectInputDevice8A_iface;
}
static inline IDirectInputDevice8W *IDirectInputDevice8W_from_impl(SysKeyboardImpl *This)
{
    return &This->base.IDirectInputDevice8W_iface;
}

static BYTE map_dik_code(DWORD scanCode, DWORD vkCode)
{
    static const BYTE asciiCodes[] =
     {/*32*/  DIK_SPACE,0,0,0,0,0,0,DIK_APOSTROPHE,
      /*40*/  0,0,0,0,DIK_COMMA,DIK_MINUS,DIK_PERIOD,DIK_SLASH,
      /*48*/  DIK_0,DIK_1,DIK_2,DIK_3,DIK_4,DIK_5,DIK_6,DIK_7,
      /*56*/  DIK_8,DIK_9,DIK_COLON,DIK_SEMICOLON,0,DIK_EQUALS,0,0,
      /*64*/  DIK_AT,DIK_A,DIK_B,DIK_C,DIK_D,DIK_E,DIK_F,DIK_G,
      /*72*/  DIK_H,DIK_I,DIK_J,DIK_K,DIK_L,DIK_M,DIK_N,DIK_O,
      /*80*/  DIK_P,DIK_Q,DIK_R,DIK_S,DIK_T,DIK_U,DIK_V,DIK_W,
      /*88*/  DIK_X,DIK_Y,DIK_Z,DIK_LBRACKET,0,DIK_RBRACKET,DIK_CIRCUMFLEX,DIK_UNDERLINE}      /*95*/ ;

    BYTE out_code = 0;
    WCHAR c = MapVirtualKeyW(vkCode,MAPVK_VK_TO_CHAR);

    if (c > 31 && c < 96)
        out_code = asciiCodes[c - 32];

    if (out_code == 0)
        out_code = scanCode;

    return out_code;
}

static int KeyboardCallback( LPDIRECTINPUTDEVICE8A iface, WPARAM wparam, LPARAM lparam )
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8A(iface);
    int dik_code, ret = This->base.dwCoopLevel & DISCL_EXCLUSIVE;
    KBDLLHOOKSTRUCT *hook = (KBDLLHOOKSTRUCT *)lparam;
    BYTE new_diks;

    if (wparam != WM_KEYDOWN && wparam != WM_KEYUP &&
        wparam != WM_SYSKEYDOWN && wparam != WM_SYSKEYUP)
        return 0;

    TRACE("(%p) %ld,%ld\n", iface, wparam, lparam);

    switch (hook->vkCode)
    {
        /* R-Shift is special - it is an extended key with separate scan code */
        case VK_RSHIFT  : dik_code = DIK_RSHIFT; break;
        case VK_PAUSE   : dik_code = DIK_PAUSE; break;
        case VK_NUMLOCK : dik_code = DIK_NUMLOCK; break;
        case VK_SUBTRACT: dik_code = DIK_SUBTRACT; break;
        default:
            dik_code = map_dik_code(hook->scanCode & 0xff, hook->vkCode);
            if (hook->flags & LLKHF_EXTENDED) dik_code |= 0x80;
    }
    new_diks = hook->flags & LLKHF_UP ? 0 : 0x80;

    /* returns now if key event already known */
    if (new_diks == This->DInputKeyState[dik_code])
        return ret;

    This->DInputKeyState[dik_code] = new_diks;
    TRACE(" setting %02X to %02X\n", dik_code, This->DInputKeyState[dik_code]);

    EnterCriticalSection(&This->base.crit);
    queue_event(iface, DIDFT_MAKEINSTANCE(dik_code) | DIDFT_PSHBUTTON,
                new_diks, GetCurrentTime(), This->base.dinput->evsequence++);
    LeaveCriticalSection(&This->base.crit);

    return ret;
}

const GUID DInput_Wine_Keyboard_GUID = { /* 0ab8648a-7735-11d2-8c73-71df54a96441 */
    0x0ab8648a, 0x7735, 0x11d2, {0x8c, 0x73, 0x71, 0xdf, 0x54, 0xa9, 0x64, 0x41}
};

static void fill_keyboard_dideviceinstanceA(LPDIDEVICEINSTANCEA lpddi, DWORD version) {
    DWORD dwSize;
    DIDEVICEINSTANCEA ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    
    memset(lpddi, 0, dwSize);
    memset(&ddi, 0, sizeof(ddi));

    ddi.dwSize = dwSize;
    ddi.guidInstance = GUID_SysKeyboard;/* DInput's GUID */
    ddi.guidProduct = DInput_Wine_Keyboard_GUID; /* Vendor's GUID */
    if (version >= 0x0800)
        ddi.dwDevType = DI8DEVTYPE_KEYBOARD | (DI8DEVTYPEKEYBOARD_UNKNOWN << 8);
    else
        ddi.dwDevType = DIDEVTYPE_KEYBOARD | (DIDEVTYPEKEYBOARD_UNKNOWN << 8);
    strcpy(ddi.tszInstanceName, "Keyboard");
    strcpy(ddi.tszProductName, "Wine Keyboard");

    memcpy(lpddi, &ddi, (dwSize < sizeof(ddi) ? dwSize : sizeof(ddi)));
}

static void fill_keyboard_dideviceinstanceW(LPDIDEVICEINSTANCEW lpddi, DWORD version) {
    DWORD dwSize;
    DIDEVICEINSTANCEW ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    
    memset(lpddi, 0, dwSize);
    memset(&ddi, 0, sizeof(ddi));
 
    ddi.dwSize = dwSize;
    ddi.guidInstance = GUID_SysKeyboard;/* DInput's GUID */
    ddi.guidProduct = DInput_Wine_Keyboard_GUID; /* Vendor's GUID */
    if (version >= 0x0800)
        ddi.dwDevType = DI8DEVTYPE_KEYBOARD | (DI8DEVTYPEKEYBOARD_UNKNOWN << 8);
    else
        ddi.dwDevType = DIDEVTYPE_KEYBOARD | (DIDEVTYPEKEYBOARD_UNKNOWN << 8);
    MultiByteToWideChar(CP_ACP, 0, "Keyboard", -1, ddi.tszInstanceName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, "Wine Keyboard", -1, ddi.tszProductName, MAX_PATH);

    memcpy(lpddi, &ddi, (dwSize < sizeof(ddi) ? dwSize : sizeof(ddi)));
}
 
static HRESULT keyboarddev_enum_deviceA(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi, DWORD version, int id)
{
  if (id != 0)
    return E_FAIL;

  if ((dwDevType == 0) ||
      ((dwDevType == DIDEVTYPE_KEYBOARD) && (version < 0x0800)) ||
      (((dwDevType == DI8DEVCLASS_KEYBOARD) || (dwDevType == DI8DEVTYPE_KEYBOARD)) && (version >= 0x0800))) {
    TRACE("Enumerating the Keyboard device\n");
 
    fill_keyboard_dideviceinstanceA(lpddi, version);
    
    return S_OK;
  }

  return S_FALSE;
}

static HRESULT keyboarddev_enum_deviceW(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
  if (id != 0)
    return E_FAIL;

  if ((dwDevType == 0) ||
      ((dwDevType == DIDEVTYPE_KEYBOARD) && (version < 0x0800)) ||
      (((dwDevType == DI8DEVCLASS_KEYBOARD) || (dwDevType == DI8DEVTYPE_KEYBOARD)) && (version >= 0x0800))) {
    TRACE("Enumerating the Keyboard device\n");

    fill_keyboard_dideviceinstanceW(lpddi, version);
    
    return S_OK;
  }

  return S_FALSE;
}

static SysKeyboardImpl *alloc_device(REFGUID rguid, IDirectInputImpl *dinput)
{
    SysKeyboardImpl* newDevice;
    LPDIDATAFORMAT df = NULL;
    int i, idx = 0;

    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysKeyboardImpl));
    newDevice->base.IDirectInputDevice8A_iface.lpVtbl = &SysKeyboardAvt;
    newDevice->base.IDirectInputDevice8W_iface.lpVtbl = &SysKeyboardWvt;
    newDevice->base.ref = 1;
    memcpy(&newDevice->base.guid, rguid, sizeof(*rguid));
    newDevice->base.dinput = dinput;
    newDevice->base.event_proc = KeyboardCallback;
    InitializeCriticalSection(&newDevice->base.crit);
    newDevice->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": SysKeyboardImpl*->base.crit");

    /* Create copy of default data format */
    if (!(df = HeapAlloc(GetProcessHeap(), 0, c_dfDIKeyboard.dwSize))) goto failed;
    memcpy(df, &c_dfDIKeyboard, c_dfDIKeyboard.dwSize);
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto failed;

    for (i = 0; i < df->dwNumObjs; i++)
    {
        char buf[MAX_PATH];

        if (!GetKeyNameTextA(((i & 0x7f) << 16) | ((i & 0x80) << 17), buf, sizeof(buf)))
            continue;

        memcpy(&df->rgodf[idx], &c_dfDIKeyboard.rgodf[i], df->dwObjSize);
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(i) | DIDFT_PSHBUTTON;
    }
    df->dwNumObjs = idx;

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


static HRESULT keyboarddev_create_device(IDirectInputImpl *dinput, REFGUID rguid, REFIID riid, LPVOID *pdev, int unicode)
{
    TRACE("%p %s %s %p %i\n", dinput, debugstr_guid(rguid), debugstr_guid(riid), pdev, unicode);
    *pdev = NULL;

    if (IsEqualGUID(&GUID_SysKeyboard, rguid) ||        /* Generic Keyboard */
        IsEqualGUID(&DInput_Wine_Keyboard_GUID, rguid)) /* Wine Keyboard */
    {
        SysKeyboardImpl *This;

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
        TRACE("Created a Keyboard device (%p)\n", This);

        if (!This) return DIERR_OUTOFMEMORY;

        if (unicode)
            *pdev = &This->base.IDirectInputDevice8W_iface;
        else
            *pdev = &This->base.IDirectInputDevice8A_iface;

        return DI_OK;
    }

    return DIERR_DEVICENOTREG;
}

const struct dinput_device keyboard_device = {
  "Wine keyboard driver",
  keyboarddev_enum_deviceA,
  keyboarddev_enum_deviceW,
  keyboarddev_create_device
};

static HRESULT WINAPI SysKeyboardWImpl_GetDeviceState(LPDIRECTINPUTDEVICE8W iface, DWORD len, LPVOID ptr)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    TRACE("(%p)->(%d,%p)\n", This, len, ptr);

    if (!This->base.acquired) return DIERR_NOTACQUIRED;

    if (len != This->base.data_format.user_df->dwDataSize )
        return DIERR_INVALIDPARAM;

#ifndef __REACTOS__
    __wine_check_for_events( QS_ALLINPUT );
#endif

    EnterCriticalSection(&This->base.crit);

    if (TRACE_ON(dinput)) {
	int i;
	for (i = 0; i < WINE_DINPUT_KEYBOARD_MAX_KEYS; i++) {
	    if (This->DInputKeyState[i] != 0x00)
		TRACE(" - %02X: %02x\n", i, This->DInputKeyState[i]);
	}
    }

    fill_DataFormat(ptr, len, This->DInputKeyState, &This->base.data_format);
    LeaveCriticalSection(&This->base.crit);

    return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_GetDeviceState(LPDIRECTINPUTDEVICE8A iface, DWORD len, LPVOID ptr)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8A(iface);
    return SysKeyboardWImpl_GetDeviceState(IDirectInputDevice8W_from_impl(This), len, ptr);
}

/******************************************************************************
  *     GetCapabilities : get the device capabilities
  */
static HRESULT WINAPI SysKeyboardWImpl_GetCapabilities(LPDIRECTINPUTDEVICE8W iface, LPDIDEVCAPS lpDIDevCaps)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    DIDEVCAPS devcaps;

    TRACE("(this=%p,%p)\n",This,lpDIDevCaps);

    if ((lpDIDevCaps->dwSize != sizeof(DIDEVCAPS)) && (lpDIDevCaps->dwSize != sizeof(DIDEVCAPS_DX3))) {
        WARN("invalid parameter\n");
        return DIERR_INVALIDPARAM;
    }

    devcaps.dwSize = lpDIDevCaps->dwSize;
    devcaps.dwFlags = DIDC_ATTACHED | DIDC_EMULATED;
    if (This->base.dinput->dwVersion >= 0x0800)
        devcaps.dwDevType = DI8DEVTYPE_KEYBOARD | (DI8DEVTYPEKEYBOARD_PCENH << 8);
    else
        devcaps.dwDevType = DIDEVTYPE_KEYBOARD | (DIDEVTYPEKEYBOARD_PCENH << 8);
    devcaps.dwAxes = 0;
    devcaps.dwButtons = This->base.data_format.wine_df->dwNumObjs;
    devcaps.dwPOVs = 0;
    devcaps.dwFFSamplePeriod = 0;
    devcaps.dwFFMinTimeResolution = 0;
    devcaps.dwFirmwareRevision = 100;
    devcaps.dwHardwareRevision = 100;
    devcaps.dwFFDriverVersion = 0;

    memcpy(lpDIDevCaps, &devcaps, lpDIDevCaps->dwSize);

    return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_GetCapabilities(LPDIRECTINPUTDEVICE8A iface, LPDIDEVCAPS lpDIDevCaps)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8A(iface);
    return SysKeyboardWImpl_GetCapabilities(IDirectInputDevice8W_from_impl(This), lpDIDevCaps);
}

/******************************************************************************
  *     GetObjectInfo : get information about a device object such as a button
  *                     or axis
  */
static HRESULT WINAPI
SysKeyboardAImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
    HRESULT res;
    LONG scan;

    res = IDirectInputDevice2AImpl_GetObjectInfo(iface, pdidoi, dwObj, dwHow);
    if (res != DI_OK) return res;

    scan = DIDFT_GETINSTANCE(pdidoi->dwType);
    if (scan == DIK_PAUSE || scan == DIK_NUMLOCK) scan ^= 0x80;
    if (!GetKeyNameTextA((scan & 0x80) << 17 | (scan & 0x7f) << 16,
                         pdidoi->tszName, sizeof(pdidoi->tszName)))
        return DIERR_OBJECTNOTFOUND;

    _dump_OBJECTINSTANCEA(pdidoi);
    return res;
}

static HRESULT WINAPI SysKeyboardWImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface,
						     LPDIDEVICEOBJECTINSTANCEW pdidoi,
						     DWORD dwObj,
						     DWORD dwHow)
{
    HRESULT res;
    LONG scan;

    res = IDirectInputDevice2WImpl_GetObjectInfo(iface, pdidoi, dwObj, dwHow);
    if (res != DI_OK) return res;

    scan = DIDFT_GETINSTANCE(pdidoi->dwType);
    if (scan == DIK_PAUSE || scan == DIK_NUMLOCK) scan ^= 0x80;
    if (!GetKeyNameTextW((scan & 0x80) << 17 | (scan & 0x7f) << 16,
                         pdidoi->tszName, sizeof(pdidoi->tszName)/sizeof(pdidoi->tszName[0])))
        return DIERR_OBJECTNOTFOUND;

    _dump_OBJECTINSTANCEW(pdidoi);
    return res;
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
static HRESULT WINAPI SysKeyboardAImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8A(iface);
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEA)) {
        WARN(" dinput3 not supported yet...\n");
	return DI_OK;
    }

    fill_keyboard_dideviceinstanceA(pdidi, This->base.dinput->dwVersion);
    
    return DI_OK;
}

static HRESULT WINAPI SysKeyboardWImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8W iface, LPDIDEVICEINSTANCEW pdidi)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW)) {
        WARN(" dinput3 not supported yet...\n");
	return DI_OK;
    }

    fill_keyboard_dideviceinstanceW(pdidi, This->base.dinput->dwVersion);
    
    return DI_OK;
}

/******************************************************************************
 *      GetProperty : Retrieves information about the input device.
 */
static HRESULT WINAPI SysKeyboardWImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface,
                                                   REFGUID rguid, LPDIPROPHEADER pdiph)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p) %s,%p\n", iface, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (!IS_DIPROP(rguid)) return DI_OK;

    switch (LOWORD(rguid))
    {
        case (DWORD_PTR)DIPROP_KEYNAME:
        {
            HRESULT hr;
            LPDIPROPSTRING ps = (LPDIPROPSTRING)pdiph;
            DIDEVICEOBJECTINSTANCEW didoi;

            if (pdiph->dwSize != sizeof(DIPROPSTRING))
                return DIERR_INVALIDPARAM;

            didoi.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW);

            hr = SysKeyboardWImpl_GetObjectInfo(iface, &didoi, ps->diph.dwObj, ps->diph.dwHow);
            if (hr == DI_OK)
                memcpy(ps->wsz, didoi.tszName, sizeof(ps->wsz));
            return hr;
        }
        case (DWORD_PTR) DIPROP_RANGE:
            return DIERR_UNSUPPORTED;
        default:
            return IDirectInputDevice2AImpl_GetProperty( IDirectInputDevice8A_from_impl(This), rguid, pdiph );
    }
    return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface,
                                                   REFGUID rguid, LPDIPROPHEADER pdiph)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8A(iface);
    return SysKeyboardWImpl_GetProperty(IDirectInputDevice8W_from_impl(This), rguid, pdiph);
}

static HRESULT WINAPI SysKeyboardWImpl_BuildActionMap(LPDIRECTINPUTDEVICE8W iface,
                                                      LPDIACTIONFORMATW lpdiaf,
                                                      LPCWSTR lpszUserName,
                                                      DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    return  _build_action_map(iface, lpdiaf, lpszUserName, dwFlags, DIKEYBOARD_MASK, &c_dfDIKeyboard);
}

static HRESULT WINAPI SysKeyboardAImpl_BuildActionMap(LPDIRECTINPUTDEVICE8A iface,
                                                      LPDIACTIONFORMATA lpdiaf,
                                                      LPCSTR lpszUserName,
                                                      DWORD dwFlags)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8A(iface);
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

    hr = SysKeyboardWImpl_BuildActionMap(&This->base.IDirectInputDevice8W_iface, &diafW, lpszUserNameW, dwFlags);

    _copy_diactionformatWtoA(lpdiaf, &diafW);
    HeapFree(GetProcessHeap(), 0, diafW.rgoAction);
    HeapFree(GetProcessHeap(), 0, lpszUserNameW);

    return hr;
}

static HRESULT WINAPI SysKeyboardWImpl_SetActionMap(LPDIRECTINPUTDEVICE8W iface,
                                                    LPDIACTIONFORMATW lpdiaf,
                                                    LPCWSTR lpszUserName,
                                                    DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    return _set_action_map(iface, lpdiaf, lpszUserName, dwFlags, &c_dfDIKeyboard);
}

static HRESULT WINAPI SysKeyboardAImpl_SetActionMap(LPDIRECTINPUTDEVICE8A iface,
                                                    LPDIACTIONFORMATA lpdiaf,
                                                    LPCSTR lpszUserName,
                                                    DWORD dwFlags)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8A(iface);
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

    hr = SysKeyboardWImpl_SetActionMap(&This->base.IDirectInputDevice8W_iface, &diafW, lpszUserNameW, dwFlags);

    HeapFree(GetProcessHeap(), 0, diafW.rgoAction);
    HeapFree(GetProcessHeap(), 0, lpszUserNameW);

    return hr;
}

static const IDirectInputDevice8AVtbl SysKeyboardAvt =
{
    IDirectInputDevice2AImpl_QueryInterface,
    IDirectInputDevice2AImpl_AddRef,
    IDirectInputDevice2AImpl_Release,
    SysKeyboardAImpl_GetCapabilities,
    IDirectInputDevice2AImpl_EnumObjects,
    SysKeyboardAImpl_GetProperty,
    IDirectInputDevice2AImpl_SetProperty,
    IDirectInputDevice2AImpl_Acquire,
    IDirectInputDevice2AImpl_Unacquire,
    SysKeyboardAImpl_GetDeviceState,
    IDirectInputDevice2AImpl_GetDeviceData,
    IDirectInputDevice2AImpl_SetDataFormat,
    IDirectInputDevice2AImpl_SetEventNotification,
    IDirectInputDevice2AImpl_SetCooperativeLevel,
    SysKeyboardAImpl_GetObjectInfo,
    SysKeyboardAImpl_GetDeviceInfo,
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
    SysKeyboardAImpl_BuildActionMap,
    SysKeyboardAImpl_SetActionMap,
    IDirectInputDevice8AImpl_GetImageInfo
};

static const IDirectInputDevice8WVtbl SysKeyboardWvt =
{
    IDirectInputDevice2WImpl_QueryInterface,
    IDirectInputDevice2WImpl_AddRef,
    IDirectInputDevice2WImpl_Release,
    SysKeyboardWImpl_GetCapabilities,
    IDirectInputDevice2WImpl_EnumObjects,
    SysKeyboardWImpl_GetProperty,
    IDirectInputDevice2WImpl_SetProperty,
    IDirectInputDevice2WImpl_Acquire,
    IDirectInputDevice2WImpl_Unacquire,
    SysKeyboardWImpl_GetDeviceState,
    IDirectInputDevice2WImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    SysKeyboardWImpl_GetObjectInfo,
    SysKeyboardWImpl_GetDeviceInfo,
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
    SysKeyboardWImpl_BuildActionMap,
    SysKeyboardWImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};
