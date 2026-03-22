/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/dnsapi/dnsapi/context.c
 * PURPOSE:     DNSAPI context handle management.
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              12/15/03 -- Created
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* DnsAcquireContextHandle *************
 * Create a context handle.
 *
 * DWORD CredentialsFlags --            TRUE  -- Unicode
 *                                      FALSE -- Ansi or UTF-8?
 *
 * PVOID Credentials      --            Pointer to a SEC_WINNT_AUTH_IDENTITY
 *                                      TODO: Use it.
 *
 * PHANDLE ContextHandle  --            Pointer to a HANDLE that will receive
 *                                      our context pointer.
 *
 * RETURNS:
 * ERROR_SUCCESS or a failure code.
 */

extern DNS_STATUS WINAPI DnsAcquireContextHandle_UTF8(DWORD CredentialsFlags, PVOID Credentials, HANDLE *ContextHandle);

DNS_STATUS WINAPI
DnsAcquireContextHandle_W(DWORD CredentialsFlags,
                          PVOID Credentials,
                          HANDLE *ContextHandle)
{
    if (CredentialsFlags)
    {
        /* Allocate a trivial opaque token; credentials are not used yet. */
        PVOID Context = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PVOID));

        if (!Context)
        {
            *ContextHandle = 0;
            return ERROR_OUTOFMEMORY;
        }

        *ContextHandle = (HANDLE)Context;
        return ERROR_SUCCESS;
    }
    else
    {
        return DnsAcquireContextHandle_UTF8(CredentialsFlags, Credentials, ContextHandle);
    }
}

DNS_STATUS WINAPI
DnsAcquireContextHandle_UTF8(DWORD CredentialsFlags,
                             PVOID Credentials,
                             HANDLE *ContextHandle)
{
    if (CredentialsFlags)
    {
        return DnsAcquireContextHandle_W(CredentialsFlags, Credentials, ContextHandle);
    }
    else
    {
        /* Convert to unicode, then call the _W version
         * For now, there is no conversion */
        DNS_STATUS Status;

        Status = DnsAcquireContextHandle_W(TRUE, Credentials, /* XXX arty */ ContextHandle);

        /* Free the unicode credentials when they exist. */

        return Status;
    }
}

DNS_STATUS WINAPI
DnsAcquireContextHandle_A(DWORD CredentialFlags,
                          PVOID Credentials,
                          HANDLE *ContextHandle)
{
    if (CredentialFlags)
    {
        return DnsAcquireContextHandle_W(CredentialFlags, Credentials, ContextHandle);
    }
    else
    {
        return DnsAcquireContextHandle_UTF8(CredentialFlags, Credentials, ContextHandle);
    }
}

/* DnsReleaseContextHandle *************
 * Release a context handle, freeing all resources.
 */
void WINAPI
DnsReleaseContextHandle(HANDLE ContextHandle)
{
    if (ContextHandle)
        RtlFreeHeap(RtlGetProcessHeap(), 0, (PVOID)ContextHandle);
}
