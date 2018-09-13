/* xfr_todo.c -- a file used to handle unwriten needs
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#pragma hdrstop

#define BYTE	char

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\com.h>
#include "xfr_todo.h"

/*
 * This function is here to provide stubs for functions that have not yet
 * been ported over to WACKER.  By the time WACKER is functional, this file
 * should be empty.
 */

/* Replace the old CNFG structure */

int cnfgBitRate()
	{
	/*
	 * TODO: decide if we actually need this kind of stuff or can we skip it ?
	 */
	return 9600;
	}

int cnfgBitsPerChar(HSESSION h)
	{
	HCOM hC;
	int nRet = 7;
	/*
	 * TODO: decide if we actually need this kind of stuff or can we skip it ?
	 */
	// return 7;
	hC = sessQueryComHdl(h);

	if (hC)
		ComGetDataBits(hC, &nRet);

	return nRet;
	}
