/*
 * authz.h
 *
 * Authorization Framework
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __AUTHZ_H
#define __AUTHZ_H

#if !defined(_AUTHZ_)
#define AUTHZAPI DECLSPEC_IMPORT
#else
#define AUTHZAPI DECLSPEC_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AUTHZ_ACCESS_CHECK_NO_DEEP_COPY_SD  0x1

#define AUTHZ_GENERATE_SUCCESS_AUDIT    0x1
#define AUTHZ_GENERATE_FAILURE_AUDIT    0x2

#define AUTHZ_SKIP_TOKEN_GROUPS 0x2
#define AUTHZ_REQUIRE_S4U_LOGON 0x4

#define AUTHZ_NO_SUCCESS_AUDIT  0x1
#define AUTHZ_NO_FAILURE_AUDIT  0x2
#define AUTHZ_NO_ALLOC_STRINGS  0x4

#define AUTHZ_RM_FLAG_NO_AUDIT  0x1
#define AUTHZ_RM_FLAG_INITIALIZE_UNDER_IMPERSONATION    0x2

typedef HANDLE AUTHZ_CLIENT_CONTEXT_HANDLE, *PAUTHZ_CLIENT_CONTEXT_HANDLE;
typedef HANDLE AUTHZ_AUDIT_INFO_HANDLE, *PAUTHZ_AUDIT_INFO_HANDLE;
typedef HANDLE AUTHZ_AUDIT_EVENT_HANDLE, *PAUTHZ_AUDIT_EVENT_HANDLE;
typedef HANDLE AUTHZ_AUDIT_EVENT_TYPE_HANDLE, *PAUTHZ_AUDIT_EVENT_TYPE_HANDLE;
typedef HANDLE AUTHZ_ACCESS_CHECK_RESULTS_HANDLE, *PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE;
typedef HANDLE AUTHZ_RESOURCE_MANAGER_HANDLE, *PAUTHZ_RESOURCE_MANAGER_HANDLE;
typedef HANDLE AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE, *PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE;

#if !defined(_ADTGEN_H)
/* FIXME - AUDIT_PARAMS is defined in adtgen.h!!!!! */
typedef PVOID PAUDIT_PARAMS;
#endif

typedef enum _AUTHZ_CONTEXT_INFORMATION_CLASS
{
    AuthzContextInfoUserSid = 1,
    AuthzContextInfoGroupsSids,
    AuthzContextInfoRestrictedSids,
    AuthzContextInfoPrivileges,
    AuthzContextInfoExpirationTime,
    AuthzContextInfoServerContext,
    AuthzContextInfoIdentifier,
    AuthzContextInfoSource,
    AuthzContextInfoAll,
    AuthzContextInfoAuthenticationId
} AUTHZ_CONTEXT_INFORMATION_CLASS, *PAUTHZ_CONTEXT_INFORMATION_CLASS;

typedef struct _AUTHZ_ACCESS_REQUEST
{
    ACCESS_MASK DesiredAccess;
    PSID PrincipalSelfSid;
    POBJECT_TYPE_LIST ObjectTypeList;
    DWORD ObjectTypeListLength;
    PVOID OptionalArguments;
} AUTHZ_ACCESS_REQUEST, *PAUTHZ_ACCESS_REQUEST;

typedef struct _AUTHZ_ACCESS_REPLY
{
    DWORD ResultListLength;
    PACCESS_MASK GrantedAccessMask;
    PDWORD SaclEvaluationResults;
    PDWORD Error;
} AUTHZ_ACCESS_REPLY, *PAUTHZ_ACCESS_REPLY;

typedef struct _AUTHZ_REGISTRATION_OBJECT_TYPE_NAME_OFFSET
{
    PWSTR szObjectTypeName;
    DWORD dwOffset;
} AUTHZ_REGISTRATION_OBJECT_TYPE_NAME_OFFSET, *PAUTHZ_REGISTRATION_OBJECT_TYPE_NAME_OFFSET;

typedef struct _AUTHZ_SOURCE_SCHEMA_REGISTRATION
{
    DWORD dwFlags;
    PWSTR szEventSourceName;
    PWSTR szEventMessageFile;
    PWSTR szEventSourceXmlSchemaFile;
    PWSTR szEventAccessStringsFile;
    PWSTR szExecutableImagePath;
    PVOID pReserved;
    DWORD dwObjectTypeNameCount;
    AUTHZ_REGISTRATION_OBJECT_TYPE_NAME_OFFSET ObjectTypeNames[ANYSIZE_ARRAY];
} AUTHZ_SOURCE_SCHEMA_REGISTRATION, *PAUTHZ_SOURCE_SCHEMA_REGISTRATION;

typedef BOOL (CALLBACK *PFN_AUTHZ_DYNAMIC_ACCESS_CHECK)(IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
                                                        IN PACE_HEADER pAce,
                                                        IN PVOID pArgs  OPTIONAL,
                                                        IN OUT PBOOL pbAceApplicable);

typedef BOOL (CALLBACK *PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS)(IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
                                                          IN PVOID Args,
                                                          OUT PSID_AND_ATTRIBUTES* pSidAttrArray,
                                                          OUT PDWORD pSidCount,
                                                          OUT PSID_AND_ATTRIBUTES* pRestrictedSidAttrArray,
                                                          OUT PDWORD pRestrictedSidCount);

typedef VOID (CALLBACK *PFN_AUTHZ_FREE_DYNAMIC_GROUPS)(IN PSID_AND_ATTRIBUTES pSidAttrArray);

AUTHZAPI
BOOL
WINAPI
AuthzAccessCheck(IN DWORD flags,
                 IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext,
                 IN PAUTHZ_ACCESS_REQUEST pRequest,
                 IN AUTHZ_AUDIT_INFO_HANDLE AuditInfo,
                 IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
                 IN PSECURITY_DESCRIPTOR* OptionalSecurityDescriptorArray,
                 IN DWORD OptionalSecurityDescriptorCount  OPTIONAL,
                 IN OUT PAUTHZ_ACCESS_REPLY pReply,
                 OUT PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE pAuthzHandle);

AUTHZAPI
BOOL
WINAPI
AuthzAddSidsToContext(IN AUTHZ_CLIENT_CONTEXT_HANDLE OrigClientContext,
                      IN PSID_AND_ATTRIBUTES Sids,
                      IN DWORD SidCount,
                      IN PSID_AND_ATTRIBUTES RestrictedSids,
                      IN DWORD RestrictedSidCount,
                      OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pNewClientContext);

AUTHZAPI
BOOL
WINAPI
AuthzCachedAccessCheck(IN DWORD Flags,
                       IN AUTHZ_ACCESS_CHECK_RESULTS_HANDLE AuthzHandle,
                       IN PAUTHZ_ACCESS_REQUEST pRequest,
                       IN AUTHZ_AUDIT_EVENT_HANDLE AuditInfo,
                       OUT PAUTHZ_ACCESS_REPLY pReply);

AUTHZAPI
BOOL
WINAPI
AuthzEnumerateSecurityEventSources(IN DWORD dwFlags,
                                   OUT PAUTHZ_SOURCE_SCHEMA_REGISTRATION Buffer,
                                   OUT PDWORD pdwCount,
                                   IN OUT PDWORD pdwLength);

AUTHZAPI
BOOL
WINAPI
AuthzFreeAuditEvent(IN AUTHZ_AUDIT_EVENT_HANDLE pAuditEventInfo);

AUTHZAPI
BOOL
WINAPI
AuthzFreeContext(IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext);

AUTHZAPI
BOOL
WINAPI
AuthzFreeHandle(IN AUTHZ_ACCESS_CHECK_RESULTS_HANDLE AuthzHandle);

AUTHZAPI
BOOL
WINAPI
AuthzFreeResourceManager(IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager);

AUTHZAPI
BOOL
WINAPI
AuthzGetInformationFromContext(IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
                               IN AUTHZ_CONTEXT_INFORMATION_CLASS InfoClass,
                               IN DWORD BufferSize,
                               OUT PDWORD pSizeRequired,
                               OUT PVOID Buffer);

AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromAuthzContext(IN DWORD flags,
                                       IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzHandle,
                                       IN PLARGE_INTEGER ExpirationTime,
                                       IN LUID Identifier,
                                       IN PVOID DynamicGroupArgs,
                                       OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzHandle);

AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromSid(IN DWORD Flags,
                              IN PSID UserSid,
                              IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
                              IN PLARGE_INTEGER pExpirationTime,
                              IN LUID Identifier,
                              IN PVOID DynamicGroupArgs,
                              OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext);

AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromToken(IN DWORD Flags,
                                IN HANDLE TokenHandle,
                                IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
                                IN PLARGE_INTEGER pExpirationTime,
                                IN LUID Identifier,
                                IN PVOID DynamicGroupArgs,
                                OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext);

AUTHZAPI
BOOL
WINAPI
AuthzInitializeObjectAccessAuditEvent(IN DWORD Flags,
                                      IN AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType,
                                      IN PWSTR szOperationType,
                                      IN PWSTR szObjectType,
                                      IN PWSTR szObjectName,
                                      IN PWSTR szAdditionalInfo,
                                      OUT PAUTHZ_AUDIT_EVENT_HANDLE phAuditEvent,
                                      IN DWORD dwAdditionalParamCount);

AUTHZAPI
BOOL
WINAPI
AuthzInitializeObjectAccessAuditEvent2(IN DWORD Flags,
                                       IN AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType,
                                       IN PWSTR szOperationType,
                                       IN PWSTR szObjectType,
                                       IN PWSTR szObjectName,
                                       IN PWSTR szAdditionalInfo,
                                       IN PWSTR szAdditionalInfo2,
                                       OUT PAUTHZ_AUDIT_EVENT_HANDLE phAuditEvent,
                                       IN DWORD dwAdditionalParameterCount);

AUTHZAPI
BOOL
WINAPI
AuthzInitializeResourceManager(IN DWORD flags,
                               IN PFN_AUTHZ_DYNAMIC_ACCESS_CHECK pfnAccessCheck,
                               IN PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups,
                               IN PFN_AUTHZ_FREE_DYNAMIC_GROUPS pfnFreeDynamicGroups,
                               IN PCWSTR ResourceManagerName,
                               IN PAUTHZ_RESOURCE_MANAGER_HANDLE pAuthzResourceManager);

AUTHZAPI
BOOL
WINAPI
AuthzInstallSecurityEventSource(IN DWORD dwFlags,
                                IN PAUTHZ_SOURCE_SCHEMA_REGISTRATION pRegistration);

AUTHZAPI
BOOL
WINAPI
AuthzOpenObjectAudit(IN DWORD Flags,
                     IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
                     IN PAUTHZ_ACCESS_REQUEST pRequest,
                     IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent,
                     IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
                     IN PSECURITY_DESCRIPTOR* SecurityDescriptorArray,
                     IN DWORD SecurityDescriptorCount,
                     OUT PAUTHZ_ACCESS_REPLY pReply);

AUTHZAPI
BOOL
WINAPI
AuthzRegisterSecurityEventSource(IN DWORD dwFlags,
                                 IN PCWSTR szEventSourceName,
                                 IN PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE phEventProvider);

AUTHZAPI
BOOL
WINAPI
AuthzReportSecurityEvent(IN DWORD dwFlags,
                         IN AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE hEventProvider,
                         IN DWORD dwAuditId,
                         IN PSID pUserSid  OPTIONAL,
                         IN DWORD dwCount,
                         ...);

AUTHZAPI
BOOL
WINAPI
AuthzReportSecurityEventFromParams(IN DWORD dwFlags,
                                   IN AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE hEventProvider,
                                   IN DWORD dwAuditId,
                                   IN PSID pUserSid  OPTIONAL,
                                   IN PAUDIT_PARAMS pParams);

AUTHZAPI
BOOL
WINAPI
AuthzUninstallSecurityEventSource(IN DWORD dwFlags,
                                  IN PWSTR szEventSourceName);

AUTHZAPI
BOOL
WINAPI
AuthzUnregisterSecurityEventSource(IN DWORD dwFlags,
                                   IN OUT PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE phEventProvider);

#ifdef __cplusplus
}
#endif
#endif /* __AUTHZ_H */
