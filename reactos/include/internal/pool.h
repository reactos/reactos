#ifndef __INTERNAL_POOL_H
#define __INTERNAL_POOL_H

#include <windows.h>

#include <internal/linkage.h>

static PVOID ExAllocatePagedPool(POOL_TYPE Type, ULONG size);
static PVOID ExAllocateNonPagedPool(POOL_TYPE Type, ULONG size);

#endif /* __INTERNAL_POOL_H */
