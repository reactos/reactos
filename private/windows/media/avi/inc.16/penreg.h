/* PENREG.h: Pen Windows Registry defines

	Copyright 1993-1994 Microsoft Corporation.  All rights reserved.
	Microsoft Confidential.

		Registry defines for Pen Windows et al.
		This file should not be localized!
	
*/

#ifndef _INCLUDE_PENREGH
#define _INCLUDE_PENREGH


/****************** Includes ***********************************************/
#ifndef WIN32
#include "\dev\sdk\inc\regstr.h"
#endif //!WIN32
#ifndef WINPAD
#include <winerror.h>
#endif
/****************** Defines ************************************************/
#define keyUndef			0x0000	// Undefined key
#define keyPenHlmCv		0x0001	// Sys stuff not editable by pen cpl
#define keyPenHlmCpl		0x0002	// Sys stuff editable by pen cpl
#define keyPenHlmBedit	0x0003	// Bedit stuff (editable by cpl for Kanji)
#define keyPenHcuCpl		0x0004	// Per-user stuff editable by pen cpl

#if defined(JAPAN) && defined(DBCS_IME)
#define keyPenHlmIme		0x0005	// Ime stuff not editable by pen cpl
#endif

// Parent Key: HKEY_LOCAL_MACHINE
#define REGSTR_PATH_CONTROL\
	"System\\CurrentControlSet\\Control"
#define REGSTR_PATH_PENHLMCV\
	"Software\\Microsoft\\Windows\\CurrentVersion\\Pen"
#define REGSTR_PATH_PENHLMCPL\
	"Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Pen"
#define REGSTR_PATH_PENHLMBEDIT\
	"Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Pen\\BEdit"

#if defined(JAPAN) && defined(DBCS_IME)
#define REGSTR_PATH_PENHLMIME\
	"Software\\Microsoft\\Windows\\CurrentVersion\\Pen\\Ime"
#endif

// Parent key: HKEY_CURRENT_USER
#define REGSTR_PATH_PENHCUCPL					"Control Panel\\Pen"

#define REGSTR_VAL_CURRENTUSER				"Current User"

#define REGSTR_VAL_PENBEDIT_BASEHEIGHT		"BaseHeight"
#define REGSTR_VAL_PENBEDIT_BASEHORZ		"BaseHorz"
#define REGSTR_VAL_PENBEDIT_CELLHEIGHT		"CellHeight"
#define REGSTR_VAL_PENBEDIT_CELLWIDTH		"CellWidth"
#define REGSTR_VAL_PENBEDIT_CUSPHEIGHT		"CuspHeight"
#define REGSTR_VAL_PENBEDIT_ENDCUSPHEIGHT	"EndCuspHeight"
#define REGSTR_VAL_PENBEDIT_GUIDECROSS		"GuideCross"
#define REGSTR_VAL_PENBEDIT_GUIDESTYLE		"GuideStyle"

// Per user items which cannot be modified from pencp.cpl.
#define REGSTR_VAL_PEN_BARRELEVENT			"BarrelEvent"
#define REGSTR_VAL_PEN_LENS					"Lens"
#define REGSTR_VAL_PEN_RECOG 					"Recognizer"
#define REGSTR_VAL_PEN_SELECTTIMEOUT		"SelectTimeOut"
#define REGSTR_VAL_PEN_USER					"User"				// May go away

// Per user items which can be modified from pencp.cpl.
#define REGSTR_VAL_PENCPL_ACTIONHANDLES	"ActionHandles"	// May go away
#define REGSTR_VAL_PENCPL_AUTOWRITE			"AutoWrite"
#define REGSTR_VAL_PENCPL_INPUTCURSOR		"InputCursor"
#define REGSTR_VAL_PENCPL_INKCOLOR			"InkColor"
#define REGSTR_VAL_PENCPL_INKWIDTH			"InkWidth"
#define REGSTR_VAL_PENCPL_INTLPREF			"IntlPreferences"
#define REGSTR_VAL_PENCPL_MENU				"MenuDropAlignment"
#define REGSTR_VAL_PENCPL_PREF				"Preferences"
#define REGSTR_VAL_PENCPL_TIMEOUT			"TimeOut"


#endif 	// _INCLUDE_PENREGH

