/*
 * PROJECT:         ReactOS kernel-mode tests - Filter Manager
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for checking filters load correctly
 * PROGRAMMER:      Ged Murphy <gedmurphy@reactos.org>
 */

#include <kmt_test.h>
#include <fltkernel.h>

//#define NDEBUG
#include <debug.h>

/* prototypes */
NTSTATUS
FLTAPI
TestClientConnect(
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionPortCookie
);

VOID
FLTAPI
TestClientDisconnect(
    _In_opt_ PVOID ConnectionCookie
);

NTSTATUS
FLTAPI
TestMessageHandler(
    _In_opt_ PVOID ConnectionCookie,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength
);


/* Globals */
static PDRIVER_OBJECT TestDriverObject;



/**
 * @name TestEntry
 *
 * Test entry point.
 * This is called by DriverEntry as early as possible, but with ResultBuffer
 * initialized, so that test macros work correctly
 *
 * @param DriverObject
 *        Driver Object.
 *        This is guaranteed not to have been touched by DriverEntry before
 *        the call to TestEntry
 * @param RegistryPath
 *        Driver Registry Path
 *        This is guaranteed not to have been touched by DriverEntry before
 *        the call to TestEntry
 * @param DeviceName
 *        Pointer to receive a test-specific name for the device to create
 * @param Flags
 *        Pointer to a flags variable instructing DriverEntry how to proceed.
 *        See the KMT_TESTENTRY_FLAGS enumeration for possible values
 *        Initialized to zero on entry
 *
 * @return Status.
 *         DriverEntry will fail if this is a failure status
 */
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

    *DeviceName = L"FltMgrLoad";

    trace("Hi, this is the filter manager load test driver\n");

    KmtFilterRegisterComms(TestClientConnect, TestClientDisconnect, TestMessageHandler, 1);

    return Status;
}

/**
 * @name TestUnload
 *
 * Test unload routine.
 * This is called by the driver's Unload routine as early as possible, with
 * ResultBuffer and the test device object still valid, so that test macros
 * work correctly
 *
 * @param DriverObject
 *        Driver Object.
 *        This is guaranteed not to have been touched by Unload before the call
 *        to TestEntry
 *
 * @return Status
 */
VOID
TestFilterUnload(
    IN ULONG Flags)
{
    PAGED_CODE();

    DPRINT("Unload!\n");

    ok_irql(PASSIVE_LEVEL);

    trace("Unloading filter manager load test driver\n");
}


/**
* @name TestInstanceSetup
*
* Test volume attach routine.
* This is called by the driver's InstanceSetupCallback routine in response to
* a new volume attaching.
*
* @param FltObjects
*        Filter Object Pointers
*        Pointer to an FLT_RELATED_OBJECTS structure that contains opaque pointers
*        for the objects related to the current operation
* @param Flags
*        Bitmask of flags that indicate why the instance is being attached
* @param VolumeDeviceType
*        Device type of the file system volume
* @param VolumeFilesystemType
*        File system type of the volume
* @param VolumeName
*        Unicode string containing the name of the volume.
*        The string is only valid within the context of this function
* @param SectorSize
*        Adjusts the sector size to a minimum of 0x200, which is more reliable
* @param ReportedSectorSize
*        Sector size of the volume as reported by the filter manager
*
* @return Status.
*         Return STATUS_SUCCESS to attach or STATUS_FLT_DO_NOT_ATTACH to ignore
*/
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

    /* We're not interested in attaching to any volumes in this test */
    return STATUS_FLT_DO_NOT_ATTACH;
}

/**
* @name TestQueryTeardown
*
* Test volume attach routine.
* This is called by the driver's InstanceSetupCallback routine in response to
* a new volume attaching.
*
* @param FltObjects
*        Filter Object Pointers
*        Pointer to an FLT_RELATED_OBJECTS structure that contains opaque pointers
*        for the objects related to the current operation
* @param Flags
*        Flag that indicates why the minifilter driver instance is being torn down
*
*/
VOID
TestQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
    trace("Received a teardown request, Flags %lu\n", Flags);

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
}

NTSTATUS
FLTAPI
TestClientConnect(
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionPortCookie)
{
    return 0;
}

VOID
FLTAPI
TestClientDisconnect(
    _In_opt_ PVOID ConnectionCookie)
{

}

NTSTATUS
FLTAPI
TestMessageHandler(
    _In_opt_ PVOID ConnectionCookie,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength)
{
    return 0;
}
