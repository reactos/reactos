
#ifndef __INCLUDE_INTERNAL_REGISTRY_H
#define __INCLUDE_INTERNAL_REGISTRY_H

NTSTATUS
RtlpGetRegistryHandle(ULONG RelativeTo,
		      PWSTR Path,
		      BOOLEAN Create,
		      PHANDLE KeyHandle);

NTSTATUS
RtlpCreateRegistryKeyPath(PWSTR Path);

#endif /* __INCLUDE_INTERNAL_REGISTRY_H */
