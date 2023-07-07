/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     zip pidl handling
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

LPITEMIDLIST _ILCreate(ZipPidlType Type, PCWSTR lpString, unz_file_info64& info)
{
    size_t cbData = sizeof(ZipPidlEntry) + wcslen(lpString) * sizeof(WCHAR);
    if (cbData > MAXWORD)
        return NULL;

    ZipPidlEntry* pidl = (ZipPidlEntry*)SHAlloc(cbData + sizeof(WORD));
    if (!pidl)
        return NULL;

    ZeroMemory(pidl, cbData + sizeof(WORD));

    pidl->cb = (WORD)cbData;
    pidl->MagicType = 'z';
    pidl->ZipType = Type;

    if (Type != ZIP_PIDL_DIRECTORY)
    {
        pidl->CompressedSize = info.compressed_size;
        pidl->UncompressedSize = info.uncompressed_size;
        pidl->DosDate = info.dosDate;
        pidl->Password = info.flag & MINIZIP_PASSWORD_FLAG;
    }

    wcscpy(pidl->Name, lpString);
    *(WORD*)((char*)pidl + cbData) = 0; // The end of an ITEMIDLIST

    return (LPITEMIDLIST)pidl;
}

const ZipPidlEntry* _ZipFromIL(LPCITEMIDLIST pidl)
{
    const ZipPidlEntry* zipPidl = (const ZipPidlEntry*)pidl;
    if (zipPidl->MagicType == 'z')
        return zipPidl;
    return NULL;
}
