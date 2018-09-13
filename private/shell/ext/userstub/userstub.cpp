//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       userstub.cpp
//
//  Contents:   exe to load webcheck
//
//  Classes:
//
//  Functions:
//
//  History:    12-12/96    rayen (Raymond Endres)  Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <docobj.h>
#include <webcheck.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shellp.h>

#ifdef DEBUG
#define ASSERT(b)   if (!b) DebugBreak();
#else
#define ASSERT(b)
#endif

// if inststub.h is used in userstub, use LoadString() OW use MLLoadString()
#define	USERSTUB	1

//
// NOTE: ActiveSetup relies on our window name and class name
// to shut us down properly in softboot.  Do not change it.
//
//const TCHAR c_szClassName[] = TEXT("userstub");
//const TCHAR c_szWebCheck[] = TEXT("WebCheck");
//const TCHAR c_szWebCheckWindow[] = TEXT("MS_WebcheckMonitor");
//const TCHAR c_szShellReg[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad");
//const TCHAR c_szWebcheckKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Webcheck");

typedef struct {
    HINSTANCE   hInstance;          // handle to current instance
    BOOL        fUninstallOnly;     // TRUE -> run uninstall stubs only, then quit
} GLOBALS;

GLOBALS g;

//
//====== DllGetVersion  =======================================================
// from shlwapi.h so we don't have to include it.

typedef struct _DllVersionInfo
{
    DWORD cbSize;
    DWORD dwMajorVersion;                   // Major version
    DWORD dwMinorVersion;                   // Minor version
    DWORD dwBuildNumber;                    // Build number
    DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;

//
// The caller should always GetProcAddress("DllGetVersion"), not
// implicitly link to it.
//

typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);



// Code to run install/uninstall stubs, from shell\inc.

#define HINST_THISDLL   g.hInstance
#include "resource.h"
#include <stubsup.h>
#include <inststub.h>

int WINAPI WinMainT(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL bParseCommandLine(LPSTR lpCmdLine, int nCmdShow);


//----------------------------------------------------------------------------
// ModuleEntry
//----------------------------------------------------------------------------
extern "C" int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPTSTR pszCmdLine;

    pszCmdLine = GetCommandLine();

    // g_hProcessHeap = GetProcessHeap();

    //
    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail
    //
    SetErrorMode(SEM_FAILCRITICALERRORS);

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
    GetStartupInfoA(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    // Since we now have a way for an extension to tell us when it is finished,
    // we will terminate all processes when the main thread goes away.

    ExitProcess(i);

    // DebugMsg(DM_TRACE, TEXT("c.me: Cabinet main thread exiting without ExitProcess."));
    return i;
}



//----------------------------------------------------------------------------
// WinMain
//----------------------------------------------------------------------------
int WINAPI WinMainT(
    HINSTANCE hInstance,            // handle to current instance
    HINSTANCE hPrevInstance,        // handle to previous instance
    LPSTR lpCmdLine,                // pointer to command line
    int nCmdShow                        // show state of window
   )
{
    // Save the globals
    g.hInstance = hInstance;
    g.fUninstallOnly = FALSE;

    // Parse the command line, for DEBUG options and for uninstall-only switch.
    if (!bParseCommandLine(lpCmdLine, nCmdShow))
        return 0;

    // Run all install/uninstall stubs for browser-only mode.
    // If IE4 has been uninstalled, we'll be run with the -u switch; this
    // means to run install/uninstall stubs only, no webcheck stuff.
    RunInstallUninstallStubs2(NULL);
 
    // Return the exit code to Windows
    return 0; 
} 

//----------------------------------------------------------------------------
// bParseCmdLine
//
// Parse the command line
//      -u  run install/uninstall stubs only, then quit
//  DEBUG options:
//      -v  visible window (easy to shutdown)
//      -a  add webcheck to shell service object
//      -r  remove webcheck from shell service object
//      -s  fix shell folders only
//      -?  these options
//----------------------------------------------------------------------------
BOOL bParseCommandLine(LPSTR lpCmdLine, int nCmdShow)
{
    if (!lpCmdLine)
        return TRUE;

    CharUpper(lpCmdLine);   /* easier to parse */
    while (*lpCmdLine) 
    {
        if (*lpCmdLine != '-' && *lpCmdLine != '/')
            break;

        lpCmdLine++;

        switch (*(lpCmdLine++)) 
        {
            case 'U':   
                g.fUninstallOnly = TRUE;
                break;
        }

        while (*lpCmdLine == ' ' || *lpCmdLine == '\t') {
            lpCmdLine++;
        }
    }

    return TRUE;
}
