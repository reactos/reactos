/*		DirectInput Device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
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

/* This file contains all the Device specific functions that can be used as stubs
   by real device implementations.

   It also contains all the helper functions.
*/
#include "config.h"

#include <stdarg.h>
#include <string.h>
#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "dinput.h"
#include "device_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/******************************************************************************
 *	Various debugging tools
 */
void _dump_cooperativelevel_DI(DWORD dwFlags) {
    if (TRACE_ON(dinput)) {
	unsigned int   i;
	static const struct {
	    DWORD       mask;
	    const char  *name;
	} flags[] = {
#define FE(x) { x, #x}
	    FE(DISCL_BACKGROUND),
	    FE(DISCL_EXCLUSIVE),
	    FE(DISCL_FOREGROUND),
	    FE(DISCL_NONEXCLUSIVE)
#undef FE
	};
	for (i = 0; i < (sizeof(flags) / sizeof(flags[0])); i++)
	    if (flags[i].mask & dwFlags)
		DPRINTF("%s ",flags[i].name);
	DPRINTF("\n");
    }
}

void _dump_EnumObjects_flags(DWORD dwFlags) {
    if (TRACE_ON(dinput)) {
	unsigned int   i;
	DWORD type, instance;
	static const struct {
	    DWORD       mask;
	    const char  *name;
	} flags[] = {
#define FE(x) { x, #x}
	    FE(DIDFT_RELAXIS),
	    FE(DIDFT_ABSAXIS),
	    FE(DIDFT_PSHBUTTON),
	    FE(DIDFT_TGLBUTTON),
	    FE(DIDFT_POV),
	    FE(DIDFT_COLLECTION),
	    FE(DIDFT_NODATA),
	    FE(DIDFT_FFACTUATOR),
	    FE(DIDFT_FFEFFECTTRIGGER),
	    FE(DIDFT_OUTPUT),
	    FE(DIDFT_VENDORDEFINED),
	    FE(DIDFT_ALIAS),
	    FE(DIDFT_OPTIONAL)
#undef FE
	};
	type = (dwFlags & 0xFF0000FF);
	instance = ((dwFlags >> 8) & 0xFFFF);
	DPRINTF("Type:");
	if (type == DIDFT_ALL) {
	    DPRINTF(" DIDFT_ALL");
	} else {
	    for (i = 0; i < (sizeof(flags) / sizeof(flags[0])); i++) {
		if (flags[i].mask & type) {
		    type &= ~flags[i].mask;
		    DPRINTF(" %s",flags[i].name);
		}
	    }
	    if (type) {
		DPRINTF(" (unhandled: %08lx)", type);
	    }
	}
	DPRINTF(" / Instance: ");
	if (instance == ((DIDFT_ANYINSTANCE >> 8) & 0xFFFF)) {
	    DPRINTF("DIDFT_ANYINSTANCE");
	} else {
	    DPRINTF("%3ld", instance);
	}
    }
}

void _dump_DIPROPHEADER(LPCDIPROPHEADER diph) {
    if (TRACE_ON(dinput)) {
	DPRINTF("  - dwObj = 0x%08lx\n", diph->dwObj);
	DPRINTF("  - dwHow = %s\n",
		((diph->dwHow == DIPH_DEVICE) ? "DIPH_DEVICE" :
		 ((diph->dwHow == DIPH_BYOFFSET) ? "DIPH_BYOFFSET" :
		  ((diph->dwHow == DIPH_BYID)) ? "DIPH_BYID" : "unknown")));
    }
}

void _dump_OBJECTINSTANCEA(DIDEVICEOBJECTINSTANCEA *ddoi) {
    if (TRACE_ON(dinput)) {
	DPRINTF("    - enumerating : %s ('%s') - %2ld - 0x%08lx - %s\n",
		debugstr_guid(&ddoi->guidType), _dump_dinput_GUID(&ddoi->guidType), ddoi->dwOfs, ddoi->dwType, ddoi->tszName);
    }
}

void _dump_OBJECTINSTANCEW(DIDEVICEOBJECTINSTANCEW *ddoi) {
    if (TRACE_ON(dinput)) {
	DPRINTF("    - enumerating : %s ('%s'), - %2ld - 0x%08lx - %s\n",
		debugstr_guid(&ddoi->guidType), _dump_dinput_GUID(&ddoi->guidType), ddoi->dwOfs, ddoi->dwType, debugstr_w(ddoi->tszName));
    }
}

/* This function is a helper to convert a GUID into any possible DInput GUID out there */
const char *_dump_dinput_GUID(const GUID *guid) {
    unsigned int i;
    static const struct {
	const GUID *guid;
	const char *name;
    } guids[] = {
#define FE(x) { &x, #x}
	FE(GUID_XAxis),
	FE(GUID_YAxis),
	FE(GUID_ZAxis),
	FE(GUID_RxAxis),
	FE(GUID_RyAxis),
	FE(GUID_RzAxis),
	FE(GUID_Slider),
	FE(GUID_Button),
	FE(GUID_Key),
	FE(GUID_POV),
	FE(GUID_Unknown),
	FE(GUID_SysMouse),
	FE(GUID_SysKeyboard),
	FE(GUID_Joystick),
	FE(GUID_ConstantForce),
	FE(GUID_RampForce),
	FE(GUID_Square),
	FE(GUID_Sine),
	FE(GUID_Triangle),
	FE(GUID_SawtoothUp),
	FE(GUID_SawtoothDown),
	FE(GUID_Spring),
	FE(GUID_Damper),
	FE(GUID_Inertia),
	FE(GUID_Friction),
	FE(GUID_CustomForce)
#undef FE
    };
    if (guid == NULL)
	return "null GUID";
    for (i = 0; i < (sizeof(guids) / sizeof(guids[0])); i++) {
	if (IsEqualGUID(guids[i].guid, guid)) {
	    return guids[i].name;
	}
    }
    return "Unknown GUID";
}

void _dump_DIDATAFORMAT(const DIDATAFORMAT *df) {
    unsigned int i;

    TRACE("Dumping DIDATAFORMAT structure:\n");
    TRACE("  - dwSize: %ld\n", df->dwSize);
    if (df->dwSize != sizeof(DIDATAFORMAT)) {
	WARN("Non-standard DIDATAFORMAT structure size (%ld instead of %d).\n", df->dwSize, sizeof(DIDATAFORMAT));
    }
    TRACE("  - dwObjsize: %ld\n", df->dwObjSize);
    if (df->dwObjSize != sizeof(DIOBJECTDATAFORMAT)) {
	WARN("Non-standard DIOBJECTDATAFORMAT structure size (%ld instead of %d).\n", df->dwObjSize, sizeof(DIOBJECTDATAFORMAT));
    }
    TRACE("  - dwFlags: 0x%08lx (", df->dwFlags);
    switch (df->dwFlags) {
        case DIDF_ABSAXIS: TRACE("DIDF_ABSAXIS"); break;
	case DIDF_RELAXIS: TRACE("DIDF_RELAXIS"); break;
	default: TRACE("unknown"); break;
    }
    TRACE(")\n");
    TRACE("  - dwDataSize: %ld\n", df->dwDataSize);
    TRACE("  - dwNumObjs: %ld\n", df->dwNumObjs);

    for (i = 0; i < df->dwNumObjs; i++) {
	TRACE("  - Object %d:\n", i);
	TRACE("      * GUID: %s ('%s')\n", debugstr_guid(df->rgodf[i].pguid), _dump_dinput_GUID(df->rgodf[i].pguid));
	TRACE("      * dwOfs: %ld\n", df->rgodf[i].dwOfs);
	TRACE("      * dwType: 0x%08lx\n", df->rgodf[i].dwType);
	TRACE("        "); _dump_EnumObjects_flags(df->rgodf[i].dwType); TRACE("\n");
	TRACE("      * dwFlags: 0x%08lx\n", df->rgodf[i].dwFlags);
    }
}

/* Conversion between internal data buffer and external data buffer */
void fill_DataFormat(void *out, const void *in, DataFormat *df) {
    int i;
    char *in_c = (char *) in;
    char *out_c = (char *) out;

    if (df->dt == NULL) {
	/* This means that the app uses Wine's internal data format */
	memcpy(out, in, df->internal_format_size);
    } else {
	for (i = 0; i < df->size; i++) {
	    if (df->dt[i].offset_in >= 0) {
		switch (df->dt[i].size) {
		    case 1:
		        TRACE("Copying (c) to %d from %d (value %d)\n",
			      df->dt[i].offset_out, df->dt[i].offset_in, *((char *) (in_c + df->dt[i].offset_in)));
			*((char *) (out_c + df->dt[i].offset_out)) = *((char *) (in_c + df->dt[i].offset_in));
			break;

		    case 2:
			TRACE("Copying (s) to %d from %d (value %d)\n",
			      df->dt[i].offset_out, df->dt[i].offset_in, *((short *) (in_c + df->dt[i].offset_in)));
			*((short *) (out_c + df->dt[i].offset_out)) = *((short *) (in_c + df->dt[i].offset_in));
			break;

		    case 4:
			TRACE("Copying (i) to %d from %d (value %d)\n",
			      df->dt[i].offset_out, df->dt[i].offset_in, *((int *) (in_c + df->dt[i].offset_in)));
			*((int *) (out_c + df->dt[i].offset_out)) = *((int *) (in_c + df->dt[i].offset_in));
			break;

		    default:
			memcpy((out_c + df->dt[i].offset_out), (in_c + df->dt[i].offset_in), df->dt[i].size);
			break;
		}
	    } else {
		switch (df->dt[i].size) {
		    case 1:
		        TRACE("Copying (c) to %d default value %d\n",
			      df->dt[i].offset_out, df->dt[i].value);
			*((char *) (out_c + df->dt[i].offset_out)) = (char) df->dt[i].value;
			break;

		    case 2:
			TRACE("Copying (s) to %d default value %d\n",
			      df->dt[i].offset_out, df->dt[i].value);
			*((short *) (out_c + df->dt[i].offset_out)) = (short) df->dt[i].value;
			break;

		    case 4:
			TRACE("Copying (i) to %d default value %d\n",
			      df->dt[i].offset_out, df->dt[i].value);
			*((int *) (out_c + df->dt[i].offset_out)) = (int) df->dt[i].value;
			break;

		    default:
			memset((out_c + df->dt[i].offset_out), df->dt[i].size, 0);
			break;
		}
	    }
	}
    }
}

void release_DataFormat(DataFormat * format)
{
    TRACE("Deleting DataTransform :\n");

    HeapFree(GetProcessHeap(), 0, format->dt);
}

DataFormat *create_DataFormat(const DIDATAFORMAT *wine_format, LPCDIDATAFORMAT asked_format, int *offset) {
    DataFormat *ret;
    DataTransform *dt;
    unsigned int i, j;
    int same = 1;
    int *done;
    int index = 0;
    DWORD next = 0;

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(DataFormat));

    done = HeapAlloc(GetProcessHeap(), 0, sizeof(int) * asked_format->dwNumObjs);
    memset(done, 0, sizeof(int) * asked_format->dwNumObjs);

    dt = HeapAlloc(GetProcessHeap(), 0, asked_format->dwNumObjs * sizeof(DataTransform));

    TRACE("Creating DataTransform :\n");

    for (i = 0; i < wine_format->dwNumObjs; i++) {
	offset[i] = -1;

	for (j = 0; j < asked_format->dwNumObjs; j++) {
	    if (done[j] == 1)
		continue;

	    if (/* Check if the application either requests any GUID and if not, it if matches
		 * the GUID of the Wine object.
		 */
		((asked_format->rgodf[j].pguid == NULL) ||
		 (wine_format->rgodf[i].pguid == NULL) ||
		 (IsEqualGUID(wine_format->rgodf[i].pguid, asked_format->rgodf[j].pguid)))
		&&
		(/* Then check if it accepts any instance id, and if not, if it matches Wine's
		  * instance id.
		  */
		 (DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType) == 0xFFFF) ||
		 (DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType) == 0x00FF) || /* This is mentionned in no DX docs, but it works fine - tested on WinXP */
		 (DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType) == DIDFT_GETINSTANCE(wine_format->rgodf[i].dwType)))
		&&
		( /* Then if the asked type matches the one Wine provides */
		 wine_format->rgodf[i].dwType & asked_format->rgodf[j].dwType)) {

		done[j] = 1;

		TRACE("Matching :\n");
		TRACE("   - Asked (%d) :\n", j);
		TRACE("       * GUID: %s ('%s')\n",
		      debugstr_guid(asked_format->rgodf[j].pguid),
		      _dump_dinput_GUID(asked_format->rgodf[j].pguid));
		TRACE("       * Offset: %3ld\n", asked_format->rgodf[j].dwOfs);
		TRACE("       * dwType: %08lx\n", asked_format->rgodf[j].dwType);
		TRACE("         "); _dump_EnumObjects_flags(asked_format->rgodf[j].dwType); TRACE("\n");

		TRACE("   - Wine  (%d) :\n", i);
		TRACE("       * GUID: %s ('%s')\n",
		      debugstr_guid(wine_format->rgodf[i].pguid),
		      _dump_dinput_GUID(wine_format->rgodf[i].pguid));
		TRACE("       * Offset: %3ld\n", wine_format->rgodf[i].dwOfs);
		TRACE("       * dwType: %08lx\n", wine_format->rgodf[i].dwType);
		TRACE("         "); _dump_EnumObjects_flags(wine_format->rgodf[i].dwType); TRACE("\n");

		if (wine_format->rgodf[i].dwType & DIDFT_BUTTON)
		    dt[index].size = sizeof(BYTE);
		else
		    dt[index].size = sizeof(DWORD);
		dt[index].offset_in = wine_format->rgodf[i].dwOfs;
                if (asked_format->rgodf[j].dwOfs < next) {
                    WARN("bad format: dwOfs=%ld, changing to %ld\n", asked_format->rgodf[j].dwOfs, next);
		    dt[index].offset_out = next;
		    offset[i] = next;
                } else {
		    dt[index].offset_out = asked_format->rgodf[j].dwOfs;
                    offset[i] = asked_format->rgodf[j].dwOfs;
                }
		dt[index].value = 0;
                next = next + dt[index].size;

		if (wine_format->rgodf[i].dwOfs != dt[index].offset_out)
		    same = 0;

		index++;
		break;
	    }
	}

	if (j == asked_format->dwNumObjs)
	    same = 0;
    }

    TRACE("Setting to default value :\n");
    for (j = 0; j < asked_format->dwNumObjs; j++) {
	if (done[j] == 0) {
	    TRACE("   - Asked (%d) :\n", j);
	    TRACE("       * GUID: %s ('%s')\n",
		  debugstr_guid(asked_format->rgodf[j].pguid),
		  _dump_dinput_GUID(asked_format->rgodf[j].pguid));
	    TRACE("       * Offset: %3ld\n", asked_format->rgodf[j].dwOfs);
	    TRACE("       * dwType: %08lx\n", asked_format->rgodf[j].dwType);
	    TRACE("         "); _dump_EnumObjects_flags(asked_format->rgodf[j].dwType); TRACE("\n");

	    if (asked_format->rgodf[j].dwType & DIDFT_BUTTON)
		dt[index].size = sizeof(BYTE);
	    else
		dt[index].size = sizeof(DWORD);
	    dt[index].offset_in  = -1;
	    dt[index].offset_out = asked_format->rgodf[j].dwOfs;
	    dt[index].value = 0;
	    index++;

	    same = 0;
	}
    }

    ret->internal_format_size = wine_format->dwDataSize;
    ret->size = index;
    if (same) {
	ret->dt = NULL;
	HeapFree(GetProcessHeap(), 0, dt);
    } else {
	ret->dt = dt;
    }

    HeapFree(GetProcessHeap(), 0, done);

    return ret;
}

BOOL DIEnumDevicesCallbackAtoW(LPCDIDEVICEOBJECTINSTANCEA lpddi, LPVOID lpvRef) {
    DIDEVICEOBJECTINSTANCEW ddtmp;
    device_enumobjects_AtoWcb_data* data;

    data = (device_enumobjects_AtoWcb_data*) lpvRef;

    memset(&ddtmp, 0, sizeof(ddtmp));

    ddtmp.dwSize = sizeof(DIDEVICEINSTANCEW);
    ddtmp.guidType     = lpddi->guidType;
    ddtmp.dwOfs        = lpddi->dwOfs;
    ddtmp.dwType       = lpddi->dwType;
    ddtmp.dwFlags      = lpddi->dwFlags;
    MultiByteToWideChar(CP_ACP, 0, lpddi->tszName, -1, ddtmp.tszName, MAX_PATH);

    if (lpddi->dwSize == sizeof(DIDEVICEINSTANCEA)) {
	/**
	 * if dwSize < sizeof(DIDEVICEINSTANCEA of DInput version >= 5)
	 *  force feedback and other newer datas aren't available
	 */
	ddtmp.dwFFMaxForce        = lpddi->dwFFMaxForce;
	ddtmp.dwFFForceResolution = lpddi->dwFFForceResolution;
	ddtmp.wCollectionNumber   = lpddi->wCollectionNumber;
	ddtmp.wDesignatorIndex    = lpddi->wDesignatorIndex;
	ddtmp.wUsagePage          = lpddi->wUsagePage;
	ddtmp.wUsage              = lpddi->wUsage;
	ddtmp.dwDimension         = lpddi->dwDimension;
	ddtmp.wExponent           = lpddi->wExponent;
	ddtmp.wReserved           = lpddi->wReserved;
    }
    return data->lpCallBack(&ddtmp, data->lpvRef);
}

/******************************************************************************
 *	IDirectInputDeviceA
 */

HRESULT WINAPI IDirectInputDevice2AImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE8A iface,LPCDIDATAFORMAT df
) {
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;

    TRACE("(this=%p,%p)\n",This,df);

    _dump_DIDATAFORMAT(df);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE8A iface,HWND hwnd,DWORD dwflags
) {
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    TRACE("(this=%p,%p,0x%08lx)\n",This,hwnd,dwflags);
    if (TRACE_ON(dinput)) {
	TRACE(" cooperative level : ");
	_dump_cooperativelevel_DI(dwflags);
    }
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE8A iface,HANDLE hnd
) {
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    FIXME("(this=%p,%p): stub\n",This,hnd);
    return DI_OK;
}

ULONG WINAPI IDirectInputDevice2AImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    ULONG ref;
    ref = InterlockedDecrement(&(This->ref));
    if (ref == 0)
        HeapFree(GetProcessHeap(),0,This);
    return ref;
}

HRESULT WINAPI IDirectInputDevice2AImpl_QueryInterface(
	LPDIRECTINPUTDEVICE8A iface,REFIID riid,LPVOID *ppobj
)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;

    TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(&IID_IUnknown,riid)) {
	IDirectInputDevice2_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    if (IsEqualGUID(&IID_IDirectInputDeviceA,riid)) {
	IDirectInputDevice2_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    if (IsEqualGUID(&IID_IDirectInputDevice2A,riid)) {
	IDirectInputDevice2_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    if (IsEqualGUID(&IID_IDirectInputDevice7A,riid)) {
	IDirectInputDevice7_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    if (IsEqualGUID(&IID_IDirectInputDevice8A,riid)) {
	IDirectInputDevice8_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    TRACE("Unsupported interface !\n");
    return E_FAIL;
}

HRESULT WINAPI IDirectInputDevice2WImpl_QueryInterface(
	LPDIRECTINPUTDEVICE8W iface,REFIID riid,LPVOID *ppobj
)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;

    TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(&IID_IUnknown,riid)) {
	IDirectInputDevice2_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    if (IsEqualGUID(&IID_IDirectInputDeviceW,riid)) {
	IDirectInputDevice2_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    if (IsEqualGUID(&IID_IDirectInputDevice2W,riid)) {
	IDirectInputDevice2_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    if (IsEqualGUID(&IID_IDirectInputDevice7W,riid)) {
	IDirectInputDevice7_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    if (IsEqualGUID(&IID_IDirectInputDevice8W,riid)) {
	IDirectInputDevice8_AddRef(iface);
	*ppobj = This;
	return DI_OK;
    }
    TRACE("Unsupported interface !\n");
    return E_FAIL;
}

ULONG WINAPI IDirectInputDevice2AImpl_AddRef(
	LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    return InterlockedIncrement(&(This->ref));
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
    FIXME("(this=%p,%p,%p,%08lx): stub!\n", iface, lpCallback, lpvRef, dwFlags);
    if (TRACE_ON(dinput)) {
	DPRINTF("  - flags = ");
	_dump_EnumObjects_flags(dwFlags);
	DPRINTF("\n");
    }

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_EnumObjects(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
    FIXME("(this=%p,%p,%p,%08lx): stub!\n", iface, lpCallback, lpvRef, dwFlags);
    if (TRACE_ON(dinput)) {
	DPRINTF("  - flags = ");
	_dump_EnumObjects_flags(dwFlags);
	DPRINTF("\n");
    }

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetProperty(
	LPDIRECTINPUTDEVICE8A iface,
	REFGUID rguid,
	LPDIPROPHEADER pdiph)
{
    FIXME("(this=%p,%s,%p): stub!\n",
	  iface, debugstr_guid(rguid), pdiph);

    if (TRACE_ON(dinput))
	_dump_DIPROPHEADER(pdiph);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
    FIXME("(this=%p,%p,%ld,0x%08lx): stub!\n",
	  iface, pdidoi, dwObj, dwHow);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIDEVICEOBJECTINSTANCEW pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
    FIXME("(this=%p,%p,%ld,0x%08lx): stub!\n",
	  iface, pdidoi, dwObj, dwHow);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
    FIXME("(this=%p,%p): stub!\n",
	  iface, pdidi);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIDEVICEINSTANCEW pdidi)
{
    FIXME("(this=%p,%p): stub!\n",
	  iface, pdidi);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel(
	LPDIRECTINPUTDEVICE8A iface,
	HWND hwndOwner,
	DWORD dwFlags)
{
    FIXME("(this=%p,%p,0x%08lx): stub!\n",
	  iface, hwndOwner, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Initialize(
	LPDIRECTINPUTDEVICE8A iface,
	HINSTANCE hinst,
	DWORD dwVersion,
	REFGUID rguid)
{
    FIXME("(this=%p,%p,%ld,%s): stub!\n",
	  iface, hinst, dwVersion, debugstr_guid(rguid));
    return DI_OK;
}

/******************************************************************************
 *	IDirectInputDevice2A
 */

HRESULT WINAPI IDirectInputDevice2AImpl_CreateEffect(
	LPDIRECTINPUTDEVICE8A iface,
	REFGUID rguid,
	LPCDIEFFECT lpeff,
	LPDIRECTINPUTEFFECT *ppdef,
	LPUNKNOWN pUnkOuter)
{
    FIXME("(this=%p,%s,%p,%p,%p): stub!\n",
	  iface, debugstr_guid(rguid), lpeff, ppdef, pUnkOuter);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumEffects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMEFFECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
    FIXME("(this=%p,%p,%p,0x%08lx): stub!\n",
	  iface, lpCallback, lpvRef, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_EnumEffects(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIENUMEFFECTSCALLBACKW lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
    FIXME("(this=%p,%p,%p,0x%08lx): stub!\n",
	  iface, lpCallback, lpvRef, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetEffectInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIEFFECTINFOA lpdei,
	REFGUID rguid)
{
    FIXME("(this=%p,%p,%s): stub!\n",
	  iface, lpdei, debugstr_guid(rguid));
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetEffectInfo(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIEFFECTINFOW lpdei,
	REFGUID rguid)
{
    FIXME("(this=%p,%p,%s): stub!\n",
	  iface, lpdei, debugstr_guid(rguid));
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE8A iface,
	LPDWORD pdwOut)
{
    FIXME("(this=%p,%p): stub!\n",
	  iface, pdwOut);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE8A iface,
	DWORD dwFlags)
{
    FIXME("(this=%p,0x%08lx): stub!\n",
	  iface, dwFlags);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
    FIXME("(this=%p,%p,%p,0x%08lx): stub!\n",
	  iface, lpCallback, lpvRef, dwFlags);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Escape(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIEFFESCAPE lpDIEEsc)
{
    FIXME("(this=%p,%p): stub!\n",
	  iface, lpDIEEsc);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Poll(
	LPDIRECTINPUTDEVICE8A iface)
{
    /* Because wine devices do not need to be polled, just return DI_NOEFFECT */
    return DI_NOEFFECT;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SendDeviceData(
	LPDIRECTINPUTDEVICE8A iface,
	DWORD cbObjectData,
	LPCDIDEVICEOBJECTDATA rgdod,
	LPDWORD pdwInOut,
	DWORD dwFlags)
{
    FIXME("(this=%p,0x%08lx,%p,%p,0x%08lx): stub!\n",
	  iface, cbObjectData, rgdod, pdwInOut, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7AImpl_EnumEffectsInFile(LPDIRECTINPUTDEVICE8A iface,
							  LPCSTR lpszFileName,
							  LPDIENUMEFFECTSINFILECALLBACK pec,
							  LPVOID pvRef,
							  DWORD dwFlags)
{
    FIXME("(%p)->(%s,%p,%p,%08lx): stub !\n", iface, lpszFileName, pec, pvRef, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7WImpl_EnumEffectsInFile(LPDIRECTINPUTDEVICE8W iface,
							  LPCWSTR lpszFileName,
							  LPDIENUMEFFECTSINFILECALLBACK pec,
							  LPVOID pvRef,
							  DWORD dwFlags)
{
    FIXME("(%p)->(%s,%p,%p,%08lx): stub !\n", iface, debugstr_w(lpszFileName), pec, pvRef, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7AImpl_WriteEffectToFile(LPDIRECTINPUTDEVICE8A iface,
							  LPCSTR lpszFileName,
							  DWORD dwEntries,
							  LPDIFILEEFFECT rgDiFileEft,
							  DWORD dwFlags)
{
    FIXME("(%p)->(%s,%08lx,%p,%08lx): stub !\n", iface, lpszFileName, dwEntries, rgDiFileEft, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7WImpl_WriteEffectToFile(LPDIRECTINPUTDEVICE8W iface,
							  LPCWSTR lpszFileName,
							  DWORD dwEntries,
							  LPDIFILEEFFECT rgDiFileEft,
							  DWORD dwFlags)
{
    FIXME("(%p)->(%s,%08lx,%p,%08lx): stub !\n", iface, debugstr_w(lpszFileName), dwEntries, rgDiFileEft, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8AImpl_BuildActionMap(LPDIRECTINPUTDEVICE8A iface,
						       LPDIACTIONFORMATA lpdiaf,
						       LPCSTR lpszUserName,
						       DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08lx): stub !\n", iface, lpdiaf, lpszUserName, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8WImpl_BuildActionMap(LPDIRECTINPUTDEVICE8W iface,
						       LPDIACTIONFORMATW lpdiaf,
						       LPCWSTR lpszUserName,
						       DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08lx): stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8AImpl_SetActionMap(LPDIRECTINPUTDEVICE8A iface,
						     LPDIACTIONFORMATA lpdiaf,
						     LPCSTR lpszUserName,
						     DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08lx): stub !\n", iface, lpdiaf, lpszUserName, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8WImpl_SetActionMap(LPDIRECTINPUTDEVICE8W iface,
						     LPDIACTIONFORMATW lpdiaf,
						     LPCWSTR lpszUserName,
						     DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08lx): stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8AImpl_GetImageInfo(LPDIRECTINPUTDEVICE8A iface,
						     LPDIDEVICEIMAGEINFOHEADERA lpdiDevImageInfoHeader)
{
    FIXME("(%p)->(%p): stub !\n", iface, lpdiDevImageInfoHeader);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8WImpl_GetImageInfo(LPDIRECTINPUTDEVICE8W iface,
						     LPDIDEVICEIMAGEINFOHEADERW lpdiDevImageInfoHeader)
{
    FIXME("(%p)->(%p): stub !\n", iface, lpdiDevImageInfoHeader);

    return DI_OK;
}
