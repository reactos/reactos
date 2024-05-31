/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite - ObInsertObject test
 * PROGRAMMER:      Gleb Surikov <glebs.surikovs@gmail.com>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

typedef struct DUMMY_TYPE
{
    ULONG Data;
} DUMMY_TYPE, *PDUMMY_TYPE;

static HANDLE DirectoryHandle;
static UNICODE_STRING DirectoryName;

static POBJECT_TYPE DummyType;
static UNICODE_STRING DummyTypeName;
static OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
static OBJECT_ATTRIBUTES DummyObjectAttributes;

static
VOID
NTAPI
DumpProc(
    _In_ PVOID Object,
    _In_opt_ POB_DUMP_CONTROL DumpControl
)
{
    DPRINT("DumpProc() called\n");
}

static
NTSTATUS
NTAPI
OpenProc(
    _In_ OB_OPEN_REASON OpenReason,
    _In_opt_ PEPROCESS Process,
    _In_ PVOID Object,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG HandleCount
)
{
    DPRINT("OpenProc() 0x%p, OpenReason %d, HandleCount %lu, AccessMask 0x%lX\n",
           Object,
           OpenReason,
           HandleCount,
           GrantedAccess);
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
CloseProc(
    _In_opt_ PEPROCESS Process,
    _In_ PVOID Object,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG ProcessHandleCount,
    _In_ ULONG SystemHandleCount
)
{
    DPRINT("CloseProc() 0x%p, ProcessHandleCount %lu, SystemHandleCount %lu, AccessMask 0x%lX\n",
           Object,
           ProcessHandleCount,
           SystemHandleCount,
           GrantedAccess);
}

static
VOID
NTAPI
DeleteProc(_In_ PVOID Object)
{
    DPRINT("DeleteProc() 0x%p\n", Object);
}

static
NTSTATUS
NTAPI
ParseProc(
    _In_ PVOID ParseObject,
    _In_ PVOID ObjectType,
    _Inout_ PACCESS_STATE AccessState,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ ULONG Attributes,
    _Inout_ PUNICODE_STRING CompleteName,
    _Inout_ PUNICODE_STRING RemainingName,
    _Inout_opt_ PVOID Context,
    _In_opt_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _Out_ PVOID *Object
)
{
    DPRINT("ParseProc() called\n");
    *Object = NULL;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

static
BOOLEAN
NTAPI
OkayToCloseProc(
    _In_opt_ PEPROCESS Process,
    _In_ PVOID Object,
    _In_ HANDLE Handle,
    _In_ KPROCESSOR_MODE AccessMode
)
{
    DPRINT("OkayToCloseProc() 0x%p, Handle 0x%p, AccessMask 0x%lX\n",
           Object,
           Handle,
           AccessMode);
    return TRUE;
}

static
NTSTATUS
NTAPI
QueryNameProc(
    _In_ PVOID Object,
    _In_ BOOLEAN HasObjectName,
    _Out_ POBJECT_NAME_INFORMATION ObjectNameInfo,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
)
{
    DPRINT("QueryNameProc() 0x%p, HasObjectName %d, Len %lu, AccessMask 0x%lX\n",
           Object,
           HasObjectName,
           Length,
           AccessMode);

    ObjectNameInfo = NULL;
    ReturnLength = 0;
    return STATUS_SUCCESS;
}

/*!
 * @brief This function creates a test directory.
 */
static
VOID
ObInsert_CreateTestDirectory(VOID)
{
    NTSTATUS Status;

    /* Initialize the directory name */
    RtlInitUnicodeString(&DirectoryName, L"\\ObInsertTest");

    /* Initialize object attributes for the directory */
    InitializeObjectAttributes(&DummyObjectAttributes,
                               &DirectoryName,
                               OBJ_KERNEL_HANDLE | OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the directory object */
    Status = ZwCreateDirectoryObject(&DirectoryHandle,
                                     DELETE,
                                     &DummyObjectAttributes);

    ok_eq_hex(Status, STATUS_SUCCESS);
}

/*!
 * @brief This function creates dummy object type.
 */
static
VOID
ObInsert_CreateDummyType(VOID)
{
    NTSTATUS Status;

    /* Initialize the dummy type name */
    RtlInitUnicodeString(&DummyTypeName, L"ObInsertDummyType");

    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));

    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;
    ObjectTypeInitializer.MaintainHandleCount = TRUE;

    /* Create the dummy object type (expected to fail initially) */
    Status = ObCreateObjectType(&DummyTypeName,
                                &ObjectTypeInitializer,
                                NULL,
                                &DummyType);

    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    ObjectTypeInitializer.CloseProcedure = CloseProc;
    ObjectTypeInitializer.DeleteProcedure = DeleteProc;
    ObjectTypeInitializer.DumpProcedure = DumpProc;
    ObjectTypeInitializer.OpenProcedure = OpenProc;
    ObjectTypeInitializer.ParseProcedure = ParseProc;
    ObjectTypeInitializer.OkayToCloseProcedure = OkayToCloseProc;
    ObjectTypeInitializer.QueryNameProcedure = QueryNameProc;

    /* Create the dummy object type again with proper procedures */
    Status = ObCreateObjectType(&DummyTypeName,
                                &ObjectTypeInitializer,
                                NULL,
                                &DummyType);

    /* Handle object name collision status */
    if (Status == STATUS_OBJECT_NAME_COLLISION)
    {
        UNICODE_STRING ObjectPath;
        OBJECT_ATTRIBUTES ObjectAttributes;
        HANDLE ObjectTypeHandle;

        /* Initialize the object path for the existing object type */
        RtlInitUnicodeString(&ObjectPath, L"\\ObjectTypes\\ObInsertDummyType");

        /* Initialize object attributes for the object type */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &ObjectPath,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        /* Open the existing object type by name */
        Status = ObOpenObjectByName(&ObjectAttributes,
                                    NULL,
                                    KernelMode,
                                    NULL,
                                    0,
                                    NULL,
                                    &ObjectTypeHandle);

        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(ObjectTypeHandle != NULL, "ObjectTypeHandle is NULL\n");

        if (!skip(Status == STATUS_SUCCESS && ObjectTypeHandle, "No handle\n"))
        {
            /* Reference the existing object type by handle */
            Status = ObReferenceObjectByHandle(ObjectTypeHandle,
                                               0,
                                               NULL,
                                               KernelMode,
                                               (PVOID)&DummyType,
                                               NULL);

            ok_eq_hex(Status, STATUS_SUCCESS);

            if (!skip(Status == STATUS_SUCCESS && DummyType, "No pointer to the dummy object's body\n"))
            {
                /* Set the procedures for the existing object type */
                DummyType->TypeInfo.CloseProcedure = CloseProc;
                DummyType->TypeInfo.DeleteProcedure = DeleteProc;
                DummyType->TypeInfo.DumpProcedure = DumpProc;
                DummyType->TypeInfo.OpenProcedure = OpenProc;
                DummyType->TypeInfo.ParseProcedure = ParseProc;
                DummyType->TypeInfo.OkayToCloseProcedure = OkayToCloseProc;
                DummyType->TypeInfo.QueryNameProcedure = QueryNameProc;
            }

            /* Close the object type handle */
            if (ObjectTypeHandle != NULL)
            {
                Status = ZwClose(ObjectTypeHandle);
            }
        }
    }

    ok_eq_hex(Status, STATUS_SUCCESS);
}

/*!
 * @brief This function cleans up resources that were not cleaned up during the tests.
 */
static
VOID
ObInsert_Cleanup(VOID)
{
    NTSTATUS Status;

    /* Make the directory object temporary (which is permanent) if the handle is valid */
    if (!skip(DirectoryHandle != NULL, "No directory handle\n"))
    {
        Status = ZwMakeTemporaryObject(DirectoryHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);

        /* Close the directory handle */
        Status = ZwClose(DirectoryHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
}

/*!
 * @brief This function tests for successful object insertion.
 */
static
VOID
ObInsert_Success(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING ObjectName;
    PDUMMY_TYPE Object;
    HANDLE Handle;
    POBJECT_HEADER ObjectHeader;

    /* Initialize the object name */
    RtlInitUnicodeString(&ObjectName,
                         L"\\ObInsertTest\\DummyObject_Success");

    /* Initialize object attributes with the name */
    InitializeObjectAttributes(&DummyObjectAttributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the object */
    Status = ObCreateObject(KernelMode,
                            DummyType,
                            &DummyObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(DUMMY_TYPE),
                            0,
                            0,
                            (PVOID *)&Object);

    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Initialize the object data */
    RtlZeroMemory(Object, sizeof(DUMMY_TYPE));
    Object->Data = 123;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

    ok_eq_ulong(ObjectHeader->PointerCount, 1LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 0LU);

    /* Insert the object */
    Status = ObInsertObject(Object,
                            NULL,
                            STANDARD_RIGHTS_ALL,
                            0,
                            NULL,
                            &Handle);

    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(Object != NULL, "Object is NULL\n");
    ok(Handle != NULL, "Handle is NULL\n");

    ok_eq_ulong(ObjectHeader->PointerCount, 2LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 1LU);

    ObDereferenceObject(Object);

    Status = ZwClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);
}

/*!
 * @brief This function tests STATUS_OBJECT_PATH_NOT_FOUND.
 */
static
VOID
ObInsert_PathNotFound(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING ObjectName;
    PDUMMY_TYPE Object;
    HANDLE Handle;
    POBJECT_HEADER ObjectHeader;

    /* Initialize the object name. To simulate STATUS_OBJECT_PATH_NOT_FOUND, provide
       an invalid non-last path component ("ObInsertTestt") */
    RtlInitUnicodeString(&ObjectName,
                         L"\\ObInsertTestt\\DummyObject_PathNotFound");

    /* Initialize object attributes with the name */
    InitializeObjectAttributes(&DummyObjectAttributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the object */
    Status = ObCreateObject(KernelMode,
                            DummyType,
                            &DummyObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(DUMMY_TYPE),
                            0,
                            0,
                            (PVOID *)&Object);

    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Initialize the object data */
    RtlZeroMemory(Object, sizeof(DUMMY_TYPE));
    Object->Data = 123;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

    /* Check the object handles before insertion */
    ok_eq_ulong(ObjectHeader->PointerCount, 1LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 0LU);

    /* Insert the object */
    Status = ObInsertObject(Object,
                            NULL,
                            STANDARD_RIGHTS_ALL,
                            0,
                            NULL,
                            &Handle);

    /* STATUS_OBJECT_PATH_NOT_FOUND because "ObInsertTestt" doesn't exist, but there are
       remaining components of the path */
    ok_eq_hex(Status, STATUS_OBJECT_PATH_NOT_FOUND);

    /* Check the object handles after invalid insertion. ObInsertObject should've
       automatically dereferenced the object */
    ok_eq_ulong(ObjectHeader->PointerCount, 0LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 0LU);
}

/*!
 * @brief This function tests STATUS_OBJECT_NAME_INVALID.
 */
static
VOID
ObInsert_NameInvalid(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING ObjectName;
    PDUMMY_TYPE Object;
    HANDLE Handle;
    POBJECT_HEADER ObjectHeader;

    /* Initialize the object name. To simulate STATUS_OBJECT_NAME_INVALID, provide
       an invalid path end component */
    RtlInitUnicodeString(&ObjectName,
                         L"\\ObInsertTest\\");

    /* Initialize object attributes with the name */
    InitializeObjectAttributes(&DummyObjectAttributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the object */
    Status = ObCreateObject(KernelMode,
                            DummyType,
                            &DummyObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(DUMMY_TYPE),
                            0,
                            0,
                            (PVOID *)&Object);

    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Initialize the object data */
    RtlZeroMemory(Object, sizeof(DUMMY_TYPE));
    Object->Data = 123;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

    /* Check the object handles before insertion */
    ok_eq_ulong(ObjectHeader->PointerCount, 1LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 0LU);

    /* Insert the object */
    Status = ObInsertObject(Object,
                            NULL,
                            STANDARD_RIGHTS_ALL,
                            0,
                            NULL,
                            &Handle);

    ok_eq_hex(Status, STATUS_OBJECT_NAME_INVALID);

    /* Check the object handles after invalid insertion. ObInsertObject should've
       automatically dereferenced the object */
    ok_eq_ulong(ObjectHeader->PointerCount, 0LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 0LU);
}

/*!
 * @brief This function tests STATUS_OBJECT_PATH_SYNTAX_BAD.
 */
static
VOID
ObInsert_PathSyntaxBad(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING ObjectName;
    PDUMMY_TYPE Object;
    HANDLE Handle;
    POBJECT_HEADER ObjectHeader;

    /* Initialize the object name. To simulate STATUS_OBJECT_PATH_SYNTAX_BAD, provide
       a path that doesn't start with "\" */
    RtlInitUnicodeString(&ObjectName,
                         L"ObInsertTest");

    /* Initialize object attributes with the name */
    InitializeObjectAttributes(&DummyObjectAttributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the object */
    Status = ObCreateObject(KernelMode,
                            DummyType,
                            &DummyObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(DUMMY_TYPE),
                            0,
                            0,
                            (PVOID *)&Object);

    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Initialize the object data */
    RtlZeroMemory(Object, sizeof(DUMMY_TYPE));
    Object->Data = 123;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

    /* Check the object handles before insertion */
    ok_eq_ulong(ObjectHeader->PointerCount, 1LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 0LU);

    /* Insert the object */
    Status = ObInsertObject(Object,
                            NULL,
                            STANDARD_RIGHTS_ALL,
                            0,
                            NULL,
                            &Handle);

    ok_eq_hex(Status, STATUS_OBJECT_PATH_SYNTAX_BAD);

    /* Check the object handles after invalid insertion. ObInsertObject should've
       automatically dereferenced the object */
    ok_eq_ulong(ObjectHeader->PointerCount, 0LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 0LU);
}

/*!
 * @brief This function tests STATUS_OBJECT_NAME_COLLISION.
 */
static
VOID
ObInsert_NameCollision(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING ObjectName;
    PDUMMY_TYPE Object;
    HANDLE Handle;
    POBJECT_HEADER ObjectHeader;

    /* Initialize the object name. To simulate STATUS_OBJECT_NAME_COLLISION, provide
       a name that already exists */
    RtlInitUnicodeString(&ObjectName,
                         L"\\");

    /* Initialize object attributes with the name. To simulate STATUS_OBJECT_NAME_COLLISION and
       avoid STATUS_OBJECT_NAME_EXISTS don't specify OBJ_OPENIF */
    InitializeObjectAttributes(&DummyObjectAttributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the object */
    Status = ObCreateObject(KernelMode,
                            DummyType,
                            &DummyObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(DUMMY_TYPE),
                            0,
                            0,
                            (PVOID *)&Object);

    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Initialize the object data */
    RtlZeroMemory(Object, sizeof(DUMMY_TYPE));
    Object->Data = 123;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

    /* Check the object handles before insertion */
    ok_eq_ulong(ObjectHeader->PointerCount, 1LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 0LU);

    /* Insert the object */
    Status = ObInsertObject(Object,
                            NULL,
                            STANDARD_RIGHTS_ALL,
                            0,
                            NULL,
                            &Handle);

    ok_eq_hex(Status, STATUS_OBJECT_NAME_COLLISION);

    /* Check the object handles after invalid insertion. ObInsertObject should've
       automatically dereferenced the object */
    ok_eq_ulong(ObjectHeader->PointerCount, 0LU);
    ok_eq_ulong(ObjectHeader->HandleCount, 0LU);
}

START_TEST(ObInsert)
{
    /*
     * Initialize a dummy object type
     */
    ObInsert_CreateDummyType();

    /*
     * Create a directory where the dummy objects will be stored
     */
    ObInsert_CreateTestDirectory();

    /*
     * Test a success case
     */
    ObInsert_Success();

    /*
     * Test failure cases
     */
    ObInsert_PathNotFound();
    ObInsert_NameInvalid();
    ObInsert_PathSyntaxBad();
    ObInsert_NameCollision();

    /*
     * Clean up resources and objects created during the tests
     */
    ObInsert_Cleanup();
}
