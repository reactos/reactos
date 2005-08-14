#ifndef __NTOSKRNL_INCLUDE_INTERNAL_PO_H
#define __NTOSKRNL_INCLUDE_INTERNAL_PO_H

extern PDEVICE_NODE PopSystemPowerDeviceNode;

VOID
PoInit(
    PLOADER_PARAMETER_BLOCK LoaderBlock, 
    BOOLEAN ForceAcpiDisable
);

NTSTATUS
PopSetSystemPowerState(SYSTEM_POWER_STATE PowerState);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_PO_H */
