#ifndef __INCLUDE_DDK_LDRFUNCS_H
#define __INCLUDE_DDK_LDRFUNCS_H
/* $Id: ldrfuncs.h,v 1.2 2000/08/28 21:45:08 ekohl Exp $ */

typedef struct _LDR_RESOURCE_INFO
{
    ULONG Type;
    ULONG Name;
    ULONG Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

#define RESOURCE_TYPE_LEVEL      0
#define RESOURCE_NAME_LEVEL      1
#define RESOURCE_LANGUAGE_LEVEL  2
#define RESOURCE_DATA_LEVEL      3


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
