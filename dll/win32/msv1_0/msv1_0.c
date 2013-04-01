/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.c
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "msv1_0.h"

WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);


/* GLOBALS *****************************************************************/

LSA_DISPATCH_TABLE DispatchTable;


/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApCallPackage(IN PLSA_CLIENT_REQUEST ClientRequest,
                 IN PVOID ProtocolSubmitBuffer,
                 IN PVOID ClientBufferBase,
                 IN ULONG SubmitBufferLength,
                 OUT PVOID *ProtocolReturnBuffer,
                 OUT PULONG ReturnBufferLength,
                 OUT PNTSTATUS ProtocolStatus)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApCallPackagePassthrough(IN PLSA_CLIENT_REQUEST ClientRequest,
                            IN PVOID ProtocolSubmitBuffer,
                            IN PVOID ClientBufferBase,
                            IN ULONG SubmitBufferLength,
                            OUT PVOID *ProtocolReturnBuffer,
                            OUT PULONG ReturnBufferLength,
                            OUT PNTSTATUS ProtocolStatus)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApCallPackageUntrusted(IN PLSA_CLIENT_REQUEST ClientRequest,
                          IN PVOID ProtocolSubmitBuffer,
                          IN PVOID ClientBufferBase,
                          IN ULONG SubmitBufferLength,
                          OUT PVOID *ProtocolReturnBuffer,
                          OUT PULONG ReturnBufferLength,
                          OUT PNTSTATUS ProtocolStatus)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApInitializePackage(IN ULONG AuthenticationPackageId,
                       IN PLSA_DISPATCH_TABLE LsaDispatchTable,
                       IN PLSA_STRING Database OPTIONAL,
                       IN PLSA_STRING Confidentiality OPTIONAL,
                       OUT PLSA_STRING *AuthenticationPackageName)
{
    PANSI_STRING NameString;
    PCHAR NameBuffer;

    TRACE("(%lu %p %p %p %p)\n",
          AuthenticationPackageId, LsaDispatchTable, Database,
          Confidentiality, AuthenticationPackageName);

    /* Get the dispatch table entries */
    DispatchTable.AllocateLsaHeap = LsaDispatchTable->AllocateLsaHeap;
    DispatchTable.FreeLsaHeap = LsaDispatchTable->FreeLsaHeap;


    /* Return the package name */
    NameString = DispatchTable.AllocateLsaHeap(sizeof(LSA_STRING));
    if (NameString == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    NameBuffer = DispatchTable.AllocateLsaHeap(sizeof(MSV1_0_PACKAGE_NAME));
    if (NameBuffer == NULL)
    {
        DispatchTable.FreeLsaHeap(NameString);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    strcpy(NameBuffer, MSV1_0_PACKAGE_NAME);

    RtlInitAnsiString(NameString, NameBuffer);

    *AuthenticationPackageName = (PLSA_STRING)NameString;

    return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID
NTAPI
LsaApLogonTerminated(IN PLUID LogonId)
{
    TRACE("()\n");
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApLogonUser(IN PLSA_CLIENT_REQUEST ClientRequest,
               IN SECURITY_LOGON_TYPE LogonType,
               IN PVOID AuthenticationInformation,
               IN PVOID ClientAuthenticationBase,
               IN ULONG AuthenticationInformationLength,
               OUT PVOID *ProfileBuffer,
               OUT PULONG ProfileBufferLength,
               OUT PLUID LogonId,
               OUT PNTSTATUS SubStatus,
               OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
               OUT PVOID *TokenInformation,
               OUT PLSA_UNICODE_STRING *AccountName,
               OUT PLSA_UNICODE_STRING *AuthenticatingAuthority)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
