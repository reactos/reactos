/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the File Information routines for Cdfs called by
    the Fsd/Fsp dispatch drivers.


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_FILEINFO)

//
//  Local support routines
//

VOID
CdQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
CdQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
CdQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
CdQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
CdQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
CdQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
CdQueryAlternateNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
CdQueryNetworkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN OUT PULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonQueryInfo)
#pragma alloc_text(PAGE, CdCommonSetInfo)
#pragma alloc_text(PAGE, CdFastQueryBasicInfo)
#pragma alloc_text(PAGE, CdFastQueryStdInfo)
#pragma alloc_text(PAGE, CdFastQueryNetworkInfo)
#pragma alloc_text(PAGE, CdQueryAlternateNameInfo)
#pragma alloc_text(PAGE, CdQueryBasicInfo)
#pragma alloc_text(PAGE, CdQueryEaInfo)
#pragma alloc_text(PAGE, CdQueryInternalInfo)
#pragma alloc_text(PAGE, CdQueryNameInfo)
#pragma alloc_text(PAGE, CdQueryNetworkInfo)
#pragma alloc_text(PAGE, CdQueryPositionInfo)
#pragma alloc_text(PAGE, CdQueryStandardInfo)
#endif


NTSTATUS
CdCommonQueryInfo (
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

    TypeOfOpen = CdDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb );

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
            //  Acquire shared access to this file.  NOTE that this could be
            //  a recursive acquire,  if we already preacquired in
            //  CdAcquireForCreateSection().
            //

            CdAcquireFileShared( IrpContext, Fcb );
            ReleaseFcb = TRUE;

            //
            //  Make sure we have the correct sizes for a directory.
            //

            if (!FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED )) {

                ASSERT( TypeOfOpen == UserDirectoryOpen );
                CdCreateInternalStream( IrpContext, Fcb->Vcb, Fcb );
            }

            //
            //  Make sure the Fcb is in a usable condition.  This will raise
            //  an error condition if the volume is unusable
            //

            CdVerifyFcbOperation( IrpContext, Fcb );

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

                CdQueryBasicInfo( IrpContext, Fcb, &Buffer->BasicInformation, &Length );
                CdQueryStandardInfo( IrpContext, Fcb, &Buffer->StandardInformation, &Length );
                CdQueryInternalInfo( IrpContext, Fcb, &Buffer->InternalInformation, &Length );
                CdQueryEaInfo( IrpContext, Fcb, &Buffer->EaInformation, &Length );
                CdQueryPositionInfo( IrpContext, IrpSp->FileObject, &Buffer->PositionInformation, &Length );
                Status = CdQueryNameInfo( IrpContext, IrpSp->FileObject, &Buffer->NameInformation, &Length );

                break;

            case FileBasicInformation:

                CdQueryBasicInfo( IrpContext, Fcb, (PFILE_BASIC_INFORMATION) Buffer, &Length );
                break;

            case FileStandardInformation:

                CdQueryStandardInfo( IrpContext, Fcb, (PFILE_STANDARD_INFORMATION) Buffer, &Length );
                break;

            case FileInternalInformation:

                CdQueryInternalInfo( IrpContext, Fcb, (PFILE_INTERNAL_INFORMATION) Buffer, &Length );
                break;

            case FileEaInformation:

                CdQueryEaInfo( IrpContext, Fcb, (PFILE_EA_INFORMATION) Buffer, &Length );
                break;

            case FilePositionInformation:

                CdQueryPositionInfo( IrpContext, IrpSp->FileObject, (PFILE_POSITION_INFORMATION) Buffer, &Length );
                break;

            case FileNameInformation:

                //
                //  We don't allow this operation on a file opened by file Id.
                //

                if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    Status = CdQueryNameInfo( IrpContext, IrpSp->FileObject, (PFILE_NAME_INFORMATION) Buffer, &Length );

                } else {

                    Status = STATUS_INVALID_PARAMETER;
                }

                break;

            case FileAlternateNameInformation:

                if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    Status = CdQueryAlternateNameInfo( IrpContext, Fcb, Ccb, (PFILE_NAME_INFORMATION) Buffer, &Length );

                } else {

                    Status = STATUS_INVALID_PARAMETER;
                }

                break;

            case FileNetworkOpenInformation:

                CdQueryNetworkInfo( IrpContext, Fcb, (PFILE_NETWORK_OPEN_INFORMATION) Buffer, &Length );
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

            CdReleaseFile( IrpContext, Fcb );
        }
    }

    //
    //  Complete the request if we didn't raise.
    //

    CdCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


NTSTATUS
CdCommonSetInfo (
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

    TypeOfOpen = CdDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb );

    //
    //  We only support a SetPositionInformation on a user file.
    //

    if ((TypeOfOpen != UserFileOpen) ||
        (IrpSp->Parameters.QueryFile.FileInformationClass != FilePositionInformation)) {

        CdCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  Acquire shared access to this file.
    //

    CdAcquireFileShared( IrpContext, Fcb );

    try {

        //
        //  Make sure the Fcb is in a usable condition.  This
        //  will raise an error condition if the fcb is unusable
        //

        CdVerifyFcbOperation( IrpContext, Fcb );

        Buffer = Irp->AssociatedIrp.SystemBuffer;

        //
        //  Check if the file does not use intermediate buffering.  If it
        //  does not use intermediate buffering then the new position we're
        //  supplied must be aligned properly for the device
        //

        if (FlagOn( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ) &&
            ((Buffer->CurrentByteOffset.LowPart & Fcb->Vcb->BlockMask) != 0)) {

            try_return( NOTHING );
        }

        //
        //  The input parameter is fine so set the current byte offset and
        //  complete the request
        //

        //
        //  Lock the Fcb to provide synchronization.
        //

        CdLockFcb( IrpContext, Fcb );
        IrpSp->FileObject->CurrentByteOffset = Buffer->CurrentByteOffset;
        CdUnlockFcb( IrpContext, Fcb );

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        CdReleaseFile( IrpContext, Fcb );
    }

    //
    //  Complete the request if there was no raise.
    //

    CdCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


BOOLEAN
CdFastQueryBasicInfo (
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

    PAGED_CODE();

    ASSERT_FILE_OBJECT( FileObject );

    FsRtlEnterFileSystem();

    //
    //  Decode the file object to find the type of open and the data
    //  structures.
    //

    TypeOfOpen = CdFastDecodeFileObject( FileObject, &Fcb );

    //
    //  We only support this request on user file or directory objects.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        ((TypeOfOpen != UserDirectoryOpen) || !FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED))) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Acquire the file shared to access the Fcb.
    //

    if (!ExAcquireResourceSharedLite( Fcb->Resource, Wait )) {

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

        if (CdVerifyFcbOperation( NULL, Fcb )) {

            //
            //  Fill in the input buffer from the Fcb fields.
            //

            Buffer->CreationTime.QuadPart =
            Buffer->LastWriteTime.QuadPart =
            Buffer->ChangeTime.QuadPart = Fcb->CreationTime;

            Buffer->LastAccessTime.QuadPart = 0;

            Buffer->FileAttributes = Fcb->FileAttributes;

            //
            //  Update the IoStatus block with the size of this data.
            //

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( FILE_BASIC_INFORMATION );

            Result = TRUE;
        }

    } finally {

        ExReleaseResourceLite( Fcb->Resource );

        FsRtlExitFileSystem();
    }

    return Result;
}


BOOLEAN
CdFastQueryStdInfo (
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

    TypeOfOpen = CdFastDecodeFileObject( FileObject, &Fcb );

    //
    //  We only support this request on initialized user file or directory objects.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        ((TypeOfOpen != UserDirectoryOpen) || !FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED ))) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Acquire the file shared to access the Fcb.
    //

    if (!ExAcquireResourceSharedLite( Fcb->Resource, Wait )) {

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

        if (CdVerifyFcbOperation( NULL, Fcb )) {

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

            Buffer->NumberOfLinks = 1;
            Buffer->DeletePending = FALSE;

            //
            //  Update the IoStatus block with the size of this data.
            //

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof( FILE_STANDARD_INFORMATION );

            Result = TRUE;
        }

    } finally {

        ExReleaseResourceLite( Fcb->Resource );

        FsRtlExitFileSystem();
    }

    return Result;
}


BOOLEAN
CdFastQueryNetworkInfo (
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

    PAGED_CODE();

    ASSERT_FILE_OBJECT( FileObject );

    FsRtlEnterFileSystem();

    //
    //  Decode the file object to find the type of open and the data
    //  structures.
    //

    TypeOfOpen = CdFastDecodeFileObject( FileObject, &Fcb );

    //
    //  We only support this request on user file or directory objects.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        ((TypeOfOpen != UserDirectoryOpen) || !FlagOn( Fcb->FcbState, FCB_STATE_INITIALIZED))) {

        FsRtlExitFileSystem();
        return FALSE;
    }

    //
    //  Acquire the file shared to access the Fcb.
    //

    if (!ExAcquireResourceSharedLite( Fcb->Resource, Wait )) {

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

        if (CdVerifyFcbOperation( NULL, Fcb )) {

            //
            //  Fill in the input buffer from the Fcb fields.
            //

            Buffer->CreationTime.QuadPart =
            Buffer->LastWriteTime.QuadPart =
            Buffer->ChangeTime.QuadPart = Fcb->CreationTime;

            Buffer->LastAccessTime.QuadPart = 0;

            Buffer->FileAttributes = Fcb->FileAttributes;

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

        ExReleaseResourceLite( Fcb->Resource );

        FsRtlExitFileSystem();
    }

    return Result;
}


//
//  Local support routine
//

VOID
CdQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

 Description:

    This routine performs the query basic information function for Cdfs

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
    //  We only support creation, last modify and last write times on Cdfs.
    //

    Buffer->LastWriteTime.QuadPart =
    Buffer->CreationTime.QuadPart =
    Buffer->ChangeTime.QuadPart = Fcb->CreationTime;

    Buffer->LastAccessTime.QuadPart = 0;

    Buffer->FileAttributes = Fcb->FileAttributes;

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
CdQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length
    )
/*++

Routine Description:

    This routine performs the query standard information function for cdfs.

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
    //  There is only one link and delete is never pending on a Cdrom file.
    //

    Buffer->NumberOfLinks = 1;
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
CdQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query internal information function for cdfs.

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
CdQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query Ea information function for cdfs.

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
    //  No Ea's on Cdfs volumes.
    //

    Buffer->EaSize = 0;
    *Length -= sizeof( FILE_EA_INFORMATION );

    return;
}


//
//  Local support routine
//

VOID
CdQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query position information function for cdfs.

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
CdQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query name information function for cdfs.

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
CdQueryAlternateNameInfo (
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

    DIRENT_ENUM_CONTEXT DirContext;
    DIRENT Dirent;

    PUNICODE_STRING NameToUse;
    ULONG DirentOffset;

    COMPOUND_PATH_ENTRY CompoundPathEntry;
    FILE_ENUM_CONTEXT FileContext;

    PFCB ParentFcb;
    BOOLEAN ReleaseParentFcb = FALSE;

    BOOLEAN CleanupFileLookup = FALSE;
    BOOLEAN CleanupDirectoryLookup = FALSE;

    WCHAR ShortNameBuffer[ BYTE_COUNT_8_DOT_3 / 2 ];
    USHORT ShortNameLength;

    PAGED_CODE();

    //
    //  Initialize the buffer length to zero.
    //

    Buffer->FileNameLength = 0;

    //
    //  If this is the root or this file was opened using a version number then
    //  there is no short name.
    //

    if ((Fcb == Fcb->Vcb->RootIndexFcb) ||
        FlagOn( Ccb->Flags, CCB_FLAG_OPEN_WITH_VERSION)) {

        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    //  Use a try-finally to cleanup the structures.
    //

    try {

        ParentFcb = Fcb->ParentFcb;
        CdAcquireFileShared( IrpContext, ParentFcb );
        ReleaseParentFcb = TRUE;
    
        //
        //  Do an unsafe test to see if we need to create a file object.
        //

        if (ParentFcb->FileObject == NULL) {

            CdCreateInternalStream( IrpContext, ParentFcb->Vcb, ParentFcb );
        }

        if (CdFidIsDirectory( Fcb->FileId)) {

            //
            //  Fcb is for a directory, so we need to dig the dirent from the parent.  In
            //  order to do this we need to get the name of the directory from its pathtable
            //  entry and then search in the parent for a matching dirent.
            //
            //  This could be optimized somewhat.
            //

            CdInitializeCompoundPathEntry( IrpContext, &CompoundPathEntry );
            CdInitializeFileContext( IrpContext, &FileContext );

            CleanupDirectoryLookup = TRUE;

            CdLookupPathEntry( IrpContext,
                               CdQueryFidPathTableOffset( Fcb->FileId ),
                               Fcb->Ordinal,
                               FALSE,
                               &CompoundPathEntry );

            CdUpdatePathEntryName( IrpContext, &CompoundPathEntry.PathEntry, TRUE );

            if (!CdFindDirectory( IrpContext,
                                  ParentFcb,
                                  &CompoundPathEntry.PathEntry.CdCaseDirName,
                                  TRUE,
                                  &FileContext )) {

                //
                //  If we failed to find the child directory by name in the parent
                //  something is quite wrong with this disc.
                //

                CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
            }

            NameToUse = &FileContext.InitialDirent->Dirent.CdCaseFileName.FileName;
            DirentOffset = FileContext.InitialDirent->Dirent.DirentOffset;
        
        } else {

            //
            //  Initialize the search dirent structures.
            //
        
            CdInitializeDirContext( IrpContext, &DirContext );
            CdInitializeDirent( IrpContext, &Dirent );
    
            CleanupFileLookup = TRUE;
        
            CdLookupDirent( IrpContext,
                            ParentFcb,
                            CdQueryFidDirentOffset( Fcb->FileId ),
                            &DirContext );
    
            CdUpdateDirentFromRawDirent( IrpContext,
                                         ParentFcb,
                                         &DirContext,
                                         &Dirent );

            //
            //  Now update the dirent name.
            //
    
            CdUpdateDirentName( IrpContext, &Dirent, TRUE );
    
            NameToUse = &Dirent.CdCaseFileName.FileName;
            DirentOffset = Dirent.DirentOffset;
        }

        //
        //  If the name is 8.3 then fail this request.
        //

        if (CdIs8dot3Name( IrpContext,
                           *NameToUse )) {


            try_return( Status = STATUS_OBJECT_NAME_NOT_FOUND );
        }

        CdGenerate8dot3Name( IrpContext,
                             NameToUse,
                             DirentOffset,
                             ShortNameBuffer,
                             &ShortNameLength );

        //
        //  We now have the short name.  We have left it in Unicode form so copy it directly.
        //

        Buffer->FileNameLength = ShortNameLength;

        if (Buffer->FileNameLength + sizeof( ULONG ) > *Length) {

            Buffer->FileNameLength = *Length - sizeof( ULONG );
            Status = STATUS_BUFFER_OVERFLOW;
        }

        RtlCopyMemory( Buffer->FileName, ShortNameBuffer, Buffer->FileNameLength );

    try_exit:  NOTHING;
    } finally {

        if (CleanupFileLookup) {

            CdCleanupDirContext( IrpContext, &DirContext );
            CdCleanupDirent( IrpContext, &Dirent );

        } else if (CleanupDirectoryLookup) {

            CdCleanupCompoundPathEntry( IrpContext, &CompoundPathEntry );
            CdCleanupFileContext( IrpContext, &FileContext );
        }

        if (ReleaseParentFcb) {

            CdReleaseFile( IrpContext, ParentFcb );
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
CdQueryNetworkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

 Description:

    This routine performs the query network open information function for Cdfs

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
    //  We only support creation, last modify and last write times on Cdfs.
    //

    Buffer->LastWriteTime.QuadPart =
    Buffer->CreationTime.QuadPart =
    Buffer->ChangeTime.QuadPart = Fcb->CreationTime;

    Buffer->LastAccessTime.QuadPart = 0;

    Buffer->FileAttributes = Fcb->FileAttributes;

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

