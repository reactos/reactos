/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/secursup.c
 * PURPOSE:     Pipes Security Support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_SECURSUP)

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
NpImpersonateClientContext(IN PNP_CCB Ccb)
{
    NTSTATUS Status;
    PSECURITY_CLIENT_CONTEXT ClientContext;
    PAGED_CODE();

    ClientContext = Ccb->ClientContext;
    if (ClientContext)
    {
        Status = SeImpersonateClientEx(ClientContext, NULL);
    }
    else
    {
        Status = STATUS_CANNOT_IMPERSONATE;
    }
    return Status;
}

VOID
NTAPI
NpFreeClientSecurityContext(IN PSECURITY_CLIENT_CONTEXT ClientContext)
{
    TOKEN_TYPE TokenType;
    PVOID ClientToken;

    if (!ClientContext) return;

    TokenType = SeTokenType(ClientContext->ClientToken);
    ClientToken = ClientContext->ClientToken;
    if ((TokenType == TokenPrimary) || (ClientToken))
    {
        ObDereferenceObject(ClientToken);
    }
    ExFreePool(ClientContext);
}

VOID
NTAPI
NpCopyClientContext(IN PNP_CCB Ccb,
                    IN PNP_DATA_QUEUE_ENTRY DataQueueEntry)
{
    PAGED_CODE();

    if (!DataQueueEntry->ClientSecurityContext) return;

    NpFreeClientSecurityContext(Ccb->ClientContext);
    Ccb->ClientContext = DataQueueEntry->ClientSecurityContext;
    DataQueueEntry->ClientSecurityContext = NULL;
}

VOID
NTAPI
NpUninitializeSecurity(IN PNP_CCB Ccb)
{
    PAGED_CODE();

    NpFreeClientSecurityContext(Ccb->ClientContext);
    Ccb->ClientContext = NULL;
}

NTSTATUS
NTAPI
NpInitializeSecurity(IN PNP_CCB Ccb,
                     IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
                     IN PETHREAD Thread)
{
    PSECURITY_CLIENT_CONTEXT ClientContext;
    NTSTATUS Status;
    PAGED_CODE();

    if (SecurityQos)
    {
        Ccb->ClientQos = *SecurityQos;
    }
    else
    {
        Ccb->ClientQos.Length = sizeof(Ccb->ClientQos);
        Ccb->ClientQos.ImpersonationLevel = SecurityImpersonation;
        Ccb->ClientQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        Ccb->ClientQos.EffectiveOnly = TRUE;
    }

    NpUninitializeSecurity(Ccb);

    if (Ccb->ClientQos.ContextTrackingMode == SECURITY_DYNAMIC_TRACKING)
    {
        Status = STATUS_SUCCESS;
        Ccb->ClientContext = NULL;
        return Status;
    }

    ClientContext = ExAllocatePoolWithQuotaTag(PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                               sizeof(*ClientContext),
                                               NPFS_CLIENT_SEC_CTX_TAG);
    Ccb->ClientContext = ClientContext;
    if (!ClientContext) return STATUS_INSUFFICIENT_RESOURCES;

    Status = SeCreateClientSecurity(Thread, &Ccb->ClientQos, 0, ClientContext);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(Ccb->ClientContext);
        Ccb->ClientContext = NULL;
    }

    return Status;
}

NTSTATUS
NTAPI
NpGetClientSecurityContext(IN ULONG NamedPipeEnd,
                           IN PNP_CCB Ccb,
                           IN PETHREAD Thread,
                           IN PSECURITY_CLIENT_CONTEXT *Context)
{
    PSECURITY_CLIENT_CONTEXT NewContext;
    NTSTATUS Status;
    PAGED_CODE();

    if (NamedPipeEnd == FILE_PIPE_SERVER_END || Ccb->ClientQos.ContextTrackingMode != SECURITY_DYNAMIC_TRACKING)
    {
        NewContext = NULL;
        Status = STATUS_SUCCESS;
    }
    else
    {
        NewContext = ExAllocatePoolWithQuotaTag(PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                                sizeof(*NewContext),
                                                NPFS_CLIENT_SEC_CTX_TAG);
        if (!NewContext) return STATUS_INSUFFICIENT_RESOURCES;

        Status = SeCreateClientSecurity(Thread, &Ccb->ClientQos, 0, NewContext);
        if (!NT_SUCCESS(Status))
        {
            ExFreePool(NewContext);
            NewContext = NULL;
        }
    }
    *Context = NewContext;
    return Status;
}

/* EOF */
