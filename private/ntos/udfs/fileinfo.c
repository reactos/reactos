/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the File Information routines for Udfs called by
    the Fsd/Fsp dispatch drivers.

Author:

    Dan Lovinger    [DanLo]     16-Jan-1997

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_FILEINFO)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_FILEINFO)

//
//  Local macros
//

INLINE
ULONG
UdfGetExtraFileAttributes (
    IN PCCB Ccb
    )

/*++

Routine Description:

    Safely figure out extra name-based file attributes given a context block.

Arguments:

    Ccb - a context block to examine.

Return Value:

    ULONG - file attributes for a file based on how it was opened (seperate from
        those based on the object that was opened).

--*/

{
    return ( Ccb->Lcb != NULL? Ccb->Lcb->FileAttributes : 0 );
}

//
//  Local support routines
//

VOID
UdfQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
UdfQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
UdfQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
UdfQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
UdfQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UdfQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UdfQueryAlternateNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
UdfQueryNetworkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN OUT PULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCommonQueryInfo)
#pragma alloc_text(PAGE, UdfCommonSetInfo)
#pragma alloc_text(PAGE, UdfFastQueryBasicInfo)
#pragma alloc_text(PAGE, UdfFastQueryStdInfo)
#pragma alloc_text(PAGE, UdfFastQueryNetworkInfo)
#pragma alloc_text(PAGE, UdfQueryAlternateNameInfo)
#pragma alloc_text(PAGE, UdfQueryBasicInfo)
#pragma alloc_text(PAGE, UdfQueryEaInfo)
#pragma alloc_text(PAGE, UdfQueryInternalInfo)
#pragma alloc_text(PAGE, UdfQueryNameInfo)
#pragma alloc_text(PAGE, UdfQueryNetworkInfo)
#pragma alloc_text(PAGE, UdfQueryPositionInfo)
#pragma alloc_text(PAGE, UdfQueryStandardInfo)
#endif


NTSTATUS
UdfCommonQueryInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for query file information called by both the
    fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    NTSTATUS - The return status for this operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    ULONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    PFILE_ALL_INFORMATION Buffer;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN ReleaseFcb = FALSE;

    PAGED_CODE();

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryFile.Length;
    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Decode the file object
    //

    TypeOfOpen = UdfDecodeFileObject( IrpSp->FileObject, &Fcb, &Ccb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We only support query on file and directory handles.
        //

        switch (TypeOfOpen) {

        case UserDirectoryOpen :
        case UserFileOpen :

            //
            //  Acquire shared access to this file.
            //

            UdfAcquireFileShared( IrpContext, Fcb );
            ReleaseFcb = TRUE;

            ASSERT( FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED ));
            
            //
            //  Make sure the Fcb is in a usable condition.  This will raise
            //  an error condition if the volume is unusable
            //

            UdfVerifyFcbOperation( IrpContext, Fcb );

            //
            //  Based on the information class we'll do different
            //  actions.  Each of hte procedures that we're calling fills
            //  up the output buffer, if possible.  They will raise the
            //  status STATUS_BUFFER_OVERFLOW for an insufficient buffer.
            //  This is considered a somewhat unusual case and is handled
            //  more cleanly with the exception mechanism rather than
            //  testing a return status value for each call.
            //

            switch (FileInformationClass) {

            case FileAllInformation:

                //
                //  We don't allow this operation on a file opened by file Id.
                //

                if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                //
                //  In this case go ahead and call the individual routines to
                //  fill in the buffer.  Only the name routine will
                //  pointer to the output buffer and then call the
                //  individual routines to fill in the buffer.
                //

                Length -= (sizeof( FILE_ACCESS_INFORMATION ) +
                           sizeof( FILE_MODE_INFORMATION ) +
                           sizeof( FILE_ALIGNMENT_INFORMATION ));

                UdfQueryBasicInfo( IrpContext, Fcb, Ccb, &Buffer->BasicInformation, &Length );
                UdfQueryStandardInfo( IrpContext, Fcb, &Buffer->StandardInformation, &Length );
                UdfQueryInternalInfo( IrpContext, Fcb, &Buffer->InternalInformation, &Length );
                UdfQueryEaInfo( IrpContext, Fcb, &Buffer->EaInformation, &Length );
                UdfQueryPositionInfo( IrpContext, IrpSp->FileObject, &Buffer->PositionInformation, &Length );
                Status = UdfQueryNameInfo( IrpContext, IrpSp->FileObject, &Buffer->NameInformation, &Length );

                break;

            case FileBasicInformation:

                UdfQueryBasicInfo( IrpContext, Fcb, Ccb, (PFILE_BASIC_INFORMATION) Buffer, &Length );
                break;

            case FileStandardInformation:

                UdfQueryStandardInfo( IrpContext, Fcb, (PFILE_STANDARD_INFORMATION) Buffer, &Length );
                break;

            case FileInternalInformation:

                UdfQueryInternalInfo( IrpContext, Fcb, (PFILE_INTERNAL_INFORMATION) Buffer, &Length );
                break;

            case FileEaInformation:

                UdfQueryEaInfo( IrpContext, Fcb, (PFILE_EA_INFORMATION) Buffer, &Length );
                break;

            case FilePositionInformation:

                UdfQueryPositionInfo( IrpContext, IrpSp->FileObject, (PFILE_POSITION_INFORMATION) Buffer, &Length );
                break;

            case FileNameInformation:

                //
                //  We don't allow this operation on a file opened by file Id.
                //

                if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    Status = UdfQueryNameInfo( IrpContext, IrpSp->FileObject, (PFILE_NAME_INFORMATION) Buffer, &Length );

                } else {

                    Status = STATUS_INVALID_PARAMETER;
                }

                break;

            case FileAlternateNameInformation:

                if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    Status = UdfQueryAlternateNameInfo( IrpContext, Fcb, Ccb, (PFILE_NAME_INFORMATION) Buffer, &Length );

                } else {

                    Status = STATUS_INVALID_PARAMETER;
                }

                break;

            case FileNetworkOpenInformation:

                UdfQueryNetworkInfo( IrpContext, Fcb, Ccb, (PFILE_NETWORK_OPEN_INFORMATION) Buffer, &Length );
                break;

            default :

                Status = STATUS_INVALID_PARAMETER;
            }

            break;

        default :

            Status = STATUS_INVALID_PARAMETER;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //  and then complete the request
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - Length;

    } finally {

        //
        //  Release the file.
        //

        if (ReleaseFcb) {

            UdfReleaseFile( IrpContext, Fcb );
        }
    }

    //
    //  Complete the request if we didn't raise.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


NTSTATUS
UdfCommonSetInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for set file information called by both the
    fsd and fsp threads.  We only support operations which set the file position.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    NTSTATUS - The return status for this operation.

--*/

{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFILE_POSITION_INFORMATION Buffer;

    PAGED_CODE();

    //
    //  Decode the file object
    //

    TypeOfOpen = UdfDecodeFileObject( IrpSp->FileObject, &Fcb, &Ccb );

    //
    //  We only support a SetPositionInformation on a user file.
    //

    if ((TypeOfOpen != UserFileOpen) ||
        (IrpSp->Parameters.QueryFile.FileInformationClass != FilePositionInformation)) {

        UdfCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  Acquire shared access to this file.
    //

    UdfAcquireFileShared( IrpContext, Fcb );

    try {

        //
        //  Make sure the Fcb is in a usable condition.  This
        //  will raise an error condition if the fcb is unusable
        //

        UdfVerifyFcbOperation( IrpContext, Fcb );

        Buffer = Irp->AssociatedIrp.SystemBuffer;

        //
        //  Check if the file does not use intermediate buffering.  If it
        //  does not use intermediate buffering then the new position we're
        //  supplied must be aligned properly for the device
        //

        if (FlagOn( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ) &&
            ((Buffer->CurrentByteOffset.LowPart & IrpSp->DeviceObject->AlignmentRequirement) != 0)) {

            try_leave( NOTHING );
        }

        //
        //  The input parameter is fine so set the current byte offset and
        //  complete the request
        //

        //
        //  Lock the Fcb to provide synchronization.
        //

        UdfLockFcb( IrpContext, Fcb );
        IrpSp->FileObject->CurrentByteOffset = Buffer->CurrentByteOffset;
        UdfUnlockFcb( IrpContext, Fcb );

        Status = STATUS_SUCCESS;

    } finally {

        UdfReleaseFile( IrpContext, Fcb );
    }

    //
    //  Complete the request if there was no raise.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


BOOLEAN
UdfFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for basic file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Result = FALSE;
    TYPE_OF_OPEN TypeOfOpen;

    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    ASSERT_FILE_OBJECT( FileObject );

    FsRtlEnterFileSystem();

    //
    //  Decode the file object to find the type of open and the data
    //  structures.
    //

    TypeOfOpen = UdfDecodeFileObject( FileObject, &Fcb, &Ccb );

    //
    //  We only support this request on user file or directory objects.
    //

    ASSERT( FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED ));
    
    if (TypeOfOpen != UserFileOpen && TypeOfOpen != UserDirectoryOpen) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Acquire the file shared to access the Fcb.
    //

    if (!ExAcquireResourceShared( Fcb->Resource, Wait )) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Only deal with 'good' Fcb's.
        //

        if (UdfVerifyFcbOperation( NULL, Fcb )) {

            //
            //  Fill in the input buffer from the Fcb fields.
            //

            Buffer->CreationTime = Fcb->Timestamps.CreationTime;
            Buffer->LastWriteTime =
            Buffer->ChangeTime =  Fcb->Timestamps.ModificationTime;
            Buffer->LastAccessTime = Fcb->Timestamps.AccessTime;

            Buffer->FileAttributes = Fcb->FileAttributes | UdfGetExtraFileAttributes( Ccb );

            //
            //  Update the IoStatus block with the size of this data.
            //

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( FILE_BASIC_INFORMATION );

            Result = TRUE;
        }

    } finally {

        ExReleaseResource( Fcb->Resource );

        FsRtlExitFileSystem();
    }

    return Result;
}


BOOLEAN
UdfFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for standard file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Result = FALSE;
    TYPE_OF_OPEN TypeOfOpen;

    PFCB Fcb;

    PAGED_CODE();

    ASSERT_FILE_OBJECT( FileObject );

    FsRtlEnterFileSystem();

    //
    //  Decode the file object to find the type of open and the data
    //  structures.
    //

    TypeOfOpen = UdfFastDecodeFileObject( FileObject, &Fcb );

    //
    //  We only support this request on initialized user file or directory objects.
    //

    if (TypeOfOpen != UserFileOpen && TypeOfOpen != UserDirectoryOpen) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Acquire the file shared to access the Fcb.
    //

    if (!ExAcquireResourceShared( Fcb->Resource, Wait )) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Only deal with 'good' Fcb's.
        //

        if (UdfVerifyFcbOperation( NULL, Fcb )) {

            //
            //  Check whether this is a directory.
            //

            if (FlagOn( Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY )) {

                Buffer->AllocationSize.QuadPart =
                Buffer->EndOfFile.QuadPart = 0;

                Buffer->Directory = TRUE;

            } else {

                Buffer->AllocationSize.QuadPart = Fcb->AllocationSize.QuadPart;
                Buffer->EndOfFile.QuadPart = Fcb->FileSize.QuadPart;

                Buffer->Directory = FALSE;
            }

            Buffer->NumberOfLinks = Fcb->LinkCount;
            Buffer->DeletePending = FALSE;

            //
            //  Update the IoStatus block with the size of this data.
            //

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( FILE_STANDARD_INFORMATION );

            Result = TRUE;
        }

    } finally {

        ExReleaseResource( Fcb->Resource );

        FsRtlExitFileSystem();
    }

    return Result;
}


BOOLEAN
UdfFastQueryNetworkInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for network file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Result = FALSE;
    TYPE_OF_OPEN TypeOfOpen;

    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    ASSERT_FILE_OBJECT( FileObject );

    FsRtlEnterFileSystem();

    //
    //  Decode the file object to find the type of open and the data
    //  structures.
    //

    TypeOfOpen = UdfDecodeFileObject( FileObject, &Fcb, &Ccb );

    //
    //  We only support this request on user file or directory objects.
    //

    if (TypeOfOpen != UserFileOpen && TypeOfOpen != UserDirectoryOpen) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Acquire the file shared to access the Fcb.
    //

    if (!ExAcquireResourceShared( Fcb->Resource, Wait )) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Only deal with 'good' Fcb's.
        //

        if (UdfVerifyFcbOperation( NULL, Fcb )) {

            //
            //  Fill in the input buffer from the Fcb fields.
            //

            Buffer->CreationTime = Fcb->Timestamps.CreationTime;
            Buffer->LastWriteTime =
            Buffer->ChangeTime =  Fcb->Timestamps.ModificationTime;
            Buffer->LastAccessTime = Fcb->Timestamps.AccessTime;

            Buffer->FileAttributes = Fcb->FileAttributes | UdfGetExtraFileAttributes( Ccb );

            //
            //  Check whether this is a directory.
            //

            if (FlagOn( Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY )) {

                Buffer->AllocationSize.QuadPart =
                Buffer->EndOfFile.QuadPart = 0;

            } else {

                Buffer->AllocationSize.QuadPart = Fcb->AllocationSize.QuadPart;
                Buffer->EndOfFile.QuadPart = Fcb->FileSize.QuadPart;
            }

            //
            //  Update the IoStatus block with the size of this data.
            //

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( FILE_NETWORK_OPEN_INFORMATION );

            Result = TRUE;
        }

    } finally {

        ExReleaseResource( Fcb->Resource );

        FsRtlExitFileSystem();
    }

    return Result;
}


//
//  Local support routine
//

VOID
UdfQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

 Description:

    This routine performs the query basic information function for Udfs

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified
    
    Ccb - Supplies the Ccb associated with the fileobject being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  We support all times on Udfs.
    //

    Buffer->CreationTime = Fcb->Timestamps.CreationTime;
    Buffer->LastWriteTime =
    Buffer->ChangeTime =  Fcb->Timestamps.ModificationTime;
    Buffer->LastAccessTime = Fcb->Timestamps.AccessTime;

    Buffer->FileAttributes = Fcb->FileAttributes | UdfGetExtraFileAttributes( Ccb );

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_BASIC_INFORMATION );

    return;
}


//
//  Local support routine
//

VOID
UdfQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length
    )
/*++

Routine Description:

    This routine performs the query standard information function for Udfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Delete is never pending on a readonly file.
    //

    Buffer->NumberOfLinks = Fcb->LinkCount;
    Buffer->DeletePending = FALSE;

    //
    //  We get the sizes from the header.  Return a size of zero
    //  for all directories.
    //

    if (FlagOn( Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY )) {

        Buffer->AllocationSize.QuadPart =
        Buffer->EndOfFile.QuadPart = 0;

        Buffer->Directory = TRUE;

    } else {

        Buffer->AllocationSize.QuadPart = Fcb->AllocationSize.QuadPart;
        Buffer->EndOfFile.QuadPart = Fcb->FileSize.QuadPart;

        Buffer->Directory = FALSE;
    }

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_STANDARD_INFORMATION );

    return;
}


//
//  Local support routine
//

VOID
UdfQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query internal information function for Udfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Index number is the file Id number in the Fcb.
    //

    Buffer->IndexNumber = Fcb->FileId;
    *Length -= sizeof( FILE_INTERNAL_INFORMATION );

    return;
}


//
//  Local support routine
//

VOID
UdfQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query Ea information function for Udfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  No Ea's on Udfs volumes.  At least not that our EA support would understand.
    //

    Buffer->EaSize = 0;
    *Length -= sizeof( FILE_EA_INFORMATION );

    return;
}


//
//  Local support routine
//

VOID
UdfQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query position information function for Udfs.

Arguments:

    FileObject - Supplies the File object being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Get the current position found in the file object.
    //

    Buffer->CurrentByteOffset = FileObject->CurrentByteOffset;

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_POSITION_INFORMATION );

    return;
}


//
//  Local support routine
//

NTSTATUS
UdfQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query name information function for Udfs.

Arguments:

    FileObject - Supplies the file object containing the name.

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    NTSTATUS - STATUS_BUFFER_OVERFLOW if the entire name can't be copied.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthToCopy;

    PAGED_CODE();

    ASSERT(*Length >= sizeof(ULONG));
    
    //
    //  Simply copy the name in the file object to the user's buffer.
    //

    //
    //  Place the size of the filename in the user's buffer and reduce the remaining
    //  size to match.
    //

    Buffer->FileNameLength = LengthToCopy = FileObject->FileName.Length;
    *Length -= sizeof(ULONG);

    if (LengthToCopy > *Length) {

        LengthToCopy = *Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory( Buffer->FileName, FileObject->FileName.Buffer, LengthToCopy );

    //
    //  Reduce the available bytes by the amount stored into this buffer.  In the overflow
    //  case, this simply drops to zero.  The returned filenamelength will indicate to the
    //  caller how much space is required.
    //

    *Length -= LengthToCopy;

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfQueryAlternateNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query alternate name information function.
    We lookup the dirent for this file and then check if there is a
    short name.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified.

    Ccb - Ccb for this open handle.

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned.

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the whole name would fit into the user buffer,
               STATUS_OBJECT_NAME_NOT_FOUND if we can't return the name,
               STATUS_BUFFER_OVERFLOW otherwise.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    DIR_ENUM_CONTEXT DirContext;

    PLCB Lcb;
    
    PFCB ParentFcb;
    BOOLEAN ReleaseParentFcb = FALSE;

    BOOLEAN CleanupDirContext = FALSE;
    BOOLEAN Result;

    PUNICODE_STRING ShortName;

    UNICODE_STRING LocalShortName;
    WCHAR LocalShortNameBuffer[ BYTE_COUNT_8_DOT_3 / sizeof(WCHAR) ];
    
    PAGED_CODE();

    //
    //  Initialize the buffer length to zero.
    //

    Buffer->FileNameLength = 0;

    //
    //  If there was no associated Lcb then there is no short name.
    //

    Lcb = Ccb->Lcb;
    
    if (Lcb == NULL) {

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    //  Use a try-finally to cleanup the structures.
    //

    try {

        if (FlagOn( Lcb->Flags, LCB_FLAG_SHORT_NAME )) {

            //
            //  This caller opened the file by a generated short name, so simply hand it back.
            //

            ShortName = &Lcb->FileName;
        
        } else {

            //
            //  The open occured by a regular name.  Now, if this name is already 8.3 legal then
            //  there is no short name.
            //

            if (UdfIs8dot3Name( IrpContext, Lcb->FileName )) {

                try_leave( Status = STATUS_OBJECT_NAME_NOT_FOUND );
            }

            //
            //  This name has a generated short name.  In order to calculate this name we have to
            //  retrieve the FID for this file, since UDF specifies that a short name is uniquified
            //  with a CRC of the original in-FID byte representation of the filename.
            //
            //  N.B.: if this is a common operation, we may wish to cache the CRC in the Lcb.
            //

            ParentFcb = Lcb->ParentFcb;
            UdfAcquireFileShared( IrpContext, ParentFcb );
            ReleaseParentFcb = TRUE;

            //
            //  Now go find the FID for this filename in the parent.
            //

            UdfInitializeDirContext( IrpContext, &DirContext );
            CleanupDirContext = TRUE;

            Result = UdfFindDirEntry( IrpContext,
                                      ParentFcb,
                                      &Lcb->FileName,
                                      BooleanFlagOn( Lcb->Flags, LCB_FLAG_IGNORE_CASE ),
                                      FALSE,
                                      &DirContext );
            
            //
            //  We should always be able to find this entry, but don't bugcheck because
            //  we screwed this up.
            //
            
            ASSERT( Result );
            
            if (!Result) {
                
                try_leave( Status = STATUS_OBJECT_NAME_NOT_FOUND );
            }

            //
            //  Build the local unicode string to use and fill it in.
            //

            ShortName = &LocalShortName;

            LocalShortName.Buffer = LocalShortNameBuffer;
            LocalShortName.Length = 0;
            LocalShortName.MaximumLength = sizeof( LocalShortNameBuffer );

            UdfGenerate8dot3Name( IrpContext,
                                  &DirContext.CaseObjectName,
                                  ShortName );
        }
        
        //
        //  We now have the short name.  We have left it in Unicode form so copy it directly.
        //

        Buffer->FileNameLength = ShortName->Length;

        if (Buffer->FileNameLength + sizeof( ULONG ) > *Length) {

            Buffer->FileNameLength = *Length - sizeof( ULONG );
            Status = STATUS_BUFFER_OVERFLOW;
        }

        RtlCopyMemory( Buffer->FileName, ShortName->Buffer, Buffer->FileNameLength );

    } finally {

        if (CleanupDirContext) {

            UdfCleanupDirContext( IrpContext, &DirContext );
        }

        if (ReleaseParentFcb) {

            UdfReleaseFile( IrpContext, ParentFcb );
        }
    }

    //
    //  Reduce the available bytes by the amount stored into this buffer.
    //

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

        *Length -= sizeof( ULONG ) + Buffer->FileNameLength;
    }

    return Status;
}


//
//  Local support routine
//

VOID
UdfQueryNetworkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

 Description:

    This routine performs the query network open information function for Udfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  We support all times on Udfs.
    //

    Buffer->CreationTime = Fcb->Timestamps.CreationTime;
    Buffer->LastWriteTime =
    Buffer->ChangeTime =  Fcb->Timestamps.ModificationTime;
    Buffer->LastAccessTime = Fcb->Timestamps.AccessTime;

    Buffer->FileAttributes = Fcb->FileAttributes | UdfGetExtraFileAttributes( Ccb );

    //
    //  We get the sizes from the header.  Return a size of zero
    //  for all directories.
    //

    if (FlagOn( Fcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY )) {

        Buffer->AllocationSize.QuadPart =
        Buffer->EndOfFile.QuadPart = 0;

    } else {

        Buffer->AllocationSize.QuadPart = Fcb->AllocationSize.QuadPart;
        Buffer->EndOfFile.QuadPart = Fcb->FileSize.QuadPart;
    }

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_NETWORK_OPEN_INFORMATION );

    return;
}
