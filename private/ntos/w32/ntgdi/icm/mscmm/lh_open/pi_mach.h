/*
	File:		PI_Mach.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef PI_Machine_h
#define PI_Machine_h

#if defined(unix) || defined(__unix) || defined(__unix__)
#define __IS_UNIX 1
#else
#define __IS_UNIX 0
#endif

#ifdef __MSDOS__
#define __IS_MSDOS 1
#else
#define __IS_MSDOS 0
#endif

#ifdef __MWERKS__
#define __IS_MAC 1
#else
#define __IS_MAC 0
#endif

#if __IS_MSDOS 
typedef long int off_t;
#endif

#endif	

