#include "shellprv.h"
#pragma  hdrstop

#ifdef WINNT
// NT APM support
#include <ntddapmt.h>
#else
// Win95 APM support
#include <vmm.h>
#include <bios.h>
#define NOPOWERSTATUSDEFINES
#include <pwrioctl.h>
#include <wshioctl.h>
#include <dbt.h>
#include <pbt.h>

#define Not_VxD
#define No_CM_Calls
#undef LODWORD
#undef HIDWORD
#include <configmg.h>
#endif

// Username length constant
#include <lmcons.h>

// Hydra functions/constants
#include <winsta.h>


#define DOCKSTATE_DOCKED            0
#define DOCKSTATE_UNDOCKED          1
#define DOCKSTATE_UNKNOWN           2

// BUGBUG:: This will be pif.h...
#ifndef OPENPROPS_FORCEREALMODE
#define OPENPROPS_FORCEREALMODE 0x0004
#endif

void FlushRunDlgMRU(void);
BOOL IsPowerOffAllowed(void);

// in shlexec.c
DWORD SHProcessMessagesUntilEvent(HWND hwnd, HANDLE hEvent, DWORD dwTimeout);

// Disconnect API fn-ptr
typedef BOOLEAN (*PWINSTATION_DISCONNECT) (
                                           HANDLE hServer,
                                           ULONG SessionId,
                                           BOOL bWait
                                           );


// thunk to 16 bit code to do this
//
STDAPI_(BOOL) SHRestartWindows(DWORD dwReturn);

#define ROP_DPna        0x000A0329

#ifndef WINNT // {
// BUGBUG perf(space): change to just use an ascii-encoded DWORD (itoa/atoi)

STDAPI_(void) SHExitWindowsEx(HWND hwndParent, HINSTANCE hinstEXE, LPSTR pszCmdLine, int nCmdShow)
{
    DWORD dwExitWinCode = 0;

    StrToIntEx(pszCmdLine, STIF_SUPPORT_HEX, &dwExitWinCode);

    TraceMsg(DM_TRACE, "s.SHewe: call ExitWindowsEx(0x%x)", dwExitWinCode);

    // The hwndParent needs to be destroyed. If not, one of the BroadcastSystemMessages that 
    // occurs when the network drives are dismounted, will cause a dead-lock during the 
    // ExitWindowsEx() call below.
    // This is a fix for a QFE issue.
    DestroyWindow(hwndParent);
    
    ExitWindowsEx(dwExitWinCode, 0);
}

//***   WaitExitWindowsProc -- wait for ExitWindows process to complete
// ENTRY/EXIT
//  returns     TRUE if ExitWindows completed, FALSE if cancelled (or failed)
DWORD WaitExitWindowsProc(HANDLE hEvent, DWORD dwTimeout)
{
    MSG msg;
    DWORD dwObject;
    TraceMsg(DM_TRACE, "s.mwfmo: waiting for RunDll...");
    while (1)
    {
        dwObject = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwTimeout, QS_ALLINPUT);
        // Are we done waiting?
        switch (dwObject) {
        case WAIT_OBJECT_0:         // process returned
        case WAIT_FAILED:
            return FALSE;           // ExitWindows/WM_ENDSESSION *canceled*

        case WAIT_OBJECT_0 + 1:     // got some input
            // Almost.
            TraceMsg(DM_TRACE, "s.mwfmo: almost done waiting");
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_ENDSESSION) {
                    TraceMsg(DM_TRACE, "s.mwfmo: WM_ENDSESSION wP=%d ret wP", msg.wParam);
                    return msg.wParam;      /* tell caller whether session is ending */
                }
            }
            TraceMsg(DM_TRACE, "s.mwfmo: !PeekMsg (!)");

            break;
        }
    }
    // never gets here
    // return dwObject;
}
#endif // }

//***   DoExitWindowsEx -- exit windows w/ some OS-dependent hacks
// NOTES
//  on win95 any files held open by the process that calls ExitWindows can't
//  be roamed, because the process is still running when we try to copy the
//  files.
//  on win95 ExitWindows doesn't notify the calling thread (process?).
//  to avoid both of these pblms, we spawn a worker process to do the call.
//
BOOL DoExitWindowsEx(DWORD dwExitWinCode, DWORD dwReserved)
{
    BOOL fOk;
#ifdef WINNT
/*NOTHING*/
#else
    char szCmdLine[MAX_PATH];
    SHELLEXECUTEINFO ei;
    DWORD dwValue, cbSize;
#endif

    ASSERT(dwReserved == 0);
#ifdef WINNT
    fOk = ExitWindowsEx(dwExitWinCode, dwReserved);
#else
    cbSize = SIZEOF(dwValue);
    if (NO_ERROR == SHGetValue(HKEY_LOCAL_MACHINE,
      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
      TEXT("NoExecExitWindows"), NULL, &dwValue, &cbSize) &&
      dwValue == 1) {
        TraceMsg(DM_WARNING, "s.dewe: NoExecExitWindows=%d skip spawn", dwValue);
        goto Lcall;
    }

    wsprintf(szCmdLine, TEXT("shell32.dll,SHExitWindowsEx 0x%x"), dwExitWinCode);
#ifdef DEBUG
    {
        #define OFF_0x  28
        DWORD dwTmp = (DWORD)-1;

        ASSERT(szCmdLine[OFF_0x] == TEXT('0') && szCmdLine[OFF_0x + 1] == TEXT('x'));
        StrToIntEx(szCmdLine + OFF_0x, STIF_SUPPORT_HEX, &dwTmp);
        ASSERT(dwTmp == dwExitWinCode);

        #undef  OFF_0x
    }
#endif
    TraceMsg(DM_TRACE, "s.dewe: dwExitWin=%x szCmd=%s", dwExitWinCode, szCmdLine);
    ei.cbSize          = sizeof(SHELLEXECUTEINFO);
    ei.hwnd            = NULL;
    ei.lpVerb          = NULL;
    ei.lpFile          = "rundll32.exe";
    ei.lpParameters    = szCmdLine;
    ei.lpDirectory     = NULL;
    ei.nShow           = SW_SHOWNORMAL;
    ei.fMask           = (SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI);
    if (ShellExecuteEx(&ei) && (ei.hProcess != NULL)) {
        fOk = WaitExitWindowsProc(ei.hProcess, INFINITE);
        CloseHandle(ei.hProcess);
        TraceMsg(DM_TRACE, "s.dewe: back from wait (?)");
    }
    else {
        TraceMsg(DM_WARNING, "s.dewe: spawn failed");
Lcall:
        fOk = ExitWindowsEx(dwExitWinCode, dwReserved);
    }
#endif
    return fOk;
}

// Process all of the strange ExitWindowsEx codes and privileges.
//
STDAPI_(BOOL) CommonRestart(DWORD dwExitWinCode)
{
    BOOL fOk, bOkToPowerOff = FALSE;
    DWORD dwExtraExitCode = 0;
#ifdef WINNT
    DWORD OldState, Status;
    DWORD dwErrorSave;
#endif

    DebugMsg(DM_TRACE, TEXT("CommonRestart(0x%x)"), dwExitWinCode);

    if ((dwExitWinCode == EWX_SHUTDOWN) && IsPowerOffAllowed())
    {
        dwExtraExitCode = EWX_POWEROFF;
    }


#ifdef WINNT
    SetLastError(0);        // Be really safe about last error value!

    Status = SetPrivilegeAttribute(SE_SHUTDOWN_NAME,
                                   SE_PRIVILEGE_ENABLED,
                                   &OldState);
    dwErrorSave = GetLastError();       // ERROR_NOT_ALL_ASSIGNED sometimes
#endif

    switch (dwExitWinCode) {
    case EWX_SHUTDOWN:
    case EWX_REBOOT:
    case EWX_LOGOFF:

        if (GetKeyState(VK_CONTROL) < 0)
        {
            dwExtraExitCode |= EWX_FORCE;
        }

        fOk = DoExitWindowsEx(dwExitWinCode|dwExtraExitCode, 0);
        break;

    default:

        fOk = SHRestartWindows(dwExitWinCode);
        break;
    }

#ifdef WINNT
    //
    // If we were able to set the privilege, then reset it.
    //
    if (NT_SUCCESS(Status) && dwErrorSave == 0)
    {
        SetPrivilegeAttribute(SE_SHUTDOWN_NAME, OldState, NULL);
    }
    else
    {
        //
        // Otherwise, if we failed, then it must have been some
        // security stuff.
        //
        if (!fOk)
        {
            ShellMessageBox(HINST_THISDLL, NULL,
                            dwExitWinCode == EWX_SHUTDOWN ?
                             MAKEINTRESOURCE(IDS_NO_PERMISSION_SHUTDOWN) :
                             MAKEINTRESOURCE(IDS_NO_PERMISSION_RESTART),
                            dwExitWinCode == EWX_SHUTDOWN ?
                             MAKEINTRESOURCE(IDS_SHUTDOWN) :
                             MAKEINTRESOURCE(IDS_RESTART),
                            MB_OK | MB_ICONSTOP);
        }
    }
#endif

    DebugMsg(DM_TRACE, TEXT("CommonRestart done"));

    return fOk;
}

void EarlySaveSomeShellState()
{
	// We flush two MRU's here (RecentMRU and RunDlgMRU).
	// Note that they won't flush if there is any reference count.
	
	FlushRunDlgMRU();
	_IconCacheSave();	
}


/* Display a dialog asking the user to restart Windows, with a button that
** will do it for them if possible.
*/
STDAPI_(int) RestartDialog(HWND hParent, LPCTSTR lpPrompt, DWORD dwReturn)
{
    UINT id;
    LPCTSTR pszMsg;

	EarlySaveSomeShellState();

    if (lpPrompt && *lpPrompt == TEXT('#'))
    {
        pszMsg = lpPrompt + 1;
    }
    else if (dwReturn == EWX_SHUTDOWN)
    {
        pszMsg = MAKEINTRESOURCE(IDS_RSDLG_SHUTDOWN);
    }
    else
    {
        pszMsg = MAKEINTRESOURCE(IDS_RSDLG_RESTART);
    }

    id = ShellMessageBox(HINST_THISDLL, hParent, pszMsg, MAKEINTRESOURCE(IDS_RSDLG_TITLE),
                MB_YESNO | MB_ICONQUESTION, lpPrompt ? lpPrompt : c_szNULL);

    if (id == IDYES)
    {
        CommonRestart(dwReturn);
    }
    return id;
}

//---------------------------------------------------------------------------
const WORD c_GrayBits[] = {0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA};

HBRUSH CreateDitheredBrush(void)
{
    HBITMAP hbmp = CreateBitmap(8, 8, 1, 1, c_GrayBits);
    if (hbmp)
    {
        HBRUSH hbr = CreatePatternBrush(hbmp);
        DeleteObject(hbmp);
        return hbr;
    }
    return NULL;
}


// ---------------------------------------------------------------------------
LRESULT CALLBACK FakeDesktopWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_CREATE:
        return TRUE;
    case WM_NCPAINT:
        return 0;
    case WM_ACTIVATE:
        DebugMsg(DM_TRACE, TEXT("FakeWndProc; WM_ACTIVATE %d hwnd=%d"), LOWORD(wparam), lparam);
        if (LOWORD(wparam) == WA_INACTIVE)
        {
            if (lparam == 0 || GetWindow((HWND)lparam,GW_OWNER) != hwnd)
            {
    // Also kill ourselves if the user clicks on us or types at us
    // This is important because there might be a dialog box
    // the user is trying to get to which we are accidentally covering
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
                DebugMsg(DM_TRACE, TEXT("FakeWndProc: death"));
                ShowWindow(hwnd, SW_HIDE);
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            return 0;

        }
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

TCHAR const c_szFakeDesktopClass[] = TEXT("FakeDesktopWClass");

#if defined(WINNT) && defined(DEBUG)
#define UnderDebugger() IsDebuggerPresent()
#else
#define UnderDebugger() FALSE
#endif

// ---------------------------------------------------------------------------
// Create a topmost window that sits on top of the desktop but which
// ignores WM_ERASEBKGND and never paints itself.
HWND CreateFakeDesktopWindow(void)
{
    BOOL fScreenReader;

    // Bad things will happen if we try to do this multiple times...
    if (FindWindow(c_szFakeDesktopClass, NULL))
    {
        DebugMsg(DM_ERROR, TEXT("s.cfdw: Shutdown desktop already exists."));
        
        return NULL;
    }
    else if (UnderDebugger() || IsRemoteSession() || 
        (SystemParametersInfo(SPI_GETSCREENREADER, 0, &fScreenReader, 0) && fScreenReader))
    {
        // Don't create the window; a debugger is attached or we are running
        // a terminal server session.
        return NULL;
    }
    else
    {
        WNDCLASS wc;
        POINT ptDesktop =
            {GetSystemMetrics(SM_XVIRTUALSCREEN),
            GetSystemMetrics(SM_YVIRTUALSCREEN)};
        SIZE sDesktop =
            {GetSystemMetrics(SM_CXVIRTUALSCREEN),
            GetSystemMetrics(SM_CYVIRTUALSCREEN)};

        // for pre-Nashville platforms
        if (!sDesktop.cx || !sDesktop.cy)
        {
            sDesktop.cx = GetSystemMetrics(SM_CXSCREEN);
            sDesktop.cy = GetSystemMetrics(SM_CYSCREEN);
        }

        wc.style = 0;
        wc.lpfnWndProc = FakeDesktopWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = HINST_THISDLL;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW) ;
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = c_szFakeDesktopClass;

        // don't really care if this is already registered...

        RegisterClass(&wc);
        return CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                c_szFakeDesktopClass, NULL, WS_POPUP,
                ptDesktop.x, ptDesktop.y, sDesktop.cx, sDesktop.cy,
                NULL, NULL, HINST_THISDLL, NULL);
    }
}

// ---------------------------------------------------------------------------
void DitherWindow(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    if (hdc)
    {
        HBRUSH hbr = CreateDitheredBrush();
        if (hbr)
        {
            RECT rc;
            HBRUSH hbrOld = SelectObject(hdc, hbr);

            GetClientRect(hwnd, &rc);
            PatBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, ROP_DPna);
            SelectObject(hdc, hbrOld);
            DeleteObject(hbr);
        }
        ReleaseDC(hwnd, hdc);
    }
}

// ---------------------------------------------------------------------------

/* Check if system is currently in a docking station.
 *
 * Returns: TRUE if docked, FALSE if undocked or can't tell.
 *
 */

BOOL GetDockedState(void)
{
#ifndef WINNT                       // BUGBUG - Fix this when NT gets docking capability
    struct _ghwpi {                 // Get_Hardware_Profile_Info parameter blk
        CMAPI   cmApi;
        ULONG   ulIndex;
        PFARHWPROFILEINFO pHWProfileInfo;
        ULONG   ulFlags;
        HWPROFILEINFO HWProfileInfo;
    } *pghwpi;

    HANDLE hCMHeap;
    UINT Result = DOCKSTATE_UNKNOWN;
    DWORD dwRecipients = BSM_VXDS;

#define HEAP_SHARED     0x04000000      /* put heap in shared memory--undoc'd */

    // Create a shared heap for CONFIGMG parameters

    if ((hCMHeap = HeapCreate(HEAP_SHARED, 1, 4096)) == NULL)
        return DOCKSTATE_UNKNOWN;

#undef HEAP_SHARED

    // Allocate parameter block in shared memory

    pghwpi = (struct _ghwpi *)HeapAlloc(hCMHeap, HEAP_ZERO_MEMORY,
                                        SIZEOF(*pghwpi));
    if (pghwpi == NULL)
    {
        HeapDestroy(hCMHeap);
        return DOCKSTATE_UNKNOWN;
    }

    pghwpi->cmApi.dwCMAPIRet     = 0;
    pghwpi->cmApi.dwCMAPIService = GetVxDServiceOrdinal(_CONFIGMG_Get_Hardware_Profile_Info);
    pghwpi->cmApi.pCMAPIStack    = (DWORD)(((LPBYTE)pghwpi) + SIZEOF(pghwpi->cmApi));
    pghwpi->ulIndex              = 0xFFFFFFFF;
    pghwpi->pHWProfileInfo       = &pghwpi->HWProfileInfo;
    pghwpi->ulFlags              = 0;

    // "Call" _CONFIGMG_Get_Hardware_Profile_Info service

    BroadcastSystemMessage(0, &dwRecipients, WM_DEVICECHANGE, DBT_CONFIGMGAPI32,
                           (LPARAM)pghwpi);

    if (pghwpi->cmApi.dwCMAPIRet == CR_SUCCESS) {

        switch (pghwpi->HWProfileInfo.HWPI_dwFlags) {

            case CM_HWPI_DOCKED:
                Result = DOCKSTATE_DOCKED;
                break;

            case CM_HWPI_UNDOCKED:
                Result = DOCKSTATE_UNDOCKED;
                break;

            default:
                Result = DOCKSTATE_UNKNOWN;
                break;

        }

    }

    HeapDestroy(hCMHeap);

    return Result;
#else
    return(DOCKSTATE_DOCKED);
#endif
}


const TCHAR c_szREGSTR_ROOT_APM[] = REGSTR_KEY_ENUM TEXT("\\") REGSTR_KEY_ROOTENUM TEXT("\\") REGSTR_KEY_APM TEXT("\\") REGSTR_DEFAULT_INSTANCE;
const TCHAR c_szREGSTR_BIOS_APM[] = REGSTR_KEY_ENUM TEXT("\\") REGSTR_KEY_BIOSENUM TEXT("\\") REGSTR_KEY_APM;
const TCHAR c_szREGSTR_VAL_APMMENUSUSPEND[] = REGSTR_VAL_APMMENUSUSPEND;

/* Open the registry APM device key
 */

BOOL OpenAPMKey(HKEY *phKey)
{
    HKEY hBiosSys;
    BOOL rc = FALSE;
    TCHAR szInst[MAX_PATH+1];
    DWORD cchInst = ARRAYSIZE(szInst);

    // Open HKLM\Enum\Root\*PNP0C05\0000 - This is the APM key for
    // non-PnP BIOS machines.

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szREGSTR_ROOT_APM, phKey) == ERROR_SUCCESS)
        return TRUE;

    // Open HKLM\Enum\BIOS\*PNP0C05, Enum the 1st subkey, open that.  Example:
    // HKLM\Enum\BIOS\*PNP0C05\03.

    if (RegOpenKey(HKEY_LOCAL_MACHINE,c_szREGSTR_BIOS_APM,&hBiosSys) == ERROR_SUCCESS)
    {
        if (RegEnumKey(hBiosSys, 0, szInst, cchInst) == ERROR_SUCCESS &&
            RegOpenKey(hBiosSys, szInst, phKey) == ERROR_SUCCESS)
            rc = TRUE;

        RegCloseKey(hBiosSys);
    }

    return rc;
}


#ifndef WINNT
const TCHAR c_szPOWERDevice[] = TEXT("\\\\.\\VPOWERD");
#else
const TCHAR c_szPOWERDevice[] = TEXT("\\\\.\\APMTEST");
#endif


BOOL CheckBIOS (void)
{
    HKEY hkey;
    BOOL fRet = TRUE;
    BOOL fSuspendUndocked = TRUE;


    /* Check the Registry APM key for an APMMenuSuspend value.
     * APMMenuSuspend may have the following values: APMMENUSUSPEND_DISABLED,
     * APMMENUSUSPEND_ENABLED, or APMMENUSUSPEND_UNDOCKED.
     *
     * An APMMenuSuspend value of APMMENUSUSPEND_DISABLED means the
     * tray should never show the Suspend menu item on its menu.
     *
     * APMMENUSUSPEND_ENABLED means the Suspend menu item should be shown
     * if the machine has APM support enabled (VPOWERD is loaded).  This is
     * the default.
     *
     * APMMENUSUSPEND_UNDOCKED means the Suspend menu item should be shown,
     * but only enabled when the machine is not in a docking station.
     *
     */

    if (OpenAPMKey(&hkey))
    {
        BYTE bMenuSuspend = APMMENUSUSPEND_ENABLED;
        DWORD dwType, cbSize = SIZEOF(bMenuSuspend);

        if (SHQueryValueEx(hkey, c_szREGSTR_VAL_APMMENUSUSPEND, 0,
                            &dwType, &bMenuSuspend, &cbSize) == ERROR_SUCCESS)
        {
            bMenuSuspend &= ~(APMMENUSUSPEND_NOCHANGE);     // don't care about nochange flag
            if (bMenuSuspend == APMMENUSUSPEND_UNDOCKED)
                fSuspendUndocked = TRUE;
            else
            {
                fSuspendUndocked = FALSE;

                if (bMenuSuspend == APMMENUSUSPEND_DISABLED)
                    fRet = FALSE;
            }
        }
        RegCloseKey(hkey);
    }

    if (fRet)
    {
        // Disable Suspend menu item if 1) only wanted when undocked and
        // system is currently docked, 2) power mgnt level < advanced

        if (fSuspendUndocked && GetDockedState() == DOCKSTATE_DOCKED)
            fRet = FALSE;
        else
        {
            DWORD dwPmLevel, cbOut;
            BOOL fIoSuccess;
            HANDLE hVPowerD;

            hVPowerD = CreateFile(c_szPOWERDevice,
                                  GENERIC_READ|GENERIC_WRITE,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, NULL);

            if (hVPowerD != INVALID_HANDLE_VALUE) {

                fIoSuccess = DeviceIoControl(hVPowerD,
#ifdef WINNT                   // BUGBUG - why does NT use a different define?
                                             APM_IOCTL_GET_PM_LEVEL,
#else
                                             VPOWERD_IOCTL_GET_PM_LEVEL,
#endif
                                             NULL, 0, &dwPmLevel, SIZEOF(dwPmLevel), &cbOut, NULL);

                fRet = (fIoSuccess && (dwPmLevel == PMLEVEL_ADVANCED));

                CloseHandle (hVPowerD);

            } else {
                fRet = FALSE;
            }
        }
    }

    return fRet;
}

typedef BOOLEAN (*PFNSETSUSPENDSTATE)(BOOLEAN, BOOLEAN, BOOLEAN);

BOOL SuspendComputer (BOOL bHibernate, BOOL bForce, BOOL bDisableWakeup)
{
    PFNSETSUSPENDSTATE pfnSetSuspendState;
    HINSTANCE hInstPowrProf;
    BOOLEAN bResult;
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);


    //
    // See if we are running on one of the 4.0 products.
    //

    // BugBug:  Memphis hasn't implemented all the NT* power mgmt
    // functions yet, so revert back to calling SetSystemPowerState
    // until they're ready.

#if 0
    if ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion == 0))
#else

    if (osvi.dwMajorVersion == 4)
#endif
    {
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            return FALSE;
        }

        return SetSystemPowerState (!bHibernate, bForce);
    }


    //
    // For NT5 and Memphis, use the new power mgmt stuff.
    //

    hInstPowrProf = LoadLibrary(TEXT("POWRPROF.DLL"));

    if (!hInstPowrProf)
    {
        return FALSE;
    }

    pfnSetSuspendState =
        (PFNSETSUSPENDSTATE)GetProcAddress(hInstPowrProf,
                                           "SetSuspendState");
    if (!pfnSetSuspendState) {
        FreeLibrary (hInstPowrProf);
        return FALSE;
    }


    //
    // Call SetSuspendState
    //

    bResult = pfnSetSuspendState((BOOLEAN)bHibernate, (BOOLEAN)bForce,
                                 (BOOLEAN) bDisableWakeup);

    FreeLibrary (hInstPowrProf);

    return (BOOL) bResult;
}


typedef BOOLEAN (*PFNISPWRSUSPENDALLOWED)(VOID);

BOOL QueryPowerInformation (VOID)
{
    PFNISPWRSUSPENDALLOWED pfnIsPwrSuspendAllowed;
    HINSTANCE hInstPowrProf;
    BOOLEAN bResult;


    //
    // Load powrprof.dll
    //

    hInstPowrProf = LoadLibrary(TEXT("POWRPROF.DLL"));

    if (!hInstPowrProf)
    {
        return FALSE;
    }

    pfnIsPwrSuspendAllowed =
        (PFNISPWRSUSPENDALLOWED)GetProcAddress(hInstPowrProf,
                                              "IsPwrSuspendAllowed");
    if (!pfnIsPwrSuspendAllowed) {
        FreeLibrary (hInstPowrProf);
        return FALSE;
    }


    //
    // Call IsPwrSuspendAllowed to query for the capabilities
    //

    bResult = pfnIsPwrSuspendAllowed();

    FreeLibrary (hInstPowrProf);


    return (BOOL) bResult;
}

typedef BOOLEAN (*PFNISPWRHIBERNATEALLOWED)(VOID);

BOOL IsHibernateAllowed(void)
{
    PFNISPWRHIBERNATEALLOWED pfnIsPwrHibernateAllowed;
    HINSTANCE hInstPowrProf;
    BOOLEAN bResult;
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);


    //
    // See if we are running on one of the 4.0 products.
    //

    // bugbug:  Memphis doesn't support calling for hibernate yet
#if 0
    if ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion == 0))
#else
    if (osvi.dwMajorVersion == 4)
#endif
    {
        return FALSE;
    }


    //
    // For NT5 and Memphis, use the new power mgmt stuff.
    //

    hInstPowrProf = LoadLibrary(TEXT("POWRPROF.DLL"));

    if (!hInstPowrProf)
    {
        return FALSE;
    }

    pfnIsPwrHibernateAllowed =
        (PFNISPWRHIBERNATEALLOWED)GetProcAddress(hInstPowrProf,
                                              "IsPwrHibernateAllowed");
    if (!pfnIsPwrHibernateAllowed) {
        FreeLibrary (hInstPowrProf);
        return FALSE;
    }


    //
    // Call IsPwrHibernateAllowed to query for the capabilities
    //

    bResult = pfnIsPwrHibernateAllowed();

    FreeLibrary (hInstPowrProf);

    return (BOOL) bResult;
}


typedef BOOLEAN (*PFNISPWRSHUTDOWNALLOWED)(VOID);

BOOL IsPowerOffAllowed(void)
{
    PFNISPWRSHUTDOWNALLOWED pfnIsPwrShutdownAllowed;
    HINSTANCE hInstPowrProf;
    BOOLEAN bResult;
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);


    //
    // See if we are running on one of the 4.0 products.
    //

    if ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion == 0))
    {
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            if (0 == GetProfileInt(TEXT("WINLOGON"), TEXT("PowerdownAfterShutdown"), 0))
            {
                return FALSE;
            }
        }

        return TRUE;
    }


    //
    // For NT5 and Memphis, use the new power mgmt stuff.
    //

    hInstPowrProf = LoadLibrary(TEXT("POWRPROF.DLL"));

    if (!hInstPowrProf)
    {
        return FALSE;
    }

    pfnIsPwrShutdownAllowed =
        (PFNISPWRSHUTDOWNALLOWED)GetProcAddress(hInstPowrProf,
                                              "IsPwrShutdownAllowed");
    if (!pfnIsPwrShutdownAllowed) {
        FreeLibrary (hInstPowrProf);
        return FALSE;
    }


    //
    // Call IsPwrShutdownAllowed to query for the capabilities
    //

    bResult = pfnIsPwrShutdownAllowed();

    FreeLibrary (hInstPowrProf);

    return (BOOL) bResult;
}


/* Determine if "Suspend" should appear in the shutdown dialog.
 *
 * Returns: TRUE if Suspend should appear, FALSE if not.
 */

STDAPI_(BOOL) IsSuspendAllowed(void)
{
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);


    //
    // See if we are running on one of the 4.0 products.
    //

    if ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion == 0))
    {
        //
        // NT 4 has no power mgmt support.
        //

        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            return FALSE;
        }


        //
        // On Win95, check the BIOS.
        //

        return CheckBIOS();
    }


    //
    // For NT5 and Memphis, use the new power mgmt stuff.
    //

    return QueryPowerInformation();
}


// ---------------------------------------------------------------------------
BOOL _LogoffAvailable()
{
        // If dwStartMenuLogoff is zero, then we remove it.
    BOOL fUpgradeFromIE4 = FALSE;
    BOOL fUserWantsLogoff = FALSE;
    DWORD dwStartMenuLogoff = 0;
    TCHAR sz[MAX_PATH];
    DWORD dwRestriction = SHRestricted(REST_STARTMENULOGOFF);

    DWORD cbData = sizeof(dwStartMenuLogoff);

    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\Advanced"), 
                    TEXT("StartMenuLogoff"), NULL, &dwStartMenuLogoff, &cbData))
    {
        fUserWantsLogoff = (dwStartMenuLogoff != 0);
    }

    cbData = ARRAYSIZE(sz);
    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_EXPLORER TEXT("\\WindowsUpdate"), 
                    TEXT("UpdateURL"), NULL, (DWORD*)sz, &cbData))
    {
        fUpgradeFromIE4 = (sz[0] != TEXT('\0'));
    }

    // Admin is forcing the logoff to be on the menu
    if (dwRestriction == 2)
        return FALSE;

    // The user does wants logoff on the start menu.
    // Or it's an upgrade from IE4
    if ((fUpgradeFromIE4 || fUserWantsLogoff) && dwRestriction != 1)
        return FALSE;

    return TRUE;
}

DWORD GetShutdownOptions()
{
    LONG lResult = ERROR_SUCCESS + 1;
    DWORD dwOptions = SHTDN_SHUTDOWN;

    // No shutdown on terminal server
    if (!IsRemoteSession())
    {
        dwOptions |= SHTDN_RESTART;
    }

    // Add logoff if supported
    if (_LogoffAvailable())
    {
        dwOptions |= SHTDN_LOGOFF;
    }

    // No restart to DOS if not supported (always in NT)
#ifndef WINNT
    // Don't have restart to dos if restricted or in clean boot as it
    // won't work there!
    if (!SHRestricted(REST_NOEXITTODOS) && !GetSystemMetrics(SM_CLEANBOOT))
    {
        dwOptions |= SHTDN_RESTART_DOS;
    }
#endif

    //
    // Add the hibernate option if it's supported.
    //

    if (IsHibernateAllowed())
    {
        dwOptions |= SHTDN_HIBERNATE;
    }

    if (IsSuspendAllowed())
    {
        HKEY hKey;
        DWORD dwAdvSuspend = 0;
        DWORD dwType, dwSize;

        // At least basic sleep is supported
        dwOptions |= SHTDN_SLEEP;

        //
        // Check if we should offer advanced suspend options
        //

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"),
                          0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(dwAdvSuspend);
            SHQueryValueEx (hKey, TEXT("Shutdown"), NULL, &dwType,
                                 (LPBYTE) &dwAdvSuspend, &dwSize);

            RegCloseKey (hKey);
        }


        if (dwAdvSuspend != 0)
        {
            dwOptions |= SHTDN_SLEEP2;
        }
    }

    return dwOptions;
}

BOOL_PTR CALLBACK LogoffWindowsDlgProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static BOOL s_fLogoffDialog = FALSE;
    HICON hIcon;

    switch (msg)
    {
    case WM_INITMENUPOPUP:
        EnableMenuItem((HMENU)wparam, SC_MOVE, MF_BYCOMMAND|MF_GRAYED);
        break;

    case WM_INITDIALOG:
		// We could call them when the user actually selects the shutdown,
		// but I put them here to leave the shutdown process faster.
		//
		EarlySaveSomeShellState();		
		
        s_fLogoffDialog = FALSE;
        hIcon = LoadImage (HINST_THISDLL, MAKEINTRESOURCE(IDI_STLOGOFF),
                           IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);

        if (hIcon)
        {
            SendDlgItemMessage (hdlg, IDD_LOGOFFICON, STM_SETICON, (WPARAM) hIcon, 0);
        }
        return TRUE;

    // Blow off moves (only really needed for 32bit land).
    case WM_SYSCOMMAND:
        if ((wparam & ~0x0F) == SC_MOVE)
            return TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            s_fLogoffDialog = TRUE;
            EndDialog(hdlg, SHTDN_LOGOFF);
            break;

        case IDCANCEL:
            s_fLogoffDialog = TRUE;
            EndDialog(hdlg, SHTDN_NONE);
            break;

        case IDHELP:
            WinHelp(hdlg, TEXT("windows.hlp>proc4"), HELP_CONTEXT, (DWORD) IDH_TRAY_SHUTDOWN_HELP);
            break;
        }
        break;

    case WM_ACTIVATE:
        // If we're loosing the activation for some other reason than
        // the user click OK/CANCEL then bail.
        if (LOWORD(wparam) == WA_INACTIVE && !s_fLogoffDialog)
        {
            s_fLogoffDialog = TRUE;
            EndDialog(hdlg, SHTDN_NONE);
        }
        break;
    }
    return FALSE;
}



#ifdef WINNT

#define ExitToDos(hwnd)

#else

// ---------------------------------------------------------------------------
// Function to try to exit to dos
void ExitToDos(HWND hwnd)
{
    // For now I will assume that the pif is called sdam.pif in the windows
    // directory... If it is not found we should generate it, but for now I will
    // fallback to ExitWindowsExec of command.com...

    TCHAR szPath[MAX_PATH];
    TCHAR szCommand[MAX_PATH];

    GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    LoadString(HINST_THISDLL,  IsLFNDrive(szPath)? IDS_RSDLG_PIFFILENAME : IDS_RSDLG_PIFSHORTFILENAME,
            szCommand, ARRAYSIZE(szCommand));
    PathCombine(szPath, szPath, szCommand);


    if (!PathFileExists(szPath))
    {
        PROPPRG ProgramProps;
        HANDLE hPif;

        lstrcpy(szCommand, TEXT("command.com"));
        PathResolve(szCommand, NULL, 0);
        hPif = PifMgr_OpenProperties(szCommand, szPath, 0, OPENPROPS_INFONLY|OPENPROPS_FORCEREALMODE);
        if (!hPif)
        {

            DebugMsg(DM_TRACE, TEXT("ExitToDos: PifMgr_OpenProperties *failed*"));
            goto Abort;
        }

        if (!PifMgr_GetProperties(hPif, (LPSTR)MAKEINTATOM(GROUP_PRG), &ProgramProps, SIZEOF(ProgramProps), 0))
        {
            DebugMsg(DM_TRACE, TEXT("ExitToDos: PifMgr_GetProperties *failed*"));
            goto Abort;
        }

        PathQuoteSpaces(szCommand);
        lstrcpyn(ProgramProps.achCmdLine, szCommand, ARRAYSIZE(ProgramProps.achCmdLine));

        // We probably do not wan't to prompt the user again that they are going into
        // MSDOS Mode...
        //
        ProgramProps.flPrgInit |= PRGINIT_REALMODESILENT | PRGINIT_REALMODE;

        if (!PifMgr_SetProperties(hPif, MAKEINTATOM(GROUP_PRG), &ProgramProps, SIZEOF(ProgramProps), 0))
        {
            DebugMsg(DM_TRACE, TEXT("ExitToDos: PifMgr_SetProperties *failed*"));
Abort:
            // BUGBUG:: SHould probably put up a message here...
            MessageBeep(0);
            return;
        }
        PifMgr_CloseProperties(hPif, 0);
    }

    ShellExecute(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}
#endif

BOOL CanDoFastRestart()
{
    return GetAsyncKeyState(VK_SHIFT) < 0;
}

// ---------------------------------------------------------------------------
// Shutdown thread

typedef struct {
    DWORD_PTR nCmd;
    HWND hwndParent;
} SDTP_PARAMS;

// Hydra-specific
#ifdef WINNT
void Disconnect()
{
    static PWINSTATION_DISCONNECT pfnWinStationDisconnect = NULL;
    HANDLE dllHandle;

    if (IsRemoteSession())
    {
        if (NULL == pfnWinStationDisconnect)
        {
            //
            // Load winsta.dll
            //
            dllHandle = LoadLibraryW(L"winsta.dll");
            if (dllHandle != NULL) 
            {
                //WinStationSetInformationW
                pfnWinStationDisconnect = (PWINSTATION_DISCONNECT) GetProcAddress(dllHandle, "WinStationDisconnect");
            }
        }

        if (pfnWinStationDisconnect != NULL)
        {
            pfnWinStationDisconnect( SERVERNAME_CURRENT, LOGONID_CURRENT, FALSE);
        }
        else
        {
            // It should never happen. What shall we do ?
            // How about assert false?
            ASSERT(FALSE);
        }
    }
}

#else // !WINNT
void Disconnect()
{
    // Don't call this on non-NT!
    ASSERT(FALSE);
}

#endif //WINNT

DWORD CALLBACK ShutdownThreadProc(void *pv)
{
    SDTP_PARAMS *psdtp = (SDTP_PARAMS *)pv;

    BOOL fShutdownWorked = FALSE;

    // tell USER that anybody can steal foreground from us
    // This allows apps to put up UI during shutdown/suspend/etc.
//    AllowSetForegroundWindow(ASFW_ANY);
 
    switch (psdtp->nCmd) {
    case SHTDN_SHUTDOWN:
        fShutdownWorked = CommonRestart(EWX_SHUTDOWN);
        break;

    case SHTDN_RESTART:
        fShutdownWorked = CommonRestart(CanDoFastRestart() ? EW_RESTARTWINDOWS : EWX_REBOOT);
        break;

    case SHTDN_LOGOFF:
        fShutdownWorked = CommonRestart(EWX_LOGOFF);
        break;

    case SHTDN_RESTART_DOS:        // Special hack to mean exit to dos
        // restart to dos, implies that we need to find the appropriate
        // pif file and exec it.  We may also need to initialize it if
        // it does not exist...
        ExitToDos(psdtp->hwndParent);
        // Fall through and leave f False...
        break;

    case SHTDN_SLEEP:
    case SHTDN_SLEEP2:
    case SHTDN_HIBERNATE:
        SuspendComputer ((psdtp->nCmd == SHTDN_HIBERNATE) ? TRUE : FALSE,
                         (GetKeyState(VK_CONTROL) < 0) ? TRUE : FALSE,
                         (psdtp->nCmd == SHTDN_SLEEP2) ? TRUE : FALSE);
        break;
    }

    //
    // if the shutdown worked terminate the calling app
    //
    TraceMsg(DM_TRACE, "s32.stp: pre-postmsg fShutdownWorked=%d", fShutdownWorked);
    if (fShutdownWorked) 
    { 
#ifndef WINNT
        // on NT, we cannot do this because it keeps explorer from saving state.
        // on win95, we have to do this because this is how taskman and shell breaks out 
        // of the message loop.
        // we cannot do it on the return of the api because before then, we kick off
        // a thread that sends out the endsession message.  after that, we do not 
        // receive the WM_QUIT from out queue.  -Chee
        ASSERT(0);      // now that we do ExitWindows in sep proc, we're killed
        PostMessage(psdtp->hwndParent, WM_QUIT, 0, 0);
#endif
        
    } 
    else 
    {
        HWND hwnd = FindWindow(c_szFakeDesktopClass, NULL);
        if (hwnd)
            PostMessage(hwnd, WM_CLOSE, 0, 0);
    }

    LocalFree(psdtp);

    return fShutdownWorked;
}

VOID ExitOrLogoffWindowsDialog(HWND hwndParent, BOOL bLogoff)
{
    INT_PTR nCmd;
    HWND hwndBackground = CreateFakeDesktopWindow();
    if (hwndBackground)
    {
        ShowWindow(hwndBackground, SW_SHOW);
        SetForegroundWindow(hwndBackground);
        DitherWindow(hwndBackground);
    }

    if (bLogoff)
    {
        nCmd = DialogBox(HINST_THISDLL, MAKEINTRESOURCE(DLG_LOGOFFWINDOWS),
            hwndBackground, LogoffWindowsDlgProc);
    }
    else
    {
        BOOL fGinaShutdownCalled = FALSE;
        HINSTANCE hGina;

        TCHAR szUsername[UNLEN];
        DWORD cchUsernameLength = UNLEN;
        DWORD dwOptions;

        if (WNetGetUser(NULL, szUsername, &cchUsernameLength) != NO_ERROR)
        {
            szUsername[0] = TEXT('\0');
        }
      
        EarlySaveSomeShellState();

        // Load MSGINA.DLL and get the ShellShutdownDialog function
        hGina = LoadLibrary(TEXT("msgina.dll"));

        if (hGina != NULL)
        {
            PFNSHELLSHUTDOWNDIALOG pfnShellShutdownDialog = (PFNSHELLSHUTDOWNDIALOG)
                GetProcAddress(hGina, "ShellShutdownDialog");

            if (pfnShellShutdownDialog != NULL)
            {
                nCmd = pfnShellShutdownDialog(hwndBackground,
                    szUsername, 0);

                // Handle disconnect right now
                if (nCmd == SHTDN_DISCONNECT)
                {
                    Disconnect();

                    // No other action
                    nCmd = SHTDN_NONE;
                }

                fGinaShutdownCalled = TRUE;
            }

            FreeLibrary(hGina);
        }

        if (!fGinaShutdownCalled)
        {
            dwOptions = GetShutdownOptions();
    
            // Gina call failed; use our cheesy private version
            nCmd = DownlevelShellShutdownDialog(hwndBackground,
                    dwOptions, szUsername);
        }

        _IconCacheSave();
        InvalidateDriveType(-1);
    }

    if (hwndBackground)
        SetForegroundWindow(hwndBackground);

    if (nCmd == SHTDN_NONE)
    {
        if (hwndBackground)
        {
            ShowWindow(hwndBackground, SW_HIDE);
            PostMessage(hwndBackground, WM_CLOSE, 0, 0);
        }
    }
    else
    {
        SDTP_PARAMS *psdtp = LocalAlloc(LPTR, sizeof(*psdtp));
        if (psdtp)
        {
            DWORD dw;
            HANDLE h;

            psdtp->nCmd = nCmd;
            psdtp->hwndParent = hwndParent;

            //  have another thread call ExitWindows() so our
            //  main pump keeps running durring shutdown.
            //
            h = CreateThread(NULL, 0, ShutdownThreadProc, psdtp, 0, &dw);
            if (h)
            {
                CloseHandle(h);
            }
            else
            {
                if (hwndBackground)
                    ShowWindow(hwndBackground, SW_HIDE);
                ShutdownThreadProc(psdtp);
            }
        }
    }
}

// API functions

STDAPI_(void) ExitWindowsDialog(HWND hwndParent)
{
    ExitOrLogoffWindowsDialog (hwndParent, FALSE);
}

STDAPI_(void) LogoffWindowsDialog(HWND hwndParent)
{
    ExitOrLogoffWindowsDialog (hwndParent, TRUE);
}
