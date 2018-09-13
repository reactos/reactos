//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strings.h
//
//--------------------------------------------------------------------------

#ifndef _STRINGS_H_
#define _STRINGS_H_

extern const TCHAR c_szStar[];               
extern const TCHAR c_szCSCKey[];             
extern const TCHAR c_szCSCShareKey[];        
extern const TCHAR c_szSyncMutex[];         
extern const TCHAR c_szSyncInProgMutex[];
extern const TCHAR c_szPurgeInProgMutex[];
extern const TCHAR c_szPolicy[];
extern const TCHAR c_szCFDataSrcClsid[];

extern const TCHAR c_szHiddenWindowClassName[];
extern const TCHAR c_szHiddenWindowTitle[];  

extern const TCHAR c_szDllName[];            

extern const TCHAR c_szRegKeyAPF[];
extern const TCHAR c_szEntryID[];
extern const TCHAR c_szLastSync[];
extern const TCHAR c_szPurgeAtNextLogoff[];

extern const TCHAR c_szLNK[];              

extern const char  c_szHtmlHelpFile[];
extern const char  c_szHtmlHelpTopic[];
extern const TCHAR c_szHelpFile[];

extern const TCHAR c_szSyncMgrInitialized[];
extern const TCHAR c_szConfirmDelShown[];

//
// These need to be macros.
//
#define STR_SYNC_VERB   "synchronize"
#define STR_PIN_VERB    "pin"
#define STR_UNPIN_VERB  "unpin"
#define STR_DELETE_VERB "delete"

#endif _STRINGS_H_
