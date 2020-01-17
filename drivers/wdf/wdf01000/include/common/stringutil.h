#ifndef _STRINGUTIL_H_
#define _STRINGUTIL_H_

#include "common/fxglobals.h"

_Must_inspect_result_
NTSTATUS
FxDuplicateUnicodeString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in const UNICODE_STRING* Source,
    __out PUNICODE_STRING Destination
    );

#endif //_STRINGUTIL_H_
