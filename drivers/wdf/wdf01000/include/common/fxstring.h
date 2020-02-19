#ifndef _FXSTRING_H_
#define _FXSTRING_H_

#include "common/fxobject.h"

class FxString : public FxObject {
public:
    //
    // Length describes the length of the string in bytes (not WCHARs)
    // MaximumLength describes the size of the buffer in bytes
    //
    UNICODE_STRING m_UnicodeString;

public:
    FxString(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    ~FxString();

    __inline
    operator PUNICODE_STRING(
        )
    {
        return &m_UnicodeString;
    }

    PUNICODE_STRING
    __inline
    GetUnicodeString(
        VOID
        )
    {
        return &m_UnicodeString;
    }

    VOID
    __inline
    ReleaseString(
        __out PUNICODE_STRING ReleaseTo
        )
    {
        RtlCopyMemory(ReleaseTo, &m_UnicodeString, sizeof(UNICODE_STRING));
        RtlZeroMemory(&m_UnicodeString, sizeof(m_UnicodeString));
    }

    _Must_inspect_result_
    NTSTATUS
    Assign(
        __in const UNICODE_STRING* UnicodeString
        );

    _Must_inspect_result_
    NTSTATUS
    Assign(
        __in PCWSTR SourceString
        );
};

#endif //_FXSTRING_H_
