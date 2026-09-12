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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sxs);

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
#define SXS_LOOKUP_CLR_GUID_FIND_ANY       (SXS_LOOKUP_CLR_GUID_FIND_SURROGATE | SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS)

struct comclassredirect_data
{
    ULONG size;
    ULONG flags;
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

struct clrsurrogate_data
{
    ULONG size;
    DWORD res;
    GUID  clsid;
    ULONG version_offset;
    ULONG version_len;
    ULONG name_offset;
    ULONG name_len;
};

BOOL WINAPI SxsLookupClrGuid(DWORD flags, GUID *clsid, HANDLE actctx, void *buffer, SIZE_T buffer_len,
                             SIZE_T *buffer_len_required)
{
    ACTCTX_SECTION_KEYED_DATA guid_info = { sizeof(ACTCTX_SECTION_KEYED_DATA) };
    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *assembly_info = NULL;
    SIZE_T bytes_assembly_info;
    unsigned int len_version = 0, len_name, len_identity;
    const void *ptr_name, *ptr_version, *ptr_identity;
    SXS_GUID_INFORMATION_CLR *ret = buffer;
    BOOL retval = FALSE;
    char *ret_strings;
    ULONG_PTR cookie;

    TRACE("%#lx, %s, %p, %p, %Ix, %p.\n", flags, wine_dbgstr_guid(clsid), actctx,
          buffer, buffer_len, buffer_len_required);

    if (flags & SXS_LOOKUP_CLR_GUID_USE_ACTCTX)
    {
        if (!ActivateActCtx(actctx, &cookie))
        {
            WARN("Failed to activate context.\n");
            return FALSE;
        }
    }

    if (flags & SXS_LOOKUP_CLR_GUID_FIND_SURROGATE)
    {
        if ((retval = FindActCtxSectionGuid(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES, clsid, &guid_info)))
        {
            flags &= ~SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS;
        }
    }

    if (!retval && (flags & SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS))
    {
        if ((retval = FindActCtxSectionGuid(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, clsid, &guid_info)))
        {
            flags &= ~SXS_LOOKUP_CLR_GUID_FIND_SURROGATE;
        }
    }

    if (!retval)
    {
        SetLastError(ERROR_NOT_FOUND);
        goto out;
    }

    retval = QueryActCtxW(0, guid_info.hActCtx, &guid_info.ulAssemblyRosterIndex,
            AssemblyDetailedInformationInActivationContext, NULL, 0, &bytes_assembly_info);
    if (!retval && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        goto out;
    }

    assembly_info = malloc(bytes_assembly_info);
    if (!(retval = QueryActCtxW(0, guid_info.hActCtx, &guid_info.ulAssemblyRosterIndex,
            AssemblyDetailedInformationInActivationContext, assembly_info,
            bytes_assembly_info, &bytes_assembly_info)))
    {
        goto out;
    }

    if (flags & SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS)
    {
        const struct comclassredirect_data *redirect_data = guid_info.lpData;
        const struct clrclass_data *class_data;

        class_data = (void *)((char *)redirect_data + redirect_data->clrdata_offset);
        ptr_name = (char *)class_data + class_data->name_offset;
        ptr_version = (char *)class_data + class_data->version_offset;
        len_name = class_data->name_len + sizeof(WCHAR);
        if (class_data->version_len)
            len_version = class_data->version_len + sizeof(WCHAR);
    }
    else
    {
        const struct clrsurrogate_data *surrogate = guid_info.lpData;
        ptr_name = (char *)surrogate + surrogate->name_offset;
        ptr_version = (char *)surrogate + surrogate->version_offset;
        len_name = surrogate->name_len + sizeof(WCHAR);
        if (surrogate->version_len)
            len_version = surrogate->version_len + sizeof(WCHAR);
    }

    ptr_identity = assembly_info->lpAssemblyEncodedAssemblyIdentity;
    len_identity = assembly_info->ulEncodedAssemblyIdentityLength + sizeof(WCHAR);

    *buffer_len_required = sizeof(*ret) + len_identity + len_version + len_name;
    if (!buffer || buffer_len < *buffer_len_required)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        retval = FALSE;
        goto out;
    }

    ret->cbSize = sizeof(*ret);
    ret->dwFlags = flags & SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS ? SXS_GUID_INFORMATION_CLR_FLAG_IS_CLASS :
            SXS_GUID_INFORMATION_CLR_FLAG_IS_SURROGATE;

    /* Copy strings into buffer */
    ret_strings = (char *)ret + sizeof(*ret);

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

out:
    ReleaseActCtx(guid_info.hActCtx);

    if (flags & SXS_LOOKUP_CLR_GUID_USE_ACTCTX)
        DeactivateActCtx(0, cookie);

    free(assembly_info);
    return retval;
}
