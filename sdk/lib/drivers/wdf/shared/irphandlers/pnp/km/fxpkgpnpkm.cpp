//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "../pnppriv.hpp"

#include <initguid.h>
#include <wdmguid.h>

extern "C" {
#if defined(EVENT_TRACING)
#include "FxPkgPnpKM.tmh"
#endif
}

NTSTATUS
FxPkgPnp::FilterResourceRequirements(
    __in IO_RESOURCE_REQUIREMENTS_LIST **IoList
    )
/*++

Routine Description:

    This routine traverses one or more alternate _IO_RESOURCE_LISTs in the input
    IO_RESOURCE_REQUIREMENTS_LIST looking for interrupt descriptor and applies
    the policy set by driver in the interrupt object to the resource descriptor.

    LBI - Line based interrupt
    MSI - Message Signalled interrupt

    Here are the assumptions made about the order of descriptors.

    - An IoRequirementList can have one or more alternate IoResourceList
    - Each IoResourceList can have one or more resource descriptors
    - A descriptor can be default (unique), preferred, or alternate descriptors
    - A preferred descriptor can have zero or more alternate descriptors (P, A, A, A..)
    - In an IoResourceList, there can be one or more LBI descriptors
      (non-pci devices)(P,A,P,A)
    - In an IoResourceList, there can be only one preferred MSI 2.2
      (single or multi message) descriptor
    - In an IoResourceList, there cannot be MSI2.2 and MSI-X descriptors
    - In an IoResourceList, there can be one or more MSI-X descriptor
    - An alternate descriptor cannot be a very first descriptor in the list


    Now with that assumption, this routines parses the list looking for interrupt
    descriptor.

    - If it finds a LBI, it starts with the very first interrupt object and applies
      the policy set by the driver to the resource descriptor.
    - If it's finds an MSI2.2 then it starts with the first interrupt object and applies
      the policy. If the MSI2.2 is a multi-message one then it loops thru looking for
      as many interrupt object as there are messages. It doesn't fail the IRP, if the
      interrupt objects are less than the messages.
    - If there is an alternate descriptor then it applies the same policy from the
      interrupt object that it used for the preceding preferred descriptor.
    - Framework always uses FULLY_SPECIFIED connection type for both LBI and MSI
      interrupts including MSI-X
    - Framework will apply the policy on the descriptor set by the driver only
      if the policy is already not included in the resource descriptor. This is
      to allow the policy set in the registry to take precedence over the hard
      coded driver policy.
    - If the driver registers filter resource requirement and applies the policy
      on its own (by escaping to WDM) then framework doesn't override that.

Arguments:

    IoList - Pointer to the list part of an IRP_MN_FILTER_RESOURCE_REQUIREMENTS.

Return Value:

    NTSTATUS

--*/
{
    ULONG altResListIndex;
    PIO_RESOURCE_REQUIREMENTS_LIST pIoRequirementList;
    PIO_RESOURCE_LIST pIoResList;

    pIoRequirementList = *IoList;

    if (pIoRequirementList == NULL) {
        return STATUS_SUCCESS;
    }

    if (IsListEmpty(&m_InterruptListHead)) {
        //
        // No interrupt objects created to filter resource requirements.
        //
        return STATUS_SUCCESS;
    }

    pIoResList = pIoRequirementList->List;

    //
    // Parse one or more alternative resource lists.
    //
    for (altResListIndex = 0;
         altResListIndex < pIoRequirementList->AlternativeLists;
         altResListIndex++) {
        PLIST_ENTRY pIntListEntryForMSI;
        PLIST_ENTRY pIntListEntryForLBI;
        BOOLEAN multiMessageMSI22Found;
        BOOLEAN previousDescMSI;
        ULONG descIndex;

        multiMessageMSI22Found = FALSE;
        previousDescMSI = FALSE;

        pIntListEntryForMSI = &m_InterruptListHead;
        pIntListEntryForLBI = &m_InterruptListHead;

        //
        // Traverse each _IO_RESOURCE_LISTs looking for interrupt descriptors
        // and call FilterResourceRequirements method so that it can apply
        // policy set on the interrupt object into the resource-descriptor.
        //

        for (descIndex = 0; descIndex < pIoResList->Count; descIndex++) {
            ULONG messageCount;
            PIO_RESOURCE_DESCRIPTOR pIoDesc;
            FxInterrupt* pInterruptInstance;

            pIoDesc = &pIoResList->Descriptors[descIndex];

            switch (pIoDesc->Type) {
            case CmResourceTypeInterrupt:

                if (FxInterrupt::_IsMessageInterrupt(pIoDesc->Flags)) {

                    previousDescMSI = TRUE;

                    //
                    // We will advance to the next interrupt object if the resource
                    // is not an alternate resource descriptor. A resource list can
                    // have a preferred and zero or more alternate resource descriptors
                    // for the same resource. We need to apply the same policy on the
                    // alternate desc that we applied on the preferred one in case one
                    // of the alernate desc is selected for this device. An alternate
                    // resource descriptor can't be the first descriptor in a list.
                    //
                    if ((pIoDesc->Option & IO_RESOURCE_ALTERNATIVE) == 0) {
                        pIntListEntryForMSI = pIntListEntryForMSI->Flink;
                    }

                    if (pIntListEntryForMSI == &m_InterruptListHead) {
                        DoTraceLevelMessage(
                            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                            "Not enough interrupt objects created for MSI by WDFDEVICE 0x%p ",
                            m_Device->GetHandle());
                        break;
                    }

                    pInterruptInstance = CONTAINING_RECORD(pIntListEntryForMSI, FxInterrupt, m_PnpList);
                    messageCount = pIoDesc->u.Interrupt.MaximumVector - pIoDesc->u.Interrupt.MinimumVector + 1;

                    if (messageCount > 1) {
                        //
                        //  PCI spec guarantees that there can be only one preferred/default
                        //  MSI 2.2 descriptor in a single list.
                        //
                        if ((pIoDesc->Option & IO_RESOURCE_ALTERNATIVE) == 0) {
#if DBG
                            ASSERT(multiMessageMSI22Found == FALSE);
#else
                            UNREFERENCED_PARAMETER(multiMessageMSI22Found);
#endif
                            multiMessageMSI22Found = TRUE;

                        }
                    }
                    else {
                        //
                        //  This is either single message MSI 2.2 or MSI-X interrupts
                        //
                        DO_NOTHING();
                    }

                    pInterruptInstance->FilterResourceRequirements(pIoDesc);
                }
                else {

                    //
                    // We will advance to next interrupt object if the desc is not an alternate
                    // descriptor. For non PCI devices, the first LBI interrupt desc can't be an
                    // alternate descriptor.
                    //
                    if ((pIoDesc->Option & IO_RESOURCE_ALTERNATIVE) == 0) {
                        pIntListEntryForLBI = pIntListEntryForLBI->Flink;
                    }

                    //
                    // An LBI can be first alternate resource if there are preceding MSI(X) descriptors
                    // listed in the list. In that case, this descriptor is the alternate interrupt resource
                    // for all of the MSI messages. As a result, we will use the first interrupt object from
                    // the list if this ends up being assigned by the system instead of MSI.
                    //
                    if (previousDescMSI) {
                        ASSERT(pIoDesc->Option & IO_RESOURCE_ALTERNATIVE);
                        pIntListEntryForLBI = m_InterruptListHead.Flink;
                        previousDescMSI = FALSE;
                    }

                    //
                    // There can be one or more LBI interrupts and each LBI interrupt
                    // could have zero or more alternate descriptors.
                    //
                    if (pIntListEntryForLBI == &m_InterruptListHead) {
                        DoTraceLevelMessage(
                            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                            "Not enough interrupt objects created for LBI by WDFDEVICE 0x%p ",
                            m_Device->GetHandle());
                        break;
                    }

                    pInterruptInstance = CONTAINING_RECORD(pIntListEntryForLBI, FxInterrupt, m_PnpList);

                    pInterruptInstance->FilterResourceRequirements(pIoDesc);
                }

                break;

            default:
                break;
            }
        }

        //
        // Since the Descriptors is a variable length list, you cannot get to the next
        // alternate list by doing pIoRequirementList->List[altResListIndex].
        // Descriptors[descIndex] will now point to the end of the descriptor list.
        // If there is another alternate list, it would be begin there.
        //
        pIoResList = (PIO_RESOURCE_LIST) &pIoResList->Descriptors[descIndex];
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::AllocateDmaEnablerList(
    VOID
    )
{
    FxSpinLockTransactionedList* pList;
    NTSTATUS status;
    KIRQL irql;

    if (m_DmaEnablerList != NULL) {
        return STATUS_SUCCESS;
    }

    Lock(&irql);
    if (m_DmaEnablerList == NULL) {
        pList = new (GetDriverGlobals()) FxSpinLockTransactionedList();

        if (pList != NULL) {
            m_DmaEnablerList = pList;
            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else {
        //
        // Already have a DMA list
        //
        status = STATUS_SUCCESS;
    }
    Unlock(irql);

    return status;
}

VOID
FxPkgPnp::AddDmaEnabler(
    __in FxDmaEnabler* Enabler
    )
{
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Adding DmaEnabler %p, WDFDMAENABLER %p",
                        Enabler, Enabler->GetObjectHandle());

    m_DmaEnablerList->Add(GetDriverGlobals(), &Enabler->m_TransactionLink);
}

VOID
FxPkgPnp::RemoveDmaEnabler(
    __in FxDmaEnabler* Enabler
    )
{
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Removing DmaEnabler %p, WDFDMAENABLER %p",
                        Enabler, Enabler->GetObjectHandle());

    m_DmaEnablerList->Remove(GetDriverGlobals(), &Enabler->m_TransactionLink);
}

VOID
FxPkgPnp::WriteStateToRegistry(
    __in HANDLE RegKey,
    __in PUNICODE_STRING ValueName,
    __in ULONG Value
    )
{
    ZwSetValueKey(RegKey, ValueName, 0, REG_DWORD, &Value, sizeof(Value));
}

// NTSTATUS __REACTOS__
// FxPkgPnp::UpdateWmiInstanceForS0Idle(
//     __in FxWmiInstanceAction Action
//     )
// {
//     FxWmiProvider* pProvider;
//     NTSTATUS status;

//     switch(Action) {
//     case AddInstance:
//         if (m_PowerPolicyMachine.m_Owner->m_IdleSettings.WmiInstance == NULL) {
//             FxWmiInstanceInternalCallbacks cb;

//             cb.SetInstance = _S0IdleSetInstance;
//             cb.QueryInstance = _S0IdleQueryInstance;
//             cb.SetItem = _S0IdleSetItem;

//             status = RegisterPowerPolicyWmiInstance(
//                 &GUID_POWER_DEVICE_ENABLE,
//                 &cb,
//                 &m_PowerPolicyMachine.m_Owner->m_IdleSettings.WmiInstance);

//             if (!NT_SUCCESS(status)) {
//                 return status;
//             }
//         }
//         else {
//             pProvider = m_PowerPolicyMachine.m_Owner->m_IdleSettings.
//                 WmiInstance->GetProvider();

//             //
//             // Enable the WMI GUID by adding the instance back to the provider's
//             // list.  If there is an error, ignore it.  It just means we were
//             // racing with another thread removing or adding the instance.
//             //
//             (void) pProvider->AddInstance(
//                 m_PowerPolicyMachine.m_Owner->m_IdleSettings.WmiInstance,
//                 TRUE
//                 );
//         }
//         break;

//     case RemoveInstance:
//         if (m_PowerPolicyMachine.m_Owner->m_IdleSettings.WmiInstance != NULL) {
//             //
//             // Disable the WMI guid by removing it from the provider's list of
//             // instances.
//             //
//             pProvider = m_PowerPolicyMachine.m_Owner->m_IdleSettings.
//                 WmiInstance->GetProvider();

//             pProvider->RemoveInstance(
//                 m_PowerPolicyMachine.m_Owner->m_IdleSettings.WmiInstance
//                 );
//         }
//         break;

//     default:
//         ASSERT(FALSE);
//         break;
//     }

//     return STATUS_SUCCESS;;
// }

VOID
FxPkgPnp::ReadRegistryS0Idle(
    __in  PCUNICODE_STRING ValueName,
    __out BOOLEAN *Enabled
    )
{
    NTSTATUS status;
    FxAutoRegKey hKey;

    status = m_Device->OpenSettingsKey(&hKey.m_Key, STANDARD_RIGHTS_READ);

    //
    // Modify the value of Enabled only if success
    //
    if (NT_SUCCESS(status)) {
        ULONG value;

        status = FxRegKey::_QueryULong(
            hKey.m_Key, ValueName, &value);

        if (NT_SUCCESS(status)) {
            //
            // Normalize the ULONG value into a BOOLEAN
            //
            *Enabled = (value == FALSE) ? FALSE : TRUE;
        }
    }
}

// NTSTATUS __REACTOS__
// FxPkgPnp::UpdateWmiInstanceForSxWake(
//     __in FxWmiInstanceAction Action
//     )
// {
//     FxWmiProvider* pProvider;
//     NTSTATUS status;

//     switch(Action) {
//     case AddInstance:
//         if (m_PowerPolicyMachine.m_Owner->m_WakeSettings.WmiInstance == NULL) {
//             FxWmiInstanceInternalCallbacks cb;

//             cb.SetInstance = _SxWakeSetInstance;
//             cb.QueryInstance = _SxWakeQueryInstance;
//             cb.SetItem = _SxWakeSetItem;

//             status = RegisterPowerPolicyWmiInstance(
//                 &GUID_POWER_DEVICE_WAKE_ENABLE,
//                 &cb,
//                 &m_PowerPolicyMachine.m_Owner->m_WakeSettings.WmiInstance);

//             if (!NT_SUCCESS(status)) {
//                 return status;
//             }
//         } else {
//             pProvider = m_PowerPolicyMachine.m_Owner->m_WakeSettings.
//                 WmiInstance->GetProvider();

//             //
//             // Enable the WMI GUID by adding the instance back to the provider's
//             // list.  If there is an error, ignore it.  It just means we were
//             // racing with another thread removing or adding the instance.
//             //
//             (void) pProvider->AddInstance(
//                 m_PowerPolicyMachine.m_Owner->m_WakeSettings.WmiInstance,
//                 TRUE
//                 );
//         }
//         break;

//     case RemoveInstance:
//         if (m_PowerPolicyMachine.m_Owner->m_WakeSettings.WmiInstance != NULL) {
//             //
//             // Disable the WMI guid by removing it from the provider's list of
//             // instances.
//             //
//             pProvider = m_PowerPolicyMachine.m_Owner->m_WakeSettings.
//                 WmiInstance->GetProvider();

//             pProvider->RemoveInstance(
//                 m_PowerPolicyMachine.m_Owner->m_WakeSettings.WmiInstance
//                 );
//         }
//         break;

//     default:
//         ASSERT(FALSE);
//         break;
//     }

//     return STATUS_SUCCESS;
// }

VOID
FxPkgPnp::ReadRegistrySxWake(
    __in  PCUNICODE_STRING ValueName,
    __out BOOLEAN *Enabled
    )
{
    FxAutoRegKey hKey;
    NTSTATUS status;

    status = m_Device->OpenSettingsKey(&hKey.m_Key, STANDARD_RIGHTS_READ);

    //
    // Modify the value of Enabled only if success
    //
    if (NT_SUCCESS(status)) {
        ULONG value;

        status = FxRegKey::_QueryULong(
            hKey.m_Key, ValueName, &value);

        if (NT_SUCCESS(status)) {
            //
            // Normalize the ULONG value into a BOOLEAN
            //
            *Enabled = (value == FALSE) ? FALSE : TRUE;
        }
    }
}

VOID
PnpPassThroughQIWorker(
    __in    MxDeviceObject* Device,
    __inout FxIrp* Irp,
    __inout FxIrp* ForwardIrp
    )
{
    PIO_STACK_LOCATION pFwdStack, pCurStack;

    pCurStack = Irp->GetCurrentIrpStackLocation();

    ForwardIrp->SetStatus(STATUS_NOT_SUPPORTED);

    pFwdStack = ForwardIrp->GetNextIrpStackLocation();
    pFwdStack->MajorFunction = Irp->GetMajorFunction();
    pFwdStack->MinorFunction = Irp->GetMinorFunction();

    RtlCopyMemory(&pFwdStack->Parameters.QueryInterface,
                  &pCurStack->Parameters.QueryInterface,
                  sizeof(pFwdStack->Parameters.QueryInterface));

    ForwardIrp->SetInformation(Irp->GetInformation());
    ForwardIrp->SendIrpSynchronously(Device->GetObject());

    pFwdStack = ForwardIrp->GetNextIrpStackLocation();

    RtlCopyMemory(&pCurStack->Parameters.QueryInterface,
                  &pFwdStack->Parameters.QueryInterface,
                  sizeof(pCurStack->Parameters.QueryInterface));
}

VOID
FxPkgPnp::RevokeDmaEnablerResources(
    __in FxDmaEnabler *DmaEnabler
    )
{
    // DmaEnabler->RevokeResources();
    ROSWDFNOTIMPLEMENTED;
}

VOID
FxPkgPnp::QueryForD3ColdInterface(
    VOID
    )
{
    MxDeviceObject deviceObject;
    PDEVICE_OBJECT topOfStack;
    PDEVICE_OBJECT pdo;
    FxAutoIrp irp;
    NTSTATUS status;

    //
    // This function can be invoked multiple times, particularly if filters
    // send IRP_MN_QUERY_CAPABILITIES.  So bail out if the interface has already
    // been acquired.
    //

    if ((m_D3ColdInterface.InterfaceDereference != NULL) ||
        (m_D3ColdInterface.GetIdleWakeInfo != NULL) ||
        (m_D3ColdInterface.SetD3ColdSupport != NULL)) {
        return;
    }

    pdo = m_Device->GetPhysicalDevice();

    if (pdo == NULL) {
        return;
    }

    //
    // Get the top of stack device object, even though normal filters and the
    // FDO may not have been added to the stack yet to ensure that this
    // query-interface is seen by bus filters.  Specifically, in a PCI device
    // which is soldered to the motherboard, ACPI will be on the stack and it
    // needs to see this IRP.
    //
    topOfStack = IoGetAttachedDeviceReference(pdo);
    deviceObject.SetObject(topOfStack);
    if (deviceObject.GetObject() != NULL) {
        irp.SetIrp(FxIrp::AllocateIrp(deviceObject.GetStackSize()));
        if (irp.GetIrp() == NULL) {

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Failed to allocate IRP to get D3COLD_SUPPORT_INTERFACE from !devobj %p",
                pdo);
        } else {

            //
            // Initialize the Irp
            //
            irp.SetStatus(STATUS_NOT_SUPPORTED);

            irp.ClearNextStack();
            irp.SetMajorFunction(IRP_MJ_PNP);
            irp.SetMinorFunction(IRP_MN_QUERY_INTERFACE);
            irp.SetParameterQueryInterfaceType(&GUID_D3COLD_SUPPORT_INTERFACE);
            irp.SetParameterQueryInterfaceVersion(D3COLD_SUPPORT_INTERFACE_VERSION);
            irp.SetParameterQueryInterfaceSize(sizeof(m_D3ColdInterface));
            irp.SetParameterQueryInterfaceInterfaceSpecificData(NULL);
            irp.SetParameterQueryInterfaceInterface((PINTERFACE)&m_D3ColdInterface);

            status = irp.SendIrpSynchronously(deviceObject.GetObject());

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "!devobj %p declined to supply D3COLD_SUPPORT_INTERFACE",
                    pdo);

                RtlZeroMemory(&m_D3ColdInterface, sizeof(m_D3ColdInterface));
            }
        }
    }
    ObDereferenceObject(topOfStack);
}

VOID
FxPkgPnp::DropD3ColdInterface(
    VOID
    )
{
    if (m_D3ColdInterface.InterfaceDereference != NULL) {
        m_D3ColdInterface.InterfaceDereference(m_D3ColdInterface.Context);
    }

    RtlZeroMemory(&m_D3ColdInterface, sizeof(m_D3ColdInterface));
}

