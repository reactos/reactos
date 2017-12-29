/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     zip pidl handling
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */


enum ZipPidlType
{
    ZIP_PIDL_DIRECTORY,
    ZIP_PIDL_FILE
};

#include <pshpack1.h>
struct ZipPidlEntry
{
    WORD cb;
    BYTE MagicType;
    ZipPidlType ZipType;

    ULONG64 CompressedSize;
    ULONG64 UncompressedSize;
    ULONG DosDate;
    BYTE Password;

    char Name[1];
};
#include <poppack.h>


LPITEMIDLIST _ILCreate(ZipPidlType Type, LPCSTR lpString, unz_file_info64& info);
const ZipPidlEntry* _ZipFromIL(LPCITEMIDLIST pidl);

