/*
 *  mdkver.h - internal header file to define the build version for post-1.0
 *	MDK.
 */

/* 
 *  All strings MUST have an explicit \0  
 *
 *  MMSYSRELEASE should be changed every build
 *
 *  Version string should be changed each build
 *
 *  Remove build extension on final release
 */

#include "vernum.h"

#define MMSYSVERSION            01
#define MMSYSREVISION           01
#define MMSYSRELEASE            rup

#if defined(DEBUG_RETAIL)
#define MMSYSVERSIONSTR     "MDK Debug Version 1.0c\0"
#elif defined(DEBUG)
#define MMSYSVERSIONSTR     "Internal Debug Version 1.0c\0"
#else 
#define MMSYSVERSIONSTR     "Version 1.0c\0"
#endif

#define OFFICIAL
#define FINAL

/***************************************************************************
 *  DO NOT TOUCH BELOW THIS LINE                                           *
 ***************************************************************************/

#ifdef RC_INVOKED

#define MMVERSIONCOMPANYNAME    "Microsoft Corporation\0"
#define MMVERSIONPRODUCTNAME    "Microsoft Windows\0"
#define MMVERSIONCOPYRIGHT      "Copyright \251 Microsoft Corp. 1991-1992\0"

/*
 *  Version flags 
 */

#ifndef OFFICIAL
#define MMVER_PRIVATEBUILD      VS_FF_PRIVATEBUILD
#else
#define MMVER_PRIVATEBUILD      0
#endif

#ifndef FINAL
#define MMVER_PRERELEASE        VS_FF_PRERELEASE
#else
#define MMVER_PRERELEASE        0
#endif

#if defined(DEBUG_RETAIL)
#define MMVER_DEBUG             VS_FF_DEBUG    
#elif defined(DEBUG)
#define MMVER_DEBUG             VS_FF_DEBUG    
#else
#define MMVER_DEBUG             0
#endif

#define MMVERSIONFLAGS          (MMVER_PRIVATEBUILD|MMVER_PRERELEASE|MMVER_DEBUG)
#define MMVERSIONFILEFLAGSMASK  0x0000003FL


VS_VERSION_INFO VERSIONINFO
FILEVERSION MMSYSVERSION,MMSYSREVISION, 0, MMSYSRELEASE
PRODUCTVERSION MMSYSVERSION,MMSYSREVISION, 0, MMSYSRELEASE
FILEFLAGSMASK MMVERSIONFILEFLAGSMASK
FILEFLAGS MMVERSIONFLAGS
FILEOS VOS_DOS_WINDOWS16
FILETYPE MMVERSIONTYPE
FILESUBTYPE MMVERSIONSUBTYPE
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
	BLOCK "040904E4"
	BEGIN
	    VALUE "CompanyName", MMVERSIONCOMPANYNAME
	    VALUE "FileDescription", MMVERSIONDESCRIPTION
            VALUE "FileVersion",  MMSYSVERSIONSTR
	    VALUE "InternalName", MMVERSIONNAME
	    VALUE "LegalCopyright", MMVERSIONCOPYRIGHT
            VALUE "OriginalFilename", MMVERSIONNAME
	    VALUE "ProductName", MMVERSIONPRODUCTNAME
	    VALUE "ProductVersion", MMSYSVERSIONSTR
	END

#ifdef INTL
        BLOCK "040904E4"
        BEGIN
            VALUE "CompanyName", MMVERSIONCOMPANYNAME
            VALUE "FileDescription", MMVERSIONDESCRIPTION
            VALUE "FileVersion", MMSYSVERSIONSTR      
            VALUE "InternalName", MMVERSIONNAME
            VALUE "LegalCopyright", MMVERSIONCOPYRIGHT
            VALUE "OriginalFilename", MMVERSIONNAME
            VALUE "ProductName", MMVERSIONPRODUCTNAME
            VALUE "ProductVersion", MMSYSVERSIONSTR
        END
#endif

    END

    BLOCK "VarFileInfo"
    BEGIN
        /* the following line should be extended for localized versions */
	VALUE "Translation", 0x409, 1252
    END

END

#endif
