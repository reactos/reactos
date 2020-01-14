#ifndef _FXDEVICETEXT_H_
#define _FXDEVICETEXT_H_

#include "common/fxstump.h"

struct  FxDeviceText : public FxStump {

    SINGLE_LIST_ENTRY m_Entry;
    PWCHAR m_Description;
    PWCHAR m_LocationInformation;
    LCID m_LocaleId;

    FxDeviceText(
        VOID
        ) :
    m_Description(NULL),
    m_LocationInformation(NULL),
    m_LocaleId(0)
    {
        m_Entry.Next = NULL;
    }

    ~FxDeviceText(
        VOID
        )
    {
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

        ASSERT(m_Entry.Next == NULL);

        if (m_Description != NULL)
        {
            FxPoolFree(m_Description);
            m_Description = NULL;
        }

        if (m_LocationInformation != NULL)
        {
            FxPoolFree(m_LocationInformation);
            m_LocationInformation = NULL;
        }
    }

    static
    FxDeviceText*
    _FromEntry(
        __in PSINGLE_LIST_ENTRY Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxDeviceText, m_Entry);
    }

    static
    VOID
    _CleanupList(
        __inout PSINGLE_LIST_ENTRY Head
        )
    {
        PSINGLE_LIST_ENTRY ple;

        ple = Head->Next;

        if (ple != NULL)
        {
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

#endif //_FXDEVICETEXT_H_
