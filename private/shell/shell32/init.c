#include "shellprv.h"
#pragma  hdrstop

#include "copy.h"

#include "ovrlaymn.h"
#include "drives.h"

#include "unicpp\admovr2.h"

void FreeExtractIconInfo(int);
void DAD_ThreadDetach(void);
void DAD_ProcessDetach(void);
void TaskMem_MakeInvalid(void);
void UltRoot_Term(void);
void NetRoot_Terminate(void);
void FlushRunDlgMRU(void);

STDAPI_(BOOL) ATL_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/);

// from mtpt.cpp
STDAPI_(void) CMtPt_FinalCleanUp();
STDAPI_(void) CMtPt_Initialize();
STDAPI_(void) CMtPt_FakeVolatileKeys();
// from rgsprtc.cpp
STDAPI_(void) CRegSupportCached_RSEnableKeyCaching(BOOL fEnable);

// Global data

BOOL g_bMirroredOS = FALSE;         // Is Mirroring enabled 
BOOL g_bBiDiPlatform = FALSE;       // Is DATE_LTRREADING flag supported by GetDateFormat() API?   
#ifdef WINDOWS_ME
// This is needed for BiDi localized win95 RTL stuff
BOOL g_bBiDiW95Loc = FALSE;
#endif // WINDOWS_ME
HINSTANCE g_hinst = NULL;
extern HANDLE g_hCounter;   // Global count of mods to Special Folder cache.
extern HANDLE g_hRestrictions ; // Global count of mods to restriction cache.
extern HANDLE g_hSettings;  // global count of mods to shellsettings cache

HKEY g_hkcrCLSID = NULL;        // HKEY_CLASSES_ROOT\CLSID
HKEY g_hklmExplorer = NULL;     // caching for HKEY_LOCAL_MACHINE\...\Explorer


#ifdef DEBUG
BOOL  g_bInDllEntry = FALSE;
#endif

#pragma data_seg(DATASEG_SHARED)

// evil global global data on Win95 only. on NT these are regular per process

LONG g_cProcesses = 0;
CRITICAL_SECTION g_csDll = {0};
CRITICAL_SECTION g_csPrinters = {0};


// these will always be zero
const LARGE_INTEGER g_li0 = {0};
const ULARGE_INTEGER g_uli0 = {0};

#ifdef WINNT
BOOL g_bRunOnNT5 = FALSE;
#else
BOOL g_bRunOnMemphis = FALSE;
#endif

#pragma data_seg()


#ifdef DEBUG
// Undefine what shlwapi.h defined so our ordinal asserts map correctly
#undef PathAddBackslash 
WINSHELLAPI LPTSTR WINAPI PathAddBackslash(LPTSTR lpszPath);
#undef PathMatchSpec
WINSHELLAPI BOOL  WINAPI PathMatchSpec(LPCTSTR pszFile, LPCTSTR pszSpec);
#endif

BOOL _ProcessAttach(HINSTANCE hDll)
{
    g_hinst = hDll;

    g_uCodePage = GetACP();

#ifdef WINNT
    InitializeCriticalSection(&g_csDll);
    InitializeCriticalSection(&g_csPrinters);
#else
    // these are in global global data, thus we need the "Re" version of these APIs
    ReinitializeCriticalSection(&g_csPrinters);
    ReinitializeCriticalSection(&g_csDll);
#endif

    // Initialize the MountPoint stuff
    CMtPt_Initialize();

    // We need to disable HKEY caching for classes using CRegSupportCached in
    // winlogon since it does not get unloaded.  This must alos apply to the other
    // services loading shell32.dllNot being unloaded the _ProcessDetach
    // never gets called for shell32, and we never release the cached HKEYs.  This
    // prevents the user hives from being unloaded.  SO we'll enable caching only
    // for Explorer.exe.  (stephstm, 08/20/99)
    //
    {
        TCHAR  szModulePath[MAX_PATH];
        LPTSTR pszModuleName;
        BOOL fEnable = FALSE;

        GetModuleFileName(GetModuleHandle(NULL), szModulePath, ARRAYSIZE(szModulePath));
        pszModuleName = PathFindFileName(szModulePath);

        if (pszModuleName)
        {
            // Is this winlogon?
            if (!lstrcmpi(TEXT("explorer.exe"), pszModuleName))
            {
                // No
                fEnable = TRUE;
            }
        }
    
        CRegSupportCached_RSEnableKeyCaching(fEnable);
    }

    //
    // Check if the mirroring APIs exist on the current
    // platform.
    //
    g_bMirroredOS = IS_MIRRORING_ENABLED();

#ifdef WINDOWS_ME
    //
    // Check to see if running on BiDi localized Win95
    //
    g_bBiDiW95Loc = IsBiDiLocalizedWin95(FALSE);
#endif // WINDOWS_ME


    // globally useful registry keys
    RegOpenKey(HKEY_CLASSES_ROOT, c_szCLSID, &g_hkcrCLSID);
    RegCreateKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_EXPLORER, &g_hklmExplorer);

    InterlockedIncrement(&g_cProcesses);

#ifdef WINNT
    g_bRunOnNT5     = IsOS(OS_NT5);
    g_bBiDiPlatform =  BOOLFROMPTR(GetModuleHandle(TEXT("LPK.DLL")));
#else
    g_bRunOnMemphis = IsOS(OS_MEMPHIS);
    g_bBiDiPlatform = ((g_uCodePage == CP_ARABIC) || (g_uCodePage == CP_HEBREW));
#endif

#ifdef DEBUG
#define DEREFMACRO(x) x
#define ValidateORD(_name) ASSERT( _name == (LPVOID)GetProcAddress(hDll, (LPSTR)MAKEINTRESOURCE(DEREFMACRO(_name##ORD))) )
    if (g_cProcesses==1)        // no need to be in critical section (just debug)
    {
        ValidateORD(SHValidateUNC);
        ValidateORD(SHChangeNotifyRegister);
        ValidateORD(SHChangeNotifyDeregister);
        ValidateORD(OleStrToStrN);
        ValidateORD(SHCloneSpecialIDList);
        ASSERT(DllGetClassObject==(LPVOID)GetProcAddress(hDll,(LPSTR)MAKEINTRESOURCE(SHDllGetClassObjectORD)));
        ValidateORD(SHLogILFromFSIL);
        ValidateORD(SHMapPIDLToSystemImageListIndex);
        ValidateORD(SHShellFolderView_Message);
        ValidateORD(Shell_GetImageLists);
        ValidateORD(SHGetSpecialFolderPath);
        ValidateORD(StrToOleStrN);

        ValidateORD(ILClone);
        ValidateORD(ILCloneFirst);
        ValidateORD(ILCombine);
        ValidateORD(ILCreateFromPath);
        ValidateORD(ILFindChild);
        ValidateORD(ILFree);
        ValidateORD(ILGetNext);
        ValidateORD(ILGetSize);
        ValidateORD(ILIsEqual);
        ValidateORD(ILRemoveLastID);
        ValidateORD(PathAddBackslash);
        ValidateORD(PathCombine);
        ValidateORD(PathIsExe);
        ValidateORD(PathMatchSpec);
        ValidateORD(SHGetSetSettings);
        ValidateORD(SHILCreateFromPath);
        ValidateORD(SHFree);

        ValidateORD(SHAddFromPropSheetExtArray);
        ValidateORD(SHCreatePropSheetExtArray);
        ValidateORD(SHDestroyPropSheetExtArray);
        ValidateORD(SHReplaceFromPropSheetExtArray);
        ValidateORD(SHCreateDefClassObject);
        ValidateORD(SHGetNetResource);
    }
#endif  // DEBUG

#ifdef DEBUG
    {
        extern LPMALLOC g_pmemTask;
        AssertMsg(g_pmemTask == NULL, TEXT("Somebody called SHAlloc in DllEntry!"));
    }
#endif

    return TRUE;
}

#if defined(WINNT) || defined(DEBUG)
//
//  Table of all window classes we register so we can unregister them
//  at DLL unload.
//
extern const TCHAR c_szDefViewClass[];
extern TCHAR const c_szFakeDesktopClass[];
extern const TCHAR c_szBackgroundPreview2[];
extern const TCHAR c_szComponentPreview[];
extern const TCHAR c_szWindowClassName[];

const LPCTSTR c_rgszClasses[] = {
    c_szDefViewClass,                       // defview.cpp
    TEXT("WOACnslWinPreview"),              // lnkcon.c
    TEXT("WOACnslFontPreview"),             // lnkcon.c
    TEXT("cpColor"),                        // lnkcon.c
    TEXT("cpShowColor"),                    // lnkcon.c
    c_szFakeDesktopClass,                   // restart.c
    c_szStubWindowClass,                    // rundll32.c
    c_szBackgroundPreview2,                 // unicpp\dbackp.cpp
    c_szComponentPreview,                   // unicpp\dcompp.cpp
    TEXT(STR_DESKTOPCLASS),                 // unicpp\desktop.cpp
    TEXT("MSGlobalFolderOptionsStub"),      // unicpp\options.cpp
    c_szWindowClassName,                    // fsnotify.c
    TEXT("DivWindow"),                      // fsrchdlg.h
    LINKWINDOW_CLASS,                       // linkwnd.cpp
    TEXT("ATL Shell Embedding"),            // unicpp\dvoc.h
    TEXT("ShellFileSearchControl"),         // fsearch.h
    TEXT("GroupButton"),                    // fsearch
    TEXT("ATL:STATIC"),                     // unicpp\deskmovr.cpp
    TEXT("DeskMover"),                      // unicpp\deskmovr.cpp
};

#endif

#ifdef WINNT
#define UnregisterWindowClasses() \
    SHUnregisterClasses(HINST_THISDLL, c_rgszClasses, ARRAYSIZE(c_rgszClasses))
#else
// Win95 automatically unregisters classes when a DLL unloads
#define UnregisterWindowClasses()
#endif

void _ProcessDetach(BOOL bProcessShutdown)
{
#ifdef DEBUG
    if (bProcessShutdown)
    {
        // to catch bugs where people use the task allocator at process
        // detatch time (this is a problem becuase OLE32.DLL could be unloaded)
        TaskMem_MakeInvalid(); 
    }
#endif

    FlushRunDlgMRU();

    // Flush the file class before we free any DLLs, to work around a possible
    // Windows 95 loader bug.  The scenario is:
    //
    // MPRSERV.DLL loads SYNCENG to do some user profile file copying.  SYNCENG
    // in turn links to LINKINFO which links to MPR;  it also links to SHELL32,
    // which links to COMCTL32 and delay-links to MPR as well.  When SYNCENG
    // is unloaded, the loader sends process detach notifications to the entire
    // DLL tree.  When we unload our reference to MPR here, that sometimes
    // seems to cause the loader to continue with its process detach messages
    // to more DLLs in the tree, including COMCTL32.  COMCTL32 responds to this
    // by destroying the shared heap.  Unfortunately we then return back to
    // this chunk of code (we're in SHELL32, remember), and call FlushFileClass.
    // That tries to access COMCTL32's shared heap and faults.
    //
    // Since this is really a bug in the Windows 95 loader, we work around it
    // here, by calling FlushFileClass (and anything else that references the
    // shared heap) before any FreeLibrary calls.
    //
    // Do not reference the shared heap after the block of Xxxx_Term() calls
    // below.
    // -- gregj, 03/26/97

    // Flush the file class cache, some app may have changed associations
    // BUGBUG is this too often?
    FlushFileClass();

    //
    // All the per-instance terminate code should be done here.
    // WARNING: Do not touch the shared heap below this point!
    //

    if (!bProcessShutdown)
    {
        // some of these may use the task allocator. we can only do
        // this when we our DLL is being unloaded in a process, not
        // at process term since OLE32 might not be around to be called
        // at process shutdown time this memory will be freed as a result
        // of the process address space going away.

        SpecialFolderIDTerminate();
        BitBucket_Terminate();

        UltRoot_Term();
        RLTerminate();          // close our use of the Registry list...
        DAD_ProcessDetach();

        CDrives_Terminate();
        NetRoot_Terminate();
        CopyHooksTerminate();
        IconOverlayManagerTerminate();

        // being unloaded via FreeLibrary, then do some more stuff.
        // Don't need to do this on process terminate.
        UnregisterWindowClasses();
        FreeExtractIconInfo(-1);
    }

    if (InterlockedDecrement(&g_cProcesses) == 0)
    {
        // On Win95 where we have global global data only do 
        // this on the very last proces detatch. g_cProcesses
        // is a global global variable that tells how many process are
        // using shell32.dll
        
        // on NT this is value is always not global so we always execute this code

        DestroyHashItemTable(NULL);
        FileIconTerm();
        SHChangeNotifyTerminate(TRUE);

#ifndef WINNT
        CMtPt_FakeVolatileKeys();
#endif
    }
    else
        SHChangeNotifyTerminate(FALSE);  // still some clients out there

    // global resources that we need to free in all cases

    CMtPt_FinalCleanUp();

    if (g_hkcrCLSID)
        RegCloseKey(g_hkcrCLSID);
    if (g_hklmExplorer)
        RegCloseKey(g_hklmExplorer);

    SHDestroyCachedGlobalCounter(&g_hCounter);
    SHDestroyCachedGlobalCounter(&g_hRestrictions);
    SHDestroyCachedGlobalCounter(&g_hSettings);

#ifdef WINNT
    if (g_hklmApprovedExt && g_hklmApprovedExt != INVALID_HANDLE_VALUE)
        RegCloseKey(g_hklmApprovedExt);

    DeleteCriticalSection(&g_csDll);
    DeleteCriticalSection(&g_csPrinters);
#endif
}

BOOL _ThreadDetach()
{
    ASSERTNONCRITICAL           // Thread shouldn't term while holding CS
    DAD_ThreadDetach();
    return TRUE;
}

#ifndef WINNT
// created by the thunk scripts
BOOL WINAPI Shl3216_ThunkConnect32(LPCTSTR pszDll16, LPCTSTR pszDll32, HANDLE hIinst, DWORD dwReason);
BOOL WINAPI Shl1632_ThunkConnect32(LPCTSTR pszDll16, LPCTSTR pszDll32, HANDLE hIinst, DWORD dwReason);
#endif

// ccover uses c run time, so we need to have entry point called DllMain
#if defined(CCOVER) || defined(_WIN64)
#define DllEntry DllMain
#endif

STDAPI_(BOOL) DllEntry(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
#ifndef WINNT
    if (!Shl3216_ThunkConnect32(c_szShell16Dll, c_szShell32Dll, hDll, dwReason))
        return FALSE;

    if (!Shl1632_ThunkConnect32(c_szShell16Dll, c_szShell32Dll, hDll, dwReason))
        return FALSE;
#endif

#ifdef DEBUG
    g_bInDllEntry = TRUE;
#endif

    switch(dwReason) {
    case DLL_PROCESS_ATTACH:
        CcshellGetDebugFlags();     // Don't put this line under #ifdef
        _ProcessAttach(hDll);
        break;

    case DLL_PROCESS_DETACH:
        _ProcessDetach(lpReserved != NULL);
        break;

    case DLL_THREAD_DETACH:
        _ThreadDetach();
        break;

    default:
        break;
    }

    ATL_DllMain(hDll, dwReason, lpReserved);

#ifdef DEBUG
    g_bInDllEntry = FALSE;
#endif

    return TRUE;
}

#ifdef DEBUG
LRESULT WINAPI SendMessageD( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    ASSERTNONCRITICAL;
#ifdef UNICODE
    return SendMessageW(hWnd, Msg, wParam, lParam);
#else
    return SendMessageA(hWnd, Msg, wParam, lParam);
#endif
}

//
//  In DEBUG, make sure every class we register lives in the c_rgszClasses
//  table so we can clean up properly at DLL unload.  NT does not automatically
//  unregister classes when a DLL unloads, so we have to do it manually.
//
ATOM WINAPI RegisterClassD(CONST WNDCLASS *pwc)
{
    int i;
    for (i = 0; i < ARRAYSIZE(c_rgszClasses); i++) {
        if (lstrcmpi(c_rgszClasses[i], pwc->lpszClassName) == 0) {
            return RealRegisterClass(pwc);
        }
    }
    AssertMsg(0, TEXT("Class %s needs to be added to the c_rgszClasses list"), pwc->lpszClassName);
    return 0;
}

ATOM WINAPI RegisterClassExD(CONST WNDCLASSEX *pwc)
{
    int i;
    for (i = 0; i < ARRAYSIZE(c_rgszClasses); i++) {
        if (lstrcmpi(c_rgszClasses[i], pwc->lpszClassName) == 0) {
            return RealRegisterClassEx(pwc);
        }
    }
    AssertMsg(0, TEXT("Class %s needs to be added to the c_rgszClasses list"), pwc->lpszClassName);
    return 0;
}

//
//  In DEBUG, send FindWindow through a wrapper that ensures that the
//  critical section is not taken.  FindWindow'ing for a window title
//  sends inter-thread WM_GETTEXT messages, which is not obvious.
//
STDAPI_(HWND) FindWindowD(LPCTSTR lpClassName, LPCTSTR lpWindowName)
{
    return FindWindowExD(NULL, NULL, lpClassName, lpWindowName);
}

STDAPI_(HWND) FindWindowExD(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpClassName, LPCTSTR lpWindowName)
{
    if (lpWindowName) {
        ASSERTNONCRITICAL;
    }
    return RealFindWindowEx(hwndParent, hwndChildAfter, lpClassName, lpWindowName);
}

#endif // DEBUG

STDAPI DllCanUnloadNow()
{
    // shell32 won't be able to be unloaded since there are lots of APIs and
    // other non COM things that will need to keep it loaded
    return S_FALSE;
}
