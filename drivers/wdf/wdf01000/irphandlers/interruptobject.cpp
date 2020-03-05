#include "common/fxinterrupt.h"

VOID
FxInterrupt::FilterResourceRequirements(
    __inout PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor
    )
/*++

Routine Description:

    This function allows an interrupt object to change the
    IoResourceRequirementsList that the PnP Manager sends during
    IRP_MN_FILTER_RESOURCE_REQUIREMENTS.  This function takes a single
    IoResourceDescriptor and applies default or user specified policy.

Arguments:

    IoResourceDescriptor - Pointer to descriptor that matches this interrupt object

Return Value:

    VOID

--*/
{
    //
    // Set sharing policy.
    //
    switch (m_ShareVector) {
    case WdfTrue:
        //
        // Override the bus driver's value, explicitly sharing this interrupt.
        //
        IoResourceDescriptor->ShareDisposition = CmResourceShareShared;
        break;

    case WdfFalse:
        //
        // Override the bus driver's value, explicitly claiming this interrupt
        // as non-shared.
        //
        IoResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        break;

    case WdfUseDefault:
    default:
        //
        // Leave the bus driver's value alone.
        //
        break;
    }

    //
    // Apply policy.  Only do this if we are running on an OS which supports
    // the notion of Interrupt Policy and if the policy is not already included
    // by the bus driver based on registry settings.
    //
    /*if (FxLibraryGlobals.IoConnectInterruptEx != NULL &&
        m_SetPolicy &&
        (IoResourceDescriptor->Flags & CM_RESOURCE_INTERRUPT_POLICY_INCLUDED) == 0x0)
    {
        IoResourceDescriptor->Flags |= CM_RESOURCE_INTERRUPT_POLICY_INCLUDED;
        IoResourceDescriptor->u.Interrupt.AffinityPolicy      = (IRQ_DEVICE_POLICY)m_Policy;
        IoResourceDescriptor->u.Interrupt.PriorityPolicy      = (IRQ_PRIORITY)m_Priority;
        IoResourceDescriptor->u.Interrupt.TargetedProcessors  = m_Processors.Mask;
        IoResourceDescriptor->u.Interrupt.Group               = m_Processors.Group;
    }*/
}