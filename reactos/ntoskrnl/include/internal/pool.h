#ifndef __INTERNAL_POOL_H
#define __INTERNAL_POOL_H

#ifndef AS_INVOKED

PVOID STDCALL 
ExAllocateNonPagedPoolWithTag(ULONG Type,
  ULONG Size,
  ULONG Tag,
  PVOID Caller);

PVOID STDCALL ExAllocatePagedPoolWithTag (POOL_TYPE	Type,
					  ULONG		size,
					  ULONG		Tag);
VOID STDCALL ExFreeNonPagedPool (PVOID block);

VOID STDCALL
ExFreePagedPool(IN PVOID Block);
VOID MmInitializePagedPool(VOID);

extern PVOID MmPagedPoolBase;
extern ULONG MmPagedPoolSize;

#define MM_PAGED_POOL_SIZE (100*1024*1024)

/*
 * Maximum size of the kmalloc area (this is totally arbitary)
 */
#define NONPAGED_POOL_SIZE   (100*1024*1024)

#endif /* !AS_INVOKED */

#endif /* __INTERNAL_POOL_H */
