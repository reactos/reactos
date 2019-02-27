/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     zip pidl handling
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

LPITEMIDLIST _ILCreate(ZipPidlType Type, LPCSTR lpString, unz_file_info64& info)
{
    int cbData = sizeof(ZipPidlEntry) + strlen(lpString);
    ZipPidlEntry* pidl = (ZipPidlEntry*)SHAlloc(cbData + sizeof(WORD));
    if (!pidl)
        return NULL;

    ZeroMemory(pidl, cbData + sizeof(WORD));

    pidl->cb = cbData;
    pidl->MagicType = 'z';
    pidl->ZipType = Type;

    if (Type != ZIP_PIDL_DIRECTORY)
    {
        pidl->CompressedSize = info.compressed_size;
        pidl->UncompressedSize = info.uncompressed_size;
        pidl->DosDate = info.dosDate;
        pidl->Password = info.flag & MINIZIP_PASSWORD_FLAG;
    }

    strcpy(pidl->Name, lpString);
    *(WORD*)((char*)pidl + cbData) = 0;

    return (LPITEMIDLIST)pidl;
}


const ZipPidlEntry* _ZipFromIL(LPCITEMIDLIST pidl)
{
    const ZipPidlEntry* zipPidl = (const ZipPidlEntry*)pidl;
    if (zipPidl->MagicType == 'z')
        return zipPidl;
    return NULL;
}
