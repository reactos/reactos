/*
 * Wine X11drv display settings functions
 *
 * Copyright 2003 Alexander James Pasadyn
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
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "x11drv.h"

#include "windef.h"
#include "winreg.h"
#include "wingdi.h"
#include "ddrawi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11settings);

/*
 * The DDHALMODEINFO type is used to hold all the mode information.  
 * This is done because the array of DDHALMODEINFO structures must be 
 * created for use by DirectDraw anyway.
 */
static LPDDHALMODEINFO dd_modes = NULL;
static unsigned int dd_mode_count = 0;
static unsigned int dd_max_modes = 0;
/* All Windows drivers seen so far either support 32 bit depths, or 24 bit depths, but never both. So if we have
 * a 32 bit framebuffer, report 32 bit bpps, otherwise 24 bit ones.
 */
static const unsigned int depths_24[]  = {8, 16, 24};
static const unsigned int depths_32[]  = {8, 16, 32};

/* pointers to functions that actually do the hard stuff */
static int (*pGetCurrentMode)(void);
static LONG (*pSetCurrentMode)(int mode);
static const char *handler_name;

/*
 * Set the handlers for resolution changing functions
 * and initialize the master list of modes
 */
LPDDHALMODEINFO X11DRV_Settings_SetHandlers(const char *name,
                                 int (*pNewGCM)(void),
                                 LONG (*pNewSCM)(int),
                                 unsigned int nmodes,
                                 int reserve_depths)
{
    handler_name = name;
    pGetCurrentMode = pNewGCM;
    pSetCurrentMode = pNewSCM;
    TRACE("Resolution settings now handled by: %s\n", name);
    if (reserve_depths)
        /* leave room for other depths */
        dd_max_modes = (3+1)*(nmodes);
    else 
        dd_max_modes = nmodes;

    if (dd_modes) 
    {
        TRACE("Destroying old display modes array\n");
        HeapFree(GetProcessHeap(), 0, dd_modes);
    }
    dd_modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDHALMODEINFO) * dd_max_modes);
    dd_mode_count = 0;
    TRACE("Initialized new display modes array\n");
    return dd_modes;
}

/* Add one mode to the master list */
void X11DRV_Settings_AddOneMode(unsigned int width, unsigned int height, unsigned int bpp, unsigned int freq)
{
    LPDDHALMODEINFO info = &(dd_modes[dd_mode_count]);
    DWORD dwBpp = screen_bpp;
    if (dd_mode_count >= dd_max_modes)
    {
        ERR("Maximum modes (%d) exceeded\n", dd_max_modes);
        return;
    }
    if (bpp == 0) bpp = dwBpp;
    info->dwWidth        = width;
    info->dwHeight       = height;
    info->wRefreshRate   = freq;
    info->lPitch         = 0;
    info->dwBPP          = bpp;
    info->wFlags         = 0;
    info->dwRBitMask     = 0;
    info->dwGBitMask     = 0;
    info->dwBBitMask     = 0;
    info->dwAlphaBitMask = 0;
    TRACE("initialized mode %d: %dx%dx%d @%d Hz (%s)\n", 
          dd_mode_count, width, height, bpp, freq, handler_name);
    dd_mode_count++;
}

/* copy all the current modes using the other color depths */
void X11DRV_Settings_AddDepthModes(void)
{
    int i, j;
    int existing_modes = dd_mode_count;
    DWORD dwBpp = screen_bpp;
    const DWORD *depths = screen_bpp == 32 ? depths_32 : depths_24;

    for (j=0; j<3; j++)
    {
        if (depths[j] != dwBpp)
        {
            for (i=0; i < existing_modes; i++)
            {
                X11DRV_Settings_AddOneMode(dd_modes[i].dwWidth, dd_modes[i].dwHeight, 
                                           depths[j], dd_modes[i].wRefreshRate);
            }
        }
    }
}

/* return the number of modes that are initialized */
unsigned int X11DRV_Settings_GetModeCount(void)
{
    return dd_mode_count;
}

/***********************************************************************
 * Default handlers if resolution switching is not enabled
 *
 */
static int X11DRV_nores_GetCurrentMode(void)
{
    return 0;
}

static LONG X11DRV_nores_SetCurrentMode(int mode)
{
    TRACE("Ignoring mode change request\n");
    return DISP_CHANGE_FAILED;
}

/* default handler only gets the current X desktop resolution */
void X11DRV_Settings_Init(void)
{
    X11DRV_Settings_SetHandlers("NoRes", 
                                X11DRV_nores_GetCurrentMode, 
                                X11DRV_nores_SetCurrentMode, 
                                1, 0);
    X11DRV_Settings_AddOneMode(screen_width, screen_height, 0, 60);
}

static BOOL get_display_device_reg_key(char *key, unsigned len)
{
    static const char display_device_guid_prop[] = "__wine_display_device_guid";
    static const char video_path[] = "System\\CurrentControlSet\\Control\\Video\\{";
    static const char display0[] = "}\\0000";
    ATOM guid_atom;

    assert(len >= sizeof(video_path) + sizeof(display0) + 40);

    guid_atom = HandleToULong(GetPropA(GetDesktopWindow(), display_device_guid_prop));
    if (!guid_atom) return FALSE;

    memcpy(key, video_path, sizeof(video_path));

    if (!GlobalGetAtomNameA(guid_atom, key + strlen(key), 40))
        return FALSE;

    strcat(key, display0);

    TRACE("display device key %s\n", wine_dbgstr_a(key));
    return TRUE;
}

static BOOL read_registry_settings(DEVMODEW *dm)
{
    char wine_x11_reg_key[128];
    HKEY hkey;
    DWORD type, size;
    BOOL ret = TRUE;

    dm->dmFields = 0;

    if (!get_display_device_reg_key(wine_x11_reg_key, sizeof(wine_x11_reg_key)))
        return FALSE;

    if (RegOpenKeyExA(HKEY_CURRENT_CONFIG, wine_x11_reg_key, 0, KEY_READ, &hkey))
        return FALSE;

#define query_value(name, data) \
    size = sizeof(DWORD); \
    if (RegQueryValueExA(hkey, name, 0, &type, (LPBYTE)(data), &size) || \
        type != REG_DWORD || size != sizeof(DWORD)) \
        ret = FALSE

    query_value("DefaultSettings.BitsPerPel", &dm->dmBitsPerPel);
    dm->dmFields |= DM_BITSPERPEL;
    query_value("DefaultSettings.XResolution", &dm->dmPelsWidth);
    dm->dmFields |= DM_PELSWIDTH;
    query_value("DefaultSettings.YResolution", &dm->dmPelsHeight);
    dm->dmFields |= DM_PELSHEIGHT;
    query_value("DefaultSettings.VRefresh", &dm->dmDisplayFrequency);
    dm->dmFields |= DM_DISPLAYFREQUENCY;
    query_value("DefaultSettings.Flags", &dm->u2.dmDisplayFlags);
    dm->dmFields |= DM_DISPLAYFLAGS;
    query_value("DefaultSettings.XPanning", &dm->u1.s2.dmPosition.x);
    query_value("DefaultSettings.YPanning", &dm->u1.s2.dmPosition.y);
    query_value("DefaultSettings.Orientation", &dm->u1.s2.dmDisplayOrientation);
    query_value("DefaultSettings.FixedOutput", &dm->u1.s2.dmDisplayFixedOutput);

#undef query_value

    RegCloseKey(hkey);
    return ret;
}

static BOOL write_registry_settings(const DEVMODEW *dm)
{
    char wine_x11_reg_key[128];
    HKEY hkey;
    BOOL ret = TRUE;

    if (!get_display_device_reg_key(wine_x11_reg_key, sizeof(wine_x11_reg_key)))
        return FALSE;

    if (RegCreateKeyExA(HKEY_CURRENT_CONFIG, wine_x11_reg_key, 0, NULL,
                        REG_OPTION_VOLATILE, KEY_WRITE, NULL, &hkey, NULL))
        return FALSE;

#define set_value(name, data) \
    if (RegSetValueExA(hkey, name, 0, REG_DWORD, (LPBYTE)(data), sizeof(DWORD))) \
        ret = FALSE

    set_value("DefaultSettings.BitsPerPel", &dm->dmBitsPerPel);
    set_value("DefaultSettings.XResolution", &dm->dmPelsWidth);
    set_value("DefaultSettings.YResolution", &dm->dmPelsHeight);
    set_value("DefaultSettings.VRefresh", &dm->dmDisplayFrequency);
    set_value("DefaultSettings.Flags", &dm->u2.dmDisplayFlags);
    set_value("DefaultSettings.XPanning", &dm->u1.s2.dmPosition.x);
    set_value("DefaultSettings.YPanning", &dm->u1.s2.dmPosition.y);
    set_value("DefaultSettings.Orientation", &dm->u1.s2.dmDisplayOrientation);
    set_value("DefaultSettings.FixedOutput", &dm->u1.s2.dmDisplayFixedOutput);

#undef set_value

    RegCloseKey(hkey);
    return ret;
}

/***********************************************************************
 *		EnumDisplaySettingsEx  (X11DRV.@)
 *
 */
BOOL CDECL X11DRV_EnumDisplaySettingsEx( LPCWSTR name, DWORD n, LPDEVMODEW devmode, DWORD flags)
{
    static const WCHAR dev_name[CCHDEVICENAME] =
        { 'W','i','n','e',' ','X','1','1',' ','d','r','i','v','e','r',0 };

    devmode->dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod);
    devmode->dmSpecVersion = DM_SPECVERSION;
    devmode->dmDriverVersion = DM_SPECVERSION;
    memcpy(devmode->dmDeviceName, dev_name, sizeof(dev_name));
    devmode->dmDriverExtra = 0;
    devmode->u2.dmDisplayFlags = 0;
    devmode->dmDisplayFrequency = 0;
    devmode->u1.s2.dmPosition.x = 0;
    devmode->u1.s2.dmPosition.y = 0;
    devmode->u1.s2.dmDisplayOrientation = 0;
    devmode->u1.s2.dmDisplayFixedOutput = 0;

    if (n == ENUM_CURRENT_SETTINGS)
    {
        TRACE("mode %d (current) -- getting current mode (%s)\n", n, handler_name);
        n = pGetCurrentMode();
    }
    if (n == ENUM_REGISTRY_SETTINGS)
    {
        TRACE("mode %d (registry) -- getting default mode (%s)\n", n, handler_name);
        return read_registry_settings(devmode);
    }
    if (n < dd_mode_count)
    {
        devmode->dmPelsWidth = dd_modes[n].dwWidth;
        devmode->dmPelsHeight = dd_modes[n].dwHeight;
        devmode->dmBitsPerPel = dd_modes[n].dwBPP;
        devmode->dmDisplayFrequency = dd_modes[n].wRefreshRate;
        devmode->dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL |
                            DM_DISPLAYFLAGS;
        if (devmode->dmDisplayFrequency)
        {
            devmode->dmFields |= DM_DISPLAYFREQUENCY;
            TRACE("mode %d -- %dx%dx%dbpp @%d Hz (%s)\n", n,
                  devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel,
                  devmode->dmDisplayFrequency, handler_name);
        }
        else
        {
            TRACE("mode %d -- %dx%dx%dbpp (%s)\n", n,
                  devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel, 
                  handler_name);
        }
        return TRUE;
    }
    TRACE("mode %d -- not present (%s)\n", n, handler_name);
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}

#define _X_FIELD(prefix, bits) if ((fields) & prefix##_##bits) {p+=sprintf(p, "%s%s", first ? "" : ",", #bits); first=FALSE;}
static const char * _CDS_flags(DWORD fields)
{
    BOOL first = TRUE;
    char buf[128];
    char *p = buf;
    _X_FIELD(CDS,UPDATEREGISTRY);_X_FIELD(CDS,TEST);_X_FIELD(CDS,FULLSCREEN);
    _X_FIELD(CDS,GLOBAL);_X_FIELD(CDS,SET_PRIMARY);_X_FIELD(CDS,RESET);
    _X_FIELD(CDS,SETRECT);_X_FIELD(CDS,NORESET);
    *p = 0;
    return wine_dbg_sprintf("%s", buf);
}
static const char * _DM_fields(DWORD fields)
{
    BOOL first = TRUE;
    char buf[128];
    char *p = buf;
    _X_FIELD(DM,BITSPERPEL);_X_FIELD(DM,PELSWIDTH);_X_FIELD(DM,PELSHEIGHT);
    _X_FIELD(DM,DISPLAYFLAGS);_X_FIELD(DM,DISPLAYFREQUENCY);_X_FIELD(DM,POSITION);
    *p = 0;
    return wine_dbg_sprintf("%s", buf);
}
#undef _X_FIELD

/***********************************************************************
 *		ChangeDisplaySettingsEx  (X11DRV.@)
 *
 */
LONG CDECL X11DRV_ChangeDisplaySettingsEx( LPCWSTR devname, LPDEVMODEW devmode,
                                           HWND hwnd, DWORD flags, LPVOID lpvoid )
{
    DWORD i, dwBpp = 0;
    DEVMODEW dm;
    BOOL def_mode = TRUE;

    TRACE("(%s,%p,%p,0x%08x,%p)\n",debugstr_w(devname),devmode,hwnd,flags,lpvoid);
    TRACE("flags=%s\n",_CDS_flags(flags));
    if (devmode)
    {
        /* this is the minimal dmSize that XP accepts */
        if (devmode->dmSize < FIELD_OFFSET(DEVMODEW, dmFields))
            return DISP_CHANGE_FAILED;

        TRACE("DM_fields=%s\n",_DM_fields(devmode->dmFields));
        TRACE("width=%d height=%d bpp=%d freq=%d (%s)\n",
              devmode->dmPelsWidth,devmode->dmPelsHeight,
              devmode->dmBitsPerPel,devmode->dmDisplayFrequency, handler_name);

        dwBpp = devmode->dmBitsPerPel;
        if (devmode->dmFields & DM_BITSPERPEL) def_mode &= !dwBpp;
        if (devmode->dmFields & DM_PELSWIDTH)  def_mode &= !devmode->dmPelsWidth;
        if (devmode->dmFields & DM_PELSHEIGHT) def_mode &= !devmode->dmPelsHeight;
        if (devmode->dmFields & DM_DISPLAYFREQUENCY) def_mode &= !devmode->dmDisplayFrequency;
    }

    if (def_mode || !dwBpp)
    {
        if (!X11DRV_EnumDisplaySettingsEx(devname, ENUM_REGISTRY_SETTINGS, &dm, 0))
        {
            ERR("Default mode not found!\n");
            return DISP_CHANGE_BADMODE;
        }
        if (def_mode)
        {
            TRACE("Return to original display mode (%s)\n", handler_name);
            devmode = &dm;
        }
        dwBpp = dm.dmBitsPerPel;
    }

    if ((devmode->dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != (DM_PELSWIDTH | DM_PELSHEIGHT))
    {
        WARN("devmode doesn't specify the resolution: %04x\n", devmode->dmFields);
        return DISP_CHANGE_BADMODE;
    }

    for (i = 0; i < dd_mode_count; i++)
    {
        if (devmode->dmFields & DM_BITSPERPEL)
        {
            if (dwBpp != dd_modes[i].dwBPP)
                continue;
        }
        if (devmode->dmFields & DM_PELSWIDTH)
        {
            if (devmode->dmPelsWidth != dd_modes[i].dwWidth)
                continue;
        }
        if (devmode->dmFields & DM_PELSHEIGHT)
        {
            if (devmode->dmPelsHeight != dd_modes[i].dwHeight)
                continue;
        }
        if ((devmode->dmFields & DM_DISPLAYFREQUENCY) && (dd_modes[i].wRefreshRate != 0) &&
            devmode->dmDisplayFrequency != 0)
        {
            if (devmode->dmDisplayFrequency != dd_modes[i].wRefreshRate)
                continue;
        }
        /* we have a valid mode */
        TRACE("Requested display settings match mode %d (%s)\n", i, handler_name);

        if (flags & CDS_UPDATEREGISTRY)
            write_registry_settings(devmode);

        if (!(flags & (CDS_TEST | CDS_NORESET)))
            return pSetCurrentMode(i);

        return DISP_CHANGE_SUCCESSFUL;
    }

    /* no valid modes found */
    ERR("No matching mode found %ux%ux%u @%u! (%s)\n",
        devmode->dmPelsWidth, devmode->dmPelsHeight,
        devmode->dmBitsPerPel, devmode->dmDisplayFrequency, handler_name);
    return DISP_CHANGE_BADMODE;
}




/***********************************************************************
 * DirectDraw HAL interface
 *
 */
static DWORD PASCAL X11DRV_Settings_SetMode(LPDDHAL_SETMODEDATA data)
{
    TRACE("Mode %d requested by DDHAL (%s)\n", data->dwModeIndex, handler_name);
    switch (pSetCurrentMode(data->dwModeIndex))
    {
    case DISP_CHANGE_SUCCESSFUL:
        X11DRV_DDHAL_SwitchMode(data->dwModeIndex, NULL, NULL);
        data->ddRVal = DD_OK;
        break;
    case DISP_CHANGE_BADMODE:
        data->ddRVal = DDERR_WRONGMODE;
        break;
    default:
        data->ddRVal = DDERR_UNSUPPORTEDMODE;
    }
    return DDHAL_DRIVER_HANDLED;
}
int X11DRV_Settings_CreateDriver(LPDDHALINFO info)
{
    if (!dd_mode_count) return 0; /* no settings defined */

    TRACE("Setting up display settings for DDRAW (%s)\n", handler_name);
    info->dwNumModes = dd_mode_count;
    info->lpModeInfo = dd_modes;
    X11DRV_DDHAL_SwitchMode(pGetCurrentMode(), NULL, NULL);
    info->lpDDCallbacks->SetMode = X11DRV_Settings_SetMode;
    return TRUE;
}
