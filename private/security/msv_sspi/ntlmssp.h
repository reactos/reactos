/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nllmssp.h

Abstract:

    Externally visible definition of the NT Lanman Security Support Provider
    (NtLmSsp) Service.

Author:

    Cliff Van Dyke (cliffv) 01-Jul-1993

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    Borrowed from the Ciaro's ntlmssp.h by PeterWi.

    ChandanS  03-Aug-1996  Stolen from net\svcdlls\ntlmssp\ntlmssp.h
--*/

#ifndef _NTLMSSP_
#define _NTLMSSP_

//
// Defines for SecPkgInfo structure returned by SpGetInfo
//

#define NTLMSP_NAME           L"NTLM"
#define NTLMSP_COMMENT        L"NTLM Security Package"
#define NTLMSP_CAPS           (SECPKG_FLAG_TOKEN_ONLY | \
                               SECPKG_FLAG_MULTI_REQUIRED | \
                               SECPKG_FLAG_CONNECTION | \
                               SECPKG_FLAG_INTEGRITY | \
                               SECPKG_FLAG_PRIVACY | \
                               SECPKG_FLAG_IMPERSONATION | \
                               SECPKG_FLAG_ACCEPT_WIN32_NAME | \
                               SECPKG_FLAG_NEGOTIABLE | \
                               SECPKG_FLAG_LOGON )

#define NTLMSP_MAX_TOKEN_SIZE  0x770
#define NTLM_CRED_NULLSESSION  SECPKG_CRED_RESERVED

// includes that should go elsewhere.

//
// Move to secscode.h
//

#define SEC_E_PACKAGE_UNKNOWN SEC_E_SECPKG_NOT_FOUND
#define SEC_E_INVALID_CONTEXT_REQ SEC_E_NOT_SUPPORTED
#define SEC_E_INVALID_CREDENTIAL_USE SEC_E_NOT_SUPPORTED
#define SEC_I_CALL_NTLMSSP_SERVICE 0xFFFFFFFF

//
// Could be in sspi.h
//

#define SSP_RET_REAUTHENTICATION 0x8000000

#endif // _NTLMSSP_
