/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Multiple keyboard table switcher for Japanese and Korean
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <winreg.h>
#include <wchar.h>
#include <ndk/ntndk.h>
#include <ndk/kbd.h>
#include <ntstrsafe.h>

/* KBDTABLE_DESC.dwType */
#define KBD_TYPE_IBM_ENHANCED 4
#define KBD_TYPE_JAPANESE     7
#define KBD_TYPE_KOREAN       8

/* KBDTABLE_DESC.dwSubType (values seen in kbdjpn.dll / kbdkor.dll) */
#define KBD_SUBTYPE_ENGLISH_101  0
#define KBD_SUBTYPE_JAPANESE_106 2
#define KBD_SUBTYPE_KOREAN_103   6
#define KBD_SUBTYPE_JAPANESE_NEC 0x0D02

typedef struct tagCLIENTKEYBOARDTYPE
{
    ULONG Type;
    ULONG SubType;
    ULONG FunctionKey;
} CLIENTKEYBOARDTYPE, *PCLIENTKEYBOARDTYPE;

#define I8042PRT_PARAMS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\i8042prt\\Parameters"
#define TS_KBDTYPE_MAPPING \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Terminal Server\\KeyboardType Mapping\\"
#define KBD_DYNAMIC_TABLES_KEY \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Keyboard Layout\\Dynamic Tables\\"

/* Subkey under "Dynamic Tables". NOTE: the original kbdkor.dll also uses "kbdjpn". */
#define KBD_DYNAMIC_TABLES_NAME L"kbdjpn"

/* Reads REG_SZ value pwszValueName under pwszKeyPath into pszOut */
static BOOL
QueryRealDllName(
    _In_ PCWSTR pwszKeyPath,
    _In_ PCWSTR pwszValueName,
    _Out_writes_z_(cchOut) PWSTR pszOut,
    _In_ SIZE_T cchOut)
{
    UNICODE_STRING KeyName, ValueName;
    OBJECT_ATTRIBUTES oa;
    HANDLE hKey;
    NTSTATUS Status;
    ULONG cbResult;
    struct { KEY_VALUE_PARTIAL_INFORMATION; WCHAR Extra[MAX_PATH - 1]; } ValueBuffer;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo = (PVOID)&ValueBuffer;
    const SIZE_T cbHeader = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    if (!cchOut)
        return FALSE;

    RtlInitUnicodeString(&KeyName, pwszKeyPath);
    InitializeObjectAttributes(&oa, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenKey(&hKey, KEY_QUERY_VALUE, &oa);
    if (!NT_SUCCESS(Status))
        return FALSE;

    RtlInitUnicodeString(&ValueName, pwszValueName);
    cbResult = 0;
    Status = NtQueryValueKey(hKey, &ValueName, KeyValuePartialInformation,
                             &ValueBuffer, sizeof(ValueBuffer), &cbResult);
    NtClose(hKey);
    if (!NT_SUCCESS(Status))
        return FALSE;

    if (cbResult < cbHeader || pInfo->Type != REG_SZ || !pInfo->DataLength ||
        pInfo->DataLength > cbResult - cbHeader || (pInfo->DataLength % sizeof(WCHAR)))
    {
        return FALSE;
    }

    Status = RtlStringCbCopyNW(pszOut, cchOut * sizeof(WCHAR), (PCWSTR)pInfo->Data,
                               pInfo->DataLength);
    return NT_SUCCESS(Status) && pszOut[0] != UNICODE_NULL;
}

/* Terminal Server (Hydra) path: pClientKbdType != NULL */
static BOOL
KbdLayerRealDllFileForWBT(
    _In_ HKL hKL,
    _Out_writes_z_(cchRealDllName) PWSTR pRealDllName,
    _In_ SIZE_T cchRealDllName,
    _In_ PCLIENTKEYBOARDTYPE pClientKbdType)
{
    WCHAR wszKey[MAX_PATH], wszValue[8 + 4 + 1];
    WORD wLang = PRIMARYLANGID(HandleToUlong(hKL));
    NTSTATUS Status;

    /* Build the registry key */
    if (wLang == LANG_JAPANESE)
        Status = RtlStringCbCopyW(wszKey, sizeof(wszKey), TS_KBDTYPE_MAPPING L"JPN");
    else if (wLang == LANG_KOREAN)
        Status = RtlStringCbCopyW(wszKey, sizeof(wszKey), TS_KBDTYPE_MAPPING L"KOR");
    else
        return FALSE;

    if (!NT_SUCCESS(Status))
        return FALSE;

    /* Build the value name */
    Status = RtlStringCbPrintfW(wszValue, sizeof(wszValue), L"%08X%04u",
                                pClientKbdType->SubType, pClientKbdType->FunctionKey);
    if (!NT_SUCCESS(Status))
        return FALSE;

    /* Read from registry */
    if (QueryRealDllName(wszKey, wszValue, pRealDllName, cchRealDllName))
        return TRUE;

    /* Retry with truncated value name */
    wszValue[8] = UNICODE_NULL;
    if (QueryRealDllName(wszKey, wszValue, pRealDllName, cchRealDllName))
        return TRUE;

    /* Store the default value */
    if (wLang == LANG_JAPANESE)
        Status = RtlStringCchCopyW(pRealDllName, cchRealDllName, L"kbd101.dll");
    else /* LANG_KOREAN */
        Status = RtlStringCchCopyW(pRealDllName, cchRealDllName, L"kbd101a.dll");
    return NT_SUCCESS(Status);
}

/* ------------------------------------------------------------------------
 * KbdLayerRealDllFile @5
 * ------------------------------------------------------------------------ */
BOOL WINAPI
KbdLayerRealDllFile(
    _In_ HKL hKL,
    _Out_writes_z_(MAX_PATH) PWSTR pRealDllName,
    _In_opt_ PCLIENTKEYBOARDTYPE pClientKbdType,
    _In_opt_ PVOID reserved)
{
    WORD wLang;
    PCWSTR suffix;
    WCHAR wszValue[32];
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(reserved);

    if (!pRealDllName)
        return FALSE;
    *pRealDllName = UNICODE_NULL;

    if (pClientKbdType)
        return KbdLayerRealDllFileForWBT(hKL, pRealDllName, MAX_PATH, pClientKbdType);

    wLang = PRIMARYLANGID(HandleToUlong(hKL));
    if (wLang == LANG_JAPANESE)
        suffix = L" JPN";
    else if (wLang == LANG_KOREAN)
        suffix = L" KOR";
    else
        suffix = L"";

    Status = RtlStringCbPrintfW(wszValue, sizeof(wszValue), L"LayerDriver%s", suffix);
    if (!NT_SUCCESS(Status))
        return FALSE;

    return QueryRealDllName(I8042PRT_PARAMS, wszValue, pRealDllName, MAX_PATH);
}

/* ------------------------------------------------------------------------
 * KbdLayerRealDllFileNT4 @3
 * ------------------------------------------------------------------------ */
BOOL WINAPI
KbdLayerRealDllFileNT4(
    _Out_writes_z_(MAX_PATH) PWSTR pRealDllName)
{
    if (!pRealDllName)
        return FALSE;
    *pRealDllName = UNICODE_NULL;
    return QueryRealDllName(I8042PRT_PARAMS, L"LayerDriver", pRealDllName, MAX_PATH);
}

static BOOL
ParseDynamicTableEntry(
    _Out_ PKBDTABLE_MULTI pMulti,
    _In_reads_bytes_(cbInfo) PKEY_VALUE_FULL_INFORMATION pInfo,
    _In_ ULONG cbInfo)
{
    PKBDTABLE_DESC pDesc = &pMulti->aKbdTables[pMulti->nTables];
    const DWORD *pdwData;
    PWCHAR pch;

    if (cbInfo < sizeof(*pInfo) || pInfo->Type != REG_BINARY || !pInfo->NameLength ||
        FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) + pInfo->NameLength > cbInfo ||
        pInfo->NameLength >= sizeof(pDesc->wszDllName) || pInfo->NameLength % sizeof(WCHAR) ||
        pInfo->DataLength != 3 * sizeof(DWORD) || pInfo->DataOffset > cbInfo ||
        cbInfo - pInfo->DataOffset < pInfo->DataLength)
    {
        return FALSE;
    }

    /* Make wszDllName NUL-terminated */
    RtlCopyMemory(pDesc->wszDllName, pInfo->Name, pInfo->NameLength);
    pDesc->wszDllName[pInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;

    /* "kbd106.dll,something" -> "kbd106.dll" */
    pch = wcschr(pDesc->wszDllName, L',');
    if (pch)
        *pch = UNICODE_NULL;

    /* Get the descriptor from the registry data */
    pdwData = (const DWORD *)((PBYTE)pInfo + pInfo->DataOffset);
    if (pdwData[0])
        return FALSE;
    pDesc->dwType = pdwData[1];
    pDesc->dwSubType = pdwData[2];
    return TRUE;
}

/* TRUE if at least one table was read. A malformed entry discards everything. */
static BOOL
LoadDynamicTables(
    _In_z_ PCWSTR pwszName,
    _Out_ PKBDTABLE_MULTI pMulti)
{
    WCHAR wszPath[MAX_PATH];
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES oa;
    HANDLE hKey;
    NTSTATUS Status;
    ULONG cbResult;
    struct { KEY_VALUE_FULL_INFORMATION; WCHAR Extra[MAX_PATH - 1]; } ValueBuffer;
    PKEY_VALUE_FULL_INFORMATION pInfo = (PVOID)&ValueBuffer;

    RtlZeroMemory(pMulti, sizeof(*pMulti));
    Status = RtlStringCbPrintfW(wszPath, sizeof(wszPath), L"%s%s", KBD_DYNAMIC_TABLES_KEY, pwszName);
    if (!NT_SUCCESS(Status))
        return FALSE;

    RtlInitUnicodeString(&KeyName, wszPath);
    InitializeObjectAttributes(&oa, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenKey(&hKey, KEY_READ, &oa);
    if (!NT_SUCCESS(Status))
        return FALSE;

    for (pMulti->nTables = 0; pMulti->nTables < KBDTABLE_MULTI_MAX; ++pMulti->nTables)
    {
        cbResult = 0;
        Status = NtEnumerateValueKey(hKey, pMulti->nTables, KeyValueFullInformation,
                                     &ValueBuffer, sizeof(ValueBuffer), &cbResult);
        if (Status == STATUS_NO_MORE_ENTRIES)
            break;

        if (!NT_SUCCESS(Status) || !ParseDynamicTableEntry(pMulti, pInfo, cbResult))
        {
            pMulti->nTables = 0;
            break;
        }
    }

    NtClose(hKey);
    return pMulti->nTables > 0;
}

static const struct { PCWSTR pwszDll; DWORD dwType, dwSubType; } s_Defaults[] =
{
#ifdef KBD_JAPANESE /* kbdjpn.dll */
    { L"kbd101.dll",  KBD_TYPE_IBM_ENHANCED, KBD_SUBTYPE_ENGLISH_101 },
    { L"kbd106.dll",  KBD_TYPE_JAPANESE,     KBD_SUBTYPE_JAPANESE_106 },
    { L"kbdnec.dll",  KBD_TYPE_JAPANESE,     KBD_SUBTYPE_JAPANESE_NEC },
#elif defined(KBD_KOREAN) /* kbdkor.dll */
    { L"kbd101a.dll", KBD_TYPE_IBM_ENHANCED, KBD_SUBTYPE_ENGLISH_101 },
    { L"kbd103.dll",  KBD_TYPE_KOREAN,       KBD_SUBTYPE_KOREAN_103 },
#else
    #error Something is wrong.
#endif
};
C_ASSERT(RTL_NUMBER_OF(s_Defaults) <= KBDTABLE_MULTI_MAX);

static VOID
SetDefaultTables(
    _Out_ PKBDTABLE_MULTI pMulti)
{
    SIZE_T iTable;
    RtlZeroMemory(pMulti, sizeof(*pMulti));
    pMulti->nTables = RTL_NUMBER_OF(s_Defaults);
    for (iTable = 0; iTable < RTL_NUMBER_OF(s_Defaults); ++iTable)
    {
        RtlStringCbCopyW(pMulti->aKbdTables[iTable].wszDllName,
                         sizeof(pMulti->aKbdTables[iTable].wszDllName), 
                         s_Defaults[iTable].pwszDll);
        pMulti->aKbdTables[iTable].dwType = s_Defaults[iTable].dwType;
        pMulti->aKbdTables[iTable].dwSubType = s_Defaults[iTable].dwSubType;
    }
}

/* ------------------------------------------------------------------------
 * KbdLayerMultiDescriptor @6
 * ---------------------------------------------------------------------- */
BOOL WINAPI
KbdLayerMultiDescriptor(
    _Out_ PKBDTABLE_MULTI pMulti)
{
    if (!pMulti)
        return FALSE;
    if (!LoadDynamicTables(KBD_DYNAMIC_TABLES_NAME, pMulti))
        SetDefaultTables(pMulti);
    return TRUE;
}
