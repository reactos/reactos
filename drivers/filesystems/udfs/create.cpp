////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Create.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "Create"/"Open" dispatch entry point.
*
*************************************************************************/

#include            "udffs.h"

#define IsFileObjectReadOnly(FO) (!((FO)->WriteAccess | (FO)->DeleteAccess))

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_CREATE

#define         MEM_USABS_TAG                   "US_Abs"
#define         MEM_USLOC_TAG                   "US_Loc"
#define         MEM_USOBJ_TAG                   "US_Obj"

#define UDF_LOG_CREATE_DISPOSITION

/*************************************************************************
*
* Function: UDFCreate()
*
* Description:
*   The I/O Manager will invoke this routine to handle a create/open
*   request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*   to be deferred to a worker thread context)
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
NTAPI
UDFCreate(
    PDEVICE_OBJECT          DeviceObject,       // the logical volume device object
    PIRP                    Irp)                // I/O Request Packet
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PtrUDFIrpContext    PtrIrpContext;
    BOOLEAN             AreWeTopLevel = FALSE;

    TmPrint(("UDFCreate:\n"));

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    // sometimes, we may be called here with the device object representing
    //  the file system (instead of the device object created for a logical
    //  volume. In this case, there is not much we wish to do (this create
    //  typically will happen 'cause some process has to open the FSD device
    //  object so as to be able to send an IOCTL to the FSD)

    //  All of the logical volume device objects we create have a device
    //  extension whereas the device object representing the FSD has no
    //  device extension. This seems like a good enough method to identify
    //  between the two device objects ...
    if (UDFIsFSDevObj(DeviceObject)) {
        // this is an open of the FSD itself
        Irp->IoStatus.Status = RC;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        FsRtlExitFileSystem();
        return(RC);
    }

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);

    _SEH2_TRY {

        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        if(PtrIrpContext) {
            RC = UDFCommonCreate(PtrIrpContext, Irp);
        } else {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);

        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    } _SEH2_END;

    if (AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    AdPrint(("UDFCreate: %x\n", RC));

    FsRtlExitFileSystem();

    return(RC);

} // end UDFCreate()

/*
 */
VOID
__fastcall
UDFReleaseResFromCreate(
    IN PERESOURCE* PagingIoRes,
    IN PERESOURCE* Res1,
    IN PERESOURCE* Res2
    )
{
    if(*PagingIoRes) {
        UDFReleaseResource(*PagingIoRes);
        (*PagingIoRes) = NULL;
    }
    if(*Res1) {
        UDFReleaseResource(*Res1);
        (*Res1) = NULL;
    }
    if(*Res2) {
        UDFReleaseResource(*Res2);
        (*Res2) = NULL;
    }
} // end UDFReleaseResFromCreate()

/*
 */
VOID
__fastcall
UDFAcquireParent(
    IN PUDF_FILE_INFO RelatedFileInfo,
    IN PERESOURCE* Res1,
    IN PERESOURCE* Res2
    )
{
    if(RelatedFileInfo->Fcb &&
       RelatedFileInfo->Fcb->ParentFcb) {

        UDF_CHECK_PAGING_IO_RESOURCE(RelatedFileInfo->Fcb->ParentFcb->NTRequiredFCB);
        UDFAcquireResourceExclusive((*Res2) = &(RelatedFileInfo->Fcb->ParentFcb->NTRequiredFCB->MainResource),TRUE);
    }

    UDF_CHECK_PAGING_IO_RESOURCE(RelatedFileInfo->Fcb->NTRequiredFCB);
    UDFAcquireResourceExclusive((*Res1) = &(RelatedFileInfo->Fcb->NTRequiredFCB->MainResource),TRUE);

    UDFInterlockedIncrement((PLONG)&(RelatedFileInfo->Fcb->ReferenceCount));
    UDFInterlockedIncrement((PLONG)&(RelatedFileInfo->Dloc->CommonFcb->CommonRefCount));
    UDFReferenceFile__(RelatedFileInfo);
    ASSERT_REF(RelatedFileInfo->Fcb->ReferenceCount >= RelatedFileInfo->RefCount);
} // end UDFAcquireParent()

/*************************************************************************
*
* Function: UDFCommonCreate()
*
* Description:
*   The actual work is performed here. This routine may be invoked in one'
*   of the two possible contexts:
*   (a) in the context of a system worker thread
*   (b) in the context of the original caller
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFCommonCreate(
    PtrUDFIrpContext                PtrIrpContext,
    PIRP                            Irp
    )
{
    NTSTATUS                    RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION          IrpSp = NULL;
    PIO_SECURITY_CONTEXT        PtrSecurityContext = NULL;
    PFILE_OBJECT                PtrNewFileObject = NULL;
    PFILE_OBJECT                PtrRelatedFileObject = NULL;
    LONGLONG                    AllocationSize;     // if we create a new file
    PFILE_FULL_EA_INFORMATION   PtrExtAttrBuffer = NULL;
    ULONG                       RequestedOptions;
    ULONG                       RequestedDisposition;
    USHORT                      FileAttributes;
    USHORT                      TmpFileAttributes;
    USHORT                      ShareAccess;
    ULONG                       ExtAttrLength = 0;
    ACCESS_MASK                 DesiredAccess;
    PACCESS_STATE               AccessState;

    _SEH2_VOLATILE PVCB         Vcb = NULL;
    _SEH2_VOLATILE BOOLEAN      AcquiredVcb = FALSE;
    BOOLEAN                     OpenExisting = FALSE;
    PERESOURCE                  Res1 = NULL;
    PERESOURCE                  Res2 = NULL;
    PERESOURCE                  PagingIoRes = NULL;

//  BOOLEAN                     DirectoryOnlyRequested;
//  BOOLEAN                     FileOnlyRequested;
//  BOOLEAN                     NoBufferingSpecified;
    BOOLEAN                     WriteThroughRequested;
    BOOLEAN                     DeleteOnCloseSpecified;
//  BOOLEAN                     NoExtAttrKnowledge;
//  BOOLEAN                     CreateTreeConnection = FALSE;
//  BOOLEAN                     OpenByFileId;

    // Are we dealing with a page file?
    BOOLEAN                     PageFileManipulation;
    // Is this open for a target directory (used in rename operations)?
    BOOLEAN                     OpenTargetDirectory;
    // Should we ignore case when attempting to locate the object?
    BOOLEAN                     IgnoreCase;

    PtrUDFCCB                   PtrRelatedCCB = NULL, PtrNewCcb = NULL;
    PtrUDFFCB                   PtrRelatedFCB = NULL, PtrNewFcb = NULL;
    PtrUDFNTRequiredFCB         NtReqFcb;

    ULONG                       ReturnedInformation = 0;

    UNICODE_STRING              TargetObjectName;
    UNICODE_STRING              RelatedObjectName;

    UNICODE_STRING              AbsolutePathName;    // '\aaa\cdf\fff\rrrr.tre:s'
    UNICODE_STRING              LocalPath;           // '\aaa\cdf'
    UNICODE_STRING              CurName;             // 'cdf'
    UNICODE_STRING              TailName;            // 'fff\rrrr.tre:s'
    UNICODE_STRING              LastGoodName;        // it depends...
    UNICODE_STRING              LastGoodTail;        // it depends...
    UNICODE_STRING              StreamName;          // ':s'

    PUDF_FILE_INFO              RelatedFileInfo;
    PUDF_FILE_INFO              OldRelatedFileInfo = NULL;
    PUDF_FILE_INFO              NewFileInfo = NULL;
    PUDF_FILE_INFO              LastGoodFileInfo = NULL;
    PWCHAR                      TmpBuffer;
    ULONG                       TreeLength = 0;
//    ULONG                       i = 0;

    BOOLEAN                     StreamOpen = FALSE;
    BOOLEAN                     StreamExists = FALSE;
    BOOLEAN                     RestoreVCBOpenCounter = FALSE;
    BOOLEAN                     RestoreShareAccess = FALSE;
    PWCHAR                      TailNameBuffer = NULL;
    ULONG                       SNameIndex = 0;

    TmPrint(("UDFCommonCreate:\n"));

    ASSERT(PtrIrpContext);
    ASSERT(Irp);

    _SEH2_TRY {

        AbsolutePathName.Buffer =
        LocalPath.Buffer = NULL;
        //  If we were called with our file system device object instead of a
        //  volume device object, just complete this request with STATUS_SUCCESS.
        if (!(PtrIrpContext->TargetDeviceObject->DeviceExtension)) {

            ReturnedInformation = FILE_OPENED;
            try_return(RC = STATUS_SUCCESS);
        }

        AbsolutePathName.Length = AbsolutePathName.MaximumLength =
        LocalPath.Length = LocalPath.MaximumLength = 0;
        // First, get a pointer to the current I/O stack location
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);

        // If the caller cannot block, post the request to be handled
        //  asynchronously
        if (!(PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_CAN_BLOCK)) {
            // We must defer processing of this request since we could
            //  block anytime while performing the create/open ...
            ASSERT(FALSE);
            RC = UDFPostRequest(PtrIrpContext, Irp);
            try_return(RC);
        }

        // Now, we can obtain the parameters specified by the user.
        //  Note that the file object is the new object created by the
        //  I/O Manager in anticipation that this create/open request
        //  will succeed.
        PtrNewFileObject     = IrpSp->FileObject;
        TargetObjectName     = PtrNewFileObject->FileName;
        PtrRelatedFileObject = PtrNewFileObject->RelatedFileObject;

        // If a related file object is present, get the pointers
        //  to the CCB and the FCB for the related file object
        if (PtrRelatedFileObject) {
            PtrRelatedCCB = (PtrUDFCCB)(PtrRelatedFileObject->FsContext2);
            ASSERT(PtrRelatedCCB);
            ASSERT(PtrRelatedCCB->NodeIdentifier.NodeType == UDF_NODE_TYPE_CCB);
            // each CCB in turn points to a FCB
            PtrRelatedFCB = PtrRelatedCCB->Fcb;
            ASSERT(PtrRelatedFCB);
            ASSERT((PtrRelatedFCB->NodeIdentifier.NodeType == UDF_NODE_TYPE_FCB)
                 ||(PtrRelatedFCB->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB));
            RelatedObjectName = PtrRelatedFileObject->FileName;
            if (!(RelatedObjectName.Length) || (RelatedObjectName.Buffer[0] != L'\\')) {
                if(PtrRelatedFCB->FCBName)
                    RelatedObjectName = PtrRelatedFCB->FCBName->ObjectName;
            }
        }

        // Allocation size is only used if a new file is created
        //  or a file is superseded.
        AllocationSize = Irp->Overlay.AllocationSize.QuadPart;

        // Get a ptr to the supplied security context
        PtrSecurityContext = IrpSp->Parameters.Create.SecurityContext;
        AccessState = PtrSecurityContext->AccessState;

        // The desired access can be obtained from the SecurityContext
        DesiredAccess = PtrSecurityContext->DesiredAccess;

        // Two values are supplied in the Create.Options field:
        //  (a) the actual user supplied options
        //  (b) the create disposition
        RequestedOptions = (IrpSp->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS);

        // The file disposition is packed with the user options ...
        //  Disposition includes FILE_SUPERSEDE, FILE_OPEN_IF, etc.
        RequestedDisposition = (IrpSp->Parameters.Create.Options >> 24);// & 0xFF;

//#ifdef UDF_LOG_CREATE_DISPOSITION
        switch(RequestedDisposition) {
        case FILE_SUPERSEDE:
            AdPrint(("    Dispos: FILE_SUPERSEDE\n"));
            break;
        case FILE_OPEN:
            AdPrint(("    Dispos: FILE_OPEN\n"));
            break;
        case FILE_CREATE:
            AdPrint(("    Dispos: FILE_CREATE\n"));
            break;
        case FILE_OPEN_IF:
            AdPrint(("    Dispos: FILE_OPEN_IF\n"));
            break;
        case FILE_OVERWRITE:
            AdPrint(("    Dispos: FILE_OVERWRITE\n"));
            break;
        case FILE_OVERWRITE_IF:
            AdPrint(("    Dispos: FILE_OVERWRITE_IF\n"));
            break;
        default:
            AdPrint(("    Dispos: *** Unknown ***\n"));
            break;
        }
//#endif // UDF_LOG_CREATE_DISPOSITION

        FileAttributes  = (USHORT)(IrpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS);
        ShareAccess = IrpSp->Parameters.Create.ShareAccess;

        // If the FSD does not support EA manipulation, we might return
        //  invalid parameter if the following are supplied.
        //  EA arguments are only used if a new file is created or a file is
        //  superseded

        // But some applications _require_ EA support
        //  (Notepad... rather strange, isn't it ?)

        //         So, for such stupid ones
        // !!! We shall ignore these parameters !!!

//        PtrExtAttrBuffer = (struct _FILE_FULL_EA_INFORMATION *) Irp->AssociatedIrp.SystemBuffer;
//        ExtAttrLength    = IrpSp->Parameters.Create.EaLength;

        // Get the options supplied by the user

#define OpenForBackup (RequestedOptions & FILE_OPEN_FOR_BACKUP_INTENT)
        // User specifies that returned object MUST be a directory.
        //  Lack of presence of this flag does not mean it *cannot* be a
        //  directory *unless* FileOnlyRequested is set (see below)

        //  Presence of the flag however, does require that the returned object be
        //  a directory (container) object.
#define DirectoryOnlyRequested (RequestedOptions & FILE_DIRECTORY_FILE)

        // User specifies that returned object MUST NOT be a directory.
        //  Lack of presence of this flag does not mean it *cannot* be a
        //  file *unless* DirectoryOnlyRequested is set (see above).

        //  Presence of the flag however does require that the returned object be
        //  a simple file (non-container) object.
#define FileOnlyRequested (RequestedOptions & FILE_NON_DIRECTORY_FILE)

        // We cannot cache the file if the following flag is set.
        //  However, things do get a little bit interesting if caching
        //  has been already initiated due to a previous open ...
        //  (maintaining consistency then becomes a little bit more
        //  of a headache - see read/write file descriptions)
#define NoBufferingSpecified (RequestedOptions & FILE_NO_INTERMEDIATE_BUFFERING)

        // Write-through simply means that the FSD must *not* return from
        //  a user write request until the data has been flushed to secondary
        //  storage (either to disks directly connected to the node or across
        //  the network in the case of a redirector)
        WriteThroughRequested = (RequestedOptions & FILE_WRITE_THROUGH) ? TRUE : FALSE;

#define SequentialIoRequested   (RequestedOptions & FILE_SEQUENTIAL_ONLY ? TRUE : FALSE)

        // Not all of the native file system implementations support
        //  the delete-on-close option. All this means is that after the
        //  last close on the FCB has been performed, the FSD should
        //  delete the file. It simply saves the caller from issuing a
        //  separate delete request. Also, some FSD implementations might choose
        //  to implement a Windows NT idiosyncratic behavior wherein we
        //  could create such "delete-on-close" marked files under directories
        //  marked for deletion. Ordinarily, a FSD will not allow us to create
        //  a new file under a directory that has been marked for deletion.
        DeleteOnCloseSpecified = (IrpSp->Parameters.Create.Options & FILE_DELETE_ON_CLOSE) ? TRUE : FALSE;

        if(DeleteOnCloseSpecified) {
            AdPrint(("    DeleteOnClose\n"));
        }

#define NoExtAttrKnowledge /*(RequestedOptions & FILE_NO_EA_KNOWLEDGE) ?*/ TRUE /*: FALSE*/

        // The following flag is only used by the LAN Manager redirector
        //  to  initiate a "new mapping" to a remote share. Typically,
        //  a FSD will not see this flag (especially disk based FSD's)
        // CreateTreeConnection = (RequestedOptions & FILE_CREATE_TREE_CONNECTION) ? TRUE : FALSE;

        // The NTFS file system for exmaple supports the OpenByFileId option.
        //  The FSD may also be able to associate a unique numerical ID with
        //  an on-disk object. The caller would get this ID in a "query file
        //  information" call.

        //  Later, the caller might decide to reopen the object, this time
        //  though it may supply the FSD with the file identifier instead of
        //  a file/path name.
#define OpenByFileId (RequestedOptions & FILE_OPEN_BY_FILE_ID)

        // Are we dealing with a page file?
        PageFileManipulation = (IrpSp->Flags & SL_OPEN_PAGING_FILE) ? TRUE : FALSE;

        // The open target directory flag is used as part of the sequence of
        //  operations performed by the I/O Manager is response to a file/dir
        //  rename operation. See the explanation in the book for details.
        OpenTargetDirectory = (IrpSp->Flags & SL_OPEN_TARGET_DIRECTORY) ? TRUE : FALSE;

        // If the FSD supports case-sensitive file name checks, we may
        //  choose to honor the following flag ...
        IgnoreCase = (IrpSp->Flags & SL_CASE_SENSITIVE) ? FALSE : TRUE;

        // Ensure that the operation has been directed to a valid VCB ...
        Vcb = (PVCB)(PtrIrpContext->TargetDeviceObject->DeviceExtension);
        ASSERT(Vcb);
        ASSERT(Vcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB);
//        Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

        WriteThroughRequested = WriteThroughRequested ||
                                (Vcb->CompatFlags & UDF_VCB_IC_FORCE_WRITE_THROUGH);

        //  Do some preliminary checks to make sure the operation is supported.
        //  We fail in the following cases immediately.
        //      - Open a paging file.
        //      - Open a file with Eas.
        if(PageFileManipulation) {
            ReturnedInformation = 0;
            AdPrint(("Can't create a page file\n"));
            try_return(RC = STATUS_ACCESS_DENIED);
        }
        if(ExtAttrLength) {
            ReturnedInformation = 0;
            AdPrint(("Can't create file with EAs\n"));
            try_return(RC = STATUS_EAS_NOT_SUPPORTED);
        }

        UDFFlushTryBreak(Vcb);

        if (Vcb->SoftEjectReq) {
            AdPrint(("    Eject requested\n"));
            ReturnedInformation = FILE_DOES_NOT_EXIST;
            try_return(RC = STATUS_FILE_INVALID);
        }

        // If the volume has been locked, fail the request
        if ((Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_LOCKED) &&
            (Vcb->VolumeLockPID != GetCurrentPID())) {
            AdPrint(("    Volume is locked\n"));
            RC = STATUS_ACCESS_DENIED;
            try_return(RC);
        }
        // We need EXCLUSIVE access to Vcb to avoid parallel calls to UDFVerifyVcb()
        UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
        AcquiredVcb = TRUE;

        // Disk based file systems might decide to verify the logical volume
        //  (if required and only if removable media are supported) at this time
        RC = UDFVerifyVcb(PtrIrpContext,Vcb);
        if(!NT_SUCCESS(RC))
            try_return(RC);

        UDFConvertExclusiveToSharedLite(&(Vcb->VCBResource));

        ASSERT(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED);

        // We fail in the following cases for Read-Only volumes
        //      - Open a target directory.
        //      - Create a file.
        if(
           (
           ((Vcb->origIntegrityType == INTEGRITY_TYPE_OPEN) &&
            (Vcb->CompatFlags & UDF_VCB_IC_DIRTY_RO))
#ifndef UDF_READ_ONLY_BUILD
            || (Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY)
#endif //UDF_READ_ONLY_BUILD
            ) &&
            (DeleteOnCloseSpecified ||
             OpenTargetDirectory ||
             (RequestedDisposition == FILE_CREATE) ||
             (RequestedDisposition == FILE_OVERWRITE) ||
             (RequestedDisposition == FILE_OVERWRITE_IF) ||
             (RequestedDisposition == FILE_SUPERSEDE) ||
             AllocationSize) ) {
            ReturnedInformation = 0;
            AdPrint(("    Write protected or dirty\n"));
            try_return(RC = STATUS_MEDIA_WRITE_PROTECTED);
        }

/*        if(DesiredAccess & (FILE_READ_EA | FILE_WRITE_EA)) {
            ReturnedInformation = 0;
            AdPrint(("    EAs not supported\n"));
            try_return(RC = STATUS_ACCESS_DENIED);
        }*/

        // ****************
        // If a Volume open is requested, satisfy it now
        // ****************
        if (!(PtrNewFileObject->FileName.Length) && (!PtrRelatedFileObject ||
              (PtrRelatedFCB->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB))) {

            BOOLEAN UndoLock = FALSE;

            AdPrint(("  Opening Volume\n"));
            // If the supplied file name is NULL *and* either there exists
            //  no related file object *or* if a related file object was supplied
            //  but it too refers to a previously opened instance of a logical
            //  volume, this open must be for a logical volume.

            //  Note: the FSD might decide to do "special" things (whatever they
            //  might be) in response to an open request for the logical volume.

            //  Logical volume open requests are done primarily to get/set volume
            //  information, lock the volume, dismount the volume (using the IOCTL
            //  FSCTL_DISMOUNT_VOLUME) etc.

            //  If a volume open is requested, perform checks to ensure that
            //  invalid options have not also been specified ...
            if ((OpenTargetDirectory) || (PtrExtAttrBuffer)) {
                try_return(RC = STATUS_INVALID_PARAMETER);
            }

            if (DirectoryOnlyRequested) {
                // a volume is not a directory
                try_return(RC = STATUS_NOT_A_DIRECTORY);
            }

#ifndef UDF_READ_ONLY_BUILD
            if (DeleteOnCloseSpecified) {
                // delete volume.... hmm
                try_return(RC = STATUS_CANNOT_DELETE);
            }

            if ((RequestedDisposition != FILE_OPEN) && (RequestedDisposition != FILE_OPEN_IF)) {
                // cannot create a new volume, I'm afraid ...
                ReturnedInformation = FILE_DOES_NOT_EXIST;
                try_return(RC = STATUS_ACCESS_DENIED);
            }
#endif //UDF_READ_ONLY_BUILD

            UDFPrint(("  ShareAccess %x, DesiredAccess %x\n", ShareAccess, DesiredAccess));
/*
            if(!(ShareAccess & (FILE_SHARE_WRITE | FILE_SHARE_DELETE)) &&
               !(DesiredAccess & (FILE_GENERIC_WRITE & ~SYNCHRONIZE)) &&
                (ShareAccess & FILE_SHARE_READ) ) {
*/
            if(!(DesiredAccess & ((GENERIC_WRITE | FILE_GENERIC_WRITE) & ~(SYNCHRONIZE | READ_CONTROL))) &&
                (ShareAccess & FILE_SHARE_READ) ) {
                UDFPrint(("  R/O volume open\n"));
            } else {

                UDFPrint(("  R/W volume open\n"));
                if(Vcb->VCBFlags & UDF_VCB_FLAGS_MEDIA_READ_ONLY) {
                    UDFPrint(("  media-ro\n"));
                    try_return(RC = STATUS_MEDIA_WRITE_PROTECTED);
                }
            }

            if(!(ShareAccess & (FILE_SHARE_WRITE | FILE_SHARE_DELETE)) &&
               !(DesiredAccess & ((GENERIC_WRITE | FILE_GENERIC_WRITE) & ~(SYNCHRONIZE | READ_CONTROL))) &&
                (ShareAccess & FILE_SHARE_READ) ) {
                // do nothing
            } else {

                if(!(ShareAccess & FILE_SHARE_READ) ||
                    (DesiredAccess & ((GENERIC_WRITE | FILE_GENERIC_WRITE) & ~(SYNCHRONIZE | READ_CONTROL))) ) {
                    // As soon as OpenVolume flushes the volume
                    // we should complete all pending requests (Close)

                    UDFPrint(("  set UDF_IRP_CONTEXT_FLUSH2_REQUIRED\n"));
                    PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_FLUSH2_REQUIRED;

/*
                    UDFInterlockedIncrement((PLONG)&(Vcb->VCBOpenCount));
                    UDFReleaseResource(&(Vcb->VCBResource));
                    AcquiredVcb = FALSE;

                    if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK)) {
                        UDFCloseAllSystemDelayedInDir(Vcb, Vcb->RootDirFCB->FileInfo);
                    }
#ifdef UDF_DELAYED_CLOSE
                    UDFCloseAllDelayed(Vcb);
#endif //UDF_DELAYED_CLOSE

                    UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
                    AcquiredVcb = TRUE;
                    UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));
*/
                }
            }

            // If the user does not want to share write or delete then we will try
            // and take out a lock on the volume.
            if(!(ShareAccess & (FILE_SHARE_WRITE | FILE_SHARE_DELETE))) {
                // Do a quick check here for handles on exclusive open.
                if ((Vcb->VCBHandleCount) &&
                    !(ShareAccess & FILE_SHARE_READ)) {
                    // Sharing violation
                    UDFPrint(("  !FILE_SHARE_READ + open handles (%d)\n", Vcb->VCBHandleCount));
                    try_return(RC = STATUS_SHARING_VIOLATION);
                }
                if(PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_FLUSH2_REQUIRED) {

                    UDFPrint(("  perform flush\n"));
                    PtrIrpContext->IrpContextFlags &= ~UDF_IRP_CONTEXT_FLUSH2_REQUIRED;

                    UDFInterlockedIncrement((PLONG)&(Vcb->VCBOpenCount));
                    UDFReleaseResource(&(Vcb->VCBResource));
                    AcquiredVcb = FALSE;

                    if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK)) {
                        UDFCloseAllSystemDelayedInDir(Vcb, Vcb->RootDirFCB->FileInfo);
                    }
#ifdef UDF_DELAYED_CLOSE
                    UDFCloseAllDelayed(Vcb);
#endif //UDF_DELAYED_CLOSE

                    UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
                    AcquiredVcb = TRUE;
                    UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));

                    UDFFlushLogicalVolume(NULL, NULL, Vcb);

                    if((ShareAccess & FILE_SHARE_READ) &&
                       ((Vcb->VCBOpenCount - UDF_RESIDUAL_REFERENCE) != (Vcb->VCBOpenCountRO))) {
                        UDFPrint(("  FILE_SHARE_READ + R/W handles: %d(%d) -> STATUS_SHARING_VIOLATION ?\n",
                            Vcb->VCBOpenCount - UDF_RESIDUAL_REFERENCE,
                            Vcb->VCBOpenCountRO));
                        /* we shall not check it here, let System do it in IoCheckShareAccess() */
                        //try_return(RC = STATUS_SHARING_VIOLATION);
                    }
                }
                // Lock the volume
                if(!(ShareAccess & FILE_SHARE_READ)) {
                    UDFPrint(("  set Lock\n"));
                    Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_LOCKED;
                    Vcb->VolumeLockFileObject = PtrNewFileObject;
                    UndoLock = TRUE;
                } else
                if(DesiredAccess & ((GENERIC_WRITE | FILE_GENERIC_WRITE) & ~(SYNCHRONIZE | READ_CONTROL))) {
                    UDFPrint(("  set UDF_IRP_CONTEXT_FLUSH_REQUIRED\n"));
                    PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_FLUSH_REQUIRED;
                }
            }

            PtrNewFcb = (PtrUDFFCB)Vcb;
            ASSERT(!(PtrNewFcb->FCBFlags & UDF_FCB_DELETE_ON_CLOSE));

            RC = UDFOpenFile(Vcb, PtrNewFileObject, PtrNewFcb);
            if (!NT_SUCCESS(RC))
                goto op_vol_accs_dnd;

            PtrNewCcb = (PtrUDFCCB)(PtrNewFileObject->FsContext2);
            if(PtrNewCcb) PtrNewCcb->CCBFlags |= UDF_CCB_VOLUME_OPEN;
            // Check _Security_
            RC = UDFCheckAccessRights(NULL, AccessState, Vcb->RootDirFCB, PtrNewCcb, DesiredAccess, ShareAccess);
            if (!NT_SUCCESS(RC)) {
                AdPrint(("    Access violation (Volume)\n"));
                goto op_vol_accs_dnd;
            }
            // Check _ShareAccess_
            RC = UDFCheckAccessRights(PtrNewFileObject, AccessState, PtrNewFcb, PtrNewCcb, DesiredAccess, ShareAccess);
            if(!NT_SUCCESS(RC)) {
                AdPrint(("    Sharing violation (Volume)\n"));
op_vol_accs_dnd:
                if(UndoLock) {
                    Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_LOCKED;
                    Vcb->VolumeLockFileObject = NULL;
                }
                try_return(RC);
            }

//      NoBufferingSpecified = TRUE;  See #define above
            RequestedOptions |= FILE_NO_INTERMEDIATE_BUFFERING;

            ReturnedInformation = FILE_OPENED;
            UDFNotifyVolumeEvent(PtrNewFileObject, FSRTL_VOLUME_LOCK);
            try_return(RC);
        }

        if((Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) &&
           (!(Vcb->CompatFlags & UDF_VCB_IC_SHOW_BLANK_CD) || UDFGlobalData.AutoFormatCount)) {
            ReturnedInformation = 0;
            AdPrint(("    Can't open anything on blank volume ;)\n"));
            try_return(RC = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        if(UdfIllegalFcbAccess(Vcb,DesiredAccess)) {
            ReturnedInformation = 0;
            AdPrint(("    Illegal share access\n"));
            try_return(RC = STATUS_ACCESS_DENIED);
        }
        // we could mount blank R/RW media in order to allow
        // user-mode applications to get access with Write privileges
        ASSERT(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED);

        // we should check appropriate privilege if OpenForBackup requested
        if(OpenForBackup) {
            if (!SeSinglePrivilegeCheck(SeExports->SeBackupPrivilege, UserMode)) {
                try_return(RC = STATUS_PRIVILEGE_NOT_HELD);
            }
        }

        // The FSD might wish to implement the open-by-id option. The "id"
        //  is some unique numerical representation of the on-disk object.
        //  The caller then therefore give us this file id and the FSD
        //  should be completely capable of "opening" the object (it must
        //  exist since the caller received an id for the object from the
        //  FSD in a "query file" call ...

        //  If the file has been deleted in the meantime, we'll return
        //  "not found"

        // ****************
        // Open by FileID
        // ****************
        if (OpenByFileId) {
            // perform the open ...
            PUNICODE_STRING TmpPath;
            LONGLONG Id;

            UDFPrint(("    open by File ID\n"));
            if(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) {
                ReturnedInformation = 0;
                AdPrint(("    Can't open by FileID on blank volume ;)\n"));
                try_return(RC = STATUS_OBJECT_NAME_NOT_FOUND);
            }

            if (TargetObjectName.Length != sizeof(FILE_ID)) {
                AdPrint(("    Invalid file ID\n"));
                try_return(RC = STATUS_INVALID_PARAMETER);
            }
            Id = *((FILE_ID*)(TargetObjectName.Buffer));
            AdPrint(("  Opening by ID %8.8x%8.8x\n", (ULONG)(Id>>32), (ULONG)Id));
            if ((RequestedDisposition != FILE_OPEN) &&
                (RequestedDisposition != FILE_OPEN_IF)) {
                AdPrint(("    Illegal disposition for ID open\n"));
                try_return(RC = STATUS_ACCESS_DENIED);
            }

            RC = UDFGetOpenParamsByFileId(Vcb, Id, &TmpPath, &IgnoreCase);
            if(!NT_SUCCESS(RC)) {
                AdPrint(("    ID open failed\n"));
                try_return(RC);
            }
            // simulate absolute path open
/*            if(!NT_SUCCESS(RC = MyInitUnicodeString(&TargetObjectName, L"")) ||
               !NT_SUCCESS(RC = MyAppendUnicodeStringToStringTag(&TargetObjectName, TmpPath, MEM_USABS_TAG))) {*/
            if(!NT_SUCCESS(RC = MyCloneUnicodeString(&TargetObjectName, TmpPath))) {
                AdPrint(("    Init String failed\n"));
                try_return(RC);
            }
            //ASSERT(TargetObjectName.Buffer);
            AbsolutePathName = TargetObjectName;
            PtrRelatedFileObject = NULL;
        } else
        // ****************
        // Relative open
        // ****************
        // Now determine the starting point from which to begin the parsing
        if (PtrRelatedFileObject) {
            // We have a user supplied related file object.
            //  This implies a "relative" open i.e. relative to the directory
            //  represented by the related file object ...

            UDFPrint(("    PtrRelatedFileObject %x, FCB %x\n", PtrRelatedFileObject, PtrRelatedFCB));
            //  Note: The only purpose FSD implementations ever have for
            //  the related file object is to determine whether this
            //  is a relative open or not. At all other times (including
            //  during I/O operations), this field is meaningless from
            //  the FSD's perspective.
            if (!(PtrRelatedFCB->FCBFlags & UDF_FCB_DIRECTORY)) {
                // we must have a directory as the "related" object
                RC = STATUS_INVALID_PARAMETER;
                AdPrint(("    Related object must be a directory\n"));
                AdPrint(("    Flags %x\n", PtrRelatedFCB->FCBFlags));
                _SEH2_TRY {
                    AdPrint(("    ObjName %x, ", PtrRelatedFCB->FCBName->ObjectName));
                    AdPrint(("    Name %S\n", PtrRelatedFCB->FCBName->ObjectName.Buffer));
                } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                    AdPrint(("    exception when printing name\n"));
                } _SEH2_END;
                try_return(RC);
            }

            // So we have a directory, ensure that the name begins with
            //  a "\" i.e. begins at the root and does *not* begin with a "\\"
            //  NOTE: This is just an example of the kind of path-name string
            //  validation that a FSD must do. Although the remainder of
            //  the code may not include such checks, any commercial
            //  FSD *must* include such checking (no one else, including
            //  the I/O Manager will perform checks on the FSD's behalf)
            if (!(RelatedObjectName.Length) || (RelatedObjectName.Buffer[0] != L'\\')) {
                AdPrint(("    Wrong pathname (1)\n"));
                RC = STATUS_INVALID_PARAMETER;
                try_return(RC);
            }
            // similarly, if the target file name starts with a "\", it
            //  is wrong since the target file name can no longer be absolute
            ASSERT(TargetObjectName.Buffer || !TargetObjectName.Length);
            if (TargetObjectName.Length && (TargetObjectName.Buffer[0] == L'\\')) {
                AdPrint(("    Wrong pathname (2)\n"));
                RC = STATUS_INVALID_PARAMETER;
                try_return(RC);
            }
            // Create an absolute path-name. We could potentially use
            //  the absolute path-name if we cache previously opened
            //  file/directory object names.
/*            if(!NT_SUCCESS(RC = MyInitUnicodeString(&AbsolutePathName, L"")) ||
               !NT_SUCCESS(RC MyAppendUnicodeStringToStringTag(&AbsolutePathName, &RelatedObjectName, MEM_USABS_TAG)))*/
            if(!NT_SUCCESS(RC = MyCloneUnicodeString(&AbsolutePathName, &RelatedObjectName)))
                try_return(RC);
            if(RelatedObjectName.Length &&
                (RelatedObjectName.Buffer[ (RelatedObjectName.Length/sizeof(WCHAR)) - 1 ] != L'\\')) {
                RC = MyAppendUnicodeToString(&AbsolutePathName, L"\\");
                if(!NT_SUCCESS(RC)) try_return(RC);
            }
            if(!AbsolutePathName.Length ||
                (AbsolutePathName.Buffer[ (AbsolutePathName.Length/sizeof(WCHAR)) - 1 ] != L'\\')) {
                ASSERT(TargetObjectName.Buffer);
                if(TargetObjectName.Length && TargetObjectName.Buffer[0] != L'\\') {
                    RC = MyAppendUnicodeToString(&AbsolutePathName, L"\\");
                    if(!NT_SUCCESS(RC)) try_return(RC);
                }
            }
            //ASSERT(TargetObjectName.Buffer);
            RC = MyAppendUnicodeStringToStringTag(&AbsolutePathName, &TargetObjectName, MEM_USABS_TAG);
            if(!NT_SUCCESS(RC))
                try_return(RC);

        } else {
        // ****************
        // Absolute open
        // ****************
            // The suplied path-name must be an absolute path-name i.e.
            //  starting at the root of the file system tree
            UDFPrint(("    Absolute open\n"));
            ASSERT(TargetObjectName.Buffer);
            if (!TargetObjectName.Length || TargetObjectName.Buffer[0] != L'\\') {
                AdPrint(("    Wrong target name (1)\n"));
                try_return(RC = STATUS_INVALID_PARAMETER);
            }
/*            if(!NT_SUCCESS(RC = MyInitUnicodeString(&AbsolutePathName, L"")) ||
               !NT_SUCCESS(RC = MyAppendUnicodeStringToStringTag(&AbsolutePathName, &TargetObjectName, MEM_USABS_TAG)))*/
            ASSERT(TargetObjectName.Buffer);
            if(!NT_SUCCESS(RC = MyCloneUnicodeString(&AbsolutePathName, &TargetObjectName)))
                try_return(RC);
        }
        // Win 32 protection :)
        if ((AbsolutePathName.Length >= sizeof(WCHAR)*2) &&
            (AbsolutePathName.Buffer[1] == L'\\') &&
            (AbsolutePathName.Buffer[0] == L'\\')) {

            //  If there are still two beginning backslashes, the name is bogus.
            if ((AbsolutePathName.Length > 2*sizeof(WCHAR)) &&
                (AbsolutePathName.Buffer[2] == L'\\')) {
                AdPrint(("    Wrong target name (2)\n"));
                try_return (RC = STATUS_OBJECT_NAME_INVALID);
            }
            //  Slide the name down in the buffer.
            RtlMoveMemory( AbsolutePathName.Buffer,
                           AbsolutePathName.Buffer + 1,
                           AbsolutePathName.Length ); // .Length includes
                                                      //      NULL-terminator
            AbsolutePathName.Length -= sizeof(WCHAR);
        }
        if ( (AbsolutePathName.Length > sizeof(WCHAR) ) &&
            (AbsolutePathName.Buffer[ (AbsolutePathName.Length/sizeof(WCHAR)) - 1 ] == L'\\') ) {

            AbsolutePathName.Length -= sizeof(WCHAR);
        }
        // TERMINATOR (2)   ;)
        AbsolutePathName.Buffer[AbsolutePathName.Length/sizeof(WCHAR)] = 0;

        // Sometimes W2000 decides to duplicate handle of
        // already opened File/Dir. In this case it sends us
        // RelatedFileObject & specifies zero-filled RelativePath
        if(!TargetObjectName.Length) {
            TargetObjectName = AbsolutePathName;
            OpenExisting = TRUE;
        }
        //ASSERT(TargetObjectName.Buffer);

        // ****************
        //  First, check if the caller simply wishes to open the Root
        //  of the file system tree.
        // ****************
        if (AbsolutePathName.Length == sizeof(WCHAR)) {
            AdPrint(("  Opening RootDir\n"));
            // this is an open of the root directory, ensure that the caller
            // has not requested a file only
            if (FileOnlyRequested || (RequestedDisposition == FILE_SUPERSEDE) ||
                 (RequestedDisposition == FILE_OVERWRITE) ||
                 (RequestedDisposition == FILE_OVERWRITE_IF)) {
                AdPrint(("    Can't overwrite RootDir\n"));
                RC = STATUS_FILE_IS_A_DIRECTORY;
                try_return(RC);
            }

#if 0
            CollectStatistics(Vcb, MetaDataReads);
#endif

            if (DeleteOnCloseSpecified) {
                // delete RootDir.... rather strange idea... I dislike it
                AdPrint(("    Can't delete RootDir\n"));
                try_return(RC = STATUS_CANNOT_DELETE);
            }

            PtrNewFcb = Vcb->RootDirFCB;
            RC = UDFOpenFile(Vcb, PtrNewFileObject, PtrNewFcb);
            if(!NT_SUCCESS(RC)) try_return(RC);
//            DbgPrint("UDF: Open/Create RootDir : ReferenceCount %x\n",PtrNewFcb->ReferenceCount);
            UDFReferenceFile__(PtrNewFcb->FileInfo);
            PtrNewCcb = (PtrUDFCCB)(PtrNewFileObject->FsContext2);
            TreeLength = 1;

            RC = UDFCheckAccessRights(PtrNewFileObject, AccessState, PtrNewFcb, PtrNewCcb, DesiredAccess, ShareAccess);
            if(!NT_SUCCESS(RC)) {
                AdPrint(("    Access/Sharing violation (RootDir)\n"));
                try_return(RC);
            }

            ReturnedInformation = FILE_OPENED;

            try_return(RC);
        } // end of OpenRootDir

        _SEH2_TRY {
            AdPrint(("    Opening file %ws %8.8x\n",AbsolutePathName.Buffer, PtrNewFileObject));
        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            AdPrint(("    Exception when printing FN\n"));
        } _SEH2_END;
        // ****************
        // Check if we have DuplicateHandle (or Reopen) request
        // ****************
        if(OpenExisting) {

//            BrutePoint();
            // We don't handle OpenTargetDirectory in this case
            if(OpenTargetDirectory)
                try_return(RC = STATUS_INVALID_PARAMETER);

            // Init environment to simulate normal open procedure behavior
/*            if(!NT_SUCCESS(RC = MyInitUnicodeString(&LocalPath, L"")) ||
               !NT_SUCCESS(RC = MyAppendUnicodeStringToStringTag(&LocalPath, &TargetObjectName, MEM_USLOC_TAG)))*/
            ASSERT(TargetObjectName.Buffer);
            if(!NT_SUCCESS(RC = MyCloneUnicodeString(&LocalPath, &TargetObjectName)))
                try_return(RC);

            ASSERT(PtrRelatedFCB);
            RelatedFileInfo = PtrRelatedFCB->FileInfo;

            RC = STATUS_SUCCESS;
            NewFileInfo =
            LastGoodFileInfo = RelatedFileInfo;

            RelatedFileInfo =
            OldRelatedFileInfo = RelatedFileInfo->ParentFile;
            PtrRelatedFCB = PtrRelatedFCB->ParentFcb;
            // prevent releasing parent structures
            UDFAcquireParent(RelatedFileInfo, &Res1, &Res2);
            TreeLength++;

            if(Res1) UDFReleaseResource(Res1);
            if(Res2) UDFReleaseResource(Res2);

            UDF_CHECK_PAGING_IO_RESOURCE(RelatedFileInfo->Fcb->NTRequiredFCB);
            UDFAcquireResourceExclusive(Res2 = &(RelatedFileInfo->Fcb->NTRequiredFCB->MainResource),TRUE);
            PtrNewFcb = NewFileInfo->Fcb;

            UDF_CHECK_PAGING_IO_RESOURCE(PtrNewFcb->NTRequiredFCB);
            UDFAcquireResourceExclusive(Res1 = &(PtrNewFcb->NTRequiredFCB->MainResource),TRUE);
            UDFReferenceFile__(NewFileInfo);
            TreeLength++;

            goto AlreadyOpened;
        }

        if(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) {
            ReturnedInformation = 0;
            AdPrint(("    Can't open File on blank volume ;)\n"));
            ReturnedInformation = FILE_DOES_NOT_EXIST;
            try_return(RC = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        //AdPrint(("    Opening file %ws %8.8x\n",AbsolutePathName.Buffer, PtrNewFileObject));

        if(AbsolutePathName.Length > UDF_X_PATH_LEN*sizeof(WCHAR)) {
            try_return(RC = STATUS_OBJECT_NAME_INVALID);
        }

        // validate path specified
        // (sometimes we can see here very strange characters ;)
        if(!UDFIsNameValid(&AbsolutePathName, &StreamOpen, &SNameIndex)) {
            AdPrint(("    Absolute path is not valid\n"));
            try_return(RC = STATUS_OBJECT_NAME_INVALID);
        }
        if(StreamOpen && !UDFStreamsSupported(Vcb)) {
            ReturnedInformation = FILE_DOES_NOT_EXIST;
            try_return(RC = STATUS_OBJECT_NAME_INVALID);
        }

        RC = MyInitUnicodeString(&LocalPath, L"");
        if(!NT_SUCCESS(RC))
            try_return(RC);
        if (PtrRelatedFileObject) {
            // Our "start directory" is the one identified
            // by the related file object
            RelatedFileInfo = PtrRelatedFCB->FileInfo;
            if(RelatedFileInfo != Vcb->RootDirFCB->FileInfo) {
                RC = MyAppendUnicodeStringToStringTag(&LocalPath, &(PtrRelatedFCB->FCBName->ObjectName), MEM_USLOC_TAG);
                if(!NT_SUCCESS(RC))
                    try_return(RC);
            }
            if(TargetObjectName.Buffer != AbsolutePathName.Buffer) {
                ASSERT(TargetObjectName.Buffer);
                if(!NT_SUCCESS(RC = MyCloneUnicodeString(&TailName, &TargetObjectName))) {
                    AdPrint(("    Init String 'TargetObjectName' failed\n"));
                    try_return(RC);
                }
                TailNameBuffer = TailName.Buffer;
            } else {
                TailName = AbsolutePathName;
            }
        } else {
            // Start at the root of the file system
            RelatedFileInfo = Vcb->RootDirFCB->FileInfo;
            TailName = AbsolutePathName;
        }

        if(StreamOpen) {
            StreamName = AbsolutePathName;
            StreamName.Buffer += SNameIndex;
            StreamName.Length -= (USHORT)SNameIndex*sizeof(WCHAR);
            // if StreamOpen specified & stream name starts with NULL character
            // we should create Stream Dir at first
            TailName.Length -= (AbsolutePathName.Length - (USHORT)SNameIndex*sizeof(WCHAR));
            AbsolutePathName.Length = (USHORT)SNameIndex*sizeof(WCHAR);
        }
        CurName.MaximumLength = TailName.MaximumLength;

        RC = STATUS_SUCCESS;
        LastGoodName.Length = 0;
        LastGoodFileInfo = RelatedFileInfo;
        // reference RelatedObject to prevent releasing parent structures
        UDFAcquireParent(RelatedFileInfo, &Res1, &Res2);
        TreeLength++;

        // go into a loop parsing the supplied name

        //  Note that we may have to "open" intermediate directory objects
        //  while traversing the path. We should __try to reuse existing code
        //  whenever possible therefore we should consider using a common
        //  open routine regardless of whether the open is on behalf of the
        //  caller or an intermediate (internal) open performed by the driver.

        // ****************
        // now we'll parse path to desired file
        // ****************

        while (TRUE) {

            // remember last 'good' ('good' means NO ERRORS before) path tail
            if(NT_SUCCESS(RC)) {
                LastGoodTail = TailName;
                while(LastGoodTail.Buffer[0] == L'\\') {
                    LastGoodTail.Buffer++;
                    LastGoodTail.Length -= sizeof(WCHAR);
                }
            }
            // get next path part...
            TmpBuffer = TailName.Buffer;
            TailName.Buffer = UDFDissectName(TailName.Buffer,&(CurName.Length) );
            TailName.Length -= (USHORT)((ULONG_PTR)(TailName.Buffer) - (ULONG_PTR)TmpBuffer);
            CurName.Buffer = TailName.Buffer - CurName.Length;
            CurName.Length *= sizeof(WCHAR);
            CurName.MaximumLength = CurName.Length + sizeof(WCHAR);
            // check if we have already opened the component before last one
            // in this case OpenTargetDir request will be served in a special
            // way...
            if(OpenTargetDirectory && NT_SUCCESS(RC) && !TailName.Length) {
                // check if we should open SDir..
                if(!StreamOpen ||
                   (TailName.Buffer[0]!=L':')) {
                    // no, we should not. Continue with OpenTargetDir
                    break;
                }
            }

            if( CurName.Length &&
               (NT_SUCCESS(RC) || !StreamOpen)) {
                // ...wow! non-zero! try to open!
                if(!NT_SUCCESS(RC)) {
                    AdPrint(("    Error opening path component\n"));
                    // we haven't reached last name part... hm..
                    // probably, the path specified is invalid..
                    // or we had a hard error... What else can we do ?
                    // Only say ..CK OFF !!!!
                    if(RC == STATUS_OBJECT_NAME_NOT_FOUND)
                        RC = STATUS_OBJECT_PATH_NOT_FOUND;
                    ReturnedInformation = FILE_DOES_NOT_EXIST;
                    try_return(RC);
                }

                ASSERT_REF(RelatedFileInfo->Fcb->ReferenceCount >= RelatedFileInfo->RefCount);

                if(RelatedFileInfo && (TreeLength>1)) {
                    // it was an internal Open operation. Thus, assume
                    // RelatedFileInfo's Fcb to be valid
                    RelatedFileInfo->Fcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
                    RelatedFileInfo->Fcb->FCBFlags |= UDF_FCB_VALID;
                }
                // check path fragment size
                if(CurName.Length > UDF_X_NAME_LEN * sizeof(WCHAR)) {
                    AdPrint(("    Path component is too long\n"));
                    try_return(RC = STATUS_OBJECT_NAME_INVALID);
                }
                // ...and now release previously acquired objects,
                if(Res1) UDFReleaseResource(Res1);
                if(Res2) {
                    UDFReleaseResource(Res2);
                    Res2 = NULL;
                }
                // acquire new _parent_ directory & try to open what
                // we want.

                UDF_CHECK_PAGING_IO_RESOURCE(RelatedFileInfo->Fcb->NTRequiredFCB);
                UDFAcquireResourceExclusive(Res1 = &(RelatedFileInfo->Fcb->NTRequiredFCB->MainResource),TRUE);

                // check traverse rights
                RC = UDFCheckAccessRights(NULL, NULL, RelatedFileInfo->Fcb, PtrRelatedCCB, FILE_TRAVERSE, 0);
                if(!NT_SUCCESS(RC)) {
                    NewFileInfo = NULL;
                    AdPrint(("    Traverse check failed\n"));
                    goto Skip_open_attempt;
                }
                // check if we should open normal File/Dir or SDir
                if(CurName.Buffer[0] != ':') {
                    // standard open, nothing interesting....
                    RC = UDFOpenFile__(Vcb,
                                       IgnoreCase,TRUE,&CurName,
                                       RelatedFileInfo,&NewFileInfo,NULL);
                    if(RC == STATUS_FILE_DELETED) {
                        // file has gone, but system still remembers it...
                        NewFileInfo = NULL;
                        AdPrint(("    File deleted\n"));
                        RC = STATUS_ACCESS_DENIED;
#ifdef UDF_DBG
                    } else
                    if(RC == STATUS_NOT_A_DIRECTORY) {
                        AdPrint(("    Not a directory\n"));
#endif // UDF_DBG
                    } else
                    if(RC == STATUS_SHARING_PAUSED) {
                        AdPrint(("    Dloc is being initialized\n"));
                        BrutePoint();
                        RC = STATUS_SHARING_VIOLATION;
                    }
                } else {
                    // And here we should open Stream Dir (if any, of cource)
                    RC = UDFOpenStreamDir__(Vcb, RelatedFileInfo, &NewFileInfo);
                    if(NT_SUCCESS(RC)) {
SuccessOpen_SDir:
                        // this indicates that we needn't Stream Dir creation
                        StreamExists = TRUE;
                        StreamName.Buffer++;
                        StreamName.Length-=sizeof(WCHAR);
                        // update TailName
                        TailName = StreamName;
                    } else
                    if(RC == STATUS_NOT_FOUND) {
#ifndef UDF_READ_ONLY_BUILD
                        // Stream Dir doesn't exist, but caller wants it to be
                        // created. Lets try to help him...
                        if((RequestedDisposition == FILE_CREATE) ||
                           (RequestedDisposition == FILE_OPEN_IF) ||
                           (RequestedDisposition == FILE_OVERWRITE_IF) ||
                            OpenTargetDirectory ) {
                            RC = UDFCreateStreamDir__(Vcb, RelatedFileInfo, &NewFileInfo);
                            if(NT_SUCCESS(RC))
                                goto SuccessOpen_SDir;
                        }
#endif //UDF_READ_ONLY_BUILD
                    }
/*                } else {
                    AdPrint(("    File deleted (2)\n"));
                    RC = STATUS_ACCESS_DENIED;*/
                }
#if 0
                CollectStatistics(Vcb, MetaDataReads);
#endif

Skip_open_attempt:

                // check if we have successfully opened path component
                if(NT_SUCCESS(RC)) {
                    // Yesss !!!
                    if (!(PtrNewFcb = NewFileInfo->Fcb)) {
                        // It is a first open operation
                        // Allocate new FCB
                        // Here we set FileObject pointer to NULL to avoid
                        // new CCB allocation
                        RC = UDFFirstOpenFile(Vcb,
                                       NULL, &PtrNewFcb, RelatedFileInfo, NewFileInfo,
                                       &LocalPath, &CurName);

                        if(!NT_SUCCESS(RC)) {
                            BrutePoint();
                            AdPrint(("    Can't perform FirstOpen\n"));
                            UDFCloseFile__(Vcb, NewFileInfo);
                            if(PtrNewFcb) UDFCleanUpFCB(PtrNewFcb);
                            PtrNewFcb = NULL;
                            NewFileInfo->Fcb = NULL;
                            if(UDFCleanUpFile__(Vcb, NewFileInfo)) {
                                MyFreePool__(NewFileInfo);
                                NewFileInfo = NULL;
                            }
                            try_return(RC);
                        }
                    } else {
                        // It is not a first open operation
                        // Validate Fcb. It is possible to get
                        // not completly initialized Fcb here.
                        if(!(PtrNewFcb->FCBFlags & UDF_FCB_VALID)) {
                            BrutePoint();
                            AdPrint(("    Fcb not valid\n"));
                            UDFCloseFile__(Vcb, NewFileInfo);
                            PtrNewFcb = NULL;
                            if(UDFCleanUpFile__(Vcb, NewFileInfo)) {
                                MyFreePool__(NewFileInfo);
                                NewFileInfo = NULL;
                            }
                            try_return(RC = STATUS_ACCESS_DENIED);
                        }
                    }
                    // Acquire newly opened File...
                    Res2 = Res1;
                    UDF_CHECK_PAGING_IO_RESOURCE(NewFileInfo->Fcb->NTRequiredFCB);
                    UDFAcquireResourceExclusive(Res1 = &(NewFileInfo->Fcb->NTRequiredFCB->MainResource),TRUE);
                    // ...and reference it
                    UDFInterlockedIncrement((PLONG)&(PtrNewFcb->ReferenceCount));
                    UDFInterlockedIncrement((PLONG)&(PtrNewFcb->NTRequiredFCB->CommonRefCount));

                    ASSERT_REF(PtrNewFcb->ReferenceCount >= NewFileInfo->RefCount);
                    // update unwind information
                    LastGoodFileInfo = NewFileInfo;
                    LastGoodName = CurName;
                    TreeLength++;
                    // update current path
                    if(!StreamOpen ||
                         ((CurName.Buffer[0] != L':') &&
                          (!LocalPath.Length || (LocalPath.Buffer[LocalPath.Length/sizeof(WCHAR)-1] != L':'))) ) {
                        // we should not insert '\' before or after ':'
                        ASSERT(!LocalPath.Length ||
                               (LocalPath.Buffer[LocalPath.Length/2-1] != L'\\'));
                        RC = MyAppendUnicodeToString(&LocalPath, L"\\");
                        if(!NT_SUCCESS(RC)) try_return(RC);
                    }
                    RC = MyAppendUnicodeStringToStringTag(&LocalPath, &CurName, MEM_USLOC_TAG);
                    if(!NT_SUCCESS(RC))
                        try_return(RC);
//                    DbgPrint("UDF: Open/Create File %ws : ReferenceCount %x\n",CurName.Buffer,PtrNewFcb->ReferenceCount);
                } else {
                    AdPrint(("    Can't open file\n"));
                    // We have failed durring last Open attempt
                    // Roll back to last good state
                    PtrUDFNTRequiredFCB NtReqFcb = NULL;
                    // Cleanup FileInfo if any
                    if(NewFileInfo) {
                        PtrNewFcb = NewFileInfo->Fcb;
                        // acquire appropriate resource if possible
                        if(PtrNewFcb &&
                           PtrNewFcb->NTRequiredFCB) {
                            NtReqFcb = PtrNewFcb->NTRequiredFCB;
                            Res2 = Res1;
                            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                            UDFAcquireResourceExclusive(Res1 = &(NtReqFcb->MainResource),TRUE);
                        }
                        // cleanup pointer to Fcb in FileInfo to allow
                        // UDF_INFO package release FileInfo if there are
                        // no more references
                        if(PtrNewFcb &&
                           !PtrNewFcb->ReferenceCount &&
                           !PtrNewFcb->OpenHandleCount) {
                            NewFileInfo->Fcb = NULL;
                        }
                        // cleanup pointer to CommonFcb in Dloc to allow
                        // UDF_INFO package release Dloc if there are
                        // no more references
                        if(NewFileInfo->Dloc &&
                           !NewFileInfo->Dloc->LinkRefCount &&
                           (!PtrNewFcb || !PtrNewFcb->ReferenceCount)) {
                            NewFileInfo->Dloc->CommonFcb = NULL;
                        }
                        // try to release FileInfo
                        if(UDFCleanUpFile__(Vcb, NewFileInfo)) {
                            ASSERT(!PtrNewFcb);
                            if(PtrNewFcb) {
                                BrutePoint();
                                UDFCleanUpFCB(PtrNewFcb);
                            }
                            MyFreePool__(NewFileInfo);
                        } else {
                            // if we can't release FileInfo
                            // restore pointers to Fcb & CommonFcb in
                            // FileInfo & Dloc
                            NewFileInfo->Fcb = PtrNewFcb;
                            if(NtReqFcb)
                                NewFileInfo->Dloc->CommonFcb = NtReqFcb;
                        }
                        // forget about last FileInfo & Fcb,
                        // further unwind staff needs only last good
                        // structures
                        PtrNewFcb = NULL;
                        NewFileInfo = NULL;
                    }
                }

                // should return error if 'delete in progress'
                if(LastGoodFileInfo->Fcb->FCBFlags & (UDF_FCB_DELETE_ON_CLOSE |
                                                      UDF_FCB_DELETED |
                                                      UDF_FCB_POSTED_RENAME)) {
                    AdPrint(("  Return DeletePending (no err)\n"));
                    try_return(RC = STATUS_DELETE_PENDING);
                }
                // update last good state information...
                OldRelatedFileInfo = RelatedFileInfo;
                RelatedFileInfo = NewFileInfo;
                // ...and go to the next open cycle
            } else {
                // ************
                if(StreamOpen && (RC == STATUS_NOT_FOUND))
                    // handle SDir return code
                    RC = STATUS_OBJECT_NAME_NOT_FOUND;
                if(RC == STATUS_OBJECT_NAME_NOT_FOUND) {
                    // good path, but no such file.... Amen
                    // break open loop and continue with Create
                    break;
                }
                if (!NT_SUCCESS(RC)) {
                    // Hard error or damaged data structures ...
#ifdef UDF_DBG
                    if((RC != STATUS_OBJECT_PATH_NOT_FOUND) &&
                       (RC != STATUS_ACCESS_DENIED) &&
                       (RC != STATUS_NOT_A_DIRECTORY)) {
                        AdPrint(("    Hard error or damaged data structures\n"));
                    }
#endif // UDF_DBG
                    // ... and exit with error
                    try_return(RC);
                }
                // discard changes for last successfully opened file
                UDFInterlockedDecrement((PLONG)&(PtrNewFcb->ReferenceCount));
                UDFInterlockedDecrement((PLONG)&(PtrNewFcb->NTRequiredFCB->CommonRefCount));
                RC = STATUS_SUCCESS;
                ASSERT(!OpenTargetDirectory);
                // break open loop and continue with Open
                // (Create will be skipped)
                break;
            }
        } // end of while(TRUE)

        // ****************
        // If "open target directory" was specified
        // ****************
        if(OpenTargetDirectory) {

            if(!UDFIsADirectory(LastGoodFileInfo)) {
                AdPrint(("    Not a directory (2)\n"));
                RC = STATUS_NOT_A_DIRECTORY;
            }
            if(!NT_SUCCESS(RC) ||
               TailName.Length) {
                AdPrint(("    Target name should not contain (back)slashes\n"));
                NewFileInfo = NULL;
                try_return(RC = STATUS_OBJECT_NAME_INVALID);
            }

            NewFileInfo = LastGoodFileInfo;
            RtlCopyUnicodeString(&(PtrNewFileObject->FileName), &CurName);

            // now we have to check if last component exists...
            if(NT_SUCCESS(RC = UDFFindFile__(Vcb, IgnoreCase,
                                             &CurName, RelatedFileInfo))) {
                // file exists, set this information in the Information field
                ReturnedInformation = FILE_EXISTS;
                AdPrint(("  Open Target: FILE_EXISTS\n"));
            } else
            if(RC == STATUS_OBJECT_NAME_NOT_FOUND) {
#ifdef UDF_DBG
                // check name. If there are '\\'s in TailName, some
                // directories in path specified do not exist
                for(TmpBuffer = LastGoodTail.Buffer; *TmpBuffer; TmpBuffer++) {
                    if((*TmpBuffer) == L'\\') {
                        ASSERT(FALSE);
                        AdPrint(("    Target name should not contain (back)slashes\n"));
                        try_return(RC = STATUS_OBJECT_NAME_INVALID);
                    }
                }
#endif // UDF_DBG
                // Tell the I/O Manager that file does not exit
                ReturnedInformation = FILE_DOES_NOT_EXIST;
                AdPrint(("  Open Target: FILE_DOES_NOT_EXIST\n"));
                RC = STATUS_SUCCESS; // is already set here
            } else {
                AdPrint(("  Open Target: unexpected error\n"));
                NewFileInfo = NULL;
                ReturnedInformation = FILE_DOES_NOT_EXIST;
                try_return(RC = STATUS_OBJECT_NAME_INVALID);
            }

//          RC = STATUS_SUCCESS; // is already set here

            // Update the file object FsContext and FsContext2 fields
            //  to reflect the fact that the parent directory of the
            //  target has been opened
            PtrNewFcb = NewFileInfo->Fcb;
            UDFInterlockedDecrement((PLONG)&(PtrNewFcb->ReferenceCount));
            UDFInterlockedDecrement((PLONG)&(PtrNewFcb->NTRequiredFCB->CommonRefCount));
            RC = UDFOpenFile(Vcb, PtrNewFileObject, PtrNewFcb);
            ASSERT_REF(PtrNewFcb->ReferenceCount >= NewFileInfo->RefCount);
            if (!NT_SUCCESS(RC)) {
                AdPrint(("    Can't perform OpenFile operation for target\n"));
                try_return(RC);
            }
            PtrNewCcb = (PtrUDFCCB)(PtrNewFileObject->FsContext2);

            ASSERT(Res1);
            RC = UDFCheckAccessRights(PtrNewFileObject, AccessState, PtrNewFcb, PtrNewCcb, DesiredAccess, ShareAccess);
            if(!NT_SUCCESS(RC)) {
                AdPrint(("    Access/Share access check failed (Open Target)\n"));
            }

            try_return(RC);
        }

        // ****************
        // should we CREATE a new file ?
        // ****************
        if (!NT_SUCCESS(RC)) {
            if (RC == STATUS_OBJECT_NAME_NOT_FOUND ||
                RC == STATUS_OBJECT_PATH_NOT_FOUND) {
                if( ((RequestedDisposition == FILE_OPEN) ||
                    (RequestedDisposition == FILE_OVERWRITE)) /*&&
                    (!StreamOpen || !StreamExists)*/ ){
                    ReturnedInformation = FILE_DOES_NOT_EXIST;
                    AdPrint(("    File doesn't exist\n"));
                    try_return(RC);
                }
            } else {
                //  Any other operation return STATUS_ACCESS_DENIED.
                AdPrint(("    Can't create due to unexpected error\n"));
                try_return(RC);
            }
            // Object was not found, create if requested
            if ((RequestedDisposition != FILE_CREATE) && (RequestedDisposition != FILE_OPEN_IF) &&
                 (RequestedDisposition != FILE_OVERWRITE_IF) && (RequestedDisposition != FILE_SUPERSEDE)) {
                AdPrint(("    File doesn't exist (2)\n"));
                ReturnedInformation = FILE_DOES_NOT_EXIST;
                try_return(RC);
            }
            // Check Volume ReadOnly attr
#ifndef UDF_READ_ONLY_BUILD
            if((Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY)) {
#endif //UDF_READ_ONLY_BUILD
                ReturnedInformation = 0;
                AdPrint(("    Write protected\n"));
                try_return(RC = STATUS_MEDIA_WRITE_PROTECTED);
#ifndef UDF_READ_ONLY_BUILD
            }
            // Check r/o + delete on close
            if(DeleteOnCloseSpecified &&
               (FileAttributes & FILE_ATTRIBUTE_READONLY)) {
                AdPrint(("    Can't create r/o file marked for deletion\n"));
                try_return(RC = STATUS_CANNOT_DELETE);
            }

            // Create a new file/directory here ...
            if(StreamOpen)
                StreamName.Buffer[StreamName.Length/sizeof(WCHAR)] = 0;
            for(TmpBuffer = LastGoodTail.Buffer; *TmpBuffer; TmpBuffer++) {
                if((*TmpBuffer) == L'\\') {
                    AdPrint(("    Target name should not contain (back)slashes\n"));
                    try_return(RC = STATUS_OBJECT_NAME_INVALID);
                }
            }
            if(  DirectoryOnlyRequested &&
               ((IrpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_TEMPORARY) ||
                 StreamOpen || FALSE)) {
                AdPrint(("    Creation of _temporary_ directory not permited\n"));
                try_return(RC = STATUS_INVALID_PARAMETER);
            }
            // check access rights
            ASSERT(Res1);
            RC = UDFCheckAccessRights(NULL, NULL, OldRelatedFileInfo->Fcb, PtrRelatedCCB, DirectoryOnlyRequested ? FILE_ADD_SUBDIRECTORY : FILE_ADD_FILE, 0);
            if(!NT_SUCCESS(RC)) {
                AdPrint(("    Creation of File/Dir not permitted\n"));
                try_return(RC);
            }
            // Note that a FCB structure will be allocated at this time
            // and so will a CCB structure. Assume that these are called
            // PtrNewFcb and PtrNewCcb respectively.
            // Further, note that since the file is being created, no other
            // thread can have the file stream open at this time.
            RelatedFileInfo = OldRelatedFileInfo;

            RC = UDFCreateFile__(Vcb, IgnoreCase, &LastGoodTail, 0, 0,
                     Vcb->UseExtendedFE || (StreamOpen && !StreamExists),
                     (RequestedDisposition == FILE_CREATE), RelatedFileInfo, &NewFileInfo);
            if(!NT_SUCCESS(RC)) {
                AdPrint(("    Creation error\n"));
Creation_Err_1:
                if(NewFileInfo) {
                    PtrNewFcb = NewFileInfo->Fcb;
                    ASSERT(!PtrNewFcb);
                    if(PtrNewFcb &&
                       !PtrNewFcb->ReferenceCount &&
                       !PtrNewFcb->OpenHandleCount) {
                        NewFileInfo->Fcb = NULL;
                    }
                    if(NewFileInfo->Dloc &&
                       !NewFileInfo->Dloc->LinkRefCount) {
                        NewFileInfo->Dloc->CommonFcb = NULL;
                    }
                    if(UDFCleanUpFile__(Vcb, NewFileInfo)) {
                        if(PtrNewFcb) {
                            BrutePoint();
                            UDFCleanUpFCB(PtrNewFcb);
                        }
                        MyFreePool__(NewFileInfo);
                        PtrNewFcb = PtrNewFcb;
                    } else {
                        NewFileInfo->Fcb = PtrNewFcb;
                    }
                    PtrNewFcb = NULL;
                }
                try_return(RC);
            }
            // Update parent object
            if((Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_WRITE) &&
               PtrRelatedFCB &&
               PtrRelatedFileObject &&
               (PtrRelatedFCB->FileInfo == NewFileInfo->ParentFile)) {
                PtrRelatedFileObject->Flags |= (FO_FILE_MODIFIED | FO_FILE_SIZE_CHANGED);
            }
#if 0
            CollectStatistics(Vcb, MetaDataWrites);
#endif

            if(DirectoryOnlyRequested) {
                // user wants the directory to be created
                RC = UDFRecordDirectory__(Vcb, NewFileInfo);
                if(!NT_SUCCESS(RC)) {
                AdPrint(("    Can't transform to directory\n"));
Undo_Create_1:
                    if((RC != STATUS_FILE_IS_A_DIRECTORY) &&
                       (RC != STATUS_NOT_A_DIRECTORY) &&
                       (RC != STATUS_ACCESS_DENIED)) {
                        UDFFlushFile__(Vcb, NewFileInfo);
                        UDFUnlinkFile__(Vcb, NewFileInfo, TRUE);
                    }
                    UDFCloseFile__(Vcb, NewFileInfo);
                    BrutePoint();
                    goto Creation_Err_1;
                }
#if 0
                CollectStatistics(Vcb, MetaDataWrites);
#endif
            } else if(AllocationSize) {
                // set initial file size
/*                if(!NT_SUCCESS(RC = UDFResizeFile__(Vcb, NewFileInfo, AllocationSize))) {
                    AdPrint(("    Can't set initial file size\n"));
                    goto Undo_Create_1;
                }
                CollectStatistics(Vcb, MetaDataWrites);*/
            }

            if(StreamOpen && !StreamExists) {

                // PHASE 0

                // Open the newly created object (file)
                if (!(PtrNewFcb = NewFileInfo->Fcb)) {
                    // It is a first open operation
                    // Allocate new FCB
                    // Here we set FileObject pointer to NULL to avoid
                    // new CCB allocation
                    RC = UDFFirstOpenFile(Vcb,
                                   NULL, &PtrNewFcb, RelatedFileInfo, NewFileInfo,
                                   &LocalPath, &LastGoodTail);
                    if(!NT_SUCCESS(RC)) {
                        AdPrint(("    Can't perform FirstOpenFile operation for file to contain stream\n"));
                        BrutePoint();
                        UDFCleanUpFCB(NewFileInfo->Fcb);
                        NewFileInfo->Fcb = NULL;
                        goto Creation_Err_1;
                    }
                } else {
                    BrutePoint();
                }

                // Update unwind information
                TreeLength++;
                LastGoodFileInfo = NewFileInfo;
                // update FCB tree
                RC = MyAppendUnicodeToString(&LocalPath, L"\\");
                if(!NT_SUCCESS(RC)) try_return(RC);
                RC = MyAppendUnicodeStringToStringTag(&LocalPath, &LastGoodTail, MEM_USLOC_TAG);
                if(!NT_SUCCESS(RC))
                    goto Creation_Err_1;
                UDFInterlockedIncrement((PLONG)&(PtrNewFcb->ReferenceCount));
                UDFInterlockedIncrement((PLONG)&(PtrNewFcb->NTRequiredFCB->CommonRefCount));
                ASSERT_REF(PtrNewFcb->ReferenceCount >= NewFileInfo->RefCount);
                PtrNewFcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
                PtrNewFcb->FCBFlags |= UDF_FCB_VALID;

                UDFNotifyFullReportChange( Vcb, NewFileInfo,
                                           UDFIsADirectory(NewFileInfo) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                           FILE_ACTION_ADDED);

                // PHASE 1

                // we need to create Stream Dir
                RelatedFileInfo = NewFileInfo;
                RC = UDFCreateStreamDir__(Vcb, RelatedFileInfo, &NewFileInfo);
                if(!NT_SUCCESS(RC)) {
                    AdPrint(("    Can't create SDir\n"));
                    BrutePoint();
                    goto Creation_Err_1;
                }
#if 0
                CollectStatistics(Vcb, MetaDataWrites);
#endif

                // normalize stream name
                StreamName.Buffer++;
                StreamName.Length-=sizeof(WCHAR);
                // Open the newly created object
                if (!(PtrNewFcb = NewFileInfo->Fcb)) {
                    // It is a first open operation
                    // Allocate new FCB
                    // Here we set FileObject pointer to NULL to avoid
                    // new CCB allocation
                    RC = UDFFirstOpenFile(Vcb,
                                   NULL, &PtrNewFcb, RelatedFileInfo, NewFileInfo,
                                   &LocalPath, &(UDFGlobalData.UnicodeStrSDir));
                } else {
                    BrutePoint();
                }
                if(!NT_SUCCESS(RC)) {
                    AdPrint(("    Can't perform OpenFile operation for SDir\n"));
                    BrutePoint();
                    goto Creation_Err_1;
                }

                // Update unwind information
                TreeLength++;
                LastGoodFileInfo = NewFileInfo;
                // update FCB tree
                RC = MyAppendUnicodeStringToStringTag(&LocalPath, &(UDFGlobalData.UnicodeStrSDir), MEM_USLOC_TAG);
                if(!NT_SUCCESS(RC)) {
                    AdPrint(("    Can't append UNC str\n"));
                    BrutePoint();
                    goto Creation_Err_1;
                }
                UDFInterlockedIncrement((PLONG)&(PtrNewFcb->ReferenceCount));
                UDFInterlockedIncrement((PLONG)&(PtrNewFcb->NTRequiredFCB->CommonRefCount));
                ASSERT_REF(PtrNewFcb->ReferenceCount >= NewFileInfo->RefCount);
                PtrNewFcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
                PtrNewFcb->FCBFlags |= UDF_FCB_VALID;

                // PHASE 2

                // create stream
                RelatedFileInfo = NewFileInfo;
                RC = UDFCreateFile__(Vcb, IgnoreCase, &StreamName, 0, 0,
                         Vcb->UseExtendedFE, (RequestedDisposition == FILE_CREATE),
                         RelatedFileInfo, &NewFileInfo);
                if(!NT_SUCCESS(RC)) {
                    AdPrint(("    Can't create Stream\n"));
                    BrutePoint();
                    goto Creation_Err_1;
                }
#if 0
                CollectStatistics(Vcb, MetaDataWrites);
#endif

                // Update unwind information
                LastGoodTail = StreamName;
            }
            // NT wants ARCHIVE bit to be set on Files
            if(!DirectoryOnlyRequested)
                FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
            // Open the newly created object
            if (!(PtrNewFcb = NewFileInfo->Fcb)) {
                // It is a first open operation
#ifndef IFS_40
                // Set attributes for the file ...
                UDFAttributesToUDF(UDFDirIndex(UDFGetDirIndexByFileInfo(NewFileInfo),NewFileInfo->Index),
                                   NewFileInfo->Dloc->FileEntry, FileAttributes);
#endif //IFS_40
                // Allocate new FCB
                // Here we set FileObject pointer to NULL to avoid
                // new CCB allocation
                RC = UDFFirstOpenFile(Vcb,
                               PtrNewFileObject, &PtrNewFcb, RelatedFileInfo, NewFileInfo,
                               &LocalPath, &LastGoodTail);
            } else {
                BrutePoint();
            }

            if(!NT_SUCCESS(RC)) {
                AdPrint(("    Can't perform OpenFile operation for file or stream\n"));
                BrutePoint();
                goto Undo_Create_1;
            }

            PtrNewFcb->NTRequiredFCB->CommonFCBHeader.FileSize.QuadPart =
            PtrNewFcb->NTRequiredFCB->CommonFCBHeader.ValidDataLength.QuadPart = 0;
            if(AllocationSize) {
                // inform NT about size changes
                PtrNewFcb->NTRequiredFCB->CommonFCBHeader.AllocationSize.QuadPart = AllocationSize;
                MmPrint(("    CcIsFileCached()\n"));
                if(CcIsFileCached(PtrNewFileObject)) {
                     MmPrint(("    CcSetFileSizes()\n"));
                     BrutePoint();
                     CcSetFileSizes(PtrNewFileObject, (PCC_FILE_SIZES)&(PtrNewFcb->NTRequiredFCB->CommonFCBHeader.AllocationSize));
                     PtrNewFcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;
                }
            }

            // Update unwind information
            TreeLength++;
            LastGoodFileInfo = NewFileInfo;

            // Set the Share Access for the file stream.
            // The FCBShareAccess field will be set by the I/O Manager.
            PtrNewCcb = (PtrUDFCCB)(PtrNewFileObject->FsContext2);
            RC = UDFSetAccessRights(PtrNewFileObject, AccessState, PtrNewFcb, PtrNewCcb, DesiredAccess, ShareAccess);

            if(!NT_SUCCESS(RC)) {
                AdPrint(("    Can't set Access Rights on Create\n"));
                BrutePoint();
                UDFFlushFile__(Vcb, NewFileInfo);
                UDFUnlinkFile__(Vcb, NewFileInfo, TRUE);
                try_return(RC);
            }

#ifdef IFS_40
            // Set attributes for the file ...
            UDFAttributesToUDF(UDFDirIndex(UDFGetDirIndexByFileInfo(NewFileInfo),NewFileInfo->Index),
                               NewFileInfo->Dloc->FileEntry, FileAttributes);
            // It is rather strange for me, but NT requires us to allow
            // Create operation for r/o + WriteAccess, but denies all
            // the rest operations in this case. Thus, we should update
            // r/o flag in Fcb _after_ Access check :-/
            if(FileAttributes & FILE_ATTRIBUTE_READONLY)
                PtrNewFcb->FCBFlags |= UDF_FCB_READ_ONLY;
#endif //IFS_40
            // We call the notify package to report that the
            // we have added a stream.
            if(UDFIsAStream(NewFileInfo)) {
                UDFNotifyFullReportChange( Vcb, NewFileInfo,
                                           FILE_NOTIFY_CHANGE_STREAM_NAME,
                                           FILE_ACTION_ADDED_STREAM );
            } else {
                UDFNotifyFullReportChange( Vcb, NewFileInfo,
                                           UDFIsADirectory(NewFileInfo) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                           FILE_ACTION_ADDED);
            }
/*#ifdef UDF_DBG
            {
                ULONG i;
                PDIR_INDEX_HDR hDirIndex = NewFileInfo->ParentFile->Dloc->DirIndex;

                for(i=0;DirIndex[i].FName.Buffer;i++) {
                    AdPrint(("%ws\n", DirIndex[i].FName.Buffer));
                }
            }
#endif*/
            ReturnedInformation = FILE_CREATED;

            try_return(RC);
#endif //UDF_READ_ONLY_BUILD

        }

AlreadyOpened:

        // ****************
        // we have always STATUS_SUCCESS here
        // ****************

        ASSERT(NewFileInfo != OldRelatedFileInfo);
        // A new CCB will be allocated.
        // Assume that this structure named PtrNewCcb
        RC = UDFOpenFile(Vcb, PtrNewFileObject, PtrNewFcb);
        if (!NT_SUCCESS(RC)) try_return(RC);
        PtrNewCcb = (PtrUDFCCB)(PtrNewFileObject->FsContext2);

        if(RequestedDisposition == FILE_CREATE) {
            ReturnedInformation = FILE_EXISTS;
            AdPrint(("    Object name collision\n"));
            try_return(RC = STATUS_OBJECT_NAME_COLLISION);
        }

        NtReqFcb = PtrNewFcb->NTRequiredFCB;
        NtReqFcb->CommonFCBHeader.IsFastIoPossible = UDFIsFastIoPossible(PtrNewFcb);

        // Check if caller wanted a directory only and target object
        //  is not a directory, or caller wanted a file only and target
        //  object is not a file ...
        if((PtrNewFcb->FCBFlags & UDF_FCB_DIRECTORY) && ((RequestedDisposition == FILE_SUPERSEDE) ||
              (RequestedDisposition == FILE_OVERWRITE) || (RequestedDisposition == FILE_OVERWRITE_IF) ||
              FileOnlyRequested)) {
            if(FileOnlyRequested) {
                AdPrint(("    Can't open directory as a plain file\n"));
            } else {
                AdPrint(("    Can't supersede directory\n"));
            }
            RC = STATUS_FILE_IS_A_DIRECTORY;
            try_return(RC);
        }

        if(DirectoryOnlyRequested && !(PtrNewFcb->FCBFlags & UDF_FCB_DIRECTORY)) {
            AdPrint(("    This is not a directory\n"));
            RC = STATUS_NOT_A_DIRECTORY;
            try_return(RC);
        }

        if(DeleteOnCloseSpecified && (PtrNewFcb->FCBFlags & UDF_FCB_READ_ONLY)) {
            AdPrint(("    Can't delete Read-Only file\n"));
            RC = STATUS_CANNOT_DELETE;
            try_return(RC);
        }
        // Check share access and fail if the share conflicts with an existing
        // open.
        ASSERT(Res1 != NULL);
        ASSERT(Res2 != NULL);
        RC = UDFCheckAccessRights(PtrNewFileObject, AccessState, PtrNewFcb, PtrNewCcb, DesiredAccess, ShareAccess);
        if(!NT_SUCCESS(RC)) {
            AdPrint(("    Access/Share access check failed\n"));
            try_return(RC);
        }

        RestoreShareAccess = TRUE;

        if(FileOnlyRequested) {
            //  If the user wants 'write access' access to the file make sure there
            //  is not a process mapping this file as an image.  Any attempt to
            //  delete the file will be stopped in fileinfo.cpp
            //
            //  If the user wants to delete on close, we must check at this
            //  point though.
            if( (DesiredAccess & FILE_WRITE_DATA) || DeleteOnCloseSpecified ) {
                MmPrint(("    MmFlushImageSection();\n"));
                NtReqFcb->AcqFlushCount++;
                if(!MmFlushImageSection( &(NtReqFcb->SectionObject),
                                          MmFlushForWrite )) {

                    NtReqFcb->AcqFlushCount--;
                    RC = DeleteOnCloseSpecified ? STATUS_CANNOT_DELETE :
                                                  STATUS_SHARING_VIOLATION;
                    AdPrint(("    File is mapped or deletion in progress\n"));
                    try_return (RC);
                }
                NtReqFcb->AcqFlushCount--;
            }
            if(  NoBufferingSpecified &&
                /*  (PtrNewFileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) &&*/
               !(PtrNewFcb->CachedOpenHandleCount) &&
                (NtReqFcb->SectionObject.DataSectionObject) ) {
                //  If this is a non-cached open, and there are no open cached
                //  handles, but there is still a data section, attempt a flush
                //  and purge operation to avoid cache coherency overhead later.
                //  We ignore any I/O errors from the flush.
                MmPrint(("    CcFlushCache()\n"));
                CcFlushCache( &(NtReqFcb->SectionObject), NULL, 0, NULL );
                MmPrint(("    CcPurgeCacheSection()\n"));
                CcPurgeCacheSection( &(NtReqFcb->SectionObject), NULL, 0, FALSE );
            }
        }

        if(DeleteOnCloseSpecified && UDFIsADirectory(NewFileInfo) && !UDFIsDirEmpty__(NewFileInfo)) {
            AdPrint(("    Directory in not empry\n"));
            try_return (RC = STATUS_DIRECTORY_NOT_EMPTY);
        }

        // Get attributes for the file ...
        TmpFileAttributes =
            (USHORT)UDFAttributesToNT(UDFDirIndex(UDFGetDirIndexByFileInfo(NewFileInfo), NewFileInfo->Index),
                               NewFileInfo->Dloc->FileEntry);

        if(DeleteOnCloseSpecified &&
           (TmpFileAttributes & FILE_ATTRIBUTE_READONLY)) {
            ASSERT(Res1 != NULL);
            ASSERT(Res2 != NULL);
            RC = UDFCheckAccessRights(NULL, NULL, OldRelatedFileInfo->Fcb, PtrRelatedCCB, FILE_DELETE_CHILD, 0);
            if(!NT_SUCCESS(RC)) {
                AdPrint(("    Read-only. DeleteOnClose attempt failed\n"));
                try_return (RC = STATUS_CANNOT_DELETE);
            }
        }

        // If a supersede or overwrite was requested, do so now ...
        if((RequestedDisposition == FILE_SUPERSEDE) ||
           (RequestedDisposition == FILE_OVERWRITE) ||
           (RequestedDisposition == FILE_OVERWRITE_IF)) {
            // Attempt the operation here ...

#ifndef UDF_READ_ONLY_BUILD
            ASSERT(!UDFIsADirectory(NewFileInfo));

            if(RequestedDisposition == FILE_SUPERSEDE) {
                BOOLEAN RestoreRO = FALSE;

                ASSERT(Res1 != NULL);
                ASSERT(Res2 != NULL);
                // NT wants us to allow Supersede on RO files
                if(PtrNewFcb->FCBFlags & UDF_FCB_READ_ONLY) {
                    // Imagine, that file is not RO and check other permissions
                    RestoreRO = TRUE;
                    PtrNewFcb->FCBFlags &= ~UDF_FCB_READ_ONLY;
                }
                RC = UDFCheckAccessRights(NULL, NULL, PtrNewFcb, PtrNewCcb, DELETE, 0);
                if(RestoreRO) {
                    // Restore RO state if changed
                    PtrNewFcb->FCBFlags |= UDF_FCB_READ_ONLY;
                }
                if(!NT_SUCCESS(RC)) {
                    AdPrint(("    Can't supersede. DELETE permission required\n"));
                    try_return (RC);
                }
            } else {
                ASSERT(Res1 != NULL);
                ASSERT(Res2 != NULL);
                RC = UDFCheckAccessRights(NULL, NULL, PtrNewFcb, PtrNewCcb,
                            FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES, 0);
                if(!NT_SUCCESS(RC)) {
                    AdPrint(("    Can't overwrite. Permission denied\n"));
                    try_return (RC);
                }
            }
            // Existing & requested System and Hidden bits must match
            if( (TmpFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) &
                (FileAttributes ^ (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) ) {
                AdPrint(("    The Hidden and/or System bits do not match\n"));
                try_return(RC = STATUS_ACCESS_DENIED);
            }

            //  Before we actually truncate, check to see if the purge
            //  is going to fail.
            MmPrint(("    MmCanFileBeTruncated()\n"));
            if (!MmCanFileBeTruncated( &NtReqFcb->SectionObject,
                                       &(UDFGlobalData.UDFLargeZero) )) {
                AdPrint(("    Can't truncate. File is mapped\n"));
                try_return(RC = STATUS_USER_MAPPED_FILE);
            }

            ASSERT(Res1 != NULL);
            ASSERT(Res2 != NULL);

#if 0
            CollectStatistics(Vcb, MetaDataWrites);
#endif
            // Synchronize with PagingIo
            UDFAcquireResourceExclusive(PagingIoRes = &(NtReqFcb->PagingIoResource),TRUE);
            // Set file sizes
            if(!NT_SUCCESS(RC = UDFResizeFile__(Vcb, NewFileInfo, 0))) {
                AdPrint(("    Error during resize operation\n"));
                try_return(RC);
            }
/*            if(AllocationSize) {
                if(!NT_SUCCESS(RC = UDFResizeFile__(Vcb, NewFileInfo, AllocationSize))) {
                    AdPrint(("    Error during resize operation (2)\n"));
                    try_return(RC);
                }
            }*/
            NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart = UDFSysGetAllocSize(Vcb, AllocationSize);
            NtReqFcb->CommonFCBHeader.FileSize.QuadPart =
            NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart = 0 /*AllocationSize*/;
            PtrNewFcb->FCBFlags &= ~UDF_FCB_DELAY_CLOSE;
            MmPrint(("    CcSetFileSizes()\n"));
            CcSetFileSizes(PtrNewFileObject, (PCC_FILE_SIZES)&(NtReqFcb->CommonFCBHeader.AllocationSize));
            NtReqFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;
            // Release PagingIoResource
            UDFReleaseResource(PagingIoRes);
            PagingIoRes = NULL;

            if(NT_SUCCESS(RC)) {
                FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
                if (RequestedDisposition == FILE_SUPERSEDE) {
                    // Set attributes for the file ...
                    UDFAttributesToUDF(UDFDirIndex(UDFGetDirIndexByFileInfo(NewFileInfo), NewFileInfo->Index),
                                       NewFileInfo->Dloc->FileEntry, FileAttributes);
                    ReturnedInformation = FILE_SUPERSEDED;
                } else {
                    // Get attributes for the file ...
                    FileAttributes |= TmpFileAttributes;
                    // Set attributes for the file ...
                    UDFAttributesToUDF(UDFDirIndex(UDFGetDirIndexByFileInfo(NewFileInfo), NewFileInfo->Index),
                                       NewFileInfo->Dloc->FileEntry, FileAttributes);
                    ReturnedInformation = FILE_OVERWRITTEN;
                }
            }
            // notify changes
            UDFNotifyFullReportChange( Vcb, NewFileInfo,
                                       FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE,
                                       FILE_ACTION_MODIFIED);

            // Update parent object
            if((Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_WRITE) &&
               PtrRelatedFCB &&
               PtrRelatedFileObject &&
               (PtrRelatedFCB->FileInfo == NewFileInfo->ParentFile)) {
                PtrRelatedFileObject->Flags |= (FO_FILE_MODIFIED | FO_FILE_SIZE_CHANGED);
            }
#else //UDF_READ_ONLY_BUILD
            try_return(RC = STATUS_ACCESS_DENIED);
#endif //UDF_READ_ONLY_BUILD
        } else {
            ReturnedInformation = FILE_OPENED;
        }

        // Update parent object
        if((Vcb->CompatFlags & UDF_VCB_IC_UPDATE_DIR_READ) &&
           PtrRelatedFCB &&
           PtrRelatedFileObject &&
           (PtrRelatedFCB->FileInfo == NewFileInfo->ParentFile)) {
            PtrRelatedFileObject->Flags |= FO_FILE_FAST_IO_READ;
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        // Complete the request unless we are here as part of unwinding
        //  when an exception condition was encountered, OR
        //  if the request has been deferred (i.e. posted for later handling)

        if(RestoreVCBOpenCounter) {
            UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));
            RestoreVCBOpenCounter = FALSE;
        }

        if (RC != STATUS_PENDING) {
            // If any intermediate (directory) open operations were performed,
            //  implement the corresponding close (do *not* however close
            //  the target we have opened on behalf of the caller ...).

#if 0
            if(NT_SUCCESS(RC)) {
                CollectStatistics2(Vcb, SuccessfulCreates);
            } else {
                CollectStatistics2(Vcb, FailedCreates);
            }
#endif

            if (NT_SUCCESS(RC) && PtrNewFcb) {
                // Update the file object such that:
                //  (a) the FsContext field points to the NTRequiredFCB field
                //       in the FCB
                //  (b) the FsContext2 field points to the CCB created as a
                //       result of the open operation

                // If write-through was requested, then mark the file object
                //  appropriately

                // directories are not cached
                // so we should prevent flush attepmts on cleanup
                if(!(PtrNewFcb->FCBFlags & UDF_FCB_DIRECTORY)) {
#ifndef UDF_READ_ONLY_BUILD
                    if(WriteThroughRequested) {
                        PtrNewFileObject->Flags |= FO_WRITE_THROUGH;
                        PtrNewFcb->FCBFlags |= UDF_FCB_WRITE_THROUGH;
                        MmPrint(("        FO_WRITE_THROUGH\n"));
                    }
#endif //UDF_READ_ONLY_BUILD
                    if(SequentialIoRequested &&
                       !(Vcb->CompatFlags & UDF_VCB_IC_IGNORE_SEQUENTIAL_IO)) {
                        PtrNewFileObject->Flags |= FO_SEQUENTIAL_ONLY;
                        MmPrint(("        FO_SEQUENTIAL_ONLY\n"));
#ifndef UDF_READ_ONLY_BUILD
                        if(Vcb->TargetDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {
                            PtrNewFileObject->Flags &= ~FO_WRITE_THROUGH;
                            PtrNewFcb->FCBFlags &= ~UDF_FCB_WRITE_THROUGH;
                            MmPrint(("        FILE_REMOVABLE_MEDIA + FO_SEQUENTIAL_ONLY => ~FO_WRITE_THROUGH\n"));
                        }
#endif //UDF_READ_ONLY_BUILD
                        if(PtrNewFcb->FileInfo) {
                            UDFSetFileAllocMode__(PtrNewFcb->FileInfo, EXTENT_FLAG_ALLOC_SEQUENTIAL);
                        }
                    }
                    if(NoBufferingSpecified) {
                        PtrNewFileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
                        MmPrint(("        FO_NO_INTERMEDIATE_BUFFERING\n"));
                    } else {
                        PtrNewFileObject->Flags |= FO_CACHE_SUPPORTED;
                        MmPrint(("        FO_CACHE_SUPPORTED\n"));
                    }
                }

                if((DesiredAccess & FILE_EXECUTE) /*&&
                   !(PtrNewFcb->FCBFlags & UDF_FCB_DIRECTORY)*/) {
                    MmPrint(("        FO_FILE_FAST_IO_READ\n"));
                    PtrNewFileObject->Flags |= FO_FILE_FAST_IO_READ;
                }
                // All right. Now we can safely increment OpenHandleCount
                UDFInterlockedIncrement((PLONG)&(Vcb->VCBHandleCount));
                UDFInterlockedIncrement((PLONG)&(PtrNewFcb->OpenHandleCount));

                if(PtrNewFileObject->Flags & FO_CACHE_SUPPORTED)
                    UDFInterlockedIncrement((PLONG)&(PtrNewFcb->CachedOpenHandleCount));
                // Store some flags in CCB
                if(PtrNewCcb) {
                    PtrNewCcb->TreeLength = TreeLength;
                    // delete on close
#ifndef UDF_READ_ONLY_BUILD
                    if(DeleteOnCloseSpecified) {
                        ASSERT(!(PtrNewFcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
                        PtrNewCcb->CCBFlags |= UDF_CCB_DELETE_ON_CLOSE;
                    }
#endif //UDF_READ_ONLY_BUILD
                    // case sensetivity
                    if(!IgnoreCase) {
                        // remember this for possible Rename/Move operation
                        PtrNewCcb->CCBFlags |= UDF_CCB_CASE_SENSETIVE;
                        PtrNewFileObject->Flags |= FO_OPENED_CASE_SENSITIVE;
                    }
                    if(IsFileObjectReadOnly(PtrNewFileObject)) {
                        UDFInterlockedIncrement((PLONG)&(Vcb->VCBOpenCountRO));
                        PtrNewCcb->CCBFlags |= UDF_CCB_READ_ONLY;
                    }
                } else {
                    BrutePoint();
                }
                // it was a stream...
                if(StreamOpen)
                    PtrNewFileObject->Flags |= FO_STREAM_FILE;
//                PtrNewCcb->CCBFlags |= UDF_CCB_VALID;
                // increment the number of outstanding open operations on this
                // logical volume (i.e. volume cannot be dismounted)
                UDFInterlockedIncrement((PLONG)&(Vcb->VCBOpenCount));
                PtrNewFcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
                PtrNewFcb->FCBFlags |= UDF_FCB_VALID;
#ifdef UDF_DBG
                // We have no FileInfo for Volume
                if(PtrNewFcb->FileInfo) {
                    ASSERT_REF(PtrNewFcb->ReferenceCount >= PtrNewFcb->FileInfo->RefCount);
                }
#endif // UDF_DBG
                AdPrint(("    FCB %x, CCB %x, FO %x, Flags %x\n", PtrNewFcb, PtrNewCcb, PtrNewFileObject, PtrNewFcb->FCBFlags));

                UDFReleaseResFromCreate(&PagingIoRes, &Res1, &Res2);

            } else if(!NT_SUCCESS(RC)) {
                // Perform failure related post-processing now
                if(RestoreShareAccess && NtReqFcb && PtrNewFileObject) {
                    IoRemoveShareAccess(PtrNewFileObject, &(NtReqFcb->FCBShareAccess));
                }
                UDFCleanUpCCB(PtrNewCcb);
                if(PtrNewFileObject) {
                    PtrNewFileObject->FsContext2 = NULL;
                }
                // We have successfully opened LastGoodFileInfo,
                // so mark it as VALID to avoid future troubles...
                if(LastGoodFileInfo && LastGoodFileInfo->Fcb) {
                    LastGoodFileInfo->Fcb->FCBFlags |= UDF_FCB_VALID;
                    if(LastGoodFileInfo->Fcb->NTRequiredFCB) {
                        LastGoodFileInfo->Fcb->NTRequiredFCB->NtReqFCBFlags |= UDF_NTREQ_FCB_VALID;
                    }
                }
                // Release resources...
                UDFReleaseResFromCreate(&PagingIoRes, &Res1, &Res2);
                ASSERT(AcquiredVcb);
                // close the chain
                UDFCloseFileInfoChain(Vcb, LastGoodFileInfo, TreeLength, TRUE);
                // cleanup FCBs (if any)
                if(  Vcb && (PtrNewFcb != Vcb->RootDirFCB) &&
                     LastGoodFileInfo ) {
                    UDFCleanUpFcbChain(Vcb, LastGoodFileInfo, TreeLength, TRUE);
                } else {
                    ASSERT(!LastGoodFileInfo);
                }
            } else {
                UDFReleaseResFromCreate(&PagingIoRes, &Res1, &Res2);
            }
            // As long as this unwinding is not being performed as a result of
            //  an exception condition, complete the IRP ...
            if (!_SEH2_AbnormalTermination()) {
                Irp->IoStatus.Status = RC;
                Irp->IoStatus.Information = ReturnedInformation;

                // complete the IRP
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                // Free up the Irp Context
                UDFReleaseIrpContext(PtrIrpContext);
            }
        } else {
            UDFReleaseResFromCreate(&PagingIoRes, &Res1, &Res2);
        }

        if(AcquiredVcb) {
            UDFReleaseResource(&(Vcb->VCBResource));
        }
        // free allocated tmp buffers (if any)
        if(AbsolutePathName.Buffer)
            MyFreePool__(AbsolutePathName.Buffer);
        if(LocalPath.Buffer)
            MyFreePool__(LocalPath.Buffer);
        if(TailNameBuffer)
            MyFreePool__(TailNameBuffer);
    } _SEH2_END;

    return(RC);
} // end UDFCommonCreate()

/*************************************************************************
*
* Function: UDFFirstOpenFile()
*
* Description:
*   Perform first Open/Create initialization.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFFirstOpenFile(
    IN PVCB                    Vcb,                // volume control block
    IN PFILE_OBJECT            PtrNewFileObject,   // I/O Mgr. created file object
   OUT PtrUDFFCB*              PtrNewFcb,
    IN PUDF_FILE_INFO          RelatedFileInfo,
    IN PUDF_FILE_INFO          NewFileInfo,
    IN PUNICODE_STRING         LocalPath,
    IN PUNICODE_STRING         CurName
    )
{
//    DIR_INDEX           NewFileIndex;
    PtrUDFObjectName    NewFCBName;
    PtrUDFNTRequiredFCB NtReqFcb;
    NTSTATUS            RC;
    BOOLEAN             Linked = TRUE;
    PDIR_INDEX_HDR      hDirIndex;
    PDIR_INDEX_ITEM     DirIndex;

    AdPrint(("UDFFirstOpenFile\n"));

    if(!((*PtrNewFcb) = UDFAllocateFCB())) {
        AdPrint(("Can't allocate FCB\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate and set new FCB unique name (equal to absolute path name)
    if(!(NewFCBName = UDFAllocateObjectName())) return STATUS_INSUFFICIENT_RESOURCES;

    if(RelatedFileInfo && RelatedFileInfo->Fcb &&
       !(RelatedFileInfo->Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY)) {
        RC = MyCloneUnicodeString(&(NewFCBName->ObjectName), &(RelatedFileInfo->Fcb->FCBName->ObjectName));
    } else {
        RC = MyInitUnicodeString(&(NewFCBName->ObjectName), L"");
    }
    if(!NT_SUCCESS(RC))
        return STATUS_INSUFFICIENT_RESOURCES;
    if( (CurName->Buffer[0] != L':') &&
        (!LocalPath->Length ||
            ((LocalPath->Buffer[LocalPath->Length/sizeof(WCHAR)-1] != L':') /*&&
             (LocalPath->Buffer[LocalPath->Length/sizeof(WCHAR)-1] != L'\\')*/) )) {
        RC = MyAppendUnicodeToString(&(NewFCBName->ObjectName), L"\\");
        if(!NT_SUCCESS(RC)) {
            UDFReleaseObjectName(NewFCBName);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // Make link between Fcb and FileInfo
    (*PtrNewFcb)->FileInfo = NewFileInfo;
    NewFileInfo->Fcb = (*PtrNewFcb);
    (*PtrNewFcb)->ParentFcb = RelatedFileInfo->Fcb;

    if(!((*PtrNewFcb)->NTRequiredFCB = NewFileInfo->Dloc->CommonFcb)) {
        (*PtrNewFcb)->NTRequiredFCB = (PtrUDFNTRequiredFCB)MyAllocatePool__(NonPagedPool, UDFQuadAlign(sizeof(UDFNTRequiredFCB)));
        if(!((*PtrNewFcb)->NTRequiredFCB)) {
            UDFReleaseObjectName(NewFCBName);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        UDFPrint(("UDFAllocateNtReqFCB: %x\n", (*PtrNewFcb)->NTRequiredFCB));
        RtlZeroMemory((*PtrNewFcb)->NTRequiredFCB, UDFQuadAlign(sizeof(UDFNTRequiredFCB)));
        (*PtrNewFcb)->FileInfo->Dloc->CommonFcb = (*PtrNewFcb)->NTRequiredFCB;
        Linked = FALSE;
    } else {
        if(!(NewFileInfo->Dloc->CommonFcb->NtReqFCBFlags & UDF_NTREQ_FCB_VALID)) {
            (*PtrNewFcb)->NTRequiredFCB = NULL;
            BrutePoint();
            UDFReleaseObjectName(NewFCBName);
            return STATUS_ACCESS_DENIED;
        }
    }

    NtReqFcb = (*PtrNewFcb)->NTRequiredFCB;
    // Set times
    if(!Linked) {
        UDFGetFileXTime((*PtrNewFcb)->FileInfo,
            &(NtReqFcb->CreationTime.QuadPart),
            &(NtReqFcb->LastAccessTime.QuadPart),
            &(NtReqFcb->ChangeTime.QuadPart),
            &(NtReqFcb->LastWriteTime.QuadPart) );

        // Set the allocation size for the object is specified
        NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart =
            UDFSysGetAllocSize(Vcb, NewFileInfo->Dloc->DataLoc.Length);
//        NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart = UDFGetFileAllocationSize(Vcb, NewFileInfo);
        NtReqFcb->CommonFCBHeader.FileSize.QuadPart =
        NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart = NewFileInfo->Dloc->DataLoc.Length;
    }
    // begin transaction
    UDFAcquireResourceExclusive(&(Vcb->FcbListResource), TRUE);

    RC = UDFInitializeFCB(*PtrNewFcb, Vcb, NewFCBName,
                 UDFIsADirectory(NewFileInfo) ? UDF_FCB_DIRECTORY : 0, PtrNewFileObject);
    if(!NT_SUCCESS(RC)) {
        if(!Linked) {
            MyFreePool__((*PtrNewFcb)->NTRequiredFCB);
            (*PtrNewFcb)->NTRequiredFCB = NULL;
        }
        UDFReleaseResource(&(Vcb->FcbListResource));
        return RC;
    }
    // set Read-only attribute
    if(!UDFIsAStreamDir(NewFileInfo)) {
        hDirIndex = UDFGetDirIndexByFileInfo(NewFileInfo);
#ifdef UDF_DBG
        if(!hDirIndex) {
            BrutePoint();
        } else {
#endif // UDF_DBG
            if(UDFAttributesToNT(DirIndex = UDFDirIndex(hDirIndex, NewFileInfo->Index),NULL) & FILE_ATTRIBUTE_READONLY) {
                (*PtrNewFcb)->FCBFlags |= UDF_FCB_READ_ONLY;
            }
            MyAppendUnicodeStringToStringTag(&(NewFCBName->ObjectName), &(DirIndex->FName), MEM_USOBJ_TAG);
#ifdef UDF_DBG
        }
#endif // UDF_DBG
    } else if (RelatedFileInfo->ParentFile) {
        hDirIndex = UDFGetDirIndexByFileInfo(RelatedFileInfo);
        if(UDFAttributesToNT(DirIndex = UDFDirIndex(hDirIndex, RelatedFileInfo->Index),NULL) & FILE_ATTRIBUTE_READONLY) {
            (*PtrNewFcb)->FCBFlags |= UDF_FCB_READ_ONLY;
        }
        RC = MyAppendUnicodeStringToStringTag(&(NewFCBName->ObjectName), CurName, MEM_USOBJ_TAG);
//    } else {
//        BrutePoint();
    }
    // do not allocate CCB if it is internal Create/Open
    if(NT_SUCCESS(RC)) {
        if(PtrNewFileObject) {
            RC = UDFOpenFile(Vcb, PtrNewFileObject, *PtrNewFcb);
        } else {
            RC = STATUS_SUCCESS;
        }
    }
    UDFReleaseResource(&(Vcb->FcbListResource));
    // end transaction

//    if(!NT_SUCCESS(RC)) return RC;

    return RC;
} // end UDFFirstOpenFile()

/*************************************************************************
*
* Function: UDFOpenFile()
*
* Description:
*   Open a file/dir for the caller.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFOpenFile(
    PVCB                 Vcb,                // volume control block
    PFILE_OBJECT         PtrNewFileObject,   // I/O Mgr. created file object
    PtrUDFFCB            PtrNewFcb
    )
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PtrUDFCCB               Ccb = NULL;
    PtrUDFNTRequiredFCB     NtReqFcb;

    AdPrint(("UDFOpenFile\n"));
    ASSERT((PtrNewFcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_FCB)
         ||(PtrNewFcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB));

    _SEH2_TRY {

#if 0
        CollectStatistics2(Vcb, CreateHits);
#endif
        // create a new CCB structure
        if (!(Ccb = UDFAllocateCCB())) {
            AdPrint(("Can't allocate CCB\n"));
            PtrNewFileObject->FsContext2 = NULL;
            //
            UDFInterlockedIncrement((PLONG)&(PtrNewFcb->ReferenceCount));
            UDFInterlockedIncrement((PLONG)&(PtrNewFcb->NTRequiredFCB->CommonRefCount));
            RC = STATUS_INSUFFICIENT_RESOURCES;
            try_return(RC);
        }
        // initialize the CCB
        Ccb->Fcb = PtrNewFcb;
        // initialize the CCB to point to the file object
        Ccb->FileObject = PtrNewFileObject;

        // initialize the file object appropriately
        PtrNewFileObject->FsContext2 = (PVOID)(Ccb);
        PtrNewFileObject->Vpb = Vcb->Vpb;
        PtrNewFileObject->FsContext = (PVOID)(NtReqFcb = PtrNewFcb->NTRequiredFCB);
        PtrNewFileObject->SectionObjectPointer = &(NtReqFcb->SectionObject);
#ifdef DBG
//        NtReqFcb ->FileObject = PtrNewFileObject;
#endif //DBG

#ifdef UDF_DELAYED_CLOSE
        PtrNewFcb->FCBFlags &= ~UDF_FCB_DELAY_CLOSE;
#endif //UDF_DELAYED_CLOSE

        UDFAcquireResourceExclusive(&(PtrNewFcb->CcbListResource),TRUE);
        // insert CCB into linked list of open file object to Fcb or
        // to Vcb and do other intialization
        InsertTailList(&(PtrNewFcb->NextCCB), &(Ccb->NextCCB));
        UDFInterlockedIncrement((PLONG)&(PtrNewFcb->ReferenceCount));
        UDFInterlockedIncrement((PLONG)&(PtrNewFcb->NTRequiredFCB->CommonRefCount));
        UDFReleaseResource(&(PtrNewFcb->CcbListResource));

try_exit:   NOTHING;
    } _SEH2_FINALLY {
        NOTHING;
    } _SEH2_END;

    return(RC);
} // end UDFOpenFile()


/*************************************************************************
*
* Function: UDFInitializeFCB()
*
* Description:
*   Initialize a new FCB structure and also the sent-in file object
*   (if supplied)
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
NTSTATUS
UDFInitializeFCB(
    IN PtrUDFFCB        PtrNewFcb,      // FCB structure to be initialized
    IN PVCB             Vcb,            // logical volume (VCB) pointer
    IN PtrUDFObjectName PtrObjectName,  // name of the object
    IN ULONG            Flags,          // is this a file/directory, etc.
    IN PFILE_OBJECT     FileObject)     // optional file object to be initialized
{
    AdPrint(("UDFInitializeFCB\n"));
    NTSTATUS status;
    BOOLEAN Linked = TRUE;

    if(!PtrNewFcb->NTRequiredFCB->CommonFCBHeader.Resource) {
        // record signature
        PtrNewFcb->NTRequiredFCB->CommonFCBHeader.NodeTypeCode = UDF_NODE_TYPE_NT_REQ_FCB;
        PtrNewFcb->NTRequiredFCB->CommonFCBHeader.NodeByteSize = sizeof(UDFNTRequiredFCB);
        // Initialize the ERESOURCE objects
        if(!NT_SUCCESS(status = UDFInitializeResourceLite(&(PtrNewFcb->NTRequiredFCB->MainResource)))) {
            AdPrint(("    Can't init resource\n"));
            return status;
        }
        if(!NT_SUCCESS(status = UDFInitializeResourceLite(&(PtrNewFcb->NTRequiredFCB->PagingIoResource)))) {
            AdPrint(("    Can't init resource (2)\n"));
            UDFDeleteResource(&(PtrNewFcb->NTRequiredFCB->MainResource));
            return status;
        }
        // Fill NT required Fcb part
        PtrNewFcb->NTRequiredFCB->CommonFCBHeader.Resource = &(PtrNewFcb->NTRequiredFCB->MainResource);
        PtrNewFcb->NTRequiredFCB->CommonFCBHeader.PagingIoResource = &(PtrNewFcb->NTRequiredFCB->PagingIoResource);
        // Itialize byte-range locks support structure
        FsRtlInitializeFileLock(&(PtrNewFcb->NTRequiredFCB->FileLock),NULL,NULL);
        // Init reference counter
        PtrNewFcb->NTRequiredFCB->CommonRefCount = 0;
        Linked = FALSE;
    } else {
        ASSERT(PtrNewFcb->NTRequiredFCB->CommonFCBHeader.NodeTypeCode == UDF_NODE_TYPE_NT_REQ_FCB);
    }
    if(!NT_SUCCESS(status = UDFInitializeResourceLite(&(PtrNewFcb->CcbListResource)))) {
        AdPrint(("    Can't init resource (3)\n"));
        BrutePoint();
        if(!Linked) {
            UDFDeleteResource(&(PtrNewFcb->NTRequiredFCB->PagingIoResource));
            UDFDeleteResource(&(PtrNewFcb->NTRequiredFCB->MainResource));
            PtrNewFcb->NTRequiredFCB->CommonFCBHeader.Resource =
            PtrNewFcb->NTRequiredFCB->CommonFCBHeader.PagingIoResource = NULL;
            FsRtlUninitializeFileLock(&(PtrNewFcb->NTRequiredFCB->FileLock));
        }
        return status;
    }

    // caller MUST ensure that VCB has been acquired exclusively
    InsertTailList(&(Vcb->NextFCB), &(PtrNewFcb->NextFCB));

    // initialize the various list heads
    InitializeListHead(&(PtrNewFcb->NextCCB));

    PtrNewFcb->ReferenceCount = 0;
    PtrNewFcb->OpenHandleCount = 0;

    PtrNewFcb->FCBFlags = Flags | UDF_FCB_INITIALIZED_CCB_LIST_RESOURCE;

    PtrNewFcb->FCBName = PtrObjectName;

    PtrNewFcb->Vcb = Vcb;

    return STATUS_SUCCESS;
} // end UDFInitializeFCB()

