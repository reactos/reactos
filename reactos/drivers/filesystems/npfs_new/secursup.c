#include "npfs.h"

VOID
NTAPI
NpUninitializeSecurity(IN PNP_CCB Ccb)
{
    PACCESS_TOKEN ClientToken;
    PSECURITY_CLIENT_CONTEXT ClientContext;
    TOKEN_TYPE TokenType;
    PAGED_CODE();

    ClientContext = Ccb->ClientContext;
    if (!ClientContext) return;

    TokenType = SeTokenType(ClientContext->ClientToken);
    ClientToken = Ccb->ClientContext->ClientToken;
    if ((TokenType == TokenPrimary) || (ClientToken))
    {
        ObfDereferenceObject(ClientToken);
    }
    ExFreePool(Ccb->ClientContext);
    Ccb->ClientContext = 0;
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
        Ccb->ClientQos.Length = 12;
        Ccb->ClientQos.ImpersonationLevel = 2;
        Ccb->ClientQos.ContextTrackingMode = 1;
        Ccb->ClientQos.EffectiveOnly = 1;
    }

    NpUninitializeSecurity(Ccb);

    if (Ccb->ClientQos.ContextTrackingMode)
    {
        Status = 0;
        Ccb->ClientContext = 0;
        return Status;
    }

    ClientContext = ExAllocatePoolWithTag(PagedPool, sizeof(*ClientContext), 'sFpN');
    Ccb->ClientContext = ClientContext;
    if (!ClientContext) return STATUS_INSUFFICIENT_RESOURCES;

    Status = SeCreateClientSecurity(Thread, &Ccb->ClientQos, 0, ClientContext);
    if (Status >= 0) return Status;
    ExFreePool(Ccb->ClientContext);
    Ccb->ClientContext = 0;
    return Status;
}
