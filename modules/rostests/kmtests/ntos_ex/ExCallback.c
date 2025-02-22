/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Executive Callback test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

static
PEX_CALLBACK_ROUTINE_BLOCK
(NTAPI
*ExAllocateCallBack)(
    IN PEX_CALLBACK_FUNCTION Function,
    IN PVOID Context
)
//= (PVOID)0x809af1f4 // 2003 sp1 x86
//= (PVOID)0x80a7f04a // 2003 sp1 x86 checked
;

static
VOID
(NTAPI
*ExFreeCallBack)(
    IN PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock
)
//= (PVOID)0x80918bb5 // 2003 sp1 x86
//= (PVOID)0x80a355f0 // 2003 sp1 x86 checked
;

static INT CallbackArgument1;
static INT CallbackArgument2;

static
NTSTATUS
NTAPI
ExCallbackFunction(
    IN PVOID CallbackContext,
    IN PVOID Argument1 OPTIONAL,
    IN PVOID Argument2 OPTIONAL)
{
    ok(0, "Callback function unexpectedly called\n");
    return STATUS_SUCCESS;
}

static
VOID
TestPrivateFunctions(VOID)
{
    UNICODE_STRING ExAllocateCallBackName = RTL_CONSTANT_STRING(L"ExAllocateCallBack");
    UNICODE_STRING ExFreeCallBackName = RTL_CONSTANT_STRING(L"ExFreeCallBack");
    PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock;
    INT CallbackContext;

    if (!ExAllocateCallBack)
        ExAllocateCallBack = MmGetSystemRoutineAddress(&ExAllocateCallBackName);
    if (!ExFreeCallBack)
        ExFreeCallBack = MmGetSystemRoutineAddress(&ExFreeCallBackName);

    if (skip(ExAllocateCallBack && ExFreeCallBack,
             "ExAllocateCallBack and/or ExFreeCallBack unavailable\n"))
        return;

    CallbackBlock = ExAllocateCallBack(ExCallbackFunction, &CallbackContext);
    ok(CallbackBlock != NULL, "CallbackBlock = NULL\n");

    if (skip(CallbackBlock != NULL, "Allocating callback failed\n"))
        return;

    ok_eq_pointer(CallbackBlock->Function, ExCallbackFunction);
    ok_eq_pointer(CallbackBlock->Context, &CallbackContext);
    ok_eq_hex(KmtGetPoolTag(CallbackBlock), 'brbC');

    ExFreeCallBack(CallbackBlock);
}

static
VOID
NTAPI
CallbackFunction(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2)
{
    INT *InvocationCount = CallbackContext;

    ok_irql(PASSIVE_LEVEL);

    (*InvocationCount)++;
    ok_eq_pointer(Argument1, &CallbackArgument1);
    ok_eq_pointer(Argument2, &CallbackArgument2);
}

START_TEST(ExCallback)
{
    NTSTATUS Status;
    PCALLBACK_OBJECT CallbackObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING CallbackName = RTL_CONSTANT_STRING(L"\\Callback\\KmtestExCallbackTestCallback");
    PVOID CallbackRegistration;
    INT InvocationCount = 0;

    TestPrivateFunctions();

    /* TODO: Parameter tests */
    /* TODO: Test the three predefined callbacks */
    /* TODO: Test opening an existing callback */
    /* TODO: Test AllowMultipleCallbacks */
    /* TODO: Test calling multiple callbacks */
    /* TODO: Test registering the same function twice */
    /* TODO: Test callback object fields */
    /* TODO: Test callback registration fields */
    InitializeObjectAttributes(&ObjectAttributes,
                               &CallbackName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    CallbackObject = KmtInvalidPointer;
    Status = ExCreateCallback(&CallbackObject,
                              &ObjectAttributes,
                              TRUE,
                              TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(CallbackObject != NULL && CallbackObject != KmtInvalidPointer,
        "CallbackObject = %p", CallbackObject);

    if (skip(NT_SUCCESS(Status), "Creating callback failed\n"))
        return;

    CallbackRegistration = ExRegisterCallback(CallbackObject,
                                              CallbackFunction,
                                              &InvocationCount);
    ok(CallbackRegistration != NULL, "CallbackRegistration = NULL\n");

    if (!skip(CallbackRegistration != NULL, "Registering callback failed\n"))
    {
        ok_eq_hex(KmtGetPoolTag(CallbackRegistration), 'eRBC');
        ok_eq_int(InvocationCount, 0);
        ExNotifyCallback(CallbackObject,
                         &CallbackArgument1,
                         &CallbackArgument2);
        ok_eq_int(InvocationCount, 1);
        ExNotifyCallback(CallbackObject,
                         &CallbackArgument1,
                         &CallbackArgument2);
        ok_eq_int(InvocationCount, 2);

        ExUnregisterCallback(CallbackRegistration);
    }

    ObDereferenceObject(CallbackObject);
}
