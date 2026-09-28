/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     The resource-translation interface ACPI and the Resource Hub
 *              exchange, so that connection resources can be translated
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */


#pragma once

#define IOCTL_UACPINT_RH_QUERY_TRANSLATION \
    CTL_CODE(FILE_DEVICE_BUS_EXTENDER,       \
             0xA,                            \
             METHOD_BUFFERED,                \
             FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define UACPINT_RH_TRANSLATION_VERSION 1

typedef struct _UACPINT_RH_TRANSLATION_RESULT
{
    ULONG Count;
    ULONG Reserved;
    IO_RESOURCE_DESCRIPTOR Descriptor;
    ULONG ScopedNameOffset;
    ULONG ScopedNameLength;
    CHAR ReferenceName[ANYSIZE_ARRAY];
} UACPINT_RH_TRANSLATION_RESULT, *PUACPINT_RH_TRANSLATION_RESULT;

#define UACPINT_RH_TRANSLATION_RESULT_MIN_SIZE 56

/* ACPI -> hub */

typedef VOID
(NTAPI *PUACPINT_RH_RELEASE)(
    _In_ PVOID Context);

typedef NTSTATUS
(NTAPI *PUACPINT_RH_QUALIFY_BIOS_NAME)(
    _In_ PDEVICE_OBJECT ScopeDevice,
    _In_ PSTRING ReferenceName,
    _Out_ PUNICODE_STRING QualifiedName,
    _Out_ PULONG StringLength);

typedef NTSTATUS
(NTAPI *PUACPINT_RH_ALLOCATE_SECONDARY_GSIV)(
    _In_ PCHAR DescriptorName,
    _In_ ULONG DescriptorNameSize,
    _Out_ PULONG Gsiv);

typedef NTSTATUS
(NTAPI *PUACPINT_RH_SET_INTERRUPT_PROPERTIES)(
    _In_ ULONG Gsiv,
    _In_ KINTERRUPT_MODE Mode,
    _In_ KINTERRUPT_POLARITY Polarity);

/* hub -> ACPI */

typedef NTSTATUS
(NTAPI *PUACPINT_RH_TRANSLATE_DESCRIPTOR)(
    _In_ PVOID Context,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_reads_bytes_(DescriptorLength) PVOID Descriptor,
    _In_ ULONG DescriptorLength,
    _In_ ULONG Flags,
    _Out_ PUACPINT_RH_TRANSLATION_RESULT Result,
    _Inout_ PULONG ResultSize);

typedef NTSTATUS
(NTAPI *PUACPINT_RH_ASSOCIATE_BIOS_NAME)(
    _In_ PVOID Context,
    _In_ PCUNICODE_STRING ReferenceName,
    _In_ PDEVICE_OBJECT DeviceObject);

typedef NTSTATUS
(NTAPI *PUACPINT_RH_QUERY_GSIV_DESCRIPTOR)(
    _In_ ULONG Gsiv,
    _In_ ULONG Flags,
    _Out_ PIO_RESOURCE_DESCRIPTOR IoDescriptor);

typedef struct _UACPINT_RH_TRANSLATION_INTERFACE
{
    INTERFACE Interface;

    /* Filled in by ACPI before the IOCTL */
    PVOID HostContext;
    PUACPINT_RH_RELEASE ReleaseInterface;
    PUACPINT_RH_QUALIFY_BIOS_NAME QualifyBiosName;
    PUACPINT_RH_ALLOCATE_SECONDARY_GSIV AllocateSecondaryGsiv;
    PUACPINT_RH_SET_INTERRUPT_PROPERTIES SetInterruptProperties;

    /* Filled in by the hub before the IOCTL completes */
    PVOID Context;
    PUACPINT_RH_TRANSLATE_DESCRIPTOR TranslateDescriptor;
    PUACPINT_RH_ASSOCIATE_BIOS_NAME AssociateBiosName;
    PUACPINT_RH_QUERY_GSIV_DESCRIPTOR QueryGsivDescriptor;
} UACPINT_RH_TRANSLATION_INTERFACE, *PUACPINT_RH_TRANSLATION_INTERFACE;
