//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strings.cpp
//
//--------------------------------------------------------------------------

#include <pch.h>
const TCHAR c_szStar[]                  = TEXT("*");
const TCHAR c_szCSCKey[]                = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\NetCache");
const TCHAR c_szCSCShareKey[]           = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\NetCache\\Shares");
const TCHAR c_szSyncMutex[]             = TEXT("CscUpdate_SyncMutex");
const TCHAR c_szSyncInProgMutex[]       = TEXT("CscUpdate_SyncInProgMutex");
const TCHAR c_szPurgeInProgMutex[]      = TEXT("CscCache_PurgeInProgMutex");
const TCHAR c_szPolicy[]                = TEXT("Policy");
const CHAR  c_szCmVerbSync[]            = "synchronize";
const CHAR  c_szCmVerbPin[]             = "pin";
const TCHAR c_szCFDataSrcClsid[]        = TEXT("Data Source CLSID");
const TCHAR c_szPurgeAtNextLogoff[]     = TEXT("PurgeAtNextLogoff");

const TCHAR c_szHiddenWindowClassName[] = STR_CSCHIDDENWND_CLASSNAME;
const TCHAR c_szHiddenWindowTitle[]     = STR_CSCHIDDENWND_TITLE;

const TCHAR c_szDllName[]               = TEXT("cscui.dll");

const TCHAR c_szRegKeyAPF[]             = TEXT("Software\\Policies\\Microsoft\\Windows\\NetCache\\AssignedOfflineFolders");
const TCHAR c_szEntryID[]               = TEXT("ID");
const TCHAR c_szLastSync[]              = TEXT("LastSyncTime");

const TCHAR c_szLNK[]                   = TEXT(".lnk");

const TCHAR c_szSyncMgrInitialized[]    = TEXT("SyncMgrInitialized");
const TCHAR c_szConfirmDelShown[]       = TEXT("ConfirmDelShown");
//
// BUGBUG: Review these once help content is available [brianau 4/22/98]
//
const char c_szHtmlHelpFile[]           = "OFFLINEFOLDERS.CHM > windefault";
const char c_szHtmlHelpTopic[]          = "csc_overview.htm";
const TCHAR c_szHelpFile[]              = TEXT("CSCUI.HLP");
