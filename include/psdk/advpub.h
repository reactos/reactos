/*
 * Copyright 2004 Huw D M Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __WINE_ADVPUB_H
#define __WINE_ADVPUB_H

#include <setupapi.h>
/* FIXME: #include <cfgmgr32.h> */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef S_ASYNCHRONOUS
#define S_ASYNCHRONOUS  _HRESULT_TYPEDEF_(0x401E8L)
#endif

typedef struct _CabInfoA
{
    LPSTR  pszCab;
    LPSTR  pszInf;
    LPSTR  pszSection;
    CHAR   szSrcPath[MAX_PATH];
    DWORD  dwFlags;
} CABINFOA, *PCABINFOA;

typedef struct _CabInfoW
{
    LPWSTR pszCab;
    LPWSTR pszInf;
    LPWSTR pszSection;
    WCHAR  szSrcPath[MAX_PATH];
    DWORD  dwFlags;
} CABINFOW, *PCABINFOW;

DECL_WINELIB_TYPE_AW(CABINFO)
DECL_WINELIB_TYPE_AW(PCABINFO)

typedef struct _PERUSERSECTIONA
{
    CHAR  szGUID[39 /*MAX_GUID_STRING_LEN*/ + 20];
    CHAR  szDispName[128];
    CHAR  szLocale[10];
    CHAR  szStub[MAX_PATH * 4];
    CHAR  szVersion[32];
    CHAR  szCompID[128];
    DWORD dwIsInstalled;
    BOOL  bRollback;
} PERUSERSECTIONA, *PPERUSERSECTIONA;

typedef struct _PERUSERSECTIONW
{
    WCHAR szGUID[39 /*MAX_GUID_STRING_LEN*/ + 20];
    WCHAR szDispName[128];
    WCHAR szLocale[10];
    WCHAR szStub[MAX_PATH * 4];
    WCHAR szVersion[32];
    WCHAR szCompID[128];
    DWORD dwIsInstalled;
    BOOL  bRollback;
} PERUSERSECTIONW, *PPERUSERSECTIONW;

DECL_WINELIB_TYPE_AW(PERUSERSECTION)
DECL_WINELIB_TYPE_AW(PPERUSERSECTION)

typedef struct _StrEntryA
{
    LPSTR pszName;
    LPSTR pszValue;
} STRENTRYA, *LPSTRENTRYA;

typedef struct _StrEntryW
{
    LPWSTR pszName;
    LPWSTR pszValue;
} STRENTRYW, *LPSTRENTRYW;

DECL_WINELIB_TYPE_AW(STRENTRY)
DECL_WINELIB_TYPE_AW(LPSTRENTRY)

typedef struct _StrTableA
{
    DWORD cEntries;
    STRENTRYA* pse;
} STRTABLEA, *LPSTRTABLEA;
typedef const STRTABLEA CSTRTABLEA, *LPCSTRTABLEA;

typedef struct _StrTableW
{
    DWORD cEntries;
    STRENTRYW* pse;
} STRTABLEW, *LPSTRTABLEW;
typedef const STRTABLEW CSTRTABLEW, *LPCSTRTABLEW;

DECL_WINELIB_TYPE_AW(STRTABLE)
DECL_WINELIB_TYPE_AW(CSTRTABLE)
DECL_WINELIB_TYPE_AW(LPSTRTABLE)
DECL_WINELIB_TYPE_AW(LPCSTRTABLE)

/* Flags for AddDelBackupEntry */
#define AADBE_ADD_ENTRY             0x01
#define AADBE_DEL_ENTRY             0x02

/* Flags for AdvInstallFile */
#define AIF_WARNIFSKIP              0x00000001
#define AIF_NOSKIP                  0x00000002
#define AIF_NOVERSIONCHECK          0x00000004
#define AIF_FORCE_FILE_IN_USE       0x00000008
#define AIF_NOOVERWRITE             0x00000010
#define AIF_NO_VERSION_DIALOG       0x00000020
#define AIF_REPLACEONLY             0x00000400
#define AIF_NOLANGUAGECHECK         0x10000000
#define AIF_QUIET                   0x20000000

/* Flags for RunSetupCommand */
#define RSC_FLAG_INF                0x00000001
#define RSC_FLAG_SKIPDISKSPACECHECK 0x00000002
#define RSC_FLAG_QUIET              0x00000004
#define RSC_FLAG_NGCONV             0x00000008
#define RSC_FLAG_UPDHLPDLLS         0x00000010
#define RSC_FLAG_DELAYREGISTEROCX   0x00000200
#define RSC_FLAG_SETUPAPI           0x00000400

/* Flags for DelNode */
#define ADN_DEL_IF_EMPTY            0x00000001
#define ADN_DONT_DEL_SUBDIRS        0x00000002
#define ADN_DONT_DEL_DIR            0x00000004
#define ADN_DEL_UNC_PATHS           0x00000008

/* Flags for RegRestoreAll, RegSaveRestore, RegSaveRestoreOnINF */
#define  IE4_RESTORE                0x00000001
#define  IE4_BACKNEW                0x00000002
#define  IE4_NODELETENEW            0x00000004
#define  IE4_NOMESSAGES             0x00000008
#define  IE4_NOPROGRESS             0x00000010
#define  IE4_NOENUMKEY              0x00000020
#define  IE4_NO_CRC_MAPPING         0x00000040
#define  IE4_REGSECTION             0x00000080
#define  IE4_FRDOALL                0x00000100
#define  IE4_UPDREFCNT              0x00000200
#define  IE4_USEREFCNT              0x00000400
#define  IE4_EXTRAINCREFCNT         0x00000800

/* Flags for file save and restore functions */
#define  AFSR_RESTORE               IE4_RESTORE
#define  AFSR_BACKNEW               IE4_BACKNEW
#define  AFSR_NODELETENEW           IE4_NODELETENEW
#define  AFSR_NOMESSAGES            IE4_NOMESSAGES
#define  AFSR_NOPROGRESS            IE4_NOPROGRESS
#define  AFSR_UPDREFCNT             IE4_UPDREFCNT
#define  AFSR_USEREFCNT             IE4_USEREFCNT
#define  AFSR_EXTRAINCREFCNT        IE4_EXTRAINCREFCNT

HRESULT WINAPI AddDelBackupEntryA(LPCSTR lpcszFileList, LPCSTR lpcszBackupDir,
     LPCSTR lpcszBaseName, DWORD dwFlags);
HRESULT WINAPI AddDelBackupEntryW(LPCWSTR lpcszFileList, LPCWSTR lpcszBackupDir,
     LPCWSTR lpcszBaseName, DWORD dwFlags);
#define AddDelBackupEntry WINELIB_NAME_AW(AddDelBackupEntry)
HRESULT WINAPI AdvInstallFileA(HWND hwnd, LPCSTR lpszSourceDir,
     LPCSTR lpszSourceFile, LPCSTR lpszDestDir, LPCSTR lpszDestFile,
     DWORD dwFlags, DWORD dwReserved);
HRESULT WINAPI AdvInstallFileW(HWND hwnd, LPCWSTR lpszSourceDir,
     LPCWSTR lpszSourceFile, LPCWSTR lpszDestDir, LPCWSTR lpszDestFile,
     DWORD dwFlags, DWORD dwReserved);
#define AdvInstallFile WINELIB_NAME_AW(AdvInstallFile)
HRESULT WINAPI CloseINFEngine(HINF hInf);
HRESULT WINAPI DelNodeA(LPCSTR pszFileOrDirName, DWORD dwFlags);
HRESULT WINAPI DelNodeW(LPCWSTR pszFileOrDirName, DWORD dwFlags);
#define DelNode WINELIB_NAME_AW(DelNode)
HRESULT WINAPI DelNodeRunDLL32A(HWND,HINSTANCE,LPSTR,INT);
HRESULT WINAPI DelNodeRunDLL32W(HWND,HINSTANCE,LPWSTR,INT);
#define DelNodeRunDLL32 WINELIB_NAME_AW(DelNodeRunDLL32)
HRESULT WINAPI ExecuteCabA( HWND hwnd, CABINFOA* pCab, LPVOID pReserved );
HRESULT WINAPI ExecuteCabW( HWND hwnd, CABINFOW* pCab, LPVOID pReserved );
#define ExecuteCab WINELIB_NAME_AW(ExecuteCab)
HRESULT WINAPI ExtractFilesA(LPCSTR,LPCSTR,DWORD,LPCSTR,LPVOID,DWORD);
HRESULT WINAPI ExtractFilesW(LPCWSTR,LPCWSTR,DWORD,LPCWSTR,LPVOID,DWORD);
#define ExtractFiles WINELIB_NAME_AW(ExtractFiles)
HRESULT WINAPI FileSaveMarkNotExistA(LPSTR pszFileList, LPSTR pszDir, LPSTR pszBaseName);
HRESULT WINAPI FileSaveMarkNotExistW(LPWSTR pszFileList, LPWSTR pszDir, LPWSTR pszBaseName);
#define FileSaveMarkNotExist WINELIB_NAME_AW(FileSaveMarkNotExist)
HRESULT WINAPI FileSaveRestoreA(HWND hDlg, LPSTR pszFileList, LPSTR pszDir,
     LPSTR pszBaseName, DWORD dwFlags);
HRESULT WINAPI FileSaveRestoreW(HWND hDlg, LPWSTR pszFileList, LPWSTR pszDir,
     LPWSTR pszBaseName, DWORD dwFlags);
#define FileSaveRestore WINELIB_NAME_AW(FileSaveRestore)
HRESULT WINAPI FileSaveRestoreOnINFA(HWND hWnd, LPCSTR pszTitle, LPCSTR pszINF,
     LPCSTR pszSection, LPCSTR pszBackupDir, LPCSTR pszBaseBackupFile, DWORD dwFlags);
HRESULT WINAPI FileSaveRestoreOnINFW(HWND hWnd, LPCWSTR pszTitle, LPCWSTR pszINF,
     LPCWSTR pszSection, LPCWSTR pszBackupDir, LPCWSTR pszBaseBackupFile, DWORD dwFlags);
#define FileSaveRestoreOnINF WINELIB_NAME_AW(FileSaveRestoreOnINF)
HRESULT WINAPI GetVersionFromFileA(LPCSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
HRESULT WINAPI GetVersionFromFileW(LPCWSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
#define GetVersionFromFile WINELIB_NAME_AW(GetVersionFromFile)
HRESULT WINAPI GetVersionFromFileExA(LPCSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
HRESULT WINAPI GetVersionFromFileExW(LPCWSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
#define GetVersionFromFileEx WINELIB_NAME_AW(GetVersionFromFileEx)
BOOL WINAPI IsNTAdmin(DWORD,LPDWORD);
INT WINAPI LaunchINFSectionA(HWND,HINSTANCE,LPSTR,INT);
INT WINAPI LaunchINFSectionW(HWND,HINSTANCE,LPWSTR,INT);
#define LaunchINFSection WINELIB_NAME_AW(LaunchINFSection)
HRESULT WINAPI LaunchINFSectionExA(HWND,HINSTANCE,LPSTR,INT);
HRESULT WINAPI LaunchINFSectionExW(HWND,HINSTANCE,LPWSTR,INT);
#define LaunchINFSectionEx WINELIB_NAME_AW(LaunchINFSectionEx)
DWORD WINAPI NeedRebootInit(VOID);
BOOL WINAPI NeedReboot(DWORD dwRebootCheck);
HRESULT WINAPI OpenINFEngineA(LPCSTR pszInfFilename, LPCSTR pszInstallSection,
     DWORD dwFlags, HINF *phInf, PVOID pvReserved);
HRESULT WINAPI OpenINFEngineW(LPCWSTR pszInfFilename, LPCWSTR pszInstallSection,
     DWORD dwFlags, HINF *phInf, PVOID pvReserved);
#define OpenINFEngine WINELIB_NAME_AW(OpenINFEngine)
HRESULT WINAPI RebootCheckOnInstallA(HWND hWnd, LPCSTR pszINF, LPCSTR pszSec, DWORD dwReserved);
HRESULT WINAPI RebootCheckOnInstallW(HWND hWnd, LPCWSTR pszINF, LPCWSTR pszSec, DWORD dwReserved);
#define RebootCheckOnInstall WINELIB_NAME_AW(RebootCheckOnInstall)
HRESULT WINAPI RegInstallA(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
HRESULT WINAPI RegInstallW(HMODULE hm, LPCWSTR pszSection, const STRTABLEW* pstTable);
#define RegInstall WINELIB_NAME_AW(RegInstall)
HRESULT WINAPI RegRestoreAllA(HWND hWnd, LPSTR pszTitleString, HKEY hkBackupKey);
HRESULT WINAPI RegRestoreAllW(HWND hWnd, LPWSTR pszTitleString, HKEY hkBackupKey);
#define RegRestoreAll WINELIB_NAME_AW(RegRestoreAll)
HRESULT WINAPI RegSaveRestoreA(HWND hWnd, LPCSTR pszTitleString, HKEY hkBackupKey,
     LPCSTR pcszRootKey, LPCSTR pcszSubKey, LPCSTR pcszValueName, DWORD dwFlags);
HRESULT WINAPI RegSaveRestoreW(HWND hWnd, LPCWSTR pszTitleString, HKEY hkBackupKey,
     LPCWSTR pcszRootKey, LPCWSTR pcszSubKey, LPCWSTR pcszValueName, DWORD dwFlags);
#define RegSaveRestore WINELIB_NAME_AW(RegSaveRestore)
HRESULT WINAPI RegSaveRestoreOnINFA(HWND hWnd, LPCSTR pszTitle, LPCSTR pszINF,
     LPCSTR pszSection, HKEY hHKLMBackKey, HKEY hHKCUBackKey, DWORD dwFlags);
HRESULT WINAPI RegSaveRestoreOnINFW(HWND hWnd, LPCWSTR pszTitle, LPCWSTR pszINF,
     LPCWSTR pszSection, HKEY hHKLMBackKey, HKEY hHKCUBackKey, DWORD dwFlags);
#define RegSaveRestoreOnINF WINELIB_NAME_AW(RegSaveRestoreOnINF)
HRESULT WINAPI RunSetupCommandA(HWND hWnd,
     LPCSTR szCmdName, LPCSTR szInfSection, LPCSTR szDir, LPCSTR lpszTitle,
     HANDLE *phEXE, DWORD dwFlags, LPVOID pvReserved);
HRESULT WINAPI RunSetupCommandW(HWND hWnd,
     LPCWSTR szCmdName, LPCWSTR szInfSection, LPCWSTR szDir, LPCWSTR lpszTitle,
     HANDLE *phEXE, DWORD dwFlags, LPVOID pvReserved);
#define RunSetupCommand WINELIB_NAME_AW(RunSetupCommand)
HRESULT WINAPI SetPerUserSecValuesA(PPERUSERSECTIONA pPerUser);
HRESULT WINAPI SetPerUserSecValuesW(PPERUSERSECTIONW pPerUser);
#define SetPerUserSecValues WINELIB_NAME_AW(SetPerUserSecValues)
HRESULT WINAPI TranslateInfStringA(LPCSTR pszInfFilename, LPCSTR pszInstallSection,
     LPCSTR pszTranslateSection, LPCSTR pszTranslateKey, LPSTR pszBuffer,
     DWORD dwBufferSize, PDWORD pdwRequiredSize, PVOID pvReserved);
HRESULT WINAPI TranslateInfStringW(LPCWSTR pszInfFilename, LPCWSTR pszInstallSection,
     LPCWSTR pszTranslateSection, LPCWSTR pszTranslateKey, LPWSTR pszBuffer,
     DWORD dwBufferSize, PDWORD pdwRequiredSize, PVOID pvReserved);
#define TranslateInfString WINELIB_NAME_AW(TranslateInfString)
HRESULT WINAPI TranslateInfStringExA(HINF hInf, LPCSTR pszInfFilename,
    LPCSTR pszTranslateSection, LPCSTR pszTranslateKey, LPSTR pszBuffer,
    DWORD dwBufferSize, PDWORD pdwRequiredSize, PVOID pvReserved);
HRESULT WINAPI TranslateInfStringExW(HINF hInf, LPCWSTR pszInfFilename,
    LPCWSTR pszTranslateSection, LPCWSTR pszTranslateKey, LPWSTR pszBuffer,
    DWORD dwBufferSize, PDWORD pdwRequiredSize, PVOID pvReserved);
#define TranslateInfStringEx WINELIB_NAME_AW(TranslateInfStringEx)
HRESULT WINAPI UserInstStubWrapperA(HWND hWnd, HINSTANCE hInstance, LPSTR pszParms, INT nShow);
HRESULT WINAPI UserInstStubWrapperW(HWND hWnd, HINSTANCE hInstance, LPWSTR pszParms, INT nShow);
#define UserInstStubWrapper WINELIB_NAME_AW(UserInstStubWrapper)
HRESULT WINAPI UserUnInstStubWrapperA(HWND hWnd, HINSTANCE hInstance, LPSTR pszParms, INT nShow);
HRESULT WINAPI UserUnInstStubWrapperW(HWND hWnd, HINSTANCE hInstance, LPWSTR pszParms, INT nShow);
#define UserUnInstStubWrapper WINELIB_NAME_AW(UserUnInstStubWrapper)

#ifdef __cplusplus
}
#endif

#endif /* __WINE_ADVPUB_H */
