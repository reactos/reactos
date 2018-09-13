/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    main.c

Abstract:

    This module contains the main routines for the Control Panel
    interface of the 32bit MAIN.CPL.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "rc.h"
#include "applet.h"
#include "mousectl.h"
#include "drvaplet.h"




//
//  Global Variables.
//

#ifdef WINNT

HINSTANCE g_hInst = NULL;

#else

#pragma data_seg(".idata")
HINSTANCE g_hInst = NULL;
#pragma data_seg()

#endif




//
//  Externally Defined Applets.
//

int MouseApplet(HINSTANCE, HWND, LPCTSTR);  // mouse.c
int KeybdApplet(HINSTANCE, HWND, LPCTSTR);  // keybd.c

BOOL RegisterPointerStuff(HINSTANCE);       // from mouseptr.c




//
//  Typedef Declarations.
//
typedef struct
{
    int            idIcon;
    int            idTitle;
    int            idExplanation;
    PFNAPPLETQUERY pfnAppletQuery;
    PFNAPPLET      pfnApplet;
    LPCTSTR        szDriver;
} APPLET;

APPLET Applets[] =
{
    { IDI_MOUSE,       IDS_MOUSE_TITLE,  IDS_MOUSE_EXPLAIN,  NULL,  MouseApplet,  TEXT("MOUSE") },
    { IDI_KEYBD,       IDS_KEYBD_TITLE,  IDS_KEYBD_EXPLAIN,  NULL,  KeybdApplet,  NULL    },
};

#define NUM_APPLETS (sizeof(Applets) / sizeof(Applets[0]))

int cApplets = NUM_APPLETS;





////////////////////////////////////////////////////////////////////////////
//
//  LibMain
//
////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY LibMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            g_hInst = hDll;
            DisableThreadLibraryCalls(hDll);
            break;
        }
        case ( DLL_PROCESS_DETACH ) :
        case ( DLL_THREAD_ATTACH ) :
        case ( DLL_THREAD_DETACH ) :
        {
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CplInit
//
//  Called when a CPL consumer initializes a CPL.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CplInit(
    HWND hParent)
{
    int i;

    InitCommonControls();

    RegisterPointerStuff(g_hInst);

    RegisterMouseControlStuff(g_hInst);

    for (i = 0; i < cApplets; i++)
    {
        if ((Applets[i].pfnAppletQuery != NULL) &&
            ((*Applets[i].pfnAppletQuery)(hParent, APPLET_QUERY_EXISTS) == FALSE))
        {
            cApplets--;

            if (i != cApplets)
            {
                Applets[i] = Applets[cApplets];
            }

            i--;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CplExit
//
//  Called when a CPL consumer is done with a CPL.
//
////////////////////////////////////////////////////////////////////////////

void CplExit(void)
{
}


////////////////////////////////////////////////////////////////////////////
//
//  CplInquire
//
//  Called when a CPL consumer wants info about an applet.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CplInquire(
    LPCPLINFO info,
    int iApplet)
{
    APPLET *applet = Applets + iApplet;
    HMODULE hDriverApplet = NULL;

    info->idIcon = applet->idIcon;

    if (applet->szDriver) {

        if (hDriverApplet = GetDriverModule(applet->szDriver)) {

            info->idIcon = CPL_DYNAMIC_RES;
            ReleaseDriverModule(hDriverApplet);

        } // if (hDriverApplet = ...

    } // if (applet->szDriver)

    info->idName = applet->idTitle;
    info->idInfo = applet->idExplanation;
    info->lData  = 0L;

    return (1L);
}


////////////////////////////////////////////////////////////////////////////
//
//  CplNewInquire
//
//  Called when a CPL consumer wants info about an applet.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CplNewInquire(
    HWND parent,
    LPNEWCPLINFO info,
    int iApplet)
{
    APPLET *applet = Applets + iApplet;
    HDAP hdap;

    info->dwSize = sizeof(NEWCPLINFO);
    info->hIcon = NULL;

    //
    //  See if the applet is associated with a driver which can provide us
    //  an icon.
    //
    if ((applet->szDriver) &&
        ((hdap = OpenDriverApplet(applet->szDriver)) != NULL))
    {
        info->hIcon = GetDriverAppletIcon(hdap);
        CloseDriverApplet(hdap);
    }

    if ((!info->hIcon) && (applet->pfnAppletQuery != NULL))
    {
        info->hIcon = (HICON)(*(applet->pfnAppletQuery))( parent,
                                                          APPLET_QUERY_GETICON );
    }

    if (!info->hIcon)
    {
        info->hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(applet->idIcon));
    }

    LoadString(g_hInst, applet->idTitle, info->szName, sizeof(info->szName));
    LoadString(g_hInst, applet->idExplanation, info->szInfo, sizeof(info->szInfo));

    info->lData = 0L;
    *info->szHelpFile = 0;
    info->dwHelpContext = 0UL;

    return (1L);
}


////////////////////////////////////////////////////////////////////////////
//
//  CplInvoke
//
//  Called to invoke an applet.  It checks the applet's return value to see
//  if we need to restart.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CplInvoke(
    HWND parent,
    int iApplet,
    LPCTSTR cmdline)
{
    DWORD exitparam = 0UL;

    switch (Applets[iApplet].pfnApplet(g_hInst, parent, cmdline))
    {
        case ( APPLET_RESTART ) :
        {
            exitparam = EW_RESTARTWINDOWS;
            break;
        }
        case ( APPLET_REBOOT ) :
        {
            exitparam = EW_REBOOTSYSTEM;
            break;
        }
        default :
        {
            return (1L);
        }
    }

    RestartDialog(parent, NULL, exitparam);
    return (1L);
}


////////////////////////////////////////////////////////////////////////////
//
//  CplApplet
//
//  A CPL consumer calls this to request stuff from us.
//
////////////////////////////////////////////////////////////////////////////

LRESULT APIENTRY CPlApplet(
    HWND parent,
    UINT msg,
    LPARAM lparam1,
    LPARAM lparam2)
{
    switch (msg)
    {
        case ( CPL_INIT ) :
        {
            return (CplInit(parent));
        }
        case ( CPL_EXIT ) :
        {
            CplExit();
            break;
        }
        case ( CPL_GETCOUNT ) :
        {
            return (cApplets);
        }
        case ( CPL_INQUIRE ) :
        {
            return (CplInquire((LPCPLINFO)lparam2, (int)lparam1));
        }
        case ( CPL_NEWINQUIRE ) :
        {
            return (CplNewInquire(parent, (LPNEWCPLINFO)lparam2, (int)lparam1));
        }
        case ( CPL_DBLCLK ) :
        {
            lparam2 = 0L;

            // fall through...
        }
        case ( CPL_STARTWPARMS ) :
        {
            return (CplInvoke(parent, (int)lparam1, (LPTSTR)lparam2));
        }
    }

    return (0L);
}
