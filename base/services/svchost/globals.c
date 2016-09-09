/*
 * PROJECT:     ReactOS Service Host
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        base/services/svchost/globals.c
 * PURPOSE:     Functions to initialize global settings and support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "svchost.h"

/* GLOBALS *******************************************************************/

PSID NullSid, WorldSid, LocalSid, NetworkSid, InteractiveSid, ServiceLogonSid;
PSID LocalSystemSid, LocalServiceSid, NetworkServiceSid, BuiltinDomainSid;
PSID AuthenticatedUserSid, AnonymousLogonSid, AliasAdminsSid, AliasUsersSid;
PSID AliasGuestsSid, AliasPowerUsersSid, AliasAccountOpsSid, AliasSystemOpsSid;
PSID AliasPrintOpsSid;
PSID AliasBackupOpsSid;

SID_DATA SidData[12] =
{
    { &NullSid, { SECURITY_NULL_SID_AUTHORITY }, SECURITY_NULL_RID },
    { &WorldSid, { SECURITY_WORLD_SID_AUTHORITY }, SECURITY_WORLD_RID },
    { &LocalSid, { SECURITY_LOCAL_SID_AUTHORITY }, SECURITY_LOCAL_RID },
    { &NetworkSid, { SECURITY_NT_AUTHORITY }, SECURITY_NETWORK_RID },
    { &InteractiveSid, { SECURITY_NT_AUTHORITY }, SECURITY_INTERACTIVE_RID },
    { &ServiceLogonSid, { SECURITY_NT_AUTHORITY }, SECURITY_SERVICE_RID },
    { &LocalSystemSid, { SECURITY_NT_AUTHORITY }, SECURITY_LOCAL_SYSTEM_RID },
    { &LocalServiceSid, { SECURITY_NT_AUTHORITY }, SECURITY_LOCAL_SERVICE_RID },
    { &NetworkServiceSid, { SECURITY_NT_AUTHORITY }, SECURITY_NETWORK_SERVICE_RID },
    { &BuiltinDomainSid, { SECURITY_NT_AUTHORITY }, SECURITY_BUILTIN_DOMAIN_RID },
    { &AuthenticatedUserSid, { SECURITY_NT_AUTHORITY }, SECURITY_AUTHENTICATED_USER_RID },
    { &AnonymousLogonSid, { SECURITY_NT_AUTHORITY }, SECURITY_ANONYMOUS_LOGON_RID },
};

DOMAIN_SID_DATA DomainSidData[8] =
{
    { &AliasAdminsSid, DOMAIN_ALIAS_RID_ADMINS },
    { &AliasUsersSid, DOMAIN_ALIAS_RID_USERS },
    { &AliasGuestsSid, DOMAIN_ALIAS_RID_GUESTS },
    { &AliasPowerUsersSid, DOMAIN_ALIAS_RID_POWER_USERS },
    { &AliasAccountOpsSid, DOMAIN_ALIAS_RID_ACCOUNT_OPS },
    { &AliasSystemOpsSid, DOMAIN_ALIAS_RID_SYSTEM_OPS },
    { &AliasPrintOpsSid, DOMAIN_ALIAS_RID_PRINT_OPS },
    { &AliasBackupOpsSid, DOMAIN_ALIAS_RID_BACKUP_OPS },
};

PSVCHOST_GLOBALS g_pSvchostSharedGlobals;
DWORD g_SvchostInitFlag;
HANDLE g_hHeap;

/* FUNCTIONS *****************************************************************/

VOID
WINAPI
MemInit (
    _In_ HANDLE Heap
    )
{
    /* Save the heap handle */
    g_hHeap = Heap;
}

BOOL
WINAPI
MemFree (
    _In_ LPVOID lpMem
    )
{
    /* Free memory back into the heap */
    return HeapFree(g_hHeap, 0, lpMem);
}

PVOID
WINAPI
MemAlloc (
    _In_ DWORD dwFlags,
    _In_ DWORD dwBytes
    )
{
    /* Allocate memory from the heap */
    return HeapAlloc(g_hHeap, dwFlags, dwBytes);
}

VOID
WINAPI
SvchostBuildSharedGlobals (
    VOID
    )
{
    ASSERT(g_pSvchostSharedGlobals == NULL);

    /* Is RPC initialized? */
    if (!(g_SvchostInitFlag & SVCHOST_RPC_INIT_COMPLETE))
    {
        /* Nope, go initialize it */
        if (!NT_SUCCESS(RpcpInitRpcServer())) return;

        /* This is now done */
        g_SvchostInitFlag |= SVCHOST_RPC_INIT_COMPLETE;
    }

    /* Is NetBIOS initialized? */
    if (!(g_SvchostInitFlag & SVCHOST_NBT_INIT_COMPLETE))
    {
        /* Nope, set it up */
        SvcNetBiosInit();

        /* This is now done */
        g_SvchostInitFlag |= SVCHOST_NBT_INIT_COMPLETE;
    }

    /* Do we have the global SIDs initialized? */
    if (!(g_SvchostInitFlag & SVCHOST_SID_INIT_COMPLETE))
    {
        /* Create the SIDs we'll export in the global structure */
        if (!NT_SUCCESS(ScCreateWellKnownSids())) return;

        /* This is now done */
        g_SvchostInitFlag |= SVCHOST_SID_INIT_COMPLETE;
    }

    /* Allocate memory for the globals */
    g_pSvchostSharedGlobals = MemAlloc(HEAP_ZERO_MEMORY,
                                       sizeof(*g_pSvchostSharedGlobals));
    if (g_pSvchostSharedGlobals == NULL) return;

    /* Write the pointers to the SIDs */
    g_pSvchostSharedGlobals->NullSid = NullSid;
    g_pSvchostSharedGlobals->WorldSid = WorldSid;
    g_pSvchostSharedGlobals->LocalSid = LocalSid;
    g_pSvchostSharedGlobals->NetworkSid = NetworkSid;
    g_pSvchostSharedGlobals->LocalSystemSid = LocalSystemSid;
    g_pSvchostSharedGlobals->LocalServiceSid = LocalServiceSid;
    g_pSvchostSharedGlobals->NetworkServiceSid = NetworkServiceSid;
    g_pSvchostSharedGlobals->BuiltinDomainSid = BuiltinDomainSid;
    g_pSvchostSharedGlobals->AuthenticatedUserSid = AuthenticatedUserSid;
    g_pSvchostSharedGlobals->AnonymousLogonSid = AnonymousLogonSid;
    g_pSvchostSharedGlobals->AliasAdminsSid = AliasAdminsSid;
    g_pSvchostSharedGlobals->AliasUsersSid = AliasUsersSid;
    g_pSvchostSharedGlobals->AliasGuestsSid = AliasGuestsSid;
    g_pSvchostSharedGlobals->AliasPowerUsersSid = AliasPowerUsersSid;
    g_pSvchostSharedGlobals->AliasAccountOpsSid = AliasAccountOpsSid;
    g_pSvchostSharedGlobals->AliasSystemOpsSid = AliasSystemOpsSid;
    g_pSvchostSharedGlobals->AliasPrintOpsSid = AliasPrintOpsSid;
    g_pSvchostSharedGlobals->AliasBackupOpsSid = AliasBackupOpsSid;

    /* Write the pointers to the callbacks */
    g_pSvchostSharedGlobals->RpcpStartRpcServer = RpcpStartRpcServer;
    g_pSvchostSharedGlobals->RpcpStopRpcServer = RpcpStopRpcServer;
    g_pSvchostSharedGlobals->RpcpStopRpcServerEx = RpcpStopRpcServerEx;
    g_pSvchostSharedGlobals->SvcNetBiosOpen = SvcNetBiosOpen;
    g_pSvchostSharedGlobals->SvcNetBiosClose = SvcNetBiosClose;
    g_pSvchostSharedGlobals->SvcNetBiosReset = SvcNetBiosReset;
    g_pSvchostSharedGlobals->SvcRegisterStopCallback = SvcRegisterStopCallback;
}

VOID
WINAPI
SvchostCharLowerW (
    _In_ LPCWSTR lpSrcStr
    )
{
    DWORD cchSrc;

    /* If there's nothing to do, bail out */
    if (lpSrcStr == NULL) return;

    /* Get the length of the input string */
    cchSrc = wcslen(lpSrcStr);

    /* Call the locale API to lower-case it */
    if (LCMapStringW(LANG_USER_DEFAULT,
                     LCMAP_LOWERCASE,
                     lpSrcStr,
                     cchSrc + 1,
                     (LPWSTR)lpSrcStr,
                     cchSrc + 1) == FALSE)
    {
        DBG_ERR("SvchostCharLowerW failed for %ws\n", lpSrcStr);
    }
}

NTSTATUS
NTAPI
ScDomainIdToSid (
    _In_ PSID SourceSid,
    _In_ ULONG DomainId,
    _Out_ PSID *DestinationSid
    )
{
    ULONG sidCount, sidLength;
    NTSTATUS status;

    /* Get the length of the SID based onthe number of subauthorities */
    sidCount = *RtlSubAuthorityCountSid(SourceSid);
    sidLength = RtlLengthRequiredSid(sidCount + 1);

    /* Allocate it */
    *DestinationSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, sidLength);
    if (*DestinationSid)
    {
        /* Make a copy of it */
        status = RtlCopySid(sidLength, *DestinationSid, SourceSid);
        if (NT_SUCCESS(status))
        {
            /* Increase the subauthority count */
            ++*RtlSubAuthorityCountSid(*DestinationSid);

            /* And add the specific domain RID we're creating */
            *RtlSubAuthoritySid(*DestinationSid, sidCount) = DomainId;

            /* Everything worked */
            status = STATUS_SUCCESS;
        }
        else
        {
            /* The SID copy failed, so free the SID we just allocated */
            RtlFreeHeap(RtlGetProcessHeap(), 0, *DestinationSid);
        }
    }
    else
    {
        /* No space for the SID, bail out */
        status = STATUS_NO_MEMORY;
    }

    /* Return back to the caller */
    return status;
}

NTSTATUS
NTAPI
ScAllocateAndInitializeSid (
    _Out_ PVOID *Sid,
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    _In_ ULONG SubAuthorityCount
    )
{
    NTSTATUS Status;

    /* Allocate room for the SID */
    *Sid = RtlAllocateHeap(RtlGetProcessHeap(),
                           0,
                           RtlLengthRequiredSid(SubAuthorityCount));
    if (*Sid)
    {
        /* Initialize it, we're done */
        RtlInitializeSid(*Sid, IdentifierAuthority, SubAuthorityCount);
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* No memory, we'll fail */
        Status = STATUS_NO_MEMORY;
    }

    /* Return what happened */
    return Status;
}

NTSTATUS
NTAPI
ScCreateWellKnownSids (
    VOID
    )
{
    ULONG i;
    NTSTATUS Status;

    /* Loop the non-domain SIDs */
    for (i = 0; i < RTL_NUMBER_OF(SidData); i++)
    {
        /* Convert our optimized structure into an actual SID */
        Status = ScAllocateAndInitializeSid(&SidData[i].Sid,
                                            &SidData[i].Authority,
                                            1);
        if (!NT_SUCCESS(Status)) break;

        /* Write the correct sub-authority */
        *RtlSubAuthoritySid(SidData[i].Sid, 0) = SidData[i].SubAuthority;
    }

    /* Now loop the domain SIDs  */
    for (i = 0; i < RTL_NUMBER_OF(DomainSidData); i++)
    {
        /* Convert our optimized structure into an actual SID */
        Status = ScDomainIdToSid(BuiltinDomainSid,
                                 DomainSidData[i].SubAuthority,
                                 &DomainSidData[i].Sid);
        if (!NT_SUCCESS(Status)) break;
    }

    /* If we got to the end, return success */
    return (i == RTL_NUMBER_OF(DomainSidData)) ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

