
#ifndef __INCLUDE_INTERNAL_REGISTRY_H
#define __INCLUDE_INTERNAL_REGISTRY_H

#ifndef AS_INVOKED

NTSTATUS
RtlpGetRegistryHandle(ULONG RelativeTo,
		      PCWSTR Path,
		      BOOLEAN Create,
		      PHANDLE KeyHandle);

NTSTATUS
RtlpCreateRegistryKeyPath(PWSTR Path);

#endif /* !AS_INVOKED */

#endif /* __INCLUDE_INTERNAL_REGISTRY_H */
