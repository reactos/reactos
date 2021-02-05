//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "fxsupportpch.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "FxRegKey.tmh"
#endif
}

_Must_inspect_result_
NTSTATUS
FxRegKey::_VerifyMultiSzString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING RegValueName,
    __in_bcount(DataLength) PWCHAR DataString,
    __in ULONG DataLength
    )
/*++

Routine Description:
    This static function checks the input buffer to verify that it contains
    a multi-sz string with a double-NULL termination at the end of the buffer.

Arguments:
    DataString - buffer containing multi-sz strings. If there are no strings
        in the buffer, the buffer should at least contain two UNICODE_NULL
        characters.

    DataLength - the size in bytes of the input buffer.

Return Value:
    STATUS_OBJECT_TYPE_MISMATCH - if the the data buffer is off odd-length,
        or it doesnt end with two UNICODE_NULL characters.

    STATUS_SUCCESS - if the buffer contains valid multi-sz strings.

  --*/
{
    ULONG numChars;

    if ((DataLength % 2) != 0) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Reg value name %wZ, DataLength %d, Data buffer length is invalid, "
            "STATUS_OBJECT_TYPE_MISMATCH",
            RegValueName, DataLength);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    numChars = DataLength / sizeof(WCHAR);
    if (numChars < 2 ||
        DataString[numChars-1] != UNICODE_NULL ||
        DataString[numChars-2] != UNICODE_NULL) {

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Read value name %wZ, DataLength %d, Data buffer from registry does not "
            "have double NULL terminal chars, STATUS_OBJECT_TYPE_MISMATCH",
            RegValueName, DataLength);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    return STATUS_SUCCESS;
}
