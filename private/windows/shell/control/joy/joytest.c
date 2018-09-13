//****************************************************************************
//
//  File:       joytest.c
//  Content:    Joystick test dialog
//  History:
//   Date	By	Reason
//   ====	==	======
//   11-dec-94	craige	split out of joycpl.c; some tweaks
//   15-dec-94	craige	allow N joysticks
//
//  Copyright (c) Microsoft Corporation 1994-1995
//
//****************************************************************************

#include "joycpl.h"

/*
 * variables used by test process
 */
typedef struct {
    LPGLOBALVARS	pgv;
    MMRESULT		mmr_capture;
    HWND		hwnd;
    BOOL		bHasTimer;
    BOOL		bUseTimer;
    int			iButtonShift;
    JOYINFOEX		ji;
} test_vars, *LPTESTVARS;

/*
 * fillButton - light up a specific button
 */
static void fillButton( LPGLOBALVARS pgv, HWND hwnd, int id, BOOL isup )
{
    HWND	hwb;
    RECT	r;
    HDC		hdc;

    hwb = GetDlgItem( hwnd, id );
    if( hwb == NULL ) {
	return;
    }
    hdc = GetDC( hwb );
    if( hdc == NULL ) {
	return;
    }
    GetClientRect( hwb, &r );
    if( isup ) {
	FillRect( hdc, &r, pgv->pjd->hbUp );
    } else {
	FillRect( hdc, &r, pgv->pjd->hbDown );
    }
    ReleaseDC( hwb, hdc );

} /* fillButton */

/*
 * doTestButton - try to light the relevant buttons
 */
static void doTestButton( LPTESTVARS ptv, HWND hwnd, LPJOYINFOEX pji )
{

    if( (ptv->ji.dwButtons & JOY_BUTTON1) != (pji->dwButtons & JOY_BUTTON1) ) {
	fillButton( ptv->pgv, hwnd, IDC_JOYB1+ptv->iButtonShift,
				pji->dwButtons & JOY_BUTTON1 );
    }
    if( (ptv->ji.dwButtons & JOY_BUTTON2) != (pji->dwButtons & JOY_BUTTON2) ) {
	fillButton( ptv->pgv, hwnd, IDC_JOYB2+ptv->iButtonShift,
				pji->dwButtons & JOY_BUTTON2 );
    }
    if( ptv->iButtonShift == 0 ) {
	if( (ptv->ji.dwButtons & JOY_BUTTON3) != (pji->dwButtons & JOY_BUTTON3) ) {
	    fillButton( ptv->pgv, hwnd, IDC_JOYB3, pji->dwButtons & JOY_BUTTON3 );
	}
	if( (ptv->ji.dwButtons & JOY_BUTTON4) != (pji->dwButtons & JOY_BUTTON4) ) {
	    fillButton( ptv->pgv, hwnd, IDC_JOYB4, pji->dwButtons & JOY_BUTTON4 );
	}
    }
    ptv->ji.dwButtons = pji->dwButtons;

} /* doTestButton */

/*
 * doTestPOV - try to light the POV indicators
 */
static void doTestPOV( LPTESTVARS ptv, HWND hwnd, LPJOYINFOEX pji )
{
    int		idi;

    if( ptv->ji.dwPOV != pji->dwPOV ) {
	idi = IDI_JOYPOV_NONE;
	if( pji->dwPOV == JOY_POVFORWARD ) {
	    idi = IDI_JOYPOV_UP;
	} else if( pji->dwPOV == JOY_POVBACKWARD ) {
	    idi = IDI_JOYPOV_DOWN;
	} else if( pji->dwPOV == JOY_POVLEFT ) {
	    idi = IDI_JOYPOV_LEFT;
	} else if( pji->dwPOV == JOY_POVRIGHT ) {
	    idi = IDI_JOYPOV_RIGHT;
	}
    	ChangeIcon( hwnd, idi, IDC_JOYPOV );
    }
    ptv->ji.dwPOV = pji->dwPOV;

} /* doTestPOV */

/*
 * joyTestInitDialog - init the testing dialog
 */
static BOOL joyTestInitDialog( HWND hwnd, LPARAM lParam)
{
    HINSTANCE		hinst;
    char		str[MAX_STR];
    LPJOYREGHWCONFIG	pcfg;
    LPTESTVARS		ptv;
    LPGLOBALVARS	pgv;

    /*
     * create test vars
     */
    ptv = DoAlloc( sizeof( test_vars ) );
    SetWindowLong( hwnd, DWL_USER, (LONG) ptv );
    if( ptv == NULL ) {
	return FALSE;
    }
    pgv = (LPGLOBALVARS) lParam;
    ptv->pgv = pgv;
    ptv->hwnd = hwnd;

    /*
     * set dialog text based on OEM strings
     */
    SetOEMText( pgv, hwnd, TRUE );

    /*
     * customize test dialog's button display
     */
    pcfg = &pgv->joyHWCurr;
    if( pcfg->hws.dwNumButtons <= 2 ) {
	ptv->iButtonShift = 1;
	ShowWindow( GetDlgItem( hwnd, IDC_JOYB1 ), SW_HIDE );
	ShowWindow( GetDlgItem( hwnd, IDC_JOYB4 ), SW_HIDE );
	ShowWindow( GetDlgItem( hwnd, IDC_JOYB1_LABEL ), SW_HIDE );
	ShowWindow( GetDlgItem( hwnd, IDC_JOYB4_LABEL ), SW_HIDE );
	hinst = GetWindowInstance( hwnd );
	if( LoadString( hinst, IDS_JOYBUTTON1, str, sizeof( str ) ) ) {
	    SetWindowText( GetDlgItem( hwnd, IDC_JOYB2_LABEL ), str );
	}
	if( LoadString( hinst, IDS_JOYBUTTON2, str, sizeof( str ) ) ) {
	    SetWindowText( GetDlgItem( hwnd, IDC_JOYB3_LABEL ), str );
	}
    } else {
	ptv->iButtonShift = 0;
    }

    ShowControls( pcfg, hwnd );
    
    /*
     * other misc setup
     */
    ptv->bHasTimer = SetTimer( hwnd, TIMER_ID, JOYPOLLTIME, NULL );
    ptv->bUseTimer = TRUE;
    if( !ptv->bHasTimer ) {
	DPF( "No timer for joystick test!\r\n" );
	return FALSE;
    }
    
    return TRUE;

} /* joyTestInitDialog */

/*
 * context help for the test dialog
 */
    const static DWORD aTestHelpIDs[] = {  // Context Help IDs
        IDC_GROUPBOX,         IDH_JOYSTICK_GROUPBOX,
        IDC_JOYLIST1_LABEL,   IDH_JOYSTICK_TEST_RANGE,
        IDC_JOYLIST1,         IDH_JOYSTICK_TEST_RANGE,
        IDC_JOYLIST2_LABEL,   IDH_JOYSTICK_TEST_THROTTLE,
        IDC_JOYLIST2,         IDH_JOYSTICK_TEST_THROTTLE,
        IDC_JOYLIST3_LABEL,   IDH_JOYSTICK_TEST_RUDDER,
        IDC_JOYLIST3,         IDH_JOYSTICK_TEST_RUDDER,
        IDC_JOYPOV_LABEL,     IDH_JOYSTICK_TEST_POV,
        IDC_JOYPOV,           IDH_JOYSTICK_TEST_POV,
        IDC_TEXT_1,           NO_HELP,
        IDC_TEXT_2,           NO_HELP,
        IDC_ICON_1,           NO_HELP,
        IDC_GROUPBOX_2,       IDH_JOYSTICK_TEST_BUTTONS,
        IDC_JOYB1_LABEL,      IDH_JOYSTICK_TEST_BUTTONS,
        IDC_JOYB2_LABEL,      IDH_JOYSTICK_TEST_BUTTONS,
        IDC_JOYB3_LABEL,      IDH_JOYSTICK_TEST_BUTTONS,
        IDC_JOYB4_LABEL,      IDH_JOYSTICK_TEST_BUTTONS,
        IDC_JOYB1,            IDH_JOYSTICK_TEST_BUTTONS,
        IDC_JOYB2,            IDH_JOYSTICK_TEST_BUTTONS,
        IDC_JOYB3,            IDH_JOYSTICK_TEST_BUTTONS,
        IDC_JOYB4,            IDH_JOYSTICK_TEST_BUTTONS,
        IDC_JOYLIST4_LABEL,   IDH_JOYSTICK_TEST_THROTTLE,
        IDC_JOYLIST4,         IDH_JOYSTICK_TEST_THROTTLE,
        IDC_JOYLIST5_LABEL,   NO_HELP,
        IDC_JOYLIST5,         NO_HELP,
        0, 0
    };

/*
 * TestProc - callback procedure for joystick test dialog
 */
BOOL CALLBACK TestProc( HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    BOOL	rc;
    switch( umsg ) {
    case WM_TIMER:
    {
	LPTESTVARS	ptv;
	ptv = (LPTESTVARS) GetWindowLong( hwnd, DWL_USER );

    	if( ptv->bUseTimer ) {
	    JOYINFOEX	ji;
	    MMRESULT	rc;
	    ptv->bUseTimer = FALSE;
	    ji.dwSize = sizeof( ji );
//	    ji.dwFlags = JOY_RETURNALL | JOY_USEDEADZONE | JOY_RETURNCENTERED;
	    ji.dwFlags = JOY_RETURNALL | JOY_RETURNCENTERED;
	    rc = joyGetPosEx( ptv->pgv->iJoyId, &ji );
	    if( rc == JOYERR_NOERROR ) {
		DoJoyMove( ptv->pgv, hwnd, &ji, &ptv->ji, JOYMOVE_DRAWALL );
		doTestButton( ptv, hwnd, &ji );
		doTestPOV( ptv, hwnd, &ji );
		ptv->bUseTimer = TRUE;
	    } else {
		if( JoyError( hwnd ) ) {
		    ptv->bUseTimer = TRUE;
		}
	    }
	}
	break;
    }
	
    case WM_DESTROY:
    {
	LPTESTVARS	ptv;
	ptv = (LPTESTVARS) GetWindowLong( hwnd, DWL_USER );
	DoFree( ptv );
	break;
    }
	
    case WM_INITDIALOG:
	rc = joyTestInitDialog( hwnd, lParam );
	if( !rc ) {
	    EndDialog( hwnd, 0 );
	}
	return FALSE;

    case WM_PAINT:
    {
	LPTESTVARS	ptv;
	ptv = (LPTESTVARS) GetWindowLong( hwnd, DWL_USER );
    	CauseRedraw( &ptv->ji, TRUE );
	return FALSE;
    }

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, cszHelpFile,
            HELP_WM_HELP, (DWORD)(LPSTR) aTestHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, cszHelpFile, HELP_CONTEXTMENU,
            (DWORD)(LPVOID) aTestHelpIDs);
        return TRUE;

    case WM_COMMAND:
    {
	int 		id;
	LPTESTVARS	ptv;

	ptv = (LPTESTVARS) GetWindowLong( hwnd, DWL_USER );
	id = GET_WM_COMMAND_ID(wParam, lParam);
	switch( id ) {
	case IDCANCEL:
	case IDOK:
	    if( ptv->bHasTimer ) {
		KillTimer( hwnd, TIMER_ID );
	    }
	    EndDialog(hwnd, (id == IDOK));
	    break;
	    
	default:
	    break;
	}
	break;
    }
    default:
	break;
    }
    return FALSE;

} /* TestProc */

/*
 * DoTest - do the test dialog
 */
void DoTest( LPGLOBALVARS pgv, HWND hwnd, LPUPDCFGFN pupdcfgfn, LPVOID pparm )
{
    JOYREGHWCONFIG	save_joycfg;
    int			id;

    /*
     * save the current config, and then update config if required
     */
    save_joycfg = pgv->joyHWCurr;
    if( pupdcfgfn != NULL ) {
	pupdcfgfn( pparm );
    }

    /*
     * update the registry with our new joystick info
     */
    RegSaveCurrentJoyHW( pgv );
    RegistryUpdated( pgv );

    /*
     * process the test dialog
     */
    if( pgv->joyHWCurr.hws.dwFlags & (JOY_HWS_HASU|JOY_HWS_HASV) ) {
	id = IDD_JOYTEST1;
    } else {
	id = IDD_JOYTEST;
    }
    DialogBoxParam( (HINSTANCE)GetWindowLong( hwnd, GWL_HINSTANCE ),
		MAKEINTRESOURCE( id ), hwnd, TestProc, (LONG) pgv );

    /*
     * restore the old registry info
     */
    pgv->joyHWCurr = save_joycfg;
    RegSaveCurrentJoyHW( pgv );
    RegistryUpdated( pgv );

} /* DoTest */
