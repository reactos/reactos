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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"
#include "device_private.h"
#include "dinput_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/******************************************************************************
 *	Various debugging tools
 */
static void _dump_cooperativelevel_DI(DWORD dwFlags) {
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
	    FE(DISCL_NONEXCLUSIVE),
	    FE(DISCL_NOWINKEY)
#undef FE
	};
	TRACE(" cooperative level : ");
	for (i = 0; i < (sizeof(flags) / sizeof(flags[0])); i++)
	    if (flags[i].mask & dwFlags)
		TRACE("%s ",flags[i].name);
	TRACE("\n");
    }
}

static void _dump_EnumObjects_flags(DWORD dwFlags) {
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
	TRACE("Type:");
	if (type == DIDFT_ALL) {
	    TRACE(" DIDFT_ALL");
	} else {
	    for (i = 0; i < (sizeof(flags) / sizeof(flags[0])); i++) {
		if (flags[i].mask & type) {
		    type &= ~flags[i].mask;
		    TRACE(" %s",flags[i].name);
		}
	    }
	    if (type) {
                TRACE(" (unhandled: %08x)", type);
	    }
	}
	TRACE(" / Instance: ");
	if (instance == ((DIDFT_ANYINSTANCE >> 8) & 0xFFFF)) {
	    TRACE("DIDFT_ANYINSTANCE");
	} else {
            TRACE("%3d", instance);
	}
    }
}

void _dump_DIPROPHEADER(LPCDIPROPHEADER diph) {
    if (TRACE_ON(dinput)) {
        TRACE("  - dwObj = 0x%08x\n", diph->dwObj);
        TRACE("  - dwHow = %s\n",
            ((diph->dwHow == DIPH_DEVICE) ? "DIPH_DEVICE" :
            ((diph->dwHow == DIPH_BYOFFSET) ? "DIPH_BYOFFSET" :
            ((diph->dwHow == DIPH_BYID)) ? "DIPH_BYID" : "unknown")));
    }
}

void _dump_OBJECTINSTANCEA(const DIDEVICEOBJECTINSTANCEA *ddoi) {
    TRACE("    - enumerating : %s ('%s') - %2d - 0x%08x - %s\n",
        debugstr_guid(&ddoi->guidType), _dump_dinput_GUID(&ddoi->guidType), ddoi->dwOfs, ddoi->dwType, ddoi->tszName);
}

void _dump_OBJECTINSTANCEW(const DIDEVICEOBJECTINSTANCEW *ddoi) {
    TRACE("    - enumerating : %s ('%s'), - %2d - 0x%08x - %s\n",
        debugstr_guid(&ddoi->guidType), _dump_dinput_GUID(&ddoi->guidType), ddoi->dwOfs, ddoi->dwType, debugstr_w(ddoi->tszName));
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
    return debugstr_guid(guid);
}

void _dump_DIDATAFORMAT(const DIDATAFORMAT *df) {
    unsigned int i;

    TRACE("Dumping DIDATAFORMAT structure:\n");
    TRACE("  - dwSize: %d\n", df->dwSize);
    if (df->dwSize != sizeof(DIDATAFORMAT)) {
        WARN("Non-standard DIDATAFORMAT structure size %d\n", df->dwSize);
    }
    TRACE("  - dwObjsize: %d\n", df->dwObjSize);
    if (df->dwObjSize != sizeof(DIOBJECTDATAFORMAT)) {
        WARN("Non-standard DIOBJECTDATAFORMAT structure size %d\n", df->dwObjSize);
    }
    TRACE("  - dwFlags: 0x%08x (", df->dwFlags);
    switch (df->dwFlags) {
        case DIDF_ABSAXIS: TRACE("DIDF_ABSAXIS"); break;
	case DIDF_RELAXIS: TRACE("DIDF_RELAXIS"); break;
	default: TRACE("unknown"); break;
    }
    TRACE(")\n");
    TRACE("  - dwDataSize: %d\n", df->dwDataSize);
    TRACE("  - dwNumObjs: %d\n", df->dwNumObjs);
    
    for (i = 0; i < df->dwNumObjs; i++) {
	TRACE("  - Object %d:\n", i);
	TRACE("      * GUID: %s ('%s')\n", debugstr_guid(df->rgodf[i].pguid), _dump_dinput_GUID(df->rgodf[i].pguid));
        TRACE("      * dwOfs: %d\n", df->rgodf[i].dwOfs);
        TRACE("      * dwType: 0x%08x\n", df->rgodf[i].dwType);
	TRACE("        "); _dump_EnumObjects_flags(df->rgodf[i].dwType); TRACE("\n");
        TRACE("      * dwFlags: 0x%08x\n", df->rgodf[i].dwFlags);
    }
}

/******************************************************************************
 * Get the default and the app-specific config keys.
 */
BOOL get_app_key(HKEY *defkey, HKEY *appkey)
{
    char buffer[MAX_PATH+16];
    DWORD len;

    *appkey = 0;

    /* @@ Wine registry key: HKCU\Software\Wine\DirectInput */
    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\DirectInput", defkey))
        *defkey = 0;

    len = GetModuleFileNameA(0, buffer, MAX_PATH);
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\DirectInput */
        if (!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey))
        {
            char *p, *appname = buffer;
            if ((p = strrchr(appname, '/'))) appname = p + 1;
            if ((p = strrchr(appname, '\\'))) appname = p + 1;
            strcat(appname, "\\DirectInput");

            if (RegOpenKeyA(tmpkey, appname, appkey)) *appkey = 0;
            RegCloseKey(tmpkey);
        }
    }

    return *defkey || *appkey;
}

/******************************************************************************
 * Get a config key from either the app-specific or the default config
 */
DWORD get_config_key( HKEY defkey, HKEY appkey, const char *name,
                             char *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE)buffer, &size ))
        return 0;

    if (defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE)buffer, &size ))
        return 0;

    return ERROR_FILE_NOT_FOUND;
}

/* Conversion between internal data buffer and external data buffer */
void fill_DataFormat(void *out, DWORD size, const void *in, const DataFormat *df)
{
    int i;
    const char *in_c = in;
    char *out_c = out;

    memset(out, 0, size);
    if (df->dt == NULL) {
	/* This means that the app uses Wine's internal data format */
        memcpy(out, in, min(size, df->internal_format_size));
    } else {
	for (i = 0; i < df->size; i++) {
	    if (df->dt[i].offset_in >= 0) {
		switch (df->dt[i].size) {
		    case 1:
		        TRACE("Copying (c) to %d from %d (value %d)\n",
                              df->dt[i].offset_out, df->dt[i].offset_in, *(in_c + df->dt[i].offset_in));
			*(out_c + df->dt[i].offset_out) = *(in_c + df->dt[i].offset_in);
			break;

		    case 2:
			TRACE("Copying (s) to %d from %d (value %d)\n",
			      df->dt[i].offset_out, df->dt[i].offset_in, *((const short *)(in_c + df->dt[i].offset_in)));
			*((short *)(out_c + df->dt[i].offset_out)) = *((const short *)(in_c + df->dt[i].offset_in));
			break;

		    case 4:
			TRACE("Copying (i) to %d from %d (value %d)\n",
                              df->dt[i].offset_out, df->dt[i].offset_in, *((const int *)(in_c + df->dt[i].offset_in)));
                        *((int *)(out_c + df->dt[i].offset_out)) = *((const int *)(in_c + df->dt[i].offset_in));
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
			*(out_c + df->dt[i].offset_out) = (char) df->dt[i].value;
			break;
			
		    case 2:
			TRACE("Copying (s) to %d default value %d\n",
			      df->dt[i].offset_out, df->dt[i].value);
			*((short *) (out_c + df->dt[i].offset_out)) = (short) df->dt[i].value;
			break;
			
		    case 4:
			TRACE("Copying (i) to %d default value %d\n",
			      df->dt[i].offset_out, df->dt[i].value);
			*((int *) (out_c + df->dt[i].offset_out)) = df->dt[i].value;
			break;
			
		    default:
			memset((out_c + df->dt[i].offset_out), 0, df->dt[i].size);
			break;
		}
	    }
	}
    }
}

void release_DataFormat(DataFormat * format)
{
    TRACE("Deleting DataFormat: %p\n", format);

    HeapFree(GetProcessHeap(), 0, format->dt);
    format->dt = NULL;
    HeapFree(GetProcessHeap(), 0, format->offsets);
    format->offsets = NULL;
    HeapFree(GetProcessHeap(), 0, format->user_df);
    format->user_df = NULL;
}

static inline LPDIOBJECTDATAFORMAT dataformat_to_odf(LPCDIDATAFORMAT df, int idx)
{
    if (idx < 0 || idx >= df->dwNumObjs) return NULL;
    return (LPDIOBJECTDATAFORMAT)((LPBYTE)df->rgodf + idx * df->dwObjSize);
}

static HRESULT create_DataFormat(LPCDIDATAFORMAT asked_format, DataFormat *format)
{
    DataTransform *dt;
    unsigned int i, j;
    int same = 1;
    int *done;
    int index = 0;
    DWORD next = 0;

    if (!format->wine_df) return DIERR_INVALIDPARAM;
    done = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, asked_format->dwNumObjs * sizeof(int));
    dt = HeapAlloc(GetProcessHeap(), 0, asked_format->dwNumObjs * sizeof(DataTransform));
    if (!dt || !done) goto failed;

    if (!(format->offsets = HeapAlloc(GetProcessHeap(), 0, format->wine_df->dwNumObjs * sizeof(int))))
        goto failed;

    if (!(format->user_df = HeapAlloc(GetProcessHeap(), 0, asked_format->dwSize)))
        goto failed;
    memcpy(format->user_df, asked_format, asked_format->dwSize);

    TRACE("Creating DataTransform :\n");
    
    for (i = 0; i < format->wine_df->dwNumObjs; i++)
    {
        format->offsets[i] = -1;

	for (j = 0; j < asked_format->dwNumObjs; j++) {
	    if (done[j] == 1)
		continue;
	    
	    if (/* Check if the application either requests any GUID and if not, it if matches
		 * the GUID of the Wine object.
		 */
		((asked_format->rgodf[j].pguid == NULL) ||
		 (format->wine_df->rgodf[i].pguid == NULL) ||
		 (IsEqualGUID(format->wine_df->rgodf[i].pguid, asked_format->rgodf[j].pguid)))
		&&
		(/* Then check if it accepts any instance id, and if not, if it matches Wine's
		  * instance id.
		  */
		 ((asked_format->rgodf[j].dwType & DIDFT_INSTANCEMASK) == DIDFT_ANYINSTANCE) ||
		 (DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType) == 0x00FF) || /* This is mentionned in no DX docs, but it works fine - tested on WinXP */
		 (DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType) == DIDFT_GETINSTANCE(format->wine_df->rgodf[i].dwType)))
		&&
		( /* Then if the asked type matches the one Wine provides */
                 DIDFT_GETTYPE(asked_format->rgodf[j].dwType) & format->wine_df->rgodf[i].dwType))
            {
		done[j] = 1;
		
		TRACE("Matching :\n");
		TRACE("   - Asked (%d) :\n", j);
		TRACE("       * GUID: %s ('%s')\n",
		      debugstr_guid(asked_format->rgodf[j].pguid),
		      _dump_dinput_GUID(asked_format->rgodf[j].pguid));
                TRACE("       * Offset: %3d\n", asked_format->rgodf[j].dwOfs);
                TRACE("       * dwType: %08x\n", asked_format->rgodf[j].dwType);
		TRACE("         "); _dump_EnumObjects_flags(asked_format->rgodf[j].dwType); TRACE("\n");
		
		TRACE("   - Wine  (%d) :\n", i);
		TRACE("       * GUID: %s ('%s')\n",
                      debugstr_guid(format->wine_df->rgodf[i].pguid),
                      _dump_dinput_GUID(format->wine_df->rgodf[i].pguid));
                TRACE("       * Offset: %3d\n", format->wine_df->rgodf[i].dwOfs);
                TRACE("       * dwType: %08x\n", format->wine_df->rgodf[i].dwType);
                TRACE("         "); _dump_EnumObjects_flags(format->wine_df->rgodf[i].dwType); TRACE("\n");
		
                if (format->wine_df->rgodf[i].dwType & DIDFT_BUTTON)
		    dt[index].size = sizeof(BYTE);
		else
		    dt[index].size = sizeof(DWORD);
                dt[index].offset_in = format->wine_df->rgodf[i].dwOfs;
                dt[index].offset_out = asked_format->rgodf[j].dwOfs;
                format->offsets[i]   = asked_format->rgodf[j].dwOfs;
		dt[index].value = 0;
                next = next + dt[index].size;
		
                if (format->wine_df->rgodf[i].dwOfs != dt[index].offset_out)
		    same = 0;
		
		index++;
		break;
	    }
	}
    }

    TRACE("Setting to default value :\n");
    for (j = 0; j < asked_format->dwNumObjs; j++) {
	if (done[j] == 0) {
	    TRACE("   - Asked (%d) :\n", j);
	    TRACE("       * GUID: %s ('%s')\n",
		  debugstr_guid(asked_format->rgodf[j].pguid),
		  _dump_dinput_GUID(asked_format->rgodf[j].pguid));
            TRACE("       * Offset: %3d\n", asked_format->rgodf[j].dwOfs);
            TRACE("       * dwType: %08x\n", asked_format->rgodf[j].dwType);
	    TRACE("         "); _dump_EnumObjects_flags(asked_format->rgodf[j].dwType); TRACE("\n");
	    
	    if (asked_format->rgodf[j].dwType & DIDFT_BUTTON)
		dt[index].size = sizeof(BYTE);
	    else
		dt[index].size = sizeof(DWORD);
	    dt[index].offset_in  = -1;
	    dt[index].offset_out = asked_format->rgodf[j].dwOfs;
            if (asked_format->rgodf[j].dwType & DIDFT_POV)
                dt[index].value = -1;
            else
                dt[index].value = 0;
	    index++;

	    same = 0;
	}
    }
    
    format->internal_format_size = format->wine_df->dwDataSize;
    format->size = index;
    if (same) {
	HeapFree(GetProcessHeap(), 0, dt);
        dt = NULL;
    }
    format->dt = dt;

    HeapFree(GetProcessHeap(), 0, done);

    return DI_OK;

failed:
    HeapFree(GetProcessHeap(), 0, done);
    HeapFree(GetProcessHeap(), 0, dt);
    format->dt = NULL;
    HeapFree(GetProcessHeap(), 0, format->offsets);
    format->offsets = NULL;
    HeapFree(GetProcessHeap(), 0, format->user_df);
    format->user_df = NULL;

    return DIERR_OUTOFMEMORY;
}

/* find an object by it's offset in a data format */
static int offset_to_object(const DataFormat *df, int offset)
{
    int i;

    if (!df->offsets) return -1;

    for (i = 0; i < df->wine_df->dwNumObjs; i++)
        if (df->offsets[i] == offset) return i;

    return -1;
}

int id_to_object(LPCDIDATAFORMAT df, int id)
{
    int i;

    id &= 0x00ffffff;
    for (i = 0; i < df->dwNumObjs; i++)
        if ((dataformat_to_odf(df, i)->dwType & 0x00ffffff) == id)
            return i;

    return -1;
}

int id_to_offset(const DataFormat *df, int id)
{
    int obj = id_to_object(df->wine_df, id);

    return obj >= 0 && df->offsets ? df->offsets[obj] : -1;
}

int find_property(const DataFormat *df, LPCDIPROPHEADER ph)
{
    switch (ph->dwHow)
    {
        case DIPH_BYID:     return id_to_object(df->wine_df, ph->dwObj);
        case DIPH_BYOFFSET: return offset_to_object(df, ph->dwObj);
    }
    FIXME("Unhandled ph->dwHow=='%04X'\n", (unsigned int)ph->dwHow);

    return -1;
}

/******************************************************************************
 *	queue_event - add new event to the ring queue
 */

void queue_event(LPDIRECTINPUTDEVICE8A iface, int ofs, DWORD data, DWORD time, DWORD seq)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    int next_pos;

    /* Event is being set regardless of the queue state */
    if (This->hEvent) SetEvent(This->hEvent);

    if (!This->queue_len || This->overflow || ofs < 0) return;

    next_pos = (This->queue_head + 1) % This->queue_len;
    if (next_pos == This->queue_tail)
    {
        TRACE(" queue overflowed\n");
        This->overflow = TRUE;
        return;
    }

    TRACE(" queueing %d at offset %d (queue head %d / size %d)\n",
          data, ofs, This->queue_head, This->queue_len);

    This->data_queue[This->queue_head].dwOfs       = ofs;
    This->data_queue[This->queue_head].dwData      = data;
    This->data_queue[This->queue_head].dwTimeStamp = time;
    This->data_queue[This->queue_head].dwSequence  = seq;
    This->queue_head = next_pos;
    /* Send event if asked */
}

/******************************************************************************
 *	Acquire
 */

HRESULT WINAPI IDirectInputDevice2AImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    HRESULT res;

    if (!This->data_format.user_df) return DIERR_INVALIDPARAM;
    if (This->dwCoopLevel & DISCL_FOREGROUND && This->win != GetForegroundWindow())
        return DIERR_OTHERAPPHASPRIO;

    EnterCriticalSection(&This->crit);
    res = This->acquired ? S_FALSE : DI_OK;
    This->acquired = 1;
    if (res == DI_OK)
    {
        This->queue_head = This->queue_tail = This->overflow = 0;
        check_dinput_hooks(iface);
    }
    LeaveCriticalSection(&This->crit);

    return res;
}

/******************************************************************************
 *	Unacquire
 */

HRESULT WINAPI IDirectInputDevice2AImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    HRESULT res;

    EnterCriticalSection(&This->crit);
    res = !This->acquired ? DI_NOEFFECT : DI_OK;
    This->acquired = 0;
    if (res == DI_OK)
        check_dinput_hooks(iface);
    LeaveCriticalSection(&This->crit);

    return res;
}

/******************************************************************************
 *	IDirectInputDeviceA
 */

HRESULT WINAPI IDirectInputDevice2AImpl_SetDataFormat(
        LPDIRECTINPUTDEVICE8A iface, LPCDIDATAFORMAT df)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    HRESULT res = DI_OK;

    if (!df) return E_POINTER;
    TRACE("(%p) %p\n", This, df);
    _dump_DIDATAFORMAT(df);

    if (df->dwSize != sizeof(DIDATAFORMAT)) return DIERR_INVALIDPARAM;
    if (This->acquired) return DIERR_ACQUIRED;

    EnterCriticalSection(&This->crit);

    release_DataFormat(&This->data_format);
    res = create_DataFormat(df, &This->data_format);

    LeaveCriticalSection(&This->crit);
    return res;
}

/******************************************************************************
  *     SetCooperativeLevel
  *
  *  Set cooperative level and the source window for the events.
  */
HRESULT WINAPI IDirectInputDevice2AImpl_SetCooperativeLevel(
        LPDIRECTINPUTDEVICE8A iface, HWND hwnd, DWORD dwflags)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;

    TRACE("(%p) %p,0x%08x\n", This, hwnd, dwflags);
    _dump_cooperativelevel_DI(dwflags);

    if ((dwflags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == 0 ||
        (dwflags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE) ||
        (dwflags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == 0 ||
        (dwflags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == (DISCL_FOREGROUND | DISCL_BACKGROUND))
        return DIERR_INVALIDPARAM;

    if (dwflags == (DISCL_NONEXCLUSIVE | DISCL_BACKGROUND))
        hwnd = GetDesktopWindow();

    if (!hwnd) return E_HANDLE;

    /* For security reasons native does not allow exclusive background level
       for mouse and keyboard only */
    if (dwflags & DISCL_EXCLUSIVE && dwflags & DISCL_BACKGROUND &&
        (IsEqualGUID(&This->guid, &GUID_SysMouse) ||
         IsEqualGUID(&This->guid, &GUID_SysKeyboard)))
        return DIERR_UNSUPPORTED;

    /* Store the window which asks for the mouse */
    EnterCriticalSection(&This->crit);
    This->win = hwnd;
    This->dwCoopLevel = dwflags;
    LeaveCriticalSection(&This->crit);

    return DI_OK;
}

/******************************************************************************
  *     SetEventNotification : specifies event to be sent on state change
  */
HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE8A iface, HANDLE event)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;

    TRACE("(%p) %p\n", This, event);

    EnterCriticalSection(&This->crit);
    This->hEvent = event;
    LeaveCriticalSection(&This->crit);
    return DI_OK;
}

ULONG WINAPI IDirectInputDevice2AImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    ULONG ref;

    ref = InterlockedDecrement(&(This->ref));
    if (ref) return ref;

    IDirectInputDevice_Unacquire(iface);
    /* Reset the FF state, free all effects, etc */
    IDirectInputDevice8_SendForceFeedbackCommand(iface, DISFFC_RESET);

    HeapFree(GetProcessHeap(), 0, This->data_queue);

    /* Free data format */
    HeapFree(GetProcessHeap(), 0, This->data_format.wine_df->rgodf);
    HeapFree(GetProcessHeap(), 0, This->data_format.wine_df);
    release_DataFormat(&This->data_format);

    EnterCriticalSection( &This->dinput->crit );
    list_remove( &This->entry );
    LeaveCriticalSection( &This->dinput->crit );

    IDirectInput_Release((LPDIRECTINPUTDEVICE8A)This->dinput);
    This->crit.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->crit);

    HeapFree(GetProcessHeap(), 0, This);

    return DI_OK;
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

HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects(LPDIRECTINPUTDEVICE8A iface,
        LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback, LPVOID lpvRef, DWORD dwFlags)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    DIDEVICEOBJECTINSTANCEA ddoi;
    int i;

    TRACE("(%p) %p,%p flags:%08x)\n", iface, lpCallback, lpvRef, dwFlags);
    TRACE("  - flags = ");
    _dump_EnumObjects_flags(dwFlags);
    TRACE("\n");

    /* Only the fields till dwFFMaxForce are relevant */
    memset(&ddoi, 0, sizeof(ddoi));
    ddoi.dwSize = FIELD_OFFSET(DIDEVICEOBJECTINSTANCEA, dwFFMaxForce);

    for (i = 0; i < This->data_format.wine_df->dwNumObjs; i++)
    {
        LPDIOBJECTDATAFORMAT odf = dataformat_to_odf(This->data_format.wine_df, i);

        if (dwFlags != DIDFT_ALL && !(dwFlags & DIEFT_GETTYPE(odf->dwType))) continue;
        if (IDirectInputDevice_GetObjectInfo(iface, &ddoi, odf->dwType, DIPH_BYID) != DI_OK)
            continue;

	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) break;
    }

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_EnumObjects(LPDIRECTINPUTDEVICE8W iface,
        LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback, LPVOID lpvRef, DWORD dwFlags)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    DIDEVICEOBJECTINSTANCEW ddoi;
    int i;

    TRACE("(%p) %p,%p flags:%08x)\n", iface, lpCallback, lpvRef, dwFlags);
    TRACE("  - flags = ");
    _dump_EnumObjects_flags(dwFlags);
    TRACE("\n");

    /* Only the fields till dwFFMaxForce are relevant */
    memset(&ddoi, 0, sizeof(ddoi));
    ddoi.dwSize = FIELD_OFFSET(DIDEVICEOBJECTINSTANCEW, dwFFMaxForce);

    for (i = 0; i < This->data_format.wine_df->dwNumObjs; i++)
    {
        LPDIOBJECTDATAFORMAT odf = dataformat_to_odf(This->data_format.wine_df, i);

        if (dwFlags != DIDFT_ALL && !(dwFlags & DIEFT_GETTYPE(odf->dwType))) continue;
        if (IDirectInputDevice_GetObjectInfo(iface, &ddoi, odf->dwType, DIPH_BYID) != DI_OK)
            continue;

	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) break;
    }

    return DI_OK;
}

/******************************************************************************
 *	GetProperty
 */

HRESULT WINAPI IDirectInputDevice2AImpl_GetProperty(
	LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;

    TRACE("(%p) %s,%p\n", iface, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (HIWORD(rguid)) return DI_OK;

    switch (LOWORD(rguid))
    {
        case (DWORD_PTR) DIPROP_BUFFERSIZE:
        {
            LPDIPROPDWORD pd = (LPDIPROPDWORD)pdiph;

            if (pdiph->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;

            pd->dwData = This->queue_len;
            TRACE("buffersize = %d\n", pd->dwData);
            break;
        }
        default:
            WARN("Unknown property %s\n", debugstr_guid(rguid));
            break;
    }

    return DI_OK;
}

/******************************************************************************
 *	SetProperty
 */

HRESULT WINAPI IDirectInputDevice2AImpl_SetProperty(
        LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPCDIPROPHEADER pdiph)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;

    TRACE("(%p) %s,%p\n", iface, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (HIWORD(rguid)) return DI_OK;

    switch (LOWORD(rguid))
    {
        case (DWORD_PTR) DIPROP_AXISMODE:
        {
            LPCDIPROPDWORD pd = (LPCDIPROPDWORD)pdiph;

            if (pdiph->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
            if (pdiph->dwHow == DIPH_DEVICE && pdiph->dwObj) return DIERR_INVALIDPARAM;
            if (This->acquired) return DIERR_ACQUIRED;
            if (pdiph->dwHow != DIPH_DEVICE) return DIERR_UNSUPPORTED;
            if (!This->data_format.user_df) return DI_OK;

            TRACE("Axis mode: %s\n", pd->dwData == DIPROPAXISMODE_ABS ? "absolute" :
                                                                        "relative");

            EnterCriticalSection(&This->crit);
            This->data_format.user_df->dwFlags &= ~DIDFT_AXIS;
            This->data_format.user_df->dwFlags |= pd->dwData == DIPROPAXISMODE_ABS ?
                                                  DIDF_ABSAXIS : DIDF_RELAXIS;
            LeaveCriticalSection(&This->crit);
            break;
        }
        case (DWORD_PTR) DIPROP_BUFFERSIZE:
        {
            LPCDIPROPDWORD pd = (LPCDIPROPDWORD)pdiph;

            if (pdiph->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
            if (This->acquired) return DIERR_ACQUIRED;

            TRACE("buffersize = %d\n", pd->dwData);

            EnterCriticalSection(&This->crit);
            HeapFree(GetProcessHeap(), 0, This->data_queue);

            This->data_queue = !pd->dwData ? NULL : HeapAlloc(GetProcessHeap(), 0,
                                pd->dwData * sizeof(DIDEVICEOBJECTDATA));
            This->queue_head = This->queue_tail = This->overflow = 0;
            This->queue_len  = pd->dwData;

            LeaveCriticalSection(&This->crit);
            break;
        }
        default:
            WARN("Unknown property %s\n", debugstr_guid(rguid));
            return DIERR_UNSUPPORTED;
    }

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
    DIDEVICEOBJECTINSTANCEW didoiW;
    HRESULT res;

    if (!pdidoi ||
        (pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCEA) &&
         pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3A)))
        return DIERR_INVALIDPARAM;

    didoiW.dwSize = sizeof(didoiW);
    res = IDirectInputDevice2WImpl_GetObjectInfo((LPDIRECTINPUTDEVICE8W)iface, &didoiW, dwObj, dwHow);
    if (res == DI_OK)
    {
        DWORD dwSize = pdidoi->dwSize;

        memset(pdidoi, 0, pdidoi->dwSize);
        pdidoi->dwSize   = dwSize;
        pdidoi->guidType = didoiW.guidType;
        pdidoi->dwOfs    = didoiW.dwOfs;
        pdidoi->dwType   = didoiW.dwType;
        pdidoi->dwFlags  = didoiW.dwFlags;
    }

    return res;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIDEVICEOBJECTINSTANCEW pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    DWORD dwSize;
    LPDIOBJECTDATAFORMAT odf;
    int idx = -1;

    TRACE("(%p) %d(0x%08x) -> %p\n", This, dwHow, dwObj, pdidoi);

    if (!pdidoi ||
        (pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCEW) &&
         pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3W)))
        return DIERR_INVALIDPARAM;

    switch (dwHow)
    {
    case DIPH_BYOFFSET:
        if (!This->data_format.offsets) break;
        for (idx = This->data_format.wine_df->dwNumObjs - 1; idx >= 0; idx--)
            if (This->data_format.offsets[idx] == dwObj) break;
        break;
    case DIPH_BYID:
        dwObj &= 0x00ffffff;
        for (idx = This->data_format.wine_df->dwNumObjs - 1; idx >= 0; idx--)
            if ((dataformat_to_odf(This->data_format.wine_df, idx)->dwType & 0x00ffffff) == dwObj)
                break;
        break;

    case DIPH_BYUSAGE:
        FIXME("dwHow = DIPH_BYUSAGE not implemented\n");
        break;
    default:
        WARN("invalid parameter: dwHow = %08x\n", dwHow);
        return DIERR_INVALIDPARAM;
    }
    if (idx < 0) return DIERR_OBJECTNOTFOUND;

    odf = dataformat_to_odf(This->data_format.wine_df, idx);
    dwSize = pdidoi->dwSize; /* save due to memset below */
    memset(pdidoi, 0, pdidoi->dwSize);
    pdidoi->dwSize   = dwSize;
    if (odf->pguid) pdidoi->guidType = *odf->pguid;
    pdidoi->dwOfs    = This->data_format.offsets ? This->data_format.offsets[idx] : odf->dwOfs;
    pdidoi->dwType   = odf->dwType;
    pdidoi->dwFlags  = odf->dwFlags;

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceData(
        LPDIRECTINPUTDEVICE8A iface, DWORD dodsize, LPDIDEVICEOBJECTDATA dod,
        LPDWORD entries, DWORD flags)
{
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;
    HRESULT ret = DI_OK;
    int len;

    TRACE("(%p) %p -> %p(%d) x%d, 0x%08x\n",
          This, dod, entries, entries ? *entries : 0, dodsize, flags);

    if (!This->acquired)
        return DIERR_NOTACQUIRED;
    if (!This->queue_len)
        return DIERR_NOTBUFFERED;
    if (dodsize < sizeof(DIDEVICEOBJECTDATA_DX3))
        return DIERR_INVALIDPARAM;

    IDirectInputDevice2_Poll(iface);
    EnterCriticalSection(&This->crit);

    len = This->queue_head - This->queue_tail;
    if (len < 0) len += This->queue_len;

    if ((*entries != INFINITE) && (len > *entries)) len = *entries;

    if (dod)
    {
        int i;
        for (i = 0; i < len; i++)
        {
            int n = (This->queue_tail + i) % This->queue_len;
            memcpy((char *)dod + dodsize * i, This->data_queue + n, dodsize);
        }
    }
    *entries = len;

    if (This->overflow)
        ret = DI_BUFFEROVERFLOW;

    if (!(flags & DIGDD_PEEK))
    {
        /* Advance reading position */
        This->queue_tail = (This->queue_tail + len) % This->queue_len;
        This->overflow = FALSE;
    }

    LeaveCriticalSection(&This->crit);

    TRACE("Returning %d events queued\n", *entries);
    return ret;
}

HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel(
	LPDIRECTINPUTDEVICE8A iface,
	HWND hwndOwner,
	DWORD dwFlags)
{
    FIXME("(this=%p,%p,0x%08x): stub!\n",
	  iface, hwndOwner, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Initialize(
	LPDIRECTINPUTDEVICE8A iface,
	HINSTANCE hinst,
	DWORD dwVersion,
	REFGUID rguid)
{
    FIXME("(this=%p,%p,%d,%s): stub!\n",
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
    FIXME("(this=%p,%p,%p,0x%08x): stub!\n",
	  iface, lpCallback, lpvRef, dwFlags);
    
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_EnumEffects(
	LPDIRECTINPUTDEVICE8W iface,
	LPDIENUMEFFECTSCALLBACKW lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
    FIXME("(this=%p,%p,%p,0x%08x): stub!\n",
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
    TRACE("(%p) 0x%08x:\n", iface, dwFlags);
    return DI_NOEFFECT;
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
    FIXME("(this=%p,%p,%p,0x%08x): stub!\n",
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
    IDirectInputDevice2AImpl *This = (IDirectInputDevice2AImpl *)iface;

    if (!This->acquired) return DIERR_NOTACQUIRED;
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
    FIXME("(this=%p,0x%08x,%p,%p,0x%08x): stub!\n",
	  iface, cbObjectData, rgdod, pdwInOut, dwFlags);
    
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7AImpl_EnumEffectsInFile(LPDIRECTINPUTDEVICE8A iface,
							  LPCSTR lpszFileName,
							  LPDIENUMEFFECTSINFILECALLBACK pec,
							  LPVOID pvRef,
							  DWORD dwFlags)
{
    FIXME("(%p)->(%s,%p,%p,%08x): stub !\n", iface, lpszFileName, pec, pvRef, dwFlags);
    
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7WImpl_EnumEffectsInFile(LPDIRECTINPUTDEVICE8W iface,
							  LPCWSTR lpszFileName,
							  LPDIENUMEFFECTSINFILECALLBACK pec,
							  LPVOID pvRef,
							  DWORD dwFlags)
{
    FIXME("(%p)->(%s,%p,%p,%08x): stub !\n", iface, debugstr_w(lpszFileName), pec, pvRef, dwFlags);
    
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7AImpl_WriteEffectToFile(LPDIRECTINPUTDEVICE8A iface,
							  LPCSTR lpszFileName,
							  DWORD dwEntries,
							  LPDIFILEEFFECT rgDiFileEft,
							  DWORD dwFlags)
{
    FIXME("(%p)->(%s,%08x,%p,%08x): stub !\n", iface, lpszFileName, dwEntries, rgDiFileEft, dwFlags);
    
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7WImpl_WriteEffectToFile(LPDIRECTINPUTDEVICE8W iface,
							  LPCWSTR lpszFileName,
							  DWORD dwEntries,
							  LPDIFILEEFFECT rgDiFileEft,
							  DWORD dwFlags)
{
    FIXME("(%p)->(%s,%08x,%p,%08x): stub !\n", iface, debugstr_w(lpszFileName), dwEntries, rgDiFileEft, dwFlags);
    
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8AImpl_BuildActionMap(LPDIRECTINPUTDEVICE8A iface,
						       LPDIACTIONFORMATA lpdiaf,
						       LPCSTR lpszUserName,
						       DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08x): stub !\n", iface, lpdiaf, lpszUserName, dwFlags);
#define X(x) if (dwFlags & x) FIXME("\tdwFlags =|"#x"\n");
	X(DIDBAM_DEFAULT)
	X(DIDBAM_PRESERVE)
	X(DIDBAM_INITIALIZE)
	X(DIDBAM_HWDEFAULTS)
#undef X
    _dump_diactionformatA(lpdiaf);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8WImpl_BuildActionMap(LPDIRECTINPUTDEVICE8W iface,
						       LPDIACTIONFORMATW lpdiaf,
						       LPCWSTR lpszUserName,
						       DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08x): stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);
#define X(x) if (dwFlags & x) FIXME("\tdwFlags =|"#x"\n");
	X(DIDBAM_DEFAULT)
	X(DIDBAM_PRESERVE)
	X(DIDBAM_INITIALIZE)
	X(DIDBAM_HWDEFAULTS)
#undef X
  
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8AImpl_SetActionMap(LPDIRECTINPUTDEVICE8A iface,
						     LPDIACTIONFORMATA lpdiaf,
						     LPCSTR lpszUserName,
						     DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08x): stub !\n", iface, lpdiaf, lpszUserName, dwFlags);
    
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8WImpl_SetActionMap(LPDIRECTINPUTDEVICE8W iface,
						     LPDIACTIONFORMATW lpdiaf,
						     LPCWSTR lpszUserName,
						     DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08x): stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);
    
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
