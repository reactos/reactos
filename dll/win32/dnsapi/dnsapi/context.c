/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/dnsapi/dnsapi/context.c
 * PURPOSE:     DNSAPI functions built on the ADNS library.
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              12/15/03 -- Created
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* DnsAcquireContextHandle *************
 * Create a context handle that will allow us to open and retrieve queries.
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
 * TODO: Which ones area allowed?
 */

extern DNS_STATUS WINAPI DnsAcquireContextHandle_UTF8
( DWORD CredentialsFlags,
  PVOID Credentials,
  HANDLE *ContextHandle );

DNS_STATUS WINAPI DnsAcquireContextHandle_W
( DWORD CredentialsFlags,
  PVOID Credentials,
  HANDLE *ContextHandle ) {
  if( CredentialsFlags ) {
    PWINDNS_CONTEXT Context;
    int adns_status;

    /* For now, don't worry about the user's identity. */
    Context = (PWINDNS_CONTEXT)RtlAllocateHeap( RtlGetProcessHeap(), 0,
						sizeof( WINDNS_CONTEXT ) );
    /* The real work here is to create an adns_state that will help us
     * do what we want to later. */
    adns_status = adns_init( &Context->State,
			     adns_if_noenv |
			     adns_if_noerrprint |
			     adns_if_noserverwarn,
			     0 );
    if( adns_status != adns_s_ok ) {
      *ContextHandle = 0;
      return DnsIntTranslateAdnsToDNS_STATUS( adns_status );
    } else {
      *ContextHandle = (HANDLE)Context;
      return ERROR_SUCCESS;
    }
  } else {
    return DnsAcquireContextHandle_UTF8( CredentialsFlags,
					 Credentials,
					 ContextHandle );
  }
}

DNS_STATUS WINAPI DnsAcquireContextHandle_UTF8
( DWORD CredentialsFlags,
  PVOID Credentials,
  HANDLE *ContextHandle ) {
  if( CredentialsFlags ) {
    return DnsAcquireContextHandle_W( CredentialsFlags,
				      Credentials,
				      ContextHandle );
  } else {
    /* Convert to unicode, then call the _W version
     * For now, there is no conversion */
    DNS_STATUS Status;

    Status = DnsAcquireContextHandle_W( TRUE,
					Credentials, /* XXX arty */
					ContextHandle );

    /* Free the unicode credentials when they exist. */

    return Status;
  }
}

DNS_STATUS WINAPI DnsAcquireContextHandle_A
( DWORD CredentialFlags,
  PVOID Credentials,
  HANDLE *ContextHandle ) {
  if( CredentialFlags ) {
    return DnsAcquireContextHandle_W( CredentialFlags,
				      Credentials,
				      ContextHandle );
  } else {
    return DnsAcquireContextHandle_UTF8( CredentialFlags,
					 Credentials,
					 ContextHandle );
  }
}
/* DnsReleaseContextHandle *************
 * Release a context handle, freeing all resources.
 */

void WINAPI DnsReleaseContextHandle
( HANDLE ContextHandle ) {
  PWINDNS_CONTEXT Context = (PWINDNS_CONTEXT)ContextHandle;
  adns_finish( Context->State );
  RtlFreeHeap( RtlGetProcessHeap(), 0, Context );
}

