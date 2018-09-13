/***************************************************************************
 *  msctls.c
 *
 *  Utils library initialization code
 *
 ***************************************************************************/

#include "ctlspriv.h"

#ifndef WIN32
#pragma code_seg(CODESEG_INIT)
#endif

HINSTANCE g_hinst;
int g_cProcesses = 0;

#ifdef IEWIN31_25
ATOM g_aCC32Subclass = 0;
#endif

#ifdef WIN32

CRITICAL_SECTION g_csControls = {{0},0, 0, NULL, NULL, 0 };

#ifdef DEBUG
int   g_CriticalSectionCount=0;
DWORD g_CriticalSectionOwner=0;
#endif

#endif // WIN32


BOOL FAR PASCAL InitAnimateClass(HINSTANCE hInstance);
BOOL ListView_Init(HINSTANCE hinst);
BOOL TV_Init(HINSTANCE hinst);
BOOL FAR PASCAL Header_Init(HINSTANCE hinst);
BOOL FAR PASCAL Tab_Init(HINSTANCE hinst);
void Mem_Terminate();

int PASCAL _ProcessAttach(HANDLE hInstance)
{
#ifdef WIN32
    BOOL fSuccess = TRUE;
#endif

    g_hinst = hInstance;

#ifdef WIN32

    ReinitializeCriticalSection(&g_csControls);

    g_cProcesses++;

    DebugMsg(DM_TRACE, "commctrl:ProcessAttach: %d", g_cProcesses);

#endif

    InitGlobalMetrics(0);
    InitGlobalColors();
    LoadString(HINST_THISDLL, IDS_ELLIPSES, (char FAR*)c_szEllipses, sizeof(c_szEllipses));

#ifndef WIN31   // WIN31 wants the tab control and the updown
    if (!InitToolbarClass(HINST_THISDLL))
        return(0);

    if (!InitToolTipsClass(HINST_THISDLL))
        return(0);

    if (!InitStatusClass(HINST_THISDLL))
        return(0);

#endif //!WIN31

#if !defined( WIN31 ) || defined( IEWIN31_25 )

    if (!ListView_Init(HINST_THISDLL))
        return 0;

    if (!Header_Init(HINST_THISDLL))
        return 0;

    // IEWIN31_25 uses toolbars and tooltips
    if (!InitToolbarClass(HINST_THISDLL))
        return(0);

    if (!InitToolTipsClass(HINST_THISDLL))
        return(0);

#endif //!WIN31 || IEWIN31_25

    if (!Tab_Init(HINST_THISDLL))
        return 0;

#if !defined( WIN31 ) || defined( IEWIN31_25 )
    if (!TV_Init(HINST_THISDLL))
        return 0;
#endif

#ifndef WIN31
#ifndef WIN32

#ifdef WANT_SUCKY_HEADER
    if (!InitHeaderClass(HINST_THISDLL))
        return(0);
#endif

    if (!InitButtonListBoxClass(HINST_THISDLL))
        return(0);

#endif //Win32

    if (!InitTrackBar(HINST_THISDLL))
        return(0);
#endif // !WIN31

    if (!InitUpDownClass(HINST_THISDLL))
#ifndef WIN31
        return(0);
#else
    {
        WNDCLASS wc;
        // Check if already registered by old commctrl
        if (!GetClassInfo(GetModuleHandle("COMMCTRL"),s_szUpdownClass,&wc))
            return(0);
    }
#endif


#ifndef WIN31
    if (!InitProgressClass(HINST_THISDLL))
        return(0);

    if (!InitHotKeyClass(HINST_THISDLL))
        return(0);
#else // !WIN31
#ifdef IEWIN31
    if (!InitProgressClass(HINST_THISDLL))
        return(0);

#endif
#endif


#ifdef WIN32
    if (!InitAnimateClass(HINST_THISDLL))
        return 0;
#endif

    return 1;  /* success */
}



void NEAR PASCAL _ProcessDetach(HANDLE hInstance)
{
    // BUGBUG serialize
    ENTERCRITICAL
    if (--g_cProcesses == 0) {
        // terminate shared data

        //  Mem_Terminate must be called after all other termination routines
        Mem_Terminate();
    }
    LEAVECRITICAL;
}

#pragma data_seg(DATASEG_READONLY)
char const c_szCommCtrlDll[] = "commctrl.dll";
char const c_szComCtl32Dll[] = "comctl32.dll";
#pragma data_seg()

#ifdef WIN32

BOOL WINAPI Cctl1632_ThunkConnect32(LPCSTR pszDll16,LPCSTR pszDll32,HANDLE hIinst,DWORD dwReason);


BOOL APIENTRY LibMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (!Cctl1632_ThunkConnect32(c_szCommCtrlDll, c_szComCtl32Dll, hDll, dwReason))
        return FALSE;

    switch(dwReason) {
    case DLL_PROCESS_ATTACH:
    DisableThreadLibraryCalls(hDll);
        _ProcessAttach(hDll);
        break;

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

void Controls_EnterCriticalSection(void)
{
    EnterCriticalSection(&g_csControls);
#ifdef DEBUG
    if (g_CriticalSectionCount++ == 0)
        g_CriticalSectionOwner = GetCurrentThreadId();
#endif
}

void Controls_LeaveCriticalSection(void)
{
#ifdef DEBUG
    if (--g_CriticalSectionCount == 0)
        g_CriticalSectionOwner = 0;
#endif
    LeaveCriticalSection(&g_csControls);
}

#else
int FAR PASCAL LibMain(HANDLE hInstance, WORD wDataSeg, WORD wcbHeapSize, LPSTR lpstrCmdLine)
{
    _ProcessAttach(hInstance);
    return TRUE;
}

/*  WEP
 *  Windows Exit Procedure
 */

#ifdef WIN31
int FAR PASCAL _loadds WEP(int nParameter)
#else
int FAR PASCAL WEP(int nParameter)
#endif
{

#ifdef WIN31
    DestroyGlobalColors();
#endif

  return 1;
}
#endif


/* Stub function to call if all you want to do is make sure this DLL is loaded
 */
void WINAPI InitCommonControls(void)
{
}

#ifndef WIN32

#ifndef WIN31

BOOL FAR PASCAL Cctl1632_ThunkConnect16(LPCSTR pszDll16, LPCSTR pszDll32, WORD hInst, DWORD dwReason);

BOOL FAR PASCAL DllEntryPoint(DWORD dwReason, WORD  hInst, WORD  wDS, WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
    if (!(Cctl1632_ThunkConnect16(c_szCommCtrlDll, c_szComCtl32Dll, hInst, dwReason)))
        return FALSE;
    return TRUE;
}

#endif // WIN31

#endif // WIN32

#ifdef IEWIN31_25
/* InitCommonControlsEx creates the classes. Only those classes requested are created!
** The process attach figures out if it's an old app and supplies ICC_WIN95_CLASSES.
*/
#ifndef WINNT
#pragma data_seg(DATASEG_READONLY)
#endif
typedef BOOL (PASCAL *PFNINIT)(HINST);
struct {PFNINIT pfnInit; DWORD dw;} icc[] =
{
    // Init function    Requested class sets which use this class
    {InitToolbarClass,  ICC_BAR_CLASSES},
    {InitReBarClass,    ICC_COOL_CLASSES},
    {InitToolTipsClass, ICC_TREEVIEW_CLASSES|ICC_BAR_CLASSES|ICC_TAB_CLASSES},
//    {InitStatusClass,   ICC_BAR_CLASSES},
    {ListView_Init,     ICC_LISTVIEW_CLASSES},
    {Header_Init,       ICC_LISTVIEW_CLASSES},
    {Tab_Init,          ICC_TAB_CLASSES},
    {TV_Init,           ICC_TREEVIEW_CLASSES},
    {InitTrackBar,      ICC_BAR_CLASSES},
    {InitUpDownClass,   ICC_UPDOWN_CLASS},
    {InitProgressClass, ICC_PROGRESS_CLASS},
//    {InitHotKeyClass,   ICC_HOTKEY_CLASS},
//    {InitAnimateClass,  ICC_ANIMATE_CLASS},
//    {InitDateClasses,   ICC_DATE_CLASSES},
//    {InitComboExClass,  ICC_USEREX_CLASSES}
};
#ifndef WINNT
#pragma data_seg()
#endif

BOOL WINAPI InitCommonControlsEx(LPINITCOMMONCONTROLSEX picce)
{
    int i;

    if (!picce ||
        (picce->dwSize != sizeof(INITCOMMONCONTROLSEX)) ||
        (picce->dwICC & ~ICC_ALL_CLASSES))
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
#endif  //IEWIN31_25

#if defined(WIN32) && defined(DEBUG)
LRESULT
WINAPI
SendMessageD(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    ASSERTNONCRITICAL;
    return SendMessageA(hWnd, Msg, wParam, lParam);
}
#endif // defined(WIN32) && defined(DEBUG)
