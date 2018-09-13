/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    advapi.h

Abstract:

    This module contains private function prototypes
    and types for the advanced 32-bit windows base APIs.

Author:

    Mark Lucovsky (markl) 18-Sep-1990

Revision History:

--*/

#ifndef _ADVAPI_
#define _ADVAPI_

#undef UNICODE

//
// get thunks right
//

#ifndef _ADVAPI32_
#define _ADVAPI32_
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
//
// Include Common Definitions.
//

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    );


extern RTL_CRITICAL_SECTION Logon32Lock ;

#ifdef __cplusplus
} // extern "C"
#endif

#endif _ADVAPI_
