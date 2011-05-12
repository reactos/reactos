/*
 * Copyright 2011 Samuel Serapión
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include "ntlm.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

CRITICAL_SECTION ContextCritSect;
LIST_ENTRY ValidContextList;

NTSTATUS
NtlmContextInitialize(VOID)
{
    InitializeCriticalSection(&ContextCritSect);
    InitializeListHead(&ValidContextList);

    return STATUS_SUCCESS;
}

VOID
NtlmReferenceContext(IN ULONG_PTR Handle)
{
    PNTLMSSP_CONTEXT Context = (PNTLMSSP_CONTEXT)Handle;

    EnterCriticalSection(&ContextCritSect);

    ASSERT(Context->RefCount > 0);

    /* A context that is not authenticated is only valid for a 
       pre-determined interval */
    if (NtlmIntervalElapsed(Context->StartTime, Context->Timeout))
    {
        if ((Context->State != Authenticated) &&
            (Context->State != AuthenticateSent) &&
            (Context->State != PassedToService))
        {
            ERR("Context %p has timed out\n", Context);
            LeaveCriticalSection(&ContextCritSect);
            return;
        }
    }
    Context->RefCount += 1;
    LeaveCriticalSection(&ContextCritSect);
}

VOID
NtlmDereferenceContext(IN ULONG_PTR Handle)
{
    PNTLMSSP_CONTEXT Context = (PNTLMSSP_CONTEXT)Handle;

    EnterCriticalSection(&ContextCritSect);

    ASSERT(Context->RefCount >= 1);

    Context->RefCount -= 1;

    /* If there are no references free the object */
    if (Context->RefCount == 0)
    {
        ERR("Deleting context %p\n",Context);
        /* free memory */
        NtlmFree(Context);
    }

    LeaveCriticalSection(&ContextCritSect);
}

VOID
NtlmContextTerminate(VOID)
{
    EnterCriticalSection(&ContextCritSect);

    /* dereference all items */
    while (!IsListEmpty(&ValidContextList))
    {
        PNTLMSSP_CONTEXT Context;
        Context = CONTAINING_RECORD(ValidContextList.Flink,
                                    NTLMSSP_CONTEXT,
                                    Entry);

        NtlmDereferenceContext((ULONG_PTR)Context);
    }

    LeaveCriticalSection(&ContextCritSect);

    /* free critical section */
    DeleteCriticalSection(&ContextCritSect);

    return;
}

/* public functions */

SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextW(IN OPTIONAL PCredHandle phCredential,
                           IN OPTIONAL PCtxtHandle phContext,
                           IN OPTIONAL SEC_WCHAR *pszTargetName,
                           IN ULONG fContextReq,
                           IN ULONG Reserved1,
                           IN ULONG TargetDataRep,
                           IN OPTIONAL PSecBufferDesc pInput,
                           IN ULONG Reserved2,
                           IN OUT OPTIONAL PCtxtHandle phNewContext,
                           IN OUT OPTIONAL PSecBufferDesc pOutput,
                           OUT ULONG *pfContextAttr,
                           OUT OPTIONAL PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    SecBuffer inputTokens[2];
    SecBuffer outputTokens[2];
    UCHAR sessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];

    TRACE("%p %p %s 0x%08x %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    if(TargetDataRep == SECURITY_NETWORK_DREP)
        WARN("SECURITY_NETWORK_DREP\n");

    RtlZeroMemory(sessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);

    return ret;
}

SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextA(IN OPTIONAL PCredHandle phCredential,
                           IN OPTIONAL PCtxtHandle phContext,
                           IN OPTIONAL SEC_CHAR *pszTargetName,
                           IN ULONG fContextReq,
                           IN ULONG Reserved1,
                           IN ULONG TargetDataRep,
                           IN OPTIONAL PSecBufferDesc pInput,
                           IN ULONG Reserved2,
                           IN OUT OPTIONAL PCtxtHandle phNewContext,
                           IN OUT OPTIONAL PSecBufferDesc pOutput,
                           OUT ULONG *pfContextAttr,
                           OUT OPTIONAL PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    SEC_WCHAR *target = NULL;

    TRACE("%p %p %s %d %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_a(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    if(pszTargetName != NULL)
    {
        int target_size = MultiByteToWideChar(CP_ACP, 0, pszTargetName,
            strlen(pszTargetName)+1, NULL, 0);
        target = HeapAlloc(GetProcessHeap(), 0, target_size *
                sizeof(SEC_WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pszTargetName, strlen(pszTargetName)+1,
            target, target_size);
    }

    ret = InitializeSecurityContextW(phCredential, phContext, target,
            fContextReq, Reserved1, TargetDataRep, pInput, Reserved2,
            phNewContext, pOutput, pfContextAttr, ptsExpiry);

    HeapFree(GetProcessHeap(), 0, target);
    return ret;
}

SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesW(PCtxtHandle phContext,
                        ULONG ulAttribute,
                        void *pBuffer)
{
    TRACE("%p %d %p\n", phContext, ulAttribute, pBuffer);
    if (!phContext)
        return SEC_E_INVALID_HANDLE;

    UNIMPLEMENTED;

    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesA(PCtxtHandle phContext,
                        ULONG ulAttribute,
                        void *pBuffer)
{
    return QueryContextAttributesW(phContext, ulAttribute, pBuffer);
}

SECURITY_STATUS
SEC_ENTRY
AcceptSecurityContext(IN PCredHandle phCredential,
                      IN OUT PCtxtHandle phContext,
                      IN PSecBufferDesc pInput,
                      IN ULONG fContextReq,
                      IN ULONG TargetDataRep,
                      IN OUT PCtxtHandle phNewContext,
                      IN OUT PSecBufferDesc pOutput,
                      OUT ULONG *pfContextAttr,
                      OUT PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;

    TRACE("%p %p %p %d %d %p %p %p %p\n", phCredential, phContext, pInput,
     fContextReq, TargetDataRep, phNewContext, pOutput, pfContextAttr,
     ptsExpiry);

    UNIMPLEMENTED;

    return ret;
}

SECURITY_STATUS
SEC_ENTRY
DeleteSecurityContext(PCtxtHandle phContext)
{
    if (!phContext)
    {
        return SEC_E_INVALID_HANDLE;
    }

    NtlmDereferenceContext((ULONG_PTR)phContext);
    phContext = NULL;
    return SEC_E_OK;
}

SECURITY_STATUS
SEC_ENTRY
ImpersonateSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              RevertSecurityContext
 */
SECURITY_STATUS
SEC_ENTRY
RevertSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

SECURITY_STATUS
SEC_ENTRY
FreeContextBuffer(PVOID pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
    return SEC_E_OK;
}

SECURITY_STATUS
SEC_ENTRY
ApplyControlToken(IN  PCtxtHandle phContext,
                  IN  PSecBufferDesc pInput)
{

    UNIMPLEMENTED;
    return SEC_E_UNSUPPORTED_FUNCTION;
}
