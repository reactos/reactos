/*
 * Direct3D 9
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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
 *
 */

#include "config.h"
#include "initguid.h"
#include "d3d9_private.h"

typedef IWineD3D* (WINAPI *fnWineDirect3DCreate)(UINT, UINT, IUnknown *);

static HMODULE hWineD3D = (HMODULE) -1;
static fnWineDirect3DCreate pWineDirect3DCreate;

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static int D3DPERF_event_level = 0;

#ifdef WINE_NATIVEWIN32
static HMODULE hD3D9;

HRESULT (WINAPI *pD3D9GetSWInfo)(void);
BOOL (WINAPI *pCheckFullscreen)();
HRESULT (WINAPI *pDebugSetLevel)(void);
HRESULT (WINAPI *pDebugSetMute)(void);
IDirect3D9* (WINAPI *pDirect3DCreate9)(UINT SDKVersion);
HRESULT (WINAPI *pDirect3DCreate9Ex)(UINT SDKVersion, VOID** ppD3D);

BOOL
IsPassthrough()
{
    if (!hD3D9)
        return FALSE;
    if (hD3D9 != (HMODULE) -1)
        return TRUE;
    hD3D9 = LoadLibraryA("d3d9.sav");
    if (hD3D9) {
#define getproc(x) *(FARPROC *)&p##x = GetProcAddress(hD3D9, #x)
        getproc(D3D9GetSWInfo);
        getproc(CheckFullscreen);
        getproc(DebugSetLevel);
        getproc(DebugSetMute);
        getproc(Direct3DCreate9);
        getproc(Direct3DCreate9Ex);
#undef getproc
    }
    return hD3D9 != NULL;
}
#endif

HRESULT WINAPI D3D9GetSWInfo(void) {
#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pD3D9GetSWInfo();
#endif
    FIXME("(void): stub\n");
    return 0;
}

BOOL WINAPI
CheckFullscreen()
{
#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pCheckFullscreen();
#endif
    return FALSE;
}

HRESULT WINAPI DebugSetLevel(void) {
#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pDebugSetLevel();
#endif
    FIXME("(void): stub\n");
    return 0;
}

HRESULT WINAPI DebugSetMute(void) {
#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pDebugSetMute();
#endif
    FIXME("(void): stub\n");
    return 0;
}

IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion) {
    IDirect3D9Impl* object;

#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pDirect3DCreate9(SDKVersion);
#endif
    if (hWineD3D == (HMODULE) -1)
    {
        hWineD3D = LoadLibraryA("wined3d");
        if (hWineD3D)
            pWineDirect3DCreate = (fnWineDirect3DCreate) GetProcAddress(hWineD3D, "WineDirect3DCreate");
    }

    if (!hWineD3D)
    {
        ERR("Couldn't load WineD3D - OpenGL libs not present?\n");
        return NULL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3D9Impl));

    object->lpVtbl = &Direct3D9_Vtbl;
    object->ref = 1;
    object->WineD3D = pWineDirect3DCreate(SDKVersion, 9, (IUnknown *)object);

    TRACE("SDKVersion = %x, Created Direct3D object @ %p, WineObj @ %p\n", SDKVersion, object, object->WineD3D);

    return (IDirect3D9*) object;
}

HRESULT WINAPI Direct3DCreate9Ex(UINT SDKVersion, VOID** ppD3D) {
#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pDirect3DCreate9Ex(SDKVersion, ppD3D);
#endif
    TRACE("SDKVersion = %x\n", SDKVersion);
    /* When D3D9L features are not supported (no WDDM driver installed) */

    if (ppD3D)
        *ppD3D = NULL;

    return D3DERR_NOTAVAILABLE;
}

HRESULT WINAPI Direct3DShaderValidatorCreate9(void) {
    FIXME("(void): stub\n");

    return 0;
}

HRESULT WINAPI PSGPError(void) {
    FIXME("(void): stub\n");

    return 0;
}

HRESULT WINAPI PSGPSampleTexture(void) {
    FIXME("(void): stub\n");

    return 0;
}

/***********************************************************************
 * get_config_key
 *
 * Reads a config key from the registry. Taken from WineD3D
 *
 ***********************************************************************/
static inline DWORD get_config_key(HKEY defkey, HKEY appkey, const char* name, char* buffer, DWORD size)
{
    if (0 != appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    if (0 != defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    return ERROR_FILE_NOT_FOUND;
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("fdwReason=%d\n", fdwReason);
    if (fdwReason == DLL_PROCESS_ATTACH) {
        char buffer[MAX_PATH+64];
        DWORD size = sizeof(buffer);
        HKEY hkey = 0;
        HKEY appkey = 0;
        DWORD len;

       DisableThreadLibraryCalls(hInstDLL);

       /* @@ Wine registry key: HKCU\Software\Wine\Direct3D */
       if ( RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Direct3D", &hkey )
           && RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Direct3D", &hkey ) )
           hkey = 0;

       len = GetModuleFileNameA( 0, buffer, MAX_PATH );
       if (len && len < MAX_PATH)
       {
            char *p, *appname = buffer;
            if ((p = strrchr( appname, '/' ))) appname = p + 1;
            if ((p = strrchr( appname, '\\' ))) appname = p + 1;
            TRACE("appname = [%s]\n", appname);
            memmove(
                buffer + strlen( "Software\\Wine\\AppDefaults\\" ),
                appname, strlen( appname ) + 1);
            memcpy(
                buffer, "Software\\Wine\\AppDefaults\\",
                strlen( "Software\\Wine\\AppDefaults\\" ));
            strcat( buffer, "\\Direct3D" );

            /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Direct3D */
            if (RegOpenKeyA( HKEY_CURRENT_USER, buffer, &appkey )
                && RegOpenKeyA( HKEY_LOCAL_MACHINE, buffer, &appkey ))
                appkey = 0;
       }

       if ( 0 != hkey || 0 != appkey )
       {
#ifdef WINE_NATIVEWIN32
            if ( !get_config_key( hkey, appkey, "Passthrough", buffer, size) )
            {
                if (!strcmp(buffer,"true") || !strcmp(buffer,"yes"))
                {
                    TRACE("Passthrough mode\n");
                    hD3D9 = (HMODULE) -1;
                }
            }
#endif
        }
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (hWineD3D && hWineD3D != (HMODULE) -1)
            FreeLibrary(hWineD3D);
#ifdef WINE_NATIVEWIN32
        if (hD3D9 && hD3D9 != (HMODULE) -1)
            FreeLibrary(hD3D9);
#endif
    }

    return TRUE;
}

/***********************************************************************
 *              D3DPERF_BeginEvent (D3D9.@)
 */
int WINAPI D3DPERF_BeginEvent(D3DCOLOR color, LPCWSTR name) {
    FIXME("(color %#x, name %s) : stub\n", color, debugstr_w(name));

    return D3DPERF_event_level++;
}

/***********************************************************************
 *              D3DPERF_EndEvent (D3D9.@)
 */
int WINAPI D3DPERF_EndEvent(void) {
    FIXME("(void) : stub\n");

    return --D3DPERF_event_level;
}

/***********************************************************************
 *              D3DPERF_GetStatus (D3D9.@)
 */
DWORD WINAPI D3DPERF_GetStatus(void) {
    FIXME("(void) : stub\n");

    return 0;
}

/***********************************************************************
 *              D3DPERF_SetOptions (D3D9.@)
 *
 */
void WINAPI D3DPERF_SetOptions(DWORD options)
{
  FIXME("(%#x) : stub\n", options);
}

/***********************************************************************
 *              D3DPERF_QueryRepeatFrame (D3D9.@)
 */
BOOL WINAPI D3DPERF_QueryRepeatFrame(void) {
    FIXME("(void) : stub\n");

    return FALSE;
}

/***********************************************************************
 *              D3DPERF_SetMarker (D3D9.@)
 */
void WINAPI D3DPERF_SetMarker(D3DCOLOR color, LPCWSTR name) {
    FIXME("(color %#x, name %s) : stub\n", color, debugstr_w(name));
}

/***********************************************************************
 *              D3DPERF_SetRegion (D3D9.@)
 */
void WINAPI D3DPERF_SetRegion(D3DCOLOR color, LPCWSTR name) {
    FIXME("(color %#x, name %s) : stub\n", color, debugstr_w(name));
}
