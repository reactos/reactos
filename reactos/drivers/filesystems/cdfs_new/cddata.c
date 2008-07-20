
/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    CdData.c

Abstract:

    This module declares the global data used by the Cdfs file system.

    This module also handles the dispath routines in the Fsd threads as well as
    handling the IrpContext and Irp through the exception path.


--*/

#include "CdProcs.h"

#ifdef CD_SANITY
BOOLEAN CdTestTopLevel = TRUE;
BOOLEAN CdTestRaisedStatus = TRUE;
BOOLEAN CdBreakOnAnyRaise = FALSE;
BOOLEAN CdTraceRaises = FALSE;
NTSTATUS CdInterestingExceptionCodes[] = { STATUS_DISK_CORRUPT_ERROR, 
                                           STATUS_FILE_CORRUPT_ERROR,
                                           0, 0, 0, 0, 0, 0, 0, 0 };
#endif

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_CDDATA)

//
//  Global data structures
//

CD_DATA CdData;
FAST_IO_DISPATCH CdFastIoDispatch;

//
//  Reserved directory strings.
//

WCHAR CdUnicodeSelfArray[] = { L'.' };
WCHAR CdUnicodeParentArray[] = { L'.', L'.' };

UNICODE_STRING CdUnicodeDirectoryNames[] = {
    { 2, 2, CdUnicodeSelfArray},
    { 4, 4, CdUnicodeParentArray}
};

//
//  Volume descriptor identifier strings.
//

CHAR CdHsgId[] = { 'C', 'D', 'R', 'O', 'M' };
CHAR CdIsoId[] = { 'C', 'D', '0', '0', '1' };
CHAR CdXaId[] = { 'C', 'D', '-', 'X', 'A', '0', '0', '1' };

//
//  Volume label for audio disks.
//

WCHAR CdAudioLabel[] = { L'A', L'u', L'd', L'i', L'o', L' ', L'C', L'D' };
USHORT CdAudioLabelLength = sizeof( CdAudioLabel );

//
//  Pseudo file names for audio disks.
//

CHAR CdAudioFileName[] = { 'T', 'r', 'a', 'c', 'k', '0', '0', '.', 'c', 'd', 'a' };
UCHAR CdAudioFileNameLength = sizeof( CdAudioFileName );
ULONG CdAudioDirentSize = FIELD_OFFSET( RAW_DIRENT, FileId ) + sizeof( CdAudioFileName ) + sizeof( SYSTEM_USE_XA );
ULONG CdAudioDirentsPerSector = SECTOR_SIZE / (FIELD_OFFSET( RAW_DIRENT, FileId ) + sizeof( CdAudioFileName ) + sizeof( SYSTEM_USE_XA ));
ULONG CdAudioSystemUseOffset = FIELD_OFFSET( RAW_DIRENT, FileId ) + sizeof( CdAudioFileName );

//
//  Escape sequences for mounting Unicode volumes.
//

PCHAR CdJolietEscape[] = { "%/@", "%/C", "%/E" };

//
//  Audio Play Files consist completely of this header block.  These
//  files are readable in the root of any audio disc regardless of
//  the capabilities of the drive.
//
//  The "Unique Disk ID Number" is a calculated value consisting of
//  a combination of parameters, including the number of tracks and
//  the starting locations of those tracks.
//
//  Applications interpreting CDDA RIFF files should be advised that
//  additional RIFF file chunks may be added to this header in the
//  future in order to add information, such as the disk and song title.
//

LONG CdAudioPlayHeader[] = {
    0x46464952,                         // Chunk ID = 'RIFF'
    4 * 11 - 8,                         // Chunk Size = (file size - 8)
    0x41444443,                         // 'CDDA'
    0x20746d66,                         // 'fmt '
    24,                                 // Chunk Size (of 'fmt ' subchunk) = 24
    0x00000001,                         // WORD Format Tag, WORD Track Number
    0x00000000,                         // DWORD Unique Disk ID Number
    0x00000000,                         // DWORD Track Starting Sector (LBN)
    0x00000000,                         // DWORD Track Length (LBN count)
    0x00000000,                         // DWORD Track Starting Sector (MSF)
    0x00000000                          // DWORD Track Length (MSF)
};

//  Audio Philes begin with this header block to identify the data as a
//  PCM waveform.  AudioPhileHeader is coded as if it has no data included
//  in the waveform.  Data must be added in 2352-byte multiples.
//
//  Fields marked 'ADJUST' need to be adjusted based on the size of the
//  data: Add (nSectors*2352) to the DWORDs at offsets 1*4 and 10*4.
//
//  File Size of TRACK??.WAV = nSectors*2352 + sizeof(AudioPhileHeader)
//  RIFF('WAVE' fmt(1, 2, 44100, 176400, 16, 4) data( <CD Audio Raw Data> )
//
//  The number of sectors in a CD-XA CD-DA file is (DataLen/2048).
//  CDFS will expose these files to applications as if they were just
//  'WAVE' files, adjusting the file size so that the RIFF file is valid.
//
//  NT NOTE: We do not do any fidelity adjustment. These are presented as raw
//  2352 byte sectors - 95 has the glimmer of an idea to allow CDFS to expose
//  the CDXA CDDA data at different sampling rates in a virtual directory
//  structure, but we will never do that.
//

LONG CdXAAudioPhileHeader[] = {
    0x46464952,                         // Chunk ID = 'RIFF'
    -8,                                 // Chunk Size = (file size - 8) ADJUST1
    0x45564157,                         // 'WAVE'
    0x20746d66,                         // 'fmt '
    16,                                 // Chunk Size (of 'fmt ' subchunk) = 16
    0x00020001,                         // WORD Format Tag WORD nChannels
    44100,                              // DWORD nSamplesPerSecond
    2352 * 75,                          // DWORD nAvgBytesPerSec
    0x00100004,                         // WORD nBlockAlign WORD nBitsPerSample
    0x61746164,                         // 'data'
    -44                                 // <CD Audio Raw Data>          ADJUST2
};

//
//  XA Files begin with this RIFF header block to identify the data as
//  raw CD-XA sectors.  Data must be added in 2352-byte multiples.
//
//  This header is added to all CD-XA files which are marked as having
//  mode2form2 sectors.
//
//  Fields marked 'ADJUST' need to be adjusted based on the size of the
//  data: Add file size to the marked DWORDS.
//
//  File Size of TRACK??.WAV = nSectors*2352 + sizeof(XAFileHeader)
//
//  RIFF('CDXA' FMT(Owner, Attr, 'X', 'A', FileNum, 0) data ( <CDXA Raw Data> )
//

LONG CdXAFileHeader[] = {
    0x46464952,                         // Chunk ID = 'RIFF'
    -8,                                 // Chunk Size = (file size - 8) ADJUST
    0x41584443,                         // 'CDXA'
    0x20746d66,                         // 'fmt '
    16,                                 // Chunk Size (of CDXA chunk) = 16
    0,                                  // DWORD Owner ID
    0x41580000,                         // WORD Attributes
                                        // BYTE Signature byte 1 'X'
                                        // BYTE Signature byte 2 'A'
    0,                                  // BYTE File Number
    0,                                  // BYTE Reserved[7]
    0x61746164,                         // 'data'
    -44                                 // <CD-XA Raw Sectors>          ADJUST
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdFastIoCheckIfPossible)
#pragma alloc_text(PAGE, CdSerial32)
#endif


NTSTATUS
CdFsdDispatch (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry to all of the Fsd dispatch points.

    Conceptually the Io routine will call this routine on all requests
    to the file system.  We case on the type of request and invoke the
    correct handler for this type of request.  There is an exception filter
    to catch any exceptions in the CDFS code as well as the CDFS process
    exception routine.

    This routine allocates and initializes the IrpContext for this request as
    well as updating the top-level thread context as necessary.  We may loop
    in this routine if we need to retry the request for any reason.  The
    status code STATUS_CANT_WAIT is used to indicate this.  Suppose the disk
    in the drive has changed.  An Fsd request will proceed normally until it
    recognizes this condition.  STATUS_VERIFY_REQUIRED is raised at that point
    and the exception code will handle the verify and either return
    STATUS_CANT_WAIT or STATUS_PENDING depending on whether the request was
    posted.

Arguments:

    VolumeDeviceObject - Supplies the volume device object for this request

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    THREAD_CONTEXT ThreadContext;
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN Wait;

#ifdef CD_SANITY
    PVOID PreviousTopLevel;
#endif

    NTSTATUS Status;

#if DBG

    KIRQL SaveIrql = KeGetCurrentIrql();

#endif

    ASSERT_OPTIONAL_IRP( Irp );

    FsRtlEnterFileSystem();

#ifdef CD_SANITY
    PreviousTopLevel = IoGetTopLevelIrp();
#endif

    //
    //  Loop until this request has been completed or posted.
    //

    do {

        //
        //  Use a try-except to handle the exception cases.
        //

        try {

            //
            //  If the IrpContext is NULL then this is the first pass through
            //  this loop.
            //

            if (IrpContext == NULL) {

                //
                //  Decide if this request is waitable an allocate the IrpContext.
                //  If the file object in the stack location is NULL then this
                //  is a mount which is always waitable.  Otherwise we look at
                //  the file object flags.
                //

                if (IoGetCurrentIrpStackLocation( Irp )->FileObject == NULL) {

                    Wait = TRUE;

                } else {

                    Wait = CanFsdWait( Irp );
                }

                IrpContext = CdCreateIrpContext( Irp, Wait );

                //
                //  Update the thread context information.
                //

                CdSetThreadContext( IrpContext, &ThreadContext );

#ifdef CD_SANITY
                ASSERT( !CdTestTopLevel ||
                        SafeNodeType( IrpContext->TopLevel ) == CDFS_NTC_IRP_CONTEXT );
#endif

            //
            //  Otherwise cleanup the IrpContext for the retry.
            //

            } else {

                //
                //  Set the MORE_PROCESSING flag to make sure the IrpContext
                //  isn't inadvertently deleted here.  Then cleanup the
                //  IrpContext to perform the retry.
                //

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING );
                CdCleanupIrpContext( IrpContext, FALSE );
            }

            //
            //  Case on the major irp code.
            //

            switch (IrpContext->MajorFunction) {

            case IRP_MJ_CREATE :

                Status = CdCommonCreate( IrpContext, Irp );
                break;

            case IRP_MJ_CLOSE :

                Status = CdCommonClose( IrpContext, Irp );
                break;

            case IRP_MJ_READ :

                //
                //  If this is an Mdl complete request, don't go through
                //  common read.
                //

                if (FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE )) {

                    Status = CdCompleteMdl( IrpContext, Irp );

                } else {

                    Status = CdCommonRead( IrpContext, Irp );
                }

                break;

            case IRP_MJ_QUERY_INFORMATION :

                Status = CdCommonQueryInfo( IrpContext, Irp );
                break;

            case IRP_MJ_SET_INFORMATION :

                Status = CdCommonSetInfo( IrpContext, Irp );
                break;

            case IRP_MJ_QUERY_VOLUME_INFORMATION :

                Status = CdCommonQueryVolInfo( IrpContext, Irp );
                break;

            case IRP_MJ_DIRECTORY_CONTROL :

                Status = CdCommonDirControl( IrpContext, Irp );
                break;

            case IRP_MJ_FILE_SYSTEM_CONTROL :

                Status = CdCommonFsControl( IrpContext, Irp );
                break;

            case IRP_MJ_DEVICE_CONTROL :

                Status = CdCommonDevControl( IrpContext, Irp );
                break;

            case IRP_MJ_LOCK_CONTROL :

                Status = CdCommonLockControl( IrpContext, Irp );
                break;

            case IRP_MJ_CLEANUP :

                Status = CdCommonCleanup( IrpContext, Irp );
                break;

            case IRP_MJ_PNP :

                Status = CdCommonPnp( IrpContext, Irp );
                break;

            default :

                Status = STATUS_INVALID_DEVICE_REQUEST;
                CdCompleteRequest( IrpContext, Irp, Status );
            }

        } except( CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

            Status = CdProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT);

#ifdef CD_SANITY
    ASSERT( !CdTestTopLevel ||
            (PreviousTopLevel == IoGetTopLevelIrp()) );
#endif

    FsRtlExitFileSystem();

    ASSERT( SaveIrql == KeGetCurrentIrql( ));

    return Status;
}


#ifdef CD_SANITY

VOID
CdRaiseStatusEx(
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status,
    IN BOOLEAN NormalizeStatus,
    IN OPTIONAL ULONG FileId,
    IN OPTIONAL ULONG Line
    )
{
    BOOLEAN BreakIn = FALSE;
    
    AssertVerifyDevice( IrpContext, Status);

    if (CdTraceRaises)  {

        DbgPrint( "%p CdRaiseStatusEx 0x%x @ fid %d, line %d\n", PsGetCurrentThread(), Status, FileId, Line);
    }

    if (CdTestRaisedStatus && !CdBreakOnAnyRaise)  {

        ULONG Index;

        for (Index = 0; 
             Index < (sizeof( CdInterestingExceptionCodes) / sizeof( CdInterestingExceptionCodes[0])); 
             Index++)  {

            if ((STATUS_SUCCESS != CdInterestingExceptionCodes[Index]) &&
                (CdInterestingExceptionCodes[Index] == Status))  {

                BreakIn = TRUE;
                break;
            }
        }
    }

    if (BreakIn || CdBreakOnAnyRaise)  {
        
        DbgPrint( "CDFS: Breaking on raised status %08x  (BI=%d,BA=%d)\n", Status, BreakIn, CdBreakOnAnyRaise);
        DbgPrint( "CDFS: (FILEID %d LINE %d)\n", FileId, Line);
        DbgPrint( "CDFS: Contact CDFS.SYS component owner for triage.\n");
        DbgPrint( "CDFS: 'eb %p 0;eb %p 0' to disable this alert.\n", &CdTestRaisedStatus, &CdBreakOnAnyRaise);

        DbgBreakPoint();
    }
    
    if (NormalizeStatus)  {

        IrpContext->ExceptionStatus = FsRtlNormalizeNtstatus( Status, STATUS_UNEXPECTED_IO_ERROR);
    }
    else {

        IrpContext->ExceptionStatus = Status;
    }

    IrpContext->RaisedAtLineFile = (FileId << 16) | Line;
    
    ExRaiseStatus( IrpContext->ExceptionStatus);
}

#endif


LONG
CdExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide whether we will handle a raised exception
    status.  If CDFS explicitly raised an error then this status is already
    in the IrpContext.  We choose which is the correct status code and
    either indicate that we will handle the exception or bug-check the system.

Arguments:

    ExceptionCode - Supplies the exception code to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode;
    BOOLEAN TestStatus = TRUE;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );

    ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

    //
    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.
    //

    if ((ExceptionCode == STATUS_IN_PAGE_ERROR) &&
        (ExceptionPointer->ExceptionRecord->NumberParameters >= 3)) {

        ExceptionCode =
            (NTSTATUS)ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
    }

    //
    //  If there is an Irp context then check which status code to use.
    //

    if (ARGUMENT_PRESENT( IrpContext )) {

        if (IrpContext->ExceptionStatus == STATUS_SUCCESS) {

            //
            //  Store the real status into the IrpContext.
            //

            IrpContext->ExceptionStatus = ExceptionCode;

        } else {

            //
            //  No need to test the status code if we raised it ourselves.
            //

            TestStatus = FALSE;
        }
    }

    AssertVerifyDevice( IrpContext, IrpContext->ExceptionStatus );
    
    //
    //  Bug check if this status is not supported.
    //

    if (TestStatus && !FsRtlIsNtstatusExpected( ExceptionCode )) {

        CdBugCheck( (ULONG_PTR) ExceptionPointer->ExceptionRecord,
                    (ULONG_PTR) ExceptionPointer->ContextRecord,
                    (ULONG_PTR) ExceptionPointer->ExceptionRecord->ExceptionAddress );

    }

    return EXCEPTION_EXECUTE_HANDLER;
}


NTSTATUS
CdProcessException (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine processes an exception.  It either completes the request
    with the exception status in the IrpContext, sends this off to the Fsp
    workque or causes it to be retried in the current thread if a verification
    is needed.

    If the volume needs to be verified (STATUS_VERIFY_REQUIRED) and we can
    do the work in the current thread we will translate the status code
    to STATUS_CANT_WAIT to indicate that we need to retry the request.

Arguments:

    Irp - Supplies the Irp being processed

    ExceptionCode - Supplies the normalized exception status being handled

Return Value:

    NTSTATUS - Returns the results of either posting the Irp or the
        saved completion status.

--*/

{
    PDEVICE_OBJECT Device;
    PVPB Vpb;
    PETHREAD Thread;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    
    //
    //  If there is not an irp context, then complete the request with the
    //  current status code.
    //

    if (!ARGUMENT_PRESENT( IrpContext )) {

        CdCompleteRequest( NULL, Irp, ExceptionCode );
        return ExceptionCode;
    }

    //
    //  Get the real exception status from the IrpContext.
    //

    ExceptionCode = IrpContext->ExceptionStatus;

    //
    //  If we are not a top level request then we just complete the request
    //  with the current status code.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL )) {

        CdCompleteRequest( IrpContext, Irp, ExceptionCode );
        return ExceptionCode;
    }

    //
    //  Check if we are posting this request.  One of the following must be true
    //  if we are to post a request.
    //
    //      - Status code is STATUS_CANT_WAIT and the request is asynchronous
    //          or we are forcing this to be posted.
    //
    //      - Status code is STATUS_VERIFY_REQUIRED and we are at APC level
    //          or higher.  Can't wait for IO in the verify path in this case.
    //
    //  Set the MORE_PROCESSING flag in the IrpContext to keep if from being
    //  deleted if this is a retryable condition.
    //
    //
    //  Note that (children of) CdFsdPostRequest can raise (Mdl allocation).
    //

    try {
    
        if (ExceptionCode == STATUS_CANT_WAIT) {

            if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST )) {

                ExceptionCode = CdFsdPostRequest( IrpContext, Irp );
            }

        } else if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

            if (KeGetCurrentIrql() >= APC_LEVEL) {

                ExceptionCode = CdFsdPostRequest( IrpContext, Irp );
            }
        }
    }
    except( CdExceptionFilter( IrpContext, GetExceptionInformation() ))  {
    
        ExceptionCode = GetExceptionCode();        
    }
    
    //
    //  If we posted the request or our caller will retry then just return here.
    //

    if ((ExceptionCode == STATUS_PENDING) ||
        (ExceptionCode == STATUS_CANT_WAIT)) {

        return ExceptionCode;
    }

    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING );

    //
    //  Store this error into the Irp for posting back to the Io system.
    //

    Irp->IoStatus.Status = ExceptionCode;

    if (IoIsErrorUserInduced( ExceptionCode )) {

        //
        //  Check for the various error conditions that can be caused by,
        //  and possibly resolved my the user.
        //

        if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

            //
            //  Now we are at the top level file system entry point.
            //
            //  If we have already posted this request then the device to
            //  verify is in the original thread.  Find this via the Irp.
            //

            Device = IoGetDeviceToVerify( Irp->Tail.Overlay.Thread );
            IoSetDeviceToVerify( Irp->Tail.Overlay.Thread, NULL );
            
            //
            //  If there is no device in that location then check in the
            //  current thread.
            //

            if (Device == NULL) {

                Device = IoGetDeviceToVerify( PsGetCurrentThread() );
                IoSetDeviceToVerify( PsGetCurrentThread(), NULL );

                ASSERT( Device != NULL );

                //
                //  Let's not BugCheck just because the driver messes up.
                //

                if (Device == NULL) {

                    ExceptionCode = STATUS_DRIVER_INTERNAL_ERROR;

                    CdCompleteRequest( IrpContext, Irp, ExceptionCode );

                    return ExceptionCode;
                }
            }

            //
            //  CdPerformVerify() will do the right thing with the Irp.
            //  If we return STATUS_CANT_WAIT then the current thread
            //  can retry the request.
            //

            return CdPerformVerify( IrpContext, Irp, Device );
        }

        //
        //  The other user induced conditions generate an error unless
        //  they have been disabled for this request.
        //

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS )) {

            CdCompleteRequest( IrpContext, Irp, ExceptionCode );

            return ExceptionCode;

        } 
        //
        //  Generate a pop-up.
        //
        else {

            if (IoGetCurrentIrpStackLocation( Irp )->FileObject != NULL) {

                Vpb = IoGetCurrentIrpStackLocation( Irp )->FileObject->Vpb;

            } else {

                Vpb = NULL;
            }

            //
            //  The device to verify is either in my thread local storage
            //  or that of the thread that owns the Irp.
            //

            Thread = Irp->Tail.Overlay.Thread;
            Device = IoGetDeviceToVerify( Thread );

            if (Device == NULL) {

                Thread = PsGetCurrentThread();
                Device = IoGetDeviceToVerify( Thread );

                ASSERT( Device != NULL );

                //
                //  Let's not BugCheck just because the driver messes up.
                //

                if (Device == NULL) {

                    CdCompleteRequest( IrpContext, Irp, ExceptionCode );

                    return ExceptionCode;
                }
            }

            //
            //  This routine actually causes the pop-up.  It usually
            //  does this by queuing an APC to the callers thread,
            //  but in some cases it will complete the request immediately,
            //  so it is very important to IoMarkIrpPending() first.
            //

            IoMarkIrpPending( Irp );
            IoRaiseHardError( Irp, Vpb, Device );

            //
            //  We will be handing control back to the caller here, so
            //  reset the saved device object.
            //

            IoSetDeviceToVerify( Thread, NULL );

            //
            //  The Irp will be completed by Io or resubmitted.  In either
            //  case we must clean up the IrpContext here.
            //

            CdCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );
            return STATUS_PENDING;
        }
    }

    //
    //  This is just a run of the mill error.
    //

    CdCompleteRequest( IrpContext, Irp, ExceptionCode );

    return ExceptionCode;
}


VOID
CdCompleteRequest (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine completes a Irp and cleans up the IrpContext.  Either or
    both of these may not be specified.

Arguments:

    Irp - Supplies the Irp being processed.

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );
    ASSERT_OPTIONAL_IRP( Irp );

    //
    //  Cleanup the IrpContext if passed in here.
    //

    if (ARGUMENT_PRESENT( IrpContext )) {

        CdCleanupIrpContext( IrpContext, FALSE );
    }

    //
    //  If we have an Irp then complete the irp.
    //

    if (ARGUMENT_PRESENT( Irp )) {

        //
        //  Clear the information field in case we have used this Irp
        //  internally.
        //

        if (NT_ERROR( Status ) &&
            FlagOn( Irp->Flags, IRP_INPUT_OPERATION )) {

            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;

        AssertVerifyDeviceIrp( Irp );
        
        IoCompleteRequest( Irp, IO_CD_ROM_INCREMENT );
    }

    return;
}


VOID
CdSetThreadContext (
    IN PIRP_CONTEXT IrpContext,
    IN PTHREAD_CONTEXT ThreadContext
    )

/*++

Routine Description:

    This routine is called at each Fsd/Fsp entry point set up the IrpContext
    and thread local storage to track top level requests.  If there is
    not a Cdfs context in the thread local storage then we use the input one.
    Otherwise we use the one already there.  This routine also updates the
    IrpContext based on the state of the top-level context.

    If the TOP_LEVEL flag in the IrpContext is already set when we are called
    then we force this request to appear top level.

Arguments:

    ThreadContext - Address on stack for local storage if not already present.

    ForceTopLevel - We force this request to appear top level regardless of
        any previous stack value.

Return Value:

    None

--*/

{
    PTHREAD_CONTEXT CurrentThreadContext;
    ULONG_PTR StackTop;
    ULONG_PTR StackBottom;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Get the current top-level irp out of the thread storage.
    //  If NULL then this is the top-level request.
    //

    CurrentThreadContext = (PTHREAD_CONTEXT) IoGetTopLevelIrp();

    if (CurrentThreadContext == NULL) {

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL );
    }

    //
    //  Initialize the input context unless we are using the current
    //  thread context block.  We use the new block if our caller
    //  specified this or the existing block is invalid.
    //
    //  The following must be true for the current to be a valid Cdfs context.
    //
    //      Structure must lie within current stack.
    //      Address must be ULONG aligned.
    //      Cdfs signature must be present.
    //
    //  If this is not a valid Cdfs context then use the input thread
    //  context and store it in the top level context.
    //

    IoGetStackLimits( &StackTop, &StackBottom);

    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL ) ||
        (((ULONG_PTR) CurrentThreadContext > StackBottom - sizeof( THREAD_CONTEXT )) ||
         ((ULONG_PTR) CurrentThreadContext <= StackTop) ||
         FlagOn( (ULONG_PTR) CurrentThreadContext, 0x3 ) ||
         (CurrentThreadContext->Cdfs != 0x53464443))) {

        ThreadContext->Cdfs = 0x53464443;
        ThreadContext->SavedTopLevelIrp = (PIRP) CurrentThreadContext;
        ThreadContext->TopLevelIrpContext = IrpContext;
        IoSetTopLevelIrp( (PIRP) ThreadContext );

        IrpContext->TopLevel = IrpContext;
        IrpContext->ThreadContext = ThreadContext;

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL_CDFS );

    //
    //  Otherwise use the IrpContext in the thread context.
    //

    } else {

        IrpContext->TopLevel = CurrentThreadContext->TopLevelIrpContext;
    }

    return;
}


BOOLEAN
CdFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine checks if fast i/o is possible for a read/write operation

Arguments:

    FileObject - Supplies the file object used in the query

    FileOffset - Supplies the starting byte offset for the read/write operation

    Length - Supplies the length, in bytes, of the read/write operation

    Wait - Indicates if we can wait

    LockKey - Supplies the lock key

    CheckForReadOperation - Indicates if this is a check for a read or write
        operation

    IoStatus - Receives the status of the operation if our return value is
        FastIoReturnError

Return Value:

    BOOLEAN - TRUE if fast I/O is possible and FALSE if the caller needs
        to take the long route.

--*/

{
    PFCB Fcb;
    TYPE_OF_OPEN TypeOfOpen;
    LARGE_INTEGER LargeLength;

    PAGED_CODE();

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure that is is only a user file open.
    //

    TypeOfOpen = CdFastDecodeFileObject( FileObject, &Fcb );

    if ((TypeOfOpen != UserFileOpen) || !CheckForReadOperation) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return TRUE;
    }

    LargeLength.QuadPart = Length;

    //
    //  Check whether the file locks will allow for fast io.
    //

    if ((Fcb->FileLock == NULL) ||
        FsRtlFastCheckLockForRead( Fcb->FileLock,
                                   FileOffset,
                                   &LargeLength,
                                   LockKey,
                                   FileObject,
                                   PsGetCurrentProcess() )) {

        return TRUE;
    }

    return FALSE;
}


ULONG
CdSerial32 (
    IN PCHAR Buffer,
    IN ULONG ByteCount
    )
/*++

Routine Description:

    This routine is called to generate a 32 bit serial number.  This is
    done by doing four separate checksums into an array of bytes and
    then treating the bytes as a ULONG.

Arguments:

    Buffer - Pointer to the buffer to generate the ID for.

    ByteCount - Number of bytes in the buffer.

Return Value:

    ULONG - The 32 bit serial number.

--*/

{
    union {
        UCHAR   Bytes[4];
        ULONG   SerialId;
    } Checksum;

    PAGED_CODE();

    //
    //  Initialize the serial number.
    //

    Checksum.SerialId = 0;

    //
    //  Continue while there are more bytes to use.
    //

    while (ByteCount--) {

        //
        //  Increment this sub-checksum.
        //

        Checksum.Bytes[ByteCount & 0x3] += *(Buffer++);
    }

    //
    //  Return the checksums as a ULONG.
    //

    return Checksum.SerialId;
}


