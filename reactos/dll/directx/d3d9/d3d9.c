#include <windows.h>
#include "d3d9_private.h"

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
    UNIMPLEMENTED
    return 0;
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
