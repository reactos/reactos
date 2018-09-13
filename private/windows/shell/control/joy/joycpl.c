//****************************************************************************
//
//  File:       joycpl.c
//  Content:    Joystick configuration and testing
//  History:
//   Date	By	Reason
//   ====	==	======
//   03-oct-94	craige	initial implementation
//   05-nov-94	craige	generalized 4 axis joysticks and other improvements
//   11-nov-94	craige	allow multiple copies of tab to run
//   22-nov-94	craige	tweaks to calibration code
//   29-nov-94	craige	small bugs
//   08-dec-94	craige	generalized second joystick
//   11-dec-94	craige	split into component parts
//   15-dec-94	craige	allow N joysticks
//   18-dec-94	craige	process UV
//   05-jan-95	craige	external rudder bug
//   05-mar-95	craige	Bug 9998: pass id -1 to get base dev caps
//			Bug 15334: allow reset of user values for compatiblity
//   06-mar-95	craige	Bug 7608: deleting VxD name if joystick not present
//				  caused unplugged joystick to never come back
//   06-may-96	richj	ported to NT
//
//  Copyright (c) Microsoft Corporation 1994
//
//****************************************************************************


#include "joycpl.h"

extern HINSTANCE hInstance;	// main.c

#define cchLENGTH(_sz)       (sizeof(_sz)/sizeof(_sz[0]))
#define GetString(_sz,_ids)  LoadString (hInstance, _ids, _sz, cchLENGTH(_sz))


#ifdef DEBUG
void cdecl MBOX(LPSTR szFormat, ...)
{
    char ach[256];

    wvsprintf( ach,szFormat,(LPSTR)(&szFormat+1));
    MessageBox( NULL, ach, "JOYCPL", MB_OK | MB_SYSTEMMODAL );
                                                        
}
#endif

/***************************************************************************
 
 
  MEMORY MANAGEMENT ROUTINES FOLLOW
 
 
 ***************************************************************************/

#ifdef DEBUG
DWORD	allocCount;
#endif

/*
 * DoAlloc - allocate memory
 */
LPVOID DoAlloc( DWORD size )
{
    LPVOID	res;

    res = LocalAlloc( LPTR, size );
    #ifdef DEBUG
    	allocCount++;
    #endif
    return res;

} /* DoAlloc */

/*
 * DoFree - free allocated memory
 */
void DoFree( LPVOID ptr )
{
    if( ptr != NULL ) {
	LocalFree( ptr );
    #ifdef DEBUG
    	allocCount--;
	if( allocCount < 0 ) {
	    DPF( "JOYCPL:  Too many frees, allocCount=%d\r\n", allocCount );
	}
    #endif
    }

} /* DoFree */


/***************************************************************************
 
 
  REGISTRY RELATED ROUTINES FOLLOW
 
 
 ***************************************************************************/

/*
 * getDevCaps - get the joystick device caps
 */
static void getDevCaps( LPGLOBALVARS pgv )
{
    JOYCAPS	jc;

    if( joyGetDevCaps( pgv->iJoyId, &jc, sizeof( jc ) ) == JOYERR_NOERROR ) {
	pgv->joyRange.jpMin.dwX = jc.wXmin;
	pgv->joyRange.jpMax.dwX = jc.wXmax;
	pgv->joyRange.jpMin.dwY = jc.wYmin;
	pgv->joyRange.jpMax.dwY = jc.wYmax;
	pgv->joyRange.jpMin.dwZ = jc.wZmin;
	pgv->joyRange.jpMax.dwZ = jc.wZmax;
	pgv->joyRange.jpMin.dwR = jc.wRmin;
	pgv->joyRange.jpMax.dwR = jc.wRmax;
	pgv->joyRange.jpMin.dwU = jc.wUmin;
	pgv->joyRange.jpMax.dwU = jc.wUmax;
	pgv->joyRange.jpMin.dwV = jc.wVmin;
	pgv->joyRange.jpMax.dwV = jc.wVmax;
	pgv->dwMaxAxes = (DWORD) jc.wMaxAxes;
    }

} /* getDevCaps */

extern MMRESULT WINAPI joyConfigChanged( DWORD dwFlags );

/*
 * RegistryUpdated - notify the driver that the registry is updated
 */
void RegistryUpdated( LPGLOBALVARS pgv )
{
    joyConfigChanged( 0 );
    if( pgv != NULL ) {
	getDevCaps( pgv );		// devcaps could change
    }

} /* RegistryUpdated */

/*
 * createSettingsKeyFromCurr - create a settings key for a specific joystick
 */
static void createSettingsKeyFromCurr( LPGLOBALVARS pgv, LPSTR str )
{
    char	tmp[MAX_STR];
    int		type;
    LPJOYDATA	pjd;

    pjd = pgv->pjd;

    if( pgv->joyHWCurr.dwUsageSettings & JOY_US_ISOEM ) {
	type = pgv->joyHWCurr.dwType - JOY_HW_LASTENTRY;
	if( type < 0 || type >= pjd->oemCount ) {
	    tmp[0] = 0;
	} else {
	    strcpy( tmp, pjd->oemList[type].keyname );
	}
    } else {
	wsprintf( tmp, "predef%d", pgv->joyHWCurr.dwType );
    }
    wsprintf( str, "%s\\%s", pjd->regSettingsCfgKey, tmp );

} /* createSettingsKeyFromCurr */

/*
 * regSaveSpecificJoyHW - save specific joystick hardware config. to the registry
 */
static void regSaveSpecificJoyHW( LPGLOBALVARS pgv )
{
    char	str[MAX_STR];
    HKEY	hkey;
    char	jcfg[MAX_STR];

    if( pgv->joyHWCurr.dwType == JOY_HW_NONE ) {
	return;
    }
    if( !(pgv->joyActiveFlags & HASJOY) ) {
	return;
    }

    createSettingsKeyFromCurr( pgv, str );
    if( !RegCreateKey( HKEY_LOCAL_MACHINE, str, &hkey ) ) {
	GETKEYNAME( pgv, jcfg, REGSTR_VAL_JOYNCONFIG );
	RegSetValueEx( hkey, jcfg, 0, REG_BINARY,
	    (CONST LPBYTE)&pgv->joyHWCurr, sizeof( pgv->joyHWCurr ));
	RegCloseKey( hkey );
    }

} /* regSaveSpecificJoyHW */

/*
 * regCreateCurrKey - create the current joystick settings key
 */
static HKEY regCreateCurrKey( LPGLOBALVARS pgv )
{
    HKEY	hkey;

    if( !RegCreateKey( HKEY_LOCAL_MACHINE, pgv->pjd->regCurrCfgKey, &hkey ) ) {
	return hkey;
    } else {
	return NULL;
    }

} /* regCreateCurrKey */

/*
 * RegSaveCurrentJoyHW - save the joystick info to the current entry in
 * 		      the registry
 */
void RegSaveCurrentJoyHW( LPGLOBALVARS pgv )
{
    HKEY	hkey;
    LPSTR	sptr;
    char	vname[MAX_STR];
    char	oname[MAX_STR];
    char	coname[MAX_STR];
    int		type;
    LPJOYDATA	pjd;

    if( pgv->joyHWCurr.dwType == JOY_HW_NONE ) {
	return;
    }
    if( !(pgv->joyActiveFlags & HASJOY) ) {
	return;
    }

    hkey = regCreateCurrKey( pgv );
    if( hkey == NULL ) {
	DPF( "Could not save current joystick settings!\r\n" );
	return;
    }
    pjd = pgv->pjd;
    if( pgv->joyHWCurr.dwUsageSettings & JOY_US_ISOEM ) {
	sptr = pjd->oemList[ pgv->joyHWCurr.dwType - JOY_HW_LASTENTRY ].keyname;
    }
    GETKEYNAME( pgv, vname, REGSTR_VAL_JOYNCONFIG );
    GETKEYNAME( pgv, oname, REGSTR_VAL_JOYNOEMNAME );
    GETKEYNAME( pgv, coname, REGSTR_VAL_JOYNOEMCALLOUT );

    RegSetValueEx( hkey, vname, 0, REG_BINARY,
		(CONST LPBYTE)&pgv->joyHWCurr, sizeof( pgv->joyHWCurr ) );
    if( pgv->joyHWCurr.dwUsageSettings & JOY_US_ISOEM ) {
	RegSetValueEx( hkey, oname, 0, REG_SZ, sptr, strlen( sptr ) + 1 );

	/*
	 * set up VxD name for this joystick
	 */
	type = pgv->joyHWCurr.dwType - JOY_HW_LASTENTRY;
	if( (pjd->oemList[type].vxd_name[0] != 0) ) {
	    RegSetValueEx( hkey, coname, 0, REG_SZ, pjd->oemList[type].vxd_name,
				strlen( pjd->oemList[type].vxd_name )+1 );
	} else {
	    RegDeleteValue( hkey, coname );
	}
    } else {
	RegDeleteValue( hkey, oname );
	RegDeleteValue( hkey, coname );
    }

    RegCloseKey( hkey );

} /* RegSaveCurrentJoyHW */

/*
 * regPermSaveAllInfo - save joystick data to the registry for good
 */
static void regPermSaveAllInfo( LPGLOBALVARS pgv )
{
    // save specific hardware settings to the registry
    regSaveSpecificJoyHW( pgv );

    // save current current hardware to the registry
    RegSaveCurrentJoyHW( pgv );

    RegistryUpdated( pgv );

} /* regPermSaveAllInfo */

/*
 * setHWCurrType - set the current hardware type (check for OEM type)
 */
static BOOL setHWCurrType( LPGLOBALVARS pgv, HKEY hkey, LPJOYREGHWCONFIG pcfg )
{
    char	str[MAX_STR];
    char	pname[MAX_STR];
    int		i;
    DWORD	regtype;
    DWORD	cb;
    LPJOYDATA	pjd;

    if( !(pcfg->dwUsageSettings & JOY_US_ISOEM) ) {
	return TRUE;
    }
    GETKEYNAME( pgv, pname, REGSTR_VAL_JOYNOEMNAME );
    cb = sizeof( str );
    if( RegQueryValueEx( hkey, pname, NULL, &regtype, (CONST LPBYTE)str, &cb)) {
	return FALSE;
    }
    if( regtype != REG_SZ ) {
	return FALSE;
    }
    pjd = pgv->pjd;
    for( i=0;i<pjd->oemCount;i++ ) {
	if( !_stricmp( str, pjd->oemList[i].keyname ) ) {
	    pcfg->dwType = i + JOY_HW_LASTENTRY;
	    return TRUE;
	}
    }
    return FALSE;

} /* setHWCurrType */

/*
 * regGetCurrHW - get the information about the current configuration
 *		from the registry
 */
static void regGetCurrHW( LPGLOBALVARS pgv )
{
    DWORD   regtype;
    DWORD   cb;
    JOYREGHWCONFIG   config;
    HKEY    hkey;
    char    str[MAX_STR];
    JOYCAPS jc;

    pgv->joyHWCurr = pgv->pjd->joyHWDefaults[ JOY_HW_NONE ];

    // Give the joystick driver a chance to write its current settings,
    // if there are none.
    //
    joyGetDevCaps( pgv->iJoyId, &jc, sizeof( jc ) );

    // Read those current settings (if they exist), and use them to determine
    // what type of joystick we've got
    //
    if ((hkey = regCreateCurrKey (pgv)) != NULL)
    {
        cb = sizeof( config );
        GETKEYNAME( pgv, str, REGSTR_VAL_JOYNCONFIG );
        if( !RegQueryValueEx( hkey, str, NULL,
                              &regtype, (CONST LPBYTE)&config, &cb)) {
            if( regtype == REG_BINARY && cb == sizeof( config ) ) {
                if( setHWCurrType( pgv, hkey, &config ) ) {
                    pgv->joyHWCurr = config;
                }
            }
        }
        RegCloseKey(  hkey );
    }

    // Does this joystick match a known type?
    //
    if (pgv->joyHWCurr.hws.dwNumButtons != 0)
    {
        int ii;
        for ( ii=0;ii<pgv->pjd->oemCount;ii++ )
        {
            if ( (pgv->pjd->oemList[ii].hws.dwFlags ==
                    pgv->joyHWCurr.hws.dwFlags) &&
                 (pgv->pjd->oemList[ii].hws.dwNumButtons ==
                    pgv->joyHWCurr.hws.dwNumButtons) )
            {
                pgv->joyHWCurr.dwType = ii + JOY_HW_LASTENTRY;
                break;
            }
        }

        if (pgv->joyHWCurr.dwType == JOY_HW_NONE)
        {
            pgv->joyHWCurr.dwType = JOY_HW_CUSTOM;
        }
    }
} /* regGetCurrHW */

/*
 * regGetOEMStr - get an OEM string
 */
static BOOL regGetOEMStr( HKEY hkey, LPSTR keyname, LPSTR buff, int size,
			LPSTR *res )
{
    DWORD	cb;
    DWORD	type;
    LPSTR	str;
    int		slen;

    cb = size;
    slen = 1;
    if( !RegQueryValueEx( hkey, keyname, NULL, &type, (CONST LPBYTE)buff, &cb ) ) {
	if( type == REG_SZ ) {
	    slen = strlen( buff ) + 1;
	}
    }
    str = DoAlloc( slen );
    if( str != NULL ) {
	if( slen == 1 ) {
	    str[0] = 0;
	} else {
	    strcpy( str, buff );
	}
    }
    *res = str;
    if( str == NULL ) {
	return TRUE;
    }
    return FALSE;

} /* regGetOEMStr */

/*
 * checkNonStandardUserVals
 */
static BOOL checkNonStandardUserVals( LPJOYREGUSERVALUES puv )
{
    if( (puv->jrvRanges.jpMin.dwX != RANGE_MIN) ||
	(puv->jrvRanges.jpMin.dwY != RANGE_MIN) ||
	(puv->jrvRanges.jpMin.dwZ != RANGE_MIN) ||
	(puv->jrvRanges.jpMin.dwR != RANGE_MIN) ||
	(puv->jrvRanges.jpMin.dwU != RANGE_MIN) ||
	(puv->jrvRanges.jpMin.dwV != RANGE_MIN) ||
	(puv->jrvRanges.jpMax.dwX != RANGE_MAX) ||
	(puv->jrvRanges.jpMax.dwY != RANGE_MAX) ||
	(puv->jrvRanges.jpMax.dwZ != RANGE_MAX) ||
	(puv->jrvRanges.jpMax.dwR != RANGE_MAX) ||
	(puv->jrvRanges.jpMax.dwU != RANGE_MAX) ||
	(puv->jrvRanges.jpMax.dwV != RANGE_MAX) ||
	(puv->dwTimeOut != 0x1000) ||
	(puv->jpDeadZone.dwX != 0) ||
	(puv->jpDeadZone.dwY != 0) ) {
	return TRUE;
    }
    return FALSE;

} /* checkNonStandardUserVals */

/*
 * regSetUserVals - set user values to our defaults
 */
static void regSetUserVals( LPJOYDATA pjd, BOOL retest )
{
    JOYREGUSERVALUES	uv;
    JOYREGUSERVALUES	ouv;
    HKEY		hkey;
    DWORD		regtype;
    DWORD		cb;

    if( !RegOpenKey( HKEY_LOCAL_MACHINE, pjd->regCfgKey, &hkey ) ) {
	/*
	 * build the default settings
	 */
	memset( &uv, 0, sizeof( uv ) );
	uv.dwTimeOut = 0x1000;
	uv.jpDeadZone.dwX = 0;
	uv.jpDeadZone.dwY = 0;
	uv.jrvRanges.jpMin.dwX = RANGE_MIN;
	uv.jrvRanges.jpMin.dwY = RANGE_MIN;
	uv.jrvRanges.jpMin.dwZ = RANGE_MIN;
	uv.jrvRanges.jpMin.dwR = RANGE_MIN;
	uv.jrvRanges.jpMin.dwU = RANGE_MIN;
	uv.jrvRanges.jpMin.dwV = RANGE_MIN;
	uv.jrvRanges.jpMax.dwX = RANGE_MAX;
	uv.jrvRanges.jpMax.dwY = RANGE_MAX;
	uv.jrvRanges.jpMax.dwZ = RANGE_MAX;
	uv.jrvRanges.jpMax.dwR = RANGE_MAX;
	uv.jrvRanges.jpMax.dwU = RANGE_MAX;
	uv.jrvRanges.jpMax.dwV = RANGE_MAX;

	if( retest ) {
	    /*
	     * see if the values have changed since we last set them:
	     * if yes, then we need to reset our remembered values
	     */
	    DPF( "Looking for USER entries\r\n" );
	    cb = sizeof( ouv );
	    if( !RegQueryValueEx( hkey, REGSTR_VAL_JOYUSERVALUES, NULL,
				    &regtype, (CONST LPBYTE)&ouv, &cb)) {
		DPF( "found REGSTR_VAL_JOYUSERVALUES\r\n" );
		if( regtype == REG_BINARY && cb == sizeof( ouv ) ) {
		    if( memcmp( &uv, &ouv, sizeof( uv ) ) ) {
			DPF( "USER entries changed!\r\n" );
			pjd->bHasUserVals = TRUE;
			pjd->bDeleteUserVals = FALSE;
			pjd->userVals = ouv;
		    }
		}
	    } else {
		if( pjd->bHasUserVals ) {
		    DPF( "USER entries changed, no longer exist!\r\n" );
		    pjd->bHasUserVals = FALSE;
		    pjd->bDeleteUserVals = TRUE;
		}
	    }
	}

	/*
	 * set our new values
	 */
	RegSetValueEx( hkey, REGSTR_VAL_JOYUSERVALUES, 0, REG_BINARY,
	    (CONST LPBYTE)&uv, sizeof( uv ) );
	RegCloseKey( hkey );
    }

} /* regSetUserVals */

/*
 * regUserValsInit - save old user values, and init to ones we like
 */
static void regUserValsInit( LPJOYDATA pjd )
{
    HKEY		hkey;
    DWORD		regtype;
    DWORD		cb;

    pjd->bHasUserVals = FALSE;
    pjd->bDeleteUserVals = FALSE;
    if( !RegOpenKey( HKEY_LOCAL_MACHINE, pjd->regCfgKey, &hkey ) ) {
	cb = sizeof( pjd->userVals );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_JOYUSERVALUES, NULL,
				&regtype, (CONST LPBYTE)&pjd->userVals, &cb)) {
	    if( regtype == REG_BINARY && cb == sizeof( pjd->userVals ) ) {
		pjd->bHasUserVals = TRUE;
		DPF( "USER entries exist!\r\n" );
	    }
	    pjd->bHasNonStandardUserVals = checkNonStandardUserVals( &pjd->userVals );
	} else {
	    pjd->bDeleteUserVals = TRUE;
	    pjd->bHasNonStandardUserVals = FALSE;
	    DPF( "USER entries don't exist!\r\n" );
	}
	RegCloseKey( hkey );
    }
    regSetUserVals( pjd, FALSE );

} /* regUserValsInit */

/*
 * regUserValsFini - restore old user values
 */
static void regUserValsFini( LPJOYDATA pjd )
{
    HKEY	hkey;
    if( pjd->bResetUserVals ) {
	RegDeleteValue( hkey, REGSTR_VAL_JOYUSERVALUES );
	RegistryUpdated( NULL );
    } else if( pjd->bHasUserVals || pjd->bDeleteUserVals ) {
	if( !RegOpenKey( HKEY_LOCAL_MACHINE, pjd->regCfgKey, &hkey ) ) {
	    if( pjd->bHasUserVals ) {
		DPF( "resetting USER entries!\r\n" );
		RegSetValueEx( hkey, REGSTR_VAL_JOYUSERVALUES, 0, REG_BINARY,
		    (CONST LPBYTE)&pjd->userVals, sizeof( pjd->userVals ) );
	    } else {
		DPF( "deleting USER entries!\r\n" );
		RegDeleteValue( hkey, REGSTR_VAL_JOYUSERVALUES );
	    }
	    RegistryUpdated( NULL );
	}
	pjd->bHasUserVals = FALSE;
	pjd->bDeleteUserVals = FALSE;
    }

} /* regUserValsFini */


/***************************************************************************
 
 
  CUSTOM JOYSTICK SELECTION FUNCTIONS FOLLOW
 
 
 ***************************************************************************/


/*
 * custom joystick variables
 */
typedef struct {
    LPGLOBALVARS	pgv;
    BOOL 		bHasZ;
    BOOL 		bHasR;
    BOOL 		bHasPOV;
    BOOL 		bIsYoke;
    BOOL 		bIsGamePad;
    BOOL 		bIsCarCtrl;
    BOOL 		bHas2Buttons;
} cust_vars, *LPCUSTVARS;


/*
 * enableCustomSpecial - enable the special section of the custom dialog box
 */
static void enableCustomSpecial( HWND hwnd, BOOL on )
{
    EnableWindow( GetDlgItem( hwnd, IDC_JOYISYOKE ), on );
    EnableWindow( GetDlgItem( hwnd, IDC_JOYISGAMEPAD ), on );
    EnableWindow( GetDlgItem( hwnd, IDC_JOYISCARCTRL ), on );
    CheckDlgButton( hwnd, IDC_JOYUSESPECIAL, on );
    if( !on ) {
	CheckDlgButton( hwnd, IDC_JOYISYOKE, FALSE );
	CheckDlgButton( hwnd, IDC_JOYISGAMEPAD, FALSE );
	CheckDlgButton( hwnd, IDC_JOYISCARCTRL, FALSE );
    }

} /* enableCustomSpecial */

/*
 * context help for the custom settings dialog
 */
    const static DWORD aCustomHelpIDs[] = {  // Context Help IDs
        IDC_GROUPBOX,       IDH_JOYSTICK_CUSTOM_AXES,
        IDC_GROUPBOX_2,     IDH_JOYSTICK_CUSTOM_BUTTONS,
        IDC_GROUPBOX_3,     IDH_JOYSTICK_GROUPBOX,
        IDC_JOY2AXIS,       IDH_JOYSTICK_CUSTOM_AXES,
        IDC_JOY3AXIS,       IDH_JOYSTICK_CUSTOM_AXES,
        IDC_JOY4AXIS,       IDH_JOYSTICK_CUSTOM_AXES,
        IDC_JOY2BUTTON,     IDH_JOYSTICK_CUSTOM_BUTTONS,
        IDC_JOY4BUTTON,     IDH_JOYSTICK_CUSTOM_BUTTONS,
        IDC_JOYHASPOV,      IDH_JOYSTICK_CUSTOM_POV_HAT,
        IDC_JOYISYOKE,      IDH_JOYSTICK_CUSTOM_FLIGHT_YOKE,
        IDC_JOYISGAMEPAD,   IDH_JOYSTICK_CUSTOM_GAME_PAD,
        IDC_JOYISCARCTRL,   IDH_JOYSTICK_CUSTOM_CAR_CONTROL,
        IDC_JOYUSESPECIAL,  IDH_JOYSTICK_CUSTOM_CUSTOM_FEATURES,

        0, 0
    };

/*
 * CustomProc - callback procedure for custom joystick setup
 */
BOOL CALLBACK CustomProc( HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    int			id;
    LPGLOBALVARS	pgv;
    LPCUSTVARS		pcv;

    switch( umsg ) {
    case WM_DESTROY:
    	/*
	 * don't free the dialog's variables here, they are returned to the
	 * creator; the creator will free them
	 */
	break;
    case WM_INITDIALOG:
    	/*
	 * create variables for the custom dialog
	 */
    	pcv = DoAlloc( sizeof( cust_vars ) );
	SetWindowLong( hwnd, DWL_USER, (LONG) pcv );
	if( pcv == NULL ) {
	    EndDialog( hwnd, 0 );
	    return FALSE;
	}
    	pgv = (LPGLOBALVARS) lParam;
	pcv->pgv = pgv;

	/*
	 * set up initial dialog state
	 */
	pcv->bHasZ = (pgv->joyHWCurr.hws.dwFlags & JOY_HWS_HASZ);
	pcv->bHasR = (pgv->joyHWCurr.hws.dwFlags & JOY_HWS_HASR);
	pcv->bHas2Buttons = (pgv->joyHWCurr.hws.dwNumButtons == 2);
	pcv->bHasPOV = (pgv->joyHWCurr.hws.dwFlags & JOY_HWS_HASPOV);
	pcv->bIsYoke = (pgv->joyHWCurr.hws.dwFlags & JOY_HWS_ISYOKE);
	pcv->bIsGamePad = (pgv->joyHWCurr.hws.dwFlags & JOY_HWS_ISGAMEPAD);
	pcv->bIsCarCtrl = (pgv->joyHWCurr.hws.dwFlags & JOY_HWS_ISCARCTRL);
	if( pcv->bHasZ && pcv->bHasR ) {
	    CheckRadioButton( hwnd, IDC_JOY2AXIS, IDC_JOY4AXIS, IDC_JOY4AXIS );
	} else if( pcv->bHasZ ) {
	    CheckRadioButton( hwnd, IDC_JOY2AXIS, IDC_JOY4AXIS, IDC_JOY3AXIS );
	} else {
	    CheckRadioButton( hwnd, IDC_JOY2AXIS, IDC_JOY4AXIS, IDC_JOY2AXIS );
	}
	if( pcv->bHas2Buttons ) {
	    CheckRadioButton( hwnd, IDC_JOY2BUTTON, IDC_JOY4BUTTON, IDC_JOY2BUTTON );
	} else {
	    CheckRadioButton( hwnd, IDC_JOY2BUTTON, IDC_JOY4BUTTON, IDC_JOY4BUTTON );
	}
	CheckDlgButton( hwnd, IDC_JOYHASPOV, pcv->bHasPOV );
	id = -1;
	if( pcv->bIsYoke ) {
	    id = IDC_JOYISYOKE;
	} else if( pcv->bIsGamePad ) {
	    id = IDC_JOYISGAMEPAD;
	} else if( pcv->bIsCarCtrl ) {
	    id = IDC_JOYISCARCTRL;
	}
	if( id != -1 ) {
	    enableCustomSpecial( hwnd, TRUE );
	    CheckRadioButton( hwnd, IDC_JOYISYOKE, IDC_JOYISCARCTRL, id );
	} else {
	    enableCustomSpecial( hwnd, FALSE );
	}
	return FALSE;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, cszHelpFile,
            HELP_WM_HELP, (DWORD)(LPSTR) aCustomHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, cszHelpFile, HELP_CONTEXTMENU,
            (DWORD)(LPVOID) aCustomHelpIDs);
        return TRUE;

    case WM_COMMAND:
	pcv = (LPCUSTVARS) GetWindowLong( hwnd, DWL_USER );
	id = GET_WM_COMMAND_ID(wParam, lParam);
	switch( id ) {
	case IDC_JOY2AXIS:
	case IDC_JOY3AXIS:
	case IDC_JOY4AXIS:
	    CheckRadioButton( hwnd, IDC_JOY2AXIS, IDC_JOY4AXIS, id );
	    pcv->bHasZ = FALSE;
	    pcv->bHasR = FALSE;
	    if( id == IDC_JOY3AXIS ) {
		pcv->bHasZ = TRUE;
	    } else if( id == IDC_JOY4AXIS ) {
		pcv->bHasZ = TRUE;
		pcv->bHasR = TRUE;
	    }
	    break;
	case IDC_JOY2BUTTON:
	case IDC_JOY4BUTTON:
	    CheckRadioButton( hwnd, IDC_JOY2BUTTON, IDC_JOY4BUTTON, id );
	    pcv->bHas2Buttons = (id == IDC_JOY2BUTTON);
	    break;
	case IDC_JOYUSESPECIAL:
	    enableCustomSpecial( hwnd, IsDlgButtonChecked( hwnd, IDC_JOYUSESPECIAL ) );
	    pcv->bIsYoke = FALSE;
	    pcv->bIsGamePad = FALSE;
	    pcv->bIsCarCtrl = FALSE;
	    break;
	case IDC_JOYHASPOV:
	    pcv->bHasPOV = !pcv->bHasPOV;
	    break;
	case IDC_JOYISYOKE:
	case IDC_JOYISGAMEPAD:
	case IDC_JOYISCARCTRL:
	    pcv->bIsYoke = (id == IDC_JOYISYOKE);
	    pcv->bIsGamePad = (id == IDC_JOYISGAMEPAD);
	    pcv->bIsCarCtrl = (id == IDC_JOYISCARCTRL);
	    CheckRadioButton( hwnd, IDC_JOYISYOKE, IDC_JOYISCARCTRL, id );
	    break;
	case IDCANCEL:
	    pcv = (LPCUSTVARS) GetWindowLong( hwnd, DWL_USER );
	    DoFree( pcv );
	    EndDialog( hwnd, 0 );
	    break;
	case IDOK:
	    pcv = (LPCUSTVARS) GetWindowLong( hwnd, DWL_USER );
	    EndDialog(hwnd, (int) pcv );
	    break;
	}
	break;
    default:
	break;
    }
    return FALSE;

} /* CustomProc */


/***************************************************************************
 
 
  MAIN DIALOG FUNCTIONS FOLLOW
 
 
 ***************************************************************************/


/*
 * variables used by joystick tab dialog
 */
typedef struct {
    LPGLOBALVARS	pgv;
} JTVARS, *LPJTVARS;

/*
 * numJoyAxes - get number of axes on a joystick
 */
static int numJoyAxes( LPGLOBALVARS pgv )
{
    DWORD	flags;
    int		axis_count;

    flags = pgv->joyHWCurr.hws.dwFlags;
    axis_count = 2;
    if( flags & JOY_HWS_HASZ ) {
	axis_count++;
    }
    if( flags & JOY_HWS_HASR ) {
	axis_count++;
    }
    if( (flags & JOY_HWS_HASPOV) && (flags & JOY_HWS_POVISPOLL) ) {
	axis_count++;
    }
    return axis_count;

} /* numJoyAxes */

/*
 * saveHWSettings - save the current hardware settings
 */
static void saveHWSettings( LPGLOBALVARS pgv )
{
    pgv->joyHWOrig = pgv->joyHWCurr;

} /* saveHWSettings */

/*
 * restoreHWSettings - restore current hw settings to saved values
 */
static void restoreHWSettings( LPGLOBALVARS pgv )
{
    pgv->joyHWCurr = pgv->joyHWOrig;
    RegSaveCurrentJoyHW( pgv );

} /* restoreHWSettings */

/*
 * getActiveFlags - poll and test which joysticks are currently plugged in
 */
static unsigned getActiveFlags( LPGLOBALVARS pgv )
{
    JOYINFOEX	ji;
    MMRESULT	rc;
    unsigned	val;

    /*
     * check for presense of joystick 1 and joystick 2
     */
    val = 0;
    ji.dwSize = sizeof( ji );
    ji.dwFlags = JOY_RETURNX|JOY_RETURNY|JOY_CAL_READXYONLY|JOY_CAL_READALWAYS;
    rc = joyGetPosEx( pgv->iJoyId, &ji );
    DPF( "joyGetPosEx = %d\r\n", rc );
    if( rc == JOYERR_NOERROR ) {
	val = HASJOY;
    }

    /*
     * check if either could have a rudder attached.
     */
    ji.dwFlags = JOY_RETURNR | JOY_CAL_READRONLY;
    if( (numJoyAxes( pgv ) < 4) &&
    		!(pgv->joyHWCurr.hws.dwFlags & JOY_HWS_HASR ) ) {
	rc = joyGetPosEx( pgv->iJoyId, &ji );
	if( rc ==JOYERR_NOERROR ) {
	    val |= HASRUDDERMAYBE;
	}
    }
    return val;

} /* getActiveFlags */

/*
 * enableTestCal - enable/disable test and calibrate buttons
 */
static void enableTestCal( HWND hwnd, int hw_type )
{
    BOOL	enable;

    enable = (hw_type != JOY_HW_NONE);

    EnableWindow( GetDlgItem( hwnd, IDC_JOYCALIBRATE ), enable );
    EnableWindow( GetDlgItem( hwnd, IDC_JOYTEST ), enable );

} /* enableTestCal */

/*
 * cleanUpJoyDlg - clean up allocated stuff
 */
static void cleanUpJoyDlg( HWND hwnd )
{
    LPGLOBALVARS	pgv;

    pgv = (LPGLOBALVARS) GetWindowLong( hwnd, DWL_USER );
    if( pgv == NULL ) {
	return;
    }

    /*
     * ditch timer
     */
    if( pgv->pjd->bHasTimer ) {
	KillTimer( hwnd, TIMER_ID );
	pgv->pjd->bHasTimer = FALSE;
    }

    /*
     * done with our variables
     */
    #if defined( WANT_SHEETS )
	DoFree( pgv );
    #endif

} /* cleanUpJoyDlg */

/*
 * enableJoyWindows - enable controls for a joystick
 */
static void  enableJoyWindows( LPGLOBALVARS pgv, HWND hwnd, BOOL enable )
{
//    EnableWindow( GetDlgItem(hwnd,IDC_JOYSELECT), enable );
//    EnableWindow( GetDlgItem(hwnd,IDC_JOYSELECTMSG), enable );
//    EnableWindow( GetDlgItem(hwnd,IDC_JOYSTICK1_FRAME), enable );
    EnableWindow( GetDlgItem(hwnd,IDC_JOYCALIBRATE), enable );
    EnableWindow( GetDlgItem(hwnd,IDC_JOYTEST), enable );

} /* enableJoyWindows */

/*
 * enableActiveJoystick - enable dialog controls based on presence of joysticks
 */
static void enableActiveJoystick( LPGLOBALVARS pgv, HWND hwnd )
{
    BOOL		allowj;
    HINSTANCE		hinst;
    char		str[MAX_STR];
    unsigned		joys;
    LPSTR		text;

    /*
     * check what joysticks are active; if it hasn't changed, just return
     */
    joys = getActiveFlags( pgv );
    if( pgv->joyActiveFlags == joys ) {
	return;
    }
    pgv->joyActiveFlags = joys;

    /*
     * turn off the rudder if it is gone
     */
    if( !(joys & HASRUDDERMAYBE) ) {
	pgv->joyHWCurr.dwUsageSettings &= ~JOY_US_HASRUDDER;
	CheckDlgButton( hwnd, IDC_JOY1HASRUDDER, FALSE );
    }

    /*
     * enable the appropriate windows
     */
    allowj = ((joys & HASJOY) != 0);
    enableJoyWindows( pgv, hwnd, allowj );
    EnableWindow( GetDlgItem( hwnd, IDC_JOY1HASRUDDER ), allowj && (joys & HASRUDDERMAYBE) );

    /*
     * set message for the user if there is no joystick plugged in, or if
     * there is no joystick driver present
     */
    if( allowj ) {
	text = "";
    } else {
	str[0] = 0;
	text = str;
	hinst = GetWindowInstance( hwnd );
	if( joyGetNumDevs() ) {
	    LoadString( hinst , IDS_JOYUNPLUGGED, str, sizeof( str ) );
	} else {
	    LoadString( hinst , IDS_JOYNOTPRESENT, str, sizeof( str ) );
	}
    }
    SetWindowText( GetDlgItem( hwnd, IDC_JOYMSG ), text );

    if( allowj ) {
	enableTestCal( hwnd, pgv->joyHWCurr.dwType );
    }

    if( allowj ) {
	pgv->joyHWCurr.dwUsageSettings |= JOY_US_PRESENT;
    } else {
	pgv->joyHWCurr.dwUsageSettings &= ~JOY_US_PRESENT;
    }

    RegSaveCurrentJoyHW( pgv );
    RegistryUpdated( pgv );

} /* enableActiveJoystick */

/*
 * getNewJoyInfo - get information from the registry about a new joystick.
 *		   If no info, default to joyHWDefault settings
 */
static void getNewJoyInfo( LPGLOBALVARS pgv, HWND hwnd )
{
    UINT                index;
    DWORD		hw_type;
    HKEY		hkey;
    char		str[MAX_STR];
    char		jcfg[MAX_STR];
    DWORD		regtype;
    JOYREGHWCONFIG	config;
    DWORD		cb;
    BOOL		same;
    int			rc;

    GETKEYNAME( pgv, jcfg, REGSTR_VAL_JOYNCONFIG );

    /*
     * get the hardware type
     */
    index = SendDlgItemMessage( hwnd, IDC_JOYSELECT, CB_GETCURSEL, 0, 0L );
    hw_type = SendDlgItemMessage( hwnd,IDC_JOYSELECT,CB_GETITEMDATA,index,0L );
    same = (hw_type == pgv->joyHWCurr.dwType);

    /*
     * read the info from the registry if a new hardware type selected
     */
    if( !same ) {
	pgv->joyHWCurr = pgv->pjd->joyHWDefaults[ hw_type ];
	createSettingsKeyFromCurr( pgv, str );
	if( !RegOpenKey( HKEY_LOCAL_MACHINE, str, &hkey ) ) {
	    cb = sizeof( pgv->joyHWCurr );
	    if( !RegQueryValueEx( hkey, jcfg, NULL, &regtype,
			    (CONST LPBYTE)&config, &cb) ) {
		if( regtype == REG_BINARY && cb == sizeof( config ) ) {
		    pgv->joyHWCurr.hws = config.hws;
		    pgv->joyHWCurr.hwv = config.hwv;
		    pgv->joyHWCurr.dwUsageSettings = config.dwUsageSettings;
		}
	    }
	    RegCloseKey( hkey );
	}

	/*
	 * set up the rudder bit
	 */
	if( pgv->joyHWCurr.dwUsageSettings & JOY_US_HASRUDDER ) {
	    CheckDlgButton( hwnd, IDC_JOY1HASRUDDER, TRUE );
	} else {
	    if( IsDlgButtonChecked( hwnd, IDC_JOY1HASRUDDER ) ) {
		pgv->joyHWCurr.dwUsageSettings |= JOY_US_HASRUDDER;
	    } else {
		pgv->joyHWCurr.dwUsageSettings &= ~JOY_US_HASRUDDER;
	    }
	}
    }

    /*
     * disable test/calibrate buttons based on hardware picked
     */
    enableTestCal( hwnd, hw_type );

    /*
     * if custom selected, go get the data from the user
     */
    if( hw_type == JOY_HW_CUSTOM ) {
    	rc = DialogBoxParam((HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE),
		    MAKEINTRESOURCE(IDD_JOYCUSTOM), hwnd,
		    CustomProc, (LONG) pgv );
	if( rc ) {
	    LPCUSTVARS	pcv;

	    pcv = (LPCUSTVARS) rc;
	    pgv->joyHWCurr.dwUsageSettings |= JOY_US_PRESENT;
	    pgv->joyHWCurr.hws.dwFlags &= ~(JOY_HWS_HASR|JOY_HWS_HASZ|
			JOY_HWS_HASU| JOY_HWS_HASV|
	    		JOY_HWS_HASPOV|JOY_HWS_ISYOKE| JOY_HWS_ISGAMEPAD|
			JOY_HWS_ISCARCTRL| JOY_HWS_POVISPOLL|
			JOY_HWS_POVISBUTTONCOMBOS );
	    /*
	     * NOTE: for a custom joystick, we always assume that Z is
	     * implemented on J2 Y.
	     */
	    if( pcv->bHasZ ) {
		pgv->joyHWCurr.hws.dwFlags |= JOY_HWS_HASZ;
	    }
	    /*
	     * NOTE: for a custom joystick, we always assume that R is
	     * implemented on J2 X.
	     */
	    if( pcv->bHasR ) {
		pgv->joyHWCurr.hws.dwFlags |= JOY_HWS_HASR;
	    }
	    if( pcv->bHasPOV ) {
		pgv->joyHWCurr.hws.dwFlags |= JOY_HWS_HASPOV;
	    }
	    if( pcv->bIsYoke ) {
		pgv->joyHWCurr.hws.dwFlags |= JOY_HWS_ISYOKE;
	    }
	    if( pcv->bIsGamePad ) {
		pgv->joyHWCurr.hws.dwFlags |= JOY_HWS_ISGAMEPAD;
	    }
	    if( pcv->bIsCarCtrl ) {
		pgv->joyHWCurr.hws.dwFlags |= JOY_HWS_ISCARCTRL;
	    }
	    if( pcv->bHas2Buttons ) {
		pgv->joyHWCurr.hws.dwNumButtons = 2;
	    } else {
		pgv->joyHWCurr.hws.dwNumButtons = 4;
	    }
	    DoFree( pcv );
	    same = FALSE;
	}
    }

    /*
     * update the registry with the new current joystick
     */
    if( !same ) {
	RegSaveCurrentJoyHW( pgv );
	RegistryUpdated( pgv );
	PropSheet_Changed( GetParent(hwnd), hwnd );
	pgv->joyActiveFlags = (unsigned) -1;
	enableActiveJoystick( pgv, hwnd );
    }

} /* getNewJoyInfo */

/*
 * initCurrentHW - set up the current hardware for the first  time
 */
static void initCurrentHW( LPGLOBALVARS pgv )
{
    regGetCurrHW( pgv );
    pgv->joyActiveFlags = (unsigned) -1;
    saveHWSettings( pgv );

} /* initCurrentHW */

/*
 * newJoyId - set up for a new joystick id
 */
static LPGLOBALVARS newJoyId( LPGLOBALVARS pgv, HWND hwnd, int joyid )
{
    UINT  index;
    UINT  indexMax;

    if( joyid == pgv->iJoyId ) {
	return pgv;
    }
    #if !defined( WANT_SHEETS )
    	pgv = &pgv->pjd->pgvlist[ joyid ];
    #endif
    pgv->iJoyId = joyid;

    /*
     * save the pointer to the variables
     */
    SetWindowLong( hwnd, DWL_USER, (LONG) pgv );

    #if defined( WANT_SHEETS )
	/*
	 * set up current joystick hardware
	 */
	initCurrentHW( pgv );
    #endif

    /*
     * set up windows
     */
    pgv->joyActiveFlags = (unsigned) -1;
    enableActiveJoystick( pgv, hwnd );
    if( pgv->joyHWCurr.dwUsageSettings & JOY_US_HASRUDDER ) {
	CheckDlgButton( hwnd, IDC_JOY1HASRUDDER, TRUE );
    } else {
	CheckDlgButton( hwnd, IDC_JOY1HASRUDDER, FALSE );
    }

    /*
     * select the current info
     */
   indexMax = SendDlgItemMessage( hwnd, IDC_JOYSELECT, CB_GETCOUNT, 0, 0 );

   for (index = 0; index < indexMax; index++)
   {
       DWORD type;
       type = SendDlgItemMessage( hwnd, IDC_JOYSELECT, CB_GETITEMDATA,
                                  index, 0 );
       if (type == pgv->joyHWCurr.dwType)
           break;
   }

   if (index == indexMax)
      index = 1;	// custom

   SendDlgItemMessage( hwnd, IDC_JOYSELECT, CB_SETCURSEL, index, 0L );
    #if !defined( WANT_SHEETS )
	SendDlgItemMessage( hwnd, IDC_JOYCURRENTID, CB_SETCURSEL, pgv->iJoyId, 0L );
    #endif
    return pgv;


} /* newJoyId */

/*
 * showResetInfo
 */
static void showResetInfo( HWND hwnd, BOOL show )
{
    EnableWindow( GetDlgItem( hwnd, IDC_JOYTROUBLESHOOT_FRAME ), show );
    EnableWindow( GetDlgItem( hwnd, IDC_JOYTROUBLESHOOT_TEXT ), show );
    EnableWindow( GetDlgItem( hwnd, IDC_JOYRESET ), show );

} /* showResetInfo */

/*
 * doJoyDlgInitDialog - process initialization for joystick tabbed dialog
 */
static BOOL doJoyDlgInitDialog( HWND hwnd, LPARAM lParam )
{
    HINSTANCE		hinst;
    LPPROPSHEETPAGE	ppsp;
    int			i;
    char		str[MAX_STR];
    LPGLOBALVARS	pgv;
    LPJOYDATA		pjd;
    LPJOYDATAPTR	pjdp;
    HKEY		hkey;

    /*
     * pointer to data
     */
    ppsp = (LPPROPSHEETPAGE) lParam;
    pjdp = (LPJOYDATAPTR) ppsp->lParam;
    pjd = pjdp->pjd;

    /*
     * create global variables.   These will be used by all dialogs
     */
    #if defined( WANT_SHEETS )
	pgv = DoAlloc( sizeof( GLOBALVARS ) );
	if( pgv == NULL ) {
	    return FALSE;
	}

	/*
	 * get joystick id that this sheet is for
	 */
	pgv->iJoyId = pjdp->iJoyId;
	pgv->pjd = pjd;
	DPF( "Tab for joystick %d started\r\n", pgv->iJoyId );
    #else
    	pgv = &pjd->pgvlist[ pjdp->iJoyId ];
    #endif

    /*
     * get device caps
     */
    getDevCaps( pgv );

    /*
     * how many predefined joystick types should we display?
     */
    pgv->cJoystickTypes = 2;	// display None and Custom only
    if( !RegOpenKey( HKEY_LOCAL_MACHINE, REGSTR_PATH_JOYSTICK, &hkey ) ) {
	DWORD type;
	DWORD val = 0;
	DWORD cb = sizeof(val);
        if( !RegQueryValueEx( hkey, REGSTR_VAL_JOYTYPES, NULL,
            &type, (CONST LPBYTE)&val, &cb) ) {
	    pgv->cJoystickTypes += val;
	}
        RegCloseKey( hkey );
    }
    pgv->cJoystickTypes = max( pgv->cJoystickTypes,  2 ); // Must have none/cust
    pgv->cJoystickTypes = min( pgv->cJoystickTypes, 12 );

    /*
     * callback timer for checking if joysticks are plugged/unplugged
     */
     if( !pjd->bHasTimer ) {
	pjd->bHasTimer = SetTimer( hwnd, TIMER_ID, JOYCHECKTIME, NULL );
	pjd->bUseTimer = TRUE;
     }

    /*
     * set up pre-defined joystick list
     */
    hinst = GetWindowInstance( hwnd );
    for( i=IDS_JOYHW0; i<(IDS_JOYHW0 + pgv->cJoystickTypes); i++ ) {
	if( LoadString( hinst , i, str, sizeof( str ) ) ) {
	    UINT dwItem;
	    dwItem = SendDlgItemMessage( hwnd, IDC_JOYSELECT, CB_ADDSTRING, 0,
				(LONG) (LPSTR) str );
	    SendDlgItemMessage( hwnd, IDC_JOYSELECT, CB_SETITEMDATA, dwItem,
				i -IDS_JOYHW0 + JOY_HW_NONE );
	}
    }

    /*
     * set up OEM joystick list
     */
    for( i=0;i<pjd->oemCount;i++ ) {
	UINT dwItem;
	dwItem = SendDlgItemMessage( hwnd, IDC_JOYSELECT, CB_ADDSTRING, 0,
			    (LONG) (LPSTR) pjd->oemList[i].ident_string );
	SendDlgItemMessage( hwnd, IDC_JOYSELECT, CB_SETITEMDATA,
			     dwItem, i +JOY_HW_LASTENTRY);
    }

    /*
     * set up joystick choices list
     */
    #if !defined(WANT_SHEETS)
    {
	int	numdevs;
	char	strid[MAX_STR];
	if( LoadString( hinst, IDS_JOY, str, sizeof( str ) ) ) {
	    numdevs = joyGetNumDevs();
	    for( i=0;i<numdevs;i++ ) {
		wsprintf( strid, "%s %d", str, i+1 );
		SendDlgItemMessage( hwnd, IDC_JOYCURRENTID, CB_ADDSTRING, 0,
				    (LONG) (LPSTR) strid );
	    }
	}
    }
    #endif

    pgv->iJoyId = -1;
    newJoyId( pgv, hwnd, pjdp->iJoyId );

    /*
     * enable/disable our Reset button
     */
    showResetInfo( hwnd, pjd->bHasNonStandardUserVals );

    return TRUE;

} /* doJoyDlgInitDialog */

/*
 * doJoyDlgCommand - process WM_COMMAND message for main joystick tabbed dialog
 */
static void doJoyDlgCommand( HWND hwnd, int id, HWND hctl, UINT code )
{
    LPGLOBALVARS	pgv;

    pgv = (LPGLOBALVARS) GetWindowLong( hwnd, DWL_USER );

    switch( id ) {
    /*
     * new joystick has been picked
     */
    case IDC_JOYSELECT:
    	if( code == CBN_SELCHANGE ) {
	    getNewJoyInfo( pgv, hwnd );
	}
	break;

    #if !defined( WANT_SHEET )
    /*
     * new joystick id has been picked
     */
    case IDC_JOYCURRENTID:
    	if( code == CBN_SELCHANGE ) {
	    int	joyid;
	    joyid = SendDlgItemMessage( hwnd, IDC_JOYCURRENTID, CB_GETCURSEL, 0, 0L );
	    pgv = newJoyId( pgv, hwnd, joyid );
	    regSetUserVals( pgv->pjd, TRUE );
	    RegSaveCurrentJoyHW( pgv );
	    RegistryUpdated( pgv );
	}
	break;
    #endif

    /*
     * calibrate current joystick
     */
    case IDC_JOYCALIBRATE:
	pgv->pjd->bUseTimer = FALSE;
	DoCalibrate( pgv, hwnd );
	pgv->pjd->bUseTimer = TRUE;
	break;

    /*
     * test either joystick 1 or joystick 2
     */
    case IDC_JOYTEST:
	pgv->pjd->bUseTimer = FALSE;
	DoTest( pgv, hwnd, NULL, pgv );
	pgv->pjd->bUseTimer = TRUE;
	break;

    /*
     * reset to user values
     */
    case IDC_JOYRESET:
    	pgv->pjd->bResetUserVals = TRUE;
	PropSheet_Changed( GetParent(hwnd), hwnd );
    	break;

    /*
     * rudder selected/unselected
     */
    case IDC_JOY1HASRUDDER:
    {
	LPJOYREGHWCONFIG 	pcfg;
	/*
	 * rudder status changed, force recalibration (leave POV alone if
	 * it was button based)
	 */
	pcfg = &pgv->joyHWCurr;

	if( (pcfg->hws.dwFlags & JOY_HWS_HASPOV) &&
			(pcfg->hws.dwFlags & JOY_HWS_POVISBUTTONCOMBOS) ) {
	    pcfg->hwv.dwCalFlags &= JOY_ISCAL_POV;
	} else {
	    pcfg->hwv.dwCalFlags = 0;
	}

	if( IsDlgButtonChecked( hwnd, id ) ) {
	    pcfg->dwUsageSettings |= JOY_US_HASRUDDER;
	} else {
	    pcfg->dwUsageSettings &= ~JOY_US_HASRUDDER;
	}
	pgv->joyActiveFlags = (unsigned) -1;
	enableActiveJoystick( pgv, hwnd );
	PropSheet_Changed( GetParent(hwnd), hwnd );
	break;
    }
	
    case ID_APPLY:
    {
    	DPF( "ID_APPLY\r\n" );

	#if !defined( WANT_SHEETS )
	{
	    int	i;
	    int	numjoys;

	    numjoys = joyGetNumDevs();
	    for( i=0;i<numjoys;i++ ) {
		regPermSaveAllInfo( &pgv->pjd->pgvlist[i] );
		saveHWSettings( &pgv->pjd->pgvlist[i] );
	    }
	}
	#else
	    regPermSaveAllInfo( pgv );
	    saveHWSettings( pgv );
	#endif
    	if( pgv->pjd->bResetUserVals ) {
	    regUserValsFini( pgv->pjd );
	    regUserValsInit( pgv->pjd );
	    pgv->pjd->bResetUserVals = FALSE;
	}
	showResetInfo( hwnd, pgv->pjd->bHasNonStandardUserVals );
	break;
    }
    case ID_INIT:
    	DPF( "ID_INIT\r\n" );
    	/*
	 * we've been re-activated, reset the current joystick settings
	 */
	regSetUserVals( pgv->pjd, TRUE );
	RegSaveCurrentJoyHW( pgv );
	RegistryUpdated( pgv );
	break;
    case IDOK:
    	DPF( "IDOK\r\n" );
	EndDialog(hwnd, TRUE );
	break;
    case IDCANCEL:
    	DPF( "IDCANCEL\r\n" );
    	pgv->pjd->bResetUserVals = FALSE;
	#if !defined( WANT_SHEETS )
	{
	    int	i;
	    int	numjoys;

	    numjoys = joyGetNumDevs();
	    for( i=0;i<numjoys;i++ ) {
		restoreHWSettings( &pgv->pjd->pgvlist[i] );
	    }
	}
	#else
	    restoreHWSettings( pgv );
	#endif
	RegistryUpdated( pgv );
	EndDialog(hwnd, FALSE );
	break;
	
    default:
	break;
    }

} /* doJoyDlgCommand */

/*
 * context help for the main dialog
 */
    const static DWORD aJoystickHelpIDs[] = {  // Context Help IDs
        IDC_JOYCURRENTIDMSG,        IDH_JOYSTICK_CURRENT,
        IDC_JOYCURRENTID,           IDH_JOYSTICK_CURRENT,
        IDC_JOYSELECTMSG,           IDH_JOYSTICK_SELECT,
        IDC_JOYSELECT,              IDH_JOYSTICK_SELECT,
        IDC_JOY1HASRUDDER,          IDH_JOYSTICK_RUDDER,
        IDC_JOYCALIBRATE,           IDH_JOYSTICK_CALIBRATE,
        IDC_JOYTEST,                IDH_JOYSTICK_TEST,
        IDC_JOYSTICK1_FRAME,        IDH_JOYSTICK_GROUPBOX,
        IDC_JOYMSG,                 NO_HELP,
        IDC_ICON_1,                 NO_HELP,
        IDC_ICON_2,                 NO_HELP,
        IDC_JOYTROUBLESHOOT_FRAME,  IDH_JOYSTICK_RESET,
        IDC_JOYRESET,               IDH_JOYSTICK_RESET,
        IDC_JOYTROUBLESHOOT_TEXT,   NO_HELP,

        0, 0
    };

/*
 * JoystickDlg - dialog procedure for joystick tabbed dialog
 */
BOOL CALLBACK JoystickDlg( HWND	hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    BOOL	rc;

    switch( umsg ) {
    case WM_INITDIALOG:
	rc = doJoyDlgInitDialog( hwnd, lParam );
	if( !rc ) {
	    EndDialog( hwnd, 0 );
	}
	return FALSE;
	
    case WM_COMMAND:
	HANDLE_WM_COMMAND( hwnd, wParam, lParam, doJoyDlgCommand );
	break;

    case WM_ACTIVATE:
    	/*
	 * we've been activated, pretend we were re-selected
	 */
    	if( LOWORD( wParam ) != WA_INACTIVE ) {
	    FORWARD_WM_COMMAND( hwnd, ID_INIT, 0, 0, SendMessage );
	}
	break;

    case WM_DESTROY:
	cleanUpJoyDlg( hwnd );
	break;

    case WM_TIMER:
    {
	LPGLOBALVARS	pgv;
    	pgv = (LPGLOBALVARS) GetWindowLong( hwnd, DWL_USER );
    	if( pgv->pjd->bUseTimer ) {
	    pgv->pjd->bUseTimer = FALSE;
	    enableActiveJoystick( pgv, hwnd );
	    pgv->pjd->bUseTimer = TRUE;
	}
	break;
    }

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, cszHelpFile,
            HELP_WM_HELP, (DWORD)(LPSTR) aJoystickHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, cszHelpFile, HELP_CONTEXTMENU,
            (DWORD)(LPVOID) aJoystickHelpIDs);
        return TRUE;

    case WM_NOTIFY:
    {
	NMHDR FAR * lpnm = (NMHDR FAR *)lParam;
	switch(lpnm->code) {
	case PSN_KILLACTIVE:
	    FORWARD_WM_COMMAND( hwnd, IDOK, 0, 0, SendMessage );
	    return TRUE;		    

	case PSN_APPLY:
	    FORWARD_WM_COMMAND( hwnd, ID_APPLY, 0, 0, SendMessage );
	    return TRUE;

	case PSN_SETACTIVE:
	    FORWARD_WM_COMMAND( hwnd, ID_INIT, 0, 0, SendMessage );
	    return TRUE;
	    
	case PSN_RESET:
	    FORWARD_WM_COMMAND( hwnd, IDCANCEL, 0, 0, SendMessage );
	    return  TRUE;
	}
	break;
    }
    default:
	break;
    }
    return FALSE;

} /* JoystickDlg */


/***************************************************************************
 
 
  GLOBAL JOYSTICK DATA FUNCTIONS FOLLOW
 
 
 ***************************************************************************/

/*
 * default joysticks
 */
#define TYPE00	0
#define TYPE01	0
#define TYPE02	0
#define TYPE03	0
#define TYPE04	JOY_HWS_ISGAMEPAD
#define TYPE05	JOY_HWS_ISYOKE
#define TYPE06	JOY_HWS_HASZ | JOY_HWS_ISYOKE
#define TYPE07	JOY_HWS_HASZ
#define TYPE08	JOY_HWS_HASZ
#define TYPE09	JOY_HWS_ISGAMEPAD
#define TYPE10	JOY_HWS_ISYOKE
#define TYPE11	JOY_HWS_HASZ | JOY_HWS_ISYOKE

static JOYREGHWCONFIG _joyHWDefaults[] =
{
    { {TYPE00,0},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_NONE},
    { {TYPE01,2},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_CUSTOM},
    { {TYPE02,2},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_2A_2B_GENERIC},
    { {TYPE03,4},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_2A_4B_GENERIC},
    { {TYPE04,2},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_2B_GAMEPAD},
    { {TYPE05,2},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_2B_FLIGHTYOKE},
    { {TYPE06,2},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_2B_FLIGHTYOKETHROTTLE},
    { {TYPE07,2},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_3A_2B_GENERIC},
    { {TYPE08,4},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_3A_4B_GENERIC},
    { {TYPE09,4},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_4B_GAMEPAD},
    { {TYPE10,4},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_4B_FLIGHTYOKE},
    { {TYPE11,4},JOY_US_PRESENT,{{0,0,0,0,0,0,0,0,0},{0,0,0,0}},JOY_HW_4B_FLIGHTYOKETHROTTLE},
};

/*
 * registry strings for calibration messages
 */
static LPSTR _oemCalRegStrs[] =
{
    REGSTR_VAL_JOYOEMCAL1,
    REGSTR_VAL_JOYOEMCAL2,
    REGSTR_VAL_JOYOEMCAL3,
    REGSTR_VAL_JOYOEMCAL4,
    REGSTR_VAL_JOYOEMCAL5,
    REGSTR_VAL_JOYOEMCAL6,
    REGSTR_VAL_JOYOEMCAL7,
    REGSTR_VAL_JOYOEMCAL8,
    REGSTR_VAL_JOYOEMCAL9,
    REGSTR_VAL_JOYOEMCAL10,
    REGSTR_VAL_JOYOEMCAL11,
    REGSTR_VAL_JOYOEMCAL12,
};

/*
 * base registry keys
 */
static char	szCfgKey[] = REGSTR_PATH_JOYCONFIG;
static char	szCurrCfgKey[] = REGSTR_KEY_JOYCURR;
static char	szSettingsCfgKey[] = REGSTR_KEY_JOYSETTINGS;


/*
 * freeOEMListItem - free a list of oem data
 */
static void freeOEMListItem( LPJOYDATA pjd, int i )
{
    int	j;

    DoFree( pjd->oemList[i].keyname );
    DoFree( pjd->oemList[i].ident_string );
    DoFree( pjd->oemList[i].vxd_name );
    DoFree( pjd->oemList[i].xy_label );
    DoFree( pjd->oemList[i].z_label );
    DoFree( pjd->oemList[i].r_label );
    DoFree( pjd->oemList[i].u_label );
    DoFree( pjd->oemList[i].v_label );
    DoFree( pjd->oemList[i].pov_label );
    DoFree( pjd->oemList[i].testmove_desc );
    DoFree( pjd->oemList[i].testbutton_desc );
    DoFree( pjd->oemList[i].testmove_cap );
    DoFree( pjd->oemList[i].testbutton_cap );
    DoFree( pjd->oemList[i].testwin_cap );
    DoFree( pjd->oemList[i].cal_cap );
    DoFree( pjd->oemList[i].calwin_cap );
    for( j=0;j<NUM_CAL_STRS;j++ ) {
	DoFree( pjd->oemList[i].cal_strs[j] );
    }

} /* freeOEMListItem */

/*
 * initHWDefaults - initialize the hardware list: use defaults + OEM types
 * 		    defined in the registry
 */
static void initHWDefaults( LPJOYDATA pjd )
{
    int			list_size;
    int			def_size;
    DWORD		isubkey;
    DWORD		keyidx;
    HKEY		hkey;
    HKEY		hsubkey;
    char		str[MAX_STR];
    DWORD		clsize;
    DWORD		num_subkeys;
    DWORD		dont_care;
    DWORD		longest_key;
    FILETIME		ftime;
    LPSTR		keyname;
    JOYREGHWSETTINGS	hws;
    DWORD		longest_val;
    DWORD		type;
    DWORD		cb;
    int			i;
    int			j;
    int			ctype;
    LPSTR		tmpstr;
    int			fail;

    def_size = sizeof( _joyHWDefaults )/sizeof( _joyHWDefaults[0] );
    list_size = def_size;
    pjd->oemCount = 0;
    if( !RegOpenKey( HKEY_LOCAL_MACHINE, REGSTR_PATH_JOYOEM, &hkey ) ) {
	clsize = sizeof( str );
	if( !RegQueryInfoKey ( hkey, str, &clsize, NULL, &num_subkeys,
		    &longest_key, &dont_care, &dont_care, &dont_care,
		    &dont_care,	// address of buffer for longest value data length
		    &dont_care, &ftime ) ) {
	    pjd->oemList = DoAlloc( num_subkeys * sizeof( OEMLIST ));
	    if( pjd->oemList != NULL ) {
		pjd->oemCount = num_subkeys;
		list_size += num_subkeys;
	    }
	    longest_key++;
	}
    }

    pjd->joyHWDefaults = DoAlloc( list_size * sizeof( JOYREGHWCONFIG ) );
    if( pjd->joyHWDefaults == NULL ) {
	pjd->joyHWDefaults = _joyHWDefaults;
    } else {
	memcpy( pjd->joyHWDefaults, _joyHWDefaults, def_size * sizeof( JOYREGHWCONFIG ) );
	/*
	 * if we have keys in the registry, go fetch them
	 */
	if( list_size > def_size ) {
	    isubkey = 0;
	    keyidx = 0;
	    keyname = DoAlloc( longest_key );
	    if( keyname == NULL ) {
		keyname = str;
		longest_key = sizeof( str );
	    }
	    /*
	     * run through all keys, getting the info on them
	     */
	    while( !RegEnumKey( hkey, keyidx, keyname, longest_key ) ) {
		if( !RegOpenKey( hkey, keyname, &hsubkey ) ) {
		    if( !RegQueryInfoKey ( hsubkey, str, &clsize, NULL,
				&dont_care, &dont_care, &dont_care, &dont_care,
				&dont_care, &longest_val, &dont_care, &ftime ) ) {
		    	pjd->oemList[isubkey].keyname = DoAlloc( strlen( keyname ) +1 );
			tmpstr = DoAlloc( longest_val+1 );
			if( pjd->oemList[isubkey].keyname != NULL && tmpstr != NULL ) {
			    strcpy( pjd->oemList[isubkey].keyname, keyname );
			    cb = sizeof( hws );
			    if( !RegQueryValueEx( hsubkey, REGSTR_VAL_JOYOEMDATA, NULL,
					&type, (CONST LPBYTE)&hws, &cb) ) {
				if( type == REG_BINARY && cb == sizeof( hws ) ) {
				    pjd->oemList[isubkey].hws = hws;
				}
			    }
			    fail = 0;
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMCALLOUT,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].vxd_name );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMNAME,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].ident_string );
			    for( j=0;j<NUM_CAL_STRS;j++ ) {
				fail |= regGetOEMStr( hsubkey, _oemCalRegStrs[j],
					    tmpstr, longest_val,
					    &pjd->oemList[isubkey].cal_strs[j] );
			    }
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMXYLABEL,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].xy_label );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMZLABEL,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].z_label );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMRLABEL,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].r_label );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMULABEL,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].u_label );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMVLABEL,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].v_label );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMPOVLABEL,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].pov_label );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMTESTMOVEDESC,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].testmove_desc );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMTESTBUTTONDESC,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].testbutton_desc );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMTESTMOVECAP,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].testmove_cap );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMTESTBUTTONCAP,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].testbutton_cap );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMTESTWINCAP,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].testwin_cap );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMCALCAP,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].cal_cap );
			    fail |= regGetOEMStr( hsubkey, REGSTR_VAL_JOYOEMCALWINCAP,
			    		tmpstr, longest_val,
					&pjd->oemList[isubkey].calwin_cap );
			    if( fail ) {
				freeOEMListItem( pjd, isubkey );
			    } else {
				isubkey++;
			    }
			} else {
			    DoFree( pjd->oemList[isubkey].keyname );
			}
			DoFree( tmpstr );
			RegCloseKey( hsubkey );
		    }
		}
		keyidx++;
	    }
	    pjd->oemCount = isubkey;

	    /*
	     * sort the list, and then fill in the joyHWDefault array
	     */
	    if( pjd->oemCount > 0 ) {
		for( i=0;i<pjd->oemCount;i++ ) {
		    for( j=i;j<pjd->oemCount;j++ ) {
			OEMLIST	ol;
			if( strcmp( pjd->oemList[i].ident_string,
				pjd->oemList[j].ident_string ) > 0 ) {
			    ol = pjd->oemList[i];
			    pjd->oemList[i] = pjd->oemList[j];
			    pjd->oemList[j] = ol;
			}
		    }
		}
		for( i=0;i<pjd->oemCount;i++ ) {
		    ctype = i+JOY_HW_LASTENTRY;
		    memset( &pjd->joyHWDefaults[ctype], 0,
		    			sizeof( pjd->joyHWDefaults[ctype] ) );
		    pjd->joyHWDefaults[ctype].hws = pjd->oemList[i].hws;
		    pjd->joyHWDefaults[ctype].dwUsageSettings = JOY_US_ISOEM|JOY_US_PRESENT;
		    pjd->joyHWDefaults[ctype].dwType = ctype;
		}
	    }
	    if( keyname != str ) {
		DoFree( keyname );
	    }
	}
    }

} /* initHWDefaults */

/*
 * finiHWList - finished with the hardware list, free it
 */
static void finiHWList( LPJOYDATA pjd )
{
    int	i;

    if( pjd->joyHWDefaults != NULL ) {
	if( pjd->joyHWDefaults != _joyHWDefaults ) {
	    DoFree( pjd->joyHWDefaults );
	}
	pjd->joyHWDefaults = NULL;
    }
    if( pjd->oemList != NULL ) {
	for( i=0;i<pjd->oemCount;i++ ) {
	    freeOEMListItem( pjd, i );
	}
	DoFree( pjd->oemList );
	pjd->oemList = NULL;
	pjd->oemCount = 0;
    }

} /* finiHWList */

/*
 * getRegKeys - get the registry keys we need
 */
static void getRegKeys( LPJOYDATA pjd )
{
    int		len;
    JOYCAPS jc;

    if (joyGetDevCaps (0, &jc, sizeof(jc)) != JOYERR_NOERROR)
       lstrcpy (jc.szRegKey, TEXT("joystick.dll<0000>"));

    /*
     * set up registry keys
     */
    pjd->regCfgKey = NULL;
    pjd->regCurrCfgKey = NULL;
    pjd->regSettingsCfgKey = NULL;

    len = sizeof( szCfgKey );
    pjd->regCfgKey = DoAlloc( len );
    if( pjd->regCfgKey != NULL ) {
	strcpy( pjd->regCfgKey, szCfgKey );
	pjd->regCurrCfgKey = DoAlloc( len +1 +1 +lstrlen(jc.szRegKey) +sizeof( szCurrCfgKey ) );
	if( pjd->regCurrCfgKey != NULL ) {
	    strcpy( pjd->regCurrCfgKey, pjd->regCfgKey );
	    strcat( pjd->regCurrCfgKey, "\\" );
	    strcat( pjd->regCurrCfgKey, jc.szRegKey );
	    strcat( pjd->regCurrCfgKey, "\\" );
	    strcat( pjd->regCurrCfgKey, szCurrCfgKey );
	}
	pjd->regSettingsCfgKey = DoAlloc( len +1 +1 +lstrlen(jc.szRegKey) +sizeof( szSettingsCfgKey ) );
	if( pjd->regSettingsCfgKey != NULL ) {
	    strcpy( pjd->regSettingsCfgKey, pjd->regCfgKey );
	    strcat( pjd->regSettingsCfgKey, "\\" );
	    strcat( pjd->regSettingsCfgKey, jc.szRegKey );
	    strcat( pjd->regSettingsCfgKey, "\\" );
	    strcat( pjd->regSettingsCfgKey, szSettingsCfgKey );
	}
    }

} /* getRegKeys */

/*
 * JoystickDataInit
 */
LPJOYDATA JoystickDataInit( void )
{
    LPJOYDATA	pjd;

    pjd = DoAlloc( sizeof( JOYDATA ) );
    if( pjd == NULL ) {
	return NULL;
    }

    /*
     * go set up all our defaults + oem lists
     */
    initHWDefaults( pjd );

    /*
     * get registry keys used by everyone
     */
    getRegKeys( pjd );

    /*
     * brushes for use by button display and bar display (z & r info)
     */
    pjd->hbUp = CreateSolidBrush( ACTIVE_COLOR );
    pjd->hbDown = CreateSolidBrush( INACTIVE_COLOR );

    /*
     * set up user values we like
     */
    regUserValsInit( pjd );


    #if !defined( WANT_SHEETS )
    {
	/*
	 * set up array of "global" vars (global to a joystick id)
	 */

	int		numjoys;
	int		i;

	numjoys = joyGetNumDevs();
	if( numjoys == 0 ) {
	    return NULL;
	}
    
	pjd->pgvlist = DoAlloc( sizeof( GLOBALVARS ) * numjoys );
	if( pjd->pgvlist == NULL ) {
	    return NULL;
	}
	for( i=0;i<numjoys;i++ ) {
	    pjd->pgvlist[i].iJoyId = i;
	    pjd->pgvlist[i].pjd = pjd;
	    initCurrentHW( &pjd->pgvlist[i] );
	}
    }
    #endif
    return pjd;

} /* JoystickDataInit */

/*
 * JoystickDataFini - finished with DLL wide joystick data data
 */
void JoystickDataFini( LPJOYDATA pjd )
{
    /*
     * ditch brushes
     */
    if( pjd->hbUp != NULL ) {
	DeleteObject( pjd->hbUp );
    }
    if( pjd->hbDown != NULL ) {
	DeleteObject( pjd->hbDown );
    }

    /*
     * done with hardware list
     */
    finiHWList( pjd );

    /*
     * restore user values in registry
     */
    regUserValsFini( pjd );

    /*
     * done with registry keys
     */
    DoFree( pjd->regCfgKey );
    DoFree( pjd->regCurrCfgKey );
    DoFree( pjd->regSettingsCfgKey );

    #if !defined( WANT_SHEETS )
    	DoFree( pjd->pgvlist );
    #endif

    /*
     * free up the joystick data
     */
    DoFree( pjd );
    #ifdef DEBUG
	if( allocCount != 0 ) {
	    MBOX( "Memory left unfreed: %d allocations", allocCount );
	}
    #endif

} /* JoystickDataFini */
