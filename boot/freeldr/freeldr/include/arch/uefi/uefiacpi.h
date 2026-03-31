/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ACPI table definitions for UEFI FreeLDR
 */

#pragma once

#include <ntldr/winldr.h>

#define BGRT_SIGNATURE           0x54524742  /* "BGRT" */
#define BGRT_STATUS_IMAGE_VALID  0x01
#define BGRT_IMAGE_TYPE_BITMAP   0

#include <pshpack1.h>

typedef struct _DESCRIPTION_HEADER
{
    ULONG Signature;
    ULONG Length;
    UCHAR Revision;
    UCHAR Checksum;
    UCHAR OEMID[6];
    UCHAR OEMTableID[8];
    ULONG OEMRevision;
    UCHAR CreatorID[4];
    ULONG CreatorRev;
} DESCRIPTION_HEADER, *PDESCRIPTION_HEADER;

typedef struct _RSDT
{
    DESCRIPTION_HEADER Header;
    ULONG Tables[ANYSIZE_ARRAY];
} RSDT, *PRSDT;

typedef struct _XSDT
{
    DESCRIPTION_HEADER Header;
    PHYSICAL_ADDRESS Tables[ANYSIZE_ARRAY];
} XSDT, *PXSDT;

typedef struct _BGRT_TABLE
{
    DESCRIPTION_HEADER Header;
    USHORT Version;
    UCHAR Status;
    UCHAR ImageType;
    ULONGLONG LogoAddress;
    ULONG OffsetX;
    ULONG OffsetY;
} BGRT_TABLE, *PBGRT_TABLE;

#include <poppack.h>
