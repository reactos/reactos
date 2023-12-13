/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for setupapi.dll
 * COPYRIGHT:   Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#ifndef __SETUPAPI_UNDOC_H
#define __SETUPAPI_UNDOC_H

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// typedef PVOID HSTRING_TABLE;
DECLARE_HANDLE(HSTRING_TABLE);

/* Flags for StringTableAddString and StringTableLookUpString */
#define ST_CASE_SENSITIVE_COMPARE   0x00000001

WINSETUPAPI DWORD  WINAPI pSetupStringTableAddString(HSTRING_TABLE, LPWSTR, DWORD);
WINSETUPAPI DWORD  WINAPI pSetupStringTableAddStringEx(HSTRING_TABLE, LPWSTR, DWORD, LPVOID, DWORD);
WINSETUPAPI VOID   WINAPI pSetupStringTableDestroy(HSTRING_TABLE);
WINSETUPAPI HSTRING_TABLE WINAPI pSetupStringTableDuplicate(HSTRING_TABLE);
WINSETUPAPI BOOL   WINAPI pSetupStringTableGetExtraData(HSTRING_TABLE, DWORD, LPVOID, DWORD);
WINSETUPAPI HSTRING_TABLE WINAPI pSetupStringTableInitialize(VOID);
WINSETUPAPI HSTRING_TABLE WINAPI pSetupStringTableInitializeEx(DWORD, DWORD);
WINSETUPAPI DWORD  WINAPI pSetupStringTableLookUpString(HSTRING_TABLE, LPWSTR, DWORD);
WINSETUPAPI DWORD  WINAPI pSetupStringTableLookUpStringEx(HSTRING_TABLE, LPWSTR, DWORD, LPVOID, DWORD);
WINSETUPAPI BOOL   WINAPI pSetupStringTableSetExtraData(HSTRING_TABLE, DWORD, LPVOID, DWORD);
WINSETUPAPI LPWSTR WINAPI pSetupStringTableStringFromId(HSTRING_TABLE, DWORD);
WINSETUPAPI BOOL   WINAPI pSetupStringTableStringFromIdEx(HSTRING_TABLE, DWORD, LPWSTR, LPDWORD);


WINSETUPAPI LONG WINAPI AddTagToGroupOrderList(PCWSTR lpGroupName, DWORD dwUnknown2, DWORD dwUnknown3); // Not exported
WINSETUPAPI VOID WINAPI AssertFail(LPSTR, UINT, LPSTR);                                                 // Not exported
WINSETUPAPI DWORD WINAPI CaptureStringArg(PCWSTR lpSrc, PWSTR* lpDst);                                  // Not exported
WINSETUPAPI BOOL WINAPI DelayedMove(PCWSTR lpExistingFileName, PCWSTR lpNewFileName);                   // Not exported
WINSETUPAPI BOOL WINAPI DoesUserHavePrivilege(PCWSTR lpPrivilegeName);
WINSETUPAPI PWSTR WINAPI DuplicateString(PCWSTR lpSrc);                                                 // Not exported
WINSETUPAPI BOOL WINAPI EnablePrivilege(PCWSTR lpPrivilegeName, BOOL bEnable);                          // Not exported

WINSETUPAPI BOOL WINAPI FileExists(PCWSTR lpFileName, PWIN32_FIND_DATAW lpFileFindData);                // Not exported
WINSETUPAPI DWORD WINAPI GetSetFileTimestamp(PCWSTR, PFILETIME, PFILETIME, PFILETIME, BOOLEAN);         // Not exported

WINSETUPAPI BOOL WINAPI IsUserAdmin(VOID);
WINSETUPAPI PWSTR WINAPI MultiByteToUnicode(PCSTR lpMultiByteStr, UINT uCodePage);                      // Not exported
WINSETUPAPI VOID WINAPI MyFree(PVOID lpMem);
WINSETUPAPI PVOID WINAPI MyMalloc(DWORD dwSize);
WINSETUPAPI PVOID WINAPI MyRealloc(PVOID lpSrc, DWORD dwSize);
WINSETUPAPI DWORD WINAPI OpenAndMapForRead(PCWSTR, PDWORD, PHANDLE, PHANDLE, PVOID*);                   // Not exported
WINSETUPAPI LONG WINAPI QueryRegistryValue(HKEY, PCWSTR, PBYTE*, PDWORD, PDWORD);                       // Not exported
/* RetreiveFileSecurity is not a typo, as per Microsoft's dlls */
WINSETUPAPI DWORD WINAPI RetreiveFileSecurity(PCWSTR, PSECURITY_DESCRIPTOR*);                           // Not exported
WINSETUPAPI DWORD WINAPI StampFileSecurity(PCWSTR, PSECURITY_DESCRIPTOR);                               // Not exported
WINSETUPAPI DWORD WINAPI TakeOwnershipOfFile(PCWSTR);                                                   // Not exported
WINSETUPAPI PSTR WINAPI UnicodeToMultiByte(PCWSTR lpUnicodeStr, UINT uCodePage);
WINSETUPAPI BOOL WINAPI UnmapAndCloseFile(HANDLE, HANDLE, PVOID);                                       // Not exported


WINSETUPAPI DWORD WINAPI pSetupCaptureAndConvertAnsiArg(PCSTR, PWSTR*);
WINSETUPAPI VOID WINAPI pSetupCenterWindowRelativeToParent(HWND);
WINSETUPAPI BOOL WINAPI pSetupConcatenatePaths(LPWSTR, LPCWSTR, DWORD, LPDWORD);
WINSETUPAPI PWSTR WINAPI pSetupDuplicateString(PCWSTR);
WINSETUPAPI BOOL WINAPI pSetupEnablePrivilege(PCWSTR, BOOL);
WINSETUPAPI PCWSTR WINAPI pSetupGetFileTitle(PCWSTR);
WINSETUPAPI BOOL WINAPI pSetupGetVersionInfoFromImage(LPWSTR, PULARGE_INTEGER, LPWORD);
WINSETUPAPI DWORD WINAPI pSetupGuidFromString(PCWSTR, LPGUID);
WINSETUPAPI BOOL WINAPI pSetupIsGuidNull(LPGUID);
WINSETUPAPI BOOL WINAPI pSetupIsUserAdmin(VOID);
WINSETUPAPI PWSTR WINAPI pSetupMultiByteToUnicode(PCSTR, UINT);
WINSETUPAPI DWORD WINAPI pSetupOpenAndMapForRead(PCWSTR, PDWORD, PHANDLE, PHANDLE, PVOID*);
WINSETUPAPI DWORD WINAPI pSetupStringFromGuid(LPGUID, PWSTR, DWORD);

WINSETUPAPI PSTR WINAPI pSetupUnicodeToMultiByte(PCWSTR lpUnicodeStr, UINT uCodePage);
WINSETUPAPI BOOL WINAPI pSetupUnmapAndCloseFile(HANDLE, HANDLE, PVOID);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __SETUPAPI_UNDOC_H
