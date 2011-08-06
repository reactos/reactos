/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Ob Regressions KM-Test
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 *                  Thomas Faber <thfabba@gmx.de>
 */

/* TODO: split this into multiple tests! ObLifetime, ObHandle, ObName, ... */

#include <kmt_test.h>

//#define NDEBUG
#include <debug.h>

#define CheckObject(Handle, PCount, HCount) do                      \
{                                                                   \
    PUBLIC_OBJECT_BASIC_INFORMATION ObjectInfo;                     \
    Status = ZwQueryObject(Handle, ObjectBasicInformation,          \
                            &ObjectInfo, sizeof ObjectInfo, NULL);  \
    ok_eq_hex(Status, STATUS_SUCCESS);                              \
    ok_eq_ulong(ObjectInfo.PointerCount, PCount);                   \
    ok_eq_ulong(ObjectInfo.HandleCount, HCount);                    \
} while (0)

#define NUM_OBTYPES 5

typedef struct _MY_OBJECT1
{
    ULONG Something1;
} MY_OBJECT1, *PMY_OBJECT1;

typedef struct _MY_OBJECT2
{
    ULONG Something1;
    ULONG SomeLong[10];
} MY_OBJECT2, *PMY_OBJECT2;

static POBJECT_TYPE            ObTypes[NUM_OBTYPES];
static UNICODE_STRING          ObTypeName[NUM_OBTYPES];
static UNICODE_STRING          ObName[NUM_OBTYPES];
static OBJECT_TYPE_INITIALIZER ObTypeInitializer[NUM_OBTYPES];
static UNICODE_STRING          ObDirectoryName;
static OBJECT_ATTRIBUTES       ObDirectoryAttributes;
static OBJECT_ATTRIBUTES       ObAttributes[NUM_OBTYPES];
static PVOID                   ObBody[NUM_OBTYPES];
static HANDLE                  ObHandle1[NUM_OBTYPES];
static HANDLE                  ObHandle2[NUM_OBTYPES];
static HANDLE                  DirectoryHandle;

static USHORT                  DumpCount, OpenCount, CloseCount, DeleteCount,
                               ParseCount, OkayToCloseCount, QueryNameCount;

static
VOID
NTAPI
DumpProc(
    IN PVOID Object,
    IN POB_DUMP_CONTROL DumpControl)
{
    DPRINT("DumpProc() called\n");
    DumpCount++;
}

static
NTSTATUS
NTAPI
OpenProc(
    IN OB_OPEN_REASON OpenReason,
    IN PEPROCESS Process,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG HandleCount)
{
    DPRINT("OpenProc() 0x%p, OpenReason %d, HandleCount %lu, AccessMask 0x%lX\n",
        Object, OpenReason, HandleCount, GrantedAccess);
    OpenCount++;
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
CloseProc(
    IN PEPROCESS Process,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount)
{
    DPRINT("CloseProc() 0x%p, ProcessHandleCount %lu, SystemHandleCount %lu, AccessMask 0x%lX\n",
        Object, ProcessHandleCount, SystemHandleCount, GrantedAccess);
    CloseCount++;
}

static
VOID
NTAPI
DeleteProc(
    IN PVOID Object)
{
    DPRINT("DeleteProc() 0x%p\n", Object);
    DeleteCount++;
}

static
NTSTATUS
NTAPI
ParseProc(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object)
{
    DPRINT("ParseProc() called\n");
    *Object = NULL;

    ParseCount++;
    return STATUS_OBJECT_NAME_NOT_FOUND;//STATUS_SUCCESS;
}

#if 0
static
NTSTATUS
NTAPI
OkayToCloseProc(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN HANDLE Handle,
    IN KPROCESSOR_MODE AccessMode)
{
    DPRINT("OkayToCloseProc() 0x%p, Handle 0x%p, AccessMask 0x%lX\n",
        Object, Handle, AccessMode);
    OkayToCloseCount++;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
QueryNameProc(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength,
    IN KPROCESSOR_MODE AccessMode)
{
    DPRINT("QueryNameProc() 0x%p, HasObjectName %d, Len %lu, AccessMask 0x%lX\n",
        Object, HasObjectName, Length, AccessMode);
    QueryNameCount++;

    ObjectNameInfo = NULL;
    ReturnLength = 0;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}
#endif /* 0 */

static
VOID
ObtCreateObjectTypes(VOID)
{
    INT i;
    NTSTATUS Status;
    WCHAR Name[15];

    for (i = 0; i < NUM_OBTYPES; i++)
    {
        Status = RtlStringCbPrintfW(Name, sizeof Name, L"MyObjectType%x", i);
        ASSERT(NT_SUCCESS(Status));
        RtlInitUnicodeString(&ObTypeName[i], Name);

        RtlZeroMemory(&ObTypeInitializer[i], sizeof ObTypeInitializer[i]);
        ObTypeInitializer[i].Length = sizeof ObTypeInitializer[i];
        ObTypeInitializer[i].PoolType = NonPagedPool;
        ObTypeInitializer[i].MaintainHandleCount = TRUE;
        ObTypeInitializer[i].ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;

        // Test for invalid parameter
        // FIXME: Make it more exact, to see which params Win2k3 checks
        // existence of
        Status = ObCreateObjectType(&ObTypeName[i], &ObTypeInitializer[i], NULL, &ObTypes[i]);
        ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

        ObTypeInitializer[i].CloseProcedure = CloseProc;
        ObTypeInitializer[i].DeleteProcedure = DeleteProc;
        ObTypeInitializer[i].DumpProcedure = DumpProc;
        ObTypeInitializer[i].OpenProcedure = OpenProc;
        ObTypeInitializer[i].ParseProcedure = ParseProc;
        //ObTypeInitializer[i].OkayToCloseProcedure = OkayToCloseProc;
        //ObTypeInitializer[i].QueryNameProcedure = QueryNameProc;
        //ObTypeInitializer[i].SecurityProcedure = SecurityProc;

        Status = ObCreateObjectType(&ObTypeName[i], &ObTypeInitializer[i], NULL, &ObTypes[i]);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
}

static
VOID
ObtCreateDirectory(VOID)
{
    NTSTATUS Status;

    RtlInitUnicodeString(&ObDirectoryName, L"\\ObtDirectory");
    InitializeObjectAttributes(&ObDirectoryAttributes, &ObDirectoryName, OBJ_PERMANENT | OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = ZwCreateDirectoryObject(&DirectoryHandle, DELETE, &ObDirectoryAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckObject(DirectoryHandle, 3LU, 1LU);
}

#define CheckCounts(Open, Close, Delete, Parse, OkayToClose, QueryName) do  \
{                                                                           \
    ok_eq_uint(OpenCount, Open);                                            \
    ok_eq_uint(CloseCount, Close);                                          \
    ok_eq_uint(DeleteCount, Delete);                                        \
    ok_eq_uint(ParseCount, Parse);                                          \
    ok_eq_uint(OkayToCloseCount, OkayToClose);                              \
    ok_eq_uint(QueryNameCount, QueryName);                                  \
} while (0)

#define SaveCounts(Open, Close, Delete, Parse, OkayToClose, QueryName) do   \
{                                                                           \
    Open = OpenCount;                                                       \
    Close = CloseCount;                                                     \
    Delete = DeleteCount;                                                   \
    Parse = ParseCount;                                                     \
    OkayToClose = OkayToCloseCount;                                         \
    QueryName = QueryNameCount;                                             \
} while (0)

/* TODO: make this the same as NUM_OBTYPES */
#define NUM_OBTYPES2 2
static
VOID
ObtCreateObjects(VOID)
{
    PVOID ObBody1[2] = { NULL };
    NTSTATUS Status;
    WCHAR Name[NUM_OBTYPES2][MAX_PATH];
    USHORT OpenSave, CloseSave, DeleteSave, ParseSave, OkayToCloseSave, QueryNameSave;
    INT i;
    ACCESS_MASK Access[NUM_OBTYPES2] = { STANDARD_RIGHTS_ALL, GENERIC_ALL };
    ULONG ObjectSize[NUM_OBTYPES2] = { sizeof(MY_OBJECT1), sizeof(MY_OBJECT2) };

    // Create two objects
    for (i = 0; i < NUM_OBTYPES2; ++i)
    {
        ASSERT(sizeof Name[i] == MAX_PATH * sizeof(WCHAR));
        Status = RtlStringCbPrintfW(Name[i], sizeof Name[i], L"\\ObtDirectory\\MyObject%d", i + 1);
        ASSERT(Status == STATUS_SUCCESS);
        RtlInitUnicodeString(&ObName[i], Name[i]);
        InitializeObjectAttributes(&ObAttributes[i], &ObName[i], OBJ_CASE_INSENSITIVE, NULL, NULL);
    }

    for (i = 0; i < NUM_OBTYPES2; ++i)
    {
        Status = ObCreateObject(KernelMode, ObTypes[i], &ObAttributes[i], KernelMode, NULL, ObjectSize[i], 0L, 0L, &ObBody[i]);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    SaveCounts(OpenSave, CloseSave, DeleteSave, ParseSave, OkayToCloseSave, QueryNameSave);

    // Insert them
    for (i = 0; i < NUM_OBTYPES2; ++i)
    {
        Status = ObInsertObject(ObBody[i], NULL, Access[i], 0, &ObBody[i], &ObHandle1[i]);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(ObBody[i] != NULL, "Object body = NULL\n");
        ok(ObHandle1[i] != NULL, "Handle = NULL\n");
        CheckObject(ObHandle1[i], 3LU, 1LU);
        CheckCounts(OpenSave + 1, CloseSave, DeleteSave, ParseSave, OkayToCloseSave, QueryNameSave);
        SaveCounts(OpenSave, CloseSave, DeleteSave, ParseSave, OkayToCloseSave, QueryNameSave);
    }

    // Now create an object of type 0, of the same name and expect it to fail
    // inserting, but success creation
    ok_eq_wstr(ObName[0].Buffer, L"\\ObtDirectory\\MyObject1");
    RtlInitUnicodeString(&ObName[0], L"\\ObtDirectory\\MyObject1");
    InitializeObjectAttributes(&ObAttributes[0], &ObName[0], OBJ_OPENIF, NULL, NULL);

    Status = ObCreateObject(KernelMode, ObTypes[0], &ObAttributes[0], KernelMode, NULL, sizeof(MY_OBJECT1), 0L, 0L, &ObBody1[0]);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckCounts(OpenSave, CloseSave, DeleteSave, ParseSave, OkayToCloseSave, QueryNameSave);

    Status = ObInsertObject(ObBody1[0], NULL, GENERIC_ALL, 0, &ObBody1[1], &ObHandle2[0]);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED/*STATUS_OBJECT_NAME_EXISTS*/);
    ok_eq_pointer(ObBody[0], ObBody1[1]);
    ok(ObHandle2[0] != NULL, "NULL handle returned\n");

    DPRINT("%d %d %d %d %d %d %d\n", DumpCount, OpenCount, CloseCount, DeleteCount, ParseCount, OkayToCloseCount, QueryNameCount);
    CheckCounts(OpenSave + 1, CloseSave, DeleteSave + 1, ParseSave, OkayToCloseSave, QueryNameSave);
    SaveCounts(OpenSave, CloseSave, DeleteSave, ParseSave, OkayToCloseSave, QueryNameSave);

    // Close its handle
    if (!skip(ObHandle2[0] && ObHandle2[0] != INVALID_HANDLE_VALUE, "Nothing to close\n"))
    {
        Status = ZwClose(ObHandle2[0]);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckCounts(OpenSave, CloseSave + 1, DeleteSave, ParseSave, OkayToCloseSave, QueryNameSave);
        SaveCounts(OpenSave, CloseSave, DeleteSave, ParseSave, OkayToCloseSave, QueryNameSave);
    }

    // Object referenced 2 times:
    // 1) ObInsertObject
    // 2) AdditionalReferences
    if (ObBody1[1])
        ObDereferenceObject(ObBody1[1]);
    //DPRINT1("%d %d %d %d %d %d %d\n", DumpCount, OpenCount, CloseCount, DeleteCount, ParseCount, OkayToCloseCount, QueryNameCount);
    CheckCounts(OpenSave, CloseSave, DeleteSave, ParseSave, OkayToCloseSave, QueryNameSave);
}

static
VOID
ObtClose(
    BOOLEAN Clean,
    BOOLEAN AlternativeMethod)
{
    PVOID DirObject;
    NTSTATUS Status;
    PVOID TypeObject;
    INT i;
    UNICODE_STRING ObPathName[NUM_OBTYPES];
    WCHAR Name[MAX_PATH];

    // Close what we have opened and free what we allocated
    for (i = 0; i < NUM_OBTYPES2; ++i)
    {
        if (ObBody[i])
        {
            if (ObHandle1[i]) CheckObject(ObHandle1[i], 3LU, 1LU);
            ObDereferenceObject(ObBody[i]);
            if (ObHandle1[i]) CheckObject(ObHandle1[i], 2LU, 1LU);
            ObBody[i] = NULL;
        }
        if (ObHandle1[i])
        {
            ZwClose(ObHandle1[i]);
            ObHandle1[i] = NULL;
        }
        if (ObHandle2[i])
        {
            ZwClose(ObHandle2[i]);
            ObHandle2[i] = NULL;
        }
    }

    if (!Clean)
        return;

    // Now we have to get rid of a directory object
    // Since it is permanent, we have to firstly make it temporary
    // and only then kill
    // (this procedure is described in DDK)
    if (DirectoryHandle && DirectoryHandle != INVALID_HANDLE_VALUE)
    {
        CheckObject(DirectoryHandle, 3LU, 1LU);
        Status = ObReferenceObjectByHandle(DirectoryHandle, 0L, NULL, KernelMode, &DirObject, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckObject(DirectoryHandle, 4LU, 1LU);

        Status = ZwMakeTemporaryObject(DirectoryHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckObject(DirectoryHandle, 4LU, 1LU);
        
        // Dereference 2 times - first for just previous referencing
        // and 2nd time for creation of permanent object itself
        ObDereferenceObject(DirObject);
        CheckObject(DirectoryHandle, 3LU, 1LU);
        ObDereferenceObject(DirObject);
        CheckObject(DirectoryHandle, 2LU, 1LU);

        Status = ZwClose(DirectoryHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    // Now delete the last piece - object types
    // In fact, it's weird to get rid of object types, especially the way,
    // how it's done in the commented section below
    if (!AlternativeMethod)
    {
        for (i = 0; i < NUM_OBTYPES; ++i)
            if (ObTypes[i])
            {
                ObDereferenceObject(ObTypes[i]);
                ObTypes[i] = NULL;
            }
    }
    else
    {
        for (i = 0; i < NUM_OBTYPES; ++i)
        {
            if (ObTypes[i])
            {
                Status = RtlStringCbPrintfW(Name, sizeof Name, L"\\ObjectTypes\\MyObjectType%d", i);
                RtlInitUnicodeString(&ObPathName[0], Name);
                Status = ObReferenceObjectByName(&ObPathName[i], OBJ_CASE_INSENSITIVE, NULL, 0L, NULL, KernelMode, NULL, &TypeObject);

                ObDereferenceObject(TypeObject);
                ObDereferenceObject(TypeObject);
                DPRINT("Reference Name %wZ = %p, ObTypes[%d] = %p\n",
                    ObPathName[i], TypeObject, i, ObTypes[i]);
                ObTypes[i] = NULL;
            }
        }
    }
}

#if 0
static
VOID
ObtReferenceTests(VOID)
{
    INT i;
    NTSTATUS Status;
    UNICODE_STRING ObPathName[NUM_OBTYPES];

    // Reference them by handle
    for (i = 0; i < NUM_OBTYPES2; i++)
    {
        CheckObject(ObHandle1[i], 2LU, 1LU);
        Status = ObReferenceObjectByHandle(ObHandle1[i], 0L, ObTypes[i], KernelMode, &ObBody[i], NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckObject(ObHandle1[i], 3LU, 1LU);
        DPRINT("Ref by handle %lx = %p\n", ObHandle1[i], ObBody[i]);
    }

    // Reference them by pointer
    for (i = 0; i < NUM_OBTYPES2; i++)
    {
        CheckObject(ObHandle1[i], 3LU, 1LU);
        Status = ObReferenceObjectByPointer(ObBody[i], 0L, ObTypes[i], KernelMode);
        CheckObject(ObHandle1[i], 4LU, 1LU);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    // Reference them by name
    RtlInitUnicodeString(&ObPathName[0], L"\\ObtDirectory\\MyObject1");
    RtlInitUnicodeString(&ObPathName[1], L"\\ObtDirectory\\MyObject2");

#if 0
    for (i = 0; i < NUM_OBTYPES2; i++)
    {
        Status = ObReferenceObjectByName(&ObPathName[i], OBJ_CASE_INSENSITIVE, NULL, 0L, ObTypes[i], KernelMode, NULL, &ObBody[i]);
        DPRINT("Ref by name %wZ = %p\n", &ObPathName[i], ObBody[i]);
    }
#endif

    // Dereference now all of them

    for (i = 0; i < NUM_OBTYPES2; ++i)
        if (!skip(ObBody[i] != NULL, "Object pointer is NULL\n"))
        {
            CheckObject(ObHandle1[i], 4LU, 1LU);
            // For ObInsertObject, AdditionalReference
            //ObDereferenceObject(ObBody[i]);
            // For ByName
            //ObDereferenceObject(ObBody[i]);
            // For ByPointer
            ObDereferenceObject(ObBody[i]);
            CheckObject(ObHandle1[i], 3LU, 1LU);
            // For ByHandle
            ObDereferenceObject(ObBody[i]);
            CheckObject(ObHandle1[i], 2LU, 1LU);
        }
}
#endif /* 0 */

static
VOID
TestObjectType(
    IN BOOLEAN Clean)
{
    DumpCount = 0; OpenCount = 0; CloseCount = 0;
    DeleteCount = 0; ParseCount = 0;

    ObtCreateObjectTypes();
    DPRINT("ObtCreateObjectTypes() done\n");

    ObtCreateDirectory();
    DPRINT("ObtCreateDirectory() done\n");

    ObtCreateObjects();
    DPRINT("ObtCreateObjects() done\n");

    //ObtReferenceTests();

    ObtClose(Clean, FALSE);
}

START_TEST(ObType)
{
    TestObjectType(TRUE);
}

/* run this to see the objects created in user mode */
START_TEST(ObTypeNoClean)
{
    TestObjectType(FALSE);
    trace("Cleanup skipped as requested! Run ObTypeClean to clean up\n");
}

/* run this to clean up after ObTypeNoClean */
START_TEST(ObTypeClean)
{
    ObtClose(TRUE, FALSE);
    trace("Cleanup done\n");
}
