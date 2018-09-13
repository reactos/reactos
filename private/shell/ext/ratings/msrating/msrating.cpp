#include "msrating.h"

/* the following defs will make msluglob.h actually define globals */
#define EXTERN
#define ASSIGN(value) = value
#include "msluglob.h"

#include "ratings.h"

HANDLE g_hmtxShell = 0;             // for critical sections
HANDLE g_hsemStateCounter = 0;      // 

#ifdef DEBUG
BOOL g_fCritical=FALSE;
#endif

HINSTANCE hInstance = NULL;

long g_cRefThisDll = 0;        // Reference count of this DLL.
long g_cLocks = 0;            // Number of locks on this server.

BOOL g_bMirroredOS = FALSE;

void LockThisDLL(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cLocks);
    else
        InterlockedDecrement(&g_cLocks);
}


void RefThisDLL(BOOL fRef)
{
    if (fRef)
        InterlockedIncrement(&g_cRefThisDll);
    else
        InterlockedDecrement(&g_cRefThisDll);
}


void Netlib_EnterCriticalSection(void)
{
    WaitForSingleObject(g_hmtxShell, INFINITE);
#ifdef DEBUG
    g_fCritical=TRUE;
#endif
}

void Netlib_LeaveCriticalSection(void)
{
#ifdef DEBUG
    g_fCritical=FALSE;
#endif
    ReleaseMutex(g_hmtxShell);
}

#include <shlwapip.h>
#include <mluisupp.h>

void _ProcessAttach()
{
    ::DisableThreadLibraryCalls(::hInstance);

    MLLoadResources(::hInstance, TEXT("msratelc.dll"));

    g_hmtxShell = CreateMutex(NULL, FALSE, TEXT("MSRatingMutex"));  // per-instance
    g_hsemStateCounter = CreateSemaphore(NULL, 0, 0x7FFFFFFF, "MSRatingCounter");
    g_bMirroredOS = IS_MIRRORING_ENABLED();
    
    ::InitStringLibrary();

    RatingInit();
}

void _ProcessDetach()
{
    MLFreeResources(::hInstance);

    RatingTerm();

    CleanupWinINet();

    CleanupRatingHelpers();        /* important, must do this before CleanupOLE() */

    CleanupOLE();

    CloseHandle(g_hmtxShell);
    CloseHandle(g_hsemStateCounter);
}

STDAPI_(BOOL) DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID reserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hInstDll;
        _ProcessAttach();
    }
    else if (fdwReason == DLL_PROCESS_DETACH) 
    {
        _ProcessDetach();
    }
    
    return TRUE;
}
