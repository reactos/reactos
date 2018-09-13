// browseui.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "browseui.h"
#include "objbase.h"

HMODULE hShdocvw;

BOOL APIENTRY DllMain( HINSTANCE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    DisableThreadLibraryCalls(hModule);
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


typedef  VOID (CALLBACK *LPFNDllCanUnloadNow)(void);
typedef  VOID (CALLBACK *LPFNDllGetClassObject)(REFCLSID, REFIID, DWORD);

LPFNCANUNLOADNOW lpfnCan;
LPFNGETCLASSOBJECT lpfnGet;

void InitShdocvw()
{
    if (!hShdocvw)
    {
        hShdocvw=LoadLibrary("shdocvw.dll");
    }

    lpfnCan = (LPFNCANUNLOADNOW)GetProcAddress(hShdocvw, "DllCanUnloadNow");
    lpfnGet = (LPFNGETCLASSOBJECT)GetProcAddress(hShdocvw, "DllGetClassObject");
}

extern "C" STDAPI  DllCanUnloadNow(void)
{
    if (!hShdocvw)
        InitShdocvw();
    
    return (*lpfnCan)();
}


extern "C" STDAPI  DllGetClassObject(REFCLSID  rclsid, REFIID riid, LPVOID * ppv)
{
    if (!hShdocvw)
        InitShdocvw();
    
    return (*lpfnGet)(rclsid, riid, ppv);
}
