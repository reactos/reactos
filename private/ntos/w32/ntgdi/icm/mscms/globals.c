/****************************Module*Header******************************\
* Module Name: GLOBALS.C
*
* Module Descripton: This file contains all the global variables
*
* Warnings:
*
* Issues:
*
* Public Routines:
*
* Created:  6 May 1996
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "mscms.h"


#ifdef DBG

//
// Global variable used for debugging purposes
//

DWORD gdwDebugControl = DBG_LEVEL_WARNING;

#endif

//
// These are for loading & unloading CMMs and maintaining the CMM objects
// in a chain in memory
//

PCMMOBJ  gpCMMChain     = NULL; // the list of used CMM by application
PCMMOBJ  gpPreferredCMM = NULL; // application specified preferred CMM

char    *gszCMMReqFns[] = {
    "CMGetInfo",
   #ifdef UNICODE
    "CMCreateTransformW",
    "CMCreateTransformExtW",
   #else
    "CMCreateTransform",
    "CMCreateTransformExt",
   #endif
    "CMDeleteTransform",
    "CMTranslateRGBs",
    "CMTranslateRGBsExt",
    "CMCheckRGBs",
    "CMCreateMultiProfileTransform",
    "CMTranslateColors",
    "CMCheckColors"
    };

char    *gszCMMOptFns[] = {
   #ifdef UNICODE
    "CMCreateProfileW",
   #else
    "CMCreateProfile",
   #endif
    "CMGetNamedProfileInfo",
    "CMConvertColorNameToIndex",
    "CMConvertIndexToColorName",
    "CMCreateDeviceLinkProfile",
    "CMIsProfileValid"
    };

char     *gszPSFns[] = {
    "CMGetPS2ColorSpaceArray",
    "CMGetPS2ColorRenderingIntent",
    "CMGetPS2ColorRenderingDictionary"
    };

//
// These are for registry paths
//

#if !defined(_WIN95_)
TCHAR  gszMonitorGUID[]    = __TEXT("{4D36E96E-E325-11CE-BFC1-08002BE10318}");
TCHAR  gszDeviceClass[]    = __TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\");
TCHAR  gszICMatcher[]      = __TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ICMatchers");
TCHAR  gszICMRegPath[]     = __TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ICM");
#else
TCHAR  gszICMatcher[]      = __TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ICM\\ICMatchers");
TCHAR  gszICMRegPath[]     = __TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ICM");
TCHAR  gszSetupPath[]      = __TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup");
TCHAR  gszRegPrinter[]     = __TEXT("System\\CurrentControlSet\\Control\\Print\\Printers");
TCHAR  gszICMDir[]         = __TEXT("ICMPath");
TCHAR  gszPrinterData[]    = __TEXT("PrinterDriverData");
#endif

TCHAR  gszPrinter[]        = __TEXT("prtr");
TCHAR  gszMonitor[]        = __TEXT("mntr");
TCHAR  gszScanner[]        = __TEXT("scnr");
TCHAR  gszLink[]           = __TEXT("link");
TCHAR  gszAbstract[]       = __TEXT("abst");
TCHAR  gszDefault[]        = __TEXT("default");
TCHAR  gszFriendlyName[]   = __TEXT("FriendlyName");
TCHAR  gszDeviceName[]     = __TEXT("DriverDesc");
TCHAR  gszDisplay[]        = __TEXT("DISPLAY");

//
// Default CMM dll
//

TCHAR  gszDefaultCMM[] = __TEXT("icm32.dll");

//
// Synchronization objects
//

CRITICAL_SECTION   critsec;

//
// Miscellaneous
//

TCHAR  gszColorDir[]     = __TEXT("COLOR");
TCHAR  gszBackslash[]    = __TEXT("\\");

//
// Wellknown profile support
//

TCHAR  gszRegisteredProfiles[]  = __TEXT("RegisteredProfiles");
TCHAR  gszsRGBProfile[]         = __TEXT("sRGB Color Space Profile.icm");

TCHAR  gszICMProfileListKey[]   = __TEXT("CopyFiles\\ICM");
TCHAR  gszICMProfileListValue[] = __TEXT("ICMProfile");

TCHAR  gszFiles[]               = __TEXT("Files");
TCHAR  gszDirectory[]           = __TEXT("Directory");
TCHAR  gszModule[]              = __TEXT("Module");
TCHAR  gszMSCMS[]               = __TEXT("mscms.dll");

TCHAR  gszICMDeviceDataKey[]    = __TEXT("ICMData");
TCHAR  gszICMProfileEnumMode[]  = __TEXT("ProfileEnumMode");

//
// Scanner support
//

TCHAR  gszStiDll[]             = __TEXT("sti.dll");
char   gszStiCreateInstance[]  = "StiCreateInstance";

