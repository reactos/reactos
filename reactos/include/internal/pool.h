#ifndef __INTERNAL_POOL_H
#define __INTERNAL_POOL_H

#include <internal/linkage.h>

PVOID
__stdcall
ExAllocateNonPagedPoolWithTag (
	POOL_TYPE	type, 
	ULONG		size, 
	ULONG		Tag,
	PVOID		Caller
	);
PVOID
__stdcall
ExAllocatePagedPoolWithTag (
	POOL_TYPE	Type,
	ULONG		size,
	ULONG		Tag
	);

#endif /* __INTERNAL_POOL_H */
