//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#include "control.h"
#include <cpl.h>
#include <cplp.h>
#include "rcids.h"

BOOL ImmDisableIME(DWORD dwThreadId);

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  LEAVE THESE IN ENGLISH
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const TCHAR c_szCtlPanelClass[] = TEXT("CtlPanelClass");
const TCHAR c_szRunDLLShell32Etc[] = TEXT("rundll32.exe Shell32.dll,Control_RunDLL ");
const TCHAR c_szDoPrinters[] = 
    TEXT("explorer.exe \"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}\"");
const TCHAR c_szDoFonts[] = 
    TEXT("explorer.exe \"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{D20EA4E1-3957-11d2-A40B-0C5020524152}\"");
const TCHAR c_szDoAdminTools[] = 
    TEXT("explorer.exe \"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{D20EA4E1-3957-11d2-A40B-0C5020524153}\"");
const TCHAR c_szDoSchedTasks[] = 
    TEXT("explorer.exe \"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{D6277990-4C6A-11CF-8D87-00AA0060F5BF}\"");
const TCHAR c_szDoNetConnections[] = 
    TEXT("explorer.exe \"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}\"");
const TCHAR c_szDoUserPassword[] = 
    TEXT("rundll32.exe netplwiz.dll,UsersRunDll");


typedef struct
{
    const TCHAR *oldform;
    const TCHAR *newform;
    BOOL         fDoNotUseRunDLL;

} COMPATCPL;
typedef COMPATCPL const *LPCCOMPATCPL;

COMPATCPL const c_aCompatCpls[] =
{
    {   TEXT("DESKTOP"),          TEXT("desk.cpl")      , 0},
    {   TEXT("COLOR"),            TEXT("desk.cpl,,2")   , 0},
    {   TEXT("DATE/TIME"),        TEXT("timedate.cpl")  , 0},
    {   TEXT("PORTS"),            TEXT("sysdm.cpl,,1")  , 0},
    {   TEXT("INTERNATIONAL"),    TEXT("intl.cpl")      , 0},
    {   TEXT("MOUSE"),            TEXT("main.cpl")      , 0},
    {   TEXT("KEYBOARD"),         TEXT("main.cpl @1")   , 0},
    {   TEXT("FOLDERS"),          TEXT("appwiz.cpl @1") , 0},
    {   TEXT("NETWARE"),          TEXT("nwc.cpl")       , 0},
    {   TEXT("TELEPHONY"),        TEXT("telephon.cpl")  , 0},
    {   TEXT("INFRARED"),         TEXT("irprops.cpl")   , 0},

    {   TEXT("PRINTERS"),         c_szDoPrinters        , 1},
    {   TEXT("FONTS"),            c_szDoFonts           , 1},
    {   TEXT("ADMINTOOLS"),       c_szDoAdminTools      , 1},
    {   TEXT("SCHEDTASKS"),       c_szDoSchedTasks      , 1},
    {   TEXT("USERPASSWORDS"),    c_szDoUserPassword    , 1},
    {   TEXT("NETCONNECTIONS"),   c_szDoNetConnections  , 1}
};

#define NUMCOMPATCPLS   ARRAYSIZE(c_aCompatCpls)
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Timer
#define TIMER_QUITNOW   1
#define TIMEOUT         10000

#define DM_CPTRACE      0

//----------------------------------------------------------------------------
#ifdef UNICODE
#define WinExec WinExecW

//
//  For UNICODE create a companion to ANSI only base WinExec api.
//

UINT WINAPI WinExecW (LPTSTR lpCmdLine, UINT uCmdShow)
{
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInformation;

    //
    // Create the process
    //
    
    memset (&StartupInfo, 0, sizeof(StartupInfo));
    
    StartupInfo.cb = sizeof(StartupInfo);
    
    StartupInfo.wShowWindow = (WORD)uCmdShow;
    
    if (CreateProcess ( NULL,
                        lpCmdLine,            // CommandLine
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation
                        ))
    {
        CloseHandle(ProcessInformation.hThread);
        CloseHandle(ProcessInformation.hProcess);
    }

    return(1);

}
#endif  // UNICODE

//---------------------------------------------------------------------------
LRESULT CALLBACK DummyControlPanelProc(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
    TCHAR sz[sizeof(c_szDoPrinters)];

    switch (uMsg)
    {
        case WM_CREATE:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: Created..."));
            // We only want to hang around for a little while.
            SetTimer(hwnd, TIMER_QUITNOW, TIMEOUT, NULL);
            return 0;
        case WM_DESTROY:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: Destroyed..."));
            // Quit the app when this window goes away.
            PostQuitMessage(0);
            return 0;
        case WM_TIMER:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: Timer %d"), wparam);
            if (wparam == TIMER_QUITNOW)
            {
                // Get this window to go away.
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_COMMAND:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: Command %d"), wparam);
            // NB Hack for hollywood - they send a menu command to try
            // and open the printers applet. They try to search control panels
            // menu for the printers item and then post the associated command.
            // As our fake window doesn't have a menu they can't find the item
            // and post us a -1 instead (ripping on the way).
            if (wparam == (WPARAM)-1)
            {
                lstrcpy(sz, c_szDoPrinters);
                WinExec(sz, SW_SHOWNORMAL);
            }
            return 0;
        default:
            DebugMsg(DM_CPTRACE, TEXT("cp.dcpp: %x %x %x %x"), hwnd, uMsg, wparam, lparam);
            return DefWindowProc(hwnd, uMsg, wparam, lparam);
    }
}

//---------------------------------------------------------------------------
HWND _CreateDummyControlPanel(HINSTANCE hinst)
{
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = DummyControlPanelProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinst;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = c_szCtlPanelClass;

    RegisterClass(&wc);
    return CreateWindow(c_szCtlPanelClass, NULL, 0, 0, 0, 0, 0, NULL, NULL, hinst, NULL);
}

void ProcessPolicy(void)
{
    HINSTANCE hInst;
    APPLET_PROC pfnCPLApplet;

    hInst = LoadLibrary (TEXT("desk.cpl"));

    if (!hInst) {
        return;
    }

    pfnCPLApplet = (APPLET_PROC) GetProcAddress (hInst, "CPlApplet");

    if (pfnCPLApplet) {

        (*pfnCPLApplet)(NULL, CPL_POLICYREFRESH, 0, 0);
    }

    FreeLibrary (hInst);

}

//---------------------------------------------------------------------------
int WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    TCHAR sz[sizeof(c_szDoPrinters)];
    MSG msg;

    DebugMsg(DM_TRACE, TEXT("cp.wm: Control starting."));

    ImmDisableIME(0);

    _CreateDummyControlPanel(hInstance);

    lstrcpy(sz, c_szRunDLLShell32Etc);

    // we need to check for PANEL passed in as an arg.  The run dialog
    // autocomplete shows "Control Panel" as a choice and we used to
    // interpret this as Control with panel as an arg.  So if we have
    // panel as an arg then we do the same processing as when we have
    // "Control" only.
    if (*lpCmdLine && lstrcmpi(lpCmdLine, TEXT("PANEL")))
    {
        LPCCOMPATCPL item = c_aCompatCpls;
        int i = NUMCOMPATCPLS;

        //
        // Policy hook.  Userenv.dll will call control.exe with the
        // /policy command line switch.  If so, we need to load the
        // desk.cpl applet and refresh the colors / bitmap.
        //

        if (lstrcmpi(TEXT("/policy"), lpCmdLine) == 0) {
            ProcessPolicy();
            return TRUE;
        }


        //
        // COMPAT HACK: special case some applets since apps depend on them
        //
        while (i-- > 0)
        {
            if (lstrcmpi(item->oldform, lpCmdLine) == 0)
            {
                if (item->fDoNotUseRunDLL)
                    lstrcpy(sz, item->newform);
                else
                    lstrcat(sz, item->newform);

                break;
            }

            item++;
        }

        //
        // normal case (pass command line thru)
        //
        if (i < 0)
            lstrcat(sz, lpCmdLine);
    }

    // HACK: NerdPerfect tries to open a hidden control panel to talk to
    // we are blowing off fixing the communication stuff so just make
    // sure the folder does not appear hidden
    if (nCmdShow == SW_HIDE)
        nCmdShow = SW_SHOWNORMAL;

    WinExec(sz, nCmdShow);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
    }

    DebugMsg(DM_TRACE, TEXT("cp.wm: Control exiting."));

    return TRUE;
}

#ifdef WIN32
//---------------------------------------------------------------------------
// Stolen from the CRT, used to shrink our code.
int _stdcall ModuleEntry(void)
{
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo(&si);

    return WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

}
#endif
