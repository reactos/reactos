#include "common/fxdriver.h"
#include "common/fxmacros.h"
#include "common/fxglobals.h"
#include "common/fxtrace.h"
#include "wdf.h"
#include <ntstrsafe.h>



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
    m_DriverPersistentStateKey = NULL;
    m_DriverDataDirectory = NULL;
#endif
}

FxDriver::~FxDriver()
{
    // Make it always present right now even on free builds
    if (IsDisposed() == FALSE)
    {
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
    if (m_RegistryPath.Buffer)
    {
        FxPoolFree(m_RegistryPath.Buffer);
    }

    if (m_DisposeList != NULL)
    {
        m_DisposeList->DeleteObject();
    }

#if FX_IS_USER_MODE
    //
    // Close the R/W handles to the driver's service parameters key and persistent
    // state key that we opened during Initialize.
    //
    if (m_DriverParametersKey != NULL)
    {
        NTSTATUS status = FxRegKey::_Close(m_DriverParametersKey);
        if (!NT_SUCCESS(status))
        {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDRIVER,
                                "Cannot close Driver Parameters key %!STATUS!",
                                status);
        }
        m_DriverParametersKey = NULL;
    }

    if (m_DriverPersistentStateKey != NULL)
    {
        NTSTATUS status = FxRegKey::_Close(m_DriverPersistentStateKey);
        if (!NT_SUCCESS(status))
        {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDRIVER,
                                "Cannot close Driver persistent state key %!STATUS!",
                                status);
        }
        m_DriverPersistentStateKey = NULL;
    }

    //
    // Cleanup the persistent driver state directory path created during Initialize.
    //
    if (m_DriverDataDirectory != NULL)
    {
        m_DriverDataDirectory->DeleteObject();
        m_DriverDataDirectory = NULL;
    }

    //
    // The host-created driver object holds a reference to this
    // FxDriver object. Clear it, since this object was deleted.
    //
    ClearDriverObjectFxDriver();
#endif

    //if ((GetDriverGlobals()->FxVerifierOn) &&
    //    (GetDriverGlobals()->FxVerifyLeakDetection != NULL) )
    //{
    //    ASSERT(GetDriverGlobals()->FxVerifyLeakDetection->ObjectCnt == 0);
    //}
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

    if (RegistryPath == NULL)
    {
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

    for ( ; *pCur != L'\\' && pCur != pBegin; pCur--)
    {
        DO_NOTHING();
    }

    if (pCur != pBegin && *pCur == L'\\')
    {
        size_t regLen;
        ULONG i;

        pCur++;

        //
        // Can't use wcslen becuase this is a UNICODE_STRING which means that it
        // does not necessarily have a terminating NULL in the buffer.
        //
        regLen = pEnd - pCur;
        if (regLen > WDF_DRIVER_GLOBALS_NAME_LEN-1)
        {
            regLen = WDF_DRIVER_GLOBALS_NAME_LEN-1;
        }

        for (i = 0; i < regLen; i++)
        {
            FxDriverGlobals->Public.DriverName[i] = (CHAR) pCur[i];
        }
    }
    else
    {
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

        if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        {
            status = WinErrorToNtStatus(HRESULT_CODE(hr));
        }
        else
        {
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

    if (length >= 3)
    {
        //
        // If the driver name begins with "WDF" (case insensitive), start after
        // "WDF"
        //
        if ((pBegin[0] == 'w' || pBegin[0] == 'W') &&
            (pBegin[1] == 'd' || pBegin[1] == 'D') &&
            (pBegin[2] == 'f' || pBegin[2] == 'F'))
        {
            length -=3;
            pBegin += 3;
        }
    }

    if (length <= 2)
    {
        //
        // 2 or less characters is not a unique enough tag, just use the default
        // tag.
        //
        FxDriverGlobals->Tag = FX_TAG;
    }
    else
    {

        if (length > sizeof(ULONG))
        {
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
