/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxString.cpp

Abstract:

    This module implements a simple string class to operate on
    unicode strings.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "FxSupportPch.hpp"

FxString::FxString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxObject(FX_TYPE_STRING, sizeof(FxString), FxDriverGlobals)
{
    RtlInitUnicodeString(&m_UnicodeString, NULL);
    MarkPassiveDispose(ObjectDoNotLock);
}

FxString::~FxString()
{
    if (m_UnicodeString.Buffer) {
        FxPoolFree(m_UnicodeString.Buffer);
    }
}

_Must_inspect_result_
NTSTATUS
FxString::Assign(
    __in const UNICODE_STRING* UnicodeString
    )
{
    return FxDuplicateUnicodeString(GetDriverGlobals(),
                                    UnicodeString,
                                    &m_UnicodeString);
}

_Must_inspect_result_
NTSTATUS
FxString::Assign(
    __in PCWSTR SourceString
    )

{
    UNICODE_STRING string;

    RtlInitUnicodeString(&string, SourceString);

    return Assign(&string);
}
