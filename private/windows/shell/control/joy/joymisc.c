//****************************************************************************
//
//  File:       joymisc.c
//  Content:    Misc routines used by calibration and testing dialogs
//  History:
//   Date	By	Reason
//   ====	==	======
//   11-dec-94	craige	split out of joycpl.c
//   15-dec-94	craige	allow N joysticks
//   18-dec-94	craige	process U&V
//   04-mar-95	craige	bug 13147: crosshair should erase background color
//
//  Copyright (c) Microsoft Corporation 1994-1995
//
//****************************************************************************

#include "joycpl.h"

/*
 * ADJ_VAL is used to convert a joystick position into a value in
 * a new range
 */
#define ADJ_VAL( a, pos, range ) (((pos-(pgv->joyRange.jpMin.dw##a))*range)/ \
			(pgv->joyRange.jpMax.dw##a-pgv->joyRange.jpMin.dw##a+1))


/*
 * setOEMWindowText - set window text with an OEM string
 */
static void setOEMWindowText( HWND hwnd, int id, LPSTR str )
{
    HWND	hwndctl;
    if( str[0] != 0 ) {
	hwndctl = GetDlgItem( hwnd, id );
	if( hwndctl != NULL ) {
	    SetWindowText( hwndctl, str );
	}
    }

} /* setOEMWindowText */

/*
 * SetOEMText - set OEM defined text in the dialogs
 */
void SetOEMText( LPGLOBALVARS pgv, HWND hwnd, BOOL istest )
{
    DWORD	type;
    char	str[MAX_STR];
    char	res[MAX_STR];
    HINSTANCE	hinst;
    int		id;
    LPSTR	pwincap;
    LPJOYDATA	pjd;

    pjd = pgv->pjd;

    /*
     * get the default window caption.   this will be replaced by
     * an OEM string if it is avaliable.
     */
    hinst = GetWindowInstance( hwnd );
    if( istest ) {
	id = IDS_JOYTESTCAPN;
    } else {
	id = IDS_JOYCALCAPN;
    }
    if( !LoadString( hinst, id, str, sizeof( str ) ) ) {
	res[0] = 0;
    } else {
	wsprintf( res, str, pgv->iJoyId+1 );
    }
    pwincap = res;

    /*
     * if this is an OEM joystick, use any strings that they may have defined
     */
    if( pgv->joyHWCurr.dwUsageSettings & JOY_US_ISOEM ) {
	type = pgv->joyHWCurr.dwType - JOY_HW_LASTENTRY;
	/*
	 * set up labels under each of the controls
	 */
	setOEMWindowText( hwnd, IDC_JOYLIST1_LABEL, pjd->oemList[type].xy_label );
	setOEMWindowText( hwnd, IDC_JOYLIST2_LABEL, pjd->oemList[type].z_label );
	setOEMWindowText( hwnd, IDC_JOYLIST3_LABEL, pjd->oemList[type].r_label );
	setOEMWindowText( hwnd, IDC_JOYLIST4_LABEL, pjd->oemList[type].u_label );
	setOEMWindowText( hwnd, IDC_JOYLIST5_LABEL, pjd->oemList[type].v_label );
	setOEMWindowText( hwnd, IDC_JOYPOV_LABEL, pjd->oemList[type].pov_label );
	if( istest ) {
	    /*
	     * set the various caption and description fields in the test dlg
	     */
	    setOEMWindowText( hwnd, IDC_TEXT_1, pjd->oemList[type].testmove_desc );
	    setOEMWindowText( hwnd, IDC_TEXT_2, pjd->oemList[type].testbutton_desc );
	    setOEMWindowText( hwnd, IDC_GROUPBOX, pjd->oemList[type].testmove_cap );
	    setOEMWindowText( hwnd, IDC_GROUPBOX_2, pjd->oemList[type].testbutton_cap );
	    if( pjd->oemList[type].testwin_cap[0] != 0 ) {
		pwincap = pjd->oemList[type].testwin_cap;
	    }
	} else {
	    /*
	     * set the various caption and description fields in the
	     * calibration dialog
	     */
	    setOEMWindowText( hwnd, IDC_GROUPBOX, pjd->oemList[type].cal_cap );
	    if( pjd->oemList[type].calwin_cap[0] != 0 ) {
		pwincap = pjd->oemList[type].calwin_cap;
	    }
	}
    }

    /*
     * set the window caption
     */
    if( pwincap[0] != 0 ) {
	SetWindowText( hwnd, pwincap );
    }

} /* SetOEMText */

/*
 * ShowControls - show Z and R controls, based on configuration info
 */
void ShowControls( LPJOYREGHWCONFIG pcfg, HWND hwnd )
{
    HWND	hwndctl;
    /*
     * hide Z indicatior if there is no Z axis
     */
    if( !(pcfg->hws.dwFlags & JOY_HWS_HASZ) ) {
	ShowWindow( GetDlgItem( hwnd, IDC_JOYLIST2 ), SW_HIDE );
	ShowWindow( GetDlgItem( hwnd, IDC_JOYLIST2_LABEL ), SW_HIDE );
    }

    /*
     * hide R indicatior if there is no R axis or rudder
     */
    if( !(pcfg->hws.dwFlags & JOY_HWS_HASR) && !(pcfg->dwUsageSettings & JOY_US_HASRUDDER) ) {
	ShowWindow( GetDlgItem( hwnd, IDC_JOYLIST3 ), SW_HIDE );
	ShowWindow( GetDlgItem( hwnd, IDC_JOYLIST3_LABEL ), SW_HIDE );
    }

    /*
     * hide POV indicatior if there is no POV
     */
    if( !(pcfg->hws.dwFlags & JOY_HWS_HASPOV) ) {
	ShowWindow( GetDlgItem( hwnd, IDC_JOYPOV ), SW_HIDE );
	ShowWindow( GetDlgItem( hwnd, IDC_JOYPOV_LABEL ), SW_HIDE );
    }

    /*
     * hide U indicatior if there is no U axis
     */
    if( !(pcfg->hws.dwFlags & JOY_HWS_HASU) ) {
	hwndctl = GetDlgItem( hwnd, IDC_JOYLIST4 );
	if( hwndctl != NULL ) {
	    ShowWindow( hwndctl, SW_HIDE );
	    ShowWindow( GetDlgItem( hwnd, IDC_JOYLIST4_LABEL ), SW_HIDE );
	}
    }

    /*
     * hide V indicatior if there is no V axis
     */
    if( !(pcfg->hws.dwFlags & JOY_HWS_HASV) ) {
	hwndctl = GetDlgItem( hwnd, IDC_JOYLIST5 );
	if( hwndctl != NULL ) {
	    ShowWindow( hwndctl, SW_HIDE );
	    ShowWindow( GetDlgItem( hwnd, IDC_JOYLIST5_LABEL ), SW_HIDE );
	}
    }

} /* ShowControls */

/*
 * JoyError - error reading the joystick
 */
BOOL JoyError( HWND hwnd )
{
    char	str1[MAX_STR];
    char	str2[MAX_STR];
    int		rc;
    HINSTANCE	hinst;

    hinst = GetWindowInstance( hwnd );
    rc = IDCANCEL;
    if( LoadString( hinst, IDS_JOYREADERROR, str1, sizeof( str1 ) ) ) {
	if( LoadString( hinst, IDS_JOYUNPLUGGED, str2, sizeof( str2 ) ) ) {
	    rc = MessageBox( hwnd, str2, str1,
	    		MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL );
	}
    }
    if( rc == IDCANCEL ) {
	/*
	 * terminate the dialog if we give up
	 */
	PostMessage( hwnd, WM_COMMAND, IDCANCEL, 0 );
	return FALSE;
    }
    return TRUE;

} /* JoyError */

/*
 * ChangeIcon - change the icon of a static control
 */
void ChangeIcon( HWND hwnd, int idi, int idc )
{

    HINSTANCE	hinst;
    HICON	hicon;
    HICON	holdicon;

    hinst = GetWindowInstance( hwnd );
    hicon = LoadIcon( hinst, MAKEINTRESOURCE(idi) );
    if( hicon != NULL ) {
	holdicon = Static_SetIcon( GetDlgItem(hwnd,idc), hicon );
	if( holdicon != NULL ) {
	    DestroyIcon( holdicon );
	}
    }

} /* ChangeIcon */

/*
 * CauseRedraw - cause test or calibrate dialogs to redraw their controls
 */
void CauseRedraw( LPJOYINFOEX pji, BOOL do_buttons )
{
    pji->dwXpos = (DWORD) -1;
    pji->dwYpos = (DWORD) -1;
    pji->dwZpos = (DWORD) -1;
    pji->dwRpos = (DWORD) -1;
    pji->dwPOV = JOY_POVCENTERED;
    if( do_buttons ) {
	pji->dwButtons = ALL_BUTTONS;
    }

} /* CauseRedraw */

/*
 * fillBar - fill the bar for indicating Z or R info
 */
static void fillBar( LPGLOBALVARS pgv, HWND hwnd, DWORD pos, int id )
{
    HWND	hwlb;
    RECT	r;
    HDC		hdc;
    int		height;
    LPJOYDATA	pjd;

    pjd = pgv->pjd;

    /*
     * scale the height to be inside the bar window
     */
    hwlb = GetDlgItem( hwnd, id );
    if( hwlb == NULL ) {
	return;
    }
    hdc = GetDC( hwlb );
    if( hdc == NULL ) {
	return;
    }
    GetClientRect( hwlb, &r );

    switch( id ) {
    case IDC_JOYLIST2:
	height = ADJ_VAL( Z, pos, r.bottom );
	break;
    case IDC_JOYLIST3:
	height = ADJ_VAL( R, pos, r.bottom );
	break;
    case IDC_JOYLIST4:
	height = ADJ_VAL( U, pos, r.bottom );
	break;
    case IDC_JOYLIST5:
	height = ADJ_VAL( V, pos, r.bottom );
	break;
    }

    /*
     * fill in the inactive area
     */
    r.top = height;
    FillRect( hdc, &r, pjd->hbUp );

    /*
     * fill in the active area
     */
    r.top = 0;
    r.bottom = height;
    FillRect( hdc, &r, pjd->hbDown );

    ReleaseDC( hwlb, hdc );

} /* fillBar */

#define DELTA	5
/*
 * drawCross - draw a cross in the position box
 */
static void drawCross( HWND hwnd, int x, int y, int obj )
{
    HDC		hdc;
    HPEN	hpen;
    HPEN	holdpen;

    hdc = GetDC( hwnd );
    if( hdc == NULL ) {
	return;
    }
    if( obj == -1 ) {
	COLORREF	cr;
	cr = GetSysColor( COLOR_WINDOW );
	hpen = CreatePen( PS_SOLID, 0, cr );
    } else {
	hpen = GetStockObject( obj );
    }
    if( hpen == NULL ) {
	ReleaseDC( hwnd, hdc );
	return;
    }
    holdpen = SelectObject( hdc, hpen );
    MoveToEx( hdc, x-(DELTA-1), y, NULL );
    LineTo( hdc, x+DELTA, y );
    MoveToEx( hdc, x, y-(DELTA-1), NULL );
    LineTo( hdc, x, y+DELTA );
    SelectObject( hdc, holdpen );
    if( obj == -1 ) {
	DeleteObject( hpen );
    }
    ReleaseDC( hwnd, hdc );

} /* drawCross */

#define FILLBAR( a, id ) \
    /* \
     * make sure we aren't out of alleged range \
     */ \
    if( pji->dw##a##pos > pgv->joyRange.jpMax.dw##a ) { \
	pji->dw##a##pos = pgv->joyRange.jpMax.dw##a; \
    } else if( pji->dw##a##pos < pgv->joyRange.jpMin.dw##a ) { \
	pji->dw##a##pos = pgv->joyRange.jpMin.dw##a; \
    } \
 \
    /* \
     * fill the bar if we haven't moved since last time \
     */ \
    if( pji->dw##a##pos != poji->dw##a##pos ) { \
	fillBar( pgv, hwnd, pji->dw##a##pos, id ); \
	poji->dw##a##pos = pji->dw##a##pos; \
    }

/*
 * DoJoyMove - process movement for the joystick 
 */
void DoJoyMove( LPGLOBALVARS pgv, HWND hwnd, LPJOYINFOEX pji,
		       LPJOYINFOEX poji, DWORD drawflags )
{
    HWND	hwlb;
    RECT	rc;
    int		width;
    int		height;
    DWORD	x,y;

    /*
     * draw the cross in the XY box if needed
     */
    if( drawflags & JOYMOVE_DRAWXY ) {
	/*
	 * make sure we aren't out of alleged range
	 */
	if( pji->dwXpos > pgv->joyRange.jpMax.dwX ) {
	    pji->dwXpos = pgv->joyRange.jpMax.dwX;
	} else if( pji->dwXpos < pgv->joyRange.jpMin.dwX ) {
	    pji->dwXpos = pgv->joyRange.jpMin.dwX;
	}
	if( pji->dwYpos > pgv->joyRange.jpMax.dwY ) {
	    pji->dwYpos = pgv->joyRange.jpMax.dwY;
	} else if( pji->dwYpos < pgv->joyRange.jpMin.dwY ) {
	    pji->dwYpos = pgv->joyRange.jpMin.dwY;
	}

	/*
	 * convert info to (x,y) position in window
	 */
	hwlb = GetDlgItem( hwnd, IDC_JOYLIST1 );
	GetClientRect( hwlb, &rc );
	height = rc.bottom - rc.top-2*DELTA;
	width = rc.right - rc.left-2*DELTA;
	x = ADJ_VAL( X, pji->dwXpos, width ) + DELTA;
	y = ADJ_VAL( Y, pji->dwYpos, height ) + DELTA;

	/*
	 * only draw the cross if it has moved since last time
	 */
	if( x != (DWORD) poji->dwXpos || y != (DWORD) poji->dwYpos ) {
	    if( poji->dwXpos != (DWORD) -1 ) {
//		drawCross( hwlb, (int) poji->dwXpos, (int) poji->dwYpos, WHITE_PEN );
		drawCross( hwlb, (int) poji->dwXpos, (int) poji->dwYpos, -1 );
	    }
	    drawCross( hwlb, (int) x, (int) y, BLACK_PEN );
	    poji->dwXpos = x;
	    poji->dwYpos = y;
	}
    }

    /*
     * draw Z bar if needed
     */
    if( drawflags & JOYMOVE_DRAWZ ) {
	FILLBAR( Z, IDC_JOYLIST2 );
    }

    /*
     * draw R bar if needed
     */
    if( drawflags & JOYMOVE_DRAWR ) {
	FILLBAR( R, IDC_JOYLIST3 );
    }

    /*
     * draw U bar if needed
     */
    if( drawflags & JOYMOVE_DRAWU ) {
	FILLBAR( U, IDC_JOYLIST4 );
    }

    /*
     * draw V bar if needed
     */
    if( drawflags & JOYMOVE_DRAWV ) {
	FILLBAR( V, IDC_JOYLIST5 );
    }

} /* DoJoyMove */
