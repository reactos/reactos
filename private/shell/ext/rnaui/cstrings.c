//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cstrings.c
//
//  This file contains read-only string constants
//
// History:
//  01-17-94 ScottH     Copied from mdmui
//
//---------------------------------------------------------------------------

#include "rnaui.h"

#pragma data_seg(DATASEG_READONLY)

char const FAR c_szNull[] = "";
char const FAR c_szZero[] = "0";
char const FAR c_szDelim[] = " \t,";

char const FAR szNullDevice[]  = DT_NULLMODEM;
char const FAR szModemDevice[] = DT_MODEM;
char const FAR szISDNDevice[]  = DT_ISDN;
char const FAR szUnknownDevice[] = DT_UNKNOWN;

char const FAR c_szDirect[]    = DIRECTCC;

char const FAR c_szDialUpServerFile[] = {"\\" RNA_SERVER_MOD_NAME};
char const FAR c_szDialUpServer[]     = {RNA_SERVER_MOD_NAME","CALLER_ACCESS_FUNC_NAME};

// Menu command verbs (these are *not* displayed)

char const FAR c_szConnect[] = "connect";
char const FAR c_szCreate[] = "create";
char const FAR c_szDelete[] = "delete";
char const FAR c_szCut[] = "cut";
char const FAR c_szCopy[] = "copy";
char const FAR c_szLink[] = "link";
char const FAR c_szProperties[] = "properties";
char const FAR c_szPaste[] = "paste";
char const FAR c_szRename[] = "rename";

#ifdef DEBUG

// Ini file name

char const FAR c_szIniFile[] = "rover.ini";

// Ini section names

char const FAR c_szIniSecDebugUI[] = "RNAUIDebugOptions";

// Ini key names

char const FAR c_szIniKeyTraceFlags[] = "TraceFlags";
char const FAR c_szIniKeyDumpFlags[] = "DumpFlags";
char const FAR c_szIniKeyBreakOnOpen[] = "BreakOnOpen";
char const FAR c_szIniKeyBreakOnClose[] = "BreakOnClose";
char const FAR c_szIniKeyBreakOnRunOnce[] = "BreakOnRunOnce";
char const FAR c_szIniKeyBreakOnValidate[] = "BreakOnValidate";
char const FAR c_szIniKeyBreakOnThreadAtt[] = "BreakOnThreadAttach";
char const FAR c_szIniKeyBreakOnThreadDet[] = "BreakOnThreadDetach";
char const FAR c_szIniKeyBreakOnProcessAtt[] = "BreakOnProcessAttach";
char const FAR c_szIniKeyBreakOnProcessDet[] = "BreakOnProcessDetach";

#endif

#pragma data_seg()
