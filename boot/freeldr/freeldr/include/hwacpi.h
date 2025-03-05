/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Define ACPI Structures
 * COPYRIGHT:   Copyright 2006-2019 Aleksey Bragin <aleksey@reactos.org>
 *              Copyright 2024 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

#pragma once

#include <pshpack1.h>

typedef struct /* ACPI Description Header */
{
    CHAR Signature[4];
    ULONG Length;
    UCHAR Revision;
    UCHAR Checksum;
    CHAR OEMID[6];
    CHAR OEMTableID[8];
    ULONG OEMRevision;
    ULONG CreatorID;
    ULONG CreatorRev;
} DESCRIPTION_HEADER, *PDESCRIPTION_HEADER;

typedef struct /* Root System Descriptor Table */
{
    DESCRIPTION_HEADER Header;
    ULONG PointerToOtherSDT[];
} RSDT_DESCRIPTOR, *PRSDT_DESCRIPTOR;

typedef struct /* eXtended System Descriptor Table */
{
    DESCRIPTION_HEADER Header;
    ULONGLONG PointerToOtherSDT[];
} XSDT_DESCRIPTOR, *PXSDT_DESCRIPTOR;

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
