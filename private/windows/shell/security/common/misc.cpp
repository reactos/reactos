//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       misc.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <advpub.h>     // REGINSTALL


/*-----------------------------------------------------------------------------
/ DPA_DestroyCallback
/ --------------
/   Same as in newer comctl32, but not present in NT4 SP4.
/
/ In:
/   -
/ Out:
/   
/----------------------------------------------------------------------------*/
#if(_WIN32_WINNT < 0x0500)
#include <comctrlp.h>
void
DPA_DestroyCallback(LPVOID hdpa, _PFNDPAENUMCALLBACK pfnCB, LPVOID pData)
{
    if (!hdpa)
        return;

    if (pfnCB)
    {
        for (int i = 0; i < DPA_GetPtrCount(hdpa); i++)
        {
            if (!pfnCB(DPA_FastGetPtr(hdpa, i), pData))
                break;
        }
    }
    DPA_Destroy((HDPA)hdpa);
}
#endif



/*-----------------------------------------------------------------------------
/ CallRegInstall
/ --------------
/   Called by DllRegisterServer and DllUnregisterServer to register/unregister
/   this module.  Uses the ADVPACK APIs and loads our INF data from resources.
/
/ In:
/   -
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
CallRegInstall(HMODULE hModule, LPCSTR pszSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack;

    TraceEnter(TRACE_COMMON_MISC, "CallRegInstall");

    hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnRegInstall = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);

        if ( pfnRegInstall )
        {
#ifdef UNICODE
            STRENTRY seReg[] =
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnRegInstall(hModule, pszSection, &stReg);
#else
            hr = pfnRegInstall(hModule, pszSection, NULL);
#endif
        }

        FreeLibrary(hinstAdvPack);
    }

    TraceLeaveResult(hr);
}
