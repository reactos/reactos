#ifndef _UEFI_ARCNAME_H_
#define _UEFI_ARCNAME_H_

BOOLEAN UefiInitializeBootDevices(VOID);
BOOLEAN UefiInitializeArcDisks(PLOADER_PARAMETER_BLOCK LoaderBlock);
BOOLEAN UefiGetBootPartitionInfo(OUT PULONG RDiskNumber,
                                 OUT PULONG PartitionNumber,
                                 OUT PCHAR BootDevice,
                                 IN ULONG BootDeviceSize);

#endif /* _UEFI_ARCNAME_H_ */