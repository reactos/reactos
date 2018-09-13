/****************************** Module Header ******************************\
* Module Name: strings.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Defines strings that do not need to be localized.
*
* History:
* 11-17-92 Davidc       Created.
\***************************************************************************/

//
// App name strings
//

#define WINLOGON                   TEXT("WINLOGON")
#define WINLOGON_KEY               TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define NOTIFY_KEY                 TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify")
#define SCREENSAVER_KEY            TEXT("Control Panel\\Desktop")

//
// Policies
//

#define WINLOGON_POLICY_KEY        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System")
#define SCREENSAVER_POLICY_KEY     TEXT("Software\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop")
#define DISABLE_BKGND_POLICY       TEXT("DisableBkGndGroupPolicy")
#define SYNC_MACHINE_GROUP_POLICY  TEXT("SynchronousMachineGroupPolicy")
#define SYNC_USER_GROUP_POLICY     TEXT("SynchronousUserGroupPolicy")
#define MAX_GPO_SCRIPT_WAIT        TEXT("MaxGPOScriptWait")
#define SYNC_LOGON_SCRIPT          TEXT("RunLogonScriptSync")
#define SYNC_STARTUP_SCRIPT        TEXT("RunStartupScriptSync")
#define VERBOSE_STATUS             TEXT("VerboseStatus")

//
// Win9x upgrade domain join special case
//

#define LOCAL_USERS_KEY            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\LocalUsers")

//
// Safe boot strings
//

#define SAFEBOOT_OPTION_KEY  TEXT("System\\CurrentControlSet\\Control\\SafeBoot\\Option")
#define OPTION_VALUE         TEXT("OptionValue")


#define CALAIS_PATH         TEXT("Software\\Microsoft\\Cryptography\\Calais\\Readers")

//
// Define where we store the most recent logon information
//

#define APPLICATION_NAME                WINLOGON

//
// Define where we get screen-saver information
//

#define SCREEN_SAVER_INI_FILE           TEXT("system.ini")
#define SCREEN_SAVER_INI_SECTION        TEXT("boot")
#define SCREEN_SAVER_FILENAME_KEY       TEXT("SCRNSAVE.EXE")


#define WINDOWS_INI_SECTION             TEXT("Windows")

//
// Must be ANSI
//

#define SCREEN_SAVER_ENABLED_KEY        "ScreenSaveActive"
#define SCREEN_SAVER_SECURE_KEY         "ScreenSaverIsSecure"

//
// Gina is loaded from:
//

#define GINA_KEY                        TEXT("GinaDll")
//
// Value: GINADll
//
// Definition: Alternate DLL to provide Logon UI.  By default this is
// msgina.dll, although this is not in the registry by default.
//

#define LOCK_GRACE_PERIOD_KEY           TEXT("ScreenSaverGracePeriod")

//
// Value: ScreenSaverGracePeriod
//
// Definition:  grace period allowed for user movement before screensaver
// lock is considered.  default:  5 seconds.  Not present by default.
//
#define LOCK_DEFAULT_VALUE              5


#define KEEP_RAS_AFTER_LOGOFF           TEXT("KeepRasConnections")
//
// Value:  KeepRasConnections
//
// Definition:  control whether RAS connections are dropped after logoff,
// or kept for next logon.  Default value: 0.  Not present by default.  Type
// REG_SZ
//

#define RESTRICT_NONINTERACTIVE_ACCESS  TEXT("RestrictNonInteractiveAccess")
//
// Value:  RestrictNonInteractiveAccess
//
// Definition:  Control default access control on the window station and 
// desktop.  Default - local administrators can create windows on the desktop.
// Setting to 1 will restrict the desktop and window station to only the
// interactive user.  Default 0, not present by default.  Type = REG_SZ.  
// range 0 or 1.

#define MAX_RETRY_SYSVOL_ACCESS         TEXT("MaxRetrySysvolAccess")
//
// Value:  MaxRetrySysvolAccess
//
// Definition:  On domain controllers only, this is the max number of seconds
// winlogon will wait trying to access the sysvol to confirm it is ready
// type = REG_DWORD, range 0 to MAX_DWORD
//

#define ALLOCATE_FLOPPIES               TEXT("AllocateFloppies")
//
// Value: AllocateFloppies
//
// Definition:  allocate the floppy drive so that only the interactive 
// user can read or write to it.  Default 0, not present by default,
// type = REG_SZ, range 0 or 1.
//

#define ALLOCATE_CDROMS                 TEXT("AllocateCDRoms")
//
// Value: AllocateCDRoms
//
// Definition:  allocate the CDRoms so that only the interactive user
// can read them.  Default 0, not present by default,
// type = REG_SZ, range 0 or 1.
//

#define ALLOCATE_DASD                   TEXT("AllocateDASD")
//
// Value: AllocateDASD
//
// Definition:  allocate removable hard disks so that the interactive
// user can eject.Default 0, not present by default,
// type = REG_SZ, range 0, 1, or 2.
//

#define SERVICE_CONTROLLER_START        TEXT("ServiceControllerStart")
#define LSASS_START                     TEXT("LsaStart")

#define NOTIFY_LOGON                    "Logon"
#define NOTIFY_LOGOFF                   "Logoff"
#define NOTIFY_STARTUP                  "Startup"
#define NOTIFY_SHUTDOWN                 "Shutdown"
#define NOTIFY_STARTSCREENSAVER         "StartScreenSaver"
#define NOTIFY_STOPSCREENSAVER          "StopScreenSaver"
#define NOTIFY_LOCK                     "Lock"
#define NOTIFY_UNLOCK                   "Unlock"
#define NOTIFY_STARTSHELL               "StartShell"
#define NOTIFY_POSTSHELL                "PostShell"
#define NOTIFY_IMPERSONATE              TEXT("Impersonate")
#define NOTIFY_ASYNCHRONOUS             TEXT("Asynchronous")
#define NOTIFY_DLLNAME                  TEXT("DLLName")
#define NOTIFY_SAFEMODE                 TEXT("SafeMode")
#define NOTIFY_MAXWAIT                  TEXT("MaxWait")


//
// Shell= line in the registry
//

#define SHELL_KEY           TEXT("Shell")
#define RASAPI32            TEXT("rasapi32.dll")
#define RASMAN_SERVICE_NAME TEXT("RASMAN")

#define WINSTATIONNAME_VARIABLE TEXT("WINSTATIONNAME")
