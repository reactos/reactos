/* Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved */

#if 	(rmm < 10)
#define rmmpad "0"
#else
#define rmmpad
#endif

#if 	(rup == 0)

#define VERSION_STR1(a,b,c) 		#a "." rmmpad #b

#else	/* !(rup == 0) */

#define VERSION_STR1(a,b,c) 		#a "." rmmpad #b "." ruppad #c

#if 	(rup < 10)
#define ruppad "000"
#elif	(rup < 100)
#define ruppad "00"
#elif	(rup < 1000)
#define ruppad "0"
#else
#define ruppad
#endif

#endif	/* !(rup == 0) */

#define VERSION_STR2(a,b,c) 		VERSION_STR1(a,b,c)
#define VER_PRODUCTVERSION_STR		VERSION_STR2(rmj,rmm,rup)
#define VER_PRODUCTVERSION			rmj,rmm,0,rup

/*--------------------------------------------------------------*/
/* the following section defines values used in the version 	*/
/* data structure for all files, and which do not change.		*/
/*--------------------------------------------------------------*/

#if defined(_SHIP)
#define VER_DEBUG					0
#else
#define VER_DEBUG					VS_FF_DEBUG
#endif

#if defined(_SHIP)
#define VER_PRIVATEBUILD			0
#else
#define VER_PRIVATEBUILD			VS_FF_PRIVATEBUILD
#endif

#if defined(_SHIP)
#define VER_PRERELEASE				0
#else
#define VER_PRERELEASE				VS_FF_PRERELEASE
#endif

#define VER_FILEFLAGSMASK			VS_FFI_FILEFLAGSMASK
#if defined(_WIN32) || defined(WIN32)
#define VER_FILEOS					VOS__WINDOWS32
#else
#define VER_FILEOS					VOS_DOS_WINDOWS16
#endif

#if defined(_DEBUG)
#define VER_FILEFLAGS				(VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)
#else
#define VER_FILEFLAGS				0
#endif

#define VER_COMPANYNAME_STR 		"Microsoft Corporation"
#define VER_PRODUCTNAME_STR		"Microsoft (R) Visual InterDev"
