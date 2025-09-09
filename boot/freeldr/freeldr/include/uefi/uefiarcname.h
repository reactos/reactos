#ifndef _UEFI_ARCNAME_H_
#define _UEFI_ARCNAME_H_

// AGENT-MODIFIED: Updated function declarations to match renamed implementations
BOOLEAN UefiEnumerateArcDisks(VOID);
BOOLEAN UefiInitializeArcDisks(PLOADER_PARAMETER_BLOCK LoaderBlock);
BOOLEAN UefiGetBootPartitionInfo(OUT PULONG RDiskNumber,
                                 OUT PULONG PartitionNumber,
                                 OUT PCHAR BootDevice,
                                 IN ULONG BootDeviceSize);

#endif /* _UEFI_ARCNAME_H_ */