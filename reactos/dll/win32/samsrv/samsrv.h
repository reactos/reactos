/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (LSA) Server
 * FILE:            reactos/dll/win32/samsrv/samsrv.h
 * PURPOSE:         Common header file
 *
 * PROGRAMMERS:     Eric Kohl
 */

#ifndef _SAMSRV_PCH_
#define _SAMSRV_PCH_

#include <stdio.h>
#include <stdlib.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#define NTOS_MODE_USER
#include <ndk/kefuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ddk/ntsam.h>
#include <sddl.h>
#include <sam_s.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(samsrv);

typedef enum _SAM_DB_OBJECT_TYPE
{
    SamDbIgnoreObject,
    SamDbServerObject,
    SamDbDomainObject,
    SamDbAliasObject,
    SamDbGroupObject,
    SamDbUserObject
} SAM_DB_OBJECT_TYPE;

typedef struct _SAM_DB_OBJECT
{
    ULONG Signature;
    SAM_DB_OBJECT_TYPE ObjectType;
    ULONG RefCount;
    ACCESS_MASK Access;
    LPWSTR Name;
    HANDLE KeyHandle;
    HANDLE MembersKeyHandle;  // only used by Aliases
    ULONG RelativeId;
    BOOLEAN Trusted;
    struct _SAM_DB_OBJECT *ParentObject;
} SAM_DB_OBJECT, *PSAM_DB_OBJECT;

#define SAMP_DB_SIGNATURE 0x87654321

typedef struct _SAM_ALIAS_FIXED_DATA
{
    ULONG Version;
    ULONG Reserved;
    ULONG AliasId;
} SAM_ALIAS_FIXED_DATA, *PSAM_ALIAS_FIXED_DATA;

typedef struct _SAM_DOMAIN_FIXED_DATA
{
    ULONG Version;
    ULONG Reserved;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER DomainModifiedCount;
    LARGE_INTEGER MaxPasswordAge;               /* relative Time */
    LARGE_INTEGER MinPasswordAge;               /* relative Time */
    LARGE_INTEGER ForceLogoff;                  /* relative Time */
    LARGE_INTEGER LockoutDuration;              /* relative Time */
    LARGE_INTEGER LockoutObservationWindow;     /* relative Time */
    LARGE_INTEGER ModifiedCountAtLastPromotion;
    ULONG NextRid;
    ULONG PasswordProperties;
    USHORT MinPasswordLength;
    USHORT PasswordHistoryLength;
    USHORT LockoutThreshold;
    DOMAIN_SERVER_ENABLE_STATE DomainServerState;
    DOMAIN_SERVER_ROLE DomainServerRole;
    BOOLEAN UasCompatibilityRequired;
} SAM_DOMAIN_FIXED_DATA, *PSAM_DOMAIN_FIXED_DATA;

typedef struct _SAM_GROUP_FIXED_DATA
{
    ULONG Version;
    ULONG Reserved;
    ULONG GroupId;
    ULONG Attributes;
} SAM_GROUP_FIXED_DATA, *PSAM_GROUP_FIXED_DATA;

typedef struct _SAM_USER_FIXED_DATA
{
    ULONG Version;
    ULONG Reserved;
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER AccountExpires;
    LARGE_INTEGER LastBadPasswordTime;
    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG UserAccountControl;
    USHORT CountryCode;
    USHORT CodePage;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    USHORT AdminCount;
    USHORT OperatorCount;
} SAM_USER_FIXED_DATA, *PSAM_USER_FIXED_DATA;


extern PGENERIC_MAPPING pServerMapping;
extern ENCRYPTED_NT_OWF_PASSWORD EmptyNtHash;
extern ENCRYPTED_LM_OWF_PASSWORD EmptyLmHash;
extern RTL_RESOURCE SampResource;


/* alias.c */

NTSTATUS
SampOpenAliasObject(IN PSAM_DB_OBJECT DomainObject,
                    IN ULONG AliasId,
                    IN ACCESS_MASK DesiredAccess,
                    OUT PSAM_DB_OBJECT *AliasObject);

NTSTATUS
SampAddMemberToAlias(IN PSAM_DB_OBJECT AliasObject,
                     IN PRPC_SID MemberId);

NTSTATUS
NTAPI
SampRemoveMemberFromAlias(IN PSAM_DB_OBJECT AliasObject,
                          IN PRPC_SID MemberId);

NTSTATUS
SampGetMembersInAlias(IN PSAM_DB_OBJECT AliasObject,
                      OUT PULONG MemberCount,
                      OUT PSAMPR_SID_INFORMATION *MemberArray);

NTSTATUS
SampRemoveAllMembersFromAlias(IN PSAM_DB_OBJECT AliasObject);


/* database.c */

NTSTATUS
SampInitDatabase(VOID);

NTSTATUS
SampCreateDbObject(IN PSAM_DB_OBJECT ParentObject,
                   IN LPWSTR ContainerName,
                   IN LPWSTR ObjectName,
                   IN ULONG RelativeId,
                   IN SAM_DB_OBJECT_TYPE ObjectType,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PSAM_DB_OBJECT *DbObject);

NTSTATUS
SampOpenDbObject(IN PSAM_DB_OBJECT ParentObject,
                 IN LPWSTR ContainerName,
                 IN LPWSTR ObjectName,
                 IN ULONG RelativeId,
                 IN SAM_DB_OBJECT_TYPE ObjectType,
                 IN ACCESS_MASK DesiredAccess,
                 OUT PSAM_DB_OBJECT *DbObject);

NTSTATUS
SampValidateDbObject(SAMPR_HANDLE Handle,
                     SAM_DB_OBJECT_TYPE ObjectType,
                     ACCESS_MASK DesiredAccess,
                     PSAM_DB_OBJECT *DbObject);

NTSTATUS
SampCloseDbObject(PSAM_DB_OBJECT DbObject);

NTSTATUS
SampDeleteAccountDbObject(PSAM_DB_OBJECT DbObject);

NTSTATUS
SampSetObjectAttribute(PSAM_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       ULONG AttributeType,
                       LPVOID AttributeData,
                       ULONG AttributeSize);

NTSTATUS
SampGetObjectAttribute(PSAM_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       PULONG AttributeType,
                       LPVOID AttributeData,
                       PULONG AttributeSize);

NTSTATUS
SampGetObjectAttributeString(PSAM_DB_OBJECT DbObject,
                             LPWSTR AttributeName,
                             PRPC_UNICODE_STRING String);

NTSTATUS
SampSetObjectAttributeString(PSAM_DB_OBJECT DbObject,
                             LPWSTR AttributeName,
                             PRPC_UNICODE_STRING String);

/* domain.c */

NTSTATUS
SampSetAccountNameInDomain(IN PSAM_DB_OBJECT DomainObject,
                           IN LPCWSTR lpContainerName,
                           IN LPCWSTR lpAccountName,
                           IN ULONG ulRelativeId);

NTSTATUS
SampRemoveAccountNameFromDomain(IN PSAM_DB_OBJECT DomainObject,
                                IN LPCWSTR lpContainerName,
                                IN LPCWSTR lpAccountName);

NTSTATUS
SampCheckAccountNameInDomain(IN PSAM_DB_OBJECT DomainObject,
                             IN LPCWSTR lpAccountName);

NTSTATUS
SampRemoveMemberFromAllAliases(IN PSAM_DB_OBJECT DomainObject,
                               IN PRPC_SID MemberSid);

NTSTATUS
SampCreateAccountSid(IN PSAM_DB_OBJECT DomainObject,
                     IN ULONG ulRelativeId,
                     IN OUT PSID *AccountSid);

/* group.h */

NTSTATUS
SampOpenGroupObject(IN PSAM_DB_OBJECT DomainObject,
                    IN ULONG GroupId,
                    IN ACCESS_MASK DesiredAccess,
                    OUT PSAM_DB_OBJECT *GroupObject);

NTSTATUS
SampAddMemberToGroup(IN PSAM_DB_OBJECT GroupObject,
                     IN ULONG MemberId);

NTSTATUS
SampRemoveMemberFromGroup(IN PSAM_DB_OBJECT GroupObject,
                          IN ULONG MemberId);


/* registry.h */

NTSTATUS
SampRegCloseKey(IN OUT PHANDLE KeyHandle);

NTSTATUS
SampRegCreateKey(IN HANDLE ParentKeyHandle,
                 IN LPCWSTR KeyName,
                 IN ACCESS_MASK DesiredAccess,
                 OUT PHANDLE KeyHandle);

NTSTATUS
SampRegDeleteKey(IN HANDLE ParentKeyHandle,
                 IN LPCWSTR KeyName);

NTSTATUS
SampRegEnumerateSubKey(IN HANDLE KeyHandle,
                       IN ULONG Index,
                       IN ULONG Length,
                       OUT LPWSTR Buffer);

NTSTATUS
SampRegOpenKey(IN HANDLE ParentKeyHandle,
               IN LPCWSTR KeyName,
               IN ACCESS_MASK DesiredAccess,
               OUT PHANDLE KeyHandle);

NTSTATUS
SampRegQueryKeyInfo(IN HANDLE KeyHandle,
                    OUT PULONG SubKeyCount,
                    OUT PULONG ValueCount);

NTSTATUS
SampRegDeleteValue(IN HANDLE KeyHandle,
                   IN LPCWSTR ValueName);

NTSTATUS
SampRegEnumerateValue(IN HANDLE KeyHandle,
                      IN ULONG Index,
                      OUT LPWSTR Name,
                      IN OUT PULONG NameLength,
                      OUT PULONG Type OPTIONAL,
                      OUT PVOID Data OPTIONAL,
                      IN OUT PULONG DataLength OPTIONAL);

NTSTATUS
SampRegQueryValue(IN HANDLE KeyHandle,
                  IN LPCWSTR ValueName,
                  OUT PULONG Type OPTIONAL,
                  OUT LPVOID Data OPTIONAL,
                  IN OUT PULONG DataLength OPTIONAL);

NTSTATUS
SampRegSetValue(IN HANDLE KeyHandle,
                IN LPCWSTR ValueName,
                IN ULONG Type,
                IN LPVOID Data,
                IN ULONG DataLength);


/* samspc.c */

VOID
SampStartRpcServer(VOID);


/* security.c */

NTSTATUS
SampCreateServerSD(OUT PSECURITY_DESCRIPTOR *ServerSd,
                   OUT PULONG Size);

NTSTATUS
SampCreateBuiltinDomainSD(OUT PSECURITY_DESCRIPTOR *DomainSd,
                          OUT PULONG Size);

NTSTATUS
SampCreateAccountDomainSD(OUT PSECURITY_DESCRIPTOR *DomainSd,
                          OUT PULONG Size);

NTSTATUS
SampCreateAliasSD(OUT PSECURITY_DESCRIPTOR *AliasSd,
                  OUT PULONG Size);

NTSTATUS
SampCreateGroupSD(OUT PSECURITY_DESCRIPTOR *GroupSd,
                  OUT PULONG Size);

NTSTATUS
SampCreateUserSD(IN PSID UserSid,
                 OUT PSECURITY_DESCRIPTOR *UserSd,
                 OUT PULONG Size);

/* setup.c */

BOOL
SampInitializeSAM(VOID);


/* user.c */

NTSTATUS
SampOpenUserObject(IN PSAM_DB_OBJECT DomainObject,
                   IN ULONG UserId,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PSAM_DB_OBJECT *UserObject);

NTSTATUS
SampAddGroupMembershipToUser(IN PSAM_DB_OBJECT UserObject,
                             IN ULONG GroupId,
                             IN ULONG Attributes);

NTSTATUS
SampRemoveGroupMembershipFromUser(IN PSAM_DB_OBJECT UserObject,
                                  IN ULONG GroupId);

NTSTATUS
SampGetUserGroupAttributes(IN PSAM_DB_OBJECT DomainObject,
                           IN ULONG UserId,
                           IN ULONG GroupId,
                           OUT PULONG GroupAttributes);

NTSTATUS
SampSetUserGroupAttributes(IN PSAM_DB_OBJECT DomainObject,
                           IN ULONG UserId,
                           IN ULONG GroupId,
                           IN ULONG GroupAttributes);

NTSTATUS
SampRemoveUserFromAllGroups(IN PSAM_DB_OBJECT UserObject);

NTSTATUS
SampRemoveUserFromAllAliases(IN PSAM_DB_OBJECT UserObject);

NTSTATUS
SampSetUserPassword(IN PSAM_DB_OBJECT UserObject,
                    IN PENCRYPTED_NT_OWF_PASSWORD NtPassword,
                    IN BOOLEAN NtPasswordPresent,
                    IN PENCRYPTED_LM_OWF_PASSWORD LmPassword,
                    IN BOOLEAN LmPasswordPresent);

NTSTATUS
SampGetLogonHoursAttribute(IN PSAM_DB_OBJECT UserObject,
                           IN OUT PSAMPR_LOGON_HOURS LogonHours);

NTSTATUS
SampSetLogonHoursAttribute(IN PSAM_DB_OBJECT UserObject,
                           IN PSAMPR_LOGON_HOURS LogonHours);


/* utils.c */

INT
SampLoadString(HINSTANCE hInstance,
               UINT uId,
               LPWSTR lpBuffer,
               INT nBufferMax);

BOOL
SampIsSetupRunning(VOID);

PSID
AppendRidToSid(PSID SrcSid,
               ULONG Rid);

NTSTATUS
SampGetRidFromSid(IN PSID Sid,
                  OUT PULONG Rid);

NTSTATUS
SampCheckAccountName(IN PRPC_UNICODE_STRING AccountName,
                     IN USHORT MaxLength);


/* Undocumented advapi32 functions */

NTSTATUS
WINAPI
SystemFunction006(LPCSTR password,
                  LPSTR hash);

NTSTATUS
WINAPI
SystemFunction007(PUNICODE_STRING string,
                  LPBYTE hash);

NTSTATUS
WINAPI
SystemFunction013(const BYTE *in,
                  const BYTE *key,
                  LPBYTE out);

#endif /* _SAMSRV_PCH_ */
