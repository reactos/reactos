/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mouse.c

Abstract:

    This module contains the routines for the "fake" applets.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "rc.h"
#include "applet.h"

//
// From shell\inc\shsemip.h
//
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//
//  From shelldll\help.c.
//
VOID WINAPI SHHelpShortcuts_RunDLL( HWND, HINSTANCE, LPCSTR, int );
VOID WINAPI SHHelpShortcuts_RunDLLW( HWND, HINSTANCE, LPCWSTR, int );

static const TCHAR c_szPrintersFolder[]           = TEXT("PrintersFolder");
static const TCHAR c_szFontsFolder[]              = TEXT("FontsFolder");




////////////////////////////////////////////////////////////////////////////
//
//  PrintApplet
//
////////////////////////////////////////////////////////////////////////////

int PrintApplet(
    HINSTANCE instance,
    HWND parent,
    LPCTSTR cmdline)
{
#ifdef UNICODE
    SHHelpShortcuts_RunDLLW( NULL,
                             GetModuleHandle(NULL),
                             c_szPrintersFolder,
                             SW_SHOWNORMAL );
#else
    SHHelpShortcuts_RunDLL( NULL,
                            GetModuleHandle(NULL),
                            c_szPrintersFolder,
                            SW_SHOWNORMAL );
#endif

    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  FontsApplet
//
////////////////////////////////////////////////////////////////////////////

int FontsApplet(
    HINSTANCE instance,
    HWND parent,
    LPCTSTR cmdline)
{
#ifdef UNICODE
    SHHelpShortcuts_RunDLLW( NULL,
                             GetModuleHandle(NULL),
                             c_szFontsFolder,
                             SW_SHOWNORMAL );
#else
    SHHelpShortcuts_RunDLL( NULL,
                            GetModuleHandle(NULL),
                            c_szFontsFolder,
                            SW_SHOWNORMAL );
#endif

    return (0);
}

////////////////////////////////////////////////////////////////////////////
//
//  AdmApplet
//
////////////////////////////////////////////////////////////////////////////

int AdmApplet(
    HINSTANCE instance,
    HWND parent,
    LPCTSTR cmdline)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szAdminTools[MAX_PATH];

    if ( !SHGetSpecialFolderPath(parent, szPath, CSIDL_COMMON_PROGRAMS, 0) )
        return 1;

    // load the string for the tools folder, then path combine the two so that
    // we can open that directory.

    if ( !LoadString(instance, IDS_ADM_TITLE, szAdminTools, ARRAYSIZE(szAdminTools)) )
        return 1;
    
    //+1 for backslash and +1 for '\0'

    if ( (lstrlen(szPath)+lstrlen(szAdminTools)+1+1) > ARRAYSIZE(szPath) )
        return 1;

    PathCombine(szPath, szPath, szAdminTools);
    ShellExecute(parent, NULL, szPath, NULL, NULL, SW_SHOWDEFAULT);

    return (0);
}
