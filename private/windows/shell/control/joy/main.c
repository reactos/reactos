//****************************************************************************
//
//  File:       main.c
//  Content:    main cpl control code
//  History:
//   Date	By	Reason
//   ====	==	======
//   29-nov-94	craige	initial implementation
//   11-dec-94	craige	added ShowJoyCPL
//
//  Copyright (c) Microsoft Corporation 1994-1995
//
//****************************************************************************
#include "joycpl.h"

HINSTANCE hInstance = NULL;
LPCTSTR cszHelpFile = TEXT("joy.hlp");

#define JOYSTICK_CPL	0
#define MAX_PAGES	16	/* maximum number of joysticks supported */

#ifdef DEBUG
void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    char str[256];

    wsprintf( str, "JOYCPL: " );
    wvsprintf( str+lstrlen( str ), szFormat, (LPVOID)(&szFormat+1) );

    OutputDebugString( str );
}
#endif

/*
 * LibMain - main entry point for DLL
 */
BOOL APIENTRY LibMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{
    switch( dwReason ) {
    case DLL_PROCESS_ATTACH:
    	hInstance = hDll;
    	DPF( "DLL_PROCESS_ATTACH: hInstance = %08lx\r\n" );
    	break;
    case DLL_PROCESS_DETACH:
    	DPF( "DLL_PROCESS_DETACH: hInstance = %08lx\r\n" );
    	break;
    case DLL_THREAD_DETACH:
    	DPF( "DLL_THREAD_DETACH: hInstance = %08lx\r\n" );
        break;
    case DLL_THREAD_ATTACH:
    	DPF( "DLL_THREAD_DETACH: hInstance = %08lx\r\n" );
    	break;
    default:
    	break;
    }

    return TRUE;

} /* LibMain */

/*
 * startJoyCPL - start the joystick CPL
 */
static void startJoyCPL( HWND hwnd )
{
    PROPSHEETHEADER	psh;
    char		title[MAX_STR];
    PROPSHEETPAGE	psp;
    LPJOYDATA		pjd;
    #if defined( WANT_SHEETS )
	JOYDATAPTR	jdp[MAX_PAGES];
	HPROPSHEETPAGE	hpsp[MAX_PAGES];
	int		numsheets;
	int		i;
	char		str[MAX_STR];
    #else
	JOYDATAPTR	jdp;
	HPROPSHEETPAGE	hpsp[1];
    #endif

    #if defined(WANT_SHEETS)
	numsheets = joyGetNumDevs();
	if( numsheets == 0 ) {
	    return;
	}
    #endif

    InitCommonControls();
    LoadString( hInstance, IDS_JOY, title, sizeof(title));

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hwnd;
    psh.hInstance = hInstance;
    psh.pszCaption = (LPSTR) MAKEINTRESOURCE( IDS_JOY );
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = hpsp;

    psp.dwSize = sizeof( PROPSHEETPAGE );
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE;
    psp.hInstance = hInstance;
    psp.pszTemplate = MAKEINTRESOURCE( IDD_JOYSTICK );
    psp.pszIcon = NULL;
    psp.pfnDlgProc = JoystickDlg;
    psp.pfnCallback = NULL;
    psp.pcRefParent = NULL;
    pjd = JoystickDataInit();
    #if defined(WANT_SHEETS)
	for( i=0;i<numsheets;i++ ) {
	    wsprintf( str, "%s %d", title, i+1 );
	    psp.pszTitle = str;
	    jdp[i].pjd = pjd;
	    jdp[i].iJoyId = i;
	    psp.lParam = (LPARAM) &jdp[i];
	    if( psh.phpage[ psh.nPages ] = CreatePropertySheetPage( &psp ) ) {
		psh.nPages++;
		DPF( "PropertySheetPage()\r\n" );
	    }
	}
    #else
	psp.pszTitle = title;
	jdp.pjd = pjd;
	jdp.iJoyId = JOYSTICKID1;
	psp.lParam = (LPARAM) &jdp;
	if( psh.phpage[ psh.nPages ] = CreatePropertySheetPage( &psp ) ) {
	    psh.nPages++;
	    DPF( "PropertySheetPage()\r\n" );
	}
    #endif
    if( psh.nPages ) {
	PropertySheet( &psh );
	DPF( "PropertySheet()\r\n" );
    }
    JoystickDataFini( pjd );

} /* startJoyCPL */

/*
 * ShowJoyCPL - exported function to allow apps to show the joystick CPL
 */
void WINAPI ShowJoyCPL( HWND hwnd )
{
    HWND    hwnd_parent;

    hwnd_parent = hwnd;
    if( hwnd != NULL ) {
	if( GetWindowLong( hwnd, GWL_EXSTYLE ) & WS_EX_TOPMOST ) {
	    hwnd_parent = NULL;
	}
    }
    startJoyCPL( hwnd_parent );

} /* ShowJoyCPL */

/*
 * CPlApplet - applet manager
 */
LONG WINAPI CPlApplet(HWND hwnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    LPCPLINFO		pcplinfo;
    LPNEWCPLINFO	pnewcplinfo;

    switch( uMsg ) {
    case CPL_INIT:
    	DPF( "CPL_INIT:\r\n" );
        // return TRUE; // (fall through to CPL_GETCOUNT--fail if no joysticks)
    
    case CPL_GETCOUNT:
        /*
	 * number  of applets in this DLL - 1 per device
	 * no joystick devices installed (i.e., no CPL's to display)
	 */
	if( joyGetNumDevs() ) {
	    DPF( "CPL_GETCOUNT = 1\r\n" );
	    return 1;
	} else {
	    DPF( "CPL_GETCOUNT = 0\r\n" );
	    return 0;
	}

    case CPL_INQUIRE:
        /*
	 * Fill the CPLINFO with the pertinent information for each applet
	 */
    	DPF( "CPL_INQUIRE:\r\n" );
	pcplinfo = (LPCPLINFO) lParam2;
        switch( lParam1 ) {
        case JOYSTICK_CPL:
            pcplinfo->idIcon = CPL_DYNAMIC_RES;
            pcplinfo->idName = CPL_DYNAMIC_RES;
            pcplinfo->idInfo = CPL_DYNAMIC_RES;
            break;
        }
        pcplinfo->lData = 0L;
        return TRUE;

    case CPL_NEWINQUIRE:
    	DPF( "CPL_NEWINQUIRE\r\n" );
    	pnewcplinfo = (LPNEWCPLINFO) lParam2;
    	switch( lParam1 ) {
	case JOYSTICK_CPL:
	    pnewcplinfo->hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_JOYSTICK));
	    LoadString(hInstance, IDS_JOY, pnewcplinfo->szName,
	    			sizeof(pnewcplinfo->szName));
	    LoadString( hInstance, IDS_JOYINFO, pnewcplinfo->szInfo,
	    			sizeof(pnewcplinfo->szInfo) );
	    DPF( "hIcon = %04x\r\n", pnewcplinfo->hIcon );
	    DPF( "szName = \"%s\"\r\n", pnewcplinfo->szName );
	    DPF( "szInfo = \"%s\"\r\n", pnewcplinfo->szInfo );
	    break;
	}
        pnewcplinfo->dwHelpContext = 0;
        pnewcplinfo->dwSize = sizeof( NEWCPLINFO );
        pnewcplinfo->lData = 0L;
        pnewcplinfo->szHelpFile[0] = 0;
        return TRUE;

    case CPL_DBLCLK:
    	DPF( "CPL_DBLCLK\r\n" );
        switch( lParam1 ) {
        case JOYSTICK_CPL:
	    startJoyCPL( hwnd );
            break;
        }
        break;      

    case CPL_EXIT:
    	DPF( "CPL_EXIT\r\n" );
        break;
            
    default:
        return 0L;
    }

    return 1L;

} /* CPlApplet */
