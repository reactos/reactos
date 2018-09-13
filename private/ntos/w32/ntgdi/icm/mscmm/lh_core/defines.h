/*
	File:		LHDefines.h

	Contains:	defines for the CMM

	Written by:	Werner Neubrand

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHDefines_h
#define LHDefines_h

#ifndef LUTS_ARE_PTR_BASED
#define LUTS_ARE_PTR_BASED 0
#else
#define LUTS_ARE_PTR_BASED 1
#endif

/* made a few changes to get it to compile with MrC and SC. */
#if LUTS_ARE_PTR_BASED
	#define LUT_DATA_TYPE		void*
	#define CUBE_DATA_TYPE		void*
	#define DATA_2_PTR
	#define GETDATASIZE(x)		GetPtrSize(x)
	#define SETDATASIZE(x,y)	SetPtrSize(x,y)
	#define LOCK_DATA(x)
	#define UNLOCK_DATA(x)
	#define ALLOC_DATA(x,y)		SmartNewPtr(x,y)
	#define DISPOSE_DATA(x)		DisposePtr((Ptr)(x))
	#define DISPOSE_IF_DATA(x)	DisposeIfPtr((Ptr)(x))
#else
	#define LUT_DATA_TYPE		void**
	#define CUBE_DATA_TYPE		void**
	#define DATA_2_PTR *
	#define GETDATASIZE(x)		GetHandleSize((Handle)(x))
	#define SETDATASIZE(x,y)	SetHandleSize((Handle)(x),(y))
	#define LOCK_DATA(x)		HLock((Handle)(x))
	#define UNLOCK_DATA(x)		HUnlock((Handle)(x))
	#define ALLOC_DATA(x,y)		(void **)SmartNewHandle(x,y)
	#define DISPOSE_DATA(x)		(void **)DisposeHandle((Handle)(x))
	#define DISPOSE_IF_DATA(x)	(void **)DisposeIfHandle((Handle)(x))
#endif


#define		kDoDefaultLut	0
#define		kDoGamutLut		1

#define		kNoInfo			0
#define		kDoXYZ2Lab		1
#define		kDoLab2XYZ		2

#define		kNumOfRGBchannels 3
#define		kNumOfLab_XYZchannels 3

/*							these constants are the defaults for Do3D and hardware			*/
/*- BYTE ---------------------------------------------------------------------------------- */
#define     adr_bereich_elut       256	/* ElutAdrSize   |  Elut pixeloriented 256 entries 10 bit each dim.	*/
#define     adr_breite_elut          8	/* ElutAdrShift  |  2^8 = 256										*/
#define     bit_breite_elut         10  /* ElutWordSize														*/


#define     adr_bereich_alut      1024	/* AlutAdrSize   |  Alut pixeloriented 1024 entries 8 bit each dim.	*/
#define     adr_breite_alut         10	/* AlutAdrShift  |  2^10 = 1024										*/
#define     bit_breite_alut          8
		
/* constants for the profheader-flags *
#define		kQualityMask		0x00030000
#define		kLookupOnlyMask		0x00040000
#define		kCreateGamutLutMask	0x00080000
#define		kUseRelColorimetric	0x00100000*/
							
#endif
