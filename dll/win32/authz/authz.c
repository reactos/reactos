/*
 * ReactOS Authorization Framework
 * Copyright (C) 2005 - 2006 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * PROJECT:         ReactOS Authorization Framework
 * FILE:            lib/authz/authz.c
 * PURPOSE:         Authorization Framework
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      09/30/2005  Created
 */
#include <precomp.h>

HINSTANCE hDllInstance;


/*
 * @unimplemented
 */
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
                 OUT PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE pAuthzHandle)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzAddSidsToContext(IN AUTHZ_CLIENT_CONTEXT_HANDLE OrigClientContext,
                      IN PSID_AND_ATTRIBUTES Sids,
                      IN DWORD SidCount,
                      IN PSID_AND_ATTRIBUTES RestrictedSids,
                      IN DWORD RestrictedSidCount,
                      OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pNewClientContext)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzCachedAccessCheck(IN DWORD Flags,
                       IN AUTHZ_ACCESS_CHECK_RESULTS_HANDLE AuthzHandle,
                       IN PAUTHZ_ACCESS_REQUEST pRequest,
                       IN AUTHZ_AUDIT_EVENT_HANDLE AuditInfo,
                       OUT PAUTHZ_ACCESS_REPLY pReply)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzEnumerateSecurityEventSources(IN DWORD dwFlags,
                                   OUT PAUTHZ_SOURCE_SCHEMA_REGISTRATION Buffer,
                                   OUT PDWORD pdwCount,
                                   IN OUT PDWORD pdwLength)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzFreeAuditEvent(IN AUTHZ_AUDIT_EVENT_HANDLE pAuditEventInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzFreeHandle(IN AUTHZ_ACCESS_CHECK_RESULTS_HANDLE AuthzHandle)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromAuthzContext(IN DWORD flags,
                                       IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzHandle,
                                       IN PLARGE_INTEGER ExpirationTime,
                                       IN LUID Identifier,
                                       IN PVOID DynamicGroupArgs,
                                       OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzHandle)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromToken(IN DWORD Flags,
                                IN HANDLE TokenHandle,
                                IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
                                IN PLARGE_INTEGER pExpirationTime,
                                IN LUID Identifier,
                                IN PVOID DynamicGroupArgs,
                                OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
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
                                      IN DWORD dwAdditionalParamCount)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
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
                                       IN DWORD dwAdditionalParameterCount)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzInstallSecurityEventSource(IN DWORD dwFlags,
                                IN PAUTHZ_SOURCE_SCHEMA_REGISTRATION pRegistration)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
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
                     OUT PAUTHZ_ACCESS_REPLY pReply)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzRegisterSecurityEventSource(IN DWORD dwFlags,
                                 IN PCWSTR szEventSourceName,
                                 IN PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE phEventProvider)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzReportSecurityEvent(IN DWORD dwFlags,
                         IN AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE hEventProvider,
                         IN DWORD dwAuditId,
                         IN PSID pUserSid  OPTIONAL,
                         IN DWORD dwCount,
                         ...)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzReportSecurityEventFromParams(IN DWORD dwFlags,
                                   IN AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE hEventProvider,
                                   IN DWORD dwAuditId,
                                   IN PSID pUserSid  OPTIONAL,
                                   IN PAUDIT_PARAMS pParams)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzUninstallSecurityEventSource(IN DWORD dwFlags,
                                  IN PWSTR szEventSourceName)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
AUTHZAPI
BOOL
WINAPI
AuthzUnregisterSecurityEventSource(IN DWORD dwFlags,
                                   IN OUT PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE phEventProvider)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

