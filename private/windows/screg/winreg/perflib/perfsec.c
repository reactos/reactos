/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1994   Microsoft Corporation

Module Name:

    perfsec.c

Abstract:

    This file implements the _access checking functions used by the
    performance registry API's

Author:

    Bob Watson (a-robw)

Revision History:

    8-Mar-95    Created (and extracted from Perflib.c

--*/
#define UNICODE
//
//  Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "ntconreg.h"
#include "perfsec.h"

#define INITIAL_SID_BUFFER_SIZE     4096
#define FREE_IF_ALLOC(x)    if ((x) != NULL) {FREEMEM(x);}

BOOL
TestTokenForPriv(
    HANDLE hToken,
	LPTSTR	szPrivName
)
/***************************************************************************\
* TestTokenForPriv
*
* Returns TRUE if the token passed has the specified privilege
*
* The token handle passed must have TOKEN_QUERY access.
*
* History:
* 03-07-95 a-robw		Created
\***************************************************************************/
{
	BOOL		bStatus;
	LUID		PrivLuid;
	PRIVILEGE_SET	PrivSet;
	LUID_AND_ATTRIBUTES	PrivLAndA[1];

	BOOL		bReturn = FALSE;

	// get value of priv

	bStatus = LookupPrivilegeValue (
		NULL,
		szPrivName,
		&PrivLuid);

	if (!bStatus) {
		// unable to lookup privilege
		goto Exit_Point;
	}

	// build Privilege Set for function call

	PrivLAndA[0].Luid = PrivLuid;
	PrivLAndA[0].Attributes = 0;

	PrivSet.PrivilegeCount = 1;
	PrivSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
	PrivSet.Privilege[0] = PrivLAndA[0];

	// check for the specified priv in the token

	bStatus = PrivilegeCheck (
		hToken,
		&PrivSet,
		&bReturn);

	if (bStatus) {
		SetLastError (ERROR_SUCCESS);
	}

    //
    // Tidy up
    //
Exit_Point:

    return(bReturn);
}

BOOL
TestClientForPriv (
	BOOL	*pbThread,
	LPTSTR	szPrivName
)
/***************************************************************************\
* TestClientForPriv
*
* Returns TRUE if our client has the specified privilege
* Otherwise, returns FALSE.
*
\***************************************************************************/
{
    BOOL bResult;
    BOOL bIgnore;
	DWORD	dwLastError;

	BOOL	bThreadFlag = FALSE; // assume data is from process or an error occurred

    HANDLE hClient;

	SetLastError (ERROR_SUCCESS);

    bResult = OpenThreadToken(GetCurrentThread(),	// This Thread
                             TOKEN_QUERY,           	//DesiredAccess
							 FALSE,					// use context of calling thread
                             &hClient);           	//TokenHandle
    if (!bResult) {
		// unable to get a Thread Token, try a Process Token
	    bResult = OpenProcessToken(GetCurrentProcess(),	// This Process
                             TOKEN_QUERY,           	//DesiredAccess
                             &hClient);           		//TokenHandle
	} else {
		// data is from current THREAD
		bThreadFlag = TRUE;
	}

    if (bResult) {
		try {
        	bResult = TestTokenForPriv( hClient, szPrivName );
        } except (EXCEPTION_EXECUTE_HANDLER) {
			bResult = FALSE;
		}
        bIgnore = CloseHandle( hClient );
        ASSERT(bIgnore == TRUE);
	} else {
		dwLastError = GetLastError ();
	}

	// set thread flag if present
	if (pbThread != NULL) {
		try {
			*pbThread = bThreadFlag;
        } except (EXCEPTION_EXECUTE_HANDLER) {
			SetLastError (ERROR_INVALID_PARAMETER);
		}
	}

    return(bResult);
}

LONG
GetProcessNameColMeth (
    VOID
)
{
    NTSTATUS            Status;
    HANDLE              hPerflibKey;
    OBJECT_ATTRIBUTES   oaPerflibKey;
    ACCESS_MASK         amPerflibKey;
    UNICODE_STRING      PerflibSubKeyString;
    UNICODE_STRING      NameInfoValueString;
    LONG                lReturn = PNCM_SYSTEM_INFO;
    PKEY_VALUE_PARTIAL_INFORMATION    pKeyInfo;
    DWORD               dwBufLen;
    DWORD               dwRetBufLen;
    PDWORD              pdwValue;

    RtlInitUnicodeString (
        &PerflibSubKeyString,
        L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");

    InitializeObjectAttributes(
            &oaPerflibKey,
            &PerflibSubKeyString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    Status = NtOpenKey(
                &hPerflibKey,
                MAXIMUM_ALLOWED,
                &oaPerflibKey
                );

    if (NT_SUCCESS (Status)) {
        // registry key opened, now read value.
        // allocate enough room for the structure, - the last
        // UCHAR in the struct, but + the data buffer (a dword)

        dwBufLen = sizeof(KEY_VALUE_PARTIAL_INFORMATION) -
            sizeof(UCHAR) + sizeof (DWORD);

        pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ALLOCMEM (dwBufLen);

        if (pKeyInfo != NULL) {
            // initialize value name string
            RtlInitUnicodeString (
                &NameInfoValueString,
                L"CollectUnicodeProcessNames");

            dwRetBufLen = 0;
            Status = NtQueryValueKey (
                hPerflibKey,
                &NameInfoValueString,
                KeyValuePartialInformation,
                (PVOID)pKeyInfo,
                dwBufLen,
                &dwRetBufLen);

            if (NT_SUCCESS(Status)) {
                // check value of return data buffer
                pdwValue = (PDWORD)&pKeyInfo->Data[0];
                if (*pdwValue == PNCM_MODULE_FILE) {
                    lReturn = PNCM_MODULE_FILE;
                } else {
                    // all other values will cause this routine to return
                    // the default value of PNCM_SYSTEM_INFO;
                }
            }

            FREEMEM (pKeyInfo);
        }
        // close handle
        NtClose (hPerflibKey);
    }

    return lReturn;
}

LONG
GetPerfDataAccess (
    VOID
)
{
    NTSTATUS            Status;
    HANDLE              hPerflibKey;
    OBJECT_ATTRIBUTES   oaPerflibKey;
    ACCESS_MASK         amPerflibKey;
    UNICODE_STRING      PerflibSubKeyString;
    UNICODE_STRING      NameInfoValueString;
    LONG                lReturn = CPSR_EVERYONE;
    PKEY_VALUE_PARTIAL_INFORMATION    pKeyInfo;
    DWORD               dwBufLen;
    DWORD               dwRetBufLen;
    PDWORD              pdwValue;

    RtlInitUnicodeString (
        &PerflibSubKeyString,
        L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");

    InitializeObjectAttributes(
            &oaPerflibKey,
            &PerflibSubKeyString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    Status = NtOpenKey(
                &hPerflibKey,
                MAXIMUM_ALLOWED,
                &oaPerflibKey
                );

    if (NT_SUCCESS (Status)) {
        // registry key opened, now read value.
        // allocate enough room for the structure, - the last
        // UCHAR in the struct, but + the data buffer (a dword)

        dwBufLen = sizeof(KEY_VALUE_PARTIAL_INFORMATION) -
            sizeof(UCHAR) + sizeof (DWORD);

        pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ALLOCMEM (dwBufLen);

        if (pKeyInfo != NULL) {

            // see if the user right should be checked

            // init value name string
            RtlInitUnicodeString (
                &NameInfoValueString,
                L"CheckProfileSystemRight");

            dwRetBufLen = 0;
            Status = NtQueryValueKey (
                hPerflibKey,
                &NameInfoValueString,
                KeyValuePartialInformation,
                (PVOID)pKeyInfo,
                dwBufLen,
                &dwRetBufLen);

            if (NT_SUCCESS(Status)) {
                // check value of return data buffer
                pdwValue = (PDWORD)&pKeyInfo->Data[0];
                if (*pdwValue == CPSR_CHECK_ENABLED) {
                    lReturn = CPSR_CHECK_PRIVS;
                } else {
                    // all other values will cause this routine to return
                    // the default value of CPSR_EVERYONE
                }
            }

            FREEMEM (pKeyInfo);
        }
        // close handle
        NtClose (hPerflibKey);
    }

    return lReturn;
}

BOOL
TestClientForAccess ( 
    VOID
)
/***************************************************************************\
* TestClientForAccess
*
* Returns TRUE if our client is allowed to read the perflib key.
* Otherwise, returns FALSE.
*
\***************************************************************************/
{
    HKEY hKeyPerflib;
    DWORD   dwStatus;
    BOOL bResult = FALSE;

    dwStatus = RegOpenKeyExW(
       HKEY_LOCAL_MACHINE,
       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib",
       0L,
       KEY_READ,
       & hKeyPerflib);

    if (dwStatus == ERROR_SUCCESS) {
        RegCloseKey(hKeyPerflib);
        bResult = TRUE;
    }

    return (bResult);
}
