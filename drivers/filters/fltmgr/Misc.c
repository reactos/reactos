/*
 * PROJECT:         Filesystem Filter Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filters/fltmgr/Misc.c
 * PURPOSE:         Uncataloged functions
 * PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/




/* EXPORTED FUNCTIONS ******************************************************/

NTSTATUS
FLTAPI
FltBuildDefaultSecurityDescriptor(
    _Outptr_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ ACCESS_MASK DesiredAccess
)
{
    UNREFERENCED_PARAMETER(DesiredAccess);
    *SecurityDescriptor = NULL;
    return 0;
}

VOID
FLTAPI
FltFreeSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
)
{
    UNREFERENCED_PARAMETER(SecurityDescriptor);
}

NTSTATUS
FLTAPI
FltGetDiskDeviceObject(
    _In_ PFLT_VOLUME Volume,
    _Outptr_ PDEVICE_OBJECT *DiskDeviceObject
)
{
    UNREFERENCED_PARAMETER(Volume);
    UNREFERENCED_PARAMETER(DiskDeviceObject);
    return 0;
}

NTSTATUS
FLTAPI
FltGetFileNameInformationUnsafe(
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_INSTANCE Instance,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Outptr_ PFLT_FILE_NAME_INFORMATION *FileNameInformation)
{
    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(NameOptions);
    *FileNameInformation = NULL;
    return 0;
}

NTSTATUS
FLTAPI
FltGetFileNameInformation(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Outptr_ PFLT_FILE_NAME_INFORMATION *FileNameInformation)
{
    UNREFERENCED_PARAMETER(CallbackData);
    UNREFERENCED_PARAMETER(NameOptions);
    *FileNameInformation = NULL;
    return 0;
}

NTSTATUS
FLTAPI
FltGetDestinationFileNameInformation(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ HANDLE RootDirectory,
    _In_reads_bytes_(FileNameLength) PWSTR FileName,
    _In_ ULONG FileNameLength,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Outptr_ PFLT_FILE_NAME_INFORMATION *RetFileNameInformation)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(RootDirectory);
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(FileNameLength);
    UNREFERENCED_PARAMETER(NameOptions);
    *RetFileNameInformation = NULL;
    return 0;
}

VOID
FLTAPI
FltReleaseFileNameInformation(
    _In_ PFLT_FILE_NAME_INFORMATION FileNameInformation)
{
    UNREFERENCED_PARAMETER(FileNameInformation);
}
