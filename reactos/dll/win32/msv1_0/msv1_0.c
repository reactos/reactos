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
    TRACE("(%lu %p %p %p %p)\n",
          AuthenticationPackageId, LsaDispatchTable, Database,
          Confidentiality, AuthenticationPackageName);


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
