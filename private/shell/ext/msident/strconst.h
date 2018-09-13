/*----------------------------------------------------------------------------
    strconst.h
        Non-localizable String constant definitions

 ----------------------------------------------------------------------------*/
#ifndef _STRCONST_H
#define _STRCONST_H


#ifndef WIN16
#ifdef DEFINE_STRING_CONSTANTS
#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[] = TEXT(y)
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[] = y
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[] = L##y
#else
#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[]
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[]
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[]
#endif
#else // !WIN16
#ifdef DEFINE_STRING_CONSTANTS
#ifdef __WATCOMC__
#define STR_GLOBAL(x,y)         extern "C" const TCHAR CDECL x[] = TEXT(y)
#define STR_GLOBAL_ANSI(x,y)    extern "C" const char CDECL x[] = y
#define STR_GLOBAL_WIDE(x,y)    extern "C" const WCHAR CDECL x[] = y
#else  // __WATCOMC__
#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[] = TEXT(y)
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[] = y
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[] = L##y
#endif // __WATCOMC__
#else
#ifdef __WATCOMC__
#define STR_GLOBAL(x,y)         extern "C" const TCHAR CDECL x[]
#define STR_GLOBAL_ANSI(x,y)    extern "C" const char CDECL x[]
#define STR_GLOBAL_WIDE(x,y)    extern "C" const WCHAR CDECL x[]
#else  // __WATCOMC__
#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[]
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[]
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[]
#endif // __WATCOMC__
#endif

#endif // !WIN16

#define STR_REG_PATH_ROOT           "Identities"

// --------------------------------------------------------------------------
// MultiUser
// --------------------------------------------------------------------------
STR_GLOBAL(c_szRegRoot,             STR_REG_PATH_ROOT);
STR_GLOBAL(c_szUserDirPath,         "Application Data\\Identities\\");
STR_GLOBAL(c_szUsername,            "Username");
STR_GLOBAL(c_szUserID,              "User ID");
STR_GLOBAL(c_szDirName,             "Directory Name");
STR_GLOBAL(c_szUsePassword,         "Use Password");
STR_GLOBAL(c_szPassword,            "Password");
STR_GLOBAL(c_szLastUserID,          "Last User ID");
STR_GLOBAL(c_szLastUserName,        "Last Username");
STR_GLOBAL(c_szDefaultUserID,       "Default User ID");
STR_GLOBAL(c_szDefaultUserName,     "Default Username");
STR_GLOBAL(c_szPolicyKey,           "Locked Down");
STR_GLOBAL(c_szLoginAs,             "Start As");
STR_GLOBAL(c_szRegFolders,          "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
STR_GLOBAL(c_szValueAppData,        "AppData");
STR_GLOBAL(c_szNotifyWindowClass,   "Identity Mgr Notify");
STR_GLOBAL(c_szCtxHelpFile,         "ident.hlp");
STR_GLOBAL(c_szIdentitiesFolderName,"Identities");
STR_GLOBAL(c_szEnableDCPolicyKey,   "DCPresent Enable");
STR_GLOBAL(c_szMigrated5,           "Migrated5");
STR_GLOBAL(c_szOutgoingID,          "OutgoingID");
STR_GLOBAL(c_szIncomingID,          "IncomingID");
STR_GLOBAL(c_szChanging,            "Changing");
#endif  //_STRCONST_H
