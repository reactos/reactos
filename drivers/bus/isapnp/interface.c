/*
 * PROJECT:     ReactOS ISA PnP Bus driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver interface
 * COPYRIGHT:   Copyright 2021 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "isapnp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
NTSTATUS
IsaFdoQueryInterface(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _In_ PIO_STACK_LOCATION IrpSp)
{
    PAGED_CODE();

    if (IsEqualGUIDAligned(IrpSp->Parameters.QueryInterface.InterfaceType,
                           &GUID_TRANSLATOR_INTERFACE_STANDARD))
    {
        NTSTATUS Status;
        CM_RESOURCE_TYPE ResourceType;
        ULONG ParentBusType, ParentBusNumber, Dummy;

        ResourceType = PtrToUlong(IrpSp->Parameters.QueryInterface.InterfaceSpecificData);

        if (IrpSp->Parameters.QueryInterface.Size < sizeof(TRANSLATOR_INTERFACE) ||
            ResourceType != CmResourceTypeInterrupt)
        {
            return STATUS_NOT_SUPPORTED;
        }

        Status = IoGetDeviceProperty(FdoExt->Pdo,
                                     DevicePropertyLegacyBusType,
                                     sizeof(ParentBusType),
                                     &ParentBusType,
                                     &Dummy);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("BusType request failed with status 0x%08lx\n", Status);
            return Status;
        }

        Status = IoGetDeviceProperty(FdoExt->Pdo,
                                     DevicePropertyBusNumber,
                                     sizeof(ParentBusNumber),
                                     &ParentBusNumber,
                                     &Dummy);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("BusNumber request failed with status 0x%08lx\n", Status);
            return Status;
        }

        return HalGetInterruptTranslator(ParentBusType,
                                         ParentBusNumber,
                                         Isa,
                                         IrpSp->Parameters.QueryInterface.Size,
                                         IrpSp->Parameters.QueryInterface.Version,
                                         (PTRANSLATOR_INTERFACE)IrpSp->
                                         Parameters.QueryInterface.Interface,
                                         &ParentBusNumber);
    }

    return STATUS_NOT_SUPPORTED;
}
