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

#define MM_PAGED_POOL_SIZE	(100*1024*1024)
#define MM_NONPAGED_POOL_SIZE   (100*1024*1024)

/*
 * Maximum size of the kmalloc area (this is totally arbitary)
 */
#define MM_KERNEL_MAP_SIZE	(16*1024*1024)

/*
 * (number of bytes-1) per cache page
 *
 * FIXME - different architectures have different cache alignments...
 */

#define MM_CACHE_ALIGN_BYTES 31

#define MM_CACHE_ALIGN_UP(ptr) (PVOID)((((size_t)(ptr) + MM_CACHE_ALIGN_BYTES)) & (~MM_CACHE_ALIGN_BYTES))

#define MM_CACHE_ALIGN_DOWN(ptr) (PVOID)(((size_t)(ptr)) & (~MM_CACHE_ALIGN_BYTES))

#define MM_IS_CACHE_ALIGNED(ptr) ((PVOID)(ptr) == MM_CACHE_ALIGN(ptr))

#endif /* __INTERNAL_POOL_H */
