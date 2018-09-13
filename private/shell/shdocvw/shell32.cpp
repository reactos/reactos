// Wrappers for APIs that have moved elsewhere

#include "priv.h"
#include "shlwapip.h"

//------------------------------------------------------------------------
//
// APIs from SHDOCVW that are now forwarded to SHLWAPI
//
//
//  Note that we cannot use DLL forwarders because there is a bug
//  in Win95 where the loader screws up forwarders for bound DLLs.

STDAPI_(DWORD) StopWatchModeFORWARD(VOID)
{
    return StopWatchMode();
}

STDAPI_(DWORD) StopWatchFlushFORWARD(VOID)
{
    return StopWatchFlush();
}

#ifdef ux10
/*IEUNIX : In the hp-ux linker, there is no option of specifying an internal name and an external name.  */
#define StopWatch StopWatch
#define StopWatchFORWARD StopWatch
#endif 

STDAPI_(DWORD) StopWatchFORWARD(DWORD dwId, LPCSTR pszDesc, DWORD dwType, DWORD dwFlags, DWORD dwCount)
{
    return StopWatchA(dwId, (LPCSTR)pszDesc, dwType, dwFlags, dwCount);
}

//------------------------------------------------------------------------
//
// APIs from SHDOCVW that are now forwarded to SHELL32/SHDOC41

//
//  This variable name is a misnomer.  It's really
//
//  g_hinstShell32OrShdoc401DependingOnWhatWeDetected;
//
//  I can live with the misnomer; saves typing.  Think of it as
//  "the INSTANCE of SHDOC401 or whatever DLL is masquerading as
//  SHDOC401".
//
//

extern "C" { HINSTANCE g_hinstSHDOC401 = NULL; }

//
//  GetShdoc401
//
//  Detect whether we should be using Shell32 or Shdoc401 to handle
//  active desktop stuff.  The rule is
//
//  If PF_FORCESHDOC401 is set, then use shdoc401. (DEBUG only)
//  If shell32 version >= 5, then use shell32.
//  Else use shdoc401.
//
//  Warning:  THIS FUNCTION CANNOT BE CALLED DURING PROCESS_ATTACH
//  because it calls LoadLibrary.

HINSTANCE GetShdoc401()
{
    DWORD dwMajorVersion;
    HINSTANCE hinst;
    HINSTANCE hinstSh32 = GetModuleHandle(TEXT("SHELL32.DLL"));
    ASSERT(hinstSh32);

#ifdef DEBUG
    if (g_dwPrototype & PF_FORCESHDOC401) {
        hinstSh32 = NULL; // force SHDOC401 to be loaded
    }
#endif

    if (hinstSh32) {
        DLLVERSIONINFO dllinfo;
        DLLGETVERSIONPROC pfnGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstSh32, "DllGetVersion");

        dllinfo.cbSize = sizeof(DLLVERSIONINFO);
        if (pfnGetVersion && SUCCEEDED(pfnGetVersion(&dllinfo))) {
            dwMajorVersion = dllinfo.dwMajorVersion;
        } else {
            dwMajorVersion = 0;
        }
    } else {
        dwMajorVersion = 0;
    }

    if (dwMajorVersion >= 5) {
        hinst = hinstSh32;
    } else {
        hinst = LoadLibrary(TEXT("SHDOC401.DLL"));

        if (NULL == hinst)
        {
            // If this fails we're screwed
            TraceMsg(TF_ERROR, "Failed to load SHDOC401.DLL.  We're screwed...");
        }
    }
    g_hinstSHDOC401 = hinst;

    return hinst;
}

//
//  GetShdoc401ProcAddress
//
//  Get a procedure from SHDOC401 or whoever is masquerading as same.
//
//  Warning:  THIS FUNCTION CANNOT BE CALLED DURING PROCESS_ATTACH
//  because it calls LoadLibrary.

FARPROC GetShdoc401ProcAddress(FARPROC *ppfn, UINT ord)
{
    if (*ppfn) {
        return *ppfn;
    } else {
        HINSTANCE hinst = g_hinstSHDOC401;

        //
        //  No race condition here.  If two threads both call GetShdoc401,
        //  all that happens is that we load SHDOC401 into memory and then
        //  bump his refcount up to 2 instead of leaving it at 1.  Big deal.
        //
        if (hinst == NULL) {
            hinst = GetShdoc401();
        }

        if (hinst) {
            return *ppfn = GetProcAddress(hinst, (LPCSTR)LongToHandle(ord));
        } else {
            return NULL;
        }
    }
}

//
//  Delay-load-like macros.
//

#define DELAY_LOAD_SHDOC401(_type, _err, _fn, _ord, _arg, _nargs)   \
    STDAPI_(_type) _fn _arg                                         \
    {                                                               \
        static FARPROC s_pfn##_fn = NULL;                           \
        FARPROC pfn = GetShdoc401ProcAddress(&s_pfn##_fn, _ord);    \
        if (pfn) {                                                  \
            typedef _type (__stdcall *PFN##_fn) _arg;               \
            return ((PFN##_fn)pfn) _nargs;                          \
        } else {                                                    \
            return _err;                                            \
        }                                                           \
    }                                                               \

#define DELAY_LOAD_SHDOC401_VOID(_fn, _ord, _arg, _nargs)           \
    STDAPI_(void) _fn _arg                                          \
    {                                                               \
        static FARPROC s_pfn##_fn = NULL;                           \
        FARPROC pfn = GetShdoc401ProcAddress(&s_pfn##_fn, _ord);    \
        if (pfn) {                                                  \
            typedef void (__stdcall *PFN##_fn) _arg;                \
            ((PFN##_fn)pfn) _nargs;                                 \
        }                                                           \
    }                                                               \

// IE4 Shell Integrated Explorer called ShellDDEInit in shdocvw to
// set up DDE. Forward this call to SHELL32/SHDOC401 appropriately.

DELAY_LOAD_SHDOC401_VOID(ShellDDEInit, 188,
                         (BOOL fInit),
                         (fInit));

DELAY_LOAD_SHDOC401(HANDLE, NULL,
                    SHCreateDesktop, 200,
                    (IDeskTray* pdtray),
                    (pdtray));

DELAY_LOAD_SHDOC401(BOOL, FALSE,
                    SHDesktopMessageLoop, 201,
                    (HANDLE hDesktop),
                    (hDesktop));

// This may not have been used in IE4
DELAY_LOAD_SHDOC401(BOOL, FALSE,
                    DDEHandleViewFolderNotify, 202,
                    (IShellBrowser* psb, HWND hwnd, LPNMVIEWFOLDER lpnm),
                    (psb, hwnd, lpnm));

DELAY_LOAD_SHDOC401(LPNMVIEWFOLDER, NULL,
                    DDECreatePostNotify, 82,
                   (LPNMVIEWFOLDER pnm), 
                   (pnm));
                    
                    
