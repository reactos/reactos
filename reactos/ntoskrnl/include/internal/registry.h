
#ifndef __INCLUDE_INTERNAL_REGISTRY_H
#define __INCLUDE_INTERNAL_REGISTRY_H

NTSTATUS
RtlpGetRegistryHandle(ULONG RelativeTo,
		      PWSTR Path,
		      BOOLEAN Create,
		      PHANDLE KeyHandle);

#endif /* __INCLUDE_INTERNAL_REGISTRY_H */
