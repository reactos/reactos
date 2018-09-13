/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Support.c

Abstract:

    This module contains support functions for the client side of the
    Win32 Registry APIs. That is:

        - MakeSemiUniqueName

Author:

    David J. Gilman (davegi) 15-Nov-1991

--*/

#include <rpc.h>
#include "regrpc.h"
#include <stdio.h>


#define REG_SUNAME_FORMAT_STRING    "Win32Reg.%08x.%08x"



BOOL
MakeSemiUniqueName (
    OUT PUNICODE_STRING     Name,
    IN  DWORD               Sequence
    )
/*++

Routine Description:

    Forms a name that is very probably unique in the system, based on
    the current process and thread id and a sequence provided by the
    caller.

Arguments:

    Name        -   Supplies a unicode string where the name will be put.
                    This string must contain a valid buffer of size
                    MAX_PATH * sizeof(WCHAR)

    Sequence    -   Supplies a sequence number that will be appended to
                    the name. If a name happens not to be unique, the
                    caller can try again with other sequence numbers.

Return Value:

    BOOL - Returns TRUE if a name was obtained.

--*/
{
    CHAR            NameBuffer[ MAX_PATH ];
    ANSI_STRING     AnsiName;
    NTSTATUS        NtStatus;

    ASSERT( Name && Name->Buffer );

    sprintf( NameBuffer,
             REG_SUNAME_FORMAT_STRING,
             HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess),
             Sequence
           );


    RtlInitAnsiString( &AnsiName, NameBuffer );

    NtStatus = RtlAnsiStringToUnicodeString(
                    Name,
                    &AnsiName,
                    FALSE
                    );

    return NT_SUCCESS( NtStatus );
}
