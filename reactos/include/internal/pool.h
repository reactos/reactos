#ifndef __INTERNAL_POOL_H
#define __INTERNAL_POOL_H

#include <windows.h>

#include <internal/linkage.h>

/*
 * Maximum size of the kmalloc area (this is totally arbitary)
 */
#define NONPAGED_POOL_SIZE   (4*1024*1024)

/*
 * Allocates an arbitary sized block at any alignment
 */
//asmlinkage void* ExAllocatePool(ULONG size);
//asmlinkage void ExFreePool(void* block);

#endif /* __INTERNAL_POOL_H */
