/*
 * PROJECT:         ReactOS kernel-mode tests - Filter Manager
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for checking the create operations
 * PROGRAMMER:      Ged Murphy <gedmurphy@reactos.org>
 */

#include <kmt_test.h>
#include <fltkernel.h>

//#define NDEBUG
#include <debug.h>

/* prototypes */
static
FLT_PREOP_CALLBACK_STATUS
FLTAPI
TestFilterPreOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
);

static
FLT_POSTOP_CALLBACK_STATUS
FLTAPI
TestFilterPostOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);


/* Globals */
static PDRIVER_OBJECT TestDriverObject;


CONST FLT_OPERATION_REGISTRATION Callbacks[] =
{
    { IRP_MJ_CREATE,
      0,
      TestFilterPreOperation,
      TestFilterPostOperation },

    { IRP_MJ_OPERATION_END }
};


NTSTATUS
TestEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PCUNICODE_STRING RegistryPath,
    OUT PCWSTR *DeviceName,
    IN OUT INT *Flags)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(Flags);

    DPRINT("Entry!\n");

    ok_irql(PASSIVE_LEVEL);
    TestDriverObject = DriverObject;

    *DeviceName = L"fltmgr_create";

    trace("Hi, this is the filter manager create test driver\n");

    (VOID)KmtFilterRegisterCallbacks(Callbacks);

    return Status;
}

VOID
TestFilterUnload(
    IN ULONG Flags)
{
    PAGED_CODE();

    DPRINT("Unload!\n");

    ok_irql(PASSIVE_LEVEL);

    trace("Unloading filter manager test driver\n");
}

VOID
TestQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
}

NTSTATUS
TestInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType,
    _In_ PUNICODE_STRING VolumeName,
    _In_ ULONG SectorSize,
    _In_ ULONG ReportedSectorSize
)
{
    trace("Received an attach request for VolumeType 0x%X, FileSystemType %d\n",
          VolumeDeviceType,
          VolumeFilesystemType);

    return STATUS_FLT_DO_NOT_ATTACH;
}

static
FLT_PREOP_CALLBACK_STATUS
FLTAPI
TestFilterPreOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext)
{
    PFLT_IO_PARAMETER_BLOCK Iopb = Data->Iopb;

    ok_eq_hex(Iopb->MajorFunction, IRP_MJ_CREATE);

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

static
FLT_POSTOP_CALLBACK_STATUS
FLTAPI
TestFilterPostOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags)
{
    PFLT_IO_PARAMETER_BLOCK Iopb = Data->Iopb;

    ok_eq_hex(Iopb->MajorFunction, IRP_MJ_CREATE);

    return FLT_POSTOP_FINISHED_PROCESSING;
}
