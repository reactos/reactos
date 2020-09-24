/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxGeneralUm.cpp

Abstract:

    Implementation of MxGeneralUm functions.

Author:



Revision History:



--*/

#include "Mx.h"

#include <strsafe.h>

#include "fxmin.hpp"

extern "C" IUMDFPlatform *g_IUMDFPlatform;

VOID
Mx::MxDbgPrint(
    __drv_formatString(printf)
    __in PCSTR DebugMessage,
    ...
    )
{
#if DBG

#define         TEMP_BUFFER_SIZE        1024
    va_list     list;
    CHAR        debugMessageBuffer[TEMP_BUFFER_SIZE];
    HRESULT     hr;

    va_start(list, DebugMessage);

    if (DebugMessage) {

        //
        // Using new safe string functions instead of _vsnprintf.
        // This function takes care of NULL terminating if the message
        // is longer than the buffer.
        //
        hr = StringCbVPrintfA( debugMessageBuffer,
                               sizeof(debugMessageBuffer),
                               DebugMessage,
                               list );
        if(!SUCCEEDED(hr)) {
            OutputDebugStringA("WDF DbgPrint: Unable to expand: ");
            OutputDebugStringA(DebugMessage);
        }
        else {
            OutputDebugStringA(debugMessageBuffer);
        }
    }
    va_end(list);

#else
    UNREFERENCED_PARAMETER(DebugMessage);
#endif
    return;
}

VOID
Mx::MxBugCheckEx(
    _In_ ULONG  BugCheckCode,
    _In_ ULONG_PTR  BugCheckParameter1,
    _In_ ULONG_PTR  BugCheckParameter2,
    _In_ ULONG_PTR  BugCheckParameter3,
    _In_ ULONG_PTR  BugCheckParameter4
)
{

    UNREFERENCED_PARAMETER(BugCheckParameter1);
    UNREFERENCED_PARAMETER(BugCheckParameter2);
    UNREFERENCED_PARAMETER(BugCheckParameter3);
    UNREFERENCED_PARAMETER(BugCheckParameter4);

    FX_VERIFY(DRIVER(BadAction, BugCheckCode), TRAPMSG("UMDF verification "
                "faults should not call this code path"));
}

VOID
Mx::MxGlobalInit(
    VOID
    )
{
    //
    // Currently just a placeholder. If there is any global initialization
    // needed for the user-mode primitives, it can be done here.
    //
}

VOID
Mx::MxDetachDevice(
    _Inout_ MdDeviceObject Device
    )
{





    HRESULT hrDetachDev =
        Device->GetDeviceStackInterface()->DetachDevice(Device);





    UNREFERENCED_PARAMETER(hrDetachDev);
}

