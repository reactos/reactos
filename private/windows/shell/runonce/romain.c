// **************************************************************************
//
// ROMain.C
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1993
//  All rights reserved
//
//  The window/messages pump for RunOnce
//
//      5 June 1994     FelixA  Started
//
//     23 June 94       FelixA  Moved to Shell tree. Changed UI.
//
// *************************************************************************/

#include "runonce.h"
#include "regstr.h"
#include <shellapi.h>
#include "resource.h"

HINSTANCE g_hInst;          // current instance
BOOL InitROInstance( HINSTANCE hInstance, int nCmdShow);

typedef enum {
    RRA_DEFAULT = 0x0000,
    RRA_DELETE  = 0x0001,
    RRA_WAIT    = 0x0002,
} RRA_FLAGS;

BOOL   RunRegApps(HKEY hkeyParent, LPCTSTR szSubkey, RRA_FLAGS fFlags);

int ParseCmdLine(LPCTSTR lpCmdLine)
{
    int Res=0;

    while(*lpCmdLine)
    {
        while( *lpCmdLine && *lpCmdLine!=TEXT('-') && *lpCmdLine!=TEXT('/'))
            lpCmdLine++;

        if (!(*lpCmdLine)) {
            return Res;
        }

        switch(*++lpCmdLine)
        {
            case TEXT('r'):
                Res|=CMD_DO_CHRIS;
                break;
            case TEXT('b'):
                Res|=CMD_DO_REBOOT;
                break;
            case TEXT('s'):
                Res|=CMD_DO_RESTART;
                break;
        }
        lpCmdLine++;
    }
    return Res;
}

/****************************************************************************

        FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

        PURPOSE: calls initialization function, processes message loop

****************************************************************************/
int g_iState=0;
int __stdcall WinMainT(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPTSTR lpCmdLine,
        int nCmdShow)
{
    MSG msg;

    if (!hPrevInstance)
    {       // Other instances of app running?
        if (!InitApplication(hInstance))
        { // Initialize shared things
             return (FALSE);     // Exits if unable to initialize
        }
    }

    // see if we have a commnand line switch - in a VERY bad way.
    g_iState = ParseCmdLine(GetCommandLine());
    if(g_iState & CMD_DO_CHRIS )
    {
        // Go do chris's runonce stuff.
        if (!InitROInstance(hInstance, nCmdShow))
            return (FALSE);
        return TRUE;
    }
    else
    {
        /* Perform initializations that apply to a specific instance */
        if (!InitInstance(hInstance, nCmdShow))
            return (FALSE);
    }
    return (FALSE);
}


/****************************************************************************

        FUNCTION: InitApplication(HINSTANCE)


****************************************************************************/

BOOL InitApplication(HINSTANCE hInstance)
{
//    CreateGlobals();
    return TRUE;
}


/****************************************************************************

        FUNCTION:  InitInstance(HINSTANCE, int)

****************************************************************************/

BOOL InitInstance( HINSTANCE hInstance, int nCmdShow)
{
    HWND hShell=GetShellWindow();
    g_hInst = hInstance; // Store instance handle in our global variable

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_RUNONCE),NULL,dlgProcRunOnce);
    return (TRUE);              // We succeeded...
}

BOOL InitROInstance( HINSTANCE hInstance, int nCmdShow)
{
    g_hInst = hInstance; // Store instance handle in our global variable

    // Ideally this should be sufficient.
    RunRegApps(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, RRA_DELETE| RRA_WAIT );
    return TRUE;
}

BOOL TopLeftWindow( HWND hwndChild, HWND hwndParent)
{
    return SetWindowPos(hwndChild, NULL, 32, 32, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/****************************************************************************

        FUNCTION: CenterWindow (HWND, HWND)

        PURPOSE:  Center one window over another

        COMMENTS:

        Dialog boxes take on the screen position that they were designed at,
        which is not always appropriate. Centering the dialog over a particular
        window usually results in a better position.

****************************************************************************/

BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc;

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;


    // Get the display limits
    hdc = GetDC (hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndChild, hdc);

    // Get the Height and Width of the parent window
    if( !GetWindowRect (hwndParent, &rParent) )
    {
        rParent.right = wScreen;
        rParent.left  = 0;
        rParent.top = 0;
        rParent.bottom = hScreen;
    }

        wParent = rParent.right - rParent.left;
        hParent = rParent.bottom - rParent.top;

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0)
    {
        xNew = 0;
    }
    else
    if ((xNew+wChild) > wScreen)
    {
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0)
    {
        yNew = 0;
    } else if ((yNew+hChild) > hScreen)
    {
        yNew = hScreen - hChild;
    }

    // Set it, and return
    return SetWindowPos (hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// This is taken from \shell\cabinet\initcab.c

BOOL   RunRegApps(HKEY hkeyParent, LPCTSTR szSubkey, RRA_FLAGS fFlags)
{
    HKEY hkey;
    BOOL fShellInit = FALSE;

// Additional line
    int g_fCleanBoot=FALSE;
// End additions.
    if (RegOpenKey(hkeyParent, szSubkey, &hkey) == ERROR_SUCCESS)
    {
        DWORD cbData, cbValue, dwType, i;
        TCHAR szValueName[128], szCmdLine[MAX_PATH*4];
        STARTUPINFO startup;
        PROCESS_INFORMATION pi;
        startup.cb = sizeof(startup);
        startup.lpReserved = NULL;
        startup.lpDesktop = NULL;
        startup.lpTitle = NULL;
        startup.dwFlags = 0L;
        startup.cbReserved2 = 0;
        startup.lpReserved2 = NULL;
        //startup.wShowWindow = wShowWindow;

        for (i = 0; ; i++)
        {
            LONG lEnum;

            cbValue = sizeof(szValueName) / sizeof(TCHAR);
            cbData = sizeof(szCmdLine);

            if( ( lEnum = RegEnumValue( hkey, i, szValueName, &cbValue, NULL,
                &dwType, (LPBYTE) szCmdLine, &cbData ) ) == ERROR_MORE_DATA )
            {
                // ERROR_MORE_DATA means the value name or data was too large
                // skip to the next item
                // DebugMsg( DM_ERROR, "Cannot run oversize entry in <%s>", szSubkey );
                continue;
            }
            else if( lEnum != ERROR_SUCCESS )
            {
                // could be ERROR_NO_MORE_ENTRIES, or some kind of failure
                // we can't recover from any other registry problem, anyway
                break;
            }

            if (dwType == REG_SZ)
            {
                // DebugMsg(DM_TRACE, "%s %s", szSubkey, szCmdLine);
                if ((fFlags & RRA_DELETE)) {
                    i--;        // adjust for shift in value index
                    RegDeleteValue(hkey, szValueName);
                }

                // only run things marked with a "*" in clean boot

                if (g_fCleanBoot && (szValueName[0] != TEXT('*')))
                    continue;

                if (lstrcmpi(szValueName, TEXT("InitShell")) == 0)
                    fShellInit = TRUE;
                else
                {
                    if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP,
                                      NULL, NULL, &startup, &pi))
                    {

                        if (fFlags & RRA_WAIT)
                            WaitForSingleObjectEx(pi.hProcess, INFINITE, TRUE);

                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                    }
                }
            }
        }
        RegCloseKey(hkey);
    }

    return(fShellInit);

}

// stolen from the CRT, used to shirink our code

int _stdcall ModuleEntry(void)
{
    int i;
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

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}
