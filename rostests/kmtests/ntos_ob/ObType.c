/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Ob Regressions KM-Test
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 *                  Thomas Faber <thomas.faber@reactos.org>
 */

/* TODO: split this into multiple tests! ObLife, ObHandle, ObName, ... */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define CheckObject(Handle, Pointers, Handles) do                   \
{                                                                   \
    PUBLIC_OBJECT_BASIC_INFORMATION ObjectInfo;                     \
    Status = ZwQueryObject(Handle, ObjectBasicInformation,          \
                            &ObjectInfo, sizeof ObjectInfo, NULL);  \
    ok_eq_hex(Status, STATUS_SUCCESS);                              \
    ok_eq_ulong(ObjectInfo.PointerCount, Pointers);                 \
    ok_eq_ulong(ObjectInfo.HandleCount, Handles);                   \
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
static HANDLE                  DirectoryHandle;

typedef struct _COUNTS
{
    USHORT Dump;
    USHORT Open;
    USHORT Close;
    USHORT Delete;
    USHORT Parse;
    USHORT OkayToClose;
    USHORT QueryName;
} COUNTS, *PCOUNTS;
static COUNTS Counts;

static
VOID
NTAPI
DumpProc(
    IN PVOID Object,
    IN POB_DUMP_CONTROL DumpControl)
{
    DPRINT("DumpProc() called\n");
    ++Counts.Dump;
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
    ++Counts.Open;
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
    ++Counts.Close;
}

static
VOID
NTAPI
DeleteProc(
    IN PVOID Object)
{
    DPRINT("DeleteProc() 0x%p\n", Object);
    ++Counts.Delete;
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

    ++Counts.Parse;
    return STATUS_OBJECT_NAME_NOT_FOUND;//STATUS_SUCCESS;
}

static
BOOLEAN
NTAPI
OkayToCloseProc(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN HANDLE Handle,
    IN KPROCESSOR_MODE AccessMode)
{
    DPRINT("OkayToCloseProc() 0x%p, Handle 0x%p, AccessMask 0x%lX\n",
        Object, Handle, AccessMode);
    ++Counts.OkayToClose;
    return TRUE;
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
    ++Counts.QueryName;

    ObjectNameInfo = NULL;
    ReturnLength = 0;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

static
VOID
ObtCreateObjectTypes(VOID)
{
    INT i;
    NTSTATUS Status;
    struct
    {
        WCHAR DirectoryName[sizeof "\\ObjectTypes\\" - 1];
        WCHAR TypeName[15];
    } Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ObjectTypeHandle;
    UNICODE_STRING ObjectPath;

    RtlCopyMemory(&Name.DirectoryName, L"\\ObjectTypes\\", sizeof Name.DirectoryName);

    for (i = 0; i < NUM_OBTYPES; ++i)
    {
        Status = RtlStringCbPrintfW(Name.TypeName, sizeof Name.TypeName, L"MyObjectType%x", i);
        ASSERT(NT_SUCCESS(Status));
        RtlInitUnicodeString(&ObTypeName[i], Name.TypeName);
        DPRINT("Creating object type %wZ\n", &ObTypeName[i]);

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
        ObTypeInitializer[i].OkayToCloseProcedure = OkayToCloseProc;
        ObTypeInitializer[i].QueryNameProcedure = QueryNameProc;
        //ObTypeInitializer[i].SecurityProcedure = SecurityProc;

        Status = ObCreateObjectType(&ObTypeName[i], &ObTypeInitializer[i], NULL, &ObTypes[i]);
        if (Status == STATUS_OBJECT_NAME_COLLISION)
        {
            /* as we cannot delete the object types, get a pointer if they
             * already exist */
            RtlInitUnicodeString(&ObjectPath, Name.DirectoryName);
            InitializeObjectAttributes(&ObjectAttributes, &ObjectPath, OBJ_KERNEL_HANDLE, NULL, NULL);
            Status = ObOpenObjectByName(&ObjectAttributes, NULL, KernelMode, NULL, 0, NULL, &ObjectTypeHandle);
            ok_eq_hex(Status, STATUS_SUCCESS);
            ok(ObjectTypeHandle != NULL, "ObjectTypeHandle = NULL\n");
            if (!skip(Status == STATUS_SUCCESS && ObjectTypeHandle, "No handle\n"))
            {
                Status = ObReferenceObjectByHandle(ObjectTypeHandle, 0, NULL, KernelMode, (PVOID)&ObTypes[i], NULL);
                ok_eq_hex(Status, STATUS_SUCCESS);
                if (!skip(Status == STATUS_SUCCESS && ObTypes[i], "blah\n"))
                {
                    ObTypes[i]->TypeInfo.CloseProcedure = CloseProc;
                    ObTypes[i]->TypeInfo.DeleteProcedure = DeleteProc;
                    ObTypes[i]->TypeInfo.DumpProcedure = DumpProc;
                    ObTypes[i]->TypeInfo.OpenProcedure = OpenProc;
                    ObTypes[i]->TypeInfo.ParseProcedure = ParseProc;
                    ObTypes[i]->TypeInfo.OkayToCloseProcedure = OkayToCloseProc;
                    ObTypes[i]->TypeInfo.QueryNameProcedure = QueryNameProc;
                }
                Status = ZwClose(ObjectTypeHandle);
            }
        }

        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(ObTypes[i] != NULL, "ObType = NULL\n");
    }
}

static
VOID
ObtCreateDirectory(VOID)
{
    NTSTATUS Status;

    RtlInitUnicodeString(&ObDirectoryName, L"\\ObtDirectory");
    InitializeObjectAttributes(&ObDirectoryAttributes, &ObDirectoryName, OBJ_KERNEL_HANDLE | OBJ_PERMANENT | OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = ZwCreateDirectoryObject(&DirectoryHandle, DELETE, &ObDirectoryAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckObject(DirectoryHandle, 3LU, 1LU);
}

#define CheckCounts(OpenCount, CloseCount, DeleteCount, ParseCount, \
                        OkayToCloseCount, QueryNameCount) do        \
{                                                                   \
    ok_eq_uint(Counts.Open, OpenCount);                             \
    ok_eq_uint(Counts.Close, CloseCount);                           \
    ok_eq_uint(Counts.Delete, DeleteCount);                         \
    ok_eq_uint(Counts.Parse, ParseCount);                           \
    ok_eq_uint(Counts.OkayToClose, OkayToCloseCount);               \
    ok_eq_uint(Counts.QueryName, QueryNameCount);                   \
} while (0)

#define SaveCounts(Save) memcpy(&Save, &Counts, sizeof Counts)

/* TODO: make this the same as NUM_OBTYPES */
#define NUM_OBTYPES2 2
static
VOID
ObtCreateObjects(VOID)
{
    NTSTATUS Status;
    WCHAR Name[NUM_OBTYPES2][MAX_PATH];
    COUNTS SaveCounts;
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
    CheckObject(DirectoryHandle, 3LU, 1LU);

    for (i = 0; i < NUM_OBTYPES2; ++i)
    {
        Status = ObCreateObject(KernelMode, ObTypes[i], &ObAttributes[i], KernelMode, NULL, ObjectSize[i], 0L, 0L, &ObBody[i]);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    SaveCounts(SaveCounts);

    // Insert them
    for (i = 0; i < NUM_OBTYPES2; ++i)
    {
        CheckObject(DirectoryHandle, 3LU + i, 1LU);
        Status = ObInsertObject(ObBody[i], NULL, Access[i], 0, &ObBody[i], &ObHandle1[i]);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(ObBody[i] != NULL, "Object body = NULL\n");
        ok(ObHandle1[i] != NULL, "Handle = NULL\n");
        CheckObject(ObHandle1[i], 3LU, 1LU);
        CheckCounts(SaveCounts.Open + 1, SaveCounts.Close, SaveCounts.Delete, SaveCounts.Parse, SaveCounts.OkayToClose, SaveCounts.QueryName);
        SaveCounts(SaveCounts);
        CheckObject(DirectoryHandle, 4LU + i, 1LU);
    }

    //DPRINT1("%d %d %d %d %d %d %d\n", DumpCount, OpenCount, CloseCount, DeleteCount, ParseCount, OkayToCloseCount, QueryNameCount);
    CheckCounts(SaveCounts.Open, SaveCounts.Close, SaveCounts.Delete, SaveCounts.Parse, SaveCounts.OkayToClose, SaveCounts.QueryName);
}

static
VOID
ObtClose(
    BOOLEAN Clean,
    BOOLEAN AlternativeMethod)
{
    NTSTATUS Status;
    LONG_PTR Ret;
    PVOID TypeObject;
    INT i;
    UNICODE_STRING ObPathName[NUM_OBTYPES];
    WCHAR Name[MAX_PATH];

    // Close what we have opened and free what we allocated
    for (i = 0; i < NUM_OBTYPES2; ++i)
    {
        if (!skip(ObBody[i] != NULL, "Nothing to dereference\n"))
        {
            if (ObHandle1[i]) CheckObject(ObHandle1[i], 3LU, 1LU);
            Ret = ObReferenceObject(ObBody[i]);
            if (ObHandle1[i]) CheckObject(ObHandle1[i], 4LU, 1LU);
            Ret = ObDereferenceObject(ObBody[i]);
            ok_eq_longptr(Ret, (LONG_PTR)2);
            if (ObHandle1[i]) CheckObject(ObHandle1[i], 3LU, 1LU);
            ObBody[i] = NULL;
        }
        if (!skip(ObHandle1[i] != NULL, "Nothing to close\n"))
        {
            Status = ZwClose(ObHandle1[i]);
            ok_eq_hex(Status, STATUS_SUCCESS);
            ObHandle1[i] = NULL;
        }
    }

    if (skip(Clean, "Not cleaning up, as requested. Use ObTypeClean to clean up\n"))
        return;

    // Now we have to get rid of a directory object
    // Since it is permanent, we have to firstly make it temporary
    // and only then kill
    // (this procedure is described in DDK)
    if (!skip(DirectoryHandle != NULL, "No directory handle\n"))
    {
        CheckObject(DirectoryHandle, 3LU, 1LU);

        Status = ZwMakeTemporaryObject(DirectoryHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
        CheckObject(DirectoryHandle, 3LU, 1LU);

        Status = ZwClose(DirectoryHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    /* we don't delete the object types we created. It makes Windows unstable.
     * TODO: perhaps make it work in ROS anyway */
    return;
    if (!AlternativeMethod)
    {
        for (i = 0; i < NUM_OBTYPES; ++i)
            if (!skip(ObTypes[i] != NULL, "No object type to delete\n"))
            {
                Ret = ObDereferenceObject(ObTypes[i]);
                ok_eq_longptr(Ret, (LONG_PTR)0);
                ObTypes[i] = NULL;
            }
    }
    else
    {
        for (i = 0; i < NUM_OBTYPES; ++i)
        {
            if (!skip(ObTypes[i] != NULL, "No object type to delete\n"))
            {
                Status = RtlStringCbPrintfW(Name, sizeof Name, L"\\ObjectTypes\\MyObjectType%d", i);
                RtlInitUnicodeString(&ObPathName[i], Name);
                Status = ObReferenceObjectByName(&ObPathName[i], OBJ_CASE_INSENSITIVE, NULL, 0L, NULL, KernelMode, NULL, &TypeObject);

                Ret = ObDereferenceObject(TypeObject);
                ok_eq_longptr(Ret, (LONG_PTR)2);
                Ret = ObDereferenceObject(TypeObject);
                ok_eq_longptr(Ret, (LONG_PTR)1);
                DPRINT("Reference Name %wZ = %p, ObTypes[%d] = %p\n",
                    ObPathName[i], TypeObject, i, ObTypes[i]);
                ObTypes[i] = NULL;
            }
        }
    }
}

static
VOID
TestObjectType(
    IN BOOLEAN Clean)
{
    RtlZeroMemory(&Counts, sizeof Counts);

    ObtCreateObjectTypes();
    DPRINT("ObtCreateObjectTypes() done\n");

    ObtCreateDirectory();
    DPRINT("ObtCreateDirectory() done\n");

    if (!skip(ObTypes[0] != NULL, "No object types!\n"))
        ObtCreateObjects();
    DPRINT("ObtCreateObjects() done\n");

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
}

/* run this to clean up after ObTypeNoClean */
START_TEST(ObTypeClean)
{
    ObtClose(TRUE, FALSE);
}
