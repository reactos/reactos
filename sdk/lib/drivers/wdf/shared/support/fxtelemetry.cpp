/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTelemetry.cpp

Abstract:

    This module implements a telemetry methods.

Author:



Environment:

    Both kernel and user mode

Revision History:

Notes:

--*/



#include "fxsupportpch.hpp"

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
#include "fxldr.h"
#include <ntstrsafe.h>
#else
#include "DriverFrameworks-UserMode-UmEvents.h"
#include "FxldrUm.h"
#endif

extern "C" {
#if defined(EVENT_TRACING)
#include "FxTelemetry.tmh"
#endif
}

#if defined(__cplusplus)
extern "C" {
#endif





VOID
GetNameFromPath(
    _In_ PCUNICODE_STRING Path,
    _Out_ PUNICODE_STRING Name
    )
/*++

Routine Description:

    Given a potential full path name, return just the filename portion, OR,
    Given a potential full registry path name, return just the subkey portion.

    Pointer to the filename protion in the full path name string, OR,
    Pointer to the subkey portion in the full registry path name.

Arguments:

    Path - Pointer to the full path name string.

    Name - Pointer to receive the name

Return Value:
    None

--*/
{
    BOOLEAN foundSlash;

    ASSERT(Path != NULL);

    //
    // Ideally a check of Path->Length == 0 would be sufficient except that
    // PreFAST thinks that Length can be odd, so it thinks Length == 1 is possible.
    // Comparing Length < sizeof(WCHAR) satisfies PreFAST and keeps the logic
    // at runtime correct.
    //
    if (Path->Length < sizeof(WCHAR)) {
        RtlZeroMemory(Name, sizeof(UNICODE_STRING));
        return;
    }

    //
    // Initialize Name to point to the last WCHAR of the buffer and we will work
    // our way backwards to the beginning of the string or a \
    //

    Name->Buffer = WDF_PTR_ADD_OFFSET_TYPE(Path->Buffer,
                                           Path->Length - sizeof(WCHAR),
                                           PWCH);
    Name->Length = sizeof(WCHAR);
    foundSlash = FALSE;

    while (Name->Buffer >= Path->Buffer) {
        if (*Name->Buffer == L'\\') {
            //
            // Skip the \ in the buffer moving forward a character and adjusting
            // the length
            //
            foundSlash = TRUE;
            Name->Buffer++;
            Name->Length -= sizeof(WCHAR);
            break;
        }

        //
        // Move backwards in the string
        //
        Name->Buffer--;
        Name->Length += sizeof(WCHAR);
    }

    if (foundSlash && Name->Length == 0) {
        //
        // Handle the case where a slash was found and it is the only character
        //
        Name->Buffer = NULL;
    }
    else if (foundSlash == FALSE) {
        //
        // Handle the case where no slash was found. In this case, Name->Buffer
        // points to one WCHAR before the beginning of the string.
        //
        Name->Length -= sizeof(WCHAR);
        Name->Buffer++;
    }

    //
    // Need to set MaximumLength to the same value as Length so that the struct
    // format is valid.
    //
    Name->MaximumLength = Name->Length;
}

#if defined(__cplusplus)
}
#endif

