/***************************************************************************
 *  dll.c
 *
 *  Standard DLL entry-point functions
 *
 ***************************************************************************/

#include "priv.h"
#include <ntverp.h>

#define MLUI_INIT
#include <mluisupp.h>

BOOL g_bRunningOnNT = FALSE;
BOOL g_bRunningOnNT5OrHigher = FALSE;
BOOL g_bRunningOnMemphis = FALSE;
HINSTANCE g_hinst = NULL;
CRITICAL_SECTION g_csDll = {0};
DWORD g_TpsTls = (UINT)-1;
DWORD g_tlsThreadRef  = (UINT)-1;
BOOL g_bDllTerminating = FALSE;

#ifdef DEBUG
//#define PROOFREAD_PARSES
#endif

#ifdef PROOFREAD_PARSES
enum
{
    PP_COMPARE,
    PP_ORIGINAL_ONLY,
    PP_NEW_ONLY
};


DWORD g_dwProofMode = PP_COMPARE;

#endif // PROOFREAD_PARSES

void TermPalette();
void DeinitPUI();
void FreeDynamicLibraries();
void FreeCachedAcl();

//
//  Table of all window classes we register so we can unregister them
//  at DLL unload.
//
//  Since we are single-binary, we have to play it safe and do
//  this cleanup (needed only on NT, but harmless on Win95).
//
const LPCTSTR c_rgszClasses[] = {
    TEXT("WorkerA"),                        // util.cpp
    TEXT("WorkerW"),                        // util.cpp
};

//
// Global DCs used during mirroring an Icon.
//
HDC g_hdc = NULL, g_hdcMask = NULL;
BOOL g_bMirroredOS = FALSE;


BOOL APIENTRY DllMain(IN HANDLE hDll, IN DWORD dwReason, IN LPVOID lpReserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hDll);

#ifdef DEBUG
        CcshellGetDebugFlags();
#endif
        InitializeCriticalSection(&g_csDll);   // for later use

        g_hinst = hDll;
        MLLoadResources(g_hinst, TEXT("shlwaplc.dll"));
        g_bRunningOnNT = IsOS(OS_NT);
        g_bRunningOnNT5OrHigher = IsOS(OS_NT5);
        g_bRunningOnMemphis = IsOS(OS_MEMPHIS);

        InitStopWatchMode();    // See if perf mode is enabled

        // Check if we are running on a system that supports the mirroring APIs
        // i.e. (NT5 or Memphis/BiDi)
        //
        g_bMirroredOS = IS_MIRRORING_ENABLED();
        g_TpsTls = TlsAlloc();
        g_tlsThreadRef = TlsAlloc();

#ifdef PROOFREAD_PARSES
        {
            DWORD dwSize = sizeof(g_dwProofMode);
            if (ERROR_SUCCESS != SHGetValue( HKEY_CURRENT_USER,
                TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                TEXT("Verify URLCombine"), NULL, &g_dwProofMode, &dwSize) ||
                (g_dwProofMode > PP_NEW_ONLY))
            {
                g_dwProofMode = PP_COMPARE;
            }
        }
#endif
        break;

    case DLL_PROCESS_DETACH:
        g_bDllTerminating = TRUE;
        MLFreeResources(g_hinst);
        if (lpReserved == NULL) 
            DeinitPUI();            // free up plug ui resource hinstance dpa table

        //
        // Icon mirroring stuff (see mirror.c)
        // Cleanup cached DCs. No need to synchronize the following section of
        // code since it is only called in DLL_PROCESS_DETACH which is
        // synchronized by the OS Loader.
        //
        if (g_bMirroredOS)
        {
            if (g_hdc)
                DeleteDC(g_hdc);

            if (g_hdcMask)
                DeleteDC(g_hdcMask);

            g_hdc = g_hdcMask = NULL;
        }

        FreeCachedAcl();
        TermPalette();
        if (StopWatchMode())
            StopWatchFlush();   // Flush the performance timing data to disk
        DeleteCriticalSection(&g_csDll);

        if (lpReserved == NULL) 
        {
            SHTerminateThreadPool();
            SHUnregisterClasses(HINST_THISDLL, c_rgszClasses, ARRAYSIZE(c_rgszClasses));
#ifdef I_WANT_WIN95_TO_CRASH
            // If you call FreeLibrary during PROCESS_ATTACH, Win95 will crash
            FreeDynamicLibraries();
#endif
        }

        if (g_TpsTls != (UINT)-1)
            TlsFree(g_TpsTls);

        if (g_tlsThreadRef != (UINT)-1)
            TlsFree(g_tlsThreadRef);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        ASSERT(0);  // We shouldn't get these because we called DisableThreadLibraryCalls().
        break;

    default:
        break;
    }

    return TRUE;
}


// DllGetVersion
//
// All we have to do is declare this puppy and CCDllGetVersion does the rest
//
DLLVER_SINGLEBINARY(VER_PRODUCTVERSION_DW, VER_PRODUCTBUILD_QFE);
