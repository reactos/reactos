//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _STRINGUTIL_H_
#define _STRINGUTIL_H_

size_t
FxCalculateTotalStringSize(
    __in FxCollectionInternal *StringCollection,
    __in BOOLEAN Verify = FALSE,
    __out_opt PBOOLEAN ContainsOnlyStrings = NULL
    );

size_t
FxCalculateTotalMultiSzStringSize(
    __in __nullnullterminated PCWSTR MultiSz
    );

PWSTR
FxCopyMultiSz(
    __out LPWSTR Buffer,
    __in FxCollectionInternal* StringCollection
    );

_Must_inspect_result_
NTSTATUS
FxDuplicateUnicodeString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in const UNICODE_STRING* Source,
    __out PUNICODE_STRING Destination
    );

_Must_inspect_result_
PWCHAR
FxDuplicateUnicodeStringToString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in const UNICODE_STRING* Source
    );


#endif //  _STRINGUTIL_H_
