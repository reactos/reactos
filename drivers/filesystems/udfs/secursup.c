////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: SecurSup.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "Get/Set Security" dispatch entry points.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_SECURITY

NTSTATUS
UDFCheckAccessRights(
    PFILE_OBJECT FileObject, // OPTIONAL
    PACCESS_STATE AccessState,
    PFCB Fcb,
    PCCB Ccb,        // OPTIONAL
    ACCESS_MASK DesiredAccess,
    USHORT ShareAccess
    )
{
    NTSTATUS RC;
    BOOLEAN ROCheck = FALSE;

    // Check attr compatibility
    ASSERT(Fcb);
    ASSERT(Fcb->Vcb);

    if (Fcb->FcbState & UDF_FCB_READ_ONLY) {
        ROCheck = TRUE;
    } else
    if ((Fcb->Vcb->origIntegrityType == INTEGRITY_TYPE_OPEN) &&
        Fcb != Fcb->Vcb->VolumeDasdFcb && //TODO: TypeOfOpen != UserVolumeOpen
        (Fcb->Vcb->CompatFlags & UDF_VCB_IC_DIRTY_RO)) {

        AdPrint(("force R/O on dirty\n"));
        ROCheck = TRUE;
    } if (ROCheck) {

        //
        //  Check the desired access for a read-only dirent.  AccessMask will contain
        //  the flags we're going to allow.
        //

        ACCESS_MASK AccessMask = DELETE | READ_CONTROL | WRITE_OWNER | WRITE_DAC |
                            SYNCHRONIZE | ACCESS_SYSTEM_SECURITY | FILE_READ_DATA |
                            FILE_READ_EA | FILE_WRITE_EA | FILE_READ_ATTRIBUTES |
                            FILE_WRITE_ATTRIBUTES | FILE_EXECUTE | FILE_LIST_DIRECTORY |
                            FILE_TRAVERSE;

        //
        //  If this is a subdirectory also allow add file/directory and delete.
        //

        if (FlagOn(Fcb->FcbState, UDF_FCB_DIRECTORY)) {

            AccessMask |= FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE | FILE_DELETE_CHILD;
        }

        if (FlagOn(DesiredAccess, ~AccessMask)) {

            AdPrint(("Cannot open readonly\n"));

            return STATUS_ACCESS_DENIED;
        }
    }

    if (FileObject) {
        if (Fcb->FcbCleanup) {
            // The FCB is currently in use by some thread.
            // We must check whether the requested access/share access
            // conflicts with the existing open operations.
            // Use Update=FALSE (check only, don't update ShareAccess).
            // IoSetShareAccess/IoUpdateShareAccess in UDFCompleteFcbOpen
            // handles the actual update AFTER all checks pass.
            RC = IoCheckShareAccess(DesiredAccess, ShareAccess, FileObject,
                                            &Fcb->ShareAccess, FALSE);
            if (!NT_SUCCESS(RC))
                UDFPrint(("UDFS share: %wZ refused, access %lx share %lx, "
                          "open %lu readers %lu writers %lu deleters %lu "
                          "shared r/w/d %lu/%lu/%lu\n",
                          &FileObject->FileName, DesiredAccess, (ULONG)ShareAccess,
                          Fcb->ShareAccess.OpenCount, Fcb->ShareAccess.Readers,
                          Fcb->ShareAccess.Writers, Fcb->ShareAccess.Deleters,
                          Fcb->ShareAccess.SharedRead, Fcb->ShareAccess.SharedWrite,
                          Fcb->ShareAccess.SharedDelete));
        } else {

            RC = STATUS_SUCCESS;
        }
    } else {
        // we get here if given file was opened for internal purposes
        RC = STATUS_SUCCESS;
    }
    return RC;
} // end UDFCheckAccessRights()

NTSTATUS
UDFSetAccessRights(
    PFILE_OBJECT FileObject,
    PACCESS_STATE AccessState,
    PFCB Fcb,
    PCCB         Ccb,
    ACCESS_MASK  DesiredAccess,
    USHORT       ShareAccess
    )
{
    ASSERT(Ccb);
    ASSERT(Fcb->FileInfo);

    return UDFCheckAccessRights(FileObject, AccessState, Fcb, Ccb, DesiredAccess, ShareAccess);

} // end UDFSetAccessRights()

