/****************************** Module Header ******************************\
* Module Name: usrenv.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define constants user by and apis in usrenv.c
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#define COLON   TEXT(':')
#define BSLASH  TEXT('\\')

//
// Define the source for the event log handle used to log profile failures.
//
#define EVENTLOG_SOURCE       TEXT("Winlogon")


//
// Value names for for different environment variables
//

#define PATH_VARIABLE               TEXT("PATH")
#define LIBPATH_VARIABLE            TEXT("LibPath")
#define OS2LIBPATH_VARIABLE         TEXT("Os2LibPath")
#define AUTOEXECPATH_VARIABLE       TEXT("AutoexecPath")

#define HOMEDRIVE_VARIABLE          TEXT("HOMEDRIVE")
#define HOMESHARE_VARIABLE          TEXT("HOMESHARE")
#define HOMEPATH_VARIABLE           TEXT("HOMEPATH")
#define SESSIONNAME_VARIABLE        TEXT("SESSIONNAME")
#define APPDATA_VARAIBLE            TEXT("APPDATA")

#define COMPUTERNAME_VARIABLE       TEXT("COMPUTERNAME")
#define USERNAME_VARIABLE           TEXT("USERNAME")
#define USERDOMAIN_VARIABLE         TEXT("USERDOMAIN")
#define USERPROFILE_VARIABLE        TEXT("USERPROFILE")
#define ALLUSERSPROFILE_VARIABLE    TEXT("ALLUSERSPROFILE")
#define PROGRAMFILES_VARIABLE       TEXT("ProgramFiles")
#define COMMONPROGRAMFILES_VARIABLE TEXT("CommonProgramFiles")
#if defined(WX86) || defined(_AXP64_)
#define PROGRAMFILESX86_VARIABLE    TEXT("ProgramFiles(x86)")
#define COMMONPROGRAMFILESX86_VARIABLE TEXT("CommonProgramFiles(x86)")
#endif

//
// Default directories used when the user's home directory does not exist
// or is invalid.
//

#define ROOT_DIRECTORY          TEXT("\\")
#define USERS_DIRECTORY         TEXT("\\users")
#define USERS_DEFAULT_DIRECTORY TEXT("\\users\\default")

#define NULL_STRING             TEXT("")

//
// Defines for Logon script paths.
//

#define SERVER_SCRIPT_PATH      TEXT("\\NETLOGON\\")
#define LOCAL_SCRIPT_PATH       TEXT("\\repl\\import\\scripts\\")


//
// Prototypes
//


BOOL
SetupUserEnvironment(
    PTERMINAL pTerm
    );

VOID
ResetEnvironment(
    PTERMINAL pTerm
    );

BOOL
SetupBasicEnvironment(
    PVOID * ppEnv
    );

VOID
InitSystemParametersInfo(
    PTERMINAL   pTerm,
    BOOL        bUserLoggedOn
    );

BOOL
OpenHKeyCurrentUser(
    PWINDOWSTATION pWS
    );

VOID
CloseHKeyCurrentUser(
    PWINDOWSTATION pWS
    );
