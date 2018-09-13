/*
	File:		PI_TickCount.c

	Contains:	
				
	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef PI_BasicTypes_h
#include "PI_Basic.h"
#endif

#ifndef PI_Machine_h
#include "PI_Mach.h"
#endif

#ifndef PI_Memory_h
#include "PI_Mem.h"
#endif

#include <time.h>
#if __IS_MSDOS
#include <wtypes.h>
#endif

double MyTickCount(void);
double MyTickCount(void)
{
	double timevalue;
#if __IS_MAC
	timevalue = TickCount()/60.;
#elif __IS_MSDOS
	timevalue = GetTickCount()/1000.;
#else
	timevalue = clock()/(CLOCKS_PER_SEC*1000.);
#endif
	return timevalue;
}

#if __IS_MSDOS
UINT32 TickCount(void)
{
	UINT32 timevalue;
	timevalue = (UINT32)(GetTickCount()/1000.*60 + .5 );
	return timevalue;
}
#endif

#if !__IS_MSDOS
#if !__IS_MAC
UINT32 TickCount(void)
{
	UINT32 timevalue;
	timevalue = (UINT32) time(NULL);
	return timevalue;
}
#endif
#endif
