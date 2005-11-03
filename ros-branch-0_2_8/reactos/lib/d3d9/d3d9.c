#include <windows.h>
#include "d3d9.h"

HRESULT DLLAPI Direct3DShaderValidatorCreate9(void)
{
    OutputDebugString("Direct3DShaderValidatorCreate9 not implemented.");
    return 0;
}

HRESULT DLLAPI PSGPError(void)
{
    OutputDebugString("PSGPError not implemented.");
    return 0;
}

HRESULT DLLAPI PSGPSampleTexture(void)
{
    OutputDebugString("PSGPSampleTexture not implemented.");
    return 0;
}

HRESULT DLLAPI DebugSetLevel(void)
{
    OutputDebugString("DebugSetLevel not implemented.");
    return 0;
}

HRESULT DLLAPI DebugSetMute(DWORD dw1)
{
    OutputDebugString("DebugSetMute not implemented.");
    return 0;
}

DWORD DLLAPI Direct3DCreate9( UINT SDKVersion )
{
    OutputDebugString("Direct3DCreate9 not implemented.");
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
