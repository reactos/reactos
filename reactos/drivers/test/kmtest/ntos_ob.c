/*
 * NTOSKRNL Ob Regressions KM-Test
 * ReactOS Kernel Mode Regression Testing framework
 *
 * Copyright 2006 Aleksey Bragin <aleksey@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include <ddk/ntddk.h>
#include "kmtest.h"

//#define NDEBUG
#include "debug.h"

#include "ndk/obtypes.h"
#include "ndk/obfuncs.h"

#include "ndk/ifssupp.h"
#include "ndk/setypes.h"
#include "ndk/sefuncs.h"

// I ment to make this test scalable, but for now
// we work with two object types only
#define NUM_OBTYPES 2

typedef struct _MY_OBJECT1
{
    ULONG Something1;
} MY_OBJECT1, *PMY_OBJECT1;

typedef struct _MY_OBJECT2
{
    ULONG Something1;
    ULONG SomeLong[10];
} MY_OBJECT2, *PMY_OBJECT2;

POBJECT_TYPE            ObTypes[NUM_OBTYPES];
UNICODE_STRING          ObTypeName[NUM_OBTYPES];
UNICODE_STRING          ObName[NUM_OBTYPES];
OBJECT_TYPE_INITIALIZER ObTypeInitializer[NUM_OBTYPES];
UNICODE_STRING          ObDirectoryName;
OBJECT_ATTRIBUTES       ObDirectoryAttributes;
OBJECT_ATTRIBUTES       ObAttributes[NUM_OBTYPES];
PVOID                   ObBody[NUM_OBTYPES];
PMY_OBJECT1             ObObject1;
PMY_OBJECT2             ObObject2;
HANDLE                  ObHandle1[NUM_OBTYPES];
HANDLE                  ObHandle2[NUM_OBTYPES];
HANDLE                  DirectoryHandle;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
DumpProc(IN PVOID Object,
         IN POB_DUMP_CONTROL DumpControl)
{
    DbgPrint("DumpProc() called\n");
}

// prototype doesn't match Win2003! (causes BSOD)
VOID
NTAPI
OpenProc(IN OB_OPEN_REASON OpenReason,
         IN PEPROCESS Process,
         IN PVOID Object,
         IN ACCESS_MASK GrantedAccess,
         IN ULONG HandleCount)
{
    DbgPrint("OpenProc() called\n");
    DbgBreakPoint();
}

// Tested in Win2k3
VOID
NTAPI
CloseProc(IN PEPROCESS Process,
          IN PVOID Object,
          IN ACCESS_MASK GrantedAccess,
          IN ULONG ProcessHandleCount,
          IN ULONG SystemHandleCount)
{
    DPRINT("CloseProc() called for Object=0x%p\n", Object);
}

// Tested in Win2k3
VOID
NTAPI
DeleteProc(IN PVOID Object)
{
    DPRINT("DeleteProc() called for Object=0x%p\n", Object);
}

NTSTATUS
NTAPI
ParseProc(IN PVOID ParseObject,
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
    DbgPrint("ParseProc() called\n");
    *Object = NULL;
    return STATUS_OBJECT_NAME_NOT_FOUND;//STATUS_SUCCESS;
}

VOID
ObtCreateObjectTypes()
{
    USHORT i;
    NTSTATUS Status;
    WCHAR   Name[15];

    for (i=0; i<NUM_OBTYPES; i++)
    {
        // Prepare object type name
        // TODO: Generate type names and don't use this unprofessional,
        swprintf(Name, L"MyObjectType%lx", i);
        RtlInitUnicodeString(&ObTypeName[i], Name);

        // Prepare initializer
        RtlZeroMemory(&ObTypeInitializer[i], sizeof(ObTypeInitializer[i]));
        ObTypeInitializer[i].Length = sizeof(ObTypeInitializer[i]);
        ObTypeInitializer[i].PoolType = NonPagedPool;
        ObTypeInitializer[i].MaintainHandleCount = TRUE;
        ObTypeInitializer[i].ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;

        // Test for invalid parameter
        // FIXME: Make it more exact, to see which params Win2k3 checks
        // existence of
        Status = ObCreateObjectType(&ObTypeName[i], &ObTypeInitializer[i],
            (PSECURITY_DESCRIPTOR)NULL, &ObTypes[i]);
        ok(Status == STATUS_INVALID_PARAMETER,
            "ObCreateObjectType returned 0x%lX", Status);

        // Object procedures
        ObTypeInitializer[i].CloseProcedure = (OB_CLOSE_METHOD)CloseProc;
        ObTypeInitializer[i].DeleteProcedure = (OB_DELETE_METHOD)DeleteProc;
        ObTypeInitializer[i].DumpProcedure = (OB_DUMP_METHOD)DumpProc;
        //ObTypeInitializer[i].OpenProcedure = (OB_OPEN_METHOD)OpenProc;
        ObTypeInitializer[i].ParseProcedure = (OB_PARSE_METHOD)ParseProc;

        Status = ObCreateObjectType(&ObTypeName[i], &ObTypeInitializer[i],
            (PSECURITY_DESCRIPTOR)NULL, &ObTypes[i]);
        ok(Status == STATUS_SUCCESS,
            "Failed to create object type with status=0x%lX", Status);
    }
}

VOID
ObtCreateDirectory()
{
    NTSTATUS Status;

    // Directory will have permanent and case insensitive flags
    RtlInitUnicodeString(&ObDirectoryName, L"\\ObtDirectory");
    InitializeObjectAttributes(&ObDirectoryAttributes, &ObDirectoryName,
        OBJ_PERMANENT | OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = ZwCreateDirectoryObject(&DirectoryHandle, 0, &ObDirectoryAttributes);
    ok(Status == STATUS_SUCCESS,
        "Failed to create directory object with status=0x%lX", Status);
}

VOID
ObtCreateObjects()
{
    PVOID ObBody1[2];
    NTSTATUS Status;

    // Create two objects
    RtlInitUnicodeString(&ObName[0], L"\\ObtDirectory\\MyObject1");
    InitializeObjectAttributes(&ObAttributes[0], &ObName[0],
        OBJ_CASE_INSENSITIVE, NULL, NULL);

    RtlInitUnicodeString(&ObName[1], L"\\ObtDirectory\\MyObject2");
    InitializeObjectAttributes(&ObAttributes[1], &ObName[1],
        OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = ObCreateObject(KernelMode, ObTypes[0], &ObAttributes[0],
        KernelMode, NULL, (ULONG)sizeof(MY_OBJECT1), 0L, 0L,
        (PVOID *)&ObBody[0]);
    ok(Status == STATUS_SUCCESS,
        "Failed to create object with status=0x%lX", Status);

    Status = ObCreateObject(KernelMode, ObTypes[1], &ObAttributes[1],
        KernelMode, NULL, (ULONG)sizeof(MY_OBJECT2), 0L, 0L,
        (PVOID *)&ObBody[1]);
    ok(Status == STATUS_SUCCESS,
        "Failed to create object with status=0x%lX", Status);

    // Insert them
    Status = ObInsertObject(ObBody[0], NULL, STANDARD_RIGHTS_ALL, 0,
        &ObBody[0], &ObHandle1[0]);
    ok(Status == STATUS_SUCCESS,
        "Failed to insert object 0 with status=0x%lX", Status);
    ok(ObBody[0] != NULL, "Object body = NULL");
    ok(ObHandle1[0] != NULL, "Handle = NULL");

    Status = ObInsertObject(ObBody[1], NULL, GENERIC_ALL, 0,
        &ObBody[1], &ObHandle1[1]); 
    ok(Status == STATUS_SUCCESS,
        "Failed to insert object 1 with status=0x%lX", Status);
    ok(ObBody[1] != NULL, "Object body = NULL");
    ok(ObHandle1[1] != NULL, "Handle = NULL");

    // Now create an object of type 0, of the same name and expect it to fail
    // inserting, but success creation
    RtlInitUnicodeString(&ObName[0], L"\\ObtDirectory\\MyObject1");
    InitializeObjectAttributes(&ObAttributes[0], &ObName[0], OBJ_OPENIF,
        NULL, NULL);

    Status = ObCreateObject(KernelMode, ObTypes[0], &ObAttributes[0], KernelMode,
        NULL, (ULONG)sizeof(MY_OBJECT1), 0L, 0L, (PVOID *)&ObBody1[0]);
    ok(Status == STATUS_SUCCESS,
        "Failed to create object with status=0x%lX", Status);

    Status = ObInsertObject(ObBody1[0], NULL, GENERIC_ALL, 0,
        &ObBody1[1], &ObHandle2[0]);
    ok(Status == STATUS_OBJECT_NAME_EXISTS,
        "Object insertion should have failed, but got 0x%lX", Status);
    ok(ObBody[0] == ObBody1[1],
        "Object bodies doesn't match, 0x%p != 0x%p", ObBody[0], ObBody1[1]);
    ok(ObHandle2[0] != NULL, "Bad handle returned 0x%lX", (ULONG)ObHandle2[0]);

    // Close its handle
    Status = ZwClose(ObHandle2[0]);
    ok(Status == STATUS_SUCCESS,
        "Failed to close handle status=0x%lX", Status);

    // Object referenced 2 times:
    // 1) ObInsertObject
    // 2) AdditionalReferences
    ObDereferenceObject(ObBody1[1]);
}

VOID
ObtClose()
{
    PVOID DirObject;
    NTSTATUS Status;
    //PVOID TypeObject;
    USHORT i;
    //UNICODE_STRING ObPathName[NUM_OBTYPES];

    // Close what we have opened and free what we allocated
    ZwClose(ObHandle1[0]);
    ZwClose(ObHandle1[1]);
    ZwClose(ObHandle2[0]);
    ZwClose(ObHandle2[1]);

    // Now we have to get rid of a directory object
    // Since it is permanent, we have to firstly make it temporary
    // and only then kill
    // (this procedure is described in DDK)
    Status = ObReferenceObjectByHandle(DirectoryHandle, 0L, NULL,
        KernelMode, &DirObject, NULL);
    ok(Status == STATUS_SUCCESS,
        "Failed to reference object by handle with status=0x%lX", Status);

    // Dereference 2 times - first for just previous referencing
    // and 2nd time for creation of permanent object itself
    ObDereferenceObject(DirObject);
    ObDereferenceObject(DirObject);

    Status = ZwMakeTemporaryObject(DirectoryHandle);
    ok(Status == STATUS_SUCCESS,
        "Failed to make temp object with status=0x%lX", Status);

    // Close the handle now and we are done
    Status = ZwClose(DirectoryHandle);
    ok(Status == STATUS_SUCCESS,
        "Failed to close handle with status=0x%lX", Status);

    // Now delete the last piece - object types
    // In fact, it's weird to get rid of object types, especially the way,
    // how it's done in the commented section below
    for (i=0; i<NUM_OBTYPES; i++)
        ObDereferenceObject(ObTypes[i]);
/*
    RtlInitUnicodeString(&ObPathName[0], L"\\ObjectTypes\\MyObjectType1");
    RtlInitUnicodeString(&ObPathName[1], L"\\ObjectTypes\\MyObjectType2");

    for (i=0; i<NUM_OBTYPES; i++)
    {
        Status = ObReferenceObjectByName(&ObPathName[i],
            OBJ_CASE_INSENSITIVE, NULL, 0L, NULL, KernelMode, NULL,
            &TypeObject);

        ObDereferenceObject(TypeObject);
        ObDereferenceObject(TypeObject);
        DPRINT("Reference Name %S = %p, ObTypes[%d] = %p\n",
            ObPathName[i], TypeObject, i, ObTypes[i]);
    }
*/
}

VOID
ObtReferenceTests()
{
    USHORT i;
    NTSTATUS Status;
    UNICODE_STRING ObPathName[NUM_OBTYPES];

    // Reference them by handle
    for (i=0; i<NUM_OBTYPES; i++)
    {
        Status = ObReferenceObjectByHandle(ObHandle1[i], 0L, ObTypes[i],
            KernelMode, &ObBody[i], NULL);
        ok(Status == STATUS_SUCCESS,
            "Failed to reference object by handle, status=0x%lX", Status);
        DPRINT("Ref by handle %lx = %p\n", ObHandle1[i], ObBody[i]);
    }

    // Reference them by pointer
    for (i=0; i<NUM_OBTYPES; i++)
    {
        Status = ObReferenceObjectByPointer(ObBody[i], 0L, ObTypes[i], KernelMode);
        ok(Status == STATUS_SUCCESS,
            "Failed to reference object by pointer, status=0x%lX", Status);
    }

    // Reference them by name
    RtlInitUnicodeString(&ObPathName[0], L"\\ObtDirectory\\MyObject1");
    RtlInitUnicodeString(&ObPathName[1], L"\\ObtDirectory\\MyObject2");

    for (i=0; i<NUM_OBTYPES; i++)
    {
        Status = ObReferenceObjectByName(&ObPathName[i],
            OBJ_CASE_INSENSITIVE, NULL, 0L, ObTypes[i], KernelMode, NULL,
            &ObBody[0]);

        DPRINT("Ref by name %wZ = %p\n", &ObPathName[i], ObBody[i]);
    }

    // Dereference now all of them

    // For ObInsertObject, AdditionalReference
    ObDereferenceObject(ObBody[0]);
    ObDereferenceObject(ObBody[1]);

    // For ByHandle
    ObDereferenceObject(ObBody[0]);
    ObDereferenceObject(ObBody[1]);

    // For ByPointer
    ObDereferenceObject(ObBody[0]);
    ObDereferenceObject(ObBody[1]);

    // For ByName
    ObDereferenceObject(ObBody[0]);
    ObDereferenceObject(ObBody[1]);
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
FASTCALL
NtoskrnlObTest()
{
    StartTest();

    // Create object-types to use in tests
    ObtCreateObjectTypes();
    DPRINT("ObtCreateObjectTypes() done\n");

    // Create Directory
    ObtCreateDirectory();
    DPRINT("ObtCreateDirectory() done\n");

    // Create and insert objects
    ObtCreateObjects();
    DPRINT("ObtCreateObjects() done\n");

    // Reference them in a variety of ways
    //ObtReferenceTests();

    // Clean up
    // FIXME: Disable to see results of creating objects in usermode.
    //        Also it has problems with object types removal
    ObtClose();
    DPRINT("Cleanup done\n");

    FinishTest("NTOSKRNL Ob Manager");
}
