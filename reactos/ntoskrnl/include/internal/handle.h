#ifndef __INTERNAL_HANDLE_H
#define __INTERNAL_HANDLE_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifndef AS_INVOKED

typedef struct _RTL_HANDLE
{
   struct _RTL_HANDLE *Next;	/* pointer to next free handle */
   PVOID Object;		/* pointer to object */
} RTL_HANDLE, *PRTL_HANDLE;

typedef struct _RTL_HANDLE_TABLE
{
   ULONG TableSize;		/* maximum number of handles */
   PRTL_HANDLE Handles;		/* pointer to handle array */
   PRTL_HANDLE Limit;		/* limit of pointers */
   PRTL_HANDLE FirstFree;	/* pointer to first free handle */
   PRTL_HANDLE LastUsed;	/* pointer to last allocated handle */
} RTL_HANDLE_TABLE, *PRTL_HANDLE_TABLE;

VOID RtlpInitializeHandleTable(ULONG TableSize, PRTL_HANDLE_TABLE HandleTable);
VOID RtlpDestroyHandleTable(PRTL_HANDLE_TABLE HandleTable);
BOOLEAN RtlpAllocateHandle(PRTL_HANDLE_TABLE HandleTable, PVOID Object, PULONG Index);
BOOLEAN RtlpFreeHandle(PRTL_HANDLE_TABLE HandleTable, ULONG Index);
PVOID RtlpMapHandleToPointer(PRTL_HANDLE_TABLE HandleTable, ULONG Index);

#endif /* !AS_INVOKED */

#endif /* __INTERNAL_HANDLE_H */
