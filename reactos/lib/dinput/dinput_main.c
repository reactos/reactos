/*		DirectInput
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2002 TransGaming Technologies Inc.
 *
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
/* Status:
 *
 * - Tomb Raider 2 Demo:
 *   Playable using keyboard only.
 * - WingCommander Prophecy Demo:
 *   Doesn't get Input Focus.
 *
 * - Fallout : works great in X and DGA mode
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static IDirectInput7AVtbl ddi7avt;
static IDirectInput7WVtbl ddi7wvt;
static IDirectInput8AVtbl ddi8avt;
static IDirectInput8WVtbl ddi8wvt;

/* This array will be filled a dinput.so loading */
#define MAX_WINE_DINPUT_DEVICES 4
static dinput_device * dinput_devices[MAX_WINE_DINPUT_DEVICES];
static int nrof_dinput_devices = 0;

HINSTANCE DINPUT_instance = NULL;

BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserv)
{
    switch(reason)
    {
      case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        DINPUT_instance = inst;
        break;
      case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/* register a direct draw driver. We better not use malloc for we are in
 * the ELF startup initialisation at this point.
 */
void dinput_register_device(dinput_device *device) {
    int	i;

    /* insert according to priority */
    for (i=0;i<nrof_dinput_devices;i++) {
	if (dinput_devices[i]->pref <= device->pref) {
	    memcpy(dinput_devices+i+1,dinput_devices+i,sizeof(dinput_devices[0])*(nrof_dinput_devices-i));
	    dinput_devices[i] = device;
	    break;
	}
    }
    if (i==nrof_dinput_devices)	/* not found, or too low priority */
	dinput_devices[nrof_dinput_devices] = device;

    nrof_dinput_devices++;

    /* increase MAX_DDRAW_DRIVERS if the line below triggers */
    assert(nrof_dinput_devices <= MAX_WINE_DINPUT_DEVICES);
}

/******************************************************************************
 *	DirectInputCreateEx (DINPUT.@)
 */
HRESULT WINAPI DirectInputCreateEx(
	HINSTANCE hinst, DWORD dwVersion, REFIID riid, LPVOID *ppDI,
	LPUNKNOWN punkOuter) 
{
	IDirectInputImpl* This;

	TRACE("(0x%08lx,%04lx,%s,%p,%p)\n", (DWORD)hinst,dwVersion,debugstr_guid(riid),ppDI,punkOuter);

	if (IsEqualGUID(&IID_IDirectInputA,riid) ||
	    IsEqualGUID(&IID_IDirectInput2A,riid) ||
	    IsEqualGUID(&IID_IDirectInput7A,riid)) {
	  This = (IDirectInputImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInputImpl));
	  This->lpVtbl = &ddi7avt;
	  This->ref = 1;
	  This->version = 1;
	  *ppDI = This;

	  return DI_OK;
	}

	if (IsEqualGUID(&IID_IDirectInputW,riid) ||
	    IsEqualGUID(&IID_IDirectInput2W,riid) ||
	    IsEqualGUID(&IID_IDirectInput7W,riid)) {
	  This = (IDirectInputImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInputImpl));
	  This->lpVtbl = &ddi7wvt;
	  This->ref = 1;
	  This->version = 1;
	  *ppDI = This;

	  return DI_OK;
	}

	if (IsEqualGUID(&IID_IDirectInput8A,riid)) {
	  This = (IDirectInputImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInputImpl));
	  This->lpVtbl = &ddi8avt;
	  This->ref = 1;
	  This->version = 8;
	  *ppDI = This;

	  return DI_OK;
	}

	if (IsEqualGUID(&IID_IDirectInput8W,riid)) {
	  This = (IDirectInputImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInputImpl));
	  This->lpVtbl = &ddi8wvt;
	  This->ref = 1;
	  This->version = 8;
	  *ppDI = This;

	  return DI_OK;
	}

	return DIERR_OLDDIRECTINPUTVERSION;
}

/******************************************************************************
 *	DirectInputCreateA (DINPUT.@)
 */
HRESULT WINAPI DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter)
{
	IDirectInputImpl* This;
	TRACE("(0x%08lx,%04lx,%p,%p)\n", (DWORD)hinst,dwVersion,ppDI,punkOuter);

	//trace:dinput:DirectInputCreateA (0x00400000,0500,0x42bafc54,(nil))
	This = (IDirectInputImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInputImpl));
	This->lpVtbl = &ddi7avt;
	This->ref = 1;
	if (dwVersion > 0x0700) {
	    This->version = 8;
	} else {
	    /* We do not differientiate between version 1, 2 and 7 */
	    This->version = 1;
	}
	*ppDI = (IDirectInputA*)This;
	return 0;

}

/******************************************************************************
 *	DirectInputCreateW (DINPUT.@)
 */
HRESULT WINAPI DirectInputCreateW(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTW *ppDI, LPUNKNOWN punkOuter)
{
	IDirectInputImpl* This;
	TRACE("(0x%08lx,%04lx,%p,%p)\n", (DWORD)hinst,dwVersion,ppDI,punkOuter);
	This = (IDirectInputImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInputImpl));
	This->lpVtbl = &ddi7wvt;
	This->ref = 1;
	if (dwVersion >= 0x0800) {
	    This->version = 8;
	} else {
	    /* We do not differientiate between version 1, 2 and 7 */
	    This->version = 1;
	}
	*ppDI = (IDirectInputW*)This;
	return 0;
}

static const char *_dump_DIDEVTYPE_value(DWORD dwDevType) {
    switch (dwDevType) {
        case 0: return "All devices";
	case DIDEVTYPE_MOUSE: return "DIDEVTYPE_MOUSE";
	case DIDEVTYPE_KEYBOARD: return "DIDEVTYPE_KEYBOARD";
	case DIDEVTYPE_JOYSTICK: return "DIDEVTYPE_JOYSTICK";
	case DIDEVTYPE_DEVICE: return "DIDEVTYPE_DEVICE";
	default: return "Unkown";
    }
}

static void _dump_EnumDevices_dwFlags(DWORD dwFlags) {
    if (TRACE_ON(dinput)) {
	unsigned int   i;
	static const struct {
	    DWORD       mask;
	    const char  *name;
	} flags[] = {
#define FE(x) { x, #x}
	    FE(DIEDFL_ALLDEVICES),
	    FE(DIEDFL_ATTACHEDONLY),
	    FE(DIEDFL_FORCEFEEDBACK),
	    FE(DIEDFL_INCLUDEALIASES),
	    FE(DIEDFL_INCLUDEPHANTOMS)
#undef FE
	};
	if (dwFlags == 0) {
	    DPRINTF("DIEDFL_ALLDEVICES");
	    return;
	}
	for (i = 0; i < (sizeof(flags) / sizeof(flags[0])); i++)
	    if (flags[i].mask & dwFlags)
		DPRINTF("%s ",flags[i].name);
    }
}

/******************************************************************************
 *	IDirectInputA_EnumDevices
 */
static HRESULT WINAPI IDirectInputAImpl_EnumDevices(
	LPDIRECTINPUT7A iface, DWORD dwDevType, LPDIENUMDEVICESCALLBACKA lpCallback,
	LPVOID pvRef, DWORD dwFlags)
{
    IDirectInputImpl *This = (IDirectInputImpl *)iface;
    DIDEVICEINSTANCEA devInstance;
    int i, j, r;
    
    TRACE("(this=%p,0x%04lx '%s',%p,%p,%04lx)\n",
	  This, dwDevType, _dump_DIDEVTYPE_value(dwDevType),
	  lpCallback, pvRef, dwFlags);

	if (nrof_dinput_devices==0){
	  scan_mouse();
	  scan_keyboard();
	}
	
    TRACE(" flags: "); _dump_EnumDevices_dwFlags(dwFlags); TRACE("\n");
    

    for (i = 0; i < nrof_dinput_devices; i++) {
        for (j = 0, r = -1; r != 0; j++) {
	    devInstance.dwSize = sizeof(devInstance);
	    TRACE("  - checking device %d ('%s')\n", i, dinput_devices[i]->name);
		

	    if ((r = dinput_devices[i]->enum_deviceA(dwDevType, dwFlags, &devInstance, This->version, j))) {
	        if (lpCallback(&devInstance,pvRef) == DIENUM_STOP)
		    return 0;
	    }
	}
    }
    
    return 0;
}
/******************************************************************************
 *	IDirectInputW_EnumDevices
 */
static HRESULT WINAPI IDirectInputWImpl_EnumDevices(
	LPDIRECTINPUT7W iface, DWORD dwDevType, LPDIENUMDEVICESCALLBACKW lpCallback,
	LPVOID pvRef, DWORD dwFlags) 
{
    IDirectInputImpl *This = (IDirectInputImpl *)iface;
    DIDEVICEINSTANCEW devInstance;
    int i, j, r;
    
		if (nrof_dinput_devices==0){
	  scan_mouse();
	  scan_keyboard();
	}

    TRACE("(this=%p,0x%04lx '%s',%p,%p,%04lx)\n",
	  This, dwDevType, _dump_DIDEVTYPE_value(dwDevType),
	  lpCallback, pvRef, dwFlags);
    TRACE(" flags: "); _dump_EnumDevices_dwFlags(dwFlags); TRACE("\n");


    for (i = 0; i < nrof_dinput_devices; i++) {
        for (j = 0, r = -1; r != 0; j++) {
	    devInstance.dwSize = sizeof(devInstance);
	    TRACE("  - checking device %d ('%s')\n", i, dinput_devices[i]->name);
	    if ((r = dinput_devices[i]->enum_deviceW(dwDevType, dwFlags, &devInstance, This->version, j))) {
	        if (lpCallback(&devInstance,pvRef) == DIENUM_STOP)
		    return 0;
	    }
	}
    }
    
    return 0;
}

static ULONG WINAPI IDirectInputAImpl_AddRef(LPDIRECTINPUT7A iface)
{
	IDirectInputImpl *This = (IDirectInputImpl *)iface;
	return InterlockedIncrement((&This->ref));
}

static ULONG WINAPI IDirectInputAImpl_Release(LPDIRECTINPUT7A iface)
{
	IDirectInputImpl *This = (IDirectInputImpl *)iface;
	ULONG ref;
	ref = InterlockedDecrement(&(This->ref));
	if (ref == 0)
		HeapFree(GetProcessHeap(),0,This);
	return ref;
}

static HRESULT WINAPI IDirectInputAImpl_QueryInterface(LPDIRECTINPUT7A iface, REFIID riid, LPVOID *ppobj) {
	IDirectInputImpl *This = (IDirectInputImpl *)iface;

	TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	if (IsEqualGUID(&IID_IUnknown,riid) ||
	    IsEqualGUID(&IID_IDirectInputA,riid) ||
	    IsEqualGUID(&IID_IDirectInput2A,riid) ||
	    IsEqualGUID(&IID_IDirectInput7A,riid)) {
		IDirectInputAImpl_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	TRACE("Unsupported interface !\n");
	return E_FAIL;
}

static HRESULT WINAPI IDirectInputWImpl_QueryInterface(LPDIRECTINPUT7W iface, REFIID riid, LPVOID *ppobj) {
	IDirectInputImpl *This = (IDirectInputImpl *)iface;

	TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	if (IsEqualGUID(&IID_IUnknown,riid) ||
	    IsEqualGUID(&IID_IDirectInputW,riid) ||
	    IsEqualGUID(&IID_IDirectInput2W,riid) ||
	    IsEqualGUID(&IID_IDirectInput7W,riid)) {
		IDirectInputAImpl_AddRef((LPDIRECTINPUT7A) iface);
		*ppobj = This;
		return 0;
	}
	TRACE("Unsupported interface !\n");
	return E_FAIL;
}

static HRESULT WINAPI IDirectInputAImpl_CreateDevice(
	LPDIRECTINPUT7A iface,REFGUID rguid,LPDIRECTINPUTDEVICEA* pdev,
	LPUNKNOWN punk
) {
	IDirectInputImpl *This = (IDirectInputImpl *)iface;
	HRESULT ret_value = DIERR_DEVICENOTREG;
	int i;

	TRACE("(this=%p,%s,%p,%p)\n",This,debugstr_guid(rguid),pdev,punk);
	
	if (nrof_dinput_devices==0){
	  scan_mouse();
	  scan_keyboard();
	}
	
	/* Loop on all the devices to see if anyone matches the given GUID */
	for (i = 0; i < nrof_dinput_devices; i++) {
	  HRESULT ret;
	  if ((ret = dinput_devices[i]->create_deviceA(This, rguid, NULL, pdev)) == DI_OK)
	    return DI_OK;

	  if (ret == DIERR_NOINTERFACE)
	    ret_value = DIERR_NOINTERFACE;
	}

	return ret_value;
}

static HRESULT WINAPI IDirectInputWImpl_CreateDevice(LPDIRECTINPUT7A iface, 
						     REFGUID rguid, LPDIRECTINPUTDEVICEW* pdev, LPUNKNOWN punk) {
        IDirectInputImpl *This = (IDirectInputImpl *)iface;
	HRESULT ret_value = DIERR_DEVICENOTREG;
	int i;
    
	TRACE("(this=%p,%s,%p,%p)\n",This,debugstr_guid(rguid),pdev,punk);

	if (nrof_dinput_devices==0){
	  scan_mouse();
	  scan_keyboard();
	}
	
	/* Loop on all the devices to see if anyone matches the given GUID */
	for (i = 0; i < nrof_dinput_devices; i++) {
	  HRESULT ret;
	  if ((ret = dinput_devices[i]->create_deviceW(This, rguid, NULL, pdev)) == DI_OK)
	    return DI_OK;

	  if (ret == DIERR_NOINTERFACE)
	    ret_value = DIERR_NOINTERFACE;
	}

	return ret_value;
}

static HRESULT WINAPI IDirectInputAImpl_Initialize(LPDIRECTINPUT7A iface, HINSTANCE hinst, DWORD x) {
	return DIERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectInputAImpl_GetDeviceStatus(LPDIRECTINPUT7A iface,
							REFGUID rguid) {
  IDirectInputImpl *This = (IDirectInputImpl *)iface;

  FIXME("(%p)->(%s): stub\n",This,debugstr_guid(rguid));

  return DI_OK;
}

static HRESULT WINAPI IDirectInputAImpl_RunControlPanel(LPDIRECTINPUT7A iface,
							HWND hwndOwner,
							DWORD dwFlags) {
  IDirectInputImpl *This = (IDirectInputImpl *)iface;
  FIXME("(%p)->(%08lx,%08lx): stub\n",This, (DWORD) hwndOwner, dwFlags);

  return DI_OK;
}

static HRESULT WINAPI IDirectInput2AImpl_FindDevice(LPDIRECTINPUT7A iface, REFGUID rguid,
						    LPCSTR pszName, LPGUID pguidInstance) {
  IDirectInputImpl *This = (IDirectInputImpl *)iface;
  FIXME("(%p)->(%s, %s, %p): stub\n", This, debugstr_guid(rguid), pszName, pguidInstance);

  return DI_OK;
}

static HRESULT WINAPI IDirectInput2WImpl_FindDevice(LPDIRECTINPUT7W iface, REFGUID rguid,
						    LPCWSTR pszName, LPGUID pguidInstance) {
  IDirectInputImpl *This = (IDirectInputImpl *)iface;
  FIXME("(%p)->(%s, %s, %p): stub\n", This, debugstr_guid(rguid), debugstr_w(pszName), pguidInstance);

  return DI_OK;
}

static HRESULT WINAPI IDirectInput7AImpl_CreateDeviceEx(LPDIRECTINPUT7A iface, REFGUID rguid,
							REFIID riid, LPVOID* pvOut, LPUNKNOWN lpUnknownOuter)
{
  IDirectInputImpl *This = (IDirectInputImpl *)iface;
  HRESULT ret_value = DIERR_DEVICENOTREG;
  int i;

  TRACE("(%p)->(%s, %s, %p, %p)\n", This, debugstr_guid(rguid), debugstr_guid(riid), pvOut, lpUnknownOuter);

  if (nrof_dinput_devices==0){
    scan_mouse();
    scan_keyboard();
  }  

  /* Loop on all the devices to see if anyone matches the given GUID */
  for (i = 0; i < nrof_dinput_devices; i++) {
    HRESULT ret;
    if ((ret = dinput_devices[i]->create_deviceA(This, rguid, riid, (LPDIRECTINPUTDEVICEA*) pvOut)) == DI_OK)
      return DI_OK;

    if (ret == DIERR_NOINTERFACE)
      ret_value = DIERR_NOINTERFACE;
  }

  return ret_value;
}

static HRESULT WINAPI IDirectInput7WImpl_CreateDeviceEx(LPDIRECTINPUT7W iface, REFGUID rguid,
							REFIID riid, LPVOID* pvOut, LPUNKNOWN lpUnknownOuter)
{
  IDirectInputImpl *This = (IDirectInputImpl *)iface;
  HRESULT ret_value = DIERR_DEVICENOTREG;
  int i;

  TRACE("(%p)->(%s, %s, %p, %p)\n", This, debugstr_guid(rguid), debugstr_guid(riid), pvOut, lpUnknownOuter);

  	if (nrof_dinput_devices==0){
	  scan_mouse();
	  scan_keyboard();
	}
  


  /* Loop on all the devices to see if anyone matches the given GUID */
  for (i = 0; i < nrof_dinput_devices; i++) {
    HRESULT ret;
    if ((ret = dinput_devices[i]->create_deviceW(This, rguid, riid, (LPDIRECTINPUTDEVICEW*) pvOut)) == DI_OK)
      return DI_OK;

    if (ret == DIERR_NOINTERFACE)
      ret_value = DIERR_NOINTERFACE;
  }

  return ret_value;
}

static HRESULT WINAPI IDirectInput8AImpl_QueryInterface(LPDIRECTINPUT8A iface, REFIID riid, LPVOID *ppobj) {
      IDirectInputImpl *This = (IDirectInputImpl *)iface;

      TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
      if (IsEqualGUID(&IID_IUnknown,riid) ||
          IsEqualGUID(&IID_IDirectInput8A,riid)) {
              IDirectInputAImpl_AddRef((LPDIRECTINPUT7A) iface);
              *ppobj = This;
              return 0;
      }
      TRACE("Unsupported interface !\n");
      return E_NOINTERFACE;
}

static HRESULT WINAPI IDirectInput8WImpl_QueryInterface(LPDIRECTINPUT8W iface, REFIID riid, LPVOID *ppobj) {
      IDirectInputImpl *This = (IDirectInputImpl *)iface;

      TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
      if (IsEqualGUID(&IID_IUnknown,riid) ||
          IsEqualGUID(&IID_IDirectInput8W,riid)) {
              IDirectInputAImpl_AddRef((LPDIRECTINPUT7A) iface);
              *ppobj = This;
              return 0;
      }
      TRACE("Unsupported interface !\n");
      return E_NOINTERFACE;
}

static HRESULT WINAPI IDirectInput8AImpl_EnumDevicesBySemantics(
      LPDIRECTINPUT8A iface, LPCSTR ptszUserName, LPDIACTIONFORMATA lpdiActionFormat,
      LPDIENUMDEVICESBYSEMANTICSCBA lpCallback,
      LPVOID pvRef, DWORD dwFlags
)
{
      IDirectInputImpl *This = (IDirectInputImpl *)iface;

      FIXME("(this=%p,%s,%p,%p,%p,%04lx): stub\n", This, ptszUserName, lpdiActionFormat,
            lpCallback, pvRef, dwFlags);
      return 0;
}

static HRESULT WINAPI IDirectInput8WImpl_EnumDevicesBySemantics(
      LPDIRECTINPUT8W iface, LPCWSTR ptszUserName, LPDIACTIONFORMATW lpdiActionFormat,
      LPDIENUMDEVICESBYSEMANTICSCBW lpCallback,
      LPVOID pvRef, DWORD dwFlags
)
{
      IDirectInputImpl *This = (IDirectInputImpl *)iface;

      FIXME("(this=%p,%s,%p,%p,%p,%04lx): stub\n", This, debugstr_w(ptszUserName), lpdiActionFormat,
            lpCallback, pvRef, dwFlags);
      return 0;
}

static HRESULT WINAPI IDirectInput8AImpl_ConfigureDevices(
      LPDIRECTINPUT8A iface, LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
      LPDICONFIGUREDEVICESPARAMSA lpdiCDParams, DWORD dwFlags, LPVOID pvRefData
)
{
      IDirectInputImpl *This = (IDirectInputImpl *)iface;

      FIXME("(this=%p,%p,%p,%04lx,%p): stub\n", This, lpdiCallback, lpdiCDParams,
            dwFlags, pvRefData);
      return 0;
}

static HRESULT WINAPI IDirectInput8WImpl_ConfigureDevices(
      LPDIRECTINPUT8W iface, LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
      LPDICONFIGUREDEVICESPARAMSW lpdiCDParams, DWORD dwFlags, LPVOID pvRefData
)
{
      IDirectInputImpl *This = (IDirectInputImpl *)iface;

      FIXME("(this=%p,%p,%p,%04lx,%p): stub\n", This, lpdiCallback, lpdiCDParams,
            dwFlags, pvRefData);
      return 0;
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)   (typeof(ddi7avt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static IDirectInput7AVtbl ddi7avt = {
	XCAST(QueryInterface)IDirectInputAImpl_QueryInterface,
	XCAST(AddRef)IDirectInputAImpl_AddRef,
	XCAST(Release)IDirectInputAImpl_Release,
	XCAST(CreateDevice)IDirectInputAImpl_CreateDevice,
	XCAST(EnumDevices)IDirectInputAImpl_EnumDevices,
	XCAST(GetDeviceStatus)IDirectInputAImpl_GetDeviceStatus,
	XCAST(RunControlPanel)IDirectInputAImpl_RunControlPanel,
	XCAST(Initialize)IDirectInputAImpl_Initialize,
	XCAST(FindDevice)IDirectInput2AImpl_FindDevice,
	XCAST(CreateDeviceEx)IDirectInput7AImpl_CreateDeviceEx
};

#undef XCAST
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)   (typeof(ddi7wvt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static IDirectInput7WVtbl ddi7wvt = {
	XCAST(QueryInterface)IDirectInputWImpl_QueryInterface,
	XCAST(AddRef)IDirectInputAImpl_AddRef,
	XCAST(Release)IDirectInputAImpl_Release,
	XCAST(CreateDevice)IDirectInputWImpl_CreateDevice,
	XCAST(EnumDevices)IDirectInputWImpl_EnumDevices,
	XCAST(GetDeviceStatus)IDirectInputAImpl_GetDeviceStatus,
	XCAST(RunControlPanel)IDirectInputAImpl_RunControlPanel,
	XCAST(Initialize)IDirectInputAImpl_Initialize,
	XCAST(FindDevice)IDirectInput2WImpl_FindDevice,
	XCAST(CreateDeviceEx)IDirectInput7WImpl_CreateDeviceEx
};
#undef XCAST

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(ddi8avt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static IDirectInput8AVtbl ddi8avt = {
	XCAST(QueryInterface)IDirectInput8AImpl_QueryInterface,
	XCAST(AddRef)IDirectInputAImpl_AddRef,
	XCAST(Release)IDirectInputAImpl_Release,
	XCAST(CreateDevice)IDirectInputAImpl_CreateDevice,
	XCAST(EnumDevices)IDirectInputAImpl_EnumDevices,
	XCAST(GetDeviceStatus)IDirectInputAImpl_GetDeviceStatus,
	XCAST(RunControlPanel)IDirectInputAImpl_RunControlPanel,
	XCAST(Initialize)IDirectInputAImpl_Initialize,
	XCAST(FindDevice)IDirectInput2AImpl_FindDevice,
	XCAST(EnumDevicesBySemantics)IDirectInput8AImpl_EnumDevicesBySemantics,
	XCAST(ConfigureDevices)IDirectInput8AImpl_ConfigureDevices
};
#undef XCAST

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(ddi8wvt.fun))
#else
# define XCAST(fun)	(void*)
#endif
static IDirectInput8WVtbl ddi8wvt = {
	XCAST(QueryInterface)IDirectInput8WImpl_QueryInterface,
	XCAST(AddRef)IDirectInputAImpl_AddRef,
	XCAST(Release)IDirectInputAImpl_Release,
	XCAST(CreateDevice)IDirectInputWImpl_CreateDevice,
	XCAST(EnumDevices)IDirectInputWImpl_EnumDevices,
	XCAST(GetDeviceStatus)IDirectInputAImpl_GetDeviceStatus,
	XCAST(RunControlPanel)IDirectInputAImpl_RunControlPanel,
	XCAST(Initialize)IDirectInputAImpl_Initialize,
	XCAST(FindDevice)IDirectInput2WImpl_FindDevice,
	XCAST(EnumDevicesBySemantics)IDirectInput8WImpl_EnumDevicesBySemantics,
	XCAST(ConfigureDevices)IDirectInput8WImpl_ConfigureDevices
};
#undef XCAST

/*******************************************************************************
 * DirectInput ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    IClassFactoryVtbl          *lpVtbl;
    DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI DICF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI DICF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI DICF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	/* static class, won't be  freed */
	return InterlockedDecrement(&(This->ref));
}

static HRESULT WINAPI DICF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
	if ( IsEqualGUID( &IID_IDirectInputA, riid ) ||
	     IsEqualGUID( &IID_IDirectInputW, riid ) ||
	     IsEqualGUID( &IID_IDirectInput2A, riid ) ||
	     IsEqualGUID( &IID_IDirectInput2W, riid ) ||
	     IsEqualGUID( &IID_IDirectInput7A, riid ) ||
	     IsEqualGUID( &IID_IDirectInput7W, riid ) ||
	     IsEqualGUID( &IID_IDirectInput8A, riid ) ||
	     IsEqualGUID( &IID_IDirectInput8W, riid ) ) {
		/* FIXME: reuse already created dinput if present? */
		return DirectInputCreateEx(0,0,riid,ppobj,pOuter);
	}

	FIXME("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
	return E_NOINTERFACE;
}

static HRESULT WINAPI DICF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static IClassFactoryVtbl DICF_Vtbl = {
	DICF_QueryInterface,
	DICF_AddRef,
	DICF_Release,
	DICF_CreateInstance,
	DICF_LockServer
};
static IClassFactoryImpl DINPUT_CF = {&DICF_Vtbl, 1 };

/***********************************************************************
 *		DllCanUnloadNow (DINPUT.@)
 */
HRESULT WINAPI DINPUT_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");

    return S_FALSE;
}

/***********************************************************************
 *		DllGetClassObject (DINPUT.@)
 */
HRESULT WINAPI DINPUT_DllGetClassObject(REFCLSID rclsid, REFIID riid,
					LPVOID *ppv)
{
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
    	*ppv = (LPVOID)&DINPUT_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
    }

    FIXME("(%s,%s,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
