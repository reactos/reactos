//****************************************************************************
//
//  File:       joycal.c
//  Content:    Joystick calibration dialog
//  History:
//   Date	By	Reason
//   ====	==	======
//   11-dec-94	craige	split out of joycpl.c; some tweaks
//   15-dec-94	craige	allow N joysticks
//   17-dec-94	craige	new UI as requested by ChrisB
//   18-dec-94	craige	process UV
//   05-jan-95	craige	new centering confirmation messages
//   04-mar-95	craige	bug 10761 - separate strings for pluralization
//			bug 12036 - now works when "Back" clicked off of
//				    custom 4-axis with POV hat
//
//  Copyright (c) Microsoft Corporation 1994-1995
//
//****************************************************************************

#include "joycpl.h"

/*
 * This has the look and feel of a wizard, but isn't   This leads to the
 * obvious...
 *
 * Q: Why isn't this a "real" wizard?
 *
 * A: - it doesn't have multiple pages, it has a single page.  the user
 *	sees different joystick items activate and de-activate on the dialog
 *	as he/she calibrates each axis. fussing with multiple sheets for each
 *	axis would be confusing and unnecessary.
 */

/*
 * calibration states
 */
typedef enum {
    JCS_INIT=-1,
    JCS_XY_CENTER1,
    JCS_XY_MOVE,
    JCS_XY_CENTER2,
    JCS_Z_MOVE,
    JCS_Z_PLACEHOLDER,
    JCS_R_MOVE,
    JCS_R_PLACEHOLDER,
    JCS_U_MOVE,
    JCS_U_PLACEHOLDER,
    JCS_V_MOVE,
    JCS_V_PLACEHOLDER,
    JCS_POV_MOVEUP,
    JCS_POV_MOVERIGHT,
    JCS_POV_MOVEDOWN,
    JCS_POV_MOVELEFT,
    JCS_FINI
} cal_states;

typedef enum {
    JC_XY=0,
    JC_Z,
    JC_POV_UP,
    JC_POV_RIGHT,
    JC_POV_DOWN,
    JC_POV_LEFT,
    JC_R,
    JC_U,
    JC_V,
    JC_FINI
} cal_wins;

/*
 * variables used in calibration
 */
typedef struct {
    LPGLOBALVARS	pgv;
    cal_states		cState;
    BOOL		bHasTimer;
    BOOL		bUseTimer;
    HINSTANCE		hinst;
    JOYINFOEX		ji;
    JOYRANGE		jr;
    DWORD		pov[JOY_POV_NUMDIRS];
    int			iAxisCount;
    BOOL		bPOVdone;
} CALVARS, *LPCALVARS;

#define JOY_CALIB_FLAGS	JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | \
			JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | \
			JOY_RETURNBUTTONS | JOY_RETURNRAWDATA

/*
 * setDefaultButton - make a button the default window
 */
static void setDefaultButton( HWND hwnd, HWND hwdb )
{
    DWORD	style;
    int		i;
    HWND	hwb;
    int		idList[] = { IDC_JOYCALBACK,
			     IDC_JOYCALNEXT,
			     IDC_JOYPICKPOV,
			     IDC_JOYCALDONE,
			     IDC_JOYTEST };
    /*
     * turn off the current default push button
     */
    for( i=0;i<sizeof( idList )/sizeof( idList[0] );i++ ) {
	hwb = GetDlgItem( hwnd, idList[i] );
	style = GetWindowLong( hwb, GWL_STYLE );
	if( style & BS_DEFPUSHBUTTON ) {
	    style &= ~BS_DEFPUSHBUTTON;
	    style |= BS_PUSHBUTTON;
	    SetWindowLong( hwb, GWL_STYLE, style );
	}
    }

    /*
     * make the specified button the default
     */
    style = GetWindowLong( hwdb, GWL_STYLE );
    style &= ~(BS_PUSHBUTTON|BS_DEFPUSHBUTTON);
    style |= BS_DEFPUSHBUTTON;
    SetWindowLong( hwdb, GWL_STYLE, style );

} /* setDefaultButton */

/*
 * setLabel
 *
 * set the label for an axis based on current calibration state
 */
static void setLabel(
		LPGLOBALVARS pgv,
		HWND hwnd,
		UINT id,
		LPJOYREGHWCONFIG pcfg,
		DWORD bit  )
{
    char	str[MAX_STR];
    char	calstr[MAX_STR];
    int		type;
    HINSTANCE	hinst;
    HWND	hwtext;

    hinst = GetWindowInstance( hwnd );

    /*
     * get text for this axis label...
     */
    if( pcfg->dwUsageSettings & JOY_US_ISOEM ) {
	type = pcfg->dwType - JOY_HW_LASTENTRY;
	if( type < 0 || type >= pgv->pjd->oemCount ) {
	    type = -1;
	}
    } else {
	type = -1;
    }
    switch( id ) {
    case IDC_JOYLIST1_LABEL:
    	if( (type == -1) || (pgv->pjd->oemList[type].xy_label[0] == 0) ) {
	    if( !LoadString( hinst, IDS_XYAXIS_LABEL, str, sizeof( str ) ) ) {
		return;
	    }
	} else {
	    strcpy( str, pgv->pjd->oemList[type].xy_label );
	}
    	break;
    case IDC_JOYLIST2_LABEL:
    	if( (type == -1) || (pgv->pjd->oemList[type].z_label[0] == 0 ) ) {
	    if( !LoadString( hinst, IDS_ZAXIS_LABEL, str, sizeof( str ) ) ) {
		return;
	    }
	} else {
	    strcpy( str, pgv->pjd->oemList[type].z_label );
	}
    	break;
    case IDC_JOYLIST3_LABEL:
    	if( (type == -1) || (pgv->pjd->oemList[type].r_label[0] == 0) ) {
	    if( !LoadString( hinst, IDS_RAXIS_LABEL, str, sizeof( str ) ) ) {
		return;
	    }
	} else {
	    strcpy( str, pgv->pjd->oemList[type].r_label );
	}
    	break;
    case IDC_JOYLIST4_LABEL:
    	if( (type == -1) || (pgv->pjd->oemList[type].u_label[0] == 0) ) {
	    if( !LoadString( hinst, IDS_UAXIS_LABEL, str, sizeof( str ) ) ) {
		return;
	    }
	} else {
	    strcpy( str, pgv->pjd->oemList[type].u_label );
	}
    	break;
    case IDC_JOYLIST5_LABEL:
    	if( (type == -1) || (pgv->pjd->oemList[type].v_label[0] == 0) ) {
	    if( !LoadString( hinst, IDS_VAXIS_LABEL, str, sizeof( str ) ) ) {
		return;
	    }
	} else {
	    strcpy( str, pgv->pjd->oemList[type].v_label );
	}
    	break;
    case IDC_JOYPOV_LABEL:
    	if( (type == -1) || (pgv->pjd->oemList[type].pov_label[0] == 0) ) {
	    if( !LoadString( hinst, IDS_POVAXIS_LABEL, str, sizeof( str ) ) ) {
		return;
	    }
	} else {
	    strcpy( str, pgv->pjd->oemList[type].pov_label );
	}
    	break;
    }

    /*
     * tack on the calibration indicator if needed
     */
    if( pcfg->hwv.dwCalFlags & bit ) {
	if( !LoadString( hinst, IDS_JOYCALINDICATOR,
		    calstr, sizeof( calstr ) ) ) {
	    return;
	}
	if( strlen( str ) + strlen( calstr ) + 1 >= sizeof( str ) ) {
	    return;
	}
	strcat( str, " " );
	strcat( str, calstr );
    }
    hwtext = GetDlgItem( hwnd, id );
    if( hwtext != NULL ) {
	SetWindowText( hwtext, str );
    }

} /* setLabel */

/*
 * enableCalWindows - enable or disable specific calibration windows
 */
static void enableCalWindows(
		LPGLOBALVARS pgv,
		LPJOYREGHWCONFIG pcfg,
		HWND hwnd,
		cal_wins id )
{
    BOOL		on;
    HWND		hwlb;
    HWND		hwb;
//    HWND		hwcb;
    int			iid;

    /*
     * set up the buttons
     */
    hwb = GetDlgItem( hwnd,IDC_JOYCALDONE );
    if( id == JC_FINI ) {
	ShowWindow( GetDlgItem( hwnd, IDC_JOYCALNEXT ), SW_HIDE );
	ShowWindow( GetDlgItem( hwnd, IDC_JOYTEST ), SW_NORMAL );
	EnableWindow( hwb, TRUE );
	ShowWindow( hwb, SW_NORMAL );
	SetFocus( hwb );
	setDefaultButton( hwnd, hwb );
    } else {
	ShowWindow( GetDlgItem( hwnd, IDC_JOYCALNEXT ), SW_NORMAL );
	ShowWindow( GetDlgItem( hwnd, IDC_JOYTEST ), SW_HIDE );
	EnableWindow( hwb, FALSE );
	ShowWindow( hwb, SW_HIDE );
    }

    /*
     * set up the labels with the (done) after it...
     */
    setLabel( pgv, hwnd, IDC_JOYLIST1_LABEL, pcfg, JOY_ISCAL_XY );

    if( pcfg->hws.dwFlags & JOY_HWS_HASZ ) {
	setLabel( pgv, hwnd, IDC_JOYLIST2_LABEL, pcfg, JOY_ISCAL_Z );
    }

    if( (pcfg->hws.dwFlags & JOY_HWS_HASR) || (pcfg->dwUsageSettings & JOY_US_HASRUDDER)) {
	setLabel( pgv, hwnd, IDC_JOYLIST3_LABEL, pcfg, JOY_ISCAL_R );
    }

    if( pcfg->hws.dwFlags & JOY_HWS_HASPOV ) {
	setLabel( pgv, hwnd, IDC_JOYPOV_LABEL, pcfg, JOY_ISCAL_POV );
    }

    if( pcfg->hws.dwFlags & JOY_HWS_HASU ) {
	setLabel( pgv, hwnd, IDC_JOYLIST4_LABEL, pcfg, JOY_ISCAL_U );
    }

    if( pcfg->hws.dwFlags & JOY_HWS_HASV ) {
	setLabel( pgv, hwnd, IDC_JOYLIST5_LABEL, pcfg, JOY_ISCAL_V );
    }


    /*
     * set up the XY window
     */
    on = FALSE;
    if( id == JC_XY ) {
	on = TRUE;
    }
    hwlb = GetDlgItem( hwnd, IDC_JOYLIST1 );
    if( !on ) {
	InvalidateRect( hwlb, NULL, TRUE );
    }
    EnableWindow( hwlb, on );
    EnableWindow( GetDlgItem( hwnd, IDC_JOYLIST1_LABEL ), on );

    /*
     * set up the Z window
     */
    on = FALSE;
    if( id == JC_Z ) {
	on = TRUE;
    }
    hwlb = GetDlgItem( hwnd, IDC_JOYLIST2 );
    if( !on ) {
	InvalidateRect( hwlb, NULL, TRUE );
    }
    EnableWindow( hwlb, on );
    EnableWindow( GetDlgItem( hwnd, IDC_JOYLIST2_LABEL ), on );

    /*
     * set up the R window
     */
    on = FALSE;
    if( id == JC_R ) {
	on = TRUE;
    }
    hwlb = GetDlgItem( hwnd, IDC_JOYLIST3 );
    if( !on ) {
	InvalidateRect( hwlb, NULL, TRUE );
    }
    EnableWindow( hwlb, on );
    EnableWindow( GetDlgItem( hwnd, IDC_JOYLIST3_LABEL ), on );

    /*
     * set up the U window
     */
    on = FALSE;
    if( id == JC_U ) {
	on = TRUE;
    }
    hwlb = GetDlgItem( hwnd, IDC_JOYLIST4 );
    if( hwlb != NULL ) {
	if( !on ) {
	    InvalidateRect( hwlb, NULL, TRUE );
	}
	EnableWindow( hwlb, on );
	EnableWindow( GetDlgItem( hwnd, IDC_JOYLIST4_LABEL ), on );
    }

    /*
     * set up the V window
     */
    on = FALSE;
    if( id == JC_V ) {
	on = TRUE;
    }
    hwlb = GetDlgItem( hwnd, IDC_JOYLIST5 );
    if( hwlb != NULL ) {
	if( !on ) {
	    InvalidateRect( hwlb, NULL, TRUE );
	}
	EnableWindow( hwlb, on );
	EnableWindow( GetDlgItem( hwnd, IDC_JOYLIST5_LABEL ), on );
    }

    /*
     * set up the POV icon
     */
    on = FALSE;
    if( id >= JC_POV_UP && id <= JC_POV_LEFT ) {
	on = TRUE;
    }
    EnableWindow( GetDlgItem( hwnd, IDC_JOYPOV_LABEL ), on );
    hwb = GetDlgItem( hwnd, IDC_JOYPICKPOV );
    EnableWindow( hwb, on );
    if( on ) {
	ShowWindow( hwb, SW_NORMAL );
	SetFocus( hwb );
	setDefaultButton( hwnd, hwb );
	switch( id ) {
	case JC_POV_UP:
	    iid = IDI_JOYPOV_UP;
	    break;
	case JC_POV_RIGHT:
	    iid = IDI_JOYPOV_RIGHT;
	    break;
	case JC_POV_LEFT:
	    iid = IDI_JOYPOV_LEFT;
	    break;
	case JC_POV_DOWN:
	    iid = IDI_JOYPOV_DOWN;
	    break;
	}
    } else {
	ShowWindow( hwb, SW_HIDE );
	UpdateWindow( hwb );
	iid = IDI_JOYPOV_GRAYED;
    }
    ChangeIcon( hwnd, iid, IDC_JOYPOV );

} /* enableCalWindows */


/*
 * getJoyName - get the name of a joystick
 */
static int getJoyName( LPJOYREGHWCONFIG pcfg, BOOL plural )
{
    int	str2id;

    if( pcfg->hws.dwFlags & JOY_HWS_ISYOKE ) {
	if( plural ) {
	    str2id = IDS_JOYCAL_YOKES;
	} else {
	    str2id = IDS_JOYCAL_YOKE;
	}
    } else if( pcfg->hws.dwFlags & JOY_HWS_ISCARCTRL ) {
	if( plural ) {
	    str2id = IDS_JOYCAL_CARS;
	} else {
	    str2id = IDS_JOYCAL_CAR;
	}
    } else if( pcfg->hws.dwFlags & JOY_HWS_ISGAMEPAD ) {
	if( plural ) {
	    str2id = IDS_JOYCAL_GAMEPADS;
	} else {
	    str2id = IDS_JOYCAL_GAMEPAD;
	}
    } else {
	if( plural ) {
	    str2id = IDS_JOY2S;
	} else {
	    str2id = IDS_JOY2;
	}
    }
    return str2id;

} /* getJoyName */

/*
 * joyCalStateChange - calibration state change
 */
static BOOL joyCalStateChange( LPCALVARS pcv, HWND hwnd, BOOL back )
{
    HINSTANCE		hinst;
    HWND		hwtext;
    int			strid;
    int			stridx;
    int			str2id;
    int			str3id;
    int			str4id;
    char		str[2*MAX_STR];
    char		buff[2*MAX_STR];
    char		str2[64];
    char		str3[64];
    char		str4[64];
    BOOL		done;
    LPJOYREGHWCONFIG	pcfg;
    BOOL		rc;
    int			type;
    LPGLOBALVARS	pgv;
    BOOL		isdone;

    /*
     * move to the next state: get the appropriate string
     * to display, and enable the correct controls
     */
    pgv = pcv->pgv;
    rc = TRUE;
    done = FALSE;
    pcfg = &pgv->joyHWCurr;
    str2id = -1;
    str3id = -1;
    str4id = -1;
    pcv->cState++;
    EnableWindow( GetDlgItem( hwnd, IDC_JOYCALBACK ), back );
    while( !done ) {
	done = TRUE;
	switch( pcv->cState ) {
	case JCS_XY_CENTER1:
	    /*
	     * init. range variables
	     */
	    pcv->jr.jpMin.dwX = (DWORD) -1;
	    pcv->jr.jpMin.dwY = (DWORD) -1;
	    pcv->jr.jpMin.dwZ = (DWORD) -1;
	    pcv->jr.jpMin.dwR = (DWORD) -1;
	    pcv->jr.jpMin.dwU = (DWORD) -1;
	    pcv->jr.jpMin.dwV = (DWORD) -1;
	    pcv->jr.jpMax.dwX = 0;
	    pcv->jr.jpMax.dwY = 0;
	    pcv->jr.jpMax.dwZ = 0;
	    pcv->jr.jpMax.dwR = 0;
	    pcv->jr.jpMax.dwU = 0;
	    pcv->jr.jpMax.dwV = 0;
    
	    /*
	     * set strings to display
	     */
	    stridx = CALSTR1;
	    if( pcfg->hws.dwFlags & JOY_HWS_ISYOKE ) {
		strid = IDS_JOYCALXY_CENTERYOKE;
	    } else if( pcfg->hws.dwFlags & JOY_HWS_ISCARCTRL ) {
		strid = IDS_JOYCALXY_CENTERCAR;
	    } else if( pcfg->hws.dwFlags & JOY_HWS_ISGAMEPAD ) {
		strid = IDS_JOYCALXY_CENTERGAMEPAD;
	    } else {
		strid = IDS_JOYCALXY_CENTER;
	    }
	    enableCalWindows( pgv, pcfg, hwnd, JC_XY );
	    break;
	case JCS_XY_MOVE:
	    stridx = CALSTR2;
	    if( pcfg->hws.dwFlags & JOY_HWS_ISYOKE ) {
		strid = IDS_JOYCALXY_MOVEYOKE;
	    } else if( pcfg->hws.dwFlags & JOY_HWS_ISCARCTRL ) {
		strid = IDS_JOYCALXY_MOVECAR;
	    } else if( pcfg->hws.dwFlags & JOY_HWS_ISGAMEPAD ) {
		strid = IDS_JOYCALXY_MOVEGAMEPAD;
	    } else {
		strid = IDS_JOYCALXY_MOVE;
	    }
	    break;
	case JCS_XY_CENTER2:
	    stridx = CALSTR3;
	    if( pcfg->hws.dwFlags & JOY_HWS_ISYOKE ) {
		strid = IDS_JOYCALXY_CENTERYOKE2;
	    } else if( pcfg->hws.dwFlags & JOY_HWS_ISCARCTRL ) {
		strid = IDS_JOYCALXY_CENTERCAR2;
	    } else if( pcfg->hws.dwFlags & JOY_HWS_ISGAMEPAD ) {
		strid = IDS_JOYCALXY_CENTERGAMEPAD2;
	    } else {
		strid = IDS_JOYCALXY_CENTER2;
	    }
	    break;
	case JCS_Z_MOVE:
	    stridx = CALSTR4;
	    if( !(pcfg->hws.dwFlags & JOY_HWS_HASZ) ) {
		pcv->cState = JCS_R_MOVE;
		done = FALSE;
	    } else {
		enableCalWindows( pgv, pcfg, hwnd, JC_Z );
		strid = IDS_JOYCALZ_MOVE;
		str2id = getJoyName( pcfg, TRUE );
	    }
	    break;
	case JCS_Z_PLACEHOLDER:
	    pcv->cState = JCS_R_MOVE;
	    done = FALSE;
	    break;
	case JCS_R_MOVE:
	    stridx = CALSTR5;
	    if( !(pcfg->hws.dwFlags & JOY_HWS_HASR) && !(pcfg->dwUsageSettings & JOY_US_HASRUDDER) ) {
		pcv->cState = JCS_U_MOVE;
		done = FALSE;
	    } else {
		enableCalWindows( pgv, pcfg, hwnd, JC_R );
		strid = IDS_JOYCALRUDDER_MOVE;
		str2id = getJoyName( pcfg, TRUE );
	    }
	    break;
	case JCS_R_PLACEHOLDER:
	    pcv->cState = JCS_U_MOVE;
	    done = FALSE;
	    break;
	case JCS_U_MOVE:
	    stridx = CALSTR6;
	    if( !(pcfg->hws.dwFlags & JOY_HWS_HASU) ) {
		pcv->cState = JCS_V_MOVE;
		done = FALSE;
	    } else {
		enableCalWindows( pgv, pcfg, hwnd, JC_U );
		strid = IDS_JOYCALU_MOVE;
		str2id = getJoyName( pcfg, TRUE );
	    }
	    break;
	case JCS_U_PLACEHOLDER:
	    pcv->cState = JCS_V_MOVE;
	    done = FALSE;
	    break;
	case JCS_V_MOVE:
	    stridx = CALSTR7;
	    if( !(pcfg->hws.dwFlags & JOY_HWS_HASV) ) {
		pcv->cState = JCS_POV_MOVEUP;
		done = FALSE;
	    } else {
		enableCalWindows( pgv, pcfg, hwnd, JC_V );
		strid = IDS_JOYCALV_MOVE;
		str2id = getJoyName( pcfg, TRUE );
	    }
	    break;
	case JCS_V_PLACEHOLDER:
	    pcv->cState = JCS_POV_MOVEUP;
	    done = FALSE;
	    break;
	case JCS_POV_MOVEUP:
	    stridx = CALSTR8;
	    if( !(pcfg->hws.dwFlags & JOY_HWS_HASPOV)) {
		pcv->cState = JCS_FINI;
		done = FALSE;
	    } else {
		enableCalWindows( pgv, pcfg, hwnd, JC_POV_UP );
		strid = IDS_JOYCALPOV_MOVE;
		str2id = IDS_JOYCAL_UP;
		str3id = getJoyName( pcfg, TRUE );
		str4id = IDS_JOYCAL_UP;
	    }
	    break;
	case JCS_POV_MOVERIGHT:
	    stridx = CALSTR9;
	    enableCalWindows( pgv, pcfg, hwnd, JC_POV_RIGHT );
	    strid = IDS_JOYCALPOV_MOVE;
	    str2id = IDS_JOYCAL_RIGHT;
	    str3id = getJoyName( pcfg, TRUE );
	    str4id = IDS_JOYCAL_RIGHT;
	    break;
	case JCS_POV_MOVEDOWN:
	    stridx = CALSTR10;
	    enableCalWindows( pgv, pcfg, hwnd, JC_POV_DOWN );
	    strid = IDS_JOYCALPOV_MOVE;
	    str2id = IDS_JOYCAL_DOWN;
	    str3id = getJoyName( pcfg, TRUE );
	    str4id = IDS_JOYCAL_DOWN;
	    break;
	case JCS_POV_MOVELEFT:
	    stridx = CALSTR11;
	    enableCalWindows( pgv, pcfg, hwnd, JC_POV_LEFT );
	    strid = IDS_JOYCALPOV_MOVE;
	    str2id = IDS_JOYCAL_LEFT;
	    str3id = getJoyName( pcfg, TRUE );
	    str4id = IDS_JOYCAL_LEFT;
	    break;
	case JCS_FINI:
	    /*
	     * see if everything that needs to be calibrated
	     * was actually calibrated
	     */
	    if( !(pcfg->hwv.dwCalFlags & JOY_ISCAL_XY) ) {
		isdone = FALSE;
	    } else if( (pcfg->hws.dwFlags & JOY_HWS_HASZ) &&
			!(pcfg->hwv.dwCalFlags & JOY_ISCAL_Z) ) {
		isdone = FALSE;
	    } else if( ((pcfg->hws.dwFlags & JOY_HWS_HASR) ||
	    		(pcfg->dwUsageSettings & JOY_US_HASRUDDER)) &&
    			!(pcfg->hwv.dwCalFlags & JOY_ISCAL_R) ) {
		isdone = FALSE;
	    } else if( (pcfg->hws.dwFlags & JOY_HWS_HASPOV) &&
    			!(pcfg->hwv.dwCalFlags & JOY_ISCAL_POV) )  {
		isdone = FALSE;
	    } else if( (pcfg->hws.dwFlags & JOY_HWS_HASU) &&
    			!(pcfg->hwv.dwCalFlags & JOY_ISCAL_U) )  {
		isdone = FALSE;
	    } else if( (pcfg->hws.dwFlags & JOY_HWS_HASV) &&
    			!(pcfg->hwv.dwCalFlags & JOY_ISCAL_V) )  {
		isdone = FALSE;
	    } else {
		isdone = TRUE;
	    }
	    if( isdone ) {
		strid = IDS_JOYCAL_DONE;
	    } else {
		strid = IDS_JOYCAL_NOTDONE;
	    }
	    str2id = getJoyName( pcfg, FALSE );
	    str3id = getJoyName( pcfg, TRUE );
	    stridx = CALSTR12;
	    enableCalWindows( pgv, pcfg, hwnd, JC_FINI );
	    rc = FALSE;
	    break;
	}
    }

    /*
     * see if there is any OEM text specified
     */
    hinst = GetWindowInstance( hwnd );
    hwtext = GetDlgItem( hwnd, IDC_JOYCALMSG );
    if( pcfg->dwUsageSettings & JOY_US_ISOEM ) {
	LPJOYDATA	pjd;
	pjd = pgv->pjd;
	type = pcfg->dwType - JOY_HW_LASTENTRY;
	if( pjd->oemList[type].cal_strs[ stridx ][0] != 0 ) {
	    SetWindowText( hwtext, pjd->oemList[type].cal_strs[ stridx] );
	    return rc;
	}
    }

    /*
     * no OEM text, use the defaults
     */
    if( LoadString( hinst, strid, str, sizeof( str ) ) ) {
	if( str2id != -1 ) {
	    if( LoadString( hinst, str2id, str2, sizeof( str2 ) ) ) {
		if( str3id != -1 ) {
		    if( LoadString( hinst, str3id, str3, sizeof( str3 ) ) ) {
			if( str4id != -1 ) {
			    if( LoadString( hinst, str4id, str4, sizeof( str4 ) ) ) {
				// wsprintf( buff, str, str2, str3, str4 );
				LPSTR lpargs[] = {str2, str3, str4};
				
				FormatMessage(FORMAT_MESSAGE_FROM_STRING |
					      FORMAT_MESSAGE_ARGUMENT_ARRAY,
					      (LPSTR) str,
					      0, 0,
					      buff,
					      sizeof(buff),
					      (va_list *)lpargs);
				SetWindowText( hwtext, buff );
			    }
			} else {
			    wsprintf( buff, str, str2, str3 );
			    SetWindowText( hwtext, buff );
			}
		    }
		} else {
		    wsprintf( buff, str, str2, str2 );
		    SetWindowText( hwtext, buff );
		}
	    }
	} else {
	    SetWindowText( hwtext, str );
	}
    }

    return rc;

} /* joyCalStateChange */

/*
 * joyCalStateSkip - skip the current state, move to the next one
 */
static void joyCalStateSkip( LPCALVARS pcv, HWND hwnd )
{

    /*
     * if we're calibrating XY, skip to Z
     */
    if( pcv->cState <= JCS_XY_CENTER2 ) {
	pcv->cState = JCS_XY_CENTER2;
    /*
     * if we're calibrating Z, skip to R
     */
    } else if( pcv->cState < JCS_Z_PLACEHOLDER ) {
	pcv->cState = JCS_Z_PLACEHOLDER;
    /*
     * if we're calibrating R, skip to U
     */
    } else if( pcv->cState < JCS_R_PLACEHOLDER ) {
	pcv->cState = JCS_R_PLACEHOLDER;
    /*
     * if we're calibrating U, skip to V
     */
    } else if( pcv->cState < JCS_U_PLACEHOLDER ) {
	pcv->cState = JCS_U_PLACEHOLDER;
    /*
     * if we're calibrating V, skip to POV
     */
    } else if( pcv->cState < JCS_V_PLACEHOLDER ) {
	pcv->cState = JCS_V_PLACEHOLDER;
    /*
     * we must be calibration POV, skip to the end
     */
    } else  {
	pcv->cState = JCS_POV_MOVELEFT;
    }

    /*
     * state changed, reset to the new one
     */
    CauseRedraw( &pcv->ji, FALSE );
    joyCalStateChange( pcv, hwnd, TRUE );

} /* joyCalStateSkip */

/*
 * resetCustomPOVFlags - set POV flags based on original values for custom joystick
 */
static void resetCustomPOVFlags( LPGLOBALVARS pgv, LPJOYREGHWCONFIG pcfg )
{
    if( pcfg->dwType == JOY_HW_CUSTOM ) {
	pcfg->hws.dwFlags &= ~(JOY_HWS_POVISPOLL|JOY_HWS_POVISBUTTONCOMBOS);
	if( pgv->bOrigPOVIsPoll ) {
	    pcfg->hws.dwFlags |= JOY_HWS_POVISPOLL;
	}
	if( pgv->bOrigPOVIsButtonCombos ) {
	    pcfg->hws.dwFlags |= JOY_HWS_POVISBUTTONCOMBOS;
	}
    }

} /* resetCustomPOVFlags */


/*
 * joyCalStateBack - move back to start the previous state
 */
static void joyCalStateBack( LPCALVARS pcv, HWND hwnd )
{
    BOOL		back;
    LPJOYREGHWCONFIG	pcfg;
    LPGLOBALVARS	pgv;

    pgv = pcv->pgv;
    back = TRUE;
    pcfg = &pgv->joyHWCurr;
    /*
     * at the end, backup
     */
    if( pcv->cState == JCS_FINI ) {
	/*
	 * if there is POV, back up to it
	 */
	if( pcfg->hws.dwFlags & JOY_HWS_HASPOV ) {
	    pcv->cState = JCS_V_PLACEHOLDER;
	    resetCustomPOVFlags( pgv, pcfg );
	/*
	 * if there is V, back up to it
	 */
	} else if( pcfg->hws.dwFlags & JOY_HWS_HASV ) {
	    pcv->cState = JCS_U_PLACEHOLDER;
	/*
	 * if there is U, back up to it
	 */
	} else if( pcfg->hws.dwFlags & JOY_HWS_HASU ) {
	    pcv->cState = JCS_R_PLACEHOLDER;
	/*
	 * if there is R, back up to it
	 */
	} else if( (pcfg->hws.dwFlags & JOY_HWS_HASR) ||
	    (pcfg->dwUsageSettings & JOY_US_HASRUDDER) ) {
	    pcv->cState = JCS_Z_PLACEHOLDER;
	/*
	 * if there is Z, back up to it
	 */
	} else if( pcfg->hws.dwFlags & JOY_HWS_HASZ ) {
	    pcv->cState = JCS_XY_CENTER2;
	/*
	 * no where else to go, back up to XY
	 */
	} else {
	    pcv->cState = JCS_INIT;
	    back = FALSE;
	}
    /*
     * doing POV, so restart it
     */
    } else if( pcv->cState > JCS_POV_MOVEUP ) {
	pcv->cState = JCS_V_PLACEHOLDER;
//	pcfg->hws.dwFlags &= ~(JOY_HWS_POVISPOLL|JOY_HWS_POVISBUTTONCOMBOS);
	resetCustomPOVFlags( pgv, pcfg );
    /*
     * just starting POV, back up
     */
    } else if( pcv->cState == JCS_POV_MOVEUP ) {
	/*
	 * if there is V, back up to it
	 */
	if( pcfg->hws.dwFlags & JOY_HWS_HASV ) {
	    pcv->cState = JCS_U_PLACEHOLDER;
	/*
	 * if there is U, back up to it
	 */
	} else if( pcfg->hws.dwFlags & JOY_HWS_HASU ) {
	    pcv->cState = JCS_R_PLACEHOLDER;
	/*
	 * if there is R, back up to it
	 */
	} else if( (pcfg->hws.dwFlags & JOY_HWS_HASR) ||
	    (pcfg->dwUsageSettings & JOY_US_HASRUDDER) ) {
	    pcv->cState = JCS_Z_PLACEHOLDER;
	/*
	 * if there is Z, back up to it
	 */
	} else if( pcfg->hws.dwFlags & JOY_HWS_HASZ ) {
	    pcv->cState = JCS_XY_CENTER2;
	/*
	 * no where else to go, back up to XY
	 */
	} else {
	    pcv->cState = JCS_INIT;
	    back = FALSE;
	}
    /*
     * doing V, backup
     */
    } else if( pcv->cState == JCS_V_MOVE ) {
	/*
	 * if there is U, back up to it
	 */
	if( pcfg->hws.dwFlags & JOY_HWS_HASU ) {
	    pcv->cState = JCS_R_PLACEHOLDER;
	/*
	 * if there is R, back up to it
	 */
	} else if( (pcfg->hws.dwFlags & JOY_HWS_HASR) ||
	    (pcfg->dwUsageSettings & JOY_US_HASRUDDER) ) {
	    pcv->cState = JCS_Z_PLACEHOLDER;
	/*
	 * if there is Z, back up to it
	 */
	} else if( pcfg->hws.dwFlags & JOY_HWS_HASZ ) {
	    pcv->cState = JCS_XY_CENTER2;
	/*
	 * no where else to go, back up to XY
	 */
	} else {
	    pcv->cState = JCS_INIT;
	    back = FALSE;
	}
    /*
     * doing U, backup
     */
    } else if( pcv->cState == JCS_U_MOVE ) {
	/*
	 * if there is R, back up to it
	 */
	if( (pcfg->hws.dwFlags & JOY_HWS_HASR) ||
	    (pcfg->dwUsageSettings & JOY_US_HASRUDDER) ) {
	    pcv->cState = JCS_Z_PLACEHOLDER;
	/*
	 * if there is Z, back up to it
	 */
	} else if( pcfg->hws.dwFlags & JOY_HWS_HASZ ) {
	    pcv->cState = JCS_XY_CENTER2;
	/*
	 * no where else to go, back up to XY
	 */
	} else {
	    pcv->cState = JCS_INIT;
	    back = FALSE;
	}
    /*
     * doing R, backup
     */
    } else if( pcv->cState == JCS_R_MOVE ) {
	/*
	 * if there is Z, back up to it
	 */
	if( pcfg->hws.dwFlags & JOY_HWS_HASZ ) {
	    pcv->cState = JCS_XY_CENTER2;
	/*
	 * no where else to go, back up to XY
	 */
	} else {
	    pcv->cState = JCS_INIT;
	    back = FALSE;
	}
    /*
     * if we're doing Z or in the middle of XY, backup to XY
     */
    } else {
	pcv->cState = JCS_INIT;
	back = FALSE;
    }

    /*
     * state changed, reset to the new one
     */
    CauseRedraw( &pcv->ji, FALSE );
    joyCalStateChange( pcv, hwnd, back );

} /* joyCalStateBack */

/*
 * macro to get new max/min data for an axis
 */
#define NEWMINMAX( a ) \
    if( pji->dw##a##pos > pcv->jr.jpMax.dw##a ) { \
	pcv->jr.jpMax.dw##a = pji->dw##a##pos; \
    } \
    if( pji->dw##a##pos < pcv->jr.jpMin.dw##a ) { \
	pcv->jr.jpMin.dw##a = pji->dw##a##pos; \
    } \

/*
 * macro to do continuous calibration--changes jpi->dw*pos based on current
 * position and latest minimum/maximum values
 */
#define CAL_MIN  50   // Pretend it's centered unless HW range is >= CAL_MIN

#define SIMULATECALIBRATION(_pcv,_pji,_AXIS_) \
    { \
        NEWMINMAX(_AXIS_); \
        _pji->dw##_AXIS_##pos -= _pcv->jr.jpMin.dw##_AXIS_; \
        if (_pcv->jr.jpMax.dw##_AXIS_ - _pcv->jr.jpMin.dw##_AXIS_ < CAL_MIN) { \
            _pji->dw##_AXIS_##pos = (RANGE_MAX -RANGE_MIN)/2; \
        } else if ( RANGE_MAX == RANGE_MIN ) { \
            _pji->dw##_AXIS_##pos = (RANGE_MAX -RANGE_MIN)/2; \
        } else { \
            _pji->dw##_AXIS_##pos *= (RANGE_MAX-RANGE_MIN); \
            _pji->dw##_AXIS_##pos /= ( _pcv->jr.jpMax.dw##_AXIS_ - \
                                       _pcv->jr.jpMin.dw##_AXIS_ ); \
        } \
        _pji->dw##_AXIS_##pos += RANGE_MIN; \
    }

/*
 * joyCollectCalInfo - record calibration info 
 */
static BOOL joyCollectCalInfo( LPCALVARS pcv, HWND hwnd, LPJOYINFOEX pji )
{
    LPGLOBALVARS	pgv;
    LPJOYREGHWCONFIG	pcfg;

    pgv = pcv->pgv;
    switch( pcv->cState ) {
    /*
     * remember XY center
     */
    case JCS_XY_CENTER1:
    case JCS_XY_CENTER2:
    {
	JOYINFOEX jiex = *pji;
	SIMULATECALIBRATION( pcv, pji, X );
	SIMULATECALIBRATION( pcv, pji, Y );
	DoJoyMove( pgv, hwnd, pji, &pcv->ji, JOYMOVE_DRAWXY );
	*pji = jiex;
	break;
    }

    /*
     * remember max/min XY values
     */
    case JCS_XY_MOVE:
    {
	JOYINFOEX jiex = *pji;
	SIMULATECALIBRATION( pcv, pji, X );
	SIMULATECALIBRATION( pcv, pji, Y );
	DoJoyMove( pgv, hwnd, pji, &pcv->ji, JOYMOVE_DRAWXY );
	*pji = jiex;
    	break;
    }

    /*
     * remember max/min Z value
     */
    case JCS_Z_MOVE:
    {
	JOYINFOEX jiex = *pji;
	SIMULATECALIBRATION( pcv, pji, Z );
	DoJoyMove( pgv, hwnd, pji, &pcv->ji, JOYMOVE_DRAWZ );
	*pji = jiex;
    	break;
    }

    /*
     * remember max/min R value
     */
    case JCS_R_MOVE:
    {
	JOYINFOEX jiex = *pji;
	SIMULATECALIBRATION( pcv, pji, R );
	DoJoyMove( pgv, hwnd, pji, &pcv->ji, JOYMOVE_DRAWR );
	*pji = jiex;
    	break;
    }

    /*
     * remember max/min U value
     */
    case JCS_U_MOVE:
    {
	JOYINFOEX jiex = *pji;
	SIMULATECALIBRATION( pcv, pji, U );
	DoJoyMove( pgv, hwnd, pji, &pcv->ji, JOYMOVE_DRAWU );
	*pji = jiex;
    	break;
    }

    /*
     * remember max/min V value
     */
    case JCS_V_MOVE:
    {
	JOYINFOEX jiex = *pji;
	SIMULATECALIBRATION( pcv, pji, V );
	DoJoyMove( pgv, hwnd, pji, &pcv->ji, JOYMOVE_DRAWV );
	*pji = jiex;
    	break;
    }
    }

    /*
     * if a button was pressed, move to the next state
     */
    if( ((pcv->ji.dwButtons & ALL_BUTTONS) != (pji->dwButtons & ALL_BUTTONS)) &&
    	((pji->dwButtons & JOY_BUTTON1) ||
    	 (pji->dwButtons & JOY_BUTTON2) ||
    	 (pji->dwButtons & JOY_BUTTON3) ||
    	 (pji->dwButtons & JOY_BUTTON4) ) ) {
	/*
	 * check and see if we are leaving one calibration to the next;
	 * if yes, take time to stop and remember what the user just did
	 */
	pcfg = &pgv->joyHWCurr;
	switch( pcv->cState ) {
	case JCS_XY_CENTER1:
	    pcv->jr.jpCenter.dwX = pji->dwXpos;
	    pcv->jr.jpCenter.dwY = pji->dwYpos;
	    DPF( "Center 1: %d,%d\r\n", pji->dwXpos, pji->dwYpos );
	    break;

	case JCS_XY_CENTER2:
	    DPF( "Center 2: %d,%d\r\n", pji->dwXpos, pji->dwYpos );
	    pcv->jr.jpCenter.dwX += pji->dwXpos;
	    pcv->jr.jpCenter.dwY += pji->dwYpos;
	    pcv->jr.jpCenter.dwX /= 2;
	    pcv->jr.jpCenter.dwY /= 2;
	    DPF( "Center Avg: %d,%d\r\n", pcv->jr.jpCenter.dwX, pcv->jr.jpCenter.dwY );
	    pcfg->hwv.jrvHardware.jpMin.dwX = pcv->jr.jpMin.dwX;
	    pcfg->hwv.jrvHardware.jpMin.dwY = pcv->jr.jpMin.dwY;
	    pcfg->hwv.jrvHardware.jpMax.dwX = pcv->jr.jpMax.dwX;
	    pcfg->hwv.jrvHardware.jpMax.dwY = pcv->jr.jpMax.dwY;
	    pcfg->hwv.jrvHardware.jpCenter.dwX = pcv->jr.jpCenter.dwX;
	    pcfg->hwv.jrvHardware.jpCenter.dwY = pcv->jr.jpCenter.dwY;
	    pcfg->hwv.dwCalFlags |= JOY_ISCAL_XY;
	    break;
	case JCS_Z_MOVE:
	    pcfg->hwv.jrvHardware.jpMin.dwZ = pcv->jr.jpMin.dwZ;
	    pcfg->hwv.jrvHardware.jpMax.dwZ = pcv->jr.jpMax.dwZ;
	    pcfg->hwv.dwCalFlags |= JOY_ISCAL_Z;
	    break;
	case JCS_R_MOVE:
	    pcfg->hwv.jrvHardware.jpMin.dwR = pcv->jr.jpMin.dwR;
	    pcfg->hwv.jrvHardware.jpMax.dwR = pcv->jr.jpMax.dwR;
	    pcfg->hwv.dwCalFlags |= JOY_ISCAL_R;
	    break;
	case JCS_U_MOVE:
	    pcfg->hwv.jrvHardware.jpMin.dwU = pcv->jr.jpMin.dwU;
	    pcfg->hwv.jrvHardware.jpMax.dwU = pcv->jr.jpMax.dwU;
	    pcfg->hwv.dwCalFlags |= JOY_ISCAL_U;
	    break;
	case JCS_V_MOVE:
	    pcfg->hwv.jrvHardware.jpMin.dwV = pcv->jr.jpMin.dwV;
	    pcfg->hwv.jrvHardware.jpMax.dwV = pcv->jr.jpMax.dwV;
	    pcfg->hwv.dwCalFlags |= JOY_ISCAL_V;
	    break;
	}
	pcv->ji.dwButtons = pji->dwButtons;
	return joyCalStateChange( pcv, hwnd, TRUE );
    }
    pcv->ji.dwButtons = pji->dwButtons;
    return TRUE;

} /* joyCollectCalInfo */

/*
 * joyCalibrateInitDialog - init the calibration dialog
 */
static BOOL joyCalibrateInitDialog( HWND hwnd, LPARAM lParam )
{
    LPJOYREGHWCONFIG	pcfg;
    LPCALVARS		pcv;
    LPGLOBALVARS	pgv;

    /*
     * set up calibration variables
     */
    pcv = DoAlloc( sizeof( CALVARS ) );
    SetWindowLong( hwnd, DWL_USER, (LONG) pcv );
    if( pcv == NULL ) {
	return FALSE;
    }
    pgv = (LPGLOBALVARS) lParam;
    pcv->pgv = pgv;

    /*
     * init state info
     */
    pcv->cState = JCS_INIT;

    /*
     * set dialog text based on OEM strings
     */
    SetOEMText( pgv, hwnd, FALSE );

    /*
     * customize dialog based on Z axis, R axis, and POV hat
     */
    pcfg = &pgv->joyHWCurr;
    pcv->iAxisCount = 2;
    if( pcfg->hws.dwFlags & JOY_HWS_HASZ ) {
	pcv->iAxisCount++;
    }
    if( (pcfg->hws.dwFlags & JOY_HWS_HASR) || (pcfg->dwUsageSettings & JOY_US_HASRUDDER) ) {
	pcv->iAxisCount++;
    }
    if( (pcfg->hws.dwFlags & JOY_HWS_HASPOV) &&
    	(pcfg->hws.dwFlags & JOY_HWS_POVISPOLL) ) {
	pcv->iAxisCount++;
    }
    if( pcfg->hws.dwFlags & JOY_HWS_HASU ) {
	pcv->iAxisCount++;
    }
    if( pcfg->hws.dwFlags & JOY_HWS_HASV ) {
	pcv->iAxisCount++;
    }
    ShowControls( pcfg, hwnd );

    /*
     * if all axes are used and we have POV then it MUST be buttons
     */
    if( pcfg->hws.dwFlags & JOY_HWS_HASPOV ) {
	if( pgv->dwMaxAxes == 4 && pcv->iAxisCount == 4 ) {
	    pcfg->hws.dwFlags |= JOY_HWS_POVISBUTTONCOMBOS;
	}
    }

    /*
     * other misc setup
     */
    pcv->bPOVdone = FALSE;
    pcv->bHasTimer = SetTimer( hwnd, TIMER_ID, JOYPOLLTIME, NULL );
    pcv->bUseTimer = TRUE;
    if( !pcv->bHasTimer ) {
	DPF( "No timer for joystick calibration!\r\n" );
	return FALSE;
    }
    if( !joyCalStateChange( pcv, hwnd, FALSE ) ) {
	DPF( "Could not initialize joystick calibration\r\n" );
	return FALSE;
    }
    
    return TRUE;

} /* joyCalibrateInitDialog */

/*
 * setJIFlagsForPOV - get joyinfo flags to allow a raw POV poll
 */
static void setJIFlagsForPOV( LPCALVARS pcv, LPJOYREGHWCONFIG pcfg, DWORD *pflags )
{
    /*
     * for polled POV, we need to specifiy JOY_CAL_READ(3|4) to make
     * the driver give us position values back instead of trying to
     * give us a POV value back
     */
    if( pcfg->hws.dwFlags & JOY_HWS_HASPOV ) {
	if( pcfg->hws.dwFlags & JOY_HWS_POVISPOLL ) {
	    if( pcv->iAxisCount == 6 ) {
		(*pflags) |= JOY_CAL_READ6;
	    } else if( pcv->iAxisCount == 5 ) {
		(*pflags) |= JOY_CAL_READ5;
	    } else if( pcv->iAxisCount == 4 ) {
		(*pflags) |= JOY_CAL_READ4;
	    } else if( pcv->iAxisCount == 3 ) {
		(*pflags) |= JOY_CAL_READ3;
	    }
	/*
	 * if we don't have a 3rd or 4th axis on this joystick, try reading
	 * another axis anyway to see if the POV hat is on it
	 */
	} else if( !(pcfg->hws.dwFlags & (JOY_HWS_POVISPOLL|JOY_HWS_POVISBUTTONCOMBOS)) ) {
	    if( pcv->iAxisCount == 5 ) {
		(*pflags) |= JOY_CAL_READ6;
	    } else if( pcv->iAxisCount == 4 ) {
		(*pflags) |= JOY_CAL_READ5;
	    } else if( pcv->iAxisCount == 3 ) {
		(*pflags) |= JOY_CAL_READ4;
	    } else if( pcv->iAxisCount == 2 ) {
		(*pflags) |= JOY_CAL_READ3;
	    }
	}
    }

} /* setJIFlagsForPOV */

/*
 * tryPOV - try for a POV access
 */
static BOOL tryPOV( LPCALVARS pcv, HWND hwnd )
{
    int			rc;
    BOOL		ispoll;
    BOOL		isb;
    BOOL		nowaypoll;
    JOYINFOEX		ji;
    DWORD		val;
    LPJOYREGHWCONFIG	pcfg;
    LPGLOBALVARS	pgv;
    int			i;

    pgv = pcv->pgv;

    /*
     * reject call if not in a POV state
     */
    if( !(pcv->cState == JCS_POV_MOVEUP ||
	pcv->cState == JCS_POV_MOVEDOWN ||
	pcv->cState == JCS_POV_MOVELEFT ||
	pcv->cState == JCS_POV_MOVERIGHT) ) {
	return FALSE;
    }

    /*
     * take a snapshot of the current joystick state
     */
    pcfg = &pgv->joyHWCurr;
    nowaypoll = FALSE;
    ji.dwSize = sizeof( ji );
    while( 1 ) {
	/*
	 * get joystick info
	 */
	ji.dwFlags = JOY_CALIB_FLAGS;
	setJIFlagsForPOV( pcv, pcfg, &ji.dwFlags );
	rc = joyGetPosEx( pgv->iJoyId, &ji );
	if( rc == JOYERR_NOERROR ) {
	    break;
	}
	if( !(pcfg->hws.dwFlags & JOY_HWS_POVISPOLL) &&
		(ji.dwFlags & (JOY_CAL_READ3|JOY_CAL_READ4|JOY_CAL_READ5|JOY_CAL_READ6)) ) {
	    /*
	     * try again, but don't ask for extra axis
	     */
	    ji.dwFlags &= ~(JOY_CAL_READ6 | JOY_CAL_READ5 | JOY_CAL_READ4 | JOY_CAL_READ3);
	    rc = joyGetPosEx( pgv->iJoyId, &ji );
	    if( rc == JOYERR_NOERROR ) {
		nowaypoll = TRUE;	// pov can't possibly be polled
		break;
	    } else {
		if( !JoyError( hwnd ) ) {
		    return FALSE;
		}
		return TRUE;	// have to wait for next "Select POV" to retry
	    }
	} else {
	    if( !JoyError( hwnd ) ) {
		return FALSE;
	    }
	    return TRUE;	// have to wait for next "Select POV" to retry
	}
    }

    /*
     * here is where we determine if POV is polled or is button combos.
     *
     * See if we already know the answer (bits in joyHWCurr):
     *     if yes:
     *	       we're done.
     *     if no:
     *         We see if there are currently multiple buttons down.
     *         if yes:
     *             POV is assumed to be button combos.
     *         if no:
     *             POV is assumed to be done with polling
     */
    ispoll = FALSE;
    isb = FALSE;
    if( pcfg->hws.dwFlags & JOY_HWS_POVISPOLL ) {
	ispoll = TRUE;
    }  else if( pcfg->hws.dwFlags & JOY_HWS_POVISBUTTONCOMBOS ) {
	isb = TRUE;
    }
    if( !isb && !ispoll ) {
	/*
	 * the type is indeterminate, so we identify it 
	 */
	if( nowaypoll ||
	    ((ji.dwButtons != 0) && (ji.dwButtons != JOY_BUTTON1) &&
	    (ji.dwButtons != JOY_BUTTON2) && (ji.dwButtons != JOY_BUTTON3) &&
	    (ji.dwButtons != JOY_BUTTON4)) ) {
	    isb = TRUE;
	    pcfg->hws.dwFlags |= JOY_HWS_POVISBUTTONCOMBOS;
	} else {
	    /*
	     * we always assume J2 Y for a polling POV if unspecified
	     */
	    ispoll = TRUE;
	    pcfg->hws.dwFlags |= JOY_HWS_POVISPOLL;
	}
	/*
	 * the driver needs to notified that we've made this decision
	 */
	RegSaveCurrentJoyHW( pgv );
	RegistryUpdated( pgv );
    }

    /*
     * record the data value for this POV reading
     */
    if( isb ) {
	val = ji.dwButtons;
    } else {
	if( !(pcfg->hws.dwFlags & JOY_HWS_HASZ) ) {
	    val = ji.dwZpos;
	} else {
	    val = ji.dwRpos;
	}
    }
    switch( pcv->cState ) {
    case JCS_POV_MOVEUP:
	pcv->pov[JOY_POVVAL_FORWARD] = val;
	break;
    case JCS_POV_MOVERIGHT:
	pcv->pov[JOY_POVVAL_RIGHT] = val;
	break;
    case JCS_POV_MOVEDOWN:
	pcv->pov[JOY_POVVAL_BACKWARD] = val;
	break;
    case JCS_POV_MOVELEFT:
	pcv->pov[JOY_POVVAL_LEFT] = val;
	/*
	 * since this was the last POV thing to calibrate, we need
	 * to save the calibration info
	 */
	for( i=0;i<JOY_POV_NUMDIRS;i++ ) {
	    pcfg->hwv.dwPOVValues[i] = pcv->pov[i];
	}
	pcfg->hwv.dwCalFlags |= JOY_ISCAL_POV;
	pcv->bPOVdone = TRUE;
	break;
    }
    return joyCalStateChange( pcv, hwnd, TRUE );

} /* tryPOV */

/*
 * FixCustomPOVType - fix custom POV type info if POV wasn't calibrated;
 *		      called by test dlg to update config
 */
void FixCustomPOVType( LPCALVARS pcv )
{
    if( !pcv->bPOVdone ) {
	resetCustomPOVFlags( pcv->pgv, &pcv->pgv->joyHWCurr );
    }

} /* FixCustomPOVType */

/*
 * CalibrateProc - calibrate a joystick
 */
BOOL CALLBACK CalibrateProc( HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    BOOL		rc;

    switch( umsg ) {
    case WM_TIMER:
    {
	LPCALVARS		pcv;

	pcv = (LPCALVARS) GetWindowLong( hwnd, DWL_USER );
    	if( pcv->bUseTimer ) {
	    JOYINFOEX		ji;
	    MMRESULT		rc;
	    LPJOYREGHWCONFIG	pcfg;
	    LPGLOBALVARS	pgv;

	    pgv = pcv->pgv;
	    pcv->bUseTimer = FALSE;
	    ji.dwSize = sizeof( ji );
	    while( 1 ) {
		/*
		 * get current joystick info
		 */
		ji.dwFlags = JOY_CALIB_FLAGS;
		pcfg = &pgv->joyHWCurr;
		setJIFlagsForPOV( pcv, pcfg, &ji.dwFlags );
		rc = joyGetPosEx( pgv->iJoyId, &ji );
		if( rc == JOYERR_NOERROR ) {
		    break;
		}

		/*
		 * didn't work, try without extra POV axis
		 */
		if( !(pcfg->hws.dwFlags & JOY_HWS_POVISPOLL) &&
			(ji.dwFlags & (JOY_CAL_READ3|JOY_CAL_READ4|JOY_CAL_READ5|JOY_CAL_READ6)) ) {
		    ji.dwFlags &= ~(JOY_CAL_READ6 | JOY_CAL_READ5 | JOY_CAL_READ4 | JOY_CAL_READ3);
		    rc = joyGetPosEx( pgv->iJoyId, &ji );
		    if( rc == JOYERR_NOERROR ) {
			break;
		    }
		}
		if( !JoyError( hwnd ) ) {
		    /*
		     * return now if cancel selected; don't turn back
		     * on the timer
		     */
		    return FALSE;	
		}
		continue;
	    }
	    if( rc == JOYERR_NOERROR ) {
		joyCollectCalInfo( pcv, hwnd, &ji );
	    }
	    /*
	     * If we've started POV calibration, we need to look at the
	     * keyboard and ignore joystick, so don't turn the timer
	     * back on if we've started the POV calibration
	     */
	    if( pcv->cState < JCS_POV_MOVEUP ) {
		pcv->bUseTimer = TRUE;
	    }
	}
	break;
    }
	
    case WM_DESTROY:
    {
	LPCALVARS	pcv;
	pcv = (LPCALVARS) GetWindowLong( hwnd, DWL_USER );
	DoFree( pcv );
	break;
    }
	
    case WM_INITDIALOG:
    {
	LPCALVARS	pcv;

	rc = joyCalibrateInitDialog( hwnd, lParam );
	if( !rc ) {
	    pcv = (LPCALVARS) GetWindowLong( hwnd, DWL_USER );
	    if( pcv != NULL && pcv->bHasTimer ) {
		KillTimer( hwnd, TIMER_ID );
		pcv->bHasTimer = FALSE;
	    }
	    EndDialog( hwnd, 0 );
	}
	return FALSE;
    }

    case WM_PAINT:
    {
	LPCALVARS	pcv;
	pcv = (LPCALVARS) GetWindowLong( hwnd, DWL_USER );
    	CauseRedraw( &pcv->ji, FALSE );
	return FALSE;
    }

    case WM_COMMAND:
    {
	int 		id;
	LPCALVARS	pcv;

	pcv = (LPCALVARS) GetWindowLong( hwnd, DWL_USER );
	id = GET_WM_COMMAND_ID(wParam, lParam);
	switch( id ) {
	case IDC_JOYTEST:
	{
	    BOOL		timeon;

	    timeon = pcv->bUseTimer;
	    pcv->bUseTimer = FALSE;
	    DoTest( pcv->pgv, hwnd, FixCustomPOVType, pcv );
	    pcv->bUseTimer = timeon;
	    break;
	}
	case IDCANCEL:
	    // fall through
	case IDC_JOYCALDONE:
	    if( pcv->bHasTimer ) {
		KillTimer( hwnd, TIMER_ID );
		pcv->bHasTimer = FALSE;
	    }
	    {
		LPJOYREGHWCONFIG	pcfg;
		pcfg = &pcv->pgv->joyHWCurr;
	    }
	    EndDialog( hwnd, (id == IDC_JOYCALDONE) );
	    break;
	case IDC_JOYPICKPOV:
	    if( !tryPOV( pcv, hwnd ) ) {
		HWND	hwb;
		hwb = GetDlgItem( hwnd, IDC_JOYPICKPOV );
		ShowWindow( hwb, SW_HIDE );
		EnableWindow( hwb, FALSE );
	    }
	    break;

	case IDC_JOYCALNEXT:
	    pcv->bUseTimer = TRUE;
	    joyCalStateSkip( pcv, hwnd );
	    break;

	case IDC_JOYCALBACK:
	    pcv->bUseTimer = TRUE;
	    joyCalStateBack( pcv, hwnd );
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

} /* CalibrateProc */

/*
 * DoCalibrate - do the calibration dialog
 */
void DoCalibrate( LPGLOBALVARS pgv, HWND hwnd )
{
    JOYREGHWCONFIG	save_joycfg;
    int			rc;
    int			id;

    /*
     * save the current config, and then add the rudder if it is present
     */
    save_joycfg = pgv->joyHWCurr;

    /*
     * if this is a custom joystick, then don't assume anything
     * about how the POV is set up
     */
    if( pgv->joyHWCurr.dwType == JOY_HW_CUSTOM ) {
	pgv->bOrigPOVIsPoll = (pgv->joyHWCurr.hws.dwFlags & JOY_HWS_POVISPOLL);
	pgv->bOrigPOVIsButtonCombos = (pgv->joyHWCurr.hws.dwFlags & JOY_HWS_POVISBUTTONCOMBOS);
	pgv->joyHWCurr.hws.dwFlags &= ~(JOY_HWS_POVISPOLL|JOY_HWS_POVISBUTTONCOMBOS);
    }

    /*
     * update the registry with our new joystick info
     */
    RegSaveCurrentJoyHW( pgv );
    RegistryUpdated( pgv );

    if( pgv->joyHWCurr.hws.dwFlags & (JOY_HWS_HASU|JOY_HWS_HASV) ) {
	id = IDD_JOYCALIBRATE1;
    } else {
	id = IDD_JOYCALIBRATE;
    }
    rc = DialogBoxParam((HINSTANCE)GetWindowLong( hwnd, GWL_HINSTANCE ),
		    MAKEINTRESOURCE( id ), hwnd, CalibrateProc, (LONG) pgv );

    /*
     * update the registry with the new info or the old info
     */
    if( rc ) {
	PropSheet_Changed( GetParent(hwnd), hwnd );
    } else {
	pgv->joyHWCurr = save_joycfg;
    }
    RegSaveCurrentJoyHW( pgv );
    RegistryUpdated( pgv );

} /* DoCalibrate */
