/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    uilist.c

Abstract:

    Contains routine to convert a list of workstation names from UI/Service
    list format to API list format

    Contents:
        RtlConvertUiListToApiList
        (NextElement)
        (ValidateName)

Author:

    Richard L Firth (rfirth) 01-May-1992

Environment:

    User mode (makes Windows calls)

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wchar.h>

//
// macros
//

#define IS_DELIMITER(c,_BlankOk) \
    (((c) == L' ' && (_BlankOk)) || \
    ((c) == L'\t') || ((c) == L',') || ((c) == L';'))


//
// prototypes
//

static
ULONG
NextElement(
    IN OUT PWSTR* InputBuffer,
    IN OUT PULONG InputBufferLength,
    OUT PWSTR OutputBuffer,
    IN ULONG OutputBufferLength,
    IN BOOLEAN BlankIsDelimiter
    );

static
BOOLEAN
ValidateName(
    IN  PWSTR Name,
    IN  ULONG Length
    );

//
// functions
//

NTSTATUS
RtlConvertUiListToApiList(
    IN  PUNICODE_STRING UiList OPTIONAL,
    OUT PUNICODE_STRING ApiList,
    IN BOOLEAN BlankIsDelimiter
    )

/*++

Routine Description:

    Converts a list of workstation names in UI/Service format into a list of
    canonicalized names in API list format. UI/Service list format allows
    multiple delimiters, leading and trailing delimiters. Delimiters are the
    set "\t,;". API list format has no leading or trailing delimiters and
    elements are delimited by a single comma character.

    For each name parsed from UiList, the name is canonicalized (which checks
    the character set and name length) as a workstation name. If this fails,
    an error is returned. No information is returned as to which element
    failed canonicalization: the list should be discarded and a new one re-input

Arguments:

    UiList  - The list to canonicalize in UI/Service list format
    ApiList - The place to store the canonicalized version of the list in
              API list format.  The list will have a trailing zero character.
    BlankIsDelimiter - TRUE indicates blank should be considered a delimiter
              character.

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    List converted ok

        Failure = STATUS_INVALID_PARAMETER
                    UiList parameter is in error

                  STATUS_INVALID_COMPUTER_NAME
                    A name parsed from UiList has an incorrect format for a
                    computer (aka workstation) name
--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG inLen;
    PWSTR input;
    PWSTR buffer;
    PWSTR output;
    ULONG cLen;
    ULONG len;
    ULONG outLen = 0;
    WCHAR element[MAX_COMPUTERNAME_LENGTH+1];
    BOOLEAN firstElement = TRUE;
    BOOLEAN ok;

    try {
        if (ARGUMENT_PRESENT(UiList)) {
            inLen = UiList->MaximumLength;  // read memory test
            inLen = UiList->Length;
            input = UiList->Buffer;
            if (inLen & sizeof(WCHAR)-1) {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        RtlInitUnicodeString(ApiList, NULL);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = STATUS_ACCESS_VIOLATION;
    }
    if (NT_SUCCESS(status) && ARGUMENT_PRESENT(UiList) && inLen) {
        buffer = RtlAllocateHeap(RtlProcessHeap(), 0, inLen + sizeof(WCHAR));
        if (buffer == NULL) {
            status = STATUS_NO_MEMORY;
        } else {
            ApiList->Buffer = buffer;
            ApiList->MaximumLength = (USHORT)inLen + sizeof(WCHAR);
            output = buffer;
            ok = TRUE;
            while (len = NextElement(&input,
                                     &inLen,
                                     element,
                                     sizeof(element) - sizeof(element[0]),
                                     BlankIsDelimiter )) {
                if (len == (ULONG)-1L) {
                    ok = FALSE;
                } else {
                    cLen = len/sizeof(WCHAR);
                    element[cLen] = 0;
                    ok = ValidateName(element, cLen);
                }
                if (ok) {
                    if (!firstElement) {
                        *output++ = L',';

                        //
                        // BUGBUG sizeof(L',') returns 4, not 2!!
                        //

//                        outLen += sizeof(L',');
                        outLen += sizeof(WCHAR);
                    } else {
                        firstElement = FALSE;
                    }
                    wcscpy(output, element);
                    outLen += len;
                    output += cLen;
                } else {
                    RtlFreeHeap(RtlProcessHeap(), 0, buffer);
                    ApiList->Buffer = NULL;
                    status = STATUS_INVALID_COMPUTER_NAME;
                    break;
                }
            }
        }
        if (NT_SUCCESS(status)) {
            ApiList->Length = (USHORT)outLen;
            if (!outLen) {
                ApiList->MaximumLength = 0;
                ApiList->Buffer = NULL;
                RtlFreeHeap(RtlProcessHeap(), 0, buffer);
            }
        }
    }
    return status;
}

static
ULONG
NextElement(
    IN OUT PWSTR* InputBuffer,
    IN OUT PULONG InputBufferLength,
    OUT PWSTR OutputBuffer,
    IN ULONG OutputBufferLength,
    IN BOOLEAN BlankIsDelimiter
    )

/*++

Routine Description:

    Locates the next (non-delimter) element in a string and extracts it to a
    buffer. Delimiters are the set [\t,;]

Arguments:

    InputBuffer         - pointer to pointer to input buffer including delimiters
                          Updated on successful return
    InputBufferLength   - pointer to length of characters in InputBuffer.
                          Updated on successful return
    OutputBuffer        - pointer to buffer where next element is copied
    OutputBufferLength  - size of OutputBuffer (in bytes)
    BlankIsDelimiter    - TRUE indicates blank should be considered a delimiter
              character.

Return Value:

    ULONG
                           -1 = error - extracted element breaks OutputBuffer
                            0 = no element extracted (buffer is empty or all
                                delimiters)
        1..OutputBufferLength = OutputBuffer contains extracted element

--*/

{
    ULONG elementLength = 0;
    ULONG inputLength = *InputBufferLength;
    PWSTR input = *InputBuffer;

    while (IS_DELIMITER(*input, BlankIsDelimiter) && inputLength) {
        ++input;
        inputLength -= sizeof(*input);
    }
    while (!IS_DELIMITER(*input, BlankIsDelimiter) && inputLength) {
        if (!OutputBufferLength) {
            return (ULONG)-1L;
        }
        *OutputBuffer++ = *input++;
        OutputBufferLength -= sizeof(*input);
        elementLength += sizeof(*input);
        inputLength -= sizeof(*input);
    }
    *InputBuffer = input;
    *InputBufferLength = inputLength;
    return elementLength;
}

//
// BUGBUG - illegal names characters same as those in net\api. Move to common
// include directory
//

#define ILLEGAL_NAME_CHARS      L"\001\002\003\004\005\006\007" \
                            L"\010\011\012\013\014\015\016\017" \
                            L"\020\021\022\023\024\025\026\027" \
                            L"\030\031\032\033\034\035\036\037" \
                            L"\"/\\[]:|<>+=;,?*"

static
BOOLEAN
ValidateName(
    IN  PWSTR Name,
    IN  ULONG Length
    )

/*++

Routine Description:

    Determines whether a computer name is valid or not

Arguments:

    Name    - pointer to zero terminated wide-character computer name
    Length  - of Name in characters, excluding zero-terminator

Return Value:

    BOOLEAN
        TRUE    Name is valid computer name
        FALSE   Name is not valid computer name

--*/

{
    if (Length > MAX_COMPUTERNAME_LENGTH || Length < 1) {
        return FALSE;
    }

    //
    // Don't allow leading or trailing blanks in the computername.
    //

    if ( Name[0] == ' ' || Name[Length-1] == ' ' ) {
        return(FALSE);
    }

    return (BOOLEAN)((ULONG)wcscspn(Name, ILLEGAL_NAME_CHARS) == Length);
}
