#include <windows.h>
#include "d3d8.h"

HRESULT DLLAPI ValidatePixelShader(void)
{
    OutputDebugString("ValidateVertexShader not implemented.");
    return 0;
}

HRESULT DLLAPI ValidateVertexShader(void)
{
    OutputDebugString("ValidateVertexShader not implemented.");
    return 0;
}
HRESULT DLLAPI DebugSetMute(DWORD dw1)
{
    OutputDebugString("DebugSetMute not implemented.");
    return 0;
}

DWORD DLLAPI Direct3DCreate8( UINT SDKVersion )
{
    OutputDebugString("Direct3DCreate8 not implemented.");
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
