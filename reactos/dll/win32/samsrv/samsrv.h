/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (LSA) Server
 * FILE:            reactos/dll/win32/samsrv/samsrv.h
 * PURPOSE:         Common header file
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/umtypes.h>
#include <ddk/ntsam.h>
#include <ntsecapi.h>
#include <sddl.h>

#include <samsrv/samsrv.h>

#include "sam_s.h"

#include <wine/debug.h>

typedef enum _SAM_DB_OBJECT_TYPE
{
    SamDbIgnoreObject,
    SamDbContainerObject,
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
    HANDLE MembersKeyHandle;  // only used by Aliases and Groups
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
    LARGE_INTEGER MaxPasswordAge;
    LARGE_INTEGER MinPasswordAge;
    LARGE_INTEGER ForceLogoff;
    LARGE_INTEGER LockoutDuration;
    LARGE_INTEGER LockoutObservationWindow;
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

/* database.c */

NTSTATUS
SampInitDatabase(VOID);

NTSTATUS
SampCreateDbObject(IN PSAM_DB_OBJECT ParentObject,
                   IN LPWSTR ContainerName,
                   IN LPWSTR ObjectName,
                   IN SAM_DB_OBJECT_TYPE ObjectType,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PSAM_DB_OBJECT *DbObject);

NTSTATUS
SampOpenDbObject(IN PSAM_DB_OBJECT ParentObject,
                 IN LPWSTR ContainerName,
                 IN LPWSTR ObjectName,
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
SampCheckAccountNameInDomain(IN PSAM_DB_OBJECT DomainObject,
                             IN LPWSTR lpAccountName);

NTSTATUS
SampSetAccountNameInDomain(IN PSAM_DB_OBJECT DomainObject,
                           IN LPCWSTR lpContainerName,
                           IN LPCWSTR lpAccountName,
                           IN ULONG ulRelativeId);

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
                             RPC_UNICODE_STRING *String);

/* registry.h */
NTSTATUS
SampRegCloseKey(IN HANDLE KeyHandle);

NTSTATUS
SampRegCreateKey(IN HANDLE ParentKeyHandle,
                 IN LPCWSTR KeyName,
                 IN ACCESS_MASK DesiredAccess,
                 OUT HANDLE KeyHandle);

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
               OUT HANDLE KeyHandle);

NTSTATUS
SampRegQueryKeyInfo(IN HANDLE KeyHandle,
                    OUT PULONG SubKeyCount,
                    OUT PULONG ValueCount);

NTSTATUS
SampRegDeleteValue(IN HANDLE KeyHandle,
                   IN LPWSTR ValueName);

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
                  IN LPWSTR ValueName,
                  OUT PULONG Type OPTIONAL,
                  OUT LPVOID Data OPTIONAL,
                  IN OUT PULONG DataLength OPTIONAL);

NTSTATUS
SampRegSetValue(IN HANDLE KeyHandle,
                IN LPWSTR ValueName,
                IN ULONG Type,
                IN LPVOID Data,
                IN ULONG DataLength);

/* samspc.c */
VOID SampStartRpcServer(VOID);

/* setup.c */
BOOL SampIsSetupRunning(VOID);
BOOL SampInitializeSAM(VOID);