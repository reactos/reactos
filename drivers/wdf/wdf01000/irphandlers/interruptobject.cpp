#include "common/fxinterrupt.h"
#include "common/fxtelemetry.h"

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

VOID
FxInterrupt::Reset(
    VOID
    )
/*++

Routine Description:
    Resets the interrupt info and synchronize irql for the interrupt.  The pnp
    state machine will call this function every time new resources are assigned
    to the device.

Arguments:
    None

Return Value:
    None

  --*/
{
    //
    // Other values in m_InterruptInfo survive a reset, so RtlZeroMemory is not
    // an option.  Manually set the fields that need resetting.
    //
    m_InterruptInfo.TargetProcessorSet      = 0;
    m_InterruptInfo.Group                   = 0;
    m_InterruptInfo.Irql                    = PASSIVE_LEVEL;
    m_InterruptInfo.ShareDisposition        = 0;
    m_InterruptInfo.Mode                    = LevelSensitive;
    m_InterruptInfo.Vector                  = 0;

    m_SynchronizeIrql = PASSIVE_LEVEL;

    //
    // Do mode-specific reset.
    // For KMDF, it's a no-op.
    // For UMDF, a message is sent to reflector to reset the interrupt info.
    //
    ResetInternal();
}

VOID
FxInterrupt::AssignResources(
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescRaw,
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescTrans
    )
/*++

Routine Description:

    This function allows an interrupt object to know what resources have been
    assigned to it.  It will be called as part of IRP_MN_START_DEVICE.

Arguments:

    CmDescRaw - A CmResourceDescriptor that describes raw interrupt resources

    CmDescTrans - A CmResourceDescriptor that describes translated interrupt
        resources

Return Value:

    VOID

--*/
{    
    //
    // This code not from original WDF
    //

    if (CmDescTrans->u.MessageInterrupt.Raw.MessageCount &&
         FxLibraryGlobals.ProcessorGroupSupport == FALSE)
    {
        if (GetDriverGlobals()->FxVerifierDbgBreakOnError)
        {
            __debugbreak();
        }
    }

    m_InterruptInfo.Group = CmDescTrans->u.MessageInterrupt.Raw.MessageCount;
    m_InterruptInfo.TargetProcessorSet = CmDescTrans->u.Generic.Length;
    m_InterruptInfo.Irql = CmDescTrans->u.Generic.Start.QuadPart;
    m_InterruptInfo.ShareDisposition = CmDescTrans->ShareDisposition;
    m_InterruptInfo.Mode                    =
        CmDescTrans->Flags & CM_RESOURCE_INTERRUPT_LATCHED ? Latched : LevelSensitive;
    
    //
    //  Note if this is an MSI interrupt
    //
    m_InterruptInfo.MessageSignaled = _IsMessageInterrupt(CmDescTrans->Flags);

    if (m_InterruptInfo.MessageSignaled &&
        CmDescRaw->u.MessageInterrupt.Raw.MessageCount > 1)
    {
        //
        // This is an assignment for a multi-message PCI 2.2-style resource.
        // Thus the vector and message data have to be deduced.
        //
        m_InterruptInfo.Vector = CmDescTrans->u.Interrupt.Vector + m_InterruptInfo.MessageNumber;
    }
    else
    {
        //
        // This is an assignment for a single interrupt, either line-based or
        // PCI 2.2 single-message MSI, or PCI 3.0 MSI-X-style resource.
        //
        m_InterruptInfo.Vector = CmDescTrans->u.Interrupt.Vector;
    }

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "Is MSI? %d, MSI-ID %d, AffinityPolicy %!WDF_INTERRUPT_POLICY!, "
        "Priority %!WDF_INTERRUPT_PRIORITY!, Group %d, Affinity 0x%I64x, "
        "Irql 0x%x, Vector 0x%x\n",
        m_InterruptInfo.MessageSignaled,
        m_InterruptInfo.MessageNumber,
        m_Policy,
        m_Priority,
        m_InterruptInfo.Group,
        (ULONGLONG)m_InterruptInfo.TargetProcessorSet,
        m_InterruptInfo.Irql,
        m_InterruptInfo.Vector);
}
