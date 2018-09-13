/***************************************************************************
 *  msctls.c
 *
 *      Utils library initialization code
 *
 ***************************************************************************/

#include "ctlspriv.h"

HINSTANCE g_hinst = 0;

#ifndef UNIX
CRITICAL_SECTION g_csDll = {{0},0, 0, NULL, NULL, 0 };
#else
/* IEUNIX:  MainWin uses DllMain as an entry point (Ref: mwdip) */
#define LibMain    DllMain
#include "mwversion.h"
#if defined(MW_STRUCTINIT_SUPPORTED)
CRITICAL_SECTION g_csDll = {{0},0, 0, NULL, NULL, 0 };
#else
CRITICAL_SECTION g_csDll;
#endif
#endif /* UNIX */

ATOM g_aCC32Subclass = 0;

#ifdef WINNT
BOOL g_bRunOnNT5 = FALSE;
BOOL g_bRemoteSession = FALSE;
#else
BOOL g_bRunOnMemphis = FALSE;
BOOL g_bRunOnBiDiWin95Loc = FALSE;
int g_cProcesses = 0;
#endif

UINT g_uiACP = CP_ACP;

// Is Mirroring enabled
BOOL g_bMirroredOS = FALSE;


#define PAGER //For Test Purposes

//
// Global DCs used during mirroring an Icon.
//
HDC g_hdc=NULL, g_hdcMask=NULL;

// per process mem to store PlugUI information
#ifdef WINNT
LANGID g_PUILangId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
#else
HDPA g_hdpaPUI = NULL;
void InitPUI();
void DeInitPUI(int cProcesses);
#endif

BOOL PASCAL InitAnimateClass(HINSTANCE hInstance);
BOOL ListView_Init(HINSTANCE hinst);
BOOL TV_Init(HINSTANCE hinst);
BOOL InitComboExClass(HINSTANCE hinst);
BOOL PASCAL Header_Init(HINSTANCE hinst);
BOOL PASCAL Tab_Init(HINSTANCE hinst);
int InitIPAddr(HANDLE hInstance);

#if !defined(WINNT) && defined(FONT_LINK)
void InitMLANG();
void DeinitMLANG(int cProcesses);
#endif

#ifdef PAGER
BOOL InitPager(HINSTANCE hinst);
#endif
BOOL InitNativeFontCtl(HINSTANCE hinst);
void UnregisterClasses();
void Mem_Terminate();

#define DECLARE_DELAYED_FUNC(_ret, _fn, _args, _nargs) \
_ret (__stdcall * g_pfn##_fn) _args = NULL; \
_ret __stdcall _fn _args                \
{                                       \
     if (!g_pfn##_fn) {                  \
        AssertMsg(g_pfn##_fn != NULL, TEXT("GetProcAddress failed")); \
        return 0; \
     }     \
     return g_pfn##_fn _nargs; \
}
    
#define LOAD_DELAYED_FUNC(_ret, _fn, _args) \
    (*(FARPROC*)&(g_pfn##_fn) = GetProcAddress(hinst, #_fn))


DECLARE_DELAYED_FUNC(BOOL, ImmNotifyIME, (HIMC himc, DWORD dw1, DWORD dw2, DWORD dw3), (himc, dw1, dw2, dw3));
DECLARE_DELAYED_FUNC(HIMC, ImmAssociateContext, (HWND hwnd, HIMC himc), (hwnd, himc));
DECLARE_DELAYED_FUNC(BOOL, ImmReleaseContext, (HWND hwnd, HIMC himc), (hwnd, himc));
DECLARE_DELAYED_FUNC(HIMC, ImmGetContext, (HWND hwnd), (hwnd));
DECLARE_DELAYED_FUNC(LONG, ImmGetCompositionStringA, (HIMC himc, DWORD dw1, LPVOID p1, DWORD dw2), (himc, dw1, p1, dw2) );
DECLARE_DELAYED_FUNC(BOOL, ImmSetCompositionStringA, (HIMC himc, DWORD dw1, LPCVOID p1, DWORD dw2, LPCVOID p2, DWORD dw3), (himc, dw1, p1, dw2, p2, dw3));
#ifndef UNICODE_WIN9x
DECLARE_DELAYED_FUNC(LONG, ImmGetCompositionStringW, (HIMC himc, DWORD dw1, LPVOID p1, DWORD dw2), (himc, dw1, p1, dw2) );
DECLARE_DELAYED_FUNC(BOOL, ImmSetCompositionStringW, (HIMC himc, DWORD dw1, LPCVOID p1, DWORD dw2, LPCVOID p2, DWORD dw3), (himc, dw1, p1, dw2, p2, dw3));
#endif
DECLARE_DELAYED_FUNC(BOOL, ImmSetCandidateWindow, (HIMC himc, LPCANDIDATEFORM pcf), (himc, pcf));
DECLARE_DELAYED_FUNC(HIMC, ImmCreateContext, (void), ());
DECLARE_DELAYED_FUNC(BOOL, ImmDestroyContext, (HIMC himc), (himc));
    

BOOL g_fDBCSEnabled = FALSE;
BOOL g_fMEEnabled = FALSE;
BOOL g_fThaiEnabled = FALSE;
BOOL g_fDBCSInputEnabled = FALSE;
#ifdef FONT_LINK
BOOL g_bComplexPlatform = FALSE;
#endif

#if defined(FE_IME) || !defined(WINNT)
void InitIme()
{
    g_fMEEnabled = GetSystemMetrics(SM_MIDEASTENABLED);
    g_fThaiEnabled = (GetACP() == 874);
    g_fDBCSEnabled = g_fDBCSInputEnabled = GetSystemMetrics(SM_DBCSENABLED);

    if (!g_fDBCSInputEnabled && g_bRunOnNT5)
        g_fDBCSInputEnabled =  GetSystemMetrics(SM_IMMENABLED);
    
    // We load imm32.dll per process, but initialize proc pointers just once.
    // this is to solve two different problems.
    // 1) Debugging process on win95 would get our shared table trashed
    //    if we rewrite proc address each time we get loaded.
    // 2) Some lotus application rely upon us to load imm32. They do not
    //    load/link to imm yet they use imm(!)
    //
    if (g_fDBCSInputEnabled) {
        HANDLE hinst = LoadLibrary(TEXT("imm32.dll"));
        if (! g_pfnImmSetCandidateWindow && 
           (! hinst || 
            ! LOAD_DELAYED_FUNC(HIMC, ImmCreateContext, (void)) ||
            ! LOAD_DELAYED_FUNC(HIMC, ImmDestroyContext, (HIMC)) ||
            ! LOAD_DELAYED_FUNC(BOOL, ImmNotifyIME, (HIMC, DWORD, DWORD, DWORD)) ||
            ! LOAD_DELAYED_FUNC(HIMC, ImmAssociateContext, (HWND, HIMC)) ||
            ! LOAD_DELAYED_FUNC(BOOL, ImmReleaseContext, (HWND, HIMC)) ||
            ! LOAD_DELAYED_FUNC(HIMC, ImmGetContext, (HWND)) ||
            ! LOAD_DELAYED_FUNC(LONG, ImmGetCompositionStringA, (HIMC, DWORD, LPVOID, DWORD)) ||
            ! LOAD_DELAYED_FUNC(BOOL, ImmSetCompositionStringA, (HIMC, DWORD, LPCVOID, DWORD, LPCVOID, DWORD)) ||
#ifndef UNICODE_WIN9x
            ! LOAD_DELAYED_FUNC(LONG, ImmGetCompositionStringW, (HIMC, DWORD, LPVOID, DWORD)) ||
            ! LOAD_DELAYED_FUNC(BOOL, ImmSetCompositionStringW, (HIMC, DWORD, LPCVOID, DWORD, LPCVOID, DWORD)) ||
#endif
            ! LOAD_DELAYED_FUNC(BOOL, ImmSetCandidateWindow, (HIMC, LPCANDIDATEFORM)))) {

            // if we were unable to load then bail on using IME.
            g_fDBCSEnabled = FALSE;
            g_fDBCSInputEnabled = FALSE;

        }
    }
}
#else
#define InitIme() 0
#endif


#ifdef DEBUG

// Verify that the localizers didn't accidentally change
// DLG_PROPSHEET from a DIALOG to a DIALOGEX.  _RealPropertySheet
// relies on this (as well as any apps which parse the dialog template
// in their PSCB_PRECREATE handler).

BOOL IsSimpleDialog(LPCTSTR ptszDialog)
{
    HRSRC hrsrc;
    LPDLGTEMPLATE pdlg;
    BOOL fSimple = FALSE;

    if ( (hrsrc = FindResource(HINST_THISDLL, ptszDialog, RT_DIALOG)) &&
         (pdlg = LoadResource(HINST_THISDLL, hrsrc)))
    {
        fSimple = HIWORD(pdlg->style) != 0xFFFF;
    }
    return fSimple;
}

//
//  For sublanguages to work, every language in our resources must contain
//  a SUBLANG_NEUTRAL variation so that (for example) Austria gets
//  German dialog boxes instead of English ones.
//
//  The DPA is really a DSA of WORDs, but DPA's are easier to deal with.
//  We just collect all the languages into the DPA, and study them afterwards.
//
BOOL CALLBACK CheckLangProc(HINSTANCE hinst, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIdLang, LPARAM lparam)
{
    HDPA hdpa = (HDPA)lparam;
    DPA_AppendPtr(hdpa, (LPVOID)(UINT_PTR)wIdLang);
    return TRUE;
}

void CheckResourceLanguages(void)
{
    HDPA hdpa = DPA_Create(8);
    if (hdpa) {
        int i, j;
        EnumResourceLanguages(HINST_THISDLL, RT_DIALOG,
                              MAKEINTRESOURCE(DLG_PROPSHEET), CheckLangProc,
                              (LPARAM)hdpa);

        // Walk the language list.  For each language we find, make sure
        // there is a SUBLANG_NEUTRAL version of it somewhere else
        // in the list.  We use an O(n^2) algorithm because this is debug
        // only code and happens only at DLL load.

        for (i = 0; i < DPA_GetPtrCount(hdpa); i++) {
            UINT_PTR uLangI = (UINT_PTR)DPA_FastGetPtr(hdpa, i);
            BOOL fFound = FALSE;

            //
            //  It is okay to have English (American) with no
            //  English (Neutral) because Kernel32 uses English (American)
            //  as its fallback, so we fall back to the correct language
            //  after all.
            //
            if (uLangI == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
                continue;

            //
            //  If this language is already the Neutral one, then there's
            //  no point looking for it - here it is!
            //
            if (SUBLANGID(uLangI) == SUBLANG_NEUTRAL)
                continue;

            //
            //  Otherwise, this language is a dialect.  See if there is
            //  a Neutral version elsewhere in the table.
            //
            for (j = 0; j < DPA_GetPtrCount(hdpa); j++) {
                UINT_PTR uLangJ = (UINT_PTR)DPA_FastGetPtr(hdpa, j);
                if (PRIMARYLANGID(uLangI) == PRIMARYLANGID(uLangJ) &&
                    SUBLANGID(uLangJ) == SUBLANG_NEUTRAL) {
                    fFound = TRUE; break;
                }
            }

            //
            //  If this assertion fires, it means that the localization team
            //  added support for a new language but chose to specify the
            //  language as a dialect instead of the Neutral version.  E.g.,
            //  specifying Romanian (Romanian) instead of Romanian (Neutral).
            //  This means that people who live in Moldavia will see English
            //  strings, even though Romanian (Romanian) would almost
            //  certainly have been acceptable.
            //
            //  If you want to support multiple dialects of a language
            //  (e.g., Chinese), you should nominate one of the dialects
            //  as the Neutral one.  For example, we currently support
            //  both Chinese (PRC) and Chinese (Taiwan), but the Taiwanese
            //  version is marked as Chinese (Neutral), so people who live in
            //  Singapore get Chinese instead of English.  Sure, it's
            //  Taiwanese Chinese, but at least it's Chinese.
            //
            AssertMsg(fFound, TEXT("Localization bug: No SUBLANG_NEUTRAL for language %04x"), uLangI);
        }

        DPA_Destroy(hdpa);
    }
}

#endif

int _ProcessAttach(HANDLE hInstance)
{
    INITCOMMONCONTROLSEX icce;

    g_hinst = hInstance;

    g_uiACP = GetACP();

#if defined(MAINWIN)
    MwSet3dLook(TRUE);
#endif

#ifdef DEBUG
    CcshellGetDebugFlags();
#endif

#ifdef WINNT
    InitializeCriticalSection(&g_csDll);

    g_bRunOnNT5 = staticIsOS(OS_NT5);
#ifdef FONT_LINK
    g_bComplexPlatform =  BOOLFROMPTR(GetModuleHandle(TEXT("LPK.DLL")));
#endif
#else
    ReinitializeCriticalSection(&g_csDll);

#ifdef FONT_LINK
    g_bComplexPlatform = ((g_uiACP == CP_ARABIC) || (g_uiACP == CP_HEBREW) || (g_uiACP == CP_THAI));
#endif
    g_bRunOnMemphis = staticIsOS(OS_MEMPHIS);
    g_bRunOnBiDiWin95Loc = IsBiDiLocalizedWin95(FALSE);

    g_cProcesses++;
    {
        // HACK: we are intentionally incrementing the refcount on this atom
        // WE DO NOT WANT IT TO GO BACK DOWN so we will not delete it in process
        // detach (see comments for g_aCC32Subclass in subclass.c for more info)

        // on Win95 doe this as early as possible to avoid getting
        // a trashed atom. 

        ATOM a = GlobalAddAtom(c_szCC32Subclass);
        if (a != 0)
            g_aCC32Subclass = a;    // in case the old atom got nuked
    }
#endif

    //
    // Check if the mirroring APIs exist on the current
    // platform.
    //
    g_bMirroredOS = IS_MIRRORING_ENABLED();

#ifdef WINNT
    //
    //  Must detect Terminal Server before initializing global metrics
    //  because we need to force some features off if running Terminal Server.
    //
    {
        typedef BOOL (__stdcall * PFNPROCESSIDTOSESSIONID)(DWORD, PDWORD);
        PFNPROCESSIDTOSESSIONID ProcessIdToSessionId =
                    (PFNPROCESSIDTOSESSIONID)
                    GetProcAddress(GetModuleHandle(TEXT("KERNEL32")),
                                   "ProcessIdToSessionId");
        DWORD dwSessionId;
        g_bRemoteSession = ProcessIdToSessionId &&
                           ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId) &&
                           dwSessionId != 0;
    }
#endif

    InitGlobalMetrics(0);
    InitGlobalColors();
    
    InitIme();

#ifndef WINNT
#ifdef FONT_LINK
    InitMLANG();
#endif
    InitPUI();
#endif

#ifdef DEBUG
    ASSERT(IsSimpleDialog(MAKEINTRESOURCE(DLG_WIZARD)));
    ASSERT(IsSimpleDialog(MAKEINTRESOURCE(DLG_PROPSHEET)));
    CheckResourceLanguages();
#endif

    // BUGBUG: only do this for GetProcessVersion apps <= 0x40000
    // Newer apps MUST use InitCommonControlsEx.
    icce.dwSize = sizeof(icce);
    icce.dwICC = ICC_WIN95_CLASSES;


    return InitCommonControlsEx(&icce);
}



void _ProcessDetach(HANDLE hInstance)
{
    //
    // Cleanup cached DCs. No need to synchronize the following section of
    // code since it is only called in DLL_PROCESS_DETACH which is 
    // synchronized by the OS Loader.
    //
#ifdef WINNT
    if (g_hdc)
        DeleteDC(g_hdc);

    if (g_hdcMask)
        DeleteDC(g_hdcMask);

    g_hdc = g_hdcMask = NULL;
#endif

#ifdef WINNT
    UnregisterClasses();
    DeleteCriticalSection(&g_csDll);
#else
    ENTERCRITICAL;
#ifdef FONT_LINK
    DeinitMLANG(g_cProcesses);
#endif
    DeInitPUI(g_cProcesses);
    if (--g_cProcesses == 0) 
    {
        if (g_hdc)
            DeleteDC(g_hdc);

       if (g_hdcMask)
            DeleteDC(g_hdcMask);

        g_hdc = g_hdcMask = NULL;
        
        Mem_Terminate();    // shared heap cleanup... all calls after this will die!
    }
    LEAVECRITICAL;
#endif
}


STDAPI_(BOOL) LibMain(HANDLE hDll, DWORD dwReason, LPVOID pv)
{
#ifndef WINNT
    STDAPI_(BOOL) Cctl1632_ThunkConnect32(LPCSTR pszDll16,LPCSTR pszDll32,HANDLE hIinst,DWORD dwReason);

    if (!Cctl1632_ThunkConnect32("commctrl.dll", "comctl32.dll", hDll, dwReason))
        return FALSE;
#endif

    switch(dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hDll);
        return _ProcessAttach(hDll);

    case DLL_PROCESS_DETACH:
        _ProcessDetach(hDll);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;

    } // end switch()

    return TRUE;

} // end DllEntryPoint()


/* Stub function to call if all you want to do is make sure this DLL is loaded
 */
void WINAPI InitCommonControls(void)
{
}

#ifdef WINNT
BOOL InitForWinlogon(HINSTANCE hInstance)
{
    //  Some people like to use comctl32 from inside winlogon, and
    //  for C2 security reasons, all global atoms are nuked from the
    //  window station when you log off.
    //
    //  So the rule is that all winlogon clients of comctl32 must
    //  call InitCommonControlsEx(ICC_WINLOGON_REINIT) immediately
    //  before doing any common control things (creating windows
    //  or property sheets/wizards) from winlogon.

    ATOM a = GlobalAddAtom(c_szCC32Subclass);
    if (a)
        g_aCC32Subclass = a;

    InitGlobalMetrics(0);
    InitGlobalColors();

    return TRUE;
}
#endif

/* InitCommonControlsEx creates the classes. Only those classes requested are created!
** The process attach figures out if it's an old app and supplies ICC_WIN95_CLASSES.
*/
typedef BOOL (PASCAL *PFNINIT)(HINSTANCE);
typedef struct {
    PFNINIT pfnInit;
#ifdef WINNT
    LPCTSTR pszName;
#endif
    DWORD dw;
} INITCOMMONCONTROLSINFO;

#ifdef WINNT
#define MAKEICC(pfnInit, pszClass, dwFlags) { pfnInit, pszClass, dwFlags }
#else
#define MAKEICC(pfnInit, pszClass, dwFlags) { pfnInit,           dwFlags }
#endif

const INITCOMMONCONTROLSINFO icc[] =
{
     // Init function      Class name         Requested class sets which use this class
MAKEICC(InitToolbarClass,  TOOLBARCLASSNAME,  ICC_BAR_CLASSES),
MAKEICC(InitReBarClass,    REBARCLASSNAME,    ICC_COOL_CLASSES),
MAKEICC(InitToolTipsClass, TOOLTIPS_CLASS,    ICC_TREEVIEW_CLASSES|ICC_BAR_CLASSES|ICC_TAB_CLASSES),
MAKEICC(InitStatusClass,   STATUSCLASSNAME,   ICC_BAR_CLASSES),
MAKEICC(ListView_Init,     WC_LISTVIEW,       ICC_LISTVIEW_CLASSES),
MAKEICC(Header_Init,       WC_HEADER,         ICC_LISTVIEW_CLASSES),
MAKEICC(Tab_Init,          WC_TABCONTROL,     ICC_TAB_CLASSES),
MAKEICC(TV_Init,           WC_TREEVIEW,       ICC_TREEVIEW_CLASSES),
MAKEICC(InitTrackBar,      TRACKBAR_CLASS,    ICC_BAR_CLASSES),
MAKEICC(InitUpDownClass,   UPDOWN_CLASS,      ICC_UPDOWN_CLASS),
MAKEICC(InitProgressClass, PROGRESS_CLASS,    ICC_PROGRESS_CLASS),
MAKEICC(InitHotKeyClass,   HOTKEY_CLASS,      ICC_HOTKEY_CLASS),
MAKEICC(InitAnimateClass,  ANIMATE_CLASS,     ICC_ANIMATE_CLASS),
MAKEICC(InitDateClasses,   DATETIMEPICK_CLASS,ICC_DATE_CLASSES),
MAKEICC(InitComboExClass,  WC_COMBOBOXEX,     ICC_USEREX_CLASSES),
MAKEICC(InitIPAddr,        WC_IPADDRESS,      ICC_INTERNET_CLASSES),
#ifdef PAGER
MAKEICC(InitPager,         WC_PAGESCROLLER,   ICC_PAGESCROLLER_CLASS),
#endif
MAKEICC(InitNativeFontCtl, WC_NATIVEFONTCTL,  ICC_NATIVEFNTCTL_CLASS),

//
//  These aren't really classes.  They're just goofy flags.
//
#ifdef WINNT
MAKEICC(InitForWinlogon,   NULL,              ICC_WINLOGON_REINIT),
#endif
};

BOOL WINAPI InitCommonControlsEx(LPINITCOMMONCONTROLSEX picce)
{
    int i;

#ifdef UNIX
    if (MwIsInitLite())
        return (TRUE);
#endif

    if (!picce ||
        (picce->dwSize != sizeof(INITCOMMONCONTROLSEX)) ||
        (picce->dwICC & ~ICC_ALL_VALID))
    {
        DebugMsg(DM_WARNING, TEXT("comctl32 - picce is bad"));
        return(FALSE);
    }

    for (i=0 ; i < ARRAYSIZE(icc) ; i++)
        if (picce->dwICC & icc[i].dw)
            if (!icc[i].pfnInit(HINST_THISDLL))
                return(FALSE);

    return(TRUE);
}
//
// InitMUILanguage / GetMUILanguage implementation
//
// we have a per process PUI language setting. For NT it's just a global
// initialized with LANG_NEUTRAL and SUBLANG_NEUTRAL
// For Win95 it's DPA slot for the current process.
// InitMUILanguage sets callers preferred language id for common control
// GetMUILangauge returns what the caller has set to us 
// 
#ifdef WINNT
LANGID PUIGetLangId(void)
{
    return g_PUILangId;
}
#else // WIN95
typedef struct tagPUIPROCSLOT
{
    DWORD dwPID;
    LANGID wLangId;
} PUIPROCSLOT, *PPUIPROCSLOT;

PPUIPROCSLOT PUICreateProcSlot(void)
{
    PPUIPROCSLOT pSlot = (PPUIPROCSLOT)Alloc(sizeof(PUIPROCSLOT));

    if (pSlot)
    {
        pSlot->dwPID  = GetCurrentProcessId();
        pSlot->wLangId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    }
    return pSlot;
}

int PUIGetProcIdx(DWORD dwProcessId)
{
    int i, cSlot = 0;

    ASSERTCRITICAL;

    if (g_hdpaPUI)
        cSlot = DPA_GetPtrCount(g_hdpaPUI);

    for (i = 0; i < cSlot; i++)
    {
        PPUIPROCSLOT pSlot = (PPUIPROCSLOT)DPA_FastGetPtr(g_hdpaPUI, i);

        if (pSlot && pSlot->dwPID == dwProcessId)
            return i;
    }
    return -1;
}

void InitPUI(void)
{
    if (NULL == g_hdpaPUI)
    {
        ENTERCRITICAL;
        if (NULL == g_hdpaPUI)
            g_hdpaPUI= DPA_Create(4);
        LEAVECRITICAL;
    }
}

void DeInitPUI(int cProcesses)
{
    int i = PUIGetProcIdx(GetCurrentProcessId());

    ASSERTCRITICAL;

    if (0 <= i)
    {
        Free((PPUIPROCSLOT)DPA_FastGetPtr(g_hdpaPUI, i));
        DPA_DeletePtr(g_hdpaPUI, i);
    }

    if (g_hdpaPUI&& 1 == cProcesses) // This is last process detach
    {
        DPA_Destroy(g_hdpaPUI);
        g_hdpaPUI= NULL;
    }
}

PPUIPROCSLOT PUIGetProcSlot(void)
{
    PPUIPROCSLOT pSlot = NULL;
    int i;

    ENTERCRITICAL;
    i = PUIGetProcIdx(GetCurrentProcessId());

    if (0 <= i)
    {
        pSlot = (PPUIPROCSLOT)DPA_FastGetPtr(g_hdpaPUI, i);
    }
    else
    {
        pSlot = PUICreateProcSlot();
        if (pSlot)
            DPA_AppendPtr(g_hdpaPUI, pSlot);
    }

    LEAVECRITICAL;

    return pSlot;
}

LANGID PUIGetLangId(void)
{
    PPUIPROCSLOT pSlot = PUIGetProcSlot();
    LANGID wLang;
    if (pSlot)
        wLang = pSlot->wLangId;
    else
        wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    return wLang;
}
#endif // !WINNT

void WINAPI
InitMUILanguage(LANGID wLang)
{
#ifdef WINNT
    ENTERCRITICAL;
    g_PUILangId = wLang;
    LEAVECRITICAL;
#else
    PPUIPROCSLOT pSlot = PUIGetProcSlot();
    if(pSlot)
        pSlot->wLangId = wLang;
#endif
}
LANGID WINAPI
GetMUILanguage(void)
{
#ifdef WINNT
    return g_PUILangId;
#else
    return PUIGetLangId();
#endif
}
// end MUI functions

#ifdef WINNT
//
//  Unlike Win9x, WinNT does not automatically unregister classes
//  when a DLL unloads.  We have to do it manually.  Leaving the
//  class lying around means that if an app loads our DLL, then
//  unloads it, then reloads it at a different address, all our
//  leftover RegisterClass()es will point the WndProc at the wrong
//  place and we fault at the next CreateWindow().
//
//  This is not purely theoretical - NT4/FE hit this bug.
//
void UnregisterClasses()
{
    WNDCLASS wc;
    int i;

    for (i=0 ; i < ARRAYSIZE(icc) ; i++)
    {
        if (icc[i].pszName &&
            GetClassInfo(HINST_THISDLL, icc[i].pszName, &wc))
        {
            UnregisterClass(icc[i].pszName, HINST_THISDLL);
        }
    }
}
#endif

#if defined(DEBUG)
LRESULT WINAPI SendMessageD(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    ASSERTNONCRITICAL;
#ifdef UNICODE
    return SendMessageW(hWnd, Msg, wParam, lParam);
#else
    return SendMessageA(hWnd, Msg, wParam, lParam);
#endif
}
#endif // defined(DEBUG)

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
