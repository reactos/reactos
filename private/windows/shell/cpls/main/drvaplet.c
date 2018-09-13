/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    drvaplet.c

Abstract:

    This module contains the driver routines for the project.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "drvaplet.h"




//
//  Defines for Win16 builds.
//

#ifndef WIN32
#define LoadLibrary16       LoadLibrary
#define FreeLibrary16       FreeLibrary
#define GetProcAddress16    GetProcAddress
#endif




//
//  Global Variables.
//

//
//  CplApplet.
//
const TCHAR *c_szCplApplet  = TEXT("CPlApplet");
const char  *c_szCplAppletA = "CPlApplet";




//
//  Typedef Declarations.
//

//
//  DRIVER_APPLET_INFO: the info we keep around about a driver applet.
//
typedef struct
{
    HMODULE     module;
    APPLET_PROC applet;
    HICON       icon;

} DRIVER_APPLET_INFO, *PDAI;




//
//  GetDriverModule: gets the module.
//
////////////////////////////////////////////////////////////////////////////
//
//  GetDriverModule
//
//  Gets the module.
//
////////////////////////////////////////////////////////////////////////////

HMODULE GetDriverModule(
    LPCTSTR name)
{
#ifdef WIN32

#ifdef WINNT
    return (LoadLibrary(name));
#else
    return (LoadLibrary16(name));
#endif

#else
    return (GetModuleHandle(name));
#endif
}


////////////////////////////////////////////////////////////////////////////
//
//  ReleaseDriverModule
//
////////////////////////////////////////////////////////////////////////////

void ReleaseDriverModule(
    HMODULE module)
{
#ifdef WIN32

#ifdef WINNT
    FreeLibrary(module);
#else
    FreeLibrary16(module);
#endif

#else
    //
    // do nothing (got it with GetModuleHandle)
    //
#endif
}


////////////////////////////////////////////////////////////////////////////
//
//  OpenDriverApplet
//
//  Opens a handle to the named driver applet.
//
////////////////////////////////////////////////////////////////////////////

HDAP OpenDriverApplet(
    LPCTSTR name)
{
    PDAI driver = (PDAI)LocalAlloc(LPTR, sizeof(DRIVER_APPLET_INFO));

    if (driver)
    {
        if ((driver->module = GetDriverModule(name)) != NULL)
        {
            if ((driver->applet = (APPLET_PROC)
#ifdef WINNT
                GetProcAddress(driver->module, c_szCplAppletA)) != NULL)
#else
                GetProcAddress16(driver->module, c_szCplApplet)) != NULL)
#endif
            {
                union
                {
                    NEWCPLINFO newform;
                    CPLINFO oldform;
                } info = { 0 };

                CallDriverApplet( (HDAP) driver,
                                  NULL,
                                  CPL_NEWINQUIRE,
                                  0,
                                  (LPARAM)&info.newform );

                if (info.newform.dwSize == sizeof(info.newform))
                {
                    driver->icon = info.newform.hIcon;
                    return ((HDAP)driver);
                }

//
//  NOTE: If the driver doesn't handle CPL_NEWIQUIRE, we must use CPL_INQUIRE
//  and LoadIcon the icon ourselves.  Win32 doesn't provide a LoadIcon16, so
//  in Win32 the 16 bit side of the thunk for CPL_NEWINQUIRE does this.  In
//  Win16, we do it right here.
//

#ifndef WIN32
                info.oldform.idIcon = 0;

                CallDriverApplet( (HDAP)driver,
                                  NULL,
                                  CPL_INQUIRE,
                                  0,
                                  (LPARAM)&info.oldform );

                if (info.oldform.idIcon)
                {
                    driver->icon =
                        LoadIcon( driver->module,
                                  MAKEINTRESOURCE(info.oldform.idIcon) );

                    return ((HDAP)driver);
                }
#endif
            }

            ReleaseDriverModule(driver->module);
        }

        LocalFree(driver);
    }

    return ((HDAP)0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CloseDriverApplet
//
//  Closes a handle to a driver applet.
//
////////////////////////////////////////////////////////////////////////////

void CloseDriverApplet(
    HDAP HDAP)
{
#define driver ((PDAI)HDAP)

    if (driver)
    {
        if (driver->icon)
        {
            DestroyIcon(driver->icon);
        }
        ReleaseDriverModule(driver->module);
        LocalFree(driver);
    }

#undef driver
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDriverAppletIcon
//
//  Gets a driver applet's icon (if any).
//
////////////////////////////////////////////////////////////////////////////

HICON GetDriverAppletIcon(
    HDAP HDAP)
{
#define driver ((PDAI)HDAP)

    //
    //  Must return a copy for the current process/task to own.
    //
    return ((driver && driver->icon) ? CopyIcon(driver->icon) : NULL);

#undef driver
}


////////////////////////////////////////////////////////////////////////////
//
//  CallDriverApplet
//
//  Calls the driver applet (same syntax as CplApplet).
//
////////////////////////////////////////////////////////////////////////////

LRESULT CallDriverApplet(
    HDAP HDAP,
    HWND wnd,
    UINT msg,
    LPARAM p1,
    LPARAM p2)
{
#define driver ((PDAI)HDAP)

    if (driver)
    {
#ifdef WIN32
        return ( CallCPLEntry16( driver->module,
                                 (FARPROC16)driver->applet,
                                 wnd,
                                 msg,
                                 p1,
                                 p2 ) );
#else
        return (driver->applet(wnd, msg, p1, p2));
#endif
    }

    return (0L);

#undef driver
}
