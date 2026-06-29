/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Windows-compatible NT OS Loader.
 * COPYRIGHT:   Copyright 2006-2019 Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

#include <arc/setupblk.h>

// See freeldr/ntldr/winldr.h
#define TAG_WLDR_DTE 'eDlW'
#define TAG_WLDR_BDE 'dBlW'
#define TAG_WLDR_NAME 'mNlW'

// Some definitions

#include <pshpack1.h>
typedef struct  /* Root System Descriptor Pointer */
{
    CHAR             signature [8];          /* contains "RSD PTR " */
    UCHAR            checksum;               /* to make sum of struct == 0 */
    CHAR             oem_id [6];             /* OEM identification */
    UCHAR            revision;               /* Must be 0 for 1.0, 2 for 2.0 */
    ULONG            rsdt_physical_address;  /* 32-bit physical address of RSDT */
    ULONG            length;                 /* XSDT Length in bytes including hdr */
    ULONGLONG        xsdt_physical_address;  /* 64-bit physical address of XSDT */
    UCHAR            extended_checksum;      /* Checksum of entire table */
    CHAR             reserved [3];           /* reserved field must be 0 */
} RSDP_DESCRIPTOR, *PRSDP_DESCRIPTOR;
#include <poppack.h>

typedef struct _ARC_DISK_SIGNATURE_EX
{
    ARC_DISK_SIGNATURE DiskSignature;
    CHAR ArcName[MAX_PATH];
} ARC_DISK_SIGNATURE_EX, *PARC_DISK_SIGNATURE_EX;

////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Loading Functions
//
////////////////////////////////////////////////////////////////////////////////

/* The boot options are mutually exclusive */
enum SAFEBOOT_MODE
{
    NO_OPTION = 0,

    SAFEBOOT,
    SAFEBOOT_NETWORK,
    SAFEBOOT_ALTSHELL,
    SAFEBOOT_DSREPAIR,

    LKG_CONFIG, // TODO: Make it exclusive? Or allow it to be combined with SafeBoot?
};
extern enum SAFEBOOT_MODE BootOptionChoice;

#if DBG && defined(_M_IX86) // x86 *ONLY*: HAL/Kernel auto-detection override
#define BOOT_AUTODETECT 0
#define BOOT_ACPI_APIC  1
#define BOOT_ACPI_SMP   2
extern UCHAR HALAutoDetectMode;
#endif

#define BOOT_LOGGING    (1 << 0)
#define BOOT_VGA_MODE   (1 << 1)
#define BOOT_DEBUGGING  (1 << 2)
extern LOGICAL BootFlags;

VOID
MenuNTOptions(
    _Inout_ OperatingSystemItem* OperatingSystem);

VOID
AppendBootTimeOptions(
    _Inout_z_bytecount_(BootOptionsSize)
         PSTR BootOptions,
    _In_ SIZE_T BootOptionsSize);


ARC_STATUS
LoadAndBootWindows(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[]);

ARC_STATUS
LoadReactOSSetup(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[]);


// conversion.c and conversion.h
PVOID VaToPa(PVOID Va);
PVOID PaToVa(PVOID Pa);
VOID List_PaToVa(_In_ LIST_ENTRY *ListEntry);
