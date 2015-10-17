/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             Debug.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES **************************************************************/

#include "stdarg.h"
#include "ext2fs.h"

/* GLOBALS ***************************************************************/

#if EXT2_DEBUG

#define SYSTEM_PROCESS_NAME "System"

extern PEXT2_GLOBAL Ext2Global;

ULONG   DebugFilter = DL_DEFAULT;

ULONG  ProcessNameOffset = 0;

/* DEFINITIONS ***********************************************************/


/* Static Definitions ****************************************************/

static PUCHAR IrpMjStrings[] = {
    "IRP_MJ_CREATE",
    "IRP_MJ_CREATE_NAMED_PIPE",
    "IRP_MJ_CLOSE",
    "IRP_MJ_READ",
    "IRP_MJ_WRITE",
    "IRP_MJ_QUERY_INFORMATION",
    "IRP_MJ_SET_INFORMATION",
    "IRP_MJ_QUERY_EA",
    "IRP_MJ_SET_EA",
    "IRP_MJ_FLUSH_BUFFERS",
    "IRP_MJ_QUERY_VOLUME_INFORMATION",
    "IRP_MJ_SET_VOLUME_INFORMATION",
    "IRP_MJ_DIRECTORY_CONTROL",
    "IRP_MJ_FILE_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CONTROL",
    "IRP_MJ_INTERNAL_DEVICE_CONTROL",
    "IRP_MJ_SHUTDOWN",
    "IRP_MJ_LOCK_CONTROL",
    "IRP_MJ_CLEANUP",
    "IRP_MJ_CREATE_MAILSLOT",
    "IRP_MJ_QUERY_SECURITY",
    "IRP_MJ_SET_SECURITY",
    "IRP_MJ_POWER",
    "IRP_MJ_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CHANGE",
    "IRP_MJ_QUERY_QUOTA",
    "IRP_MJ_SET_QUOTA",
    "IRP_MJ_PNP"
};

static PUCHAR FileInformationClassStrings[] = {
    "Unknown FileInformationClass 0",
    "FileDirectoryInformation",
    "FileFullDirectoryInformation",
    "FileBothDirectoryInformation",
    "FileBasicInformation",
    "FileStandardInformation",
    "FileInternalInformation",
    "FileEaInformation",
    "FileAccessInformation",
    "FileNameInformation",
    "FileRenameInformation",
    "FileLinkInformation",
    "FileNamesInformation",
    "FileDispositionInformation",
    "FilePositionInformation",
    "FileFullEaInformation",
    "FileModeInformation",
    "FileAlignmentInformation",
    "FileAllInformation",
    "FileAllocationInformation",
    "FileEndOfFileInformation",
    "FileAlternateNameInformation",
    "FileStreamInformation",
    "FilePipeInformation",
    "FilePipeLocalInformation",
    "FilePipeRemoteInformation",
    "FileMailslotQueryInformation",
    "FileMailslotSetInformation",
    "FileCompressionInformation",
    "FileObjectIdInformation",
    "FileCompletionInformation",
    "FileMoveClusterInformation",
    "FileQuotaInformation",
    "FileReparsePointInformation",
    "FileNetworkOpenInformation",
    "FileAttributeTagInformation",
    "FileTrackingInformation"
};

static PUCHAR FsInformationClassStrings[] = {
    "Unknown FsInformationClass 0",
    "FileFsVolumeInformation",
    "FileFsLabelInformation",
    "FileFsSizeInformation",
    "FileFsDeviceInformation",
    "FileFsAttributeInformation",
    "FileFsControlInformation",
    "FileFsFullSizeInformation",
    "FileFsObjectIdInformation"
};

/*
 * Ext2Printf
 *   This function is variable-argument, level-sensitive debug print routine.
 *   If the specified debug level for the print statement is lower or equal
 *   to the current debug level, the message will be printed.
 *
 * Arguments:
 *   DebugMessage - Variable argument ascii c string
 *
 * Return Value:
 *   N/A
 *
 * NOTES:
 *   N/A
 */

#define DBG_BUF_LEN 0x100
VOID
Ext2Printf(
    PCHAR DebugMessage,
    ...
)
{
    va_list             ap;
    LARGE_INTEGER       CurrentTime;
    TIME_FIELDS         TimeFields;
    CHAR                Buffer[DBG_BUF_LEN];
    ULONG               i;

    RtlZeroMemory(Buffer, DBG_BUF_LEN);
    va_start(ap, DebugMessage);

    KeQuerySystemTime( &CurrentTime);
    RtlTimeToTimeFields(&CurrentTime, &TimeFields);
    _vsnprintf(&Buffer[0], DBG_BUF_LEN, DebugMessage, ap);

    DbgPrint(DRIVER_NAME":~%d: %2.2d:%2.2d:%2.2d:%3.3d %8.8x:   %s",
             KeGetCurrentProcessorNumber(),
             TimeFields.Hour, TimeFields.Minute,
             TimeFields.Second, TimeFields.Milliseconds,
             PsGetCurrentThread(), Buffer);

    va_end(ap);
}

VOID
Ext2NiPrintf(
    PCHAR DebugMessage,
    ...
)
{
    va_list             ap;
    LARGE_INTEGER       CurrentTime;
    TIME_FIELDS         TimeFields;
    CHAR                Buffer[0x100];
    ULONG               i;

    va_start(ap, DebugMessage);

    KeQuerySystemTime( &CurrentTime);
    RtlTimeToTimeFields(&CurrentTime, &TimeFields);
    _vsnprintf(&Buffer[0], 0x100, DebugMessage, ap);

    DbgPrint(DRIVER_NAME":~%d: %2.2d:%2.2d:%2.2d:%3.3d %8.8x: %s",
             KeGetCurrentProcessorNumber(),
             TimeFields.Hour, TimeFields.Minute,
             TimeFields.Second, TimeFields.Milliseconds,
             PsGetCurrentThread(), Buffer);

    va_end(ap);

} // Ext2NiPrintf()

ULONG
Ext2GetProcessNameOffset ( VOID )
{
    PEPROCESS   Process;
    ULONG       i;

    Process = PsGetCurrentProcess();

    for (i = 0; i < PAGE_SIZE; i++) {
        if (!strncmp(
                    SYSTEM_PROCESS_NAME,
                    (PCHAR) Process + i,
                    strlen(SYSTEM_PROCESS_NAME)
                )) {

            return i;
        }
    }

    DEBUG(DL_ERR, ( ": *** FsdGetProcessNameOffset failed ***\n"));

    return 0;
}


VOID
Ext2DbgPrintCall (IN PDEVICE_OBJECT   DeviceObject,
                  IN PIRP             Irp )
{
    PIO_STACK_LOCATION      IoStackLocation;
    PFILE_OBJECT            FileObject;
    PWCHAR                  FileName;
    PEXT2_FCB               Fcb;
    FILE_INFORMATION_CLASS  FileInformationClass;
    FS_INFORMATION_CLASS    FsInformationClass;

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    FileObject = IoStackLocation->FileObject;

    FileName = L"Unknown";

    if (DeviceObject == Ext2Global->DiskdevObject) {

        FileName = DEVICE_NAME;

    } else if (DeviceObject == Ext2Global->CdromdevObject) {

        FileName = CDROM_NAME;

    } else if (FileObject && FileObject->FsContext) {

        Fcb = (PEXT2_FCB) FileObject->FsContext;

        if (Fcb->Identifier.Type == EXT2VCB)  {
            FileName = L"\\Volume";
        } else if (Fcb->Identifier.Type == EXT2FCB && Fcb->Mcb->FullName.Buffer) {
            FileName = Fcb->Mcb->FullName.Buffer;
        }
    }

    switch (IoStackLocation->MajorFunction) {

    case IRP_MJ_CREATE:

        FileName = NULL;

        if (DeviceObject == Ext2Global->DiskdevObject) {
            FileName = DEVICE_NAME;
        } else if (DeviceObject == Ext2Global->CdromdevObject) {
            FileName = CDROM_NAME;
        } else if (IoStackLocation->FileObject->FileName.Length == 0) {
            FileName = L"\\Volume";
        }

        if (FileName) {
            DEBUGNI(DL_FUN, ("%s %s %S\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName
                            ));
        } else if (IoStackLocation->FileObject->FileName.Buffer) {
            DEBUGNI(DL_FUN, ("%s %s %S\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             IoStackLocation->FileObject->FileName.Buffer
                            ));
        } else {
            DEBUGNI(DL_FUN, ("%s %s %s\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             "Unknown"
                            ));
        }

        break;

    case IRP_MJ_CLOSE:

        DEBUGNI(DL_FUN, ("%s %s %S\n",
                         Ext2GetCurrentProcessName(),
                         IrpMjStrings[IoStackLocation->MajorFunction],
                         FileName
                        ));

        break;

    case IRP_MJ_READ:

        if (IoStackLocation->MinorFunction & IRP_MN_COMPLETE) {
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_COMPLETE\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName
                            ));
        } else {
            DEBUGNI(DL_FUN, ("%s %s %S Offset: %I64xh Length: %xh %s%s%s%s%s%s\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->Parameters.Read.ByteOffset.QuadPart,
                             IoStackLocation->Parameters.Read.Length,
                             (IoStackLocation->MinorFunction & IRP_MN_DPC ? "IRP_MN_DPC " : " "),
                             (IoStackLocation->MinorFunction & IRP_MN_MDL ? "IRP_MN_MDL " : " "),
                             (IoStackLocation->MinorFunction & IRP_MN_COMPRESSED ? "IRP_MN_COMPRESSED " : " "),
                             (Irp->Flags & IRP_PAGING_IO ? "IRP_PAGING_IO " : " "),
                             (Irp->Flags & IRP_NOCACHE ? "IRP_NOCACHE " : " "),
                             (FileObject->Flags & FO_SYNCHRONOUS_IO ? "FO_SYNCHRONOUS_IO " : " ")
                            ));
        }

        break;

    case IRP_MJ_WRITE:

        if (IoStackLocation->MinorFunction & IRP_MN_COMPLETE)  {
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_COMPLETE\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName
                            ));
        } else {
            DEBUGNI(DL_FUN, ("%s %s %S Offset: %I64xh Length: %xh %s%s%s%s%s%s\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->Parameters.Read.ByteOffset.QuadPart,
                             IoStackLocation->Parameters.Read.Length,
                             (IoStackLocation->MinorFunction & IRP_MN_DPC ? "IRP_MN_DPC " : " "),
                             (IoStackLocation->MinorFunction & IRP_MN_MDL ? "IRP_MN_MDL " : " "),
                             (IoStackLocation->MinorFunction & IRP_MN_COMPRESSED ? "IRP_MN_COMPRESSED " : " "),
                             (Irp->Flags & IRP_PAGING_IO ? "IRP_PAGING_IO " : " "),
                             (Irp->Flags & IRP_NOCACHE ? "IRP_NOCACHE " : " "),
                             (FileObject->Flags & FO_SYNCHRONOUS_IO ? "FO_SYNCHRONOUS_IO " : " ")
                            ));
        }

        break;

    case IRP_MJ_QUERY_INFORMATION:

        FileInformationClass =
            IoStackLocation->Parameters.QueryFile.FileInformationClass;

        if (FileInformationClass <= FileMaximumInformation) {
            DEBUGNI(DL_FUN, ("%s %s %S %s\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             FileInformationClassStrings[FileInformationClass]
                            ));
        } else {
            DEBUGNI(DL_FUN, ("%s %s %S Unknown FileInformationClass %u\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             FileInformationClass
                            ));
        }

        break;

    case IRP_MJ_SET_INFORMATION:

        FileInformationClass =
            IoStackLocation->Parameters.SetFile.FileInformationClass;

        if (FileInformationClass <= FileMaximumInformation) {
            DEBUGNI(DL_FUN, ("%s %s %S %s\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             FileInformationClassStrings[FileInformationClass]
                            ));
        } else {
            DEBUGNI(DL_FUN, ("%s %s %S Unknown FileInformationClass %u\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             FileInformationClass
                            ));
        }

        break;

    case IRP_MJ_QUERY_VOLUME_INFORMATION:

        FsInformationClass =
            IoStackLocation->Parameters.QueryVolume.FsInformationClass;

        if (FsInformationClass <= FileFsMaximumInformation) {
            DEBUGNI(DL_FUN, ("%s %s %S %s\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             FsInformationClassStrings[FsInformationClass]
                            ));
        } else {
            DEBUGNI(DL_FUN, ("%s %s %S Unknown FsInformationClass %u\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             FsInformationClass
                            ));
        }

        break;

    case IRP_MJ_DIRECTORY_CONTROL:

        if (IoStackLocation->MinorFunction & IRP_MN_QUERY_DIRECTORY) {

#ifndef _GNU_NTIFS_
            FileInformationClass =
                IoStackLocation->Parameters.QueryDirectory.FileInformationClass;
#else
            FileInformationClass = ((PEXTENDED_IO_STACK_LOCATION)
                                    IoStackLocation)->Parameters.QueryDirectory.FileInformationClass;
#endif

            if (FileInformationClass <= FileMaximumInformation) {
                DEBUGNI(DL_FUN, ("%s %s %S %s\n",
                                 Ext2GetCurrentProcessName(),
                                 IrpMjStrings[IoStackLocation->MajorFunction],
                                 FileName,
                                 FileInformationClassStrings[FileInformationClass]
                                ));

                if (
#ifndef _GNU_NTIFS_
                    IoStackLocation->Parameters.QueryDirectory.FileName
#else
                    ((PEXTENDED_IO_STACK_LOCATION)
                     IoStackLocation)->Parameters.QueryDirectory.FileName
#endif
                ) {
#ifndef _GNU_NTIFS_
                    DEBUGNI(DL_FUN, ("%s FileName: %.*S FileIndex: %x %s%s%s\n",
                                     Ext2GetCurrentProcessName(),

                                     IoStackLocation->Parameters.QueryDirectory.FileName->Length / 2,
                                     IoStackLocation->Parameters.QueryDirectory.FileName->Buffer,
                                     IoStackLocation->Parameters.QueryDirectory.FileIndex,
                                     (IoStackLocation->Flags & SL_RESTART_SCAN ? "SL_RESTART_SCAN " : ""),
                                     (IoStackLocation->Flags & SL_RETURN_SINGLE_ENTRY ? "SL_RETURN_SINGLE_ENTRY " : ""),
                                     ((IoStackLocation->Flags & SL_INDEX_SPECIFIED) ? "SL_INDEX_SPECIFIED " : "")
                                    ));

#else
                    DEBUGNI(DL_FUN, ("%s FileName: %.*S FileIndex: %x %s%s%s\n",
                                     Ext2GetCurrentProcessName(),

                                     ((PEXTENDED_IO_STACK_LOCATION)
                                      IoStackLocation)->Parameters.QueryDirectory.FileName->Length / 2,
                                     ((PEXTENDED_IO_STACK_LOCATION)
                                      IoStackLocation)->Parameters.QueryDirectory.FileName->Buffer,
                                     ((PEXTENDED_IO_STACK_LOCATION)
                                      IoStackLocation)->Parameters.QueryDirectory.FileIndex,
                                     (IoStackLocation->Flags & SL_RESTART_SCAN ? "SL_RESTART_SCAN " : ""),
                                     (IoStackLocation->Flags & SL_RETURN_SINGLE_ENTRY ? "SL_RETURN_SINGLE_ENTRY " : ""),
                                     ((IoStackLocation->Flags & SL_INDEX_SPECIFIED) ? "SL_INDEX_SPECIFIED " : "")
                                    ));
#endif
                } else {
#ifndef _GNU_NTIFS_
                    DEBUGNI(DL_FUN, ("%s FileName: FileIndex: %#x %s%s%s\n",
                                     Ext2GetCurrentProcessName(),
                                     IoStackLocation->Parameters.QueryDirectory.FileIndex,
                                     (IoStackLocation->Flags & SL_RESTART_SCAN ? "SL_RESTART_SCAN " : ""),
                                     (IoStackLocation->Flags & SL_RETURN_SINGLE_ENTRY ? "SL_RETURN_SINGLE_ENTRY " : ""),
                                     (IoStackLocation->Flags & SL_INDEX_SPECIFIED ? "SL_INDEX_SPECIFIED " : "")
                                    ));
#else
                    DEBUGNI(DL_FUN, ("%s FileName: FileIndex: %#x %s%s%s\n",
                                     Ext2GetCurrentProcessName(),
                                     ((PEXTENDED_IO_STACK_LOCATION)
                                      IoStackLocation)->Parameters.QueryDirectory.FileIndex,
                                     (IoStackLocation->Flags & SL_RESTART_SCAN ? "SL_RESTART_SCAN " : ""),
                                     (IoStackLocation->Flags & SL_RETURN_SINGLE_ENTRY ? "SL_RETURN_SINGLE_ENTRY " : ""),
                                     (IoStackLocation->Flags & SL_INDEX_SPECIFIED ? "SL_INDEX_SPECIFIED " : "")
                                    ));
#endif
                }
            } else {
                DEBUGNI(DL_FUN, ("%s %s %S Unknown FileInformationClass %u\n",
                                 Ext2GetCurrentProcessName(),
                                 IrpMjStrings[IoStackLocation->MajorFunction],
                                 FileName,
                                 FileInformationClass
                                ));
            }
        } else if (IoStackLocation->MinorFunction & IRP_MN_NOTIFY_CHANGE_DIRECTORY) {
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_NOTIFY_CHANGE_DIRECTORY\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName
                            ));
        } else {
            DEBUGNI(DL_FUN, ("%s %s %S Unknown minor function %#x\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->MinorFunction
                            ));
        }

        break;

    case IRP_MJ_FILE_SYSTEM_CONTROL:

        if (IoStackLocation->MinorFunction == IRP_MN_USER_FS_REQUEST) {
#ifndef _GNU_NTIFS_
            DEBUGNI(DL_FUN, ( "%s %s %S IRP_MN_USER_FS_REQUEST FsControlCode: %#x\n",
                              Ext2GetCurrentProcessName(),
                              IrpMjStrings[IoStackLocation->MajorFunction],
                              FileName,
                              IoStackLocation->Parameters.FileSystemControl.FsControlCode
                            ));
#else
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_USER_FS_REQUEST FsControlCode: %#x\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             ((PEXTENDED_IO_STACK_LOCATION)
                              IoStackLocation)->Parameters.FileSystemControl.FsControlCode
                            ));
#endif
        } else if (IoStackLocation->MinorFunction == IRP_MN_MOUNT_VOLUME) {
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_MOUNT_VOLUME DeviceObject: %#x\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->Parameters.MountVolume.DeviceObject
                            ));
        } else if (IoStackLocation->MinorFunction == IRP_MN_VERIFY_VOLUME) {
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_VERIFY_VOLUME DeviceObject: %#x\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->Parameters.VerifyVolume.DeviceObject
                            ));
        } else if (IoStackLocation->MinorFunction == IRP_MN_LOAD_FILE_SYSTEM) {
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_LOAD_FILE_SYSTEM\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName
                            ));
        }
#if (_WIN32_WINNT >= 0x0500)
        else if (IoStackLocation->MinorFunction == IRP_MN_KERNEL_CALL) {
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_KERNEL_CALL\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName
                            ));
        }
#endif // (_WIN32_WINNT >= 0x0500)
        else {
            DEBUGNI(DL_FUN, ("%s %s %S Unknown minor function %#x\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->MinorFunction
                            ));
        }

        break;

    case IRP_MJ_DEVICE_CONTROL:

        DEBUGNI(DL_FUN, ("%s %s %S IoControlCode: %#x\n",
                         Ext2GetCurrentProcessName(),
                         IrpMjStrings[IoStackLocation->MajorFunction],
                         FileName,
                         IoStackLocation->Parameters.DeviceIoControl.IoControlCode
                        ));

        break;

    case IRP_MJ_LOCK_CONTROL:

        if (IoStackLocation->MinorFunction & IRP_MN_LOCK) {
#ifndef _GNU_NTIFS_
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_LOCK Offset: %I64xh Length: %I64xh Key: %u %s%s\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->Parameters.LockControl.ByteOffset.QuadPart,
                             IoStackLocation->Parameters.LockControl.Length->QuadPart,
                             IoStackLocation->Parameters.LockControl.Key,
                             (IoStackLocation->Flags & SL_FAIL_IMMEDIATELY ? "SL_FAIL_IMMEDIATELY " : ""),
                             (IoStackLocation->Flags & SL_EXCLUSIVE_LOCK ? "SL_EXCLUSIVE_LOCK " : "")
                            ));
#else
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_LOCK Offset: %I64xh Length: %I64xh Key: %u %s%s\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             ((PEXTENDED_IO_STACK_LOCATION)
                              IoStackLocation)->Parameters.LockControl.ByteOffset.QuadPart,
                             ((PEXTENDED_IO_STACK_LOCATION)
                              IoStackLocation)->Parameters.LockControl.Length->QuadPart,
                             ((PEXTENDED_IO_STACK_LOCATION)
                              IoStackLocation)->Parameters.LockControl.Key,
                             (IoStackLocation->Flags & SL_FAIL_IMMEDIATELY ? "SL_FAIL_IMMEDIATELY " : ""),
                             (IoStackLocation->Flags & SL_EXCLUSIVE_LOCK ? "SL_EXCLUSIVE_LOCK " : "")
                            ));
#endif
        } else if (IoStackLocation->MinorFunction & IRP_MN_UNLOCK_SINGLE) {
#ifndef _GNU_NTIFS_
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_UNLOCK_SINGLE Offset: %I64xh Length: %I64xh Key: %u\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->Parameters.LockControl.ByteOffset.QuadPart,
                             IoStackLocation->Parameters.LockControl.Length->QuadPart,
                             IoStackLocation->Parameters.LockControl.Key
                            ));
#else
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_UNLOCK_SINGLE Offset: %I64xh Length: %I64xh Key: %u\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             ((PEXTENDED_IO_STACK_LOCATION)
                              IoStackLocation)->Parameters.LockControl.ByteOffset.QuadPart,
                             ((PEXTENDED_IO_STACK_LOCATION)
                              IoStackLocation)->Parameters.LockControl.Length->QuadPart,
                             ((PEXTENDED_IO_STACK_LOCATION)
                              IoStackLocation)->Parameters.LockControl.Key
                            ));
#endif
        } else if (IoStackLocation->MinorFunction & IRP_MN_UNLOCK_ALL) {
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_UNLOCK_ALL\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName
                            ));
        } else if (IoStackLocation->MinorFunction & IRP_MN_UNLOCK_ALL_BY_KEY) {
#ifndef _GNU_NTIFS_
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_UNLOCK_ALL_BY_KEY Key: %u\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->Parameters.LockControl.Key
                            ));
#else
            DEBUGNI(DL_FUN, ("%s %s %S IRP_MN_UNLOCK_ALL_BY_KEY Key: %u\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             ((PEXTENDED_IO_STACK_LOCATION)
                              IoStackLocation)->Parameters.LockControl.Key
                            ));
#endif
        } else {
            DEBUGNI(DL_FUN, ("%s %s %S Unknown minor function %#x\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             FileName,
                             IoStackLocation->MinorFunction
                            ));
        }

        break;

    case IRP_MJ_CLEANUP:

        DEBUGNI(DL_FUN, ("%s %s %S\n",
                         Ext2GetCurrentProcessName(),
                         IrpMjStrings[IoStackLocation->MajorFunction],
                         FileName
                        ));

        break;

    case IRP_MJ_SHUTDOWN:

        DEBUGNI(DL_FUN, ("%s %s %S\n",
                         Ext2GetCurrentProcessName(),
                         IrpMjStrings[IoStackLocation->MajorFunction],
                         FileName
                        ));

        break;

#if (_WIN32_WINNT >= 0x0500)
    case IRP_MJ_PNP:

        DEBUGNI(DL_FUN, ( "%s %s %S\n",
                          Ext2GetCurrentProcessName(),
                          IrpMjStrings[IoStackLocation->MajorFunction],
                          FileName
                        ));
        break;
#endif // (_WIN32_WINNT >= 0x0500)

    default:

        DEBUGNI(DL_FUN, ("%s %s %S\n",
                         Ext2GetCurrentProcessName(),
                         IrpMjStrings[IoStackLocation->MajorFunction],
                         FileName
                        ));
    }
}

VOID
Ext2DbgPrintComplete (IN PIRP Irp, IN BOOLEAN bPrint)
{
    PIO_STACK_LOCATION IoStackLocation;

    if (!Irp)
        return;

    if (Irp->IoStatus.Status != STATUS_SUCCESS) {

        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

        if (bPrint) {
            DEBUGNI(DL_FUN, ("%s %s Status: %s (%#x).\n",
                             Ext2GetCurrentProcessName(),
                             IrpMjStrings[IoStackLocation->MajorFunction],
                             Ext2NtStatusToString(Irp->IoStatus.Status),
                             Irp->IoStatus.Status
                            ));
        }
    }
}

PUCHAR
Ext2NtStatusToString ( IN NTSTATUS Status )
{
    switch (Status) {

    case 0x00000000:
        return "STATUS_SUCCESS";
    case 0x00000001:
        return "STATUS_WAIT_1";
    case 0x00000002:
        return "STATUS_WAIT_2";
    case 0x00000003:
        return "STATUS_WAIT_3";
    case 0x0000003F:
        return "STATUS_WAIT_63";
    case 0x00000080:
        return "STATUS_ABANDONED_WAIT_0";
    case 0x000000BF:
        return "STATUS_ABANDONED_WAIT_63";
    case 0x000000C0:
        return "STATUS_USER_APC";
    case 0x00000100:
        return "STATUS_KERNEL_APC";
    case 0x00000101:
        return "STATUS_ALERTED";
    case 0x00000102:
        return "STATUS_TIMEOUT";
    case 0x00000103:
        return "STATUS_PENDING";
    case 0x00000104:
        return "STATUS_REPARSE";
    case 0x00000105:
        return "STATUS_MORE_ENTRIES";
    case 0x00000106:
        return "STATUS_NOT_ALL_ASSIGNED";
    case 0x00000107:
        return "STATUS_SOME_NOT_MAPPED";
    case 0x00000108:
        return "STATUS_OPLOCK_BREAK_IN_PROGRESS";
    case 0x00000109:
        return "STATUS_VOLUME_MOUNTED";
    case 0x0000010A:
        return "STATUS_RXACT_COMMITTED";
    case 0x0000010B:
        return "STATUS_NOTIFY_CLEANUP";
    case 0x0000010C:
        return "STATUS_NOTIFY_ENUM_DIR";
    case 0x0000010D:
        return "STATUS_NO_QUOTAS_FOR_ACCOUNT";
    case 0x0000010E:
        return "STATUS_PRIMARY_TRANSPORT_CONNECT_FAILED";
    case 0x00000110:
        return "STATUS_PAGE_FAULT_TRANSITION";
    case 0x00000111:
        return "STATUS_PAGE_FAULT_DEMAND_ZERO";
    case 0x00000112:
        return "STATUS_PAGE_FAULT_COPY_ON_WRITE";
    case 0x00000113:
        return "STATUS_PAGE_FAULT_GUARD_PAGE";
    case 0x00000114:
        return "STATUS_PAGE_FAULT_PAGING_FILE";
    case 0x00000115:
        return "STATUS_CACHE_PAGE_LOCKED";
    case 0x00000116:
        return "STATUS_CRASH_DUMP";
    case 0x00000117:
        return "STATUS_BUFFER_ALL_ZEROS";
    case 0x00000118:
        return "STATUS_REPARSE_OBJECT";
    case 0x00000119:
        return "STATUS_RESOURCE_REQUIREMENTS_CHANGED";
    case 0x00000120:
        return "STATUS_TRANSLATION_COMPLETE";
    case 0x00000121:
        return "STATUS_DS_MEMBERSHIP_EVALUATED_LOCALLY";
    case 0x00010001:
        return "DBG_EXCEPTION_HANDLED";
    case 0x00010002:
        return "DBG_CONTINUE";
    case 0x40000000:
        return "STATUS_OBJECT_NAME_EXISTS";
    case 0x40000001:
        return "STATUS_THREAD_WAS_SUSPENDED";
    case 0x40000002:
        return "STATUS_WORKING_SET_LIMIT_RANGE";
    case 0x40000003:
        return "STATUS_IMAGE_NOT_AT_BASE";
    case 0x40000004:
        return "STATUS_RXACT_STATE_CREATED";
    case 0x40000005:
        return "STATUS_SEGMENT_NOTIFICATION";
    case 0x40000006:
        return "STATUS_LOCAL_USER_SESSION_KEY";
    case 0x40000007:
        return "STATUS_BAD_CURRENT_DIRECTORY";
    case 0x40000008:
        return "STATUS_SERIAL_MORE_WRITES";
    case 0x40000009:
        return "STATUS_REGISTRY_RECOVERED";
    case 0x4000000A:
        return "STATUS_FT_READ_RECOVERY_FROM_BACKUP";
    case 0x4000000B:
        return "STATUS_FT_WRITE_RECOVERY";
    case 0x4000000C:
        return "STATUS_SERIAL_COUNTER_TIMEOUT";
    case 0x4000000D:
        return "STATUS_NULL_LM_PASSWORD";
    case 0x4000000E:
        return "STATUS_IMAGE_MACHINE_TYPE_MISMATCH";
    case 0x4000000F:
        return "STATUS_RECEIVE_PARTIAL";
    case 0x40000010:
        return "STATUS_RECEIVE_EXPEDITED";
    case 0x40000011:
        return "STATUS_RECEIVE_PARTIAL_EXPEDITED";
    case 0x40000012:
        return "STATUS_EVENT_DONE";
    case 0x40000013:
        return "STATUS_EVENT_PENDING";
    case 0x40000014:
        return "STATUS_CHECKING_FILE_SYSTEM";
    case 0x40000015:
        return "STATUS_FATAL_APP_EXIT";
    case 0x40000016:
        return "STATUS_PREDEFINED_HANDLE";
    case 0x40000017:
        return "STATUS_WAS_UNLOCKED";
    case 0x40000018:
        return "STATUS_SERVICE_NOTIFICATION";
    case 0x40000019:
        return "STATUS_WAS_LOCKED";
    case 0x4000001A:
        return "STATUS_LOG_HARD_ERROR";
    case 0x4000001B:
        return "STATUS_ALREADY_WIN32";
    case 0x4000001C:
        return "STATUS_WX86_UNSIMULATE";
    case 0x4000001D:
        return "STATUS_WX86_CONTINUE";
    case 0x4000001E:
        return "STATUS_WX86_SINGLE_STEP";
    case 0x4000001F:
        return "STATUS_WX86_BREAKPOINT";
    case 0x40000020:
        return "STATUS_WX86_EXCEPTION_CONTINUE";
    case 0x40000021:
        return "STATUS_WX86_EXCEPTION_LASTCHANCE";
    case 0x40000022:
        return "STATUS_WX86_EXCEPTION_CHAIN";
    case 0x40000023:
        return "STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE";
    case 0x40000024:
        return "STATUS_NO_YIELD_PERFORMED";
    case 0x40000025:
        return "STATUS_TIMER_RESUME_IGNORED";
    case 0x40000026:
        return "STATUS_ARBITRATION_UNHANDLED";
    case 0x40000027:
        return "STATUS_CARDBUS_NOT_SUPPORTED";
    case 0x40000028:
        return "STATUS_WX86_CREATEWX86TIB";
    case 0x40000029:
        return "STATUS_MP_PROCESSOR_MISMATCH";
    case 0x40010001:
        return "DBG_REPLY_LATER";
    case 0x40010002:
        return "DBG_UNABLE_TO_PROVIDE_HANDLE";
    case 0x40010003:
        return "DBG_TERMINATE_THREAD";
    case 0x40010004:
        return "DBG_TERMINATE_PROCESS";
    case 0x40010005:
        return "DBG_CONTROL_C";
    case 0x40010006:
        return "DBG_PRINTEXCEPTION_C";
    case 0x40010007:
        return "DBG_RIPEXCEPTION";
    case 0x40010008:
        return "DBG_CONTROL_BREAK";
    case 0x80000001:
        return "STATUS_GUARD_PAGE_VIOLATION";
    case 0x80000002:
        return "STATUS_DATATYPE_MISALIGNMENT";
    case 0x80000003:
        return "STATUS_BREAKPOINT";
    case 0x80000004:
        return "STATUS_SINGLE_STEP";
    case 0x80000005:
        return "STATUS_BUFFER_OVERFLOW";
    case 0x80000006:
        return "STATUS_NO_MORE_FILES";
    case 0x80000007:
        return "STATUS_WAKE_SYSTEM_DEBUGGER";
    case 0x8000000A:
        return "STATUS_HANDLES_CLOSED";
    case 0x8000000B:
        return "STATUS_NO_INHERITANCE";
    case 0x8000000C:
        return "STATUS_GUID_SUBSTITUTION_MADE";
    case 0x8000000D:
        return "STATUS_PARTIAL_COPY";
    case 0x8000000E:
        return "STATUS_DEVICE_PAPER_EMPTY";
    case 0x8000000F:
        return "STATUS_DEVICE_POWERED_OFF";
    case 0x80000010:
        return "STATUS_DEVICE_OFF_LINE";
    case 0x80000011:
        return "STATUS_DEVICE_BUSY";
    case 0x80000012:
        return "STATUS_NO_MORE_EAS";
    case 0x80000013:
        return "STATUS_INVALID_EA_NAME";
    case 0x80000014:
        return "STATUS_EA_LIST_INCONSISTENT";
    case 0x80000015:
        return "STATUS_INVALID_EA_FLAG";
    case 0x80000016:
        return "STATUS_VERIFY_REQUIRED";
    case 0x80000017:
        return "STATUS_EXTRANEOUS_INFORMATION";
    case 0x80000018:
        return "STATUS_RXACT_COMMIT_NECESSARY";
    case 0x8000001A:
        return "STATUS_NO_MORE_ENTRIES";
    case 0x8000001B:
        return "STATUS_FILEMARK_DETECTED";
    case 0x8000001C:
        return "STATUS_MEDIA_CHANGED";
    case 0x8000001D:
        return "STATUS_BUS_RESET";
    case 0x8000001E:
        return "STATUS_END_OF_MEDIA";
    case 0x8000001F:
        return "STATUS_BEGINNING_OF_MEDIA";
    case 0x80000020:
        return "STATUS_MEDIA_CHECK";
    case 0x80000021:
        return "STATUS_SETMARK_DETECTED";
    case 0x80000022:
        return "STATUS_NO_DATA_DETECTED";
    case 0x80000023:
        return "STATUS_REDIRECTOR_HAS_OPEN_HANDLES";
    case 0x80000024:
        return "STATUS_SERVER_HAS_OPEN_HANDLES";
    case 0x80000025:
        return "STATUS_ALREADY_DISCONNECTED";
    case 0x80000026:
        return "STATUS_LONGJUMP";
    case 0x80010001:
        return "DBG_EXCEPTION_NOT_HANDLED";
    case 0xC0000001:
        return "STATUS_UNSUCCESSFUL";
    case 0xC0000002:
        return "STATUS_NOT_IMPLEMENTED";
    case 0xC0000003:
        return "STATUS_INVALID_INFO_CLASS";
    case 0xC0000004:
        return "STATUS_INFO_LENGTH_MISMATCH";
    case 0xC0000005:
        return "STATUS_ACCESS_VIOLATION";
    case 0xC0000006:
        return "STATUS_IN_PAGE_ERROR";
    case 0xC0000007:
        return "STATUS_PAGEFILE_QUOTA";
    case 0xC0000008:
        return "STATUS_INVALID_HANDLE";
    case 0xC0000009:
        return "STATUS_BAD_INITIAL_STACK";
    case 0xC000000A:
        return "STATUS_BAD_INITIAL_PC";
    case 0xC000000B:
        return "STATUS_INVALID_CID";
    case 0xC000000C:
        return "STATUS_TIMER_NOT_CANCELED";
    case 0xC000000D:
        return "STATUS_INVALID_PARAMETER";
    case 0xC000000E:
        return "STATUS_NO_SUCH_DEVICE";
    case 0xC000000F:
        return "STATUS_NO_SUCH_FILE";
    case 0xC0000010:
        return "STATUS_INVALID_DEVICE_REQUEST";
    case 0xC0000011:
        return "STATUS_END_OF_FILE";
    case 0xC0000012:
        return "STATUS_WRONG_VOLUME";
    case 0xC0000013:
        return "STATUS_NO_MEDIA_IN_DEVICE";
    case 0xC0000014:
        return "STATUS_UNRECOGNIZED_MEDIA";
    case 0xC0000015:
        return "STATUS_NONEXISTENT_SECTOR";
    case 0xC0000016:
        return "STATUS_MORE_PROCESSING_REQUIRED";
    case 0xC0000017:
        return "STATUS_NO_MEMORY";
    case 0xC0000018:
        return "STATUS_CONFLICTING_ADDRESSES";
    case 0xC0000019:
        return "STATUS_NOT_MAPPED_VIEW";
    case 0xC000001A:
        return "STATUS_UNABLE_TO_FREE_VM";
    case 0xC000001B:
        return "STATUS_UNABLE_TO_DELETE_SECTION";
    case 0xC000001C:
        return "STATUS_INVALID_SYSTEM_SERVICE";
    case 0xC000001D:
        return "STATUS_ILLEGAL_INSTRUCTION";
    case 0xC000001E:
        return "STATUS_INVALID_LOCK_SEQUENCE";
    case 0xC000001F:
        return "STATUS_INVALID_VIEW_SIZE";
    case 0xC0000020:
        return "STATUS_INVALID_FILE_FOR_SECTION";
    case 0xC0000021:
        return "STATUS_ALREADY_COMMITTED";
    case 0xC0000022:
        return "STATUS_ACCESS_DENIED";
    case 0xC0000023:
        return "STATUS_BUFFER_TOO_SMALL";
    case 0xC0000024:
        return "STATUS_OBJECT_TYPE_MISMATCH";
    case 0xC0000025:
        return "STATUS_NONCONTINUABLE_EXCEPTION";
    case 0xC0000026:
        return "STATUS_INVALID_DISPOSITION";
    case 0xC0000027:
        return "STATUS_UNWIND";
    case 0xC0000028:
        return "STATUS_BAD_STACK";
    case 0xC0000029:
        return "STATUS_INVALID_UNWIND_TARGET";
    case 0xC000002A:
        return "STATUS_NOT_LOCKED";
    case 0xC000002B:
        return "STATUS_PARITY_ERROR";
    case 0xC000002C:
        return "STATUS_UNABLE_TO_DECOMMIT_VM";
    case 0xC000002D:
        return "STATUS_NOT_COMMITTED";
    case 0xC000002E:
        return "STATUS_INVALID_PORT_ATTRIBUTES";
    case 0xC000002F:
        return "STATUS_PORT_MESSAGE_TOO_LONG";
    case 0xC0000030:
        return "STATUS_INVALID_PARAMETER_MIX";
    case 0xC0000031:
        return "STATUS_INVALID_QUOTA_LOWER";
    case 0xC0000032:
        return "STATUS_DISK_CORRUPT_ERROR";
    case 0xC0000033:
        return "STATUS_OBJECT_NAME_INVALID";
    case 0xC0000034:
        return "STATUS_OBJECT_NAME_NOT_FOUND";
    case 0xC0000035:
        return "STATUS_OBJECT_NAME_COLLISION";
    case 0xC0000037:
        return "STATUS_PORT_DISCONNECTED";
    case 0xC0000038:
        return "STATUS_DEVICE_ALREADY_ATTACHED";
    case 0xC0000039:
        return "STATUS_OBJECT_PATH_INVALID";
    case 0xC000003A:
        return "STATUS_OBJECT_PATH_NOT_FOUND";
    case 0xC000003B:
        return "STATUS_OBJECT_PATH_SYNTAX_BAD";
    case 0xC000003C:
        return "STATUS_DATA_OVERRUN";
    case 0xC000003D:
        return "STATUS_DATA_LATE_ERROR";
    case 0xC000003E:
        return "STATUS_DATA_ERROR";
    case 0xC000003F:
        return "STATUS_CRC_ERROR";
    case 0xC0000040:
        return "STATUS_SECTION_TOO_BIG";
    case 0xC0000041:
        return "STATUS_PORT_CONNECTION_REFUSED";
    case 0xC0000042:
        return "STATUS_INVALID_PORT_HANDLE";
    case 0xC0000043:
        return "STATUS_SHARING_VIOLATION";
    case 0xC0000044:
        return "STATUS_QUOTA_EXCEEDED";
    case 0xC0000045:
        return "STATUS_INVALID_PAGE_PROTECTION";
    case 0xC0000046:
        return "STATUS_MUTANT_NOT_OWNED";
    case 0xC0000047:
        return "STATUS_SEMAPHORE_LIMIT_EXCEEDED";
    case 0xC0000048:
        return "STATUS_PORT_ALREADY_SET";
    case 0xC0000049:
        return "STATUS_SECTION_NOT_IMAGE";
    case 0xC000004A:
        return "STATUS_SUSPEND_COUNT_EXCEEDED";
    case 0xC000004B:
        return "STATUS_THREAD_IS_TERMINATING";
    case 0xC000004C:
        return "STATUS_BAD_WORKING_SET_LIMIT";
    case 0xC000004D:
        return "STATUS_INCOMPATIBLE_FILE_MAP";
    case 0xC000004E:
        return "STATUS_SECTION_PROTECTION";
    case 0xC000004F:
        return "STATUS_EAS_NOT_SUPPORTED";
    case 0xC0000050:
        return "STATUS_EA_TOO_LARGE";
    case 0xC0000051:
        return "STATUS_NONEXISTENT_EA_ENTRY";
    case 0xC0000052:
        return "STATUS_NO_EAS_ON_FILE";
    case 0xC0000053:
        return "STATUS_EA_CORRUPT_ERROR";
    case 0xC0000054:
        return "STATUS_FILE_LOCK_CONFLICT";
    case 0xC0000055:
        return "STATUS_LOCK_NOT_GRANTED";
    case 0xC0000056:
        return "STATUS_DELETE_PENDING";
    case 0xC0000057:
        return "STATUS_CTL_FILE_NOT_SUPPORTED";
    case 0xC0000058:
        return "STATUS_UNKNOWN_REVISION";
    case 0xC0000059:
        return "STATUS_REVISION_MISMATCH";
    case 0xC000005A:
        return "STATUS_INVALID_OWNER";
    case 0xC000005B:
        return "STATUS_INVALID_PRIMARY_GROUP";
    case 0xC000005C:
        return "STATUS_NO_IMPERSONATION_TOKEN";
    case 0xC000005D:
        return "STATUS_CANT_DISABLE_MANDATORY";
    case 0xC000005E:
        return "STATUS_NO_LOGON_SERVERS";
    case 0xC000005F:
        return "STATUS_NO_SUCH_LOGON_SESSION";
    case 0xC0000060:
        return "STATUS_NO_SUCH_PRIVILEGE";
    case 0xC0000061:
        return "STATUS_PRIVILEGE_NOT_HELD";
    case 0xC0000062:
        return "STATUS_INVALID_ACCOUNT_NAME";
    case 0xC0000063:
        return "STATUS_USER_EXISTS";
    case 0xC0000064:
        return "STATUS_NO_SUCH_USER";
    case 0xC0000065:
        return "STATUS_GROUP_EXISTS";
    case 0xC0000066:
        return "STATUS_NO_SUCH_GROUP";
    case 0xC0000067:
        return "STATUS_MEMBER_IN_GROUP";
    case 0xC0000068:
        return "STATUS_MEMBER_NOT_IN_GROUP";
    case 0xC0000069:
        return "STATUS_LAST_ADMIN";
    case 0xC000006A:
        return "STATUS_WRONG_PASSWORD";
    case 0xC000006B:
        return "STATUS_ILL_FORMED_PASSWORD";
    case 0xC000006C:
        return "STATUS_PASSWORD_RESTRICTION";
    case 0xC000006D:
        return "STATUS_LOGON_FAILURE";
    case 0xC000006E:
        return "STATUS_ACCOUNT_RESTRICTION";
    case 0xC000006F:
        return "STATUS_INVALID_LOGON_HOURS";
    case 0xC0000070:
        return "STATUS_INVALID_WORKSTATION";
    case 0xC0000071:
        return "STATUS_PASSWORD_EXPIRED";
    case 0xC0000072:
        return "STATUS_ACCOUNT_DISABLED";
    case 0xC0000073:
        return "STATUS_NONE_MAPPED";
    case 0xC0000074:
        return "STATUS_TOO_MANY_LUIDS_REQUESTED";
    case 0xC0000075:
        return "STATUS_LUIDS_EXHAUSTED";
    case 0xC0000076:
        return "STATUS_INVALID_SUB_AUTHORITY";
    case 0xC0000077:
        return "STATUS_INVALID_ACL";
    case 0xC0000078:
        return "STATUS_INVALID_SID";
    case 0xC0000079:
        return "STATUS_INVALID_SECURITY_DESCR";
    case 0xC000007A:
        return "STATUS_PROCEDURE_NOT_FOUND";
    case 0xC000007B:
        return "STATUS_INVALID_IMAGE_FORMAT";
    case 0xC000007C:
        return "STATUS_NO_TOKEN";
    case 0xC000007D:
        return "STATUS_BAD_INHERITANCE_ACL";
    case 0xC000007E:
        return "STATUS_RANGE_NOT_LOCKED";
    case 0xC000007F:
        return "STATUS_DISK_FULL";
    case 0xC0000080:
        return "STATUS_SERVER_DISABLED";
    case 0xC0000081:
        return "STATUS_SERVER_NOT_DISABLED";
    case 0xC0000082:
        return "STATUS_TOO_MANY_GUIDS_REQUESTED";
    case 0xC0000083:
        return "STATUS_GUIDS_EXHAUSTED";
    case 0xC0000084:
        return "STATUS_INVALID_ID_AUTHORITY";
    case 0xC0000085:
        return "STATUS_AGENTS_EXHAUSTED";
    case 0xC0000086:
        return "STATUS_INVALID_VOLUME_LABEL";
    case 0xC0000087:
        return "STATUS_SECTION_NOT_EXTENDED";
    case 0xC0000088:
        return "STATUS_NOT_MAPPED_DATA";
    case 0xC0000089:
        return "STATUS_RESOURCE_DATA_NOT_FOUND";
    case 0xC000008A:
        return "STATUS_RESOURCE_TYPE_NOT_FOUND";
    case 0xC000008B:
        return "STATUS_RESOURCE_NAME_NOT_FOUND";
    case 0xC000008C:
        return "STATUS_ARRAY_BOUNDS_EXCEEDED";
    case 0xC000008D:
        return "STATUS_FLOAT_DENORMAL_OPERAND";
    case 0xC000008E:
        return "STATUS_FLOAT_DIVIDE_BY_ZERO";
    case 0xC000008F:
        return "STATUS_FLOAT_INEXACT_RESULT";
    case 0xC0000090:
        return "STATUS_FLOAT_INVALID_OPERATION";
    case 0xC0000091:
        return "STATUS_FLOAT_OVERFLOW";
    case 0xC0000092:
        return "STATUS_FLOAT_STACK_CHECK";
    case 0xC0000093:
        return "STATUS_FLOAT_UNDERFLOW";
    case 0xC0000094:
        return "STATUS_INTEGER_DIVIDE_BY_ZERO";
    case 0xC0000095:
        return "STATUS_INTEGER_OVERFLOW";
    case 0xC0000096:
        return "STATUS_PRIVILEGED_INSTRUCTION";
    case 0xC0000097:
        return "STATUS_TOO_MANY_PAGING_FILES";
    case 0xC0000098:
        return "STATUS_FILE_INVALID";
    case 0xC0000099:
        return "STATUS_ALLOTTED_SPACE_EXCEEDED";
    case 0xC000009A:
        return "STATUS_INSUFFICIENT_RESOURCES";
    case 0xC000009B:
        return "STATUS_DFS_EXIT_PATH_FOUND";
    case 0xC000009C:
        return "STATUS_DEVICE_DATA_ERROR";
    case 0xC000009D:
        return "STATUS_DEVICE_NOT_CONNECTED";
    case 0xC000009E:
        return "STATUS_DEVICE_POWER_FAILURE";
    case 0xC000009F:
        return "STATUS_FREE_VM_NOT_AT_BASE";
    case 0xC00000A0:
        return "STATUS_MEMORY_NOT_ALLOCATED";
    case 0xC00000A1:
        return "STATUS_WORKING_SET_QUOTA";
    case 0xC00000A2:
        return "STATUS_MEDIA_WRITE_PROTECTED";
    case 0xC00000A3:
        return "STATUS_DEVICE_NOT_READY";
    case 0xC00000A4:
        return "STATUS_INVALID_GROUP_ATTRIBUTES";
    case 0xC00000A5:
        return "STATUS_BAD_IMPERSONATION_LEVEL";
    case 0xC00000A6:
        return "STATUS_CANT_OPEN_ANONYMOUS";
    case 0xC00000A7:
        return "STATUS_BAD_VALIDATION_CLASS";
    case 0xC00000A8:
        return "STATUS_BAD_TOKEN_TYPE";
    case 0xC00000A9:
        return "STATUS_BAD_MASTER_BOOT_RECORD";
    case 0xC00000AA:
        return "STATUS_INSTRUCTION_MISALIGNMENT";
    case 0xC00000AB:
        return "STATUS_INSTANCE_NOT_AVAILABLE";
    case 0xC00000AC:
        return "STATUS_PIPE_NOT_AVAILABLE";
    case 0xC00000AD:
        return "STATUS_INVALID_PIPE_STATE";
    case 0xC00000AE:
        return "STATUS_PIPE_BUSY";
    case 0xC00000AF:
        return "STATUS_ILLEGAL_FUNCTION";
    case 0xC00000B0:
        return "STATUS_PIPE_DISCONNECTED";
    case 0xC00000B1:
        return "STATUS_PIPE_CLOSING";
    case 0xC00000B2:
        return "STATUS_PIPE_CONNECTED";
    case 0xC00000B3:
        return "STATUS_PIPE_LISTENING";
    case 0xC00000B4:
        return "STATUS_INVALID_READ_MODE";
    case 0xC00000B5:
        return "STATUS_IO_TIMEOUT";
    case 0xC00000B6:
        return "STATUS_FILE_FORCED_CLOSED";
    case 0xC00000B7:
        return "STATUS_PROFILING_NOT_STARTED";
    case 0xC00000B8:
        return "STATUS_PROFILING_NOT_STOPPED";
    case 0xC00000B9:
        return "STATUS_COULD_NOT_INTERPRET";
    case 0xC00000BA:
        return "STATUS_FILE_IS_A_DIRECTORY";
    case 0xC00000BB:
        return "STATUS_NOT_SUPPORTED";
    case 0xC00000BC:
        return "STATUS_REMOTE_NOT_LISTENING";
    case 0xC00000BD:
        return "STATUS_DUPLICATE_NAME";
    case 0xC00000BE:
        return "STATUS_BAD_NETWORK_PATH";
    case 0xC00000BF:
        return "STATUS_NETWORK_BUSY";
    case 0xC00000C0:
        return "STATUS_DEVICE_DOES_NOT_EXIST";
    case 0xC00000C1:
        return "STATUS_TOO_MANY_COMMANDS";
    case 0xC00000C2:
        return "STATUS_ADAPTER_HARDWARE_ERROR";
    case 0xC00000C3:
        return "STATUS_INVALID_NETWORK_RESPONSE";
    case 0xC00000C4:
        return "STATUS_UNEXPECTED_NETWORK_ERROR";
    case 0xC00000C5:
        return "STATUS_BAD_REMOTE_ADAPTER";
    case 0xC00000C6:
        return "STATUS_PRINT_QUEUE_FULL";
    case 0xC00000C7:
        return "STATUS_NO_SPOOL_SPACE";
    case 0xC00000C8:
        return "STATUS_PRINT_CANCELLED";
    case 0xC00000C9:
        return "STATUS_NETWORK_NAME_DELETED";
    case 0xC00000CA:
        return "STATUS_NETWORK_ACCESS_DENIED";
    case 0xC00000CB:
        return "STATUS_BAD_DEVICE_TYPE";
    case 0xC00000CC:
        return "STATUS_BAD_NETWORK_NAME";
    case 0xC00000CD:
        return "STATUS_TOO_MANY_NAMES";
    case 0xC00000CE:
        return "STATUS_TOO_MANY_SESSIONS";
    case 0xC00000CF:
        return "STATUS_SHARING_PAUSED";
    case 0xC00000D0:
        return "STATUS_REQUEST_NOT_ACCEPTED";
    case 0xC00000D1:
        return "STATUS_REDIRECTOR_PAUSED";
    case 0xC00000D2:
        return "STATUS_NET_WRITE_FAULT";
    case 0xC00000D3:
        return "STATUS_PROFILING_AT_LIMIT";
    case 0xC00000D4:
        return "STATUS_NOT_SAME_DEVICE";
    case 0xC00000D5:
        return "STATUS_FILE_RENAMED";
    case 0xC00000D6:
        return "STATUS_VIRTUAL_CIRCUIT_CLOSED";
    case 0xC00000D7:
        return "STATUS_NO_SECURITY_ON_OBJECT";
    case 0xC00000D8:
        return "STATUS_CANT_WAIT";
    case 0xC00000D9:
        return "STATUS_PIPE_EMPTY";
    case 0xC00000DA:
        return "STATUS_CANT_ACCESS_DOMAIN_INFO";
    case 0xC00000DB:
        return "STATUS_CANT_TERMINATE_SELF";
    case 0xC00000DC:
        return "STATUS_INVALID_SERVER_STATE";
    case 0xC00000DD:
        return "STATUS_INVALID_DOMAIN_STATE";
    case 0xC00000DE:
        return "STATUS_INVALID_DOMAIN_ROLE";
    case 0xC00000DF:
        return "STATUS_NO_SUCH_DOMAIN";
    case 0xC00000E0:
        return "STATUS_DOMAIN_EXISTS";
    case 0xC00000E1:
        return "STATUS_DOMAIN_LIMIT_EXCEEDED";
    case 0xC00000E2:
        return "STATUS_OPLOCK_NOT_GRANTED";
    case 0xC00000E3:
        return "STATUS_INVALID_OPLOCK_PROTOCOL";
    case 0xC00000E4:
        return "STATUS_INTERNAL_DB_CORRUPTION";
    case 0xC00000E5:
        return "STATUS_INTERNAL_ERROR";
    case 0xC00000E6:
        return "STATUS_GENERIC_NOT_MAPPED";
    case 0xC00000E7:
        return "STATUS_BAD_DESCRIPTOR_FORMAT";
    case 0xC00000E8:
        return "STATUS_INVALID_USER_BUFFER";
    case 0xC00000E9:
        return "STATUS_UNEXPECTED_IO_ERROR";
    case 0xC00000EA:
        return "STATUS_UNEXPECTED_MM_CREATE_ERR";
    case 0xC00000EB:
        return "STATUS_UNEXPECTED_MM_MAP_ERROR";
    case 0xC00000EC:
        return "STATUS_UNEXPECTED_MM_EXTEND_ERR";
    case 0xC00000ED:
        return "STATUS_NOT_LOGON_PROCESS";
    case 0xC00000EE:
        return "STATUS_LOGON_SESSION_EXISTS";
    case 0xC00000EF:
        return "STATUS_INVALID_PARAMETER_1";
    case 0xC00000F0:
        return "STATUS_INVALID_PARAMETER_2";
    case 0xC00000F1:
        return "STATUS_INVALID_PARAMETER_3";
    case 0xC00000F2:
        return "STATUS_INVALID_PARAMETER_4";
    case 0xC00000F3:
        return "STATUS_INVALID_PARAMETER_5";
    case 0xC00000F4:
        return "STATUS_INVALID_PARAMETER_6";
    case 0xC00000F5:
        return "STATUS_INVALID_PARAMETER_7";
    case 0xC00000F6:
        return "STATUS_INVALID_PARAMETER_8";
    case 0xC00000F7:
        return "STATUS_INVALID_PARAMETER_9";
    case 0xC00000F8:
        return "STATUS_INVALID_PARAMETER_10";
    case 0xC00000F9:
        return "STATUS_INVALID_PARAMETER_11";
    case 0xC00000FA:
        return "STATUS_INVALID_PARAMETER_12";
    case 0xC00000FB:
        return "STATUS_REDIRECTOR_NOT_STARTED";
    case 0xC00000FC:
        return "STATUS_REDIRECTOR_STARTED";
    case 0xC00000FD:
        return "STATUS_STACK_OVERFLOW";
    case 0xC00000FE:
        return "STATUS_NO_SUCH_PACKAGE";
    case 0xC00000FF:
        return "STATUS_BAD_FUNCTION_TABLE";
    case 0xC0000100:
        return "STATUS_VARIABLE_NOT_FOUND";
    case 0xC0000101:
        return "STATUS_DIRECTORY_NOT_EMPTY";
    case 0xC0000102:
        return "STATUS_FILE_CORRUPT_ERROR";
    case 0xC0000103:
        return "STATUS_NOT_A_DIRECTORY";
    case 0xC0000104:
        return "STATUS_BAD_LOGON_SESSION_STATE";
    case 0xC0000105:
        return "STATUS_LOGON_SESSION_COLLISION";
    case 0xC0000106:
        return "STATUS_NAME_TOO_LONG";
    case 0xC0000107:
        return "STATUS_FILES_OPEN";
    case 0xC0000108:
        return "STATUS_CONNECTION_IN_USE";
    case 0xC0000109:
        return "STATUS_MESSAGE_NOT_FOUND";
    case 0xC000010A:
        return "STATUS_PROCESS_IS_TERMINATING";
    case 0xC000010B:
        return "STATUS_INVALID_LOGON_TYPE";
    case 0xC000010C:
        return "STATUS_NO_GUID_TRANSLATION";
    case 0xC000010D:
        return "STATUS_CANNOT_IMPERSONATE";
    case 0xC000010E:
        return "STATUS_IMAGE_ALREADY_LOADED";
    case 0xC000010F:
        return "STATUS_ABIOS_NOT_PRESENT";
    case 0xC0000110:
        return "STATUS_ABIOS_LID_NOT_EXIST";
    case 0xC0000111:
        return "STATUS_ABIOS_LID_ALREADY_OWNED";
    case 0xC0000112:
        return "STATUS_ABIOS_NOT_LID_OWNER";
    case 0xC0000113:
        return "STATUS_ABIOS_INVALID_COMMAND";
    case 0xC0000114:
        return "STATUS_ABIOS_INVALID_LID";
    case 0xC0000115:
        return "STATUS_ABIOS_SELECTOR_NOT_AVAILABLE";
    case 0xC0000116:
        return "STATUS_ABIOS_INVALID_SELECTOR";
    case 0xC0000117:
        return "STATUS_NO_LDT";
    case 0xC0000118:
        return "STATUS_INVALID_LDT_SIZE";
    case 0xC0000119:
        return "STATUS_INVALID_LDT_OFFSET";
    case 0xC000011A:
        return "STATUS_INVALID_LDT_DESCRIPTOR";
    case 0xC000011B:
        return "STATUS_INVALID_IMAGE_NE_FORMAT";
    case 0xC000011C:
        return "STATUS_RXACT_INVALID_STATE";
    case 0xC000011D:
        return "STATUS_RXACT_COMMIT_FAILURE";
    case 0xC000011E:
        return "STATUS_MAPPED_FILE_SIZE_ZERO";
    case 0xC000011F:
        return "STATUS_TOO_MANY_OPENED_FILES";
    case 0xC0000120:
        return "STATUS_CANCELLED";
    case 0xC0000121:
        return "STATUS_CANNOT_DELETE";
    case 0xC0000122:
        return "STATUS_INVALID_COMPUTER_NAME";
    case 0xC0000123:
        return "STATUS_FILE_DELETED";
    case 0xC0000124:
        return "STATUS_SPECIAL_ACCOUNT";
    case 0xC0000125:
        return "STATUS_SPECIAL_GROUP";
    case 0xC0000126:
        return "STATUS_SPECIAL_USER";
    case 0xC0000127:
        return "STATUS_MEMBERS_PRIMARY_GROUP";
    case 0xC0000128:
        return "STATUS_FILE_CLOSED";
    case 0xC0000129:
        return "STATUS_TOO_MANY_THREADS";
    case 0xC000012A:
        return "STATUS_THREAD_NOT_IN_PROCESS";
    case 0xC000012B:
        return "STATUS_TOKEN_ALREADY_IN_USE";
    case 0xC000012C:
        return "STATUS_PAGEFILE_QUOTA_EXCEEDED";
    case 0xC000012D:
        return "STATUS_COMMITMENT_LIMIT";
    case 0xC000012E:
        return "STATUS_INVALID_IMAGE_LE_FORMAT";
    case 0xC000012F:
        return "STATUS_INVALID_IMAGE_NOT_MZ";
    case 0xC0000130:
        return "STATUS_INVALID_IMAGE_PROTECT";
    case 0xC0000131:
        return "STATUS_INVALID_IMAGE_WIN_16";
    case 0xC0000132:
        return "STATUS_LOGON_SERVER_CONFLICT";
    case 0xC0000133:
        return "STATUS_TIME_DIFFERENCE_AT_DC";
    case 0xC0000134:
        return "STATUS_SYNCHRONIZATION_REQUIRED";
    case 0xC0000135:
        return "STATUS_DLL_NOT_FOUND";
    case 0xC0000136:
        return "STATUS_OPEN_FAILED";
    case 0xC0000137:
        return "STATUS_IO_PRIVILEGE_FAILED";
    case 0xC0000138:
        return "STATUS_ORDINAL_NOT_FOUND";
    case 0xC0000139:
        return "STATUS_ENTRYPOINT_NOT_FOUND";
    case 0xC000013A:
        return "STATUS_CONTROL_C_EXIT";
    case 0xC000013B:
        return "STATUS_LOCAL_DISCONNECT";
    case 0xC000013C:
        return "STATUS_REMOTE_DISCONNECT";
    case 0xC000013D:
        return "STATUS_REMOTE_RESOURCES";
    case 0xC000013E:
        return "STATUS_LINK_FAILED";
    case 0xC000013F:
        return "STATUS_LINK_TIMEOUT";
    case 0xC0000140:
        return "STATUS_INVALID_CONNECTION";
    case 0xC0000141:
        return "STATUS_INVALID_ADDRESS";
    case 0xC0000142:
        return "STATUS_DLL_INIT_FAILED";
    case 0xC0000143:
        return "STATUS_MISSING_SYSTEMFILE";
    case 0xC0000144:
        return "STATUS_UNHANDLED_EXCEPTION";
    case 0xC0000145:
        return "STATUS_APP_INIT_FAILURE";
    case 0xC0000146:
        return "STATUS_PAGEFILE_CREATE_FAILED";
    case 0xC0000147:
        return "STATUS_NO_PAGEFILE";
    case 0xC0000148:
        return "STATUS_INVALID_LEVEL";
    case 0xC0000149:
        return "STATUS_WRONG_PASSWORD_CORE";
    case 0xC000014A:
        return "STATUS_ILLEGAL_FLOAT_CONTEXT";
    case 0xC000014B:
        return "STATUS_PIPE_BROKEN";
    case 0xC000014C:
        return "STATUS_REGISTRY_CORRUPT";
    case 0xC000014D:
        return "STATUS_REGISTRY_IO_FAILED";
    case 0xC000014E:
        return "STATUS_NO_EVENT_PAIR";
    case 0xC000014F:
        return "STATUS_UNRECOGNIZED_VOLUME";
    case 0xC0000150:
        return "STATUS_SERIAL_NO_DEVICE_INITED";
    case 0xC0000151:
        return "STATUS_NO_SUCH_ALIAS";
    case 0xC0000152:
        return "STATUS_MEMBER_NOT_IN_ALIAS";
    case 0xC0000153:
        return "STATUS_MEMBER_IN_ALIAS";
    case 0xC0000154:
        return "STATUS_ALIAS_EXISTS";
    case 0xC0000155:
        return "STATUS_LOGON_NOT_GRANTED";
    case 0xC0000156:
        return "STATUS_TOO_MANY_SECRETS";
    case 0xC0000157:
        return "STATUS_SECRET_TOO_LONG";
    case 0xC0000158:
        return "STATUS_INTERNAL_DB_ERROR";
    case 0xC0000159:
        return "STATUS_FULLSCREEN_MODE";
    case 0xC000015A:
        return "STATUS_TOO_MANY_CONTEXT_IDS";
    case 0xC000015B:
        return "STATUS_LOGON_TYPE_NOT_GRANTED";
    case 0xC000015C:
        return "STATUS_NOT_REGISTRY_FILE";
    case 0xC000015D:
        return "STATUS_NT_CROSS_ENCRYPTION_REQUIRED";
    case 0xC000015E:
        return "STATUS_DOMAIN_CTRLR_CONFIG_ERROR";
    case 0xC000015F:
        return "STATUS_FT_MISSING_MEMBER";
    case 0xC0000160:
        return "STATUS_ILL_FORMED_SERVICE_ENTRY";
    case 0xC0000161:
        return "STATUS_ILLEGAL_CHARACTER";
    case 0xC0000162:
        return "STATUS_UNMAPPABLE_CHARACTER";
    case 0xC0000163:
        return "STATUS_UNDEFINED_CHARACTER";
    case 0xC0000164:
        return "STATUS_FLOPPY_VOLUME";
    case 0xC0000165:
        return "STATUS_FLOPPY_ID_MARK_NOT_FOUND";
    case 0xC0000166:
        return "STATUS_FLOPPY_WRONG_CYLINDER";
    case 0xC0000167:
        return "STATUS_FLOPPY_UNKNOWN_ERROR";
    case 0xC0000168:
        return "STATUS_FLOPPY_BAD_REGISTERS";
    case 0xC0000169:
        return "STATUS_DISK_RECALIBRATE_FAILED";
    case 0xC000016A:
        return "STATUS_DISK_OPERATION_FAILED";
    case 0xC000016B:
        return "STATUS_DISK_RESET_FAILED";
    case 0xC000016C:
        return "STATUS_SHARED_IRQ_BUSY";
    case 0xC000016D:
        return "STATUS_FT_ORPHANING";
    case 0xC000016E:
        return "STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT";
    case 0xC0000172:
        return "STATUS_PARTITION_FAILURE";
    case 0xC0000173:
        return "STATUS_INVALID_BLOCK_LENGTH";
    case 0xC0000174:
        return "STATUS_DEVICE_NOT_PARTITIONED";
    case 0xC0000175:
        return "STATUS_UNABLE_TO_LOCK_MEDIA";
    case 0xC0000176:
        return "STATUS_UNABLE_TO_UNLOAD_MEDIA";
    case 0xC0000177:
        return "STATUS_EOM_OVERFLOW";
    case 0xC0000178:
        return "STATUS_NO_MEDIA";
    case 0xC000017A:
        return "STATUS_NO_SUCH_MEMBER";
    case 0xC000017B:
        return "STATUS_INVALID_MEMBER";
    case 0xC000017C:
        return "STATUS_KEY_DELETED";
    case 0xC000017D:
        return "STATUS_NO_LOG_SPACE";
    case 0xC000017E:
        return "STATUS_TOO_MANY_SIDS";
    case 0xC000017F:
        return "STATUS_LM_CROSS_ENCRYPTION_REQUIRED";
    case 0xC0000180:
        return "STATUS_KEY_HAS_CHILDREN";
    case 0xC0000181:
        return "STATUS_CHILD_MUST_BE_VOLATILE";
    case 0xC0000182:
        return "STATUS_DEVICE_CONFIGURATION_ERROR";
    case 0xC0000183:
        return "STATUS_DRIVER_INTERNAL_ERROR";
    case 0xC0000184:
        return "STATUS_INVALID_DEVICE_STATE";
    case 0xC0000185:
        return "STATUS_IO_DEVICE_ERROR";
    case 0xC0000186:
        return "STATUS_DEVICE_PROTOCOL_ERROR";
    case 0xC0000187:
        return "STATUS_BACKUP_CONTROLLER";
    case 0xC0000188:
        return "STATUS_LOG_FILE_FULL";
    case 0xC0000189:
        return "STATUS_TOO_LATE";
    case 0xC000018A:
        return "STATUS_NO_TRUST_LSA_SECRET";
    case 0xC000018B:
        return "STATUS_NO_TRUST_SAM_ACCOUNT";
    case 0xC000018C:
        return "STATUS_TRUSTED_DOMAIN_FAILURE";
    case 0xC000018D:
        return "STATUS_TRUSTED_RELATIONSHIP_FAILURE";
    case 0xC000018E:
        return "STATUS_EVENTLOG_FILE_CORRUPT";
    case 0xC000018F:
        return "STATUS_EVENTLOG_CANT_START";
    case 0xC0000190:
        return "STATUS_TRUST_FAILURE";
    case 0xC0000191:
        return "STATUS_MUTANT_LIMIT_EXCEEDED";
    case 0xC0000192:
        return "STATUS_NETLOGON_NOT_STARTED";
    case 0xC0000193:
        return "STATUS_ACCOUNT_EXPIRED";
    case 0xC0000194:
        return "STATUS_POSSIBLE_DEADLOCK";
    case 0xC0000195:
        return "STATUS_NETWORK_CREDENTIAL_CONFLICT";
    case 0xC0000196:
        return "STATUS_REMOTE_SESSION_LIMIT";
    case 0xC0000197:
        return "STATUS_EVENTLOG_FILE_CHANGED";
    case 0xC0000198:
        return "STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT";
    case 0xC0000199:
        return "STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT";
    case 0xC000019A:
        return "STATUS_NOLOGON_SERVER_TRUST_ACCOUNT";
    case 0xC000019B:
        return "STATUS_DOMAIN_TRUST_INCONSISTENT";
    case 0xC000019C:
        return "STATUS_FS_DRIVER_REQUIRED";
    case 0xC0000202:
        return "STATUS_NO_USER_SESSION_KEY";
    case 0xC0000203:
        return "STATUS_USER_SESSION_DELETED";
    case 0xC0000204:
        return "STATUS_RESOURCE_LANG_NOT_FOUND";
    case 0xC0000205:
        return "STATUS_INSUFF_SERVER_RESOURCES";
    case 0xC0000206:
        return "STATUS_INVALID_BUFFER_SIZE";
    case 0xC0000207:
        return "STATUS_INVALID_ADDRESS_COMPONENT";
    case 0xC0000208:
        return "STATUS_INVALID_ADDRESS_WILDCARD";
    case 0xC0000209:
        return "STATUS_TOO_MANY_ADDRESSES";
    case 0xC000020A:
        return "STATUS_ADDRESS_ALREADY_EXISTS";
    case 0xC000020B:
        return "STATUS_ADDRESS_CLOSED";
    case 0xC000020C:
        return "STATUS_CONNECTION_DISCONNECTED";
    case 0xC000020D:
        return "STATUS_CONNECTION_RESET";
    case 0xC000020E:
        return "STATUS_TOO_MANY_NODES";
    case 0xC000020F:
        return "STATUS_TRANSACTION_ABORTED";
    case 0xC0000210:
        return "STATUS_TRANSACTION_TIMED_OUT";
    case 0xC0000211:
        return "STATUS_TRANSACTION_NO_RELEASE";
    case 0xC0000212:
        return "STATUS_TRANSACTION_NO_MATCH";
    case 0xC0000213:
        return "STATUS_TRANSACTION_RESPONDED";
    case 0xC0000214:
        return "STATUS_TRANSACTION_INVALID_ID";
    case 0xC0000215:
        return "STATUS_TRANSACTION_INVALID_TYPE";
    case 0xC0000216:
        return "STATUS_NOT_SERVER_SESSION";
    case 0xC0000217:
        return "STATUS_NOT_CLIENT_SESSION";
    case 0xC0000218:
        return "STATUS_CANNOT_LOAD_REGISTRY_FILE";
    case 0xC0000219:
        return "STATUS_DEBUG_ATTACH_FAILED";
    case 0xC000021A:
        return "STATUS_SYSTEM_PROCESS_TERMINATED";
    case 0xC000021B:
        return "STATUS_DATA_NOT_ACCEPTED";
    case 0xC000021C:
        return "STATUS_NO_BROWSER_SERVERS_FOUND";
    case 0xC000021D:
        return "STATUS_VDM_HARD_ERROR";
    case 0xC000021E:
        return "STATUS_DRIVER_CANCEL_TIMEOUT";
    case 0xC000021F:
        return "STATUS_REPLY_MESSAGE_MISMATCH";
    case 0xC0000220:
        return "STATUS_MAPPED_ALIGNMENT";
    case 0xC0000221:
        return "STATUS_IMAGE_CHECKSUM_MISMATCH";
    case 0xC0000222:
        return "STATUS_LOST_WRITEBEHIND_DATA";
    case 0xC0000223:
        return "STATUS_CLIENT_SERVER_PARAMETERS_INVALID";
    case 0xC0000224:
        return "STATUS_PASSWORD_MUST_CHANGE";
    case 0xC0000225:
        return "STATUS_NOT_FOUND";
    case 0xC0000226:
        return "STATUS_NOT_TINY_STREAM";
    case 0xC0000227:
        return "STATUS_RECOVERY_FAILURE";
    case 0xC0000228:
        return "STATUS_STACK_OVERFLOW_READ";
    case 0xC0000229:
        return "STATUS_FAIL_CHECK";
    case 0xC000022A:
        return "STATUS_DUPLICATE_OBJECTID";
    case 0xC000022B:
        return "STATUS_OBJECTID_EXISTS";
    case 0xC000022C:
        return "STATUS_CONVERT_TO_LARGE";
    case 0xC000022D:
        return "STATUS_RETRY";
    case 0xC000022E:
        return "STATUS_FOUND_OUT_OF_SCOPE";
    case 0xC000022F:
        return "STATUS_ALLOCATE_BUCKET";
    case 0xC0000230:
        return "STATUS_PROPSET_NOT_FOUND";
    case 0xC0000231:
        return "STATUS_MARSHALL_OVERFLOW";
    case 0xC0000232:
        return "STATUS_INVALID_VARIANT";
    case 0xC0000233:
        return "STATUS_DOMAIN_CONTROLLER_NOT_FOUND";
    case 0xC0000234:
        return "STATUS_ACCOUNT_LOCKED_OUT";
    case 0xC0000235:
        return "STATUS_HANDLE_NOT_CLOSABLE";
    case 0xC0000236:
        return "STATUS_CONNECTION_REFUSED";
    case 0xC0000237:
        return "STATUS_GRACEFUL_DISCONNECT";
    case 0xC0000238:
        return "STATUS_ADDRESS_ALREADY_ASSOCIATED";
    case 0xC0000239:
        return "STATUS_ADDRESS_NOT_ASSOCIATED";
    case 0xC000023A:
        return "STATUS_CONNECTION_INVALID";
    case 0xC000023B:
        return "STATUS_CONNECTION_ACTIVE";
    case 0xC000023C:
        return "STATUS_NETWORK_UNREACHABLE";
    case 0xC000023D:
        return "STATUS_HOST_UNREACHABLE";
    case 0xC000023E:
        return "STATUS_PROTOCOL_UNREACHABLE";
    case 0xC000023F:
        return "STATUS_PORT_UNREACHABLE";
    case 0xC0000240:
        return "STATUS_REQUEST_ABORTED";
    case 0xC0000241:
        return "STATUS_CONNECTION_ABORTED";
    case 0xC0000242:
        return "STATUS_BAD_COMPRESSION_BUFFER";
    case 0xC0000243:
        return "STATUS_USER_MAPPED_FILE";
    case 0xC0000244:
        return "STATUS_AUDIT_FAILED";
    case 0xC0000245:
        return "STATUS_TIMER_RESOLUTION_NOT_SET";
    case 0xC0000246:
        return "STATUS_CONNECTION_COUNT_LIMIT";
    case 0xC0000247:
        return "STATUS_LOGIN_TIME_RESTRICTION";
    case 0xC0000248:
        return "STATUS_LOGIN_WKSTA_RESTRICTION";
    case 0xC0000249:
        return "STATUS_IMAGE_MP_UP_MISMATCH";
    case 0xC0000250:
        return "STATUS_INSUFFICIENT_LOGON_INFO";
    case 0xC0000251:
        return "STATUS_BAD_DLL_ENTRYPOINT";
    case 0xC0000252:
        return "STATUS_BAD_SERVICE_ENTRYPOINT";
    case 0xC0000253:
        return "STATUS_LPC_REPLY_LOST";
    case 0xC0000254:
        return "STATUS_IP_ADDRESS_CONFLICT1";
    case 0xC0000255:
        return "STATUS_IP_ADDRESS_CONFLICT2";
    case 0xC0000256:
        return "STATUS_REGISTRY_QUOTA_LIMIT";
    case 0xC0000257:
        return "STATUS_PATH_NOT_COVERED";
    case 0xC0000258:
        return "STATUS_NO_CALLBACK_ACTIVE";
    case 0xC0000259:
        return "STATUS_LICENSE_QUOTA_EXCEEDED";
    case 0xC000025A:
        return "STATUS_PWD_TOO_SHORT";
    case 0xC000025B:
        return "STATUS_PWD_TOO_RECENT";
    case 0xC000025C:
        return "STATUS_PWD_HISTORY_CONFLICT";
    case 0xC000025E:
        return "STATUS_PLUGPLAY_NO_DEVICE";
    case 0xC000025F:
        return "STATUS_UNSUPPORTED_COMPRESSION";
    case 0xC0000260:
        return "STATUS_INVALID_HW_PROFILE";
    case 0xC0000261:
        return "STATUS_INVALID_PLUGPLAY_DEVICE_PATH";
    case 0xC0000262:
        return "STATUS_DRIVER_ORDINAL_NOT_FOUND";
    case 0xC0000263:
        return "STATUS_DRIVER_ENTRYPOINT_NOT_FOUND";
    case 0xC0000264:
        return "STATUS_RESOURCE_NOT_OWNED";
    case 0xC0000265:
        return "STATUS_TOO_MANY_LINKS";
    case 0xC0000266:
        return "STATUS_QUOTA_LIST_INCONSISTENT";
    case 0xC0000267:
        return "STATUS_FILE_IS_OFFLINE";
    case 0xC0000268:
        return "STATUS_EVALUATION_EXPIRATION";
    case 0xC0000269:
        return "STATUS_ILLEGAL_DLL_RELOCATION";
    case 0xC000026A:
        return "STATUS_LICENSE_VIOLATION";
    case 0xC000026B:
        return "STATUS_DLL_INIT_FAILED_LOGOFF";
    case 0xC000026C:
        return "STATUS_DRIVER_UNABLE_TO_LOAD";
    case 0xC000026D:
        return "STATUS_DFS_UNAVAILABLE";
    case 0xC000026E:
        return "STATUS_VOLUME_DISMOUNTED";
    case 0xC000026F:
        return "STATUS_WX86_INTERNAL_ERROR";
    case 0xC0000270:
        return "STATUS_WX86_FLOAT_STACK_CHECK";
    case 0xC0000271:
        return "STATUS_VALIDATE_CONTINUE";
    case 0xC0000272:
        return "STATUS_NO_MATCH";
    case 0xC0000273:
        return "STATUS_NO_MORE_MATCHES";
    case 0xC0000275:
        return "STATUS_NOT_A_REPARSE_POINT";
    case 0xC0000276:
        return "STATUS_IO_REPARSE_TAG_INVALID";
    case 0xC0000277:
        return "STATUS_IO_REPARSE_TAG_MISMATCH";
    case 0xC0000278:
        return "STATUS_IO_REPARSE_DATA_INVALID";
    case 0xC0000279:
        return "STATUS_IO_REPARSE_TAG_NOT_HANDLED";
    case 0xC0000280:
        return "STATUS_REPARSE_POINT_NOT_RESOLVED";
    case 0xC0000281:
        return "STATUS_DIRECTORY_IS_A_REPARSE_POINT";
    case 0xC0000282:
        return "STATUS_RANGE_LIST_CONFLICT";
    case 0xC0000283:
        return "STATUS_SOURCE_ELEMENT_EMPTY";
    case 0xC0000284:
        return "STATUS_DESTINATION_ELEMENT_FULL";
    case 0xC0000285:
        return "STATUS_ILLEGAL_ELEMENT_ADDRESS";
    case 0xC0000286:
        return "STATUS_MAGAZINE_NOT_PRESENT";
    case 0xC0000287:
        return "STATUS_REINITIALIZATION_NEEDED";
    case 0x80000288:
        return "STATUS_DEVICE_REQUIRES_CLEANING";
    case 0x80000289:
        return "STATUS_DEVICE_DOOR_OPEN";
    case 0xC000028A:
        return "STATUS_ENCRYPTION_FAILED";
    case 0xC000028B:
        return "STATUS_DECRYPTION_FAILED";
    case 0xC000028C:
        return "STATUS_RANGE_NOT_FOUND";
    case 0xC000028D:
        return "STATUS_NO_RECOVERY_POLICY";
    case 0xC000028E:
        return "STATUS_NO_EFS";
    case 0xC000028F:
        return "STATUS_WRONG_EFS";
    case 0xC0000290:
        return "STATUS_NO_USER_KEYS";
    case 0xC0000291:
        return "STATUS_FILE_NOT_ENCRYPTED";
    case 0xC0000292:
        return "STATUS_NOT_EXPORT_FORMAT";
    case 0xC0000293:
        return "STATUS_FILE_ENCRYPTED";
    case 0x40000294:
        return "STATUS_WAKE_SYSTEM";
    case 0xC0000295:
        return "STATUS_WMI_GUID_NOT_FOUND";
    case 0xC0000296:
        return "STATUS_WMI_INSTANCE_NOT_FOUND";
    case 0xC0000297:
        return "STATUS_WMI_ITEMID_NOT_FOUND";
    case 0xC0000298:
        return "STATUS_WMI_TRY_AGAIN";
    case 0xC0000299:
        return "STATUS_SHARED_POLICY";
    case 0xC000029A:
        return "STATUS_POLICY_OBJECT_NOT_FOUND";
    case 0xC000029B:
        return "STATUS_POLICY_ONLY_IN_DS";
    case 0xC000029C:
        return "STATUS_VOLUME_NOT_UPGRADED";
    case 0xC000029D:
        return "STATUS_REMOTE_STORAGE_NOT_ACTIVE";
    case 0xC000029E:
        return "STATUS_REMOTE_STORAGE_MEDIA_ERROR";
    case 0xC000029F:
        return "STATUS_NO_TRACKING_SERVICE";
    case 0xC00002A0:
        return "STATUS_SERVER_SID_MISMATCH";
    case 0xC00002A1:
        return "STATUS_DS_NO_ATTRIBUTE_OR_VALUE";
    case 0xC00002A2:
        return "STATUS_DS_INVALID_ATTRIBUTE_SYNTAX";
    case 0xC00002A3:
        return "STATUS_DS_ATTRIBUTE_TYPE_UNDEFINED";
    case 0xC00002A4:
        return "STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS";
    case 0xC00002A5:
        return "STATUS_DS_BUSY";
    case 0xC00002A6:
        return "STATUS_DS_UNAVAILABLE";
    case 0xC00002A7:
        return "STATUS_DS_NO_RIDS_ALLOCATED";
    case 0xC00002A8:
        return "STATUS_DS_NO_MORE_RIDS";
    case 0xC00002A9:
        return "STATUS_DS_INCORRECT_ROLE_OWNER";
    case 0xC00002AA:
        return "STATUS_DS_RIDMGR_INIT_ERROR";
    case 0xC00002AB:
        return "STATUS_DS_OBJ_CLASS_VIOLATION";
    case 0xC00002AC:
        return "STATUS_DS_CANT_ON_NON_LEAF";
    case 0xC00002AD:
        return "STATUS_DS_CANT_ON_RDN";
    case 0xC00002AE:
        return "STATUS_DS_CANT_MOD_OBJ_CLASS";
    case 0xC00002AF:
        return "STATUS_DS_CROSS_DOM_MOVE_FAILED";
    case 0xC00002B0:
        return "STATUS_DS_GC_NOT_AVAILABLE";
    case 0xC00002B1:
        return "STATUS_DIRECTORY_SERVICE_REQUIRED";
    case 0xC00002B2:
        return "STATUS_REPARSE_ATTRIBUTE_CONFLICT";
    case 0xC00002B3:
        return "STATUS_CANT_ENABLE_DENY_ONLY";
    case 0xC00002B4:
        return "STATUS_FLOAT_MULTIPLE_FAULTS";
    case 0xC00002B5:
        return "STATUS_FLOAT_MULTIPLE_TRAPS";
    case 0xC00002B6:
        return "STATUS_DEVICE_REMOVED";
    case 0xC00002B7:
        return "STATUS_JOURNAL_DELETE_IN_PROGRESS";
    case 0xC00002B8:
        return "STATUS_JOURNAL_NOT_ACTIVE";
    case 0xC00002B9:
        return "STATUS_NOINTERFACE";
    case 0xC00002C1:
        return "STATUS_DS_ADMIN_LIMIT_EXCEEDED";
    case 0xC00002C2:
        return "STATUS_DRIVER_FAILED_SLEEP";
    case 0xC00002C3:
        return "STATUS_MUTUAL_AUTHENTICATION_FAILED";
    case 0xC00002C4:
        return "STATUS_CORRUPT_SYSTEM_FILE";
    case 0xC00002C5:
        return "STATUS_DATATYPE_MISALIGNMENT_ERROR";
    case 0xC00002C6:
        return "STATUS_WMI_READ_ONLY";
    case 0xC00002C7:
        return "STATUS_WMI_SET_FAILURE";
    case 0xC00002C8:
        return "STATUS_COMMITMENT_MINIMUM";
    case 0xC00002C9:
        return "STATUS_REG_NAT_CONSUMPTION";
    case 0xC00002CA:
        return "STATUS_TRANSPORT_FULL";
    case 0xC00002CB:
        return "STATUS_DS_SAM_INIT_FAILURE";
    case 0xC00002CC:
        return "STATUS_ONLY_IF_CONNECTED";
    case 0xC00002CD:
        return "STATUS_DS_SENSITIVE_GROUP_VIOLATION";
    case 0xC00002CE:
        return "STATUS_PNP_RESTART_ENUMERATION";
    case 0xC00002CF:
        return "STATUS_JOURNAL_ENTRY_DELETED";
    case 0xC00002D0:
        return "STATUS_DS_CANT_MOD_PRIMARYGROUPID";
    case 0xC00002D1:
        return "STATUS_SYSTEM_IMAGE_BAD_SIGNATURE";
    case 0xC00002D2:
        return "STATUS_PNP_REBOOT_REQUIRED";
    case 0xC00002D3:
        return "STATUS_POWER_STATE_INVALID";
    case 0xC00002D4:
        return "STATUS_DS_INVALID_GROUP_TYPE";
    case 0xC00002D5:
        return "STATUS_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN";
    case 0xC00002D6:
        return "STATUS_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN";
    case 0xC00002D7:
        return "STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER";
    case 0xC00002D8:
        return "STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER";
    case 0xC00002D9:
        return "STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER";
    case 0xC00002DA:
        return "STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER";
    case 0xC00002DB:
        return "STATUS_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER";
    case 0xC00002DC:
        return "STATUS_DS_HAVE_PRIMARY_MEMBERS";
    case 0xC00002DD:
        return "STATUS_WMI_NOT_SUPPORTED";
    case 0xC00002DE:
        return "STATUS_INSUFFICIENT_POWER";
    case 0xC00002DF:
        return "STATUS_SAM_NEED_BOOTKEY_PASSWORD";
    case 0xC00002E0:
        return "STATUS_SAM_NEED_BOOTKEY_FLOPPY";
    case 0xC00002E1:
        return "STATUS_DS_CANT_START";
    case 0xC00002E2:
        return "STATUS_DS_INIT_FAILURE";
    case 0xC00002E3:
        return "STATUS_SAM_INIT_FAILURE";
    case 0xC00002E4:
        return "STATUS_DS_GC_REQUIRED";
    case 0xC00002E5:
        return "STATUS_DS_LOCAL_MEMBER_OF_LOCAL_ONLY";
    case 0xC00002E6:
        return "STATUS_DS_NO_FPO_IN_UNIVERSAL_GROUPS";
    case 0xC00002E7:
        return "STATUS_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED";
    case 0xC00002E8:
        return "STATUS_MULTIPLE_FAULT_VIOLATION";
    case 0xC0000300:
        return "STATUS_NOT_SUPPORTED_ON_SBS";
    case 0xC0009898:
        return "STATUS_WOW_ASSERTION";
    case 0xC0010001:
        return "DBG_NO_STATE_CHANGE";
    case 0xC0010002:
        return "DBG_APP_NOT_IDLE";
    case 0xC0020001:
        return "RPC_NT_INVALID_STRING_BINDING";
    case 0xC0020002:
        return "RPC_NT_WRONG_KIND_OF_BINDING";
    case 0xC0020003:
        return "RPC_NT_INVALID_BINDING";
    case 0xC0020004:
        return "RPC_NT_PROTSEQ_NOT_SUPPORTED";
    case 0xC0020005:
        return "RPC_NT_INVALID_RPC_PROTSEQ";
    case 0xC0020006:
        return "RPC_NT_INVALID_STRING_UUID";
    case 0xC0020007:
        return "RPC_NT_INVALID_ENDPOINT_FORMAT";
    case 0xC0020008:
        return "RPC_NT_INVALID_NET_ADDR";
    case 0xC0020009:
        return "RPC_NT_NO_ENDPOINT_FOUND";
    case 0xC002000A:
        return "RPC_NT_INVALID_TIMEOUT";
    case 0xC002000B:
        return "RPC_NT_OBJECT_NOT_FOUND";
    case 0xC002000C:
        return "RPC_NT_ALREADY_REGISTERED";
    case 0xC002000D:
        return "RPC_NT_TYPE_ALREADY_REGISTERED";
    case 0xC002000E:
        return "RPC_NT_ALREADY_LISTENING";
    case 0xC002000F:
        return "RPC_NT_NO_PROTSEQS_REGISTERED";
    case 0xC0020010:
        return "RPC_NT_NOT_LISTENING";
    case 0xC0020011:
        return "RPC_NT_UNKNOWN_MGR_TYPE";
    case 0xC0020012:
        return "RPC_NT_UNKNOWN_IF";
    case 0xC0020013:
        return "RPC_NT_NO_BINDINGS";
    case 0xC0020014:
        return "RPC_NT_NO_PROTSEQS";
    case 0xC0020015:
        return "RPC_NT_CANT_CREATE_ENDPOINT";
    case 0xC0020016:
        return "RPC_NT_OUT_OF_RESOURCES";
    case 0xC0020017:
        return "RPC_NT_SERVER_UNAVAILABLE";
    case 0xC0020018:
        return "RPC_NT_SERVER_TOO_BUSY";
    case 0xC0020019:
        return "RPC_NT_INVALID_NETWORK_OPTIONS";
    case 0xC002001A:
        return "RPC_NT_NO_CALL_ACTIVE";
    case 0xC002001B:
        return "RPC_NT_CALL_FAILED";
    case 0xC002001C:
        return "RPC_NT_CALL_FAILED_DNE";
    case 0xC002001D:
        return "RPC_NT_PROTOCOL_ERROR";
    case 0xC002001F:
        return "RPC_NT_UNSUPPORTED_TRANS_SYN";
    case 0xC0020021:
        return "RPC_NT_UNSUPPORTED_TYPE";
    case 0xC0020022:
        return "RPC_NT_INVALID_TAG";
    case 0xC0020023:
        return "RPC_NT_INVALID_BOUND";
    case 0xC0020024:
        return "RPC_NT_NO_ENTRY_NAME";
    case 0xC0020025:
        return "RPC_NT_INVALID_NAME_SYNTAX";
    case 0xC0020026:
        return "RPC_NT_UNSUPPORTED_NAME_SYNTAX";
    case 0xC0020028:
        return "RPC_NT_UUID_NO_ADDRESS";
    case 0xC0020029:
        return "RPC_NT_DUPLICATE_ENDPOINT";
    case 0xC002002A:
        return "RPC_NT_UNKNOWN_AUTHN_TYPE";
    case 0xC002002B:
        return "RPC_NT_MAX_CALLS_TOO_SMALL";
    case 0xC002002C:
        return "RPC_NT_STRING_TOO_LONG";
    case 0xC002002D:
        return "RPC_NT_PROTSEQ_NOT_FOUND";
    case 0xC002002E:
        return "RPC_NT_PROCNUM_OUT_OF_RANGE";
    case 0xC002002F:
        return "RPC_NT_BINDING_HAS_NO_AUTH";
    case 0xC0020030:
        return "RPC_NT_UNKNOWN_AUTHN_SERVICE";
    case 0xC0020031:
        return "RPC_NT_UNKNOWN_AUTHN_LEVEL";
    case 0xC0020032:
        return "RPC_NT_INVALID_AUTH_IDENTITY";
    case 0xC0020033:
        return "RPC_NT_UNKNOWN_AUTHZ_SERVICE";
    case 0xC0020034:
        return "EPT_NT_INVALID_ENTRY";
    case 0xC0020035:
        return "EPT_NT_CANT_PERFORM_OP";
    case 0xC0020036:
        return "EPT_NT_NOT_REGISTERED";
    case 0xC0020037:
        return "RPC_NT_NOTHING_TO_EXPORT";
    case 0xC0020038:
        return "RPC_NT_INCOMPLETE_NAME";
    case 0xC0020039:
        return "RPC_NT_INVALID_VERS_OPTION";
    case 0xC002003A:
        return "RPC_NT_NO_MORE_MEMBERS";
    case 0xC002003B:
        return "RPC_NT_NOT_ALL_OBJS_UNEXPORTED";
    case 0xC002003C:
        return "RPC_NT_INTERFACE_NOT_FOUND";
    case 0xC002003D:
        return "RPC_NT_ENTRY_ALREADY_EXISTS";
    case 0xC002003E:
        return "RPC_NT_ENTRY_NOT_FOUND";
    case 0xC002003F:
        return "RPC_NT_NAME_SERVICE_UNAVAILABLE";
    case 0xC0020040:
        return "RPC_NT_INVALID_NAF_ID";
    case 0xC0020041:
        return "RPC_NT_CANNOT_SUPPORT";
    case 0xC0020042:
        return "RPC_NT_NO_CONTEXT_AVAILABLE";
    case 0xC0020043:
        return "RPC_NT_INTERNAL_ERROR";
    case 0xC0020044:
        return "RPC_NT_ZERO_DIVIDE";
    case 0xC0020045:
        return "RPC_NT_ADDRESS_ERROR";
    case 0xC0020046:
        return "RPC_NT_FP_DIV_ZERO";
    case 0xC0020047:
        return "RPC_NT_FP_UNDERFLOW";
    case 0xC0020048:
        return "RPC_NT_FP_OVERFLOW";
    case 0xC0030001:
        return "RPC_NT_NO_MORE_ENTRIES";
    case 0xC0030002:
        return "RPC_NT_SS_CHAR_TRANS_OPEN_FAIL";
    case 0xC0030003:
        return "RPC_NT_SS_CHAR_TRANS_SHORT_FILE";
    case 0xC0030004:
        return "RPC_NT_SS_IN_NULL_CONTEXT";
    case 0xC0030005:
        return "RPC_NT_SS_CONTEXT_MISMATCH";
    case 0xC0030006:
        return "RPC_NT_SS_CONTEXT_DAMAGED";
    case 0xC0030007:
        return "RPC_NT_SS_HANDLES_MISMATCH";
    case 0xC0030008:
        return "RPC_NT_SS_CANNOT_GET_CALL_HANDLE";
    case 0xC0030009:
        return "RPC_NT_NULL_REF_POINTER";
    case 0xC003000A:
        return "RPC_NT_ENUM_VALUE_OUT_OF_RANGE";
    case 0xC003000B:
        return "RPC_NT_BYTE_COUNT_TOO_SMALL";
    case 0xC003000C:
        return "RPC_NT_BAD_STUB_DATA";
    case 0xC0020049:
        return "RPC_NT_CALL_IN_PROGRESS";
    case 0xC002004A:
        return "RPC_NT_NO_MORE_BINDINGS";
    case 0xC002004B:
        return "RPC_NT_GROUP_MEMBER_NOT_FOUND";
    case 0xC002004C:
        return "EPT_NT_CANT_CREATE";
    case 0xC002004D:
        return "RPC_NT_INVALID_OBJECT";
    case 0xC002004F:
        return "RPC_NT_NO_INTERFACES";
    case 0xC0020050:
        return "RPC_NT_CALL_CANCELLED";
    case 0xC0020051:
        return "RPC_NT_BINDING_INCOMPLETE";
    case 0xC0020052:
        return "RPC_NT_COMM_FAILURE";
    case 0xC0020053:
        return "RPC_NT_UNSUPPORTED_AUTHN_LEVEL";
    case 0xC0020054:
        return "RPC_NT_NO_PRINC_NAME";
    case 0xC0020055:
        return "RPC_NT_NOT_RPC_ERROR";
    case 0x40020056:
        return "RPC_NT_UUID_LOCAL_ONLY";
    case 0xC0020057:
        return "RPC_NT_SEC_PKG_ERROR";
    case 0xC0020058:
        return "RPC_NT_NOT_CANCELLED";
    case 0xC0030059:
        return "RPC_NT_INVALID_ES_ACTION";
    case 0xC003005A:
        return "RPC_NT_WRONG_ES_VERSION";
    case 0xC003005B:
        return "RPC_NT_WRONG_STUB_VERSION";
    case 0xC003005C:
        return "RPC_NT_INVALID_PIPE_OBJECT";
    case 0xC003005D:
        return "RPC_NT_INVALID_PIPE_OPERATION";
    case 0xC003005E:
        return "RPC_NT_WRONG_PIPE_VERSION";
    case 0xC003005F:
        return "RPC_NT_PIPE_CLOSED";
    case 0xC0030060:
        return "RPC_NT_PIPE_DISCIPLINE_ERROR";
    case 0xC0030061:
        return "RPC_NT_PIPE_EMPTY";
    case 0xC0020062:
        return "RPC_NT_INVALID_ASYNC_HANDLE";
    case 0xC0020063:
        return "RPC_NT_INVALID_ASYNC_CALL";
    case 0x400200AF:
        return "RPC_NT_SEND_INCOMPLETE";
    case 0xC0140001:
        return "STATUS_ACPI_INVALID_OPCODE";
    case 0xC0140002:
        return "STATUS_ACPI_STACK_OVERFLOW";
    case 0xC0140003:
        return "STATUS_ACPI_ASSERT_FAILED";
    case 0xC0140004:
        return "STATUS_ACPI_INVALID_INDEX";
    case 0xC0140005:
        return "STATUS_ACPI_INVALID_ARGUMENT";
    case 0xC0140006:
        return "STATUS_ACPI_FATAL";
    case 0xC0140007:
        return "STATUS_ACPI_INVALID_SUPERNAME";
    case 0xC0140008:
        return "STATUS_ACPI_INVALID_ARGTYPE";
    case 0xC0140009:
        return "STATUS_ACPI_INVALID_OBJTYPE";
    case 0xC014000A:
        return "STATUS_ACPI_INVALID_TARGETTYPE";
    case 0xC014000B:
        return "STATUS_ACPI_INCORRECT_ARGUMENT_COUNT";
    case 0xC014000C:
        return "STATUS_ACPI_ADDRESS_NOT_MAPPED";
    case 0xC014000D:
        return "STATUS_ACPI_INVALID_EVENTTYPE";
    case 0xC014000E:
        return "STATUS_ACPI_HANDLER_COLLISION";
    case 0xC014000F:
        return "STATUS_ACPI_INVALID_DATA";
    case 0xC0140010:
        return "STATUS_ACPI_INVALID_REGION";
    case 0xC0140011:
        return "STATUS_ACPI_INVALID_ACCESS_SIZE";
    case 0xC0140012:
        return "STATUS_ACPI_ACQUIRE_GLOBAL_LOCK";
    case 0xC0140013:
        return "STATUS_ACPI_ALREADY_INITIALIZED";
    case 0xC0140014:
        return "STATUS_ACPI_NOT_INITIALIZED";
    case 0xC0140015:
        return "STATUS_ACPI_INVALID_MUTEX_LEVEL";
    case 0xC0140016:
        return "STATUS_ACPI_MUTEX_NOT_OWNED";
    case 0xC0140017:
        return "STATUS_ACPI_MUTEX_NOT_OWNER";
    case 0xC0140018:
        return "STATUS_ACPI_RS_ACCESS";
    case 0xC0140019:
        return "STATUS_ACPI_INVALID_TABLE";
    case 0xC0140020:
        return "STATUS_ACPI_REG_HANDLER_FAILED";
    case 0xC0140021:
        return "STATUS_ACPI_POWER_REQUEST_FAILED";
    case 0xC00A0001:
        return "STATUS_CTX_WINSTATION_NAME_INVALID";
    case 0xC00A0002:
        return "STATUS_CTX_INVALID_PD";
    case 0xC00A0003:
        return "STATUS_CTX_PD_NOT_FOUND";
    case 0x400A0004:
        return "STATUS_CTX_CDM_CONNECT";
    case 0x400A0005:
        return "STATUS_CTX_CDM_DISCONNECT";
    case 0xC00A0006:
        return "STATUS_CTX_CLOSE_PENDING";
    case 0xC00A0007:
        return "STATUS_CTX_NO_OUTBUF";
    case 0xC00A0008:
        return "STATUS_CTX_MODEM_INF_NOT_FOUND";
    case 0xC00A0009:
        return "STATUS_CTX_INVALID_MODEMNAME";
    case 0xC00A000A:
        return "STATUS_CTX_RESPONSE_ERROR";
    case 0xC00A000B:
        return "STATUS_CTX_MODEM_RESPONSE_TIMEOUT";
    case 0xC00A000C:
        return "STATUS_CTX_MODEM_RESPONSE_NO_CARRIER";
    case 0xC00A000D:
        return "STATUS_CTX_MODEM_RESPONSE_NO_DIALTONE";
    case 0xC00A000E:
        return "STATUS_CTX_MODEM_RESPONSE_BUSY";
    case 0xC00A000F:
        return "STATUS_CTX_MODEM_RESPONSE_VOICE";
    case 0xC00A0010:
        return "STATUS_CTX_TD_ERROR";
    case 0xC00A0012:
        return "STATUS_CTX_LICENSE_CLIENT_INVALID";
    case 0xC00A0013:
        return "STATUS_CTX_LICENSE_NOT_AVAILABLE";
    case 0xC00A0014:
        return "STATUS_CTX_LICENSE_EXPIRED";
    case 0xC00A0015:
        return "STATUS_CTX_WINSTATION_NOT_FOUND";
    case 0xC00A0016:
        return "STATUS_CTX_WINSTATION_NAME_COLLISION";
    case 0xC00A0017:
        return "STATUS_CTX_WINSTATION_BUSY";
    case 0xC00A0018:
        return "STATUS_CTX_BAD_VIDEO_MODE";
    case 0xC00A0022:
        return "STATUS_CTX_GRAPHICS_INVALID";
    case 0xC00A0024:
        return "STATUS_CTX_NOT_CONSOLE";
    case 0xC00A0026:
        return "STATUS_CTX_CLIENT_QUERY_TIMEOUT";
    case 0xC00A0027:
        return "STATUS_CTX_CONSOLE_DISCONNECT";
    case 0xC00A0028:
        return "STATUS_CTX_CONSOLE_CONNECT";
    case 0xC00A002A:
        return "STATUS_CTX_SHADOW_DENIED";
    case 0xC00A002B:
        return "STATUS_CTX_WINSTATION_ACCESS_DENIED";
    case 0xC00A002E:
        return "STATUS_CTX_INVALID_WD";
    case 0xC00A002F:
        return "STATUS_CTX_WD_NOT_FOUND";
    case 0xC00A0030:
        return "STATUS_CTX_SHADOW_INVALID";
    case 0xC00A0031:
        return "STATUS_CTX_SHADOW_DISABLED";
    case 0xC00A0032:
        return "STATUS_RDP_PROTOCOL_ERROR";
    case 0xC00A0033:
        return "STATUS_CTX_CLIENT_LICENSE_NOT_SET";
    case 0xC00A0034:
        return "STATUS_CTX_CLIENT_LICENSE_IN_USE";
    case 0xC0040035:
        return "STATUS_PNP_BAD_MPS_TABLE";
    case 0xC0040036:
        return "STATUS_PNP_TRANSLATION_FAILED";
    case 0xC0040037:
        return "STATUS_PNP_IRQ_TRANSLATION_FAILED";
    default:
        return "STATUS_UNKNOWN";
    }
}


/* avoid stack overflow for dead symlinks */

VOID
Ext2TraceMcb(PCHAR fn, USHORT lc, USHORT add, PEXT2_MCB Mcb) {
    int i;
    CHAR _space[33];

    _snprintf(&_space[0], 32, "%s:%d:", fn, lc);
    _space[32] = 0;
    i = strlen(_space);
    while (i < 32) {
        _space[i++] = ' ';
        _space[i]=0;
    }
    if (add) {
        Ext2ReferXcb(&Mcb->Refercount);
        DEBUG(DL_RES,   ("%s +%2u %wZ (%p)\n", _space, (Mcb->Refercount - 1), &Mcb->FullName, Mcb));
    } else {
        Ext2DerefXcb(&Mcb->Refercount);
        DEBUG(DL_RES, ("%s -%2u %wZ (%p)\n", _space, Mcb->Refercount, &Mcb->FullName, Mcb));
    }
}

KSPIN_LOCK  Ext2MemoryLock;
ULONGLONG   Ext2TotalMemorySize = 0;
ULONG       Ext2TotalAllocates = 0;

PVOID
Ext2AllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
)
{
    PUCHAR  Buffer =  ExAllocatePoolWithTag(
                          PoolType,
                          0x20 + NumberOfBytes,
                          Tag);
    if (Buffer) {
        KIRQL   Irql = 0;
        PULONG  Data = (PULONG)Buffer;
        Data[0] = (ULONG)NumberOfBytes;
        Data[1] = (ULONG)NumberOfBytes + 0x20;
        memset(Buffer + 0x08, 'S', 8);
        memset(Buffer + 0x10 + NumberOfBytes, 'E', 0x10);
        Buffer += 0x10;
        KeAcquireSpinLock(&Ext2MemoryLock, &Irql);
        Ext2TotalMemorySize = Ext2TotalMemorySize + NumberOfBytes;
        Ext2TotalAllocates += 1;
        KeReleaseSpinLock(&Ext2MemoryLock, Irql);
    }

    return Buffer;
}

VOID
Ext2FreePool(
    IN PVOID P,
    IN ULONG Tag
)
{
    PUCHAR  Buffer = (PUCHAR)P;
    PULONG  Data;
    ULONG   NumberOfBytes, i;
    KIRQL   Irql;

    Buffer -= 0x10;
    Data = (PULONG)(Buffer);
    NumberOfBytes = Data[0];
    if (Data[1] != NumberOfBytes + 0x20) {
        DbgBreak();
    }
    for (i=0x08; i < 0x10; i++) {
        if (Buffer[i] != 'S') {
            DbgBreak();
        }
        Buffer[i] = '-';
    }
    for (i=0; i < 0x10; i++) {
        if (Buffer[i + NumberOfBytes + 0x10] != 'E') {
            DbgBreak();
        }
        Buffer[i + NumberOfBytes + 0x10] = '-';
    }

    KeAcquireSpinLock(&Ext2MemoryLock, &Irql);
    Ext2TotalMemorySize = Ext2TotalMemorySize - NumberOfBytes;
    Ext2TotalAllocates -= 1;
    KeReleaseSpinLock(&Ext2MemoryLock, Irql);

    ExFreePoolWithTag(Buffer, Tag);
}

#else  // EXT2_DEBUG

PVOID
Ext2AllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
)
{
    return ExAllocatePoolWithTag(
               PoolType,
               NumberOfBytes,
               Tag);
}

VOID
Ext2FreePool(
    IN PVOID P,
    IN ULONG Tag
)
{
    ExFreePoolWithTag(P, Tag);
}

#endif // !EXT2_DEBUG
