/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9.c
 * PURPOSE:         d3d9.dll implementation
 * PROGRAMERS:      Magnus Olsen <greatlrd (at) reactos (dot) org>
 *                  Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include <d3d9.h>
#include "d3d9_helpers.h"
#include "d3d9_create.h"
#include <debug.h>

#define DEBUG_MESSAGE_BUFFER_SIZE   512

typedef IDirect3D9* (WINAPI *LPDIRECT3DCREATE9)(UINT);

static LPCSTR D3dError_WrongSdkVersion =
    "D3D ERROR: D3D header version mismatch.\n"
    "The application was compiled against and will only work with "
    "D3D_SDK_VERSION (%d), but the currently installed runtime is "
    "version (%d).\n"
    "Recompile the application against the appropriate SDK for the installed runtime.\n"
    "\n";

HRESULT WINAPI Direct3DShaderValidatorCreate9(void)
{
    UNIMPLEMENTED
    return 0;
}

HRESULT WINAPI PSGPError(void)
{
    UNIMPLEMENTED
    return 0;
}

HRESULT WINAPI PSGPSampleTexture(void)
{
    UNIMPLEMENTED
    return 0;
}

HRESULT WINAPI DebugSetLevel(void)
{
    UNIMPLEMENTED
    return 0;
}

HRESULT WINAPI DebugSetMute(DWORD dw1)
{
    UNIMPLEMENTED
    return 0;
}

IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion)
{
    HINSTANCE hDebugDll;
    DWORD LoadDebugDll;
    DWORD LoadDebugDllSize;
    LPDIRECT3D9 D3D9Obj = 0;
    LPDIRECT3DCREATE9 DebugDirect3DCreate9 = 0;
    CHAR DebugMessageBuffer[DEBUG_MESSAGE_BUFFER_SIZE];
    UINT NoDebugSDKVersion = SDKVersion & ~DX_D3D9_DEBUG;

    LoadDebugDllSize = sizeof(LoadDebugDll);
    if (ReadRegistryValue(REG_DWORD, "LoadDebugRuntime", (LPBYTE)&LoadDebugDll, &LoadDebugDllSize))
    {
        if (0 != LoadDebugDll)
        {
            hDebugDll = LoadLibraryA("d3d9d.dll");

            if (0 != hDebugDll)
            {
                DebugDirect3DCreate9 = (LPDIRECT3DCREATE9)GetProcAddress(hDebugDll, "Direct3DCreate9");

                return DebugDirect3DCreate9(SDKVersion);
            }
        }
    }

    if (NoDebugSDKVersion != D3D_SDK_VERSION && NoDebugSDKVersion != D3D9b_SDK_VERSION)
    {
        if (SDKVersion & DX_D3D9_DEBUG)
        {
            HRESULT hResult;
            hResult = SafeFormatString(DebugMessageBuffer, DEBUG_MESSAGE_BUFFER_SIZE, D3dError_WrongSdkVersion, NoDebugSDKVersion, D3D_SDK_VERSION);
            if (SUCCEEDED(hResult))
                OutputDebugStringA(DebugMessageBuffer);
        }

        return NULL;
    }

    CreateD3D9(&D3D9Obj, SDKVersion);

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
