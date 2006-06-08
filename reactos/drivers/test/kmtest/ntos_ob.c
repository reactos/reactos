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

/* PRIVATE FUNCTIONS **********************************************************/

VOID
DumpProc(IN PVOID Object,
         IN POB_DUMP_CONTROL DumpControl)
{
    DbgPrint("DumpProc() called\n");
}

VOID
OpenProc(IN OB_OPEN_REASON OpenReason,
          IN PEPROCESS Process,
          IN PVOID Object,
          IN ACCESS_MASK GrantedAccess,
          IN ULONG HandleCount)
{
    DbgPrint("OpenProc() called\n");
}

VOID
CloseProc(IN PEPROCESS Process,
           IN PVOID Object,
           IN ACCESS_MASK GrantedAccess,
           IN ULONG ProcessHandleCount,
           IN ULONG SystemHandleCount)
{
    DbgPrint("CloseProc() called\n");
}

VOID
DeleteProc(IN PVOID Object)
{
    DbgPrint("DeleteProc()called\n");
}

NTSTATUS
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
    return STATUS_SUCCESS;
}

VOID
ObtCreateObjectTypes()
{
    UCHAR i;
    NTSTATUS Status;

    for (i=0; i<NUM_OBTYPES; i++)
    {
        // Prepare object type name
        // TODO: Generate type names and don't use this unprofessional,
        // ugly looking, and otherwise bad if-condition
        if (i == 0)
            RtlInitUnicodeString(&ObTypeName[i], L"MyObjectType1");
        else
            RtlInitUnicodeString(&ObTypeName[i], L"MyObjectType2");

        // Prepare initializer
        RtlZeroMemory(&ObTypeInitializer[i], sizeof(ObTypeInitializer[i]));
        ObTypeInitializer[i].Length = sizeof(ObTypeInitializer[i]);
        ObTypeInitializer[i].PoolType = NonPagedPool;
        ObTypeInitializer[i].MaintainHandleCount = TRUE;
        ObTypeInitializer[i].ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;

        // Object procedures
        ObTypeInitializer[i].CloseProcedure = (OB_CLOSE_METHOD)CloseProc;
        ObTypeInitializer[i].DeleteProcedure = (OB_DELETE_METHOD)DeleteProc;
        ObTypeInitializer[i].DumpProcedure = (OB_DUMP_METHOD)DumpProc;
        ObTypeInitializer[i].OpenProcedure = (OB_OPEN_METHOD)OpenProc;
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
    HANDLE DirectoryHandle;

    // Directory will have permanent and case insensitive flags
    RtlInitUnicodeString(&ObDirectoryName, L"\\ObtDirectory");
    InitializeObjectAttributes(&ObDirectoryAttributes, &ObDirectoryName,
        OBJ_PERMANENT | OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = ZwCreateDirectoryObject(&DirectoryHandle, 0, &ObDirectoryAttributes);
    ok(Status == STATUS_SUCCESS,
        "Failed to create directory object with status=0x%lX", Status);

    Status = ZwClose(DirectoryHandle);
    ok(Status == STATUS_SUCCESS,
        "Failed to close handle with status=0x%lX", Status);
}

VOID
ObtCreateObjects()
{
    NTSTATUS Status;

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
    DPRINT("Created Object 0\n");

    Status = ObCreateObject(KernelMode, ObTypes[1], &ObAttributes[1],
        KernelMode, NULL, (ULONG)sizeof(MY_OBJECT2), 0L, 0L,
        (PVOID *)&ObBody[1]);
    ok(Status == STATUS_SUCCESS,
        "Failed to create object with status=0x%lX", Status);
    DPRINT("Created Object 1\n");
}

VOID
ObtClose()
{
    // TODO: Close what we have opened and free what we allocated
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

    // Clean up
    ObtClose();
    DPRINT("Cleanup done\n");

    FinishTest("NTOSKRNL Ob Manager");
}
