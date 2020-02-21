#include "common/fxdeviceinterface.h"


FxDeviceInterface::FxDeviceInterface(
    )
/*++

Routine Description:
    Constructor for the object.  Initializes all fields

Arguments:
    None

Return Value:
    None

  --*/
{
    RtlZeroMemory(&m_InterfaceClassGUID, sizeof(m_InterfaceClassGUID));

    RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
    RtlZeroMemory(&m_ReferenceString, sizeof(m_ReferenceString));

    m_Entry.Next = NULL;

    m_State = FALSE;
}

FxDeviceInterface::~FxDeviceInterface()
/*++

Routine Description:
    Destructor for FxDeviceInterface.  Cleans up any allocations previously
    allocated.

Arguments:
    None

Return Value:
    None

  --*/
{
    // the device interface should be off now
    ASSERT(m_State == FALSE);

    // should no longer be in any list
    ASSERT(m_Entry.Next == NULL);

    if (m_ReferenceString.Buffer != NULL)
    {
        FxPoolFree(m_ReferenceString.Buffer);
        RtlZeroMemory(&m_ReferenceString, sizeof(m_ReferenceString));
    }

    if (m_SymbolicLinkName.Buffer != NULL)
    {
        RtlFreeUnicodeString(&m_SymbolicLinkName);
    }
}