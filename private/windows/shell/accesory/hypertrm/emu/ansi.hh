/*	File: D:\WACKER\emu\ansi.hh (Created: 21-July-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

// Private emulator data for ANSI.
//
typedef struct stPrivateANSI
	{
	int iSavedRow,
		iSavedColumn;

	} ANSIPRIVATE;

typedef ANSIPRIVATE *PSTANSIPRIVATE;
