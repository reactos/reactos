/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    utlnpqos.c

Abstract:

    Security component user-mode test.

    Security quality of service test for Local Named Pipes from user mode.

    This test must be run from the SM> prompt in the debugger.

Author:

    Jim Kelly (JimK) 27-June-1990

Revision History:

--*/

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>

#define _TST_USER_  // User mode test

typedef ULONG NAMED_PIPE_TYPE;
typedef NAMED_PIPE_TYPE *PNAMED_PIPE_TYPE;


typedef ULONG READ_MODE;
typedef READ_MODE *PREAD_MODE;

typedef ULONG COMPLETION_MODE;
typedef COMPLETION_MODE *PCOMPLETION_MODE;

typedef ULONG NAMED_PIPE_CONFIGURATION;
typedef NAMED_PIPE_CONFIGURATION *PNAMED_PIPE_CONFIGURATION;

typedef ULONG NAMED_PIPE_STATE;
typedef NAMED_PIPE_STATE *PNAMED_PIPE_STATE;

typedef ULONG NAMED_PIPE_END;
typedef NAMED_PIPE_END *PNAMED_PIPE_END;


#include "tsecomm.c"     // Common routines
#include "ctlnpqos.c"     // quality of service tests





BOOLEAN
Test()
{
    BOOLEAN Result = TRUE;


    Result = CtLnpQos();


    return Result;
}

BOOLEAN
main()
{
    return Test();
}
