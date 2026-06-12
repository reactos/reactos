/*
 * DllGetVersion default implementation
 *
 * Copyright 2025 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * In addition to the permissions in the GNU Lesser General Public License,
 * the authors give you unlimited permission to link the compiled version
 * of this file with other programs, and to distribute those programs
 * without any restriction coming from the use of this file.  (The GNU
 * Lesser General Public License restrictions do apply in other respects;
 * for example, they cover modification of the file, and distribution when
 * not linked into another program.)
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "shlwapi.h"

static inline void *image_base(void)
{
#if defined(__MINGW32__) || defined(_MSC_VER)
    extern IMAGE_DOS_HEADER __ImageBase;
    return (void *)&__ImageBase;
#else
    extern IMAGE_NT_HEADERS __wine_spec_nt_header;
    return (void *)((__wine_spec_nt_header.OptionalHeader.ImageBase + 0xffff) & ~0xffff);
#endif
}

struct version_info
{
    WORD  len;
    WORD  val_len;
    WORD  type;
    WCHAR key[];
};

#define GET_VERSION_VALUE(info) ((void *)((char *)info + ((offsetof(struct version_info, key[lstrlenW(info->key) + 1]) + 3) & ~3)))

HRESULT WINAPI DllGetVersion( DLLVERSIONINFO *info )
{
    HRSRC rsrc;
    HMODULE module = image_base();
    struct version_info *data;
    VS_FIXEDFILEINFO *fileinfo;
    DLLVERSIONINFO2 *info2 = (DLLVERSIONINFO2 *)info;

    if (!info) return E_INVALIDARG;
    if (!(rsrc = FindResourceW( module, (LPWSTR)VS_VERSION_INFO, (LPWSTR)VS_FILE_INFO ))) return E_INVALIDARG;
    if (!(data = LoadResource( module, rsrc ))) return E_INVALIDARG;
    if (data->val_len < sizeof(*fileinfo)) return E_INVALIDARG;
    fileinfo = GET_VERSION_VALUE( data );
    if (fileinfo->dwSignature != VS_FFI_SIGNATURE) return E_INVALIDARG;

    switch (info->cbSize)
    {
    case sizeof(DLLVERSIONINFO2):
        info2->dwFlags    = 0;
        info2->ullVersion = ((ULONGLONG)fileinfo->dwFileVersionMS << 32) | fileinfo->dwFileVersionLS;
        /* fall through */
    case sizeof(DLLVERSIONINFO):
        info->dwMajorVersion = HIWORD( fileinfo->dwFileVersionMS );
        info->dwMinorVersion = LOWORD( fileinfo->dwFileVersionMS );
        info->dwBuildNumber  = HIWORD( fileinfo->dwFileVersionLS );
        info->dwPlatformID   = DLLVER_PLATFORM_NT;
        return S_OK;
    default:
        return E_INVALIDARG;
    }
}
