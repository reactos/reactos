/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1994   Microsoft Corporation

Module Name:

    perfsec.h

Abstract:

    This file implements the access checking functions definitions used by the
    performance registry API's

Author:

    Bob Watson (a-robw)

Revision History:

    8-Mar-95    Created (and extracted from Perflib.c

--*/
#ifndef _PERFSEC_H_
#define _PERFSEC_H_
//
//  Value to decide if process names should be collected from:
//      the SystemProcessInfo structure (fastest)
//          -- or --
//      the process's image file (slower, but shows Unicode filenames)
//
#define PNCM_NOT_DEFINED    ((LONG)-1)
#define PNCM_SYSTEM_INFO    0L
#define PNCM_MODULE_FILE    1L
//
//  Value to decide if the SE_PROFILE_SYSTEM_NAME priv should be checked
//
#define CPSR_NOT_DEFINED    ((LONG)-1)
#define CPSR_EVERYONE       0L
#define CPSR_CHECK_ENABLED  1L
#define CPSR_CHECK_PRIVS    1L

BOOL
TestClientForPriv (
	BOOL	*pbThread,
	LPTSTR	szPrivName
);

BOOL
TestClientForAccess ( 
    VOID
);

LONG
GetProcessNameColMeth (
    VOID
);

LONG
GetPerfDataAccess (
    VOID
);

#endif // _PERFSEC_H_



