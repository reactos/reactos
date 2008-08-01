/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/tests/tests/VirtualMemory.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <ntoskrnl.h>
#include "regtests.h"

#define TestProcessHandle (HANDLE) 1
#define TestProcessObject (PVOID) 0x2
#define TestBaseAddress (PVOID) 0x1000
#define TestNumberOfBytesToLock 0x2000
#define TestMdl (PMDL) 0xD0000000

static BOOLEAN MockExFreePoolCalled = FALSE;

static VOID STDCALL
MockExFreePool(PVOID Block)
{
  _AssertFalse(MockExFreePoolCalled);
  _AssertEqualValue(TestMdl, Block);
  MockExFreePoolCalled = TRUE;
}

static BOOLEAN MockMmCreateMdlCalled = FALSE;

static PMDL STDCALL
MockMmCreateMdl(PMDL Mdl,
  PVOID Base,
  ULONG Length)
{
  _AssertFalse(MockMmCreateMdlCalled);
  _AssertEqualValue(TestBaseAddress, Base);
  _AssertEqualValue(TestNumberOfBytesToLock, Length);
  MockMmCreateMdlCalled = TRUE;
  return TestMdl;
}

static BOOLEAN MockMmProbeAndLockPagesCalled = FALSE;

static VOID STDCALL
MockMmProbeAndLockPages(PMDL Mdl,
  KPROCESSOR_MODE AccessMode,
  LOCK_OPERATION Operation)
{
  _AssertFalse(MockMmProbeAndLockPagesCalled);
  _AssertEqualValue(TestMdl, Mdl);
  _AssertEqualValue(UserMode, AccessMode);
  _AssertEqualValue(IoWriteAccess, Operation);
  MockMmProbeAndLockPagesCalled = TRUE;
}

static BOOLEAN MockObDereferenceObjectCalled = FALSE;

static VOID FASTCALL
MockObDereferenceObject(PVOID Object)
{
  _AssertFalse(MockObDereferenceObjectCalled);
  _AssertEqualValue(TestProcessObject, Object);
  MockObDereferenceObjectCalled = TRUE;
}

static BOOLEAN MockObReferenceObjectByHandleCalled = FALSE;

static NTSTATUS STDCALL
MockObReferenceObjectByHandle(HANDLE Handle,
  ACCESS_MASK DesiredAccess,
  POBJECT_TYPE ObjectType,
  KPROCESSOR_MODE AccessMode,
  PVOID* Object,
  POBJECT_HANDLE_INFORMATION HandleInformation)
{
  _AssertFalse(MockObReferenceObjectByHandleCalled);
  _AssertEqualValue(TestProcessHandle, Handle);
  _AssertEqualValue(PROCESS_VM_WRITE, DesiredAccess);
  _AssertEqualValue(NULL, ObjectType);
  _AssertEqualValue(UserMode, AccessMode);
  _AssertNotEqualValue(NULL, Object);
  _AssertEqualValue(NULL, HandleInformation);
  *Object = TestProcessObject;
  MockObReferenceObjectByHandleCalled = TRUE;
  return STATUS_SUCCESS;
}

static void RunTest()
{
  ULONG NumberOfBytesLocked;
  NTSTATUS status;

  status = MiLockVirtualMemory(TestProcessHandle,
    TestBaseAddress,
    TestNumberOfBytesToLock,
    &NumberOfBytesLocked,
    MockObReferenceObjectByHandle,
    MockMmCreateMdl,
    MockObDereferenceObject,
    MockMmProbeAndLockPages,
    MockExFreePool);
  _AssertEqualValue(STATUS_SUCCESS, status);
  _AssertTrue(MockObReferenceObjectByHandleCalled);
  _AssertTrue(MockMmCreateMdlCalled);
  _AssertTrue(MockMmProbeAndLockPagesCalled);
  _AssertTrue(MockExFreePoolCalled);
  _AssertTrue(MockObDereferenceObjectCalled);
  _AssertEqualValue(TestNumberOfBytesToLock, NumberOfBytesLocked);
}

_Dispatcher(VirtualmemoryTest, "Virtual memory")
