#ifndef __INTERNAL_POOL_H
#define __INTERNAL_POOL_H

#include <internal/linkage.h>

PVOID ExAllocateNonPagedPoolWithTag(ULONG type, 
				    ULONG size, 
				    ULONG Tag,
				    PVOID Caller);
PVOID ExAllocatePagedPoolWithTag(POOL_TYPE Type, ULONG size, ULONG Tag);

#endif /* __INTERNAL_POOL_H */
