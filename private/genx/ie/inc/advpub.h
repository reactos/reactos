//*     Copyright (c) Microsoft Corporation 1995-1998. All rights reserved. *
//***************************************************************************
//*                                                                         *
//* ADVPUB.H - Specify the Interface for ADVPACK.DLL                        *
//*                                                                         *
//***************************************************************************


#ifndef _ADVPUB_H_
#define _ADVPUB_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: RunSetupCommand
//
// SYNOPSIS:    Execute an install section in an INF file, or execute a
//              program.  Advanced INF files are supported.
//
// RETURN CODES:
//
//      S_OK                                 Everything OK, no reboot needed.
//                                           No EXE to wait for.
//      S_ASYNCHRONOUS                       Please wait on phEXE.
//      ERROR_SUCCESS_REBOOT_REQUIRED        Reboot required.
//      E_INVALIDARG                         NULL specified in szCmdName or szDir
//      HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION) INF's not supported on this OS version
//      E_UNEXPECTED                         Catastrophic failure(should never happen).
//      HRESULT_FROM_WIN32(GetLastError())   Anything else
/////////////////////////////////////////////////////////////////////////////

#ifndef S_ASYNCHRONOUS
#define S_ASYNCHRONOUS  _HRESULT_TYPEDEF_(0x401e8L)
#endif

#define achRUNSETUPCOMMANDFUNCTION   "RunSetupCommand"

HRESULT WINAPI RunSetupCommand( HWND hWnd, LPCSTR szCmdName,
                                LPCSTR szInfSection, LPCSTR szDir,
                                LPCSTR lpszTitle, HANDLE *phEXE,
                                DWORD dwFlags, LPVOID pvReserved );

typedef HRESULT (WINAPI *RUNSETUPCOMMAND)(
    HWND    hWnd,                       // Handle to parent window  NULL=Quiet mode
    LPCSTR  szCmdName,                  // Inf or EXE filename to "run"
    LPCSTR  szInfSection,               // Inf section to install.  NULL="DefaultInstall"
    LPCSTR  szDir,                      // Path to extracted files
    LPCSTR  szTitle,                    // Title for all dialogs
    HANDLE *phEXE,                      // Handle to EXE to wait for
    DWORD   dwFlags,                    // Flags to specify functionality (see above)
    LPVOID  pvReserved                  // Reserved for future use
);

// FLAGS:

#define RSC_FLAG_INF                1   // exxcute INF install
#define RSC_FLAG_SKIPDISKSPACECHECK 2   // Currently does nothing
#define RSC_FLAG_QUIET              4   // quiet mode, no UI
#define RSC_FLAG_NGCONV             8   // don't run groupConv
#define RSC_FLAG_UPDHLPDLLS         16  // force to self-updating on user's system
#define RSC_FLAG_DELAYREGISTEROCX  512  // force delay of ocx registration
#define RSC_FLAG_SETUPAPI	  1024  // use setupapi.dll

// please not adding flag after this.  See LaunchINFSectionEx() flags.

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: NeedRebootInit
//
// SYNOPSIS:    Initializes state for reboot checking.  Call this function
//              before calling RunSetupCommand.
// RETURNS:     value required to be passed to NeedReboot()
/////////////////////////////////////////////////////////////////////////////

#define achNEEDREBOOTINITFUNCTION   "NeedRebootInit"

DWORD WINAPI NeedRebootInit( VOID );

typedef DWORD (WINAPI *NEEDREBOOTINIT)(VOID);

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: NeedReboot
//
// SYNOPSIS:    Compares stored state with current state to determine if a
//              reboot is required.
//      dwRebootCheck   the return value from NeedRebootInit
//
// RETURNS:
//      TRUE            if a reboot is required;
//      FALSE           otherwise.
/////////////////////////////////////////////////////////////////////////////

#define achNEEDREBOOTFUNCTION   "NeedReboot"

BOOL WINAPI NeedReboot( DWORD dwRebootCheck );

typedef BOOL (WINAPI *NEEDREBOOT)(
	DWORD dwRebootCheck                                     // Value returned from NeedRebootInit
);

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: DoReboot
//
// SYNOPSIS:    Ask advpack to do reboot.
//      hwnd        if it is INVALID_HANDLE_VALUE, no user prompt.  Otherwise promp.
//      pszTitle    User prompt UI title string.
//      dwReserved  Not used.
// RETURNS:
//      FALSE       User choose NO to reboot prompt.
/////////////////////////////////////////////////////////////////////////////

// #define achDOREBOOT "DoReboot"

// BOOL WINAPI DoReboot( HWND hwnd, BOOL bDoUI );
// typedef BOOL (WINAPI* DOREBOOT)( HWND hwnd, BOOL bDoUI );

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: RebootCheckOnInstall
//
// SYNOPSIS:    Check reboot condition if the given INF section is installed.
//      hwnd    windows handle
//      pszINF  INF filename with fully qualified path
//      pszSec  INF section.  NULL is translated as DefaultInstall or DefaultInstall.NT.
//      dwReserved Not used.
// RETURN:
//      S_OK    Reboot needed if INF section is installed.
//      S_FALSE Reboot is not needed if INF section is installed.
//      HRESULT of Win 32 errors
//
/////////////////////////////////////////////////////////////////////////////

#define achPRECHECKREBOOT   "RebootCheckOnInstall"

HRESULT WINAPI RebootCheckOnInstall( HWND hwnd, PCSTR pszINF, PCSTR pszSec, DWORD dwReserved );

typedef HRESULT (WINAPI *REBOOTCHECKONINSTALL)( HWND, PCSTR, PCSTR, DWORD );

//////////////////////////////////////////////////////////////////////////
// ENTRY POINT: TranslateInfString
//
// SYNOPSIS:    Translates a key value in an INF file, using advanced INF
//              syntax.
// RETURN CODES:
//      S_OK                                 Everything OK.
//      HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
//                                      The buffer size is too small to hold the
//                                      translated string.  Required size is in *pdwRequiredSize.
//      E_INVALIDARG                         NULL specified in pszInfFilename, pszTranslateSection,
//                                      pszTranslateKey, pdwRequiredSize.
//      HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION)
//                                      OS not supported.
//      E_UNEXPECTED                         Catastrophic failure -- should never happen.
//      HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER)
//                                      The section or key specified does not exist.
//      HRESULT_FROM_WIN32(GetLastError())   Anything else
//
/////////////////////////////////////////////////////////////////////////////

#define c_szTRANSLATEINFSTRING "TranslateInfString"

HRESULT WINAPI TranslateInfString( PCSTR pszInfFilename, PCSTR pszInstallSection,
                                   PCSTR pszTranslateSection, PCSTR pszTranslateKey,
                                   PSTR pszBuffer, DWORD dwBufferSize,
                                   PDWORD pdwRequiredSize, PVOID pvReserved );

typedef HRESULT (WINAPI *TRANSLATEINFSTRING)(
    PCSTR  pszInfFilename,              // Name of INF file to process
    PCSTR  pszInstallSection,           // Install section name (NULL=DefaultInstall)
    PCSTR  pszTranslateSection,         // Section that contains key to translate
    PCSTR  pszTranslateKey,             // Key to translate
    PSTR   pszBuffer,                   // Buffer to store translated key.  (NULL=return required size only)
    DWORD  dwBufferSize,                // Size of this buffer.  If pszBuffer==NULL, this is ignored.
    PDWORD pdwRequiredSize,             // Required size of buffer
    PVOID  pvReserved                   // Reserved for future use
);

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: RegInstall
//
// SYNOPSIS:    Loads an INF from a string resource, adds some entries to the
//              INF string substitution table, and executes the INF.
// RETURNS:
//      S_OK    success.
//      E_FAIL  failure,
/////////////////////////////////////////////////////////////////////////////

#define achREGINSTALL   "RegInstall"

typedef struct _StrEntry {
    LPSTR   pszName;            // String to substitute
    LPSTR   pszValue;           // Replacement string or string resource
} STRENTRY, *LPSTRENTRY;

typedef const STRENTRY CSTRENTRY;
typedef CSTRENTRY *LPCSTRENTRY;

typedef struct _StrTable {
    DWORD       cEntries;       // Number of entries in the table
    LPSTRENTRY  pse;            // Array of entries
} STRTABLE, *LPSTRTABLE;

typedef const STRTABLE CSTRTABLE;
typedef CSTRTABLE *LPCSTRTABLE;

HRESULT WINAPI RegInstall( HMODULE hm, LPCSTR pszSection, LPCSTRTABLE pstTable );

typedef HRESULT (WINAPI *REGINSTALL)(
    HMODULE hm,                         // Module that contains REGINST resource
    LPCSTR pszSection,                  // Section of INF to execute
    LPCSTRTABLE pstTable                // Additional string substitutions
);


/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: LaunchINFSectionEx
//
// SYNOPSIS:    Install INF section with BACKUP/ROLLBACK capabilities.
//
// RETURNS:     E_FAIL on failure, S_OK on success.
/////////////////////////////////////////////////////////////////////////////

#define achLAUNCHINFSECTIONEX   "LaunchINFSectionEx"

HRESULT WINAPI LaunchINFSectionEx( HWND hwnd, HINSTANCE hInstance, PSTR pszParms, INT nShow );

typedef HRESULT (WINAPI *LAUNCHINFSECTIONEX)(
    HWND     hwnd,                      // pass in window handle
    HINSTANCE hInst,                    // instance handle
    PSTR     pszParams,                 // String contains params: INF,section,CAB,flags
    INT      nShow
);

// FLAGS:
// FLAGS value this way is for compatibility. Don't change them.
//
#define ALINF_QUIET              4      // quiet mode, no UI
#define ALINF_NGCONV             8      // don't run groupConv
#define ALINF_UPDHLPDLLS         16     // force to self-updating on user's system
#define ALINF_BKINSTALL          32     // backup data before install
#define ALINF_ROLLBACK           64     // rollback to previous state
#define ALINF_CHECKBKDATA        128    // validate the backup data
#define ALINF_ROLLBKDOALL        256    // bypass building file list
#define ALINF_DELAYREGISTEROCX   512    // force delay of ocx registration


/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: ExecuteCab
//
// SYNOPSIS:    Extract the an INF from the CAB file, and do INF install on it.
/////////////////////////////////////////////////////////////////////////////

// RETURNS: E_FAIL on failure, S_OK on success.

#define achEXECUTECAB   "ExecuteCab"

typedef struct _CabInfo {
    PSTR  pszCab;
    PSTR  pszInf;
    PSTR  pszSection;
    char  szSrcPath[MAX_PATH];
    DWORD dwFlags;
} CABINFO, *PCABINFO;

HRESULT WINAPI ExecuteCab( HWND hwnd, PCABINFO pCab, LPVOID pReserved );

typedef HRESULT (WINAPI *EXECUTECAB)(
    HWND     hwnd,
    PCABINFO pCab,
    LPVOID   pReserved
);

// flag as LaunchINFSectionEx's flag defines

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: AdvInstallFile
//
// SYNOPSIS:    To copy a file from the source to a destination
//              Basicly a wrapper around the setupapi file copy engine
/////////////////////////////////////////////////////////////////////////////

// Flags which can be passed to AdvInstallFile
// Here is a copy of the flags defined in setupapi.h for reference below.
//#define COPYFLG_WARN_IF_SKIP            0x00000001   // warn if user tries to skip file
//#define COPYFLG_NOSKIP                  0x00000002   // disallow skipping this file
//#define COPYFLG_NOVERSIONCHECK          0x00000004   // ignore versions and overwrite target
//#define COPYFLG_FORCE_FILE_IN_USE       0x00000008   // force file-in-use behavior
//#define COPYFLG_NO_OVERWRITE            0x00000010   // do not copy if file exists on target
//#define COPYFLG_NO_VERSION_DIALOG       0x00000020   // do not copy if target is newer
//#define COPYFLG_REPLACEONLY             0x00000400   // copy only if file exists on target

#define AIF_WARNIFSKIP          0x00000001              // system critical file: warn if user tries to skip
#define AIF_NOSKIP              0x00000002              // Skip is disallowed for this file
#define AIF_NOVERSIONCHECK      0x00000004              // don't check the version number of the file overwrite
#define AIF_FORCE_FILE_IN_USE   0x00000008              // force file-in-use behavior
#define AIF_NOOVERWRITE         0x00000010              // copy only if target doesn't exist
                                                        // if AIF_QUIET, the file is not copied and
                                                        // the user is not notified
#define AIF_NO_VERSION_DIALOG   0x00000020              // do not copy if target is newer
#define AIF_REPLACEONLY         0x00000400              // copy only if target file already present

// Flags only known to AdvInstallFile
#define AIF_NOLANGUAGECHECK     0x10000000              // don't check the language of the file
                                                        // if the flags is NOT specified and AIF_QUIET
                                                        // the file is not copied and the user is not notified
#define AIF_QUIET               0x20000000              // No UI to the user


#define achADVINSTALLFILE   "AdvInstallFile"

HRESULT WINAPI AdvInstallFile(HWND hwnd, LPCSTR lpszSourceDir, LPCSTR lpszSourceFile,
                              LPCSTR lpszDestDir, LPCSTR lpszDestFile, DWORD dwFlags, DWORD dwReserved);

typedef HRESULT (WINAPI *ADVINSTALLFILE)(
                                            HWND hwnd,                  // Parent Window for messages
                                            LPCSTR lpszSourceDir,       // Source directory (does not contain filename)
                                            LPCSTR lpszSourceFile,      // Filename only
                                            LPCSTR lpszDestDir,         // Destination directory (does not contain filename)
                                            LPCSTR lpszDestFile,        // optional filename. if NULL lpszSourceFile is used
                                            DWORD dwFlags,              // AIF_* FLAGS
                                            DWORD dwReserved);

//////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////
// the following flags are for backwards compatiable.  No API user
// should reference them directly now.
//
#define  IE4_RESTORE        0x00000001      // if this bit is off, save the registries.
#define  IE4_BACKNEW        0x00000002      // backup all files which are not backed up before
#define  IE4_NODELETENEW    0x00000004      // don't delete files we don't backed up before
#define  IE4_NOMESSAGES     0x00000008      // No message display in any events.
#define  IE4_NOPROGRESS     0x00000010      // this bit on: No file backup progressbar
#define  IE4_NOENUMKEY      0x00000020      // this bit on: Don't Enum sub key even there is no given valuename
#define  IE4_NO_CRC_MAPPING 0x00000040      // Normally you should not turn on this bit, advpack creates
                                            // internal mapping for all the entries backed up.
#define  IE4_REGSECTION     0x00000080      // INF AddReg/DelReg section
#define  IE4_FRDOALL        0x00000100      // FileRestore DoAll
#define  IE4_UPDREFCNT	    0x00000200      // Update the ref count in .ini backup file list
#define  IE4_USEREFCNT	    0x00000400      // use ref count to determin if the backup file should be put back
#define  IE4_EXTRAINCREFCNT 0x00000800	    // if increase the ref cnt if it has been updated before

#define  IE4_REMOVREGBKDATA 0x00001000      // This bit should be used with restore bit

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: RegSaveRestore
//
// SYNOPSIS:    Save or Restore the given register value or given INF reg section.
//
// RETURNS:     E_FAIL on failure, S_OK on success.
/////////////////////////////////////////////////////////////////////////////

// Save or Restore the given register value
HRESULT WINAPI RegSaveRestore(HWND hWnd, PCSTR pszTitleString, HKEY hkBckupKey, PCSTR pcszRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, DWORD dwFlags);

typedef HRESULT (WINAPI *REGSAVERESTORE)( HWND hWnd,
                                          PCSTR pszTitleString,  // user specified UI title
                                          HKEY hkBckupKey,       // opened Key handle to store the backup data
                                          PCSTR pcszRootKey,     // RootKey string
                                          PCSTR pcszSubKey,      // SubKey string
                                          PCSTR pcszValueName,   // Value name string
                                          DWORD dwFlags);        // Flags

// Save or Restore the given INF Reg Section. At restore, if INF and Section pointers are NULL,
// Restore all from the given backup key handle.
HRESULT WINAPI RegSaveRestoreOnINF( HWND hWnd, PCSTR pszTitle, PCSTR pszINF,
                                    PCSTR pszSection, HKEY hHKLMBackKey, HKEY hHKCUBackKey, DWORD dwFlags );

typedef HRESULT (WINAPI *REGSAVERESTOREONINF)( HWND hWnd,
                                              PCSTR pszTitle,        // user specified UI title
                                              PCSTR pszINF,          // INF filename with fully qualified path
                                              PCSTR pszSection,       // INF section name.  NULL == default
                                              HKEY hHKLMBackKey,       // openned key handle to store the data
                                              HKEY hHKCUBackKey,       // openned key handle to store the data
                                              DWORD dwFlags );       // Flags

// FLAG:
#define ARSR_RESTORE    IE4_RESTORE       // if this bit is off, means Save. Otherwise, restore.
#define ARSR_NOMESSAGES IE4_NOMESSAGES    // Quiet no messages in any event.
#define ARSR_REGSECTION IE4_REGSECTION    // if this bit is off, the given section is GenInstall Section
#define ARSR_REMOVREGBKDATA IE4_REMOVREGBKDATA // if both this bit and restore bit on, remove the backup reg data without restore it

// Turn on the logging by add these RegVale in HKLM\software\microsoft\IE4
#define  REG_SAVE_LOG_KEY    "RegSaveLogFile"
#define  REG_RESTORE_LOG_KEY "RegRestoreLogFile"

// for backwards compatible add this one back
HRESULT WINAPI RegRestoreAll(HWND hWnd, PSTR pszTitleString, HKEY hkBckupKey);
typedef HRESULT (WINAPI *REGRESTOREALL)(HWND hWnd, PSTR pszTitleString, HKEY hkBckupKey);
/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: FileSaveRestore
//
// SYNOPSIS:    Save or Restore the files on the list lpFileList.
//              If lpFileList is NULL at restore time, the function will restore
//              all based on INI index file.
//
// RETURNS:     E_FAIL on failure, S_OK on success.
/////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI FileSaveRestore( HWND hDlg, LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName, DWORD dwFlags);

typedef HRESULT (WINAPI *FILESAVERESTORE)( HWND hDlg,
                                           LPSTR lpFileList,    // File list file1\0file2\0filen\0\0
                                           LPSTR lpDir,         // pathname of the backup directory
                                           LPSTR lpBaseName,    // backup file basename
                                           DWORD dwFlags);      // Flags

HRESULT WINAPI FileSaveRestoreOnINF( HWND hWnd, PCSTR pszTitle, PCSTR pszINF,
                                     PCSTR pszSection, PCSTR pszBackupDir, PCSTR pszBaseBackupFile,
                                     DWORD dwFlags );

typedef HRESULT (WINAPI *FILESAVERESTOREONINF)( HWND hDlg,
                                                  PCSTR pszTitle,        // user specified UI title
                                                  PCSTR pszINF,          // INF filename with fully qualified path
                                                  PCSTR pszSection,      // GenInstall INF section name.  NULL == default
                                                  PCSTR pszBackupDir,    // directory to store the backup file
                                                  PCSTR pszBaseBackFile, // Basename of the backup data files
                                                  DWORD dwFlags );       // Flags


// FLAGS:
#define  AFSR_RESTORE        IE4_RESTORE      // if this bit is off, save the file.
#define  AFSR_BACKNEW        IE4_BACKNEW      // backup all files which are not backed up before
#define  AFSR_NODELETENEW    IE4_NODELETENEW  // don't delete files we don't backed up before
#define  AFSR_NOMESSAGES     IE4_NOMESSAGES   // No message display in any events.
#define  AFSR_NOPROGRESS     IE4_NOPROGRESS   // this bit on: No file backup progressbar
#define  AFSR_UPDREFCNT      IE4_UPDREFCNT    // update the reference count for the files
#define  AFSR_USEREFCNT	     IE4_USEREFCNT    // use the ref count to guide the restore file
#define  AFSR_EXTRAINCREFCNT IE4_EXTRAINCREFCNT

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: AddDelBackupEntry
//
// SYNOPSIS:    If AADBE_ADD_ENTRY is specified, mark the file in the File list as not existing
//              during file save in the INI file.  This can be used to mark additional files that
//              they did not exist during backup to avoid having them backup the next time the
//              FileSaveRestore is called to save files.
//              If AADBE_DEL_ENTRY is specified, delete the entry from the INI.  This mechanism can
//              be used to leave files permanently on the system.
//
// RETURNS:
//      S_OK    success
//      E_FAIL  failure
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI AddDelBackupEntry(LPCSTR lpcszFileList, LPCSTR lpcszBackupDir, LPCSTR lpcszBaseName, DWORD dwFlags);

typedef HRESULT (WINAPI *ADDDELBACKUPENTRY)(LPCSTR lpcszFileList,   // File list file1\0file2\0filen\0\0
                                           LPCSTR lpcszBackupDir,   // pathname of the backup directory
                                           LPCSTR lpcszBaseName,    // backup file basename
                                           DWORD  dwFlags);

#define  AADBE_ADD_ENTRY    0x01            // add entries to the INI file
#define  AADBE_DEL_ENTRY    0x02            // delete entries from the INI file

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: FileSaveMarkNotExist
//
// SYNOPSIS:    Mark the file in the File list as not existing during file save in the INI file
//              This can be used to mark additional files that they did not exist during backup
//              to avoid having them backup the next time the FileSaveRestore is called to save
//              files
//
// RETURNS:
//      S_OK    success
//      E_FAIL  failure
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI FileSaveMarkNotExist( LPSTR lpFileList, LPSTR lpDir, LPSTR lpBaseName);

typedef HRESULT (WINAPI *FILESAVEMARKNOTEXIST)( LPSTR lpFileList,    // File list file1\0file2\0filen\0\0
                                           LPSTR lpDir,         // pathname of the backup directory
                                           LPSTR lpBaseName);    // backup file basename

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: GetVersionFromFile
//
// SYNOPSIS:    Get the given file's version and lang information.
//
// RETURNS:     E_FAIL on failure, S_OK on success.
/////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI GetVersionFromFile(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);

typedef HRESULT (WINAPI *GETVERSIONFROMFILE)(
                                                LPSTR lpszFilename,         // filename to get info from
                                                LPDWORD pdwMSVer,           // Receive Major version
                                                LPDWORD pdwLSVer,           // Receive Minor version
                                                BOOL bVersion);             // if FALSE, pdwMSVer receive lang ID
                                                                            // pdwLSVer receive Codepage ID

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: GetVersionFromFileEx
//
// SYNOPSIS:    Get the given disk file's version and lang information.
//
// RETURNS:     E_FAIL on failure, S_OK on success.
/////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI GetVersionFromFileEx(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);

typedef HRESULT (WINAPI *GETVERSIONFROMFILE)(
                                                LPSTR lpszFilename,         // filename to get info from
                                                LPDWORD pdwMSVer,           // Receive Major version
                                                LPDWORD pdwLSVer,           // Receive Minor version
                                                BOOL bVersion);             // if FALSE, pdwMSVer receive lang ID
                                                                            // pdwLSVer receive Codepage ID

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: IsNTAdmin
//
// SYNOPSIS:    On NT, check if user has admin right.
//
// RETURNS:     TURE  has admin right; FLSE  no admin right.
/////////////////////////////////////////////////////////////////////////////

#define achISNTADMIN "IsNTAdmin"

BOOL WINAPI IsNTAdmin( DWORD dwReserved, DWORD *lpdwReserved );

typedef BOOL (WINAPI *ISNTADMIN)( DWORD,        // not used
                                  DWORD * );    // not used

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: DelNode
//
// SYNOPSIS:    Deletes a file or directory
//
// RETURNS:
//      S_OK    success
//      E_FAIL  failure
/////////////////////////////////////////////////////////////////////////////

// FLAGS:
#define ADN_DEL_IF_EMPTY        0x00000001  // delete the directory only if it's empty
#define ADN_DONT_DEL_SUBDIRS    0x00000002  // don't delete any sub-dirs; delete only the files
#define ADN_DONT_DEL_DIR        0x00000004  // don't delete the dir itself
#define ADN_DEL_UNC_PATHS       0x00000008  // delete UNC paths

#define achDELNODE              "DelNode"

HRESULT WINAPI DelNode(LPCSTR pszFileOrDirName, DWORD dwFlags);

typedef HRESULT (WINAPI *DELNODE)(
    LPCSTR pszFileOrDirName,                // Name of file or directory to delete
    DWORD dwFlags                           // 0, ADN_DEL_IF_EMPTY, etc. can be specified
);

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: DelNodeRunDLL32
//
// SYNOPSIS:    Deletes a file or directory; the parameters to this API are of
//              WinMain type
//
// RETURNS:
//      S_OK    success
//      E_FAIL  failure
/////////////////////////////////////////////////////////////////////////////

#define achDELNODERUNDLL32      "DelNodeRunDLL32"

HRESULT WINAPI DelNodeRunDLL32(HWND hwnd, HINSTANCE hInstance, PSTR pszParms, INT nShow);

typedef HRESULT (WINAPI *DELNODERUNDLL32)(
    HWND     hwnd,                          // pass in window handle
    HINSTANCE hInst,                        // instance handle
    PSTR     pszParams,                     // String contains params: FileOrDirName,Flags
    INT      nShow
);

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: OpenINFEngine, TranslateINFStringEx, CloseINFEngine
//
// SYNOPSIS:    Three APIs give the caller the option to be more efficient when need
//              Advpack to translate INF file in a continue fashion.
//
// RETURNS:
//      S_OK    success
//      E_FAIL  failure
/////////////////////////////////////////////////////////////////////////////

#if !defined(UNIX) || !defined(_INC_SETUPAPI) // IEUNIX: Prevent re-def.
//
// Define type for reference to loaded inf file
// (from setupapi.h)
//
typedef PVOID HINF;
#endif

HRESULT WINAPI OpenINFEngine( PCSTR pszInfFilename, PCSTR pszInstallSection,
                              DWORD dwFlags, HINF *phInf, PVOID pvReserved );

HRESULT WINAPI TranslateInfStringEx( HINF hInf, PCSTR pszInfFilename,
                                     PCSTR pszTranslateSection, PCSTR pszTranslateKey,
                                     PSTR pszBuffer, DWORD dwBufferSize,
                                     PDWORD pdwRequiredSize, PVOID pvReserved );

HRESULT WINAPI CloseINFEngine( HINF hInf );



HRESULT WINAPI ExtractFiles( LPCSTR pszCabName, LPCSTR pszExpandDir, DWORD dwFlags,
                             LPCSTR pszFileList, LPVOID lpReserved, DWORD dwReserved);

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: LaunchINFSection
//
// SYNOPSIS:    Install INF section WITHOUT BACKUP/ROLLBACK capabilities.
//
// RETURNS:     E_FAIL on failure, S_OK on success.
/////////////////////////////////////////////////////////////////////////////

INT     WINAPI LaunchINFSection( HWND, HINSTANCE, PSTR, INT );

// LaunchINFSection flags
#define LIS_QUIET               0x0001      // Bit 0
#define LIS_NOGRPCONV           0x0002      // Bit 1

// Flags in Advanced INF RunPreSetupCommands and RunPostSetupCommands of the Install section
// Those flags can tell advpack how to run those commands, quiet or not quiet, wait or not wait.
// The Default for runing those commands are:  Not Quiet and Wait for finish before return the caller.
// I.E>  RunPostSetupCommands = MyCmdsSecA:1, MyCmdsSecB:2, MyCmdsSecC
//
#define RUNCMDS_QUIET		0x00000001
#define RUNCMDS_NOWAIT		0x00000002
#define RUNCMDS_DELAYPOSTCMD	0x00000004

// Active Setup Installed Components GUID for IE4
#define awchMSIE4GUID L"{89820200-ECBD-11cf-8B85-00AA005B4383}"

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: UserStubWrapper
//
// SYNOPSIS:    The function wrapper around the real per-user restore stub to 
//              do some generic/intelligent function on behalf of every component.
//
// RETURNS:     E_FAIL on failure, S_OK on success.
/////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI UserInstStubWrapper( HWND hwnd, HINSTANCE hInstance, PSTR pszParms, INT nShow ); 

#define achUserInstStubWrapper      "UserInstStubWrapper"

typedef HRESULT (WINAPI *USERINSTSTUBWRAPPER)(
                                           HWND     hwnd,                          // pass in window handle
                                           HINSTANCE hInst,                        // instance handle
                                           PSTR     pszParams,                     // String contains params: {GUID}
                                           INT      nShow
                                          );

HRESULT WINAPI UserUnInstStubWrapper( HWND hwnd, HINSTANCE hInstance, PSTR pszParms, INT nShow ); 

#define achUserUnInstStubWrapper      "UserUnInstStubWrapper"

typedef HRESULT (WINAPI *USERUNINSTSTUBWRAPPER)(
                                           HWND     hwnd,                          // pass in window handle
                                           HINSTANCE hInst,                        // instance handle
                                           PSTR     pszParams,                     // String contains params: {GUID}
                                           INT      nShow
                                          );

/////////////////////////////////////////////////////////////////////////////
// ENTRY POINT: SetPerUserInstValues
//
// SYNOPSIS:    The function set the per-user stub reg values under IsInstalled\{GUID} 
//              related key to ensure the later per-user process correctly.
//
// RETURNS:     E_FAIL on failure, S_OK on success.
/////////////////////////////////////////////////////////////////////////////

// Args passed to the following API
// MAX_GUID_STRING_LEN is 39 defined in cfgmgr32.h, here we just use it.
//
typedef struct _PERUSERSECTION { char szGUID[39+20];
                                 char szDispName[128];
       		                 char szLocale[10];
                                 char szStub[MAX_PATH*4];
                                 char szVersion[32];
                				 char szCompID[128]; 
                                 DWORD dwIsInstalled;
                                 BOOL  bRollback;
} PERUSERSECTION, *PPERUSERSECTION;


HRESULT WINAPI SetPerUserSecValues( PPERUSERSECTION pPerUser );

#define achSetPerUserSecValues      "SetPerUserSecValues"

typedef HRESULT (WINAPI *SETPERUSERSECVALUES)( PPERUSERSECTION pPerUser );


/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // _ADVPUB_H_
