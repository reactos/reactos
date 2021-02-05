/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxAutoString.hpp

Abstract:

    This is the C++ header for FxAutoString which represents a UNICODE_STRING
    and follows the RAII (resource acquisiion is initialization) pattern where
    it frees the buffer when the struct goes out of scope.

Author:



Revision History:



--*/

#ifndef _FXAUTOSTRING_H_
#define _FXAUTOSTRING_H_

struct FxAutoString {
    FxAutoString(
        VOID
        )
    {
        RtlZeroMemory(&m_UnicodeString, sizeof(m_UnicodeString));
    }

    ~FxAutoString(
        VOID
        )
    {
        if (m_UnicodeString.Buffer != NULL) {
#if _WDFLDR_
            ExFreePool(m_UnicodeString.Buffer);
#else
            FxPoolFree(m_UnicodeString.Buffer);
#endif
            RtlZeroMemory(&m_UnicodeString, sizeof(m_UnicodeString));
        }
    }

    UNICODE_STRING m_UnicodeString;
};

#endif // _FXAUTOSTRING_H_
