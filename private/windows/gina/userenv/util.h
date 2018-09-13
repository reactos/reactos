//*************************************************************
//
//  Header file for Util.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************


#define FreeProducedString(psz) if((psz) != NULL) {LocalFree(psz);} else

LPWSTR ProduceWFromA(LPCSTR pszA);
LPSTR ProduceAFromW(LPCWSTR pszW);
LPTSTR CheckSlash (LPTSTR lpDir);
LPTSTR CheckSemicolon (LPTSTR lpDir);
BOOL Delnode (LPTSTR lpDir);
UINT CreateNestedDirectory(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
BOOL GetProfilesDirectoryEx(LPTSTR lpProfilesDir, LPDWORD lpcchSize, BOOL bExpand);
BOOL GetDefaultUserProfileDirectoryEx (LPTSTR lpProfileDir, LPDWORD lpcchSize, BOOL bExpand);
BOOL GetAllUsersProfileDirectoryEx (LPTSTR lpProfileDir, LPDWORD lpcchSize, BOOL bExpand);
int StringToInt(LPTSTR lpNum);
BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey);
VOID DeleteAllValues(HKEY hKey);
BOOL MakeFileSecure (LPTSTR lpFile, DWORD dwOtherSids);
BOOL GetSpecialFolderPath (INT csidl, LPTSTR lpPath);
BOOL GetFolderPath (INT csidl, HANDLE hToken, LPTSTR lpPath);
BOOL SetFolderPath (INT csidl, HANDLE hToken, LPTSTR lpPath);
void CenterWindow (HWND hwnd);
BOOL UnExpandSysRoot(LPCTSTR lpFile, LPTSTR lpResult);
LPTSTR AllocAndExpandEnvironmentStrings(LPCTSTR lpszSrc);
void IntToString( INT i, LPTSTR sz);
BOOL IsUserAGuest(HANDLE hToken);
BOOL IsUserAnAdminMember(HANDLE hToken);
BOOL MakeRegKeySecure(HANDLE hToken, HKEY hKeyRoot, LPTSTR lpKeyName);
BOOL FlushSpecialFolderCache (void);
BOOL CheckForVerbosePolicy (void);
int ExtractCSIDL(LPCTSTR pcszPath, LPTSTR* ppszUsualPath);
LPTSTR MyGetDomainName (VOID);
LPTSTR MyGetUserName (EXTENDED_NAME_FORMAT  NameFormat);
LPTSTR MyGetComputerName (EXTENDED_NAME_FORMAT  NameFormat);
void StringToGuid( TCHAR *szValue, GUID *pGuid );
void GuidToString( GUID *pGuid, TCHAR * szValue );
BOOL ValidateGuid( TCHAR *szValue );
INT CompareGuid( GUID *pGuid1, GUID *pGuid2 );
BOOL ImpersonateUser (HANDLE hNewUser, HANDLE *hOldUser);
BOOL RevertToUser (HANDLE *hUser);
BOOL RegCleanUpValue (HKEY hKeyRoot, LPTSTR lpSubKey, LPTSTR lpValueName);
BOOL CreateSecureAdminDirectory (LPTSTR lpDirectory, DWORD dwOtherSids);
BOOL AddPowerUserAce (LPTSTR lpFile);
void InitializePingCritSec(void);
void ClosePingCritSec(void);
DWORD PingComputer (IPAddr ipaddr, ULONG *ulSpeed);
PISECURITY_DESCRIPTOR MakeGenericSecurityDesc();
LPTSTR GetUserGuid(HANDLE hToken);
LPTSTR GetOldSidString(HANDLE hToken, LPTSTR lpKeyName);
BOOL SetOldSidString(HANDLE hToken, LPTSTR lpSidString, LPTSTR lpKeyName);
LPTSTR GetErrString(DWORD dwErr, LPTSTR szErr);
LONG RegRenameKey(HKEY hKeyRoot, LPTSTR lpSrcKey, LPTSTR lpDestKey);
BOOL IsNullGUID (GUID *pguid);
BOOL GetMachineRole (LPINT piRole);

//
// Flags used to specify additional that needs to be present in ACEs
//
#define OTHERSIDS_EVERYONE             1
#define OTHERSIDS_POWERUSERS           2
