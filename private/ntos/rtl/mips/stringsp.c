/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    stingsup.c

Abstract:

    This module defines CPU specific routines for manipulating NT strings.

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:


--*/

#include "nt.h"
#include "ntrtl.h"


VOID
RtlInitAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PSZ SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitAnsiString function initializes an NT counted string.
    The DestinationString is initialized to point to the SourceString
    and the Length and MaximumLength fields of DestinationString are
    initialized to the length of the SourceString, which is zero if
    SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    USHORT Length;
    Length = 0;
    DestinationString->Length = 0;
    DestinationString->Buffer = SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        while (*SourceString++) {
            Length++;
            }

        DestinationString->Length = Length;

        DestinationString->MaximumLength = (SHORT)(Length+1);
        }
    else {
        DestinationString->MaximumLength = 0;
        }
}


VOID
RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    USHORT Length = 0;
    DestinationString->Length = 0;
    DestinationString->Buffer = SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        while (*SourceString++) {
            Length += sizeof(*SourceString);
            }

        DestinationString->Length = Length;

        DestinationString->MaximumLength = Length+(USHORT)sizeof(UNICODE_NULL);
        }
    else {
        DestinationString->MaximumLength = 0;
        }
}
