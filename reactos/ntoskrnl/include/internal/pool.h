#ifndef __INTERNAL_POOL_H
#define __INTERNAL_POOL_H

PVOID STDCALL ExAllocateNonPagedPoolWithTag (POOL_TYPE	type, 
					     ULONG		size, 
					     ULONG		Tag,
					     PVOID		Caller);

PVOID STDCALL ExAllocatePagedPoolWithTag (POOL_TYPE	Type,
					  ULONG		size,
					  ULONG		Tag);
VOID STDCALL ExFreeNonPagedPool (PVOID block);

VOID STDCALL
ExFreePagedPool(IN PVOID Block);
VOID MmInitializePagedPool(VOID);

extern PVOID MmPagedPoolBase;
extern ULONG MmPagedPoolSize;

#define MM_PAGED_POOL_SIZE (4*1024*1024)

/*
 * Maximum size of the kmalloc area (this is totally arbitary)
 */
#define NONPAGED_POOL_SIZE   (400*1024*1024)

#endif /* __INTERNAL_POOL_H */
