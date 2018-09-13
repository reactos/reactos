//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cstrings.h
//
//---------------------------------------------------------------------------

#ifndef _CSTRINGS_H_
#define _CSTRINGS_H_

extern char const FAR c_szNull[];
extern char const FAR c_szZero[];
extern char const FAR c_szDelim[];

extern char const FAR szNullDevice[];
extern char const FAR szModemDevice[];
extern char const FAR szISDNDevice[];
extern char const FAR szUnknownDevice[];

extern char const FAR c_szDirect[];

extern char const FAR c_szDialUpServerFile[];
extern char const FAR c_szDialUpServer[];

// Menu command verbs (these are *not* displayed)

extern char const FAR c_szConnect[];
extern char const FAR c_szCreate[];
extern char const FAR c_szDelete[];
extern char const FAR c_szCut[];
extern char const FAR c_szCopy[];
extern char const FAR c_szLink[];
extern char const FAR c_szProperties[];
extern char const FAR c_szPaste[];
extern char const FAR c_szRename[];

#ifdef DEBUG

// These declarations are located in err.c
//
extern char const FAR c_szNewline[];
extern char const FAR c_szTrace[];
extern char const FAR c_szDbg[];
extern char const FAR c_szAssertFailed[];

#endif

#ifdef DEBUG

// Ini file name

extern char const FAR c_szIniFile[];

// Ini section names

extern char const FAR c_szIniSecDebugUI[];

// Ini key names

extern char const FAR c_szIniKeyTraceFlags[];
extern char const FAR c_szIniKeyDumpFlags[];
extern char const FAR c_szIniKeyBreakOnOpen[];
extern char const FAR c_szIniKeyBreakOnClose[];
extern char const FAR c_szIniKeyBreakOnRunOnce[];
extern char const FAR c_szIniKeyBreakOnValidate[];
extern char const FAR c_szIniKeyBreakOnThreadAtt[];
extern char const FAR c_szIniKeyBreakOnThreadDet[];
extern char const FAR c_szIniKeyBreakOnProcessAtt[];
extern char const FAR c_szIniKeyBreakOnProcessDet[];

#endif


#endif  // _CSTRINGS_H_
