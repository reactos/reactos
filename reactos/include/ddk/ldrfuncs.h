#ifndef __INCLUDE_DDK_LDRFUNCS_H
#define __INCLUDE_DDK_LDRFUNCS_H
/* $Id: ldrfuncs.h,v 1.3 2001/06/22 12:39:47 ekohl Exp $ */

NTSTATUS STDCALL
LdrAccessResource(IN  PVOID BaseAddress,
                  IN  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
                  OUT PVOID *Resource OPTIONAL,
                  OUT PULONG Size OPTIONAL);

NTSTATUS STDCALL
LdrFindResource_U(IN  PVOID BaseAddress,
                  IN  PLDR_RESOURCE_INFO ResourceInfo,
                  IN  ULONG Level,
                  OUT PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry);

#endif /* __INCLUDE_DDK_LDRFUNCS_H */
