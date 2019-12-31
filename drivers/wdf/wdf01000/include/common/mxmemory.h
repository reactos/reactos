/*
    Memory allocation/deallocation functions
*/

#ifndef _MXMEMORY_H_
#define _MXMEMORY_H_

class MxMemory {
public:
    
    __inline
    static
    PVOID
    MxAllocatePoolWithTag(
        __in POOL_TYPE  PoolType,
        __in SIZE_T  NumberOfBytes,
        __in ULONG  Tag
        )
    {
        return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    }
    
    __inline
    static
    VOID
    MxFreePool(__in PVOID Ptr)
    {
        ExFreePool(Ptr);
    }
    
};

#endif //_MXMEMORY_H_