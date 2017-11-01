#ifndef _RX_NTDEFS_DEFINED_
#define _RX_NTDEFS_DEFINED_

#define INLINE __inline
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

#ifndef MINIRDR__NAME
#define RxCaptureFcb PFCB __C_Fcb = (PFCB)(RxContext->pFcb)
#define RxCaptureFobx PFOBX __C_Fobx = (PFOBX)(RxContext->pFobx)
#else
#define RxCaptureFcb PMRX_FCB __C_Fcb = (RxContext->pFcb)
#define RxCaptureFobx PMRX_FOBX __C_Fobx = (RxContext->pFobx)
#endif

#define RxCaptureParamBlock PIO_STACK_LOCATION __C_IrpSp = RxContext->CurrentIrpSp
#define RxCaptureFileObject PFILE_OBJECT __C_FileObject = __C_IrpSp-> FileObject

#define capFcb __C_Fcb
#define capFobx __C_Fobx
#define capPARAMS __C_IrpSp
#define capFileObject __C_FileObject

#define RxAllocatePoolWithTag ExAllocatePoolWithTag
#define RxFreePool ExFreePool

#define RxIsResourceOwnershipStateExclusive(Resource) (FlagOn((Resource)->Flag, ResourceOwnedExclusive))

#define RxMdlIsLocked(Mdl) ((Mdl)->MdlFlags & MDL_PAGES_LOCKED)
#define RxMdlSourceIsNonPaged(Mdl) ((Mdl)->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL)

#define RxGetRequestorProcess(RxContext) IoGetRequestorProcess(RxContext->CurrentIrp)

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
