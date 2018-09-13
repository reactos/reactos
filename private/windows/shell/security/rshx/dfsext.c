//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dfsext.c
//
//  Contents:   Code to see if a path refers to a Dfs path.
//
//  Classes:    None
//
//  Functions:  IsThisADfsPath
//
//  History:    March 11, 1996  Milans created
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <dfsfsctl.h>
#include <windows.h>

NTSTATUS
DfsFsctl(
    IN  HANDLE DfsHandle,
    IN  ULONG FsControlCode,
    IN  PVOID InputBuffer OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN  ULONG OutputBufferLength);

NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle);

//+----------------------------------------------------------------------------
//
//  Function:   IsThisADfsPath, public
//
//  Synopsis:   Given a fully qualified UNC or Drive based path, this routine
//              will identify if it is a Dfs path or not.
//
//  Arguments:  [pwszPath] -- The fully qualified path to test.
//
//              [cwPath] -- Length, in WCHARs, of pwszPath. If this is 0,
//                      this routine will compute the length. If it is
//                      non-zero, it will assume that the length of pwszPath
//                      is cwPath WCHARs.
//
//  Returns:    TRUE if pwszPath is a Dfs path, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOL
IsThisADfsPath(
    IN LPCWSTR pwszPath,
    IN DWORD cwPath OPTIONAL)
{
    NTSTATUS Status;
    HANDLE hDfs;
    BOOL fIsDfsPath = FALSE;

    //
    // We only accept UNC or drive letter paths
    //

    if (pwszPath == NULL)
        return( FALSE );

    if (cwPath == 0)
        cwPath = wcslen( pwszPath );

    if (cwPath < 2)
        return( FALSE );

    Status = DfsOpen( &hDfs );

    if (!NT_SUCCESS(Status))
        return( FALSE );

    //
    // From this point on, we must remember to close hDfs before returning.
    //

    if (pwszPath[0] == L'\\' && pwszPath[1] == L'\\') {

        Status = DfsFsctl(
                    hDfs,
                    FSCTL_DFS_IS_VALID_PREFIX,
                    (PVOID) &pwszPath[1],
                    (cwPath - 1) * sizeof(WCHAR),
                    NULL,
                    0);

        if (NT_SUCCESS(Status))
            fIsDfsPath = TRUE;

    } else if (pwszPath[1] == L':') {

        //
        // This is a drive based name. We'll fsctl to the driver to return
        // the prefix for this drive, if it is indeed a Dfs drive.
        //

        Status = DfsFsctl(
                    hDfs,
                    FSCTL_DFS_IS_VALID_LOGICAL_ROOT,
                    (PVOID) &pwszPath[0],
                    sizeof(WCHAR),
                    NULL,
                    0);

        if (NT_SUCCESS(Status))
            fIsDfsPath = TRUE;

    }

    NtClose( hDfs );

    return( fIsDfsPath );

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsOpen, private
//
//  Synopsis:   Opens a handle to the Dfs driver for fsctl purposes.
//
//  Arguments:  [DfsHandle] -- On successful return, contains handle to the
//                      driver.
//
//  Returns:    NTSTATUS of attempt to open the Dfs driver.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING name = {
        sizeof(DFS_DRIVER_NAME)-sizeof(UNICODE_NULL),
        sizeof(DFS_DRIVER_NAME)-sizeof(UNICODE_NULL),
        DFS_DRIVER_NAME};

    InitializeObjectAttributes(
        &objectAttributes,
        &name,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    status = NtCreateFile(
        DfsHandle,
        SYNCHRONIZE,
        &objectAttributes,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (NT_SUCCESS(status))
        status = ioStatus.Status;

    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctl, public
//
//  Synopsis:   Fsctl's to the Dfs driver.
//
//  Arguments:  [DfsHandle] -- Handle to the Dfs driver, usually obtained by
//                      calling DfsOpen.
//              [FsControlCode] -- The FSCTL code (see private\inc\dfsfsctl.h)
//              [InputBuffer] -- InputBuffer to the fsctl.
//              [InputBufferLength] -- Length, in BYTES, of InputBuffer
//              [OutputBuffer] -- OutputBuffer to the fsctl.
//              [OutputBufferLength] -- Length, in BYTES, of OutputBuffer
//
//  Returns:    NTSTATUS of Fsctl attempt.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsFsctl(
    IN  HANDLE DfsHandle,
    IN  ULONG FsControlCode,
    IN  PVOID InputBuffer OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN  ULONG OutputBufferLength
)
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    status = NtFsControlFile(
        DfsHandle,
        NULL,       // Event,
        NULL,       // ApcRoutine,
        NULL,       // ApcContext,
        &ioStatus,
        FsControlCode,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength
    );

    if(NT_SUCCESS(status))
        status = ioStatus.Status;

    return status;
}


