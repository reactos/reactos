#include "common/fxstring.h"
#include "common/stringutil.h"


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