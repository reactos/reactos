///////////////////////////////////////////////////////////////////////////////
//
// DESK.C
//
// Control Panel interface of 32bit DESK.CPL
//
///////////////////////////////////////////////////////////////////////////////

#include "desk.h"
#include "rc.h"
#include "applet.h"
// #include <pwdspi.h>

///////////////////////////////////////////////////////////////////////////////
// LibMain
///////////////////////////////////////////////////////////////////////////////

HINSTANCE g_hInst = NULL;

BOOL APIENTRY
LibMain( HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved )
{
    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:
    	    g_hInst = hDll;
	    break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// applet definitions
///////////////////////////////////////////////////////////////////////////////

// externally defined applets
int DisplayApplet( HINSTANCE, HWND, LPCSTR );  // display.c

typedef struct
{
    int         idIcon;
    int         idTitle;
    int         idExplanation;
    PFNAPPLET   pfnApplet;
//    LPCSTR      szHelpFile;
//    DWORD       dwHelpContext;
    BOOL        bDynamicResources;
    LPCSTR      pszRestriction;
    int         idDisabledExplanation;
} APPLET;

APPLET Applets[] = {
    { IDI_DISPLAY, IDS_DISPLAY_TITLE, IDS_DISPLAY_EXPLAIN, DisplayApplet, FALSE,
      szNoDispCPL, IDS_DISPLAY_DISABLED },
};

#define NUM_APPLETS ( sizeof( Applets ) / sizeof( Applets[ 0 ] ) )


///////////////////////////////////////////////////////////////////////////////
// CplInit -- called when a CPL consumer initializes a CPL
// CplExit -- called when a CPL consumer is done with a CPL
///////////////////////////////////////////////////////////////////////////////

LRESULT
CplInit( HWND hParent )
{
    InitCommonControls();
    return TRUE;
}

void
CplExit( void )
{
}


///////////////////////////////////////////////////////////////////////////////
// CplInquire -- called when a CPL consumer wants info about an applet
///////////////////////////////////////////////////////////////////////////////

LRESULT
CplInquire( LPCPLINFO info, int iApplet )
{
    APPLET *applet = Applets + iApplet;

    if( applet->bDynamicResources )
    {
        info->idIcon = CPL_DYNAMIC_RES;
        info->idName = CPL_DYNAMIC_RES;
        info->idInfo = CPL_DYNAMIC_RES;
    }
    else
    {
        info->idIcon = applet->idIcon;
        info->idName = applet->idTitle;
        info->idInfo = applet->idExplanation;
    }

    info->lData = 0L;

    return 1L;
}


///////////////////////////////////////////////////////////////////////////////
// CplNewInquire -- called when a CPL consumer wants info about an applet
///////////////////////////////////////////////////////////////////////////////

LRESULT
CplNewInquire( LPNEWCPLINFO info, int iApplet )
{
    APPLET *applet = Applets + iApplet;

    info->dwSize = sizeof( NEWCPLINFO );
    info->hIcon = LoadIcon( g_hInst, MAKEINTRESOURCE( applet->idIcon ) );

    LoadString( g_hInst, applet->idTitle, info->szName, sizeof( info->szName ) );
    LoadString( g_hInst, applet->idExplanation, info->szInfo, sizeof( info->szInfo ) );

    info->lData = 0L;

//    lstrcpy( info->szHelpFile, applet->szHelpFile );
//    info->dwHelpContext = applet->dwHelpContext;
    *info->szHelpFile = 0;
    info->dwHelpContext = 0UL;

    return 1L;
}


///////////////////////////////////////////////////////////////////////////////
// CplInvoke -- called to invoke an applet
// checks the applet's return value to see if we need to restart
///////////////////////////////////////////////////////////////////////////////

LRESULT
CplInvoke( HWND parent, int iApplet, LPCSTR cmdline )
{
    DWORD exitparam = 0UL;
    HKEY hKey;
    APPLET Applet = Applets[iApplet];

    // check to see if this CPL has been restricted by administrator
    if (Applet.pszRestriction &&
        (RegOpenKey(HKEY_CURRENT_USER,szRestrictionKey,&hKey) == ERROR_SUCCESS)) {
        BOOL fDisableCPL = CheckRestriction(hKey,Applet.pszRestriction);		
        RegCloseKey(hKey);
        if (fDisableCPL) {
            char szMessage[255],szTitle[255];
            LoadString(g_hInst,Applet.idDisabledExplanation,szMessage,sizeof(szMessage));
            LoadString(g_hInst,Applet.idTitle,szTitle,sizeof(szTitle));

            MessageBox(parent,szMessage,szTitle,MB_OK | MB_ICONINFORMATION);

            return 1L;
        }
    }


    switch( Applets[ iApplet ].pfnApplet( g_hInst, parent, cmdline ) )
    {
        case APPLET_RESTART:
            exitparam = EW_RESTARTWINDOWS;
            break;

        case APPLET_REBOOT:
            exitparam = EW_REBOOTSYSTEM;
            break;

        default:
            return 1L;
    }

    RestartDialog( parent, NULL, exitparam );
    return 1L;
}


///////////////////////////////////////////////////////////////////////////////
// CplApplet -- a CPL consumer calls this to request stuff from us
///////////////////////////////////////////////////////////////////////////////

LRESULT APIENTRY
CPlApplet( HWND parent, UINT msg, LPARAM lparam1, LPARAM lparam2 )
{
    static BOOL fReEntered = FALSE;

    switch( msg )
    {
        case CPL_INIT:
            return CplInit( parent );

        case CPL_EXIT:
            CplExit();
            break;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            return CplInquire( (LPCPLINFO)lparam2, (int)lparam1 );

        case CPL_NEWINQUIRE:
            return CplNewInquire( (LPNEWCPLINFO)lparam2, (int)lparam1 );

        case CPL_DBLCLK:
            lparam2 = 0L;
            // fall through...
        case CPL_STARTWPARMS:
            return CplInvoke( parent, (int)lparam1, (LPSTR)lparam2 );
    }

    return 0L;
}
