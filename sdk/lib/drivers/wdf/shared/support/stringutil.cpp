/*++

Copyright (c) Microsoft Corporation

Module Name:

    StringUtil.cpp

Abstract:

    This module implements string utlities in the framework

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "FxSupportPch.hpp"

extern "C"  {
#include "StringUtil.tmh"
}

size_t
FxCalculateTotalStringSize(
    __in FxCollectionInternal *StringCollection,
    __in BOOLEAN Verify,
    __out_opt PBOOLEAN ContainsOnlyStrings
    )
{
    size_t cbLength;
    FxString *pString;
    FxCollectionEntry* cur, *end;

    cbLength = 0;

    end = StringCollection->End();
    for (cur = StringCollection->Start();
         cur != end;
         cur = cur->Next()) {
        pString = (FxString *) cur->m_Object;

        if (Verify && pString->GetType() != FX_TYPE_STRING) {
            *ContainsOnlyStrings = FALSE;
            return 0;
        }

        cbLength += pString->ByteLength(TRUE);
    }

    if (ContainsOnlyStrings != NULL) {
        *ContainsOnlyStrings = TRUE;
    }

    if (StringCollection->Count() == 0) {
        //
        // If there are not entries, we still need 2 NULLs
        //
        cbLength = sizeof(UNICODE_NULL) * 2;
    }
    else {
        //
        // Extra NULL
        //
        cbLength += sizeof(UNICODE_NULL);
    }

    //
    // ASSERT that we are reporting an integral number of WCHARs in bytes
    //
    ASSERT((cbLength % sizeof(WCHAR)) == 0);

    return cbLength;
}

size_t
FxCalculateTotalMultiSzStringSize(
    __in __nullnullterminated PCWSTR MultiSz
    )
{
    PCWSTR pCur;
    size_t cbSize, cbLength;

    cbSize = 0;

    pCur = MultiSz;

    while (*pCur != NULL) {
        //
        // Compute length of string including terminating NULL
        //
        cbLength = (wcslen(pCur) + 1) * sizeof(WCHAR);

        cbSize += cbLength;
        pCur = (PWSTR) WDF_PTR_ADD_OFFSET(pCur, cbLength);
    }

    if (cbSize == 0) {
        //
        // If there are no strings, we still need 2 NULLs
        //
        cbSize = sizeof(UNICODE_NULL);
    }

    //
    // Final NULL which makes this a multi sz
    //
    cbSize += sizeof(UNICODE_NULL);

    //
    // ASSERT that we are reporting an integral number of WCHARs in bytes
    //
    ASSERT((cbSize % sizeof(WCHAR)) == 0);

    return cbSize;
}

#pragma prefast(push)
// Caller is responsible for allocating the correct amount of memory.
#pragma prefast(disable:__WARNING_INCORRECT_ANNOTATION_STRING )
PWSTR
FxCopyMultiSz(
    __out LPWSTR Buffer,
    __in FxCollectionInternal* StringCollection
    )
{
    LPWSTR pCur;
    ULONG length;
    FxCollectionEntry* cur, *end;

    pCur = Buffer;
    end = StringCollection->End();

    for (cur = StringCollection->Start(); cur != end; cur = cur->Next()) {
        FxString* pSourceString;

        pSourceString = (FxString *) cur->m_Object;

        length = pSourceString->ByteLength(TRUE);
        RtlCopyMemory(pCur, pSourceString->Buffer(), length);

        //
        // Length is expressed in number of bytes, not number of
        // characters.
        //
        // length includes the NULL.
        //
        pCur = WDF_PTR_ADD_OFFSET_TYPE(pCur, length, LPWSTR);
    }

    //
    // If there are no entries, we still need 2 NULLs.
    //
    if (StringCollection->Count() == 0) {
        *pCur = UNICODE_NULL;
        pCur++;
    }

    //
    // double NULL terminate the string
    //
    *pCur = UNICODE_NULL;

    //
    // Return the start of the next location in the buffer
    //
    return pCur + 1;
}
#pragma prefast(pop)

_Must_inspect_result_
NTSTATUS
FxDuplicateUnicodeString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in const UNICODE_STRING* Source,
    __out PUNICODE_STRING Destination
    )
/*++

Routine Description:
    Makes a deep copy from Source to Destination.

    Destination is assumed to have been initialized by the caller, be owned by
    an internal Fx object.  Destination could already contain a previously
    allocated buffer.  If one exists and is large enough, it will be reused
    for the copy.

    This function guarantees that the Buffer will be NULL terminated.  While
    this is not necessary because Length describes the length of the buffer, the
    resulting buffer can be copied back to the client driver and we cannot trust
    the client driver to treat the string as an unterminated buffer, typically
    the client driver will just extract the buffer and treat it as NULL
    terminated.  To be defensive with this type of (mis)use, we always NULL
    terminate the resuling Buffer.

Arguments:
    Source - source struct to copy from.  This string can originate from the
             client driver.

    Destination - destination struct to copy to.  This struct is assumed to be
                  internal and is not given to the outside caller

Return Value:
    NTSTATUS

  --*/
{
    NTSTATUS status;
    USHORT srcCbLength, srcCbLengthAndNull, dstMaxCbLength;

    //
    // NOTE: We assume the sources string will be smaller than 64k.
    //
    srcCbLength = Source->Length;
    dstMaxCbLength = Destination->MaximumLength;

    status = RtlUShortAdd(srcCbLength,
                          sizeof(UNICODE_NULL),
                          &srcCbLengthAndNull);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Interger overflow occured when duplicating string %!STATUS!",
            status);
        return status;
    }

    //
    // First see if we already have enough memory to hold the string + a NULL
    //
    if (dstMaxCbLength < srcCbLengthAndNull) {
        //
        // Allocate enough memory for the source string and a NULL character
        //
        dstMaxCbLength = srcCbLengthAndNull;

        //
        // We need to allocate memory.  Free any old memory first.
        //
        if (Destination->Buffer != NULL) {
            FxPoolFree(Destination->Buffer);

            RtlZeroMemory(Destination, sizeof(UNICODE_STRING));
        }

        Destination->Buffer = (PWSTR) FxPoolAllocate(
            FxDriverGlobals, PagedPool, dstMaxCbLength);

        if (Destination->Buffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "Failed to allocate memory when duplicating string %!STATUS!",
                status);
            return status;
        }

        Destination->MaximumLength =  dstMaxCbLength;
    }

    //
    // If we get here and we have a buffer, then we can just copy the
    // string into the buffer.
    //
    RtlCopyMemory(Destination->Buffer, Source->Buffer, srcCbLength);
    Destination->Length = srcCbLength;

    //
    // Make sure the string is NULL terminated and there is room for the NULL
    //
    ASSERT(Destination->Length + sizeof(UNICODE_NULL) <=
                                                    Destination->MaximumLength);
    Destination->Buffer[Destination->Length/sizeof(WCHAR)] = UNICODE_NULL;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
PWCHAR
FxDuplicateUnicodeStringToString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in const UNICODE_STRING* Source
    )
{
    PWSTR pDuplicate;

    pDuplicate = (PWSTR) FxPoolAllocate(
        FxDriverGlobals, PagedPool, Source->Length + sizeof(UNICODE_NULL));

    if (pDuplicate != NULL) {
        RtlCopyMemory(pDuplicate, Source->Buffer, Source->Length);

        //
        // Make sure the string is NULL terminated.  We can safely do this
        // because we allocated an extra WCHAR for the null terminator.
        //
        pDuplicate[Source->Length/sizeof(WCHAR)] = UNICODE_NULL;
    }

    return pDuplicate;
}
