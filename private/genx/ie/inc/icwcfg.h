/********************************************************************

  ICWCFG.H

  Copyright(c) Microsoft Corporation, 1996-1998

  *** N O T   F O R   E X T E R N A L   R E L E A S E *******
  *
  * This header file is not intended for distribution outside Microsoft.
  *
  ***********************************************************

  Header file for Internet Connection Wizard external configuration
  routines found in INETCFG.DLL.

  Routines:

  CheckConnectionWizard - Checks which parts of ICW are installed
		and if it has been run before.  It optionally will start
		either the full or manual path of ICW if it is insalled
		but has not been run before.

  History:	10/22/96	Created
		10/24/96	Added defines and typedefs
		2/25/97		Added CreateDirectoryService -- jmazner
		4/24/97		Removed InetCreate*, these are now owned
					by the Account Manager -- jmazner

  Support:	This header file (and INETCFG.DLL) is supported by the
			Internet Connection Wizard team (alias icwcore).  Please
			do not modify this directly.

*********************************************************************/

#ifndef _ICWCFG_H_

//
// defines
//

// ICW registry settings

// HKEY_CURRENT_USER
#define ICW_REGPATHSETTINGS	"Software\\Microsoft\\Internet Connection Wizard"
#define ICW_REGKEYCOMPLETED	"Completed"

// Maximum field lengths
#define ICW_MAX_ACCTNAME	256
#define ICW_MAX_PASSWORD	256	// PWLEN
#define ICW_MAX_LOGONNAME	256	// UNLEN
#define ICW_MAX_SERVERNAME	64
#define ICW_MAX_RASNAME		256	// RAS_MaxEntryName
#define ICW_MAX_EMAILNAME	64
#define ICW_MAX_EMAILADDR	128

// Bit-mapped flags

// CheckConnectionWizard input flags
#define ICW_CHECKSTATUS		0x0001

#define ICW_LAUNCHFULL		0x0100
#define ICW_LAUNCHMANUAL	0x0200
#define ICW_USE_SHELLNEXT	0x0400
#define ICW_FULL_SMARTSTART	0x0800

// CheckConnectionWizard output flags
#define ICW_FULLPRESENT		0x0001
#define ICW_MANUALPRESENT	0x0002
#define ICW_ALREADYRUN		0x0004

#define ICW_LAUNCHEDFULL	0x0100
#define ICW_LAUNCHEDMANUAL	0x0200

// InetCreateMailNewsAccount input flags
#define ICW_USEDEFAULTS		0x0001

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


//
// external function typedefs
//
typedef DWORD	(WINAPI *PFNCHECKCONNECTIONWIZARD) (DWORD, LPDWORD);
typedef DWORD	(WINAPI *PFNSETSHELLNEXT) (CHAR *);

//
// external function declarations
//
DWORD	WINAPI CheckConnectionWizard(DWORD, LPDWORD);
DWORD	WINAPI SetShellNext(CHAR *);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _ICWCFG_H_