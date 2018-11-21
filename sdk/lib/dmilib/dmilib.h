/*
 * PROJECT:     ReactOS DMI/SMBIOS Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dmilib.h
 * PURPOSE:     SMBIOS table parsing functions
 * PROGRAMMERS: Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#ifndef DMILIB_H
#define DMILIB_H

enum _ID_STRINGS
{
    ID_NONE = 0,
    BIOS_VENDOR,
    BIOS_VERSION,
    BIOS_DATE,
    SYS_VENDOR,
    SYS_PRODUCT,
    SYS_VERSION,
    SYS_SERIAL,
    SYS_SKU,
    SYS_FAMILY,
    BOARD_VENDOR,
    BOARD_NAME,
    BOARD_VERSION,
    BOARD_SERIAL,
    BOARD_ASSET_TAG,

    ID_STRINGS_MAX,
};

VOID
ParseSMBiosTables(
    _In_reads_bytes_(TableSize) PVOID SMBiosTables,
    _In_ ULONG TableSize,
    _Inout_updates_(ID_STRINGS_MAX) PCHAR * Strings);

#endif /* DMILIB_H */
