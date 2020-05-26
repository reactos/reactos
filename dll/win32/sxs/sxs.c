/*
 * sxs main
 *
 * Copyright 2007 EA Durbin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sxs);

/***********************************************************************
 *             DllMain   (SXS.@)
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        break;
    }
    return TRUE;
}

typedef struct _SXS_GUID_INFORMATION_CLR
{
    DWORD cbSize;
    DWORD dwFlags;
    PCWSTR pcwszRuntimeVersion;
    PCWSTR pcwszTypeName;
    PCWSTR pcwszAssemblyIdentity;
} SXS_GUID_INFORMATION_CLR, *PSXS_GUID_INFORMATION_CLR;

#define SXS_GUID_INFORMATION_CLR_FLAG_IS_SURROGATE 0x1
#define SXS_GUID_INFORMATION_CLR_FLAG_IS_CLASS     0x2

#define SXS_LOOKUP_CLR_GUID_USE_ACTCTX     0x00000001
#define SXS_LOOKUP_CLR_GUID_FIND_SURROGATE 0x00010000
#define SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS 0x00020000

struct comclassredirect_data
{
    ULONG size;
    BYTE  res;
    BYTE  miscmask;
    BYTE  res1[2];
    DWORD model;
    GUID  clsid;
    GUID  alias;
    GUID  clsid2;
    GUID  tlbid;
    ULONG name_len;
    ULONG name_offset;
    ULONG progid_len;
    ULONG progid_offset;
    ULONG clrdata_len;
    ULONG clrdata_offset;
    DWORD miscstatus;
    DWORD miscstatuscontent;
    DWORD miscstatusthumbnail;
    DWORD miscstatusicon;
    DWORD miscstatusdocprint;
};

struct clrclass_data
{
    ULONG size;
    DWORD res[2];
    ULONG module_len;
    ULONG module_offset;
    ULONG name_len;
    ULONG name_offset;
    ULONG version_len;
    ULONG version_offset;
    DWORD res2[2];
};

BOOL WINAPI SxsLookupClrGuid(DWORD flags, GUID *clsid, HANDLE actctx, void *buffer, SIZE_T buffer_len,
                             SIZE_T *buffer_len_required)
{
    ACTCTX_SECTION_KEYED_DATA guid_info = { sizeof(ACTCTX_SECTION_KEYED_DATA) };
    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *assembly_info;
    SIZE_T bytes_assembly_info;
    struct comclassredirect_data *redirect_data;
    struct clrclass_data *class_data;
    int len_version = 0, len_name, len_identity;
    const void *ptr_name, *ptr_version, *ptr_identity;
    SXS_GUID_INFORMATION_CLR *ret = buffer;
    char *ret_strings;

    TRACE("(%x, %s, %p, %p, %08lx, %p): stub\n", flags, wine_dbgstr_guid(clsid), actctx,
          buffer, buffer_len, buffer_len_required);

    if (flags & ~SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS)
        FIXME("Ignored flags: %x\n", flags & ~SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS);

    if (!FindActCtxSectionGuid(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, 0,
                               ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, clsid, &guid_info))
    {
        SetLastError(ERROR_NOT_FOUND);
        return FALSE;
    }

    QueryActCtxW(0, guid_info.hActCtx, &guid_info.ulAssemblyRosterIndex,
            AssemblyDetailedInformationInActivationContext, NULL, 0, &bytes_assembly_info);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        ReleaseActCtx(guid_info.hActCtx);
        return FALSE;
    }
    assembly_info = heap_alloc(bytes_assembly_info);
    if(!QueryActCtxW(0, guid_info.hActCtx, &guid_info.ulAssemblyRosterIndex,
            AssemblyDetailedInformationInActivationContext, assembly_info,
            bytes_assembly_info, &bytes_assembly_info))
    {
        heap_free(assembly_info);
        ReleaseActCtx(guid_info.hActCtx);
        return FALSE;
    }

    redirect_data = guid_info.lpData;
    class_data = (void *)((char*)redirect_data + redirect_data->clrdata_offset);

    ptr_identity = assembly_info->lpAssemblyEncodedAssemblyIdentity;
    ptr_name = (char *)class_data + class_data->name_offset;
    ptr_version = (char *)class_data + class_data->version_offset;

    len_identity = assembly_info->ulEncodedAssemblyIdentityLength + sizeof(WCHAR);
    len_name = class_data->name_len + sizeof(WCHAR);
    if (class_data->version_len > 0)
        len_version = class_data->version_len + sizeof(WCHAR);

    *buffer_len_required = sizeof(SXS_GUID_INFORMATION_CLR) + len_identity + len_version + len_name;
    if (!buffer || buffer_len < *buffer_len_required)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        heap_free(assembly_info);
        ReleaseActCtx(guid_info.hActCtx);
        return FALSE;
    }

    ret->cbSize = sizeof(SXS_GUID_INFORMATION_CLR);
    ret->dwFlags = SXS_GUID_INFORMATION_CLR_FLAG_IS_CLASS;

    /* Copy strings into buffer */
    ret_strings = (char *)ret + sizeof(SXS_GUID_INFORMATION_CLR);

    memcpy(ret_strings, ptr_identity, len_identity);
    ret->pcwszAssemblyIdentity = (WCHAR *)ret_strings;
    ret_strings += len_identity;

    memcpy(ret_strings, ptr_name, len_name);
    ret->pcwszTypeName = (WCHAR *)ret_strings;
    ret_strings += len_name;

    if (len_version > 0)
    {
        memcpy(ret_strings, ptr_version, len_version);
        ret->pcwszRuntimeVersion = (WCHAR *)ret_strings;
    }
    else
        ret->pcwszRuntimeVersion = NULL;

    SetLastError(0);

    ReleaseActCtx(guid_info.hActCtx);
    heap_free(assembly_info);
    return TRUE;
}
