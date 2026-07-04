/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtGetCurrentProcessorNumberEx
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <winnls.h>

typedef
NTSTATUS
NTAPI
FN_NtGetCurrentProcessorNumberEx(
    _Out_ PPROCESSOR_NUMBER);

FN_NtGetCurrentProcessorNumberEx* pNtGetCurrentProcessorNumberEx;

START_TEST(NtGetCurrentProcessorNumberEx)
{
    PROCESSOR_NUMBER ProcessorNumber;
    NTSTATUS Status;

    HINSTANCE hinstNtdll = GetModuleHandleA("ntdll.dll");
    pNtGetCurrentProcessorNumberEx = (FN_NtGetCurrentProcessorNumberEx*)GetProcAddress(hinstNtdll, "NtGetCurrentProcessorNumberEx");
    if (!pNtGetCurrentProcessorNumberEx)
    {
        skip("NtGetCurrentProcessorNumberEx is not available\n");
        return;
    }

    memset(&ProcessorNumber, 0xAA, sizeof(ProcessorNumber));
    Status = pNtGetCurrentProcessorNumberEx(&ProcessorNumber);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(ProcessorNumber.Group < 64, "Processor group is out of range");
    ok(ProcessorNumber.Number < 64, "Processor number is out of range");
    ok(ProcessorNumber.Reserved == 0, "Reserved field is not zero");

    Status = pNtGetCurrentProcessorNumberEx(NULL);
    ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);

    Status = pNtGetCurrentProcessorNumberEx((PVOID)(ULONG_PTR)0xDEADDEADDEADDEADull);
    ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);
}
