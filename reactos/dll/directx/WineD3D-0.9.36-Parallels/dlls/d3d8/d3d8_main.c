/*
 * Direct3D 8
 *
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
#include "d3d8_private.h"
#include "wine/debug.h"

typedef IWineD3D* (WINAPI *fnWineDirect3DCreate)(UINT, UINT, IUnknown *);

static HMODULE hWineD3D = (HMODULE) -1;
static fnWineDirect3DCreate pWineDirect3DCreate;

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

#ifdef WINE_NATIVEWIN32
static HMODULE hD3D8;

BOOL (WINAPI *pCheckFullscreen)();
HRESULT (WINAPI *pDebugSetMute)(void);
IDirect3D8* (WINAPI *pDirect3DCreate8)(UINT SDKVersion);
HRESULT (WINAPI *pValidateVertexShader)(
    DWORD* vertexshader, DWORD* reserved1, DWORD* reserved2, int bool, DWORD* toto);
HRESULT (WINAPI *pValidatePixelShader)(
    DWORD* pixelshader, DWORD* reserved1, int bool, DWORD* toto);

BOOL
IsPassthrough()
{
    if (!hD3D8)
        return FALSE;
    if (hD3D8 != (HMODULE) -1)
        return TRUE;
    hD3D8 = LoadLibraryA("d3d8.sav");
    if (hD3D8) {
#define getproc(x) *(FARPROC *)&p##x = GetProcAddress(hD3D8, #x)
        getproc(CheckFullscreen);
        getproc(DebugSetMute);
        getproc(Direct3DCreate8);
        getproc(ValidateVertexShader);
        getproc(ValidatePixelShader);
#undef getproc
    }
    return hD3D8 != NULL;
}
#endif

HRESULT WINAPI D3D8GetSWInfo(void) {
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

void WINAPI DebugSetMute(void) {
#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        pDebugSetMute();
#endif
    /* nothing to do */
}

IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion) {
    IDirect3D8Impl* object;

#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pDirect3DCreate8(SDKVersion);
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

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3D8Impl));
    object->lpVtbl = &Direct3D8_Vtbl;
    object->ref = 1;
    object->WineD3D = pWineDirect3DCreate(SDKVersion, 8, (IUnknown *)object);

    TRACE("SDKVersion = %x, Created Direct3D object @ %p, WineObj @ %p\n", SDKVersion, object, object->WineD3D);

    return (IDirect3D8*) object;
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

#if defined(WINE_NATIVEWIN32) && !defined(NDEBUG)
        debug_init();
#endif
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
                    hD3D8 = (HMODULE) -1;
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
        if (hD3D8 && hD3D8 != (HMODULE) -1)
            FreeLibrary(hD3D8);
#endif
    }

    return TRUE;
}

/***********************************************************************
 *		ValidateVertexShader (D3D8.@)
 *
 * I've seen reserved1 and reserved2 always passed as 0's
 * bool seems always passed as 0 or 1, but other values work as well.... 
 * toto       result?
 */
HRESULT WINAPI ValidateVertexShader(DWORD* vertexshader, DWORD* reserved1, DWORD* reserved2, int bool, DWORD* toto)
{ 
  HRESULT ret;

#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pValidateVertexShader(vertexshader, reserved1, reserved2, bool, toto);
#endif
  FIXME("(%p %p %p %d %p): stub\n", vertexshader, reserved1, reserved2, bool, toto);

  if (!vertexshader)
      return E_FAIL;

  if (reserved1 || reserved2)
      return E_FAIL;

  switch(*vertexshader) {
        case 0xFFFE0101:
        case 0xFFFE0100: 
            ret=S_OK;
            break;
        default:
            ERR("vertexshader version mismatch\n");
            ret=E_FAIL;
        }

  return ret;
}

/***********************************************************************
 *		ValidatePixelShader (D3D8.@)
 *
 * PARAMS
 * toto       result?
 */
HRESULT WINAPI ValidatePixelShader(DWORD* pixelshader, DWORD* reserved1, int bool, DWORD* toto)
{
  HRESULT ret;

#ifdef WINE_NATIVEWIN32
    if (IsPassthrough())
        return pValidatePixelShader(pixelshader, reserved1, bool, toto);
#endif
  FIXME("(%p %p %d %p): stub\n", pixelshader, reserved1, bool, toto);
  
  if (!pixelshader)
      return E_FAIL;

  if (reserved1)
      return E_FAIL;   

  switch(*pixelshader) {
        case 0xFFFF0100:
        case 0xFFFF0101:
        case 0xFFFF0102:
        case 0xFFFF0103:
        case 0xFFFF0104:
            ret=S_OK;
            break;
        default:
            ERR("pixelshader version mismatch\n");
            ret=E_FAIL;
        }
  return ret;
}
