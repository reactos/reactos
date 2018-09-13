//+---------------------------------------------------------------------------
//
//  File:       msnssp.h
//
//  Contents:	MSNSSP specific parameters and characteristics
//
//  History:    SudK    Created     7/13/95
//
//----------------------------------------------------------------------------
#ifndef _MSNSSP_H_
#define _MSNSSP_H_

#define MSNSP_NAME_A		"MSN"
#define MSNSP_COMMENT_A		"MSN Security Package"
#define MSNSP_NAME_W		L"MSN"
#define MSNSP_COMMENT_W		L"MSN Security Package"

#ifdef UNICODE
#define MSNSP_NAME  MSNSP_NAME_W
#define MSNSP_COMMENT   MSNSP_COMMENT_W
#else
#define MSNSP_NAME  MSNSP_NAME_A
#define MSNSP_COMMENT   MSNSP_COMMENT_A
#endif

//
//  The following defines the possible values for the 'Type' field in 
//  the security context structure on a server or client application.
//
#define MSNSP_CLIENT_CTXT   0
#define MSNSP_SERVER_CTXT   1

typedef ULONG MSNSP_CTXT_TYPE;

#define MSNSP_CAPABILITIES	(SECPKG_FLAG_CONNECTION |			\
							 SECPKG_FLAG_MULTI_REQUIRED |		\
							 SECPKG_FLAG_TOKEN_ONLY |			\
							 SECPKG_FLAG_INTEGRITY |			\
							 SECPKG_FLAG_PRIVACY)

#define MSNSP_VERSION			1
#define MSNSP_MAX_TOKEN_SIZE	0x0300

#define MSNSP_RPCID			11		// BUGBUG. What should this be?

#define DUMMY_CREDHANDLE	400		// BUGBUG. This should go eventually

#define SEC_E_PRINCIPAL_UNKNOWN SEC_E_UNKNOWN_CREDENTIALS
#define SEC_E_PACKAGE_UNKNOWN SEC_E_SECPKG_NOT_FOUND
//#define SEC_E_BUFFER_TOO_SMALL SEC_E_INSUFFICIENT_MEMORY
//#define SEC_I_CALLBACK_NEEDED SEC_I_CONTINUE_NEEDED
#define SEC_E_INVALID_CONTEXT_REQ SEC_E_NOT_SUPPORTED
#define SEC_E_INVALID_CREDENTIAL_USE SEC_E_NOT_SUPPORTED

#define SSP_RET_REAUTHENTICATION 0x8000000

#endif
