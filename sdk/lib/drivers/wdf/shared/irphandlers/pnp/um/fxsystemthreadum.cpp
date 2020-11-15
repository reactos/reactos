/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSystemThreadUm.cpp

Abstract:

    This is the implementation of the FxSystemThread object.


Author:





Environment:

    User mode only

Revision History:


--*/

#include <fxmin.hpp>

#pragma warning(push)
#pragma warning(disable:4100) //unreferenced parameter

FxSystemThread::FxSystemThread(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_SYSTEMTHREAD, 0, FxDriverGlobals)
{
    UfxVerifierTrapNotImpl();
}

FxSystemThread::~FxSystemThread()
{
    UfxVerifierTrapNotImpl();
}

NTSTATUS
FxSystemThread::_CreateAndInit(
    __out FxSystemThread** SystemThread,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in WDFDEVICE Device,
    __in MdDeviceObject DeviceObject
    )
{
    UfxVerifierTrapNotImpl();

    return STATUS_NOT_IMPLEMENTED;
}

//
// Create the system thread in order to be able to service work items
//
// It is recommended this is done from the system process context
// since the threads handle is available to the user mode process
// for a temporary window. XP and later supports OBJ_KERNELHANDLE, but
// DriverFrameworks must support W2K with the same binary.
//
// It is safe to call this at DriverEntry which is in the system process
// to create an initial driver thread, and this driver thread should be
// used for creating any child driver threads on demand.
//
BOOLEAN
FxSystemThread::Initialize()
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

NTSTATUS
FxSystemThread::CreateThread(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}












//
// This is called to tell the thread to exit.
//
// It must be called from thread context such as
// the driver unload routine since it will wait for the
// thread to exit.
//
BOOLEAN
FxSystemThread::ExitThread()
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

BOOLEAN
FxSystemThread::ExitThreadAsync(
    __inout FxSystemThread* Reaper
    )
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

BOOLEAN
FxSystemThread::QueueWorkItem(
    __inout PWORK_QUEUE_ITEM WorkItem
    )
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

//
// Attempt to cancel the work item.
//
// Returns TRUE if success.
//
// If returns FALSE, the work item
// routine either has been called, is running,
// or is about to be called.
//
BOOLEAN
FxSystemThread::CancelWorkItem(
    __inout PWORK_QUEUE_ITEM WorkItem
    )
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

VOID
FxSystemThread::Thread()
{
    UfxVerifierTrapNotImpl();
}













VOID
FxSystemThread::Reaper()
{
    UfxVerifierTrapNotImpl();
}

VOID
FxSystemThread::StaticThreadThunk(
    __inout PVOID Context
    )
{
    UfxVerifierTrapNotImpl();
}











VOID
FxSystemThread::StaticReaperThunk(
    __inout PVOID Context
    )
{
    UfxVerifierTrapNotImpl();
}

#pragma warning(pop)
