#ifndef __INTERNAL_POOL_H
#define __INTERNAL_POOL_H

#include <windows.h>

#include <internal/linkage.h>

PVOID ExAllocatePagedPoolWithTag(POOL_TYPE Type, ULONG size, ULONG Tag);
PVOID ExAllocateNonPagedPoolWithTag(POOL_TYPE Type, ULONG size, ULONG Tag);

#endif /* __INTERNAL_POOL_H */
