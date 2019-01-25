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

#define WM_WINE_NOTIFY_ACTIVITY WM_USER

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static inline IDirectInputDeviceImpl *impl_from_IDirectInputDevice8A(IDirectInputDevice8A *iface)
{
    return CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8A_iface);
}
static inline IDirectInputDeviceImpl *impl_from_IDirectInputDevice8W(IDirectInputDevice8W *iface)
{
    return CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface);
}

static inline IDirectInputDevice8A *IDirectInputDevice8A_from_impl(IDirectInputDeviceImpl *This)
{
    return &This->IDirectInputDevice8A_iface;
}
static inline IDirectInputDevice8W *IDirectInputDevice8W_from_impl(IDirectInputDeviceImpl *This)
{
    return &This->IDirectInputDevice8W_iface;
}

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
	for (i = 0; i < ARRAY_SIZE(flags); i++)
	    if (flags[i].mask & dwFlags)
		TRACE("%s ",flags[i].name);
	TRACE("\n");
    }
}

static void _dump_ObjectDataFormat_flags(DWORD dwFlags) {
    unsigned int   i;
    static const struct {
        DWORD       mask;
        const char  *name;
    } flags[] = {
#define FE(x) { x, #x}
        FE(DIDOI_FFACTUATOR),
        FE(DIDOI_FFEFFECTTRIGGER),
        FE(DIDOI_POLLED),
        FE(DIDOI_GUIDISUSAGE)
#undef FE
    };

    if (!dwFlags) return;

    TRACE("Flags:");

    /* First the flags */
    for (i = 0; i < ARRAY_SIZE(flags); i++) {
        if (flags[i].mask & dwFlags)
        TRACE(" %s",flags[i].name);
    }

    /* Now specific values */
#define FE(x) case x: TRACE(" "#x); break
    switch (dwFlags & DIDOI_ASPECTMASK) {
        FE(DIDOI_ASPECTACCEL);
        FE(DIDOI_ASPECTFORCE);
        FE(DIDOI_ASPECTPOSITION);
        FE(DIDOI_ASPECTVELOCITY);
    }
#undef FE

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
	    for (i = 0; i < ARRAY_SIZE(flags); i++) {
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
    TRACE("    - enumerating : %s ('%s') - %2d - 0x%08x - %s - 0x%x\n",
        debugstr_guid(&ddoi->guidType), _dump_dinput_GUID(&ddoi->guidType), ddoi->dwOfs, ddoi->dwType, ddoi->tszName, ddoi->dwFlags);
}

void _dump_OBJECTINSTANCEW(const DIDEVICEOBJECTINSTANCEW *ddoi) {
    TRACE("    - enumerating : %s ('%s'), - %2d - 0x%08x - %s - 0x%x\n",
        debugstr_guid(&ddoi->guidType), _dump_dinput_GUID(&ddoi->guidType), ddoi->dwOfs, ddoi->dwType, debugstr_w(ddoi->tszName), ddoi->dwFlags);
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
    for (i = 0; i < ARRAY_SIZE(guids); i++) {
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
	TRACE("        "); _dump_ObjectDataFormat_flags(df->rgodf[i].dwFlags); TRACE("\n");
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

/* dataformat_to_odf_by_type
 *  Find the Nth object of the selected type in the DataFormat
 */
LPDIOBJECTDATAFORMAT dataformat_to_odf_by_type(LPCDIDATAFORMAT df, int n, DWORD type)
{
    int i, nfound = 0;

    for (i=0; i < df->dwNumObjs; i++)
    {
        LPDIOBJECTDATAFORMAT odf = dataformat_to_odf(df, i);

        if (odf->dwType & type)
        {
            if (n == nfound)
                return odf;

            nfound++;
        }
    }

    return NULL;
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
		 (DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType) == 0x00FF) || /* This is mentioned in no DX docs, but it works fine - tested on WinXP */
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
                TRACE("       * dwType: 0x%08x\n", asked_format->rgodf[j].dwType);
		TRACE("         "); _dump_EnumObjects_flags(asked_format->rgodf[j].dwType); TRACE("\n");
                TRACE("       * dwFlags: 0x%08x\n", asked_format->rgodf[j].dwFlags);
		TRACE("         "); _dump_ObjectDataFormat_flags(asked_format->rgodf[j].dwFlags); TRACE("\n");
		
		TRACE("   - Wine  (%d) :\n", i);
		TRACE("       * GUID: %s ('%s')\n",
                      debugstr_guid(format->wine_df->rgodf[i].pguid),
                      _dump_dinput_GUID(format->wine_df->rgodf[i].pguid));
                TRACE("       * Offset: %3d\n", format->wine_df->rgodf[i].dwOfs);
                TRACE("       * dwType: 0x%08x\n", format->wine_df->rgodf[i].dwType);
                TRACE("         "); _dump_EnumObjects_flags(format->wine_df->rgodf[i].dwType); TRACE("\n");
                TRACE("       * dwFlags: 0x%08x\n", format->wine_df->rgodf[i].dwFlags);
                TRACE("         "); _dump_ObjectDataFormat_flags(format->wine_df->rgodf[i].dwFlags); TRACE("\n");
		
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
            TRACE("       * dwType: 0x%08x\n", asked_format->rgodf[j].dwType);
	    TRACE("         "); _dump_EnumObjects_flags(asked_format->rgodf[j].dwType); TRACE("\n");
            TRACE("       * dwFlags: 0x%08x\n", asked_format->rgodf[j].dwFlags);
	    TRACE("         "); _dump_ObjectDataFormat_flags(asked_format->rgodf[j].dwFlags); TRACE("\n");
	    
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

/* find an object by its offset in a data format */
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

static int id_to_offset(const DataFormat *df, int id)
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

static DWORD semantic_to_obj_id(IDirectInputDeviceImpl* This, DWORD dwSemantic)
{
    DWORD type = (0x0000ff00 & dwSemantic) >> 8;
    DWORD offset = 0x000000ff & dwSemantic;
    DWORD obj_instance = 0;
    BOOL found = FALSE;
    int i;

    for (i = 0; i < This->data_format.wine_df->dwNumObjs; i++)
    {
        LPDIOBJECTDATAFORMAT odf = dataformat_to_odf(This->data_format.wine_df, i);

        if (odf->dwOfs == offset)
        {
            obj_instance = DIDFT_GETINSTANCE(odf->dwType);
            found = TRUE;
            break;
        }
    }

    if (!found) return 0;

    if (type & DIDFT_AXIS)   type = DIDFT_RELAXIS;
    if (type & DIDFT_BUTTON) type = DIDFT_PSHBUTTON;

    return type | (0x0000ff00 & (obj_instance << 8));
}

/*
 * get_mapping_key
 * Retrieves an open registry key to save the mapping, parametrized for an username,
 * specific device and specific action mapping guid.
 */
static HKEY get_mapping_key(const WCHAR *device, const WCHAR *username, const WCHAR *guid)
{
    static const WCHAR subkey[] = {
        'S','o','f','t','w','a','r','e','\\',
        'W','i','n','e','\\',
        'D','i','r','e','c','t','I','n','p','u','t','\\',
        'M','a','p','p','i','n','g','s','\\','%','s','\\','%','s','\\','%','s','\0'};
    HKEY hkey;
    WCHAR *keyname;

    keyname = HeapAlloc(GetProcessHeap(), 0,
        sizeof(WCHAR) * (lstrlenW(subkey) + strlenW(username) + strlenW(device) + strlenW(guid)));
    sprintfW(keyname, subkey, username, device, guid);

    /* The key used is HKCU\Software\Wine\DirectInput\Mappings\[username]\[device]\[mapping_guid] */
    if (RegCreateKeyW(HKEY_CURRENT_USER, keyname, &hkey))
        hkey = 0;

    HeapFree(GetProcessHeap(), 0, keyname);

    return hkey;
}

static HRESULT save_mapping_settings(IDirectInputDevice8W *iface, LPDIACTIONFORMATW lpdiaf, LPCWSTR lpszUsername)
{
    WCHAR *guid_str = NULL;
    DIDEVICEINSTANCEW didev;
    HKEY hkey;
    int i;

    didev.dwSize = sizeof(didev);
    IDirectInputDevice8_GetDeviceInfo(iface, &didev);

    if (StringFromCLSID(&lpdiaf->guidActionMap, &guid_str) != S_OK)
        return DI_SETTINGSNOTSAVED;

    hkey = get_mapping_key(didev.tszInstanceName, lpszUsername, guid_str);

    if (!hkey)
    {
        CoTaskMemFree(guid_str);
        return DI_SETTINGSNOTSAVED;
    }

    /* Write each of the actions mapped for this device.
       Format is "dwSemantic"="dwObjID" and key is of type REG_DWORD
    */
    for (i = 0; i < lpdiaf->dwNumActions; i++)
    {
        static const WCHAR format[] = {'%','x','\0'};
        WCHAR label[9];

        if (IsEqualGUID(&didev.guidInstance, &lpdiaf->rgoAction[i].guidInstance) &&
            lpdiaf->rgoAction[i].dwHow != DIAH_UNMAPPED)
        {
             sprintfW(label, format, lpdiaf->rgoAction[i].dwSemantic);
             RegSetValueExW(hkey, label, 0, REG_DWORD, (const BYTE*) &lpdiaf->rgoAction[i].dwObjID, sizeof(DWORD));
        }
    }

    RegCloseKey(hkey);
    CoTaskMemFree(guid_str);

    return DI_OK;
}

static BOOL load_mapping_settings(IDirectInputDeviceImpl *This, LPDIACTIONFORMATW lpdiaf, const WCHAR *username)
{
    HKEY hkey;
    WCHAR *guid_str;
    DIDEVICEINSTANCEW didev;
    int i, mapped = 0;

    didev.dwSize = sizeof(didev);
    IDirectInputDevice8_GetDeviceInfo(&This->IDirectInputDevice8W_iface, &didev);

    if (StringFromCLSID(&lpdiaf->guidActionMap, &guid_str) != S_OK)
        return FALSE;

    hkey = get_mapping_key(didev.tszInstanceName, username, guid_str);

    if (!hkey)
    {
        CoTaskMemFree(guid_str);
        return FALSE;
    }

    /* Try to read each action in the DIACTIONFORMAT from registry */
    for (i = 0; i < lpdiaf->dwNumActions; i++)
    {
        static const WCHAR format[] = {'%','x','\0'};
        DWORD id, size = sizeof(DWORD);
        WCHAR label[9];

        sprintfW(label, format, lpdiaf->rgoAction[i].dwSemantic);

        if (!RegQueryValueExW(hkey, label, 0, NULL, (LPBYTE) &id, &size))
        {
            lpdiaf->rgoAction[i].dwObjID = id;
            lpdiaf->rgoAction[i].guidInstance = didev.guidInstance;
            lpdiaf->rgoAction[i].dwHow = DIAH_DEFAULT;
            mapped += 1;
        }
    }

    RegCloseKey(hkey);
    CoTaskMemFree(guid_str);

    return mapped > 0;
}

HRESULT _build_action_map(LPDIRECTINPUTDEVICE8W iface, LPDIACTIONFORMATW lpdiaf, LPCWSTR lpszUserName, DWORD dwFlags, DWORD devMask, LPCDIDATAFORMAT df)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    WCHAR username[MAX_PATH];
    DWORD username_size = MAX_PATH;
    int i;
    BOOL load_success = FALSE, has_actions = FALSE;

    /* Unless asked the contrary by these flags, try to load a previous mapping */
    if (!(dwFlags & DIDBAM_HWDEFAULTS))
    {
        /* Retrieve logged user name if necessary */
        if (lpszUserName == NULL)
            GetUserNameW(username, &username_size);
        else
            lstrcpynW(username, lpszUserName, MAX_PATH);

        load_success = load_mapping_settings(This, lpdiaf, username);
    }

    if (load_success) return DI_OK;

    for (i=0; i < lpdiaf->dwNumActions; i++)
    {
        /* Don't touch a user configured action */
        if (lpdiaf->rgoAction[i].dwHow == DIAH_USERCONFIG) continue;

        if ((lpdiaf->rgoAction[i].dwSemantic & devMask) == devMask)
        {
            DWORD obj_id = semantic_to_obj_id(This, lpdiaf->rgoAction[i].dwSemantic);
            DWORD type = DIDFT_GETTYPE(obj_id);
            DWORD inst = DIDFT_GETINSTANCE(obj_id);

            LPDIOBJECTDATAFORMAT odf;

            if (type == DIDFT_PSHBUTTON) type = DIDFT_BUTTON;
            if (type == DIDFT_RELAXIS) type = DIDFT_AXIS;

            /* Make sure the object exists */
            odf = dataformat_to_odf_by_type(df, inst, type);

            if (odf != NULL)
            {
                lpdiaf->rgoAction[i].dwObjID = obj_id;
                lpdiaf->rgoAction[i].guidInstance = This->guid;
                lpdiaf->rgoAction[i].dwHow = DIAH_DEFAULT;
                has_actions = TRUE;
            }
        }
        else if (!(dwFlags & DIDBAM_PRESERVE))
        {
            /* We must clear action data belonging to other devices */
            memset(&lpdiaf->rgoAction[i].guidInstance, 0, sizeof(GUID));
            lpdiaf->rgoAction[i].dwHow = DIAH_UNMAPPED;
        }
    }

    if (!has_actions) return DI_NOEFFECT;

    return  IDirectInputDevice8WImpl_BuildActionMap(iface, lpdiaf, lpszUserName, dwFlags);
}

HRESULT _set_action_map(LPDIRECTINPUTDEVICE8W iface, LPDIACTIONFORMATW lpdiaf, LPCWSTR lpszUserName, DWORD dwFlags, LPCDIDATAFORMAT df)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    DIDATAFORMAT data_format;
    DIOBJECTDATAFORMAT *obj_df = NULL;
    DIPROPDWORD dp;
    DIPROPRANGE dpr;
    DIPROPSTRING dps;
    WCHAR username[MAX_PATH];
    DWORD username_size = MAX_PATH;
    int i, action = 0, num_actions = 0;
    unsigned int offset = 0;

    if (This->acquired) return DIERR_ACQUIRED;

    data_format.dwSize = sizeof(data_format);
    data_format.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
    data_format.dwFlags = DIDF_RELAXIS;
    data_format.dwDataSize = lpdiaf->dwDataSize;

    /* Count the actions */
    for (i=0; i < lpdiaf->dwNumActions; i++)
        if (IsEqualGUID(&This->guid, &lpdiaf->rgoAction[i].guidInstance))
            num_actions++;

    if (num_actions == 0) return DI_NOEFFECT;

    This->num_actions = num_actions;

    /* Construct the dataformat and actionmap */
    obj_df = HeapAlloc(GetProcessHeap(), 0, sizeof(DIOBJECTDATAFORMAT)*num_actions);
    data_format.rgodf = (LPDIOBJECTDATAFORMAT)obj_df;
    data_format.dwNumObjs = num_actions;

    HeapFree(GetProcessHeap(), 0, This->action_map);
    This->action_map = HeapAlloc(GetProcessHeap(), 0, sizeof(ActionMap)*num_actions);

    for (i = 0; i < lpdiaf->dwNumActions; i++)
    {
        if (IsEqualGUID(&This->guid, &lpdiaf->rgoAction[i].guidInstance))
        {
            DWORD inst = DIDFT_GETINSTANCE(lpdiaf->rgoAction[i].dwObjID);
            DWORD type = DIDFT_GETTYPE(lpdiaf->rgoAction[i].dwObjID);
            LPDIOBJECTDATAFORMAT obj;

            if (type == DIDFT_PSHBUTTON) type = DIDFT_BUTTON;
            if (type == DIDFT_RELAXIS) type = DIDFT_AXIS;

            obj = dataformat_to_odf_by_type(df, inst, type);

            memcpy(&obj_df[action], obj, df->dwObjSize);

            This->action_map[action].uAppData = lpdiaf->rgoAction[i].uAppData;
            This->action_map[action].offset = offset;
            obj_df[action].dwOfs = offset;
            offset += (type & DIDFT_BUTTON) ? 1 : 4;

            action++;
        }
    }

    IDirectInputDevice8_SetDataFormat(iface, &data_format);

    HeapFree(GetProcessHeap(), 0, obj_df);

    /* Set the device properties according to the action format */
    dpr.diph.dwSize = sizeof(DIPROPRANGE);
    dpr.lMin = lpdiaf->lAxisMin;
    dpr.lMax = lpdiaf->lAxisMax;
    dpr.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dpr.diph.dwHow = DIPH_DEVICE;
    IDirectInputDevice8_SetProperty(iface, DIPROP_RANGE, &dpr.diph);

    if (lpdiaf->dwBufferSize > 0)
    {
        dp.diph.dwSize = sizeof(DIPROPDWORD);
        dp.dwData = lpdiaf->dwBufferSize;
        dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dp.diph.dwHow = DIPH_DEVICE;
        IDirectInputDevice8_SetProperty(iface, DIPROP_BUFFERSIZE, &dp.diph);
    }

    /* Retrieve logged user name if necessary */
    if (lpszUserName == NULL)
        GetUserNameW(username, &username_size);
    else
        lstrcpynW(username, lpszUserName, MAX_PATH);

    dps.diph.dwSize = sizeof(dps);
    dps.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dps.diph.dwObj = 0;
    dps.diph.dwHow = DIPH_DEVICE;
    if (dwFlags & DIDSAM_NOUSER)
        dps.wsz[0] = '\0';
    else
        lstrcpynW(dps.wsz, username, ARRAY_SIZE(dps.wsz));
    IDirectInputDevice8_SetProperty(iface, DIPROP_USERNAME, &dps.diph);

    /* Save the settings to disk */
    save_mapping_settings(iface, lpdiaf, username);

    return DI_OK;
}

/******************************************************************************
 *	queue_event - add new event to the ring queue
 */

void queue_event(LPDIRECTINPUTDEVICE8A iface, int inst_id, DWORD data, DWORD time, DWORD seq)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    int next_pos, ofs = id_to_offset(&This->data_format, inst_id);

    /* Event is being set regardless of the queue state */
    if (This->hEvent) SetEvent(This->hEvent);

    PostMessageW(GetDesktopWindow(), WM_WINE_NOTIFY_ACTIVITY, 0, 0);

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

    /* Set uAppData by means of action mapping */
    if (This->num_actions > 0)
    {
        int i;
        for (i=0; i < This->num_actions; i++)
        {
            if (This->action_map[i].offset == ofs)
            {
                TRACE("Offset %d mapped to uAppData %lu\n", ofs, This->action_map[i].uAppData);
                This->data_queue[This->queue_head].uAppData = This->action_map[i].uAppData;
                break;
            }
        }
    }

    This->queue_head = next_pos;
    /* Send event if asked */
}

/******************************************************************************
 *	Acquire
 */

HRESULT WINAPI IDirectInputDevice2WImpl_Acquire(LPDIRECTINPUTDEVICE8W iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;

    TRACE("(%p)\n", This);

    if (!This->data_format.user_df) return DIERR_INVALIDPARAM;
    if (This->dwCoopLevel & DISCL_FOREGROUND && This->win != GetForegroundWindow())
        return DIERR_OTHERAPPHASPRIO;

    EnterCriticalSection(&This->crit);
    res = This->acquired ? S_FALSE : DI_OK;
    This->acquired = 1;
    LeaveCriticalSection(&This->crit);
    if (res == DI_OK)
        check_dinput_hooks(iface, TRUE);

    return res;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_Acquire(IDirectInputDevice8W_from_impl(This));
}


/******************************************************************************
 *	Unacquire
 */

HRESULT WINAPI IDirectInputDevice2WImpl_Unacquire(LPDIRECTINPUTDEVICE8W iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;

    TRACE("(%p)\n", This);

    EnterCriticalSection(&This->crit);
    res = !This->acquired ? DI_NOEFFECT : DI_OK;
    This->acquired = 0;
    LeaveCriticalSection(&This->crit);
    if (res == DI_OK)
        check_dinput_hooks(iface, FALSE);

    return res;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_Unacquire(IDirectInputDevice8W_from_impl(This));
}

/******************************************************************************
 *	IDirectInputDeviceA
 */

HRESULT WINAPI IDirectInputDevice2WImpl_SetDataFormat(LPDIRECTINPUTDEVICE8W iface, LPCDIDATAFORMAT df)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
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

HRESULT WINAPI IDirectInputDevice2AImpl_SetDataFormat(LPDIRECTINPUTDEVICE8A iface, LPCDIDATAFORMAT df)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_SetDataFormat(IDirectInputDevice8W_from_impl(This), df);
}

/******************************************************************************
  *     SetCooperativeLevel
  *
  *  Set cooperative level and the source window for the events.
  */
HRESULT WINAPI IDirectInputDevice2WImpl_SetCooperativeLevel(LPDIRECTINPUTDEVICE8W iface, HWND hwnd, DWORD dwflags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p) %p,0x%08x\n", This, hwnd, dwflags);
    _dump_cooperativelevel_DI(dwflags);

    if ((dwflags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == 0 ||
        (dwflags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE) ||
        (dwflags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == 0 ||
        (dwflags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == (DISCL_FOREGROUND | DISCL_BACKGROUND))
        return DIERR_INVALIDPARAM;

    if (hwnd && GetWindowLongW(hwnd, GWL_STYLE) & WS_CHILD) return E_HANDLE;

    if (!hwnd && dwflags == (DISCL_NONEXCLUSIVE | DISCL_BACKGROUND))
        hwnd = GetDesktopWindow();

    if (!IsWindow(hwnd)) return E_HANDLE;

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

HRESULT WINAPI IDirectInputDevice2AImpl_SetCooperativeLevel(LPDIRECTINPUTDEVICE8A iface, HWND hwnd, DWORD dwflags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_SetCooperativeLevel(IDirectInputDevice8W_from_impl(This), hwnd, dwflags);
}

/******************************************************************************
  *     SetEventNotification : specifies event to be sent on state change
  */
HRESULT WINAPI IDirectInputDevice2WImpl_SetEventNotification(LPDIRECTINPUTDEVICE8W iface, HANDLE event)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p) %p\n", This, event);

    EnterCriticalSection(&This->crit);
    This->hEvent = event;
    LeaveCriticalSection(&This->crit);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification(LPDIRECTINPUTDEVICE8A iface, HANDLE event)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_SetEventNotification(IDirectInputDevice8W_from_impl(This), event);
}


ULONG WINAPI IDirectInputDevice2WImpl_Release(LPDIRECTINPUTDEVICE8W iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    ULONG ref = InterlockedDecrement(&(This->ref));

    TRACE("(%p) releasing from %d\n", This, ref + 1);

    if (ref) return ref;

    IDirectInputDevice_Unacquire(iface);
    /* Reset the FF state, free all effects, etc */
    IDirectInputDevice8_SendForceFeedbackCommand(iface, DISFFC_RESET);

    HeapFree(GetProcessHeap(), 0, This->data_queue);

    /* Free data format */
    HeapFree(GetProcessHeap(), 0, This->data_format.wine_df->rgodf);
    HeapFree(GetProcessHeap(), 0, This->data_format.wine_df);
    release_DataFormat(&This->data_format);

    /* Free action mapping */
    HeapFree(GetProcessHeap(), 0, This->action_map);

    EnterCriticalSection( &This->dinput->crit );
    list_remove( &This->entry );
    LeaveCriticalSection( &This->dinput->crit );

    IDirectInput_Release(&This->dinput->IDirectInput7A_iface);
    This->crit.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->crit);

    HeapFree(GetProcessHeap(), 0, This);

    return DI_OK;
}

ULONG WINAPI IDirectInputDevice2AImpl_Release(LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_Release(IDirectInputDevice8W_from_impl(This));
}

HRESULT WINAPI IDirectInputDevice2WImpl_QueryInterface(LPDIRECTINPUTDEVICE8W iface, REFIID riid, LPVOID *ppobj)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p this=%p,%s,%p)\n", iface, This, debugstr_guid(riid), ppobj);
    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IDirectInputDeviceA,  riid) ||
        IsEqualGUID(&IID_IDirectInputDevice2A, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice7A, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice8A, riid))
    {
        IDirectInputDevice2_AddRef(iface);
        *ppobj = IDirectInputDevice8A_from_impl(This);
        return DI_OK;
    }
    if (IsEqualGUID(&IID_IDirectInputDeviceW,  riid) ||
        IsEqualGUID(&IID_IDirectInputDevice2W, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice7W, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice8W, riid))
    {
        IDirectInputDevice2_AddRef(iface);
        *ppobj = IDirectInputDevice8W_from_impl(This);
        return DI_OK;
    }

    WARN("Unsupported interface!\n");
    return E_FAIL;
}

HRESULT WINAPI IDirectInputDevice2AImpl_QueryInterface(LPDIRECTINPUTDEVICE8A iface, REFIID riid, LPVOID *ppobj)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_QueryInterface(IDirectInputDevice8W_from_impl(This), riid, ppobj);
}

ULONG WINAPI IDirectInputDevice2WImpl_AddRef(LPDIRECTINPUTDEVICE8W iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IDirectInputDevice2AImpl_AddRef(LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_AddRef(IDirectInputDevice8W_from_impl(This));
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects(LPDIRECTINPUTDEVICE8A iface,
        LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback, LPVOID lpvRef, DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
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

        if (dwFlags != DIDFT_ALL && !(dwFlags & DIDFT_GETTYPE(odf->dwType))) continue;
        if (IDirectInputDevice_GetObjectInfo(iface, &ddoi, odf->dwType, DIPH_BYID) != DI_OK)
            continue;

	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) break;
    }

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_EnumObjects(LPDIRECTINPUTDEVICE8W iface,
        LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback, LPVOID lpvRef, DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
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

        if (dwFlags != DIDFT_ALL && !(dwFlags & DIDFT_GETTYPE(odf->dwType))) continue;
        if (IDirectInputDevice_GetObjectInfo(iface, &ddoi, odf->dwType, DIPH_BYID) != DI_OK)
            continue;

	if (lpCallback(&ddoi, lpvRef) != DIENUM_CONTINUE) break;
    }

    return DI_OK;
}

/******************************************************************************
 *	GetProperty
 */

HRESULT WINAPI IDirectInputDevice2WImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p) %s,%p\n", iface, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (!IS_DIPROP(rguid)) return DI_OK;

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
        case (DWORD_PTR) DIPROP_USERNAME:
        {
            LPDIPROPSTRING ps = (LPDIPROPSTRING)pdiph;
            struct DevicePlayer *device_player;

            if (pdiph->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;

            LIST_FOR_EACH_ENTRY(device_player, &This->dinput->device_players,
                struct DevicePlayer, entry)
            {
                if (IsEqualGUID(&device_player->instance_guid, &This->guid))
                {
                    if (*device_player->username)
                    {
                        lstrcpynW(ps->wsz, device_player->username, ARRAY_SIZE(ps->wsz));
                        return DI_OK;
                    }
                    else break;
                }
            }
            return S_FALSE;
        }
        case (DWORD_PTR) DIPROP_VIDPID:
            FIXME("DIPROP_VIDPID not implemented\n");
            return DIERR_UNSUPPORTED;
        default:
            FIXME("Unknown property %s\n", debugstr_guid(rguid));
            return DIERR_INVALIDPARAM;
    }

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetProperty(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_GetProperty(IDirectInputDevice8W_from_impl(This), rguid, pdiph);
}

/******************************************************************************
 *	SetProperty
 */

HRESULT WINAPI IDirectInputDevice2WImpl_SetProperty(
        LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPCDIPROPHEADER pdiph)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p) %s,%p\n", iface, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (!IS_DIPROP(rguid)) return DI_OK;

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
        case (DWORD_PTR) DIPROP_USERNAME:
        {
            LPCDIPROPSTRING ps = (LPCDIPROPSTRING)pdiph;
            struct DevicePlayer *device_player;
            BOOL found = FALSE;

            if (pdiph->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;

            LIST_FOR_EACH_ENTRY(device_player, &This->dinput->device_players,
                struct DevicePlayer, entry)
            {
                if (IsEqualGUID(&device_player->instance_guid, &This->guid))
                {
                    found = TRUE;
                    break;
                }
            }
            if (!found && (device_player =
                    HeapAlloc(GetProcessHeap(), 0, sizeof(struct DevicePlayer))))
            {
                list_add_tail(&This->dinput->device_players, &device_player->entry);
                device_player->instance_guid = This->guid;
            }
            if (device_player)
                lstrcpynW(device_player->username, ps->wsz, ARRAY_SIZE(device_player->username));
            break;
        }
        default:
            WARN("Unknown property %s\n", debugstr_guid(rguid));
            return DIERR_UNSUPPORTED;
    }

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetProperty(
        LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPCDIPROPHEADER pdiph)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_SetProperty(IDirectInputDevice8W_from_impl(This), rguid, pdiph);
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE8A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    DIDEVICEOBJECTINSTANCEW didoiW;
    HRESULT res;

    if (!pdidoi ||
        (pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCEA) &&
         pdidoi->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3A)))
        return DIERR_INVALIDPARAM;

    didoiW.dwSize = sizeof(didoiW);
    res = IDirectInputDevice2WImpl_GetObjectInfo(IDirectInputDevice8W_from_impl(This), &didoiW, dwObj, dwHow);
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
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
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

HRESULT WINAPI IDirectInputDevice2WImpl_GetDeviceData(LPDIRECTINPUTDEVICE8W iface, DWORD dodsize,
                                                      LPDIDEVICEOBJECTDATA dod, LPDWORD entries, DWORD flags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT ret = DI_OK;
    int len;

    TRACE("(%p) %p -> %p(%d) x%d, 0x%08x\n",
          This, dod, entries, entries ? *entries : 0, dodsize, flags);

    if (This->dinput->dwVersion == 0x0800 || dodsize == sizeof(DIDEVICEOBJECTDATA_DX3))
    {
        if (!This->queue_len) return DIERR_NOTBUFFERED;
        if (!This->acquired) return DIERR_NOTACQUIRED;
    }

    if (!This->queue_len)
        return DI_OK;
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

    if (This->overflow && This->dinput->dwVersion == 0x0800)
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

HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceData(LPDIRECTINPUTDEVICE8A iface, DWORD dodsize,
                                                      LPDIDEVICEOBJECTDATA dod, LPDWORD entries, DWORD flags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_GetDeviceData(IDirectInputDevice8W_from_impl(This), dodsize, dod, entries, flags);
}

HRESULT WINAPI IDirectInputDevice2WImpl_RunControlPanel(LPDIRECTINPUTDEVICE8W iface, HWND hwndOwner, DWORD dwFlags)
{
    FIXME("(this=%p,%p,0x%08x): stub!\n", iface, hwndOwner, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel(LPDIRECTINPUTDEVICE8A iface, HWND hwndOwner, DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_RunControlPanel(IDirectInputDevice8W_from_impl(This), hwndOwner, dwFlags);
}

HRESULT WINAPI IDirectInputDevice2WImpl_Initialize(LPDIRECTINPUTDEVICE8W iface, HINSTANCE hinst, DWORD dwVersion,
                                                   REFGUID rguid)
{
    FIXME("(this=%p,%p,%d,%s): stub!\n", iface, hinst, dwVersion, debugstr_guid(rguid));
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Initialize(LPDIRECTINPUTDEVICE8A iface, HINSTANCE hinst, DWORD dwVersion,
                                                   REFGUID rguid)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_Initialize(IDirectInputDevice8W_from_impl(This), hinst, dwVersion, rguid);
}

/******************************************************************************
 *	IDirectInputDevice2A
 */

HRESULT WINAPI IDirectInputDevice2WImpl_CreateEffect(LPDIRECTINPUTDEVICE8W iface, REFGUID rguid, LPCDIEFFECT lpeff,
                                                     LPDIRECTINPUTEFFECT *ppdef, LPUNKNOWN pUnkOuter)
{
    FIXME("(this=%p,%s,%p,%p,%p): stub!\n", iface, debugstr_guid(rguid), lpeff, ppdef, pUnkOuter);

    FIXME("not available in the generic implementation\n");
    *ppdef = NULL;
    return DIERR_UNSUPPORTED;
}

HRESULT WINAPI IDirectInputDevice2AImpl_CreateEffect(LPDIRECTINPUTDEVICE8A iface, REFGUID rguid, LPCDIEFFECT lpeff,
                                                     LPDIRECTINPUTEFFECT *ppdef, LPUNKNOWN pUnkOuter)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_CreateEffect(IDirectInputDevice8W_from_impl(This), rguid, lpeff, ppdef, pUnkOuter);
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

HRESULT WINAPI IDirectInputDevice2WImpl_GetForceFeedbackState(LPDIRECTINPUTDEVICE8W iface, LPDWORD pdwOut)
{
    FIXME("(this=%p,%p): stub!\n", iface, pdwOut);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetForceFeedbackState(LPDIRECTINPUTDEVICE8A iface, LPDWORD pdwOut)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_GetForceFeedbackState(IDirectInputDevice8W_from_impl(This), pdwOut);
}

HRESULT WINAPI IDirectInputDevice2WImpl_SendForceFeedbackCommand(LPDIRECTINPUTDEVICE8W iface, DWORD dwFlags)
{
    TRACE("(%p) 0x%08x:\n", iface, dwFlags);
    return DI_NOEFFECT;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SendForceFeedbackCommand(LPDIRECTINPUTDEVICE8A iface, DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_SendForceFeedbackCommand(IDirectInputDevice8W_from_impl(This), dwFlags);
}

HRESULT WINAPI IDirectInputDevice2WImpl_EnumCreatedEffectObjects(LPDIRECTINPUTDEVICE8W iface,
        LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback, LPVOID lpvRef, DWORD dwFlags)
{
    FIXME("(this=%p,%p,%p,0x%08x): stub!\n", iface, lpCallback, lpvRef, dwFlags);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects(LPDIRECTINPUTDEVICE8A iface,
        LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback, LPVOID lpvRef, DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_EnumCreatedEffectObjects(IDirectInputDevice8W_from_impl(This), lpCallback, lpvRef, dwFlags);
}

HRESULT WINAPI IDirectInputDevice2WImpl_Escape(LPDIRECTINPUTDEVICE8W iface, LPDIEFFESCAPE lpDIEEsc)
{
    FIXME("(this=%p,%p): stub!\n", iface, lpDIEEsc);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Escape(LPDIRECTINPUTDEVICE8A iface, LPDIEFFESCAPE lpDIEEsc)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_Escape(IDirectInputDevice8W_from_impl(This), lpDIEEsc);
}

HRESULT WINAPI IDirectInputDevice2WImpl_Poll(LPDIRECTINPUTDEVICE8W iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);

    if (!This->acquired) return DIERR_NOTACQUIRED;

    check_dinput_events();
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_Poll(LPDIRECTINPUTDEVICE8A iface)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_Poll(IDirectInputDevice8W_from_impl(This));
}

HRESULT WINAPI IDirectInputDevice2WImpl_SendDeviceData(LPDIRECTINPUTDEVICE8W iface, DWORD cbObjectData,
                                                       LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut,
                                                       DWORD dwFlags)
{
    FIXME("(this=%p,0x%08x,%p,%p,0x%08x): stub!\n", iface, cbObjectData, rgdod, pdwInOut, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2AImpl_SendDeviceData(LPDIRECTINPUTDEVICE8A iface, DWORD cbObjectData,
                                                       LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut,
                                                       DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8A(iface);
    return IDirectInputDevice2WImpl_SendDeviceData(IDirectInputDevice8W_from_impl(This), cbObjectData, rgdod,
                                                   pdwInOut, dwFlags);
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

HRESULT WINAPI IDirectInputDevice8WImpl_BuildActionMap(LPDIRECTINPUTDEVICE8W iface,
						       LPDIACTIONFORMATW lpdiaf,
						       LPCWSTR lpszUserName,
						       DWORD dwFlags)
{
    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", iface, lpdiaf, debugstr_w(lpszUserName), dwFlags);
#define X(x) if (dwFlags & x) FIXME("\tdwFlags =|"#x"\n");
	X(DIDBAM_DEFAULT)
	X(DIDBAM_PRESERVE)
	X(DIDBAM_INITIALIZE)
	X(DIDBAM_HWDEFAULTS)
#undef X
  
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
