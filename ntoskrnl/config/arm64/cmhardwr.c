/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Configuration Manager, ARM64 processor description
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static
NTSTATUS
CmpCreateVolatileKey(
    _In_ PCWSTR Path,
    _Out_ PHANDLE KeyHandle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    ULONG Disposition;

    RtlInitUnicodeString(&KeyName, Path);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    return ZwCreateKey(KeyHandle,
                       KEY_READ | KEY_WRITE,
                       &ObjectAttributes,
                       0,
                       NULL,
                       REG_OPTION_VOLATILE,
                       &Disposition);
}

static
NTSTATUS
CmpCreateProcessorKey(
    _In_ PCWSTR Class,
    _In_ ULONG Number,
    _Out_ PHANDLE KeyHandle)
{
    WCHAR Buffer[96];
    HANDLE ClassHandle;
    NTSTATUS Status;

    /* The loader does not describe the processors, so the class key may be missing */
    Status = RtlStringCbPrintfW(Buffer,
                                sizeof(Buffer),
                                L"\\Registry\\Machine\\Hardware\\Description\\System\\%s",
                                Class);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = CmpCreateVolatileKey(Buffer, &ClassHandle);
    if (!NT_SUCCESS(Status))
        return Status;
    ZwClose(ClassHandle);

    Status = RtlStringCbPrintfW(Buffer,
                                sizeof(Buffer),
                                L"\\Registry\\Machine\\Hardware\\Description\\System\\%s\\%lu",
                                Class,
                                Number);
    if (!NT_SUCCESS(Status))
        return Status;

    return CmpCreateVolatileKey(Buffer, KeyHandle);
}

static
VOID
CmpSetStringValue(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value)
{
    UNICODE_STRING ValueName;

    RtlInitUnicodeString(&ValueName, Name);
    ZwSetValueKey(KeyHandle,
                  &ValueName,
                  0,
                  REG_SZ,
                  (PVOID)Value,
                  (ULONG)(wcslen(Value) + 1) * sizeof(WCHAR));
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
CmpInitializeMachineDependentConfiguration(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNICODE_STRING ValueName;
    WCHAR Identifier[64];
    HANDLE KeyHandle;
    PKPRCB Prcb;
    ULONG64 Midr;
    ULONG i, Data;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(LoaderBlock);

    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        Prcb = KiProcessorBlock[i];
        Midr = Prcb->ProcessorState.ArchState.Midr_El1;

        /* MIDR_EL1 splits into implementer, architecture, part number and revision */
        RtlStringCbPrintfW(Identifier,
                           sizeof(Identifier),
                           L"ARMv8 (64-bit) Family 8 Model %X Revision %3X",
                           (ULONG)((Midr >> 4) & 0xFFF),
                           (ULONG)(((Midr >> 20) & 0xF) << 8 | (Midr & 0xF)));

        Status = CmpCreateProcessorKey(L"CentralProcessor", i, &KeyHandle);
        if (!NT_SUCCESS(Status))
            return Status;

        CmpSetStringValue(KeyHandle, L"Identifier", Identifier);
        CmpSetStringValue(KeyHandle, L"ProcessorNameString", L"ARM64 Processor");

        RtlStringCbPrintfW(Identifier, sizeof(Identifier), L"CP %02X", (ULONG)((Midr >> 24) & 0xFF));
        CmpSetStringValue(KeyHandle, L"VendorIdentifier", Identifier);

        Data = Prcb->MHz;
        RtlInitUnicodeString(&ValueName, L"~MHz");
        ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Data, sizeof(Data));

        Data = Prcb->FeatureBits;
        RtlInitUnicodeString(&ValueName, L"FeatureSet");
        ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Data, sizeof(Data));
        ZwClose(KeyHandle);

        Status = CmpCreateProcessorKey(L"FloatingPointProcessor", i, &KeyHandle);
        if (NT_SUCCESS(Status))
        {
            CmpSetStringValue(KeyHandle, L"Identifier", L"ARMv8 Advanced SIMD");
            ZwClose(KeyHandle);
        }
    }

    return STATUS_SUCCESS;
}
