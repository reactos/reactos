/****************************Module*Header******************************\
* Module Name: GLOBALS.H
*
* Module Descripton: Header file listing all global variables
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

#ifdef DBG
extern DWORD     gdwDebugControl;
#endif

extern PCMMOBJ   gpCMMChain;
extern PCMMOBJ   gpPreferredCMM;

extern char     *gszCMMReqFns[];
extern char     *gszCMMOptFns[];
extern char     *gszPSFns[];

extern TCHAR     gszICMatcher[];
extern TCHAR     gszICMRegPath[];

#if !defined(_WIN95_)
extern TCHAR     gszMonitorGUID[];
extern TCHAR     gszDeviceClass[];
#else
extern TCHAR     gszSetupPath[];
extern TCHAR     gszRegPrinter[];
extern TCHAR     gszICMDir[];
extern TCHAR     gszPrinterData[];
#endif

extern TCHAR     gszPrinter[];
extern TCHAR     gszMonitor[];
extern TCHAR     gszScanner[];
extern TCHAR     gszLink[];
extern TCHAR     gszAbstract[];
extern TCHAR     gszDefault[];
extern TCHAR     gszFriendlyName[];
extern TCHAR     gszDeviceName[];
extern TCHAR     gszDisplay[];

extern TCHAR     gszDefaultCMM[];

extern CRITICAL_SECTION critsec;

extern TCHAR     gszColorDir[];
extern TCHAR     gszBackslash[];

extern TCHAR     gszRegisteredProfiles[];
extern TCHAR     gszsRGBProfile[];

extern TCHAR     gszICMProfileListValue[];
extern TCHAR     gszICMProfileListKey[];
extern TCHAR     gszFiles[];
extern TCHAR     gszDirectory[];
extern TCHAR     gszModule[];
extern TCHAR     gszMSCMS[];
extern TCHAR     gszICMDeviceDataKey[];
extern TCHAR     gszICMProfileEnumMode[];

extern TCHAR     gszStiDll[];
extern char      gszStiCreateInstance[];

