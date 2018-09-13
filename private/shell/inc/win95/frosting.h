/*****************************************************************************
 *
 * frosting.h -  COOL.DLL functions, types, and definitions
 *
 * Copyright (c) 1993-1994, Microsoft Corp.	All rights reserved
 *
 *****************************************************************************/

#ifndef _FROSTING_H
#define _FROSTING_H

#define ordCoolWEP		    1

DWORD WINAPI CoolEnable(DWORD param);
#define ordCoolEnable		    2

void WINAPI CoolGetDosBoxTtFonts(LPSTR pszFaceSbcs, LPSTR pszFaceDbcs);
#define ordCoolGetDosBoxTtFonts	    3

#endif /* _FROSTING_H */
