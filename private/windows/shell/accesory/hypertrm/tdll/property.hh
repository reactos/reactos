/*	File: D:\WACKER\tdll\property.hh (Created: 28-Feb-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:40p $
 */

// Let's keep all of the ASCII settings in one place...
//
typedef struct STASCIISET
	{

	int fsetSendCRLF;
	int fsetLocalEcho;
	int fsetAddLF;
	int fsetASCII7;
	int fsetWrapLines;
	int iLineDelay;
	int iCharDelay;

	} STASCIISET, *pSTASCIISET;

// Local structure...
// Put in whatever else you might need to access later
//
typedef struct SDS
	{

	STEMUSET	stEmuSettings;		// Emulator settings

	STASCIISET	stAsciiSettings;	// Ascii settings

	HSESSION 	hSession;

	} SDS, *pSDS;
