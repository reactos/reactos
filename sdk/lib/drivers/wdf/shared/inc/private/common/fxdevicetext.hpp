/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceText.hpp

Abstract:

    This module implements the device text object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXDEVICETEXT_H_
#define _FXDEVICETEXT_H_

struct  FxDeviceText : public FxStump {
    SINGLE_LIST_ENTRY m_Entry;
    PWCHAR m_Description;
    PWCHAR m_LocationInformation;
    LCID m_LocaleId;

    FxDeviceText(
        VOID
        );

    ~FxDeviceText(
        VOID
        );

    static
    FxDeviceText*
    _FromEntry(
        __in PSINGLE_LIST_ENTRY Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxDeviceText, m_Entry);
    }

    static
    _CleanupList(
        __inout PSINGLE_LIST_ENTRY Head
        )
    {
        PSINGLE_LIST_ENTRY ple;

        ple = Head->Next;

        if (ple != NULL) {
            FxDeviceText* pText;

            pText = FxDeviceText::_FromEntry(ple);
            ple = ple->Next;

            //
            // Destructor verifies the entry is not on any list
            //
            pText->m_Entry.Next = NULL;
            delete pText;
        }

        Head->Next = NULL;
    }

    VOID
    operator delete(
        __in PVOID Pool
        )
    {
        FxPoolFree(Pool);
    }
};

#endif // _FXDEVICETEXT_H_
