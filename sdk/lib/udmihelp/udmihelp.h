/*
 * PROJECT:     ReactOS User-mode DMI/SMBIOS Helper Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     SMBIOS table parsing functions
 * COPYRIGHT:   Copyright 2018 Stanislav Motylkov
 */

#ifndef UDMIHELP_H
#define UDMIHELP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <../dmilib/dmilib.h>

PVOID
LoadSMBiosData(
    _Inout_updates_(ID_STRINGS_MAX) PCHAR * Strings);

VOID
TrimDmiStringW(
    _Inout_ PWSTR pStr);

SIZE_T
GetSMBiosStringW(
    _In_ PCSTR DmiString,
    _Out_ PWSTR pBuf,
    _In_ DWORD cchBuf,
    _In_ BOOL bTrim);

VOID
FreeSMBiosData(
    _In_ PVOID Buffer);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UDMIHELP_H */
