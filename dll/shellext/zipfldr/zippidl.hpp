/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     zip pidl handling
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */


enum ZipPidlType
{
    ZIP_PIDL_DIRECTORY,
    ZIP_PIDL_FILE
};

#include <pshpack1.h>
struct ZipPidlEntry
{
    WORD cb; // This must be a WORD to keep compatibility to SHITEMID
    BYTE MagicType;
    BOOLEAN Password;
    ZipPidlType ZipType;

    ULONG64 CompressedSize;
    ULONG64 UncompressedSize;
    ULONG DosDate;

    WCHAR Name[1];
};
#include <poppack.h>


LPITEMIDLIST _ILCreate(ZipPidlType Type, PCWSTR lpString, unz_file_info64& info);
const ZipPidlEntry* _ZipFromIL(LPCITEMIDLIST pidl);
