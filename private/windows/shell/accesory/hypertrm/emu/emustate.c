/*	File: D:\WACKER\emu\vidstate.c (Created: 08-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

#include <windows.h>

#include <tdll\stdtyp.h>
#include "emu.h"
#include "emu.hh"

int iCurAttrState;

STATTR attrState[2] =
	{
	{VC_WHITE, VC_BLACK},	// CS_STATE
	{VC_WHITE, VC_BLACK}	// CSCLEAR_STATE
	};
