//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cstrings.c
//
//  This file contains read-only string constants
//
// History:
//  12-21-93 ScottH     Created file
//
//---------------------------------------------------------------------------

#include "brfprv.h"

#pragma data_seg(DATASEG_READONLY)

TCHAR const  c_szNULL[] = TEXT("");

TCHAR const  c_szDelim[] = TEXT(" \t,");
TCHAR const  c_szAllFiles[] = TEXT("*.*");
TCHAR const  c_szEllipses[] = TEXT("...");

// Class names

// Executable and DLL names

TCHAR const  c_szEngineDLL[] = TEXT("SYNCENG.DLL");
TCHAR const  c_szCabinet[] = TEXT("Explorer.exe");
TCHAR const  c_szCabinetClass[] = TEXT("CabinetWClass");
TCHAR const  c_szWinHelpFile[] = TEXT("windows.hlp");
TCHAR const  c_szDllGetClassObject[]  = TEXT("DllGetClassObject");
TCHAR const  c_szOpen[] = TEXT("open");

// Ini file name

TCHAR const  c_szIniFile[] = TEXT("rover.ini");
TCHAR const  c_szDesktopIni[]  = STR_DESKTOPINI;
TCHAR const  c_szRunWizard[] = TEXT("RunWizard");

// Ini section names

TCHAR const  c_szIniSecExclude[] = TEXT("Exclude");
TCHAR const  c_szIniSecFilter[] = TEXT("Filter");
TCHAR const  c_szIniSecBriefcase[] = TEXT("Briefcase");

#ifdef DEBUG

TCHAR const  c_szIniSecDebugUI[] = TEXT("SyncUIDebugOptions");

#endif

// Ini key names

TCHAR const  c_szIniKeyCLSID[] = TEXT("CLSID");
TCHAR const  c_szCLSID[] = TEXT("{85BBD920-42A0-1069-A2E4-08002B30309D}");

TCHAR const  c_szIniKeyPBar[] = TEXT("ProgressBar");
TCHAR const  c_szIniKeyFile[] = TEXT("File");
TCHAR const  c_szIniKeyType[] = TEXT("Type");

#ifdef DEBUG

TCHAR const  c_szIniKeyTraceFlags[] = TEXT("TraceFlags");
TCHAR const  c_szIniKeyDumpFlags[] = TEXT("DumpFlags");
TCHAR const  c_szIniKeyBreakOnOpen[] = TEXT("BreakOnOpen");
TCHAR const  c_szIniKeyBreakOnClose[] = TEXT("BreakOnClose");
TCHAR const  c_szIniKeyBreakOnRunOnce[] = TEXT("BreakOnRunOnce");
TCHAR const  c_szIniKeyBreakOnValidate[] = TEXT("BreakOnValidate");
TCHAR const  c_szIniKeyBreakOnThreadAtt[] = TEXT("BreakOnThreadAttach");
TCHAR const  c_szIniKeyBreakOnThreadDet[] = TEXT("BreakOnThreadDetach");
TCHAR const  c_szIniKeyBreakOnProcessAtt[] = TEXT("BreakOnProcessAttach");
TCHAR const  c_szIniKeyBreakOnProcessDet[] = TEXT("BreakOnProcessDetach");

#endif

#pragma data_seg()

