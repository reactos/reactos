/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxString.hpp

Abstract:

    This module implements a simple string class to operate on
    unicode strings.

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/
#ifndef _FXSTRING_H_
#define _FXSTRING_H_

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

    VOID
    __inline
    ReleaseString(
        __out PUNICODE_STRING ReleaseTo
        )
    {
        RtlCopyMemory(ReleaseTo, &m_UnicodeString, sizeof(UNICODE_STRING));
        RtlZeroMemory(&m_UnicodeString, sizeof(m_UnicodeString));
    }

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

    _Must_inspect_result_
    NTSTATUS
    Assign(
        __in PCWSTR SourceString
        );

    _Must_inspect_result_
    NTSTATUS
    Assign(
        __in const UNICODE_STRING* UnicodeString
        );

    __inline
    USHORT
    Length(
        VOID
        )
    {
        return m_UnicodeString.Length;
    }

    __inline
    USHORT
    ByteLength(
        __in BOOLEAN IncludeNull
        )
    {
        if (IncludeNull) {
            return m_UnicodeString.Length + sizeof(UNICODE_NULL);
        }
        else {
            return m_UnicodeString.Length;
        }
    }

    __inline
    USHORT
    CharacterLength(
        VOID
        )
    {
        return m_UnicodeString.Length / sizeof(WCHAR);
    }

    __inline
    USHORT
    MaximumLength(
        VOID
        )
    {
        return m_UnicodeString.MaximumLength;
    }

    __inline
    USHORT
    MaximumByteLength(
        VOID
        )
    {
        return m_UnicodeString.MaximumLength;
    }

    __inline
    USHORT
    MaximumCharacterLength(
        VOID
        )
    {
        return m_UnicodeString.MaximumLength / sizeof(WCHAR);
    }

    __inline
    PWCHAR
    Buffer(
        VOID
        )
    {
        return m_UnicodeString.Buffer;
    }
};

#endif // _FXSTRING_H_
