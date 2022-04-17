#ifndef _NTDLL_APITEST_PRECOMP_H_
#define _NTDLL_APITEST_PRECOMP_H_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <apitest.h>
#include <apitest_guard.h>
#include <ndk/ntndk.h>
#include <strsafe.h>

/* probelib.c */
typedef enum _ALIGNMENT_PROBE_MODE
{
    QUERY,
    SET
} ALIGNMENT_PROBE_MODE;

VOID
QuerySetProcessValidator(
    _In_ ALIGNMENT_PROBE_MODE ValidationMode,
    _In_ ULONG InfoClassIndex,
    _In_ PVOID InfoPointer,
    _In_ ULONG InfoLength,
    _In_ NTSTATUS ExpectedStatus);

VOID
QuerySetThreadValidator(
    _In_ ALIGNMENT_PROBE_MODE ValidationMode,
    _In_ ULONG InfoClassIndex,
    _In_ PVOID InfoPointer,
    _In_ ULONG InfoLength,
    _In_ NTSTATUS ExpectedStatus);

void
SetupLocale(
    _In_ ULONG AnsiCode,
    _In_ ULONG OemCode,
    _In_ ULONG Unicode);

#define ConvertPrivLongToLuid(PrivilegeVal, ConvertedPrivLuid) \
do {                                                           \
    LUID Luid;                                                 \
    Luid.LowPart = PrivilegeVal;                               \
    Luid.HighPart = 0;                                         \
    *ConvertedPrivLuid = Luid;                                 \
} while (0)

#endif /* _NTDLL_APITEST_PRECOMP_H_ */
