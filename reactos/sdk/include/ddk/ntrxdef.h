#ifndef _RX_NTDEFS_DEFINED_
#define _RX_NTDEFS_DEFINED_

#define INLINE __inline
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

#define RxAllocatePoolWithTag ExAllocatePoolWithTag
#define RxFreePool ExFreePool

#define RxMdlIsLocked(Mdl) ((Mdl)->MdlFlags & MDL_PAGES_LOCKED)
#define RxMdlSourceIsNonPaged(Mdl) ((Mdl)->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL)

#define RxAdjustAllocationSizeforCC(Fcb)                                                       \
{                                                                                              \
    if ((Fcb)->Header.FileSize.QuadPart > (Fcb)->Header.AllocationSize.QuadPart)               \
    {                                                                                          \
        PMRX_NET_ROOT NetRoot = (Fcb)->pNetRoot;                                               \
        ULONGLONG ClusterSize = NetRoot->DiskParameters.ClusterSize;                           \
        ULONGLONG FileSize = (Fcb)->Header.FileSize.QuadPart;                                  \
        ASSERT(ClusterSize != 0);                                                              \
        (Fcb)->Header.AllocationSize.QuadPart = (FileSize + ClusterSize) &~ (ClusterSize - 1); \
    }                                                                                          \
    ASSERT ((Fcb)->Header.ValidDataLength.QuadPart <= (Fcb)->Header.FileSize.QuadPart);        \
}

#endif
