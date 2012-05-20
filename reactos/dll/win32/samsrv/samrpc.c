/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/samsrv/samrpc.c
 * PURPOSE:         RPC interface functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);


/* FUNCTIONS ***************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

void __RPC_USER SAMPR_HANDLE_rundown(SAMPR_HANDLE hHandle)
{
}

/* Function 0 */
NTSTATUS
NTAPI
SamrConnect(IN PSAMPR_SERVER_NAME ServerName,
            OUT SAMPR_HANDLE *ServerHandle,
            IN ACCESS_MASK DesiredAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 1 */
NTSTATUS
NTAPI
SamrCloseHandle(IN OUT SAMPR_HANDLE *SamHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 2 */
NTSTATUS
NTAPI
SamrSetSecurityObject(IN SAMPR_HANDLE ObjectHandle,
                      IN SECURITY_INFORMATION SecurityInformation,
                      IN PSAMPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 3 */
NTSTATUS
NTAPI
SamrQuerySecurityObject(IN SAMPR_HANDLE ObjectHandle,
                        IN SECURITY_INFORMATION SecurityInformation,
                        OUT PSAMPR_SR_SECURITY_DESCRIPTOR * SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 4 */
NTSTATUS
NTAPI
SamrShutdownSamServer(IN SAMPR_HANDLE ServerHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 5 */
NTSTATUS
NTAPI
SamrLookupDomainInSamServer(IN SAMPR_HANDLE ServerHandle,
                            IN PRPC_UNICODE_STRING Name,
                            OUT PRPC_SID *DomainId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 6 */
NTSTATUS
NTAPI
SamrEnumerateDomainsInSamServer(IN SAMPR_HANDLE ServerHandle,
                                IN OUT unsigned long *EnumerationContext,
                                OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
                                IN unsigned long PreferedMaximumLength,
                                OUT unsigned long *CountReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 7 */
NTSTATUS
NTAPI
SamrOpenDomain(IN SAMPR_HANDLE ServerHandle,
               IN ACCESS_MASK DesiredAccess,
               IN PRPC_SID DomainId,
               OUT SAMPR_HANDLE *DomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 8 */
NTSTATUS
NTAPI
SamrQueryInformationDomain(IN SAMPR_HANDLE DomainHandle,
                           IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                           OUT PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 9 */
NTSTATUS
NTAPI
SamrSetInformationDomain(IN SAMPR_HANDLE DomainHandle,
                         IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                         IN PSAMPR_DOMAIN_INFO_BUFFER DomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 10 */
NTSTATUS
NTAPI
SamrCreateGroupInDomain(IN SAMPR_HANDLE DomainHandle,
                        IN PRPC_UNICODE_STRING Name,
                        IN ACCESS_MASK DesiredAccess,
                        OUT SAMPR_HANDLE *GroupHandle,
                        OUT unsigned long *RelativeId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 10 */
NTSTATUS
NTAPI
SamrEnumerateGroupsInDomain(IN SAMPR_HANDLE DomainHandle,
                            IN OUT unsigned long *EnumerationContext,
                            OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
                            IN unsigned long PreferedMaximumLength,
                            OUT unsigned long *CountReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 12 */
NTSTATUS
__stdcall
SamrCreateUserInDomain(IN SAMPR_HANDLE DomainHandle,
                       IN PRPC_UNICODE_STRING Name,
                       IN ACCESS_MASK DesiredAccess,
                       OUT SAMPR_HANDLE *UserHandle,
                       OUT unsigned long *RelativeId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 13 */
NTSTATUS
__stdcall
SamrEnumerateUsersInDomain(IN SAMPR_HANDLE DomainHandle,
                           IN OUT unsigned long *EnumerationContext,
                           IN unsigned long UserAccountControl,
                           OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
                           IN unsigned long PreferedMaximumLength,
                           OUT unsigned long *CountReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 14 */
NTSTATUS
__stdcall
SamrCreateAliasInDomain(IN SAMPR_HANDLE DomainHandle,
                        IN PRPC_UNICODE_STRING AccountName,
                        IN ACCESS_MASK DesiredAccess,
                        OUT SAMPR_HANDLE *AliasHandle,
                        OUT unsigned long *RelativeId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 15 */
NTSTATUS
__stdcall
SamrEnumerateAliasesInDomain(IN SAMPR_HANDLE DomainHandle,
                             IN OUT unsigned long *EnumerationContext,
                             OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
                             IN unsigned long PreferedMaximumLength,
                             OUT unsigned long *CountReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 16 */
NTSTATUS
__stdcall
SamrGetAliasMembership(IN SAMPR_HANDLE DomainHandle,
                       IN PSAMPR_PSID_ARRAY SidArray,
                       OUT PSAMPR_ULONG_ARRAY Membership)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 17 */
NTSTATUS
__stdcall
SamrLookupNamesInDomain(IN SAMPR_HANDLE DomainHandle,
                        IN unsigned long Count,
                        IN RPC_UNICODE_STRING Names[],
                        OUT PSAMPR_ULONG_ARRAY RelativeIds,
                        OUT PSAMPR_ULONG_ARRAY Use)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 18 */
NTSTATUS
__stdcall
SamrLookupIdsInDomain(IN SAMPR_HANDLE DomainHandle,
                      IN unsigned long Count,
                      IN unsigned long *RelativeIds,
                      OUT PSAMPR_RETURNED_USTRING_ARRAY Names,
                      OUT PSAMPR_ULONG_ARRAY Use)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 19 */
NTSTATUS
__stdcall
SamrOpenGroup(IN SAMPR_HANDLE DomainHandle,
              IN ACCESS_MASK DesiredAccess,
              IN unsigned long GroupId,
              OUT SAMPR_HANDLE *GroupHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 20 */
NTSTATUS
__stdcall
SamrQueryInformationGroup(IN SAMPR_HANDLE GroupHandle,
                          IN GROUP_INFORMATION_CLASS GroupInformationClass,
                          OUT PSAMPR_GROUP_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 21 */
NTSTATUS
__stdcall
SamrSetInformationGroup(IN SAMPR_HANDLE GroupHandle,
                        IN GROUP_INFORMATION_CLASS GroupInformationClass,
                        IN PSAMPR_GROUP_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 22 */
NTSTATUS
NTAPI
SamrAddMemberToGroup(IN SAMPR_HANDLE GroupHandle,
                     IN unsigned long MemberId,
                     IN unsigned long Attributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 21 */
NTSTATUS
NTAPI
SamrDeleteGroup(IN OUT SAMPR_HANDLE *GroupHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 24 */
NTSTATUS
NTAPI
SamrRemoveMemberFromGroup(IN SAMPR_HANDLE GroupHandle,
                          IN unsigned long MemberId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 25 */
NTSTATUS
NTAPI
SamrGetMembersInGroup(IN SAMPR_HANDLE GroupHandle,
                      OUT PSAMPR_GET_MEMBERS_BUFFER *Members)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 26 */
NTSTATUS
NTAPI
SamrSetMemberAttributesOfGroup(IN SAMPR_HANDLE GroupHandle,
                               IN unsigned long MemberId,
                               IN unsigned long Attributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 27 */
NTSTATUS
NTAPI
SamrOpenAlias(IN SAMPR_HANDLE DomainHandle,
              IN ACCESS_MASK DesiredAccess,
              IN unsigned long AliasId,
              OUT SAMPR_HANDLE *AliasHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 28 */
NTSTATUS
NTAPI
SamrQueryInformationAlias(IN SAMPR_HANDLE AliasHandle,
                          IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                          OUT PSAMPR_ALIAS_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 29 */
NTSTATUS
NTAPI
SamrSetInformationAlias(IN SAMPR_HANDLE AliasHandle,
                        IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                        IN PSAMPR_ALIAS_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 30 */
NTSTATUS
NTAPI
SamrDeleteAlias(IN OUT SAMPR_HANDLE *AliasHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 31 */
NTSTATUS
NTAPI
SamrAddMemberToAlias(IN SAMPR_HANDLE AliasHandle,
                     IN PRPC_SID MemberId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 32 */
NTSTATUS
NTAPI
SamrRemoveMemberFromAlias(IN SAMPR_HANDLE AliasHandle,
                          IN PRPC_SID MemberId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 33 */
NTSTATUS
NTAPI
SamrGetMembersInAlias(IN SAMPR_HANDLE AliasHandle,
                      OUT PSAMPR_PSID_ARRAY_OUT Members)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 34 */
NTSTATUS
NTAPI
SamrOpenUser(IN SAMPR_HANDLE DomainHandle,
             IN unsigned long DesiredAccess,
             IN unsigned long UserId,
             OUT SAMPR_HANDLE *UserHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 35 */
NTSTATUS
NTAPI
SamrDeleteUser(IN OUT SAMPR_HANDLE *UserHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 36 */
NTSTATUS
NTAPI
SamrQueryInformationUser(IN SAMPR_HANDLE UserHandle,
                         IN USER_INFORMATION_CLASS UserInformationClass,
                         OUT PSAMPR_USER_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 37 */
NTSTATUS
NTAPI
SamrSetInformationUser(IN SAMPR_HANDLE UserHandle,
                       IN USER_INFORMATION_CLASS UserInformationClass,
                       IN PSAMPR_USER_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 38 */
NTSTATUS
NTAPI
SamrChangePasswordUser(IN SAMPR_HANDLE UserHandle,
                       IN unsigned char LmPresent,
                       IN PENCRYPTED_LM_OWF_PASSWORD OldLmEncryptedWithNewLm,
                       IN PENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithOldLm,
                       IN unsigned char NtPresent,
                       IN PENCRYPTED_NT_OWF_PASSWORD OldNtEncryptedWithNewNt,
                       IN PENCRYPTED_NT_OWF_PASSWORD NewNtEncryptedWithOldNt,
                       IN unsigned char NtCrossEncryptionPresent,
                       IN PENCRYPTED_NT_OWF_PASSWORD NewNtEncryptedWithNewLm,
                       IN unsigned char LmCrossEncryptionPresent,
                       IN PENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithNewNt)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 39 */
NTSTATUS
NTAPI
SamrGetGroupsForUser(IN SAMPR_HANDLE UserHandle,
                     OUT PSAMPR_GET_GROUPS_BUFFER *Groups)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 40 */
NTSTATUS
NTAPI
SamrQueryDisplayInformation(IN SAMPR_HANDLE DomainHandle,
                            IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                            IN unsigned long Index,
                            IN unsigned long EntryCount,
                            IN unsigned long PreferredMaximumLength,
                            OUT unsigned long *TotalAvailable,
                            OUT unsigned long *TotalReturned,
                            OUT PSAMPR_DISPLAY_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 41 */
NTSTATUS
NTAPI
SamrGetDisplayEnumerationIndex(IN SAMPR_HANDLE DomainHandle,
                               IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                               IN PRPC_UNICODE_STRING Prefix,
                               OUT unsigned long *Index)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 42 */
NTSTATUS
NTAPI
SamrTestPrivateFunctionsDomain(IN SAMPR_HANDLE DomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 43 */
NTSTATUS
NTAPI
SamrTestPrivateFunctionsUser(IN SAMPR_HANDLE UserHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 44 */
NTSTATUS
NTAPI
SamrGetUserDomainPasswordInformation(IN SAMPR_HANDLE UserHandle,
                                     OUT PUSER_DOMAIN_PASSWORD_INFORMATION PasswordInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 45 */
NTSTATUS
NTAPI
SamrRemoveMemberFromForeignDomain(IN SAMPR_HANDLE DomainHandle,
                                  IN PRPC_SID MemberSid)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 46 */
NTSTATUS
NTAPI
SamrQueryInformationDomain2(IN SAMPR_HANDLE DomainHandle,
                            IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                            OUT PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 47 */
NTSTATUS
NTAPI
SamrQueryInformationUser2(IN SAMPR_HANDLE UserHandle,
                          IN USER_INFORMATION_CLASS UserInformationClass,
                          OUT PSAMPR_USER_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 48 */
NTSTATUS
NTAPI
SamrQueryDisplayInformation2(IN SAMPR_HANDLE DomainHandle,
                             IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                             IN unsigned long Index,
                             IN unsigned long EntryCount,
                             IN unsigned long PreferredMaximumLength,
                             OUT unsigned long *TotalAvailable,
                             OUT unsigned long *TotalReturned,
                             OUT PSAMPR_DISPLAY_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 49 */
NTSTATUS
NTAPI
SamrGetDisplayEnumerationIndex2(IN SAMPR_HANDLE DomainHandle,
                                IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                                IN PRPC_UNICODE_STRING Prefix,
                                OUT unsigned long *Index)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 50 */
NTSTATUS
NTAPI
SamrCreateUser2InDomain(IN SAMPR_HANDLE DomainHandle,
                        IN PRPC_UNICODE_STRING Name,
                        IN unsigned long AccountType,
                        IN unsigned long DesiredAccess,
                        OUT SAMPR_HANDLE *UserHandle,
                        OUT unsigned long *GrantedAccess,
                        OUT unsigned long *RelativeId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 51 */
NTSTATUS
NTAPI
SamrQueryDisplayInformation3(IN SAMPR_HANDLE DomainHandle,
                             IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                             IN unsigned long Index,
                             IN unsigned long EntryCount,
                             IN unsigned long PreferredMaximumLength,
                             OUT unsigned long *TotalAvailable,
                             OUT unsigned long *TotalReturned,
                             OUT PSAMPR_DISPLAY_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 52 */
NTSTATUS
NTAPI
SamrAddMultipleMembersToAlias(IN SAMPR_HANDLE AliasHandle,
                              IN PSAMPR_PSID_ARRAY MembersBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 53 */
NTSTATUS
NTAPI
SamrRemoveMultipleMembersFromAlias(IN SAMPR_HANDLE AliasHandle,
                                   IN PSAMPR_PSID_ARRAY MembersBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 54 */
NTSTATUS
NTAPI
SamrOemChangePasswordUser2(IN handle_t BindingHandle,
                           IN PRPC_STRING ServerName,
                           IN PRPC_STRING UserName,
                           IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
                           IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLm)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 55 */
NTSTATUS
NTAPI
SamrUnicodeChangePasswordUser2(IN handle_t BindingHandle,
                               IN PRPC_UNICODE_STRING ServerName,
                               IN PRPC_UNICODE_STRING UserName,
                               IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
                               IN PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
                               IN unsigned char LmPresent,
                               IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
                               IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewNt)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 56 */
NTSTATUS
NTAPI
SamrGetDomainPasswordInformation(IN handle_t BindingHandle,
                                 IN PRPC_UNICODE_STRING Unused,
                                 OUT PUSER_DOMAIN_PASSWORD_INFORMATION PasswordInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 57 */
NTSTATUS
NTAPI
SamrConnect2(IN PSAMPR_SERVER_NAME ServerName,
             OUT SAMPR_HANDLE *ServerHandle,
             IN ACCESS_MASK DesiredAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 58 */
NTSTATUS
NTAPI
SamrSetInformationUser2(IN SAMPR_HANDLE UserHandle,
                        IN USER_INFORMATION_CLASS UserInformationClass,
                        IN PSAMPR_USER_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 59 */
NTSTATUS
NTAPI
SamrSetBootKeyInformation(IN handle_t BindingHandle) /* FIXME */
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 60 */
NTSTATUS
NTAPI
SamrGetBootKeyInformation(IN handle_t BindingHandle) /* FIXME */
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 61 */
NTSTATUS
NTAPI
SamrConnect3(IN handle_t BindingHandle) /* FIXME */
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 62 */
NTSTATUS
NTAPI
SamrConnect4(IN PSAMPR_SERVER_NAME ServerName,
             OUT SAMPR_HANDLE *ServerHandle,
             IN unsigned long ClientRevision,
             IN ACCESS_MASK DesiredAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 63 */
NTSTATUS
NTAPI
SamrUnicodeChangePasswordUser3(IN handle_t BindingHandle) /* FIXME */
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 64 */
NTSTATUS
NTAPI
SamrConnect5(IN PSAMPR_SERVER_NAME ServerName,
             IN ACCESS_MASK DesiredAccess,
             IN unsigned long InVersion,
             IN SAMPR_REVISION_INFO *InRevisionInfo,
             OUT unsigned long *OutVersion,
             OUT SAMPR_REVISION_INFO *OutRevisionInfo,
             OUT SAMPR_HANDLE *ServerHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 65 */
NTSTATUS
NTAPI
SamrRidToSid(IN SAMPR_HANDLE ObjectHandle,
             IN unsigned long Rid,
             OUT PRPC_SID *Sid)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 66 */
NTSTATUS
NTAPI
SamrSetDSRMPassword(IN handle_t BindingHandle,
                    IN PRPC_UNICODE_STRING Unused,
                    IN unsigned long UserId,
                    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 67 */
NTSTATUS
NTAPI
SamrValidatePassword(IN handle_t Handle,
                     IN PASSWORD_POLICY_VALIDATION_TYPE ValidationType,
                     IN PSAM_VALIDATE_INPUT_ARG InputArg,
                     OUT PSAM_VALIDATE_OUTPUT_ARG *OutputArg)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
