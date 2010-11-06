#include <windows.h>
#include "d3d8.h"

#include <debug.h>

HRESULT WINAPI ValidatePixelShader(DWORD* pixelshader, DWORD* reserved1, BOOL bool, DWORD* toto)
{
    UNIMPLEMENTED
    return 0;
}

HRESULT WINAPI ValidateVertexShader(DWORD* vertexshader, DWORD* reserved1, DWORD* reserved2, BOOL bool, DWORD* toto)
{
    UNIMPLEMENTED
    return 0;
}

HRESULT WINAPI D3D8GetSWInfo(void)
{
    UNIMPLEMENTED
    return 0;
}

HRESULT WINAPI DebugSetMute(void)
{
    UNIMPLEMENTED
    return 0;
}

DWORD WINAPI Direct3DCreate8( UINT SDKVersion )
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
