/**
 * File: NvmeProbe.h
 */

#ifndef __NVME_PROBE_H__
#define __NVME_PROBE_H__

#define PCI_MAX_BUS       16

BOOLEAN
ScanBusAndFindNvme(BOOLEAN *found, INT *bus, INT *slot);

PVOID
MapNvmeCtrlRegisters(PVOID pAE, INT Bus, INT SlotNum, PACCESS_RANGE pAccessRanges);

BOOLEAN
NVMeSetupPCI(ULONG busNumber, ULONG slotNumber, PPCI_COMMON_CONFIG pciData);

BOOLEAN
FindAndSetupNvmeController(PNVME_DEVICE_EXTENSION pDevExtension, PPCI_NVM_DEVICE_INFO pciNvmeDeviceInfo);

BOOLEAN
NVMeSendIdentifyController(PNVME_DEVICE_EXTENSION pDevExtension);

BOOLEAN
NVMeInitNS(PNVME_DEVICE_EXTENSION pDevExt);

BOOLEAN
NVMeInitIOQueues(PNVME_DEVICE_EXTENSION pDevExt);

VOID
NVMeFreeMemory(PNVME_DEVICE_EXTENSION pDevExt);
#endif
