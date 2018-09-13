/*
 *      !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *      !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *	!!!!!!!IF YOU CHANGE TABS TO SPACES, YOU WILL BE KILLED!!!!!!!
 *      !!!!!!!!!!!!!!DOING SO FUCKS THE BUILD PROCESS!!!!!!!!!!!!!!!!
 *      !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *      !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */


/*
 *  verinfo.h - internal header file to define the build version
 *
 */

//
//  WARNING! the following defines are used by some of the components in
//  the multimedia core. do *NOT* put LEADING ZERO's on these numbers or
//  they will end up as OCTAL numbers in the C code!
//

#ifdef MTN

#define OFFICIAL	1
#define FINAL		0

#ifdef ALTACM

#define /*ALTACM*/ MMVERSION		2
#define /*ALTACM*/ MMREVISION		0
#define /*ALTACM*/ MMRELEASE		158

#ifdef RC_INVOKED
#define VERSIONPRODUCTNAME	"Microsoft Audio Compression Manager\0"
#define VERSIONCOPYRIGHT	"Copyright \251 Microsoft Corp. 1992-1994\0"
#endif

#if defined(DEBUG_RETAIL)
#define /*ALTACM*/ VERSIONSTR	"Motown Retail Debug Version 2.00.158\0"
#elif defined(DEBUG)
#define /*ALTACM*/ VERSIONSTR	"Motown Internal Debug Version 2.00.158\0"
#else
#define /*ALTACM*/ VERSIONSTR	"2.00\0"
#endif

#elif defined(ALTVFW)

#define /*ALTVFW*/ MMVERSION		4
#define /*ALTVFW*/ MMREVISION		0
#define /*ALTVFW*/ MMRELEASE		158

#ifdef RC_INVOKED
#define VERSIONPRODUCTNAME	"Microsoft Video for Windows\0"
#define VERSIONCOPYRIGHT	"Copyright \251 Microsoft Corp. 1992-1994\0"
#endif

#if defined(DEBUG_RETAIL)
#define /*ALTVFW*/ VERSIONSTR	"Motown Retail Debug Version 4.00.158\0"
#elif defined(DEBUG)
#define /*ALTVFW*/ VERSIONSTR	"Motown Internal Debug Version 4.00.158\0"
#else
#define /*ALTVFW*/ VERSIONSTR	"4.00\0"
#endif

#else

#define /*MTN*/ MMVERSION		4
#define /*MTN*/ MMREVISION		0
#define /*MTN*/ MMRELEASE		158

#ifdef RC_INVOKED
#define VERSIONPRODUCTNAME	"Microsoft Windows\0"
#define VERSIONCOPYRIGHT	"Copyright \251 Microsoft Corp. 1991-1994\0"
#endif

#if defined(DEBUG_RETAIL)
#define /*MTN*/ VERSIONSTR	"Motown Retail Debug Version 4.00.158\0"
#elif defined(DEBUG)
#define /*MTN*/ VERSIONSTR	"Motown Internal Debug Version 4.00.158\0"
#else
#define /*MTN*/ VERSIONSTR	"4.00\0"
#endif

#endif

#elif defined(ACM)

#define OFFICIAL	1
#define FINAL		0

#define /*ACM*/ MMVERSION		3
#define /*ACM*/ MMREVISION		50
#define /*ACM*/ MMRELEASE		612

#ifdef RC_INVOKED
#define VERSIONPRODUCTNAME	"Microsoft Audio Compression Manager\0"
#define VERSIONCOPYRIGHT	"Copyright \251 Microsoft Corp. 1992-1994\0"
#endif

#if defined(DEBUG_RETAIL)
#define /*ACM*/ VERSIONSTR	"ACM Retail Debug Version 3.50.612\0"
#elif defined(DEBUG)
#define /*ACM*/ VERSIONSTR	"ACM Internal Debug Version 3.50.612\0"
#else
#define /*ACM*/ VERSIONSTR	"3.50\0"
#endif

#elif defined(VFW)

#define OFFICIAL	1
#define FINAL		0

#define /*VFW*/ MMVERSION		1
#define /*VFW*/ MMREVISION		15
#define /*VFW*/ MMRELEASE		1

#ifdef RC_INVOKED
#define VERSIONPRODUCTNAME	"Microsoft Video for Windows\0"
#define VERSIONCOPYRIGHT	"Copyright \251 Microsoft Corp. 1992-1994\0"
#endif

#if defined(DEBUG_RETAIL)
#define /*VFW*/ VERSIONSTR	"VfW Retail Debug Version 1.15.1\0"
#elif defined(DEBUG)
#define /*VFW*/ VERSIONSTR	"VfW Internal Debug Version 1.15.1\0"
#else
#define /*VFW*/ VERSIONSTR	"1.15\0"
#endif

#endif

/***************************************************************************
 *  DO NOT TOUCH BELOW THIS LINE                                           *
 ***************************************************************************/

#ifdef RC_INVOKED
#define VERSIONCOMPANYNAME	"Microsoft Corporation\0"

/*
 *  Version flags
 */

#ifndef OFFICIAL
#define VER_PRIVATEBUILD	VS_FF_PRIVATEBUILD
#else
#define VER_PRIVATEBUILD	0
#endif

#ifndef FINAL
#define VER_PRERELEASE		VS_FF_PRERELEASE
#else
#define VER_PRERELEASE		0
#endif

#if defined(DEBUG_RETAIL)
#define VER_DEBUG		VS_FF_DEBUG
#elif defined(DEBUG)
#define VER_DEBUG		VS_FF_DEBUG
#else
#define VER_DEBUG		0
#endif

#define VERSIONFLAGS		(VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)
#define VERSIONFILEFLAGSMASK	0x0030003FL

#endif
