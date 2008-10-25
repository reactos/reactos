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

static CRITICAL_SECTION_DEBUG d3d8_cs_debug =
{
    0, 0, &d3d8_cs,
    { &d3d8_cs_debug.ProcessLocksList,
      &d3d8_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": d3d8_cs") }
};
CRITICAL_SECTION d3d8_cs = { &d3d8_cs_debug, -1, 0, 0, 0, 0 };

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

HRESULT WINAPI D3D8GetSWInfo(void) {
    FIXME("(void): stub\n");
    return 0;
}

void WINAPI DebugSetMute(void) {
    /* nothing to do */
}

IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion) {
    IDirect3D8Impl* object;
    TRACE("SDKVersion = %x\n", SDKVersion);

    EnterCriticalSection(&d3d8_cs);
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3D8Impl));

    object->lpVtbl = &Direct3D8_Vtbl;
    object->ref = 1;
    object->WineD3D = WineDirect3DCreate(SDKVersion, 8, (IUnknown *)object);

    TRACE("Created Direct3D object @ %p, WineObj @ %p\n", object, object->WineD3D);
    LeaveCriticalSection(&d3d8_cs);

    if (!object->WineD3D)
    {
        HeapFree( GetProcessHeap(), 0, object );
        object = NULL;
    }
    return (IDirect3D8*) object;
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("fdwReason=%d\n", fdwReason);
    if (fdwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hInstDLL);

    return TRUE;
}

/***********************************************************************
 *		ValidateVertexShader (D3D8.@)
 *
 * I've seen reserved1 and reserved2 always passed as 0's
 * bool seems always passed as 0 or 1, but other values work as well.... 
 * toto       result?
 */
HRESULT WINAPI ValidateVertexShader(DWORD* vertexshader, DWORD* reserved1, DWORD* reserved2, BOOL bool, DWORD* toto)
{ 
  HRESULT ret;
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
HRESULT WINAPI ValidatePixelShader(DWORD* pixelshader, DWORD* reserved1, BOOL bool, DWORD* toto)
{
  HRESULT ret;
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
