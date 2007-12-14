/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9.c
 * PURPOSE:         d3d9.dll implementation
 * PROGRAMERS:      Magnus Olsen <greatlrd (at) reactos (dot) org>
 *                  Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include <windows.h>
#include "d3d9_private.h"
#include "d3d9_helpers.h"

#include <debug.h>

DLLAPI
HRESULT Direct3DShaderValidatorCreate9(void)
{
    UNIMPLEMENTED
    return 0;
}

DLLAPI
HRESULT PSGPError(void)
{
    UNIMPLEMENTED
    return 0;
}

DLLAPI
HRESULT PSGPSampleTexture(void)
{
    UNIMPLEMENTED
    return 0;
}

DLLAPI
HRESULT  DebugSetLevel(void)
{
    UNIMPLEMENTED
    return 0;
}

DLLAPI
HRESULT DebugSetMute(DWORD dw1)
{
    UNIMPLEMENTED
    return 0;
}

DLLAPI
IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion)
{
    HINSTANCE hDebugDll;
    DWORD LoadDebugDll;
    DWORD LoadDebugDllSize;
    LPDIRECT3D9 D3D9Obj = 0;
    LPDIRECT3DCREATE9 DebugDirect3DCreate9 = 0;

    UNIMPLEMENTED

    LoadDebugDllSize = sizeof(LoadDebugDll);
    if (ReadRegistryValue(REG_DWORD, "LoadDebugRuntime", (LPBYTE)&LoadDebugDll, &LoadDebugDllSize))
    {
        if (0 != LoadDebugDll)
        {
            hDebugDll = LoadLibrary("d3d9d.dll");

            if (0 != hDebugDll)
            {
                DebugDirect3DCreate9 = (LPDIRECT3DCREATE9)GetProcAddress(hDebugDll, "Direct3DCreate9");

                D3D9Obj = DebugDirect3DCreate9(SDKVersion);
            }
        }
    }

    return D3D9Obj;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }

    return TRUE;
}
