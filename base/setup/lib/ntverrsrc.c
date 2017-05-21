/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/ntverrsrc.c
 * PURPOSE:         NT Version Resource Management API
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE 1: Adapted from Wine-synced dll/win32/version DLL.
 * NOTE 2: We only deal with 32-bit PE executables.
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include <ndk/ldrtypes.h>
#include <ndk/ldrfuncs.h>

#include "ntverrsrc.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

NTSTATUS
NtGetVersionResource(
    IN PVOID BaseAddress,
    OUT PVOID* Resource,
    OUT PULONG ResourceSize OPTIONAL)
{
// #define RT_VERSION MAKEINTRESOURCE(16)  // See winuser.h
#define VS_VERSION_INFO         1       // See psdk/verrsrc.h
#define VS_FILE_INFO            RT_VERSION

    NTSTATUS Status;
    LDR_RESOURCE_INFO ResourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    PVOID Data = NULL;
    ULONG Size = 0;

    /* Try to find the resource */
    ResourceInfo.Type = 16; // RT_VERSION;
    ResourceInfo.Name = VS_VERSION_INFO; // MAKEINTRESOURCEW(VS_VERSION_INFO);
    ResourceInfo.Language = 0; // Don't care about the language

    Status = LdrFindResource_U(BaseAddress,
                               &ResourceInfo,
                               RESOURCE_DATA_LEVEL,
                               &ResourceDataEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtGetVersionResource: Version resource not found, Status 0x%08lx\n", Status);
        return Status;
    }

    /* Access the resource */
    Status = LdrAccessResource(BaseAddress,
                               ResourceDataEntry,
                               &Data,
                               &Size);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtGetVersionResource: Cannot access Version resource, Status 0x%08lx\n", Status);
        return Status;
    }

    *Resource = Data;
    if (ResourceSize) *ResourceSize = Size;

    return STATUS_SUCCESS;
}

/* NOTE: the xxx_STRUCT16 version differs by storing strings in ANSI, not in UNICODE */
typedef struct _VS_VERSION_INFO_STRUCT32
{
    WORD  wLength;
    WORD  wValueLength;
    WORD  wType; /* 1:Text, 0:Binary */
    WCHAR szKey[1];
#if 0   /* variable length structure */
    /* DWORD aligned */
    BYTE  Value[];
    /* DWORD aligned */
    VS_VERSION_INFO_STRUCT32 Children[];
#endif
}   VS_VERSION_INFO_STRUCT32, *PVS_VERSION_INFO_STRUCT32;
typedef const VS_VERSION_INFO_STRUCT32 *PCVS_VERSION_INFO_STRUCT32;

#define DWORD_ALIGN( base, ptr ) \
    ( (ULONG_PTR)(base) + ((((ULONG_PTR)(ptr) - (ULONG_PTR)(base)) + 3) & ~3) )

#define VersionInfo32_Value( ver )  \
    DWORD_ALIGN( (ver), (ver)->szKey + wcslen((ver)->szKey) + 1 )

#define VersionInfo32_Children( ver )  \
    (PCVS_VERSION_INFO_STRUCT32)( VersionInfo32_Value( ver ) + \
                           ( ( (ver)->wValueLength * \
                               ((ver)->wType? 2 : 1) + 3 ) & ~3 ) )

#define VersionInfo32_Next( ver ) \
    (PVS_VERSION_INFO_STRUCT32)( (ULONG_PTR)ver + (((ver)->wLength + 3) & ~3) )

static PCVS_VERSION_INFO_STRUCT32
VersionInfo32_FindChild(
    IN PCVS_VERSION_INFO_STRUCT32 info,
    IN PCWSTR szKey,
    IN UINT cbKey)
{
    PCVS_VERSION_INFO_STRUCT32 child = VersionInfo32_Children(info);

    while ((ULONG_PTR)child < (ULONG_PTR)info + info->wLength)
    {
        if (!_wcsnicmp(child->szKey, szKey, cbKey) && !child->szKey[cbKey])
            return child;

        if (child->wLength == 0) return NULL;
        child = VersionInfo32_Next(child);
    }

    return NULL;
}

static NTSTATUS
VersionInfo32_QueryValue(
    IN PCVS_VERSION_INFO_STRUCT32 info,
    IN PCWSTR lpSubBlock,
    OUT PVOID* lplpBuffer,
    OUT PUINT puLen OPTIONAL,
    OUT BOOL* pbText OPTIONAL)
{
    PCWSTR lpNextSlash;

    DPRINT("lpSubBlock : (%S)\n", lpSubBlock);

    while (*lpSubBlock)
    {
        /* Find next path component */
        for (lpNextSlash = lpSubBlock; *lpNextSlash; lpNextSlash++)
        {
            if (*lpNextSlash == '\\')
                break;
        }

        /* Skip empty components */
        if (lpNextSlash == lpSubBlock)
        {
            lpSubBlock++;
            continue;
        }

        /* We have a non-empty component: search info for key */
        info = VersionInfo32_FindChild(info, lpSubBlock, lpNextSlash - lpSubBlock);
        if (!info)
        {
            if (puLen) *puLen = 0;
            return STATUS_RESOURCE_TYPE_NOT_FOUND;
        }

        /* Skip path component */
        lpSubBlock = lpNextSlash;
    }

    /* Return value */
    *lplpBuffer = (PVOID)VersionInfo32_Value(info);
    if (puLen)
        *puLen = info->wValueLength;
    if (pbText)
        *pbText = info->wType;

    return STATUS_SUCCESS;
}

NTSTATUS
NtVerQueryValue(
    IN const VOID* pBlock,
    IN PCWSTR lpSubBlock,
    OUT PVOID* lplpBuffer,
    OUT PUINT puLen)
{
    PCVS_VERSION_INFO_STRUCT32 info = pBlock;

    DPRINT("%s (%p, %S, %p, %p)\n", __FUNCTION__, pBlock, lpSubBlock, lplpBuffer, puLen);

    if (!pBlock)
        return FALSE;

    if (!lpSubBlock || !*lpSubBlock)
        lpSubBlock = L"\\";

    return VersionInfo32_QueryValue(info, lpSubBlock, lplpBuffer, puLen, NULL);
}

/* EOF */
