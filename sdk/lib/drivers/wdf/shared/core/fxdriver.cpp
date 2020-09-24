/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDriver.cpp

Abstract:

    This is the main driver framework.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "coreprivshared.hpp"
#include "fxiotarget.hpp"

// Tracing support
extern "C" {
#include "FxDriver.tmh"
}

FxDriver::FxDriver(
    __in MdDriverObject     ArgDriverObject,
    __in PWDF_DRIVER_CONFIG DriverConfig,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_DRIVER, sizeof(FxDriver), FxDriverGlobals),
    m_DriverObject(ArgDriverObject),
    m_CallbackMutexLock(FxDriverGlobals)
{
    RtlInitUnicodeString(&m_RegistryPath, NULL);






#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    m_ExecutionLevel = WdfExecutionLevelDispatch;
#else
    m_ExecutionLevel = WdfExecutionLevelPassive;
#endif

    m_SynchronizationScope = WdfSynchronizationScopeNone;

    m_CallbackLockPtr = NULL;
    m_CallbackLockObjectPtr = NULL;

    m_DisposeList = NULL;

    //
    // These are initialized up front so that sub objects
    // can have the right configuration
    //
    WDF_DRIVER_CONFIG_INIT(&m_Config, NULL);

    // Only copy the smallest of what is specified and our size
    RtlCopyMemory(&m_Config, DriverConfig, min(sizeof(m_Config), DriverConfig->Size) );

    m_DebuggerConnected = FALSE;

#if FX_IS_USER_MODE
    m_DriverParametersKey = NULL;
#endif
}

FxDriver::~FxDriver()
{
    // Make it always present right now even on free builds
    if (IsDisposed() == FALSE) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_FATAL, TRACINGDRIVER,
            "FxDriver 0x%p not disposed: this maybe a driver reference count "
            "problem with WDFDRIVER 0x%p", this, GetObjectHandleUnchecked());

        FxVerifierBugCheck(GetDriverGlobals(),
                           WDF_OBJECT_ERROR,
                           (ULONG_PTR) GetObjectHandleUnchecked(),
                           (ULONG_PTR) this);
    }

    //
    // Free the memory for the registry path if required.
    //
    if (m_RegistryPath.Buffer) {
        FxPoolFree(m_RegistryPath.Buffer);
    }

    if (m_DisposeList != NULL) {
        m_DisposeList->DeleteObject();
    }

#if FX_IS_USER_MODE
    //
    // Close the R/W handle to the driver's service parameters key
    // that we opened during Initialize.
    //
    if (m_DriverParametersKey != NULL) {
        NTSTATUS status = FxRegKey::_Close(m_DriverParametersKey);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDRIVER,
                                "Cannot close Driver Parameters key %!STATUS!",
                                status);
        }
        m_DriverParametersKey = NULL;
    }

    //
    // The host-created driver object holds a reference to this
    // FxDriver object. Clear it, since this object was deleted.
    //
    ClearDriverObjectFxDriver();
#endif
}

BOOLEAN
FxDriver::Dispose(
    VOID
    )
{
    if (m_DisposeList != NULL) {
        m_DisposeList->WaitForEmpty();
    }

    return __super::Dispose();
}

VOID
FxDriver::Unload(
    __in MdDriverObject DriverObject
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDriver *pDriver;

    pDriver = FxDriver::GetFxDriver(DriverObject);
    if (pDriver == NULL) {
        return;
    }

    pFxDriverGlobals = pDriver->GetDriverGlobals();

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
                        "Unloading WDFDRIVER %p, PDRIVER_OBJECT_UM %p",
                        pDriver->GetHandle(), DriverObject);
    //
    // Invoke the driver if they specified an unload routine.
    //
    if (pDriver->m_DriverUnload.Method) {
        pDriver->m_DriverUnload.Invoke(pDriver->GetHandle());
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
                            "Driver unload routine Exit WDFDRIVER %p, PDRIVER_OBJECT_UM %p",
                            pDriver->GetHandle(), DriverObject);
    }

    //
    // Delete the FxDriver object.
    //
    // This releases the FxDriver reference.  Must be called at PASSIVE
    //
    pDriver->DeleteObject();

    pFxDriverGlobals->Driver = NULL;

    FxDestroy(pFxDriverGlobals);
}

VOID
FxDriver::_InitializeDriverName(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    Create a CHAR version of the service name contained in RegistryPath.

Arguments:
    RegistryPath - the path to the service name in the registry.

Return Value:
    None

  --*/
{
    PWCHAR pCur, pBegin, pEnd;

    RtlZeroMemory(&FxDriverGlobals->Public.DriverName[0],
                  sizeof(FxDriverGlobals->Public.DriverName) );

    if (RegistryPath == NULL) {
        return;
    }

    pBegin = RegistryPath->Buffer;

    //
    // pEnd is one past the end of the string, while pCur is a pointer to the
    // last character of the string.  We will decrement pCur down towards the
    // beginning of the string.
    //
    pEnd = pBegin + (RegistryPath->Length / sizeof(WCHAR));
    pCur = pEnd - 1;

    for ( ; *pCur != L'\\' && pCur != pBegin; pCur--) {
        DO_NOTHING();
    }

    if (pCur != pBegin && *pCur == L'\\') {
        size_t regLen;
        ULONG i;

        pCur++;

        //
        // Can't use wcslen becuase this is a UNICODE_STRING which means that it
        // does not necessarily have a terminating NULL in the buffer.
        //
        regLen = pEnd - pCur;
        if (regLen > WDF_DRIVER_GLOBALS_NAME_LEN-1) {
            regLen = WDF_DRIVER_GLOBALS_NAME_LEN-1;
        }


        for (i = 0; i < regLen; i++) {
            FxDriverGlobals->Public.DriverName[i] = (CHAR) pCur[i];
        }
    }
    else {
        NTSTATUS status;

#if FX_CORE_MODE==FX_CORE_KERNEL_MODE
        status = RtlStringCbCopyA(FxDriverGlobals->Public.DriverName,
                                  sizeof(FxDriverGlobals->Public.DriverName),
                                  "WDF");
#else // USER_MODE
        HRESULT hr;
        hr = StringCbCopyA(FxDriverGlobals->Public.DriverName,
                           sizeof(FxDriverGlobals->Public.DriverName),
                           "WDF");
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32) {
            status = WinErrorToNtStatus(HRESULT_CODE(hr));
        }
        else {
            status = SUCCEEDED(hr) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
        }
#endif

        UNREFERENCED_PARAMETER(status);
        ASSERT(NT_SUCCESS(status));
    }
}

VOID
FxDriver::_InitializeTag(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_DRIVER_CONFIG Config
    )
/*++

Routine Description:
    Tries to create a tag for the driver based off of its name.  This tag is
    used for allocations made on behalf of the driver so that it is easier to
    track which allocations belong to which image.

Arguments:
    Config - driver writer provided config.  in the future, may contain a tag
             value in it

Return Value:
    None

  --*/
{
    PCHAR  pBegin;
    size_t length;

    UNREFERENCED_PARAMETER(Config);

    length = strlen(FxDriverGlobals->Public.DriverName);
    pBegin = &FxDriverGlobals->Public.DriverName[0];

    if (length >= 3) {
        //
        // If the driver name begins with "WDF" (case insensitive), start after
        // "WDF"
        //
        if ((pBegin[0] == 'w' || pBegin[0] == 'W') &&
            (pBegin[1] == 'd' || pBegin[1] == 'D') &&
            (pBegin[2] == 'f' || pBegin[2] == 'F')) {
            length -=3;
            pBegin += 3;
        }
    }

    if (length <= 2) {
        //
        // 2 or less characters is not a unique enough tag, just use the default
        // tag.
        //
        FxDriverGlobals->Tag = FX_TAG;
    }
    else {

        if (length > sizeof(ULONG)) {
            length = sizeof(ULONG);
        }

        //
        // This copies the bytes in the right order (so that they appear correct
        // when dumped as a sequence of chars)
        //
        RtlCopyMemory(&FxDriverGlobals->Tag,
                      pBegin,
                      length);

        FxDriverGlobals->Public.DriverTag = FxDriverGlobals->Tag;
    }
}

_Must_inspect_result_
NTSTATUS
FxDriver::Initialize(
    __in PCUNICODE_STRING ArgRegistryPath,
    __in PWDF_DRIVER_CONFIG Config,
    __in_opt PWDF_OBJECT_ATTRIBUTES DriverAttributes
    )
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    NTSTATUS status;

    // WDFDRIVER can not be deleted by the device driver
    MarkNoDeleteDDI();

    MarkDisposeOverride(ObjectDoNotLock);

    //
    // Configure Constraints
    //
    ConfigureConstraints(DriverAttributes);

    if (m_DriverObject.GetObject() == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    // Allocate FxDisposeList
    status = FxDisposeList::_Create(FxDriverGlobals, m_DriverObject.GetObject(), &m_DisposeList);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Store FxDriver in Driver object extension
    //
    status = AllocateDriverObjectExtensionAndStoreFxDriver();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Store away the callback functions.
    //
    if ((Config->DriverInitFlags & WdfDriverInitNoDispatchOverride) == 0) {
        //
        // Caller doesn't want to override the dispatch table.  That
        // means that they want to create everything and still be in
        // control (or at least the port driver will take over)
        //
        m_DriverDeviceAdd.Method  = Config->EvtDriverDeviceAdd;
        m_DriverUnload.Method     = Config->EvtDriverUnload;
    }

    if (ArgRegistryPath != NULL) {
        USHORT length;

        length = ArgRegistryPath->Length + sizeof(UNICODE_NULL);

        m_RegistryPath.Length = ArgRegistryPath->Length;
        m_RegistryPath.MaximumLength = length;
        m_RegistryPath.Buffer = (PWSTR) FxPoolAllocate(
            GetDriverGlobals(), PagedPool, length);

        if (m_RegistryPath.Buffer != NULL) {
            RtlCopyMemory(m_RegistryPath.Buffer,
                          ArgRegistryPath->Buffer,
                          ArgRegistryPath->Length);

            //
            // other parts of WDF assumes m_RegistryPath.Buffer is
            // a null terminated string.  make sure it is.
            //
            m_RegistryPath.Buffer[length/sizeof(WCHAR)- 1] = UNICODE_NULL;
        }
        else {
            //
            // We failed to allocate space for the registry path
            // so set the length to 0.
            //
            m_RegistryPath.Length = 0;
            m_RegistryPath.MaximumLength = 0;

            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(status)) {
        if ((Config->DriverInitFlags & WdfDriverInitNoDispatchOverride) == 0) {
            UCHAR i;

            //
            // Set up dispatch routines.
            //
            if (Config->DriverInitFlags & WdfDriverInitNonPnpDriver) {
                //
                // NT4 style drivers must clear the AddDevice field in the
                // driver object so that they can be unloaded while still
                // having device objects around.
                //
                // If AddDevice is set, NT considers the driver a pnp driver
                // and will not allow net stop to unload the driver.
                //
                m_DriverObject.SetDriverExtensionAddDevice(NULL);

                //
                // Unload for an NT4 driver is still optional if the driver
                // does not want to be stopped (through net stop for
                // instance).
                //
                if (Config->EvtDriverUnload != NULL) {
                    m_DriverObject.SetDriverUnload(Unload);
                }
                else {
                    m_DriverObject.SetDriverUnload(NULL);
                }

            }
            else {
                //
                // PnP driver, set our routines up
                //
                m_DriverObject.SetDriverExtensionAddDevice(AddDevice);
                m_DriverObject.SetDriverUnload(Unload);
            }

            //
            // For any major control code that we use a remove lock, the
            // following locations must be updated:
            //
            // 1)  FxDevice::_RequiresRemLock() which decides if the remlock
            //          is required
            // 2)  FxDefaultIrpHandler::Dispatch might need to be changed if
            //          there is catchall generic post processing that must be done
            // 3)  Whereever the major code irp handler completes the irp or
            //          sends it down the stack.  A good design would have all
            //          spots in the irp handler where this is done to call a
            //          common function.
            //
            WDFCASSERT(IRP_MN_REMOVE_DEVICE != 0x0);

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
            for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
                if (FxDevice::_RequiresRemLock(i, 0x0) == FxDeviceRemLockNotRequired) {
                    m_DriverObject.SetMajorFunction(i, FxDevice::Dispatch);
                }
                else {
                    m_DriverObject.SetMajorFunction(i, FxDevice::DispatchWithLock);
                }
            }
#else // USER_MODE
            for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
                if (FxDevice::_RequiresRemLock(i, 0x0) == FxDeviceRemLockNotRequired) {
                    m_DriverObject.SetMajorFunction(i, FxDevice::DispatchUm);
                }
                else {
                    m_DriverObject.SetMajorFunction(i, FxDevice::DispatchWithLockUm);
                }
            }
#endif
        }

        //
        // Determine if the debugger is connected.
        //
#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
        if (KD_DEBUGGER_ENABLED == TRUE && KD_DEBUGGER_NOT_PRESENT == FALSE) {
            m_DebuggerConnected = TRUE;
        }
#endif

        //
        // Log this notable event after tracing has been initialized.
        //






        if ((Config->DriverInitFlags & WdfDriverInitNonPnpDriver) &&
            Config->EvtDriverUnload == NULL) {

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDRIVER,
                "Driver Object %p, reg path %wZ cannot be "
                "unloaded, no DriverUnload routine specified",
                m_DriverObject.GetObject(), &m_RegistryPath);
        }

#if FX_IS_USER_MODE
        //
        // Open a R/W handle to the driver's service parameters key
        //
        status = OpenParametersKey();
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
                "Cannot open Driver Parameters key %!STATUS!",
                status);
        }
#endif
    }

    return status;
}

_Must_inspect_result_
FxString *
FxDriver::GetRegistryPath(
    VOID
    )
{
    FxString *pString;

    pString = new(GetDriverGlobals(), WDF_NO_OBJECT_ATTRIBUTES)
        FxString(GetDriverGlobals());

    if (pString != NULL) {
        NTSTATUS status;

        status = pString->Assign(m_RegistryPath.Buffer);

        if (!NT_SUCCESS(status)) {
            pString->Release();
            pString = NULL;
        }
    }

    return pString;
}


VOID
FxDriver::ConfigureConstraints(
    __in_opt PWDF_OBJECT_ATTRIBUTES DriverAttributes
    )
/*++

Routine Description:

    Determine the pointer to the proper lock to acquire
    for event callbacks from the FxDriver to the device driver
    depending on the configured locking model.

Arguments:
    DriverAttributes - caller supplied scope and level, used only if they
        are not InheritFromParent.

Returns:
    None

--*/
{
    BOOLEAN automaticLockingRequired;

    automaticLockingRequired = FALSE;

    // Initialize the mutex lock
    m_CallbackMutexLock.Initialize(this);









    MarkPassiveCallbacks(ObjectDoNotLock);

    m_CallbackLockPtr = &m_CallbackMutexLock;
    m_CallbackLockObjectPtr = this;

    //
    // Use the caller supplied scope and level only if they are not
    // InheritFromParent.
    //
    if (DriverAttributes != NULL) {

        if (DriverAttributes->ExecutionLevel !=
                WdfExecutionLevelInheritFromParent) {
            m_ExecutionLevel = DriverAttributes->ExecutionLevel;
        }

        if (DriverAttributes->SynchronizationScope !=
                WdfSynchronizationScopeInheritFromParent) {
            m_SynchronizationScope = DriverAttributes->SynchronizationScope;
        }
    }

    //
    // If the driver asks for any synchronization, we synchronize the
    // WDFDRIVER object's own callbacks as well.
    //
    // (No option to extend synchronization for regular operations
    //  across all WDFDEVICE objects)
    //
    if (m_SynchronizationScope == WdfSynchronizationScopeDevice ||
        m_SynchronizationScope == WdfSynchronizationScopeQueue) {

        automaticLockingRequired = TRUE;
    }

    //
    // No FxDriver events are delivered from a thread that is
    // not already at PASSIVE_LEVEL, so we don't need to
    // allocate an FxSystemWorkItem if the execution level
    // constraint is WdfExecutionLevelPassive.
    //
    // If any events are added FxDriver that could occur on a thread
    // that is above PASSIVE_LEVEL, then an FxSystemWorkItem would
    // need to be allocated to deliver those events similar to the
    // code in FxIoQueue.
    //

    //
    // Configure FxDriver event callback locks
    //
    if (automaticLockingRequired) {
        m_DriverDeviceAdd.SetCallbackLockPtr(m_CallbackLockPtr);
    }
    else {
        m_DriverDeviceAdd.SetCallbackLockPtr(NULL);
    }
}

