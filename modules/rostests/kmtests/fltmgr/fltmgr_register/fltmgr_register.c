/*
 * PROJECT:         ReactOS kernel-mode tests - Filter Manager
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for checking filter registration
 * PROGRAMMER:      Ged Murphy <gedmurphy@reactos.org>
 */

// This tests needs to be run via a standalone driver because FltRegisterFilter
// uses the DriverObject in its internal structures, and we don't want it to be
// linked to a device object from the test suite itself.

#include <kmt_test.h>
#include <fltkernel.h>
#include <fltmgrint.h>

//#define NDEBUG
#include <debug.h>

#define RESET_REGISTRATION(basic) \
    do { \
        RtlZeroMemory(&FilterRegistration, sizeof(FLT_REGISTRATION)); \
        if (basic) { \
            FilterRegistration.Size = sizeof(FLT_REGISTRATION); \
            FilterRegistration.Version = FLT_REGISTRATION_VERSION; \
        } \
    } while (0)

#define RESET_UNLOAD(DO) DO->DriverUnload = NULL;


NTSTATUS
FLTAPI
TestRegFilterUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

/* Globals */
static PDRIVER_OBJECT TestDriverObject;
static FLT_REGISTRATION FilterRegistration;
static PFLT_FILTER TestFilter = NULL;




BOOLEAN
TestFltRegisterFilter(_In_ PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING Altitude;
    UNICODE_STRING Name;
    PFLT_FILTER Filter = NULL;
    PFLT_FILTER Temp = NULL;
    NTSTATUS Status;

    RESET_REGISTRATION(FALSE);
#if 0
    KmtStartSeh()
        Status = FltRegisterFilter(NULL, &FilterRegistration, &Filter);
    KmtEndSeh(STATUS_INVALID_PARAMETER);

    KmtStartSeh()
        Status = FltRegisterFilter(DriverObject, NULL, &Filter);
    KmtEndSeh(STATUS_INVALID_PARAMETER);

    KmtStartSeh()
        Status = FltRegisterFilter(DriverObject, &FilterRegistration, NULL);
    KmtEndSeh(STATUS_INVALID_PARAMETER)
#endif

    RESET_REGISTRATION(TRUE);
    FilterRegistration.Version = 0x0100;
    Status = FltRegisterFilter(DriverObject, &FilterRegistration, &Filter);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    RESET_REGISTRATION(TRUE);
    FilterRegistration.Version = 0x0300;
    Status = FltRegisterFilter(DriverObject, &FilterRegistration, &Filter);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    RESET_REGISTRATION(TRUE);
    FilterRegistration.Version = 0x0200;
    Status = FltRegisterFilter(DriverObject, &FilterRegistration, &Filter);
    ok_eq_hex(Status, STATUS_SUCCESS);
    FltUnregisterFilter(Filter);


    /* Test invalid sizes. MSDN says this is required, but it doesn't appear to be */
    RESET_REGISTRATION(TRUE);
    FilterRegistration.Size = 0;
    Status = FltRegisterFilter(DriverObject, &FilterRegistration, &Filter);
    ok_eq_hex(Status, STATUS_SUCCESS);
    FltUnregisterFilter(Filter);

    RESET_REGISTRATION(TRUE);
    FilterRegistration.Size = 0xFFFF;
    Status = FltRegisterFilter(DriverObject, &FilterRegistration, &Filter);
    ok_eq_hex(Status, STATUS_SUCCESS);
    FltUnregisterFilter(Filter);


    /* Now make a valid registration */
    RESET_REGISTRATION(TRUE);
    Status = FltRegisterFilter(DriverObject, &FilterRegistration, &Filter);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Try to register again */
    Status = FltRegisterFilter(DriverObject, &FilterRegistration, &Temp);
    ok_eq_hex(Status, STATUS_FLT_INSTANCE_ALTITUDE_COLLISION);


    ok_eq_hex(Filter->Base.Flags, FLT_OBFL_TYPE_FILTER);

    /* Check we have the right filter name */
    RtlInitUnicodeString(&Name, L"Kmtest-FltMgrReg");
    ok_eq_long(RtlCompareUnicodeString(&Filter->Name, &Name, FALSE), 0);

    /* And the altitude is corect */
    RtlInitUnicodeString(&Altitude, L"123456");
    ok_eq_long(RtlCompareUnicodeString(&Filter->DefaultAltitude, &Altitude, FALSE), 0);

    //
    // FIXME: More checks
    //

    /* Cleanup the valid registration */
    FltUnregisterFilter(Filter);

    /*
     * The last thing we'll do before we exit is to properly register with the filter manager
     * and set an unload routine. This'll let us test the FltUnregisterFilter routine
     */
    RESET_REGISTRATION(TRUE);

    /* Set a fake unload routine we'll use to test */
    DriverObject->DriverUnload = (PDRIVER_UNLOAD)0x1234FFFF;

    FilterRegistration.FilterUnloadCallback = TestRegFilterUnload;
    Status = FltRegisterFilter(DriverObject, &FilterRegistration, &TestFilter);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test all the unlod routines */
    ok_eq_pointer(TestFilter->FilterUnload, TestRegFilterUnload);
    ok_eq_pointer(TestFilter->OldDriverUnload, (PFLT_FILTER_UNLOAD_CALLBACK)0x1234FFFF);

    // This should equal the fltmgr's private unload routine, but there's no easy way of testing it...
    //ok_eq_pointer(DriverObject->DriverUnload, FltpMiniFilterDriverUnload);

    /* Make sure our test address is never actually called */
    TestFilter->OldDriverUnload = (PFLT_FILTER_UNLOAD_CALLBACK)NULL;

    return TRUE;
}


NTSTATUS
FLTAPI
TestRegFilterUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags)
{
    //__debugbreak();

    ok_irql(PASSIVE_LEVEL);
    ok(TestFilter != NULL, "Buffer is NULL\n");

    //
    // FIXME: Add tests
    //

    FltUnregisterFilter(TestFilter);

    //
    // FIXME: Add tests
    //

    return STATUS_SUCCESS;
}










/*
 * KMT Callback routines
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

    DPRINT("FltMgrReg Entry!\n");
    trace("Entered FltMgrReg tests\n");

    /* We'll do the work ourselves in this test */
    *Flags = TESTENTRY_NO_ALL;

    ok_irql(PASSIVE_LEVEL);
    TestDriverObject = DriverObject;


    /* Run the tests */
    (VOID)TestFltRegisterFilter(DriverObject);

    return Status;
}

VOID
TestFilterUnload(
    IN ULONG Flags)
{
    PAGED_CODE();
    ok_irql(PASSIVE_LEVEL);
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
    return STATUS_FLT_DO_NOT_ATTACH;
}

VOID
TestQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
}
