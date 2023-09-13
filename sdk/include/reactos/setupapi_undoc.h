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

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __SETUPAPI_UNDOC_H
