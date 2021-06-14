/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test driver for reparse point operations
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#include "IoCreateFile.h"

typedef struct _TEST_FCB
{
    FSRTL_ADVANCED_FCB_HEADER Header;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    FAST_MUTEX HeaderMutex;
} TEST_FCB, *PTEST_FCB;

static KMT_IRP_HANDLER TestIrpHandler;
static KMT_MESSAGE_HANDLER TestMessageHandler;

static PFILE_OBJECT TestFileObject;
static PDEVICE_OBJECT TestDeviceObject;

NTSTATUS
TestEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCUNICODE_STRING RegistryPath,
    _Out_ PCWSTR *DeviceName,
    _Inout_ INT *Flags)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    *DeviceName = L"IoCreateFile";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE |
             TESTENTRY_BUFFERED_IO_DEVICE |
             TESTENTRY_NO_READONLY_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_CREATE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_CLEANUP, NULL, TestIrpHandler);
    KmtRegisterMessageHandler(0, NULL, TestMessageHandler);

    return Status;
}

VOID
TestUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();
}

static volatile long gNoLinks = FALSE;

static
NTSTATUS
TestIrpHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;
    PTEST_FCB Fcb;
    CACHE_UNINITIALIZE_EVENT CacheUninitEvent;

    PAGED_CODE();

    DPRINT("IRP %x/%x\n", IoStack->MajorFunction, IoStack->MinorFunction);
    ASSERT(IoStack->MajorFunction == IRP_MJ_CREATE ||
           IoStack->MajorFunction == IRP_MJ_CLEANUP);

    Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    if (IoStack->MajorFunction == IRP_MJ_CREATE)
    {
        ok((IoStack->Parameters.Create.Options & FILE_OPEN_REPARSE_POINT) == 0, "FILE_OPEN_REPARSE_POINT set\n");
        ok((IoStack->Flags == 0 && !gNoLinks) || (IoStack->Flags == SL_STOP_ON_SYMLINK && gNoLinks), "IoStack->Flags = %lx\n", IoStack->Flags);

        if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR))
        {
            TestDeviceObject = DeviceObject;
            TestFileObject = IoStack->FileObject;
        }
        if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR) &&
            IoStack->FileObject->FileName.Buffer[1] == 'M')
        {
            PREPARSE_DATA_BUFFER Reparse;

            Irp->Tail.Overlay.AuxiliaryBuffer = ExAllocatePoolZero(NonPagedPool, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, 'FwrI');
            Reparse = (PREPARSE_DATA_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;

            if (!Reparse)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Finish;
            }

            Reparse->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
            Reparse->ReparseDataLength = 12 + sizeof(L"\\??\\C:\\Documents and Settings");
            Reparse->MountPointReparseBuffer.SubstituteNameLength = sizeof(L"\\??\\C:\\Documents and Settings") - sizeof(UNICODE_NULL);
            Reparse->MountPointReparseBuffer.PrintNameOffset = sizeof(L"\\??\\C:\\Documents and Settings");
            RtlCopyMemory(Reparse->MountPointReparseBuffer.PathBuffer, L"\\??\\C:\\Documents and Settings", sizeof(L"\\??\\C:\\Documents and Settings"));
            Irp->IoStatus.Information = IO_REPARSE_TAG_MOUNT_POINT;
            Status = STATUS_REPARSE;
        }
        else if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR) &&
            IoStack->FileObject->FileName.Buffer[1] == 'S')
        {
            PREPARSE_DATA_BUFFER Reparse;

            if (IoStack->Flags & SL_STOP_ON_SYMLINK)
            {
                Status = STATUS_STOPPED_ON_SYMLINK;
                goto Finish;
            }

            Irp->Tail.Overlay.AuxiliaryBuffer = ExAllocatePoolZero(NonPagedPool, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, 'FwrI');
            Reparse = (PREPARSE_DATA_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;

            if (!Reparse)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Finish;
            }

            Reparse->ReparseTag = IO_REPARSE_TAG_SYMLINK;
            Reparse->ReparseDataLength = 12 + sizeof(L"\\??\\C:\\Documents and Settings");
            Reparse->SymbolicLinkReparseBuffer.SubstituteNameLength = sizeof(L"\\??\\C:\\Documents and Settings") - sizeof(UNICODE_NULL);
            Reparse->SymbolicLinkReparseBuffer.PrintNameOffset = sizeof(L"\\??\\C:\\Documents and Settings");
            RtlCopyMemory(Reparse->SymbolicLinkReparseBuffer.PathBuffer, L"\\??\\C:\\Documents and Settings", sizeof(L"\\??\\C:\\Documents and Settings"));
            Irp->IoStatus.Information = IO_REPARSE_TAG_SYMLINK;
            Status = STATUS_REPARSE;
        }
        else
        {
            Fcb = ExAllocatePoolZero(NonPagedPool, sizeof(*Fcb), 'FwrI');
            
            if (!Fcb)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Finish;
            }

            ExInitializeFastMutex(&Fcb->HeaderMutex);
            FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);
            Fcb->Header.AllocationSize.QuadPart = 0;
            Fcb->Header.FileSize.QuadPart = 0;
            Fcb->Header.ValidDataLength.QuadPart = 0;
            IoStack->FileObject->FsContext = Fcb;
            IoStack->FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;

            Irp->IoStatus.Information = FILE_OPENED;
            Status = STATUS_SUCCESS;
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_CLEANUP)
    {
        KeInitializeEvent(&CacheUninitEvent.Event, NotificationEvent, FALSE);
        CcUninitializeCacheMap(IoStack->FileObject, NULL, &CacheUninitEvent);
        KeWaitForSingleObject(&CacheUninitEvent.Event, Executive, KernelMode, FALSE, NULL);
        Fcb = IoStack->FileObject->FsContext;
        ExFreePoolWithTag(Fcb, 'FwrI');
        IoStack->FileObject->FsContext = NULL;
        Status = STATUS_SUCCESS;
    }

Finish:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static UNICODE_STRING FileObjectFileName = RTL_CONSTANT_STRING(L"\\NonSymlinked");
static UNICODE_STRING DocumentsAndSettings = RTL_CONSTANT_STRING(L"\\Documents and Settings");

static
NTSTATUS
TestIoCreateFile(
    IN PUNICODE_STRING Path)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;

    InitializeObjectAttributes(&ObjectAttributes,
                               Path,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = IoCreateFile(&Handle,
                          GENERIC_READ,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_NO_PARAMETER_CHECKING | (gNoLinks ? IO_STOP_ON_SYMLINK : 0));
    if (NT_SUCCESS(Status))
    {
        NTSTATUS IntStatus;

        IntStatus = ObReferenceObjectByHandle(Handle,
                                              FILE_READ_DATA,
                                              *IoFileObjectType,
                                              KernelMode,
                                              (PVOID *)&FileObject,
                                              NULL);
        ok_eq_hex(IntStatus, STATUS_SUCCESS);
        if (NT_SUCCESS(IntStatus))
        {
            ok(RtlCompareUnicodeString(&FileObjectFileName, &FileObject->FileName, TRUE) == 0 ||
               RtlCompareUnicodeString(&DocumentsAndSettings, &FileObject->FileName, TRUE) == 0,
               "Expected: %wZ or %wZ. Opened: %wZ\n", &FileObjectFileName, &DocumentsAndSettings, &FileObject->FileName);
            ObDereferenceObject(FileObject);
        }

        IntStatus = ObCloseHandle(Handle, KernelMode);
        ok_eq_hex(IntStatus, STATUS_SUCCESS);
    }

    return Status;
}

static
NTSTATUS
TestMessageHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlCode,
    IN PVOID Buffer OPTIONAL,
    IN SIZE_T InLength,
    IN OUT PSIZE_T OutLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    switch (ControlCode)
    {
        case IOCTL_DISABLE_SYMLINK:
        {
            if (InterlockedExchange(&gNoLinks, TRUE) == TRUE)
            {
                Status = STATUS_UNSUCCESSFUL;
            }

            break;
        }
        case IOCTL_CALL_CREATE:
        {
            ANSI_STRING Path;
            UNICODE_STRING PathW;

            ok(Buffer != NULL, "Buffer is NULL\n");
            Path.Length = Path.MaximumLength = (USHORT)InLength;
            Path.Buffer = Buffer;

            Status = RtlAnsiStringToUnicodeString(&PathW, &Path, TRUE);
            ok_eq_hex(Status, STATUS_SUCCESS);

            Status = TestIoCreateFile(&PathW);

            RtlFreeUnicodeString(&PathW);

            break;
        }
        default:
            return STATUS_NOT_SUPPORTED;
    }

    return Status;
}
