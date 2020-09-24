//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "Mx.h"

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
    NTSTATUS    status;

    va_start(list, DebugMessage);

    if (DebugMessage) {

        //
        // Using new safe string functions instead of _vsnprintf.
        // This function takes care of NULL terminating if the message
        // is longer than the buffer.
        //
        status = RtlStringCbVPrintfA( debugMessageBuffer,
                                      sizeof(debugMessageBuffer),
                                      DebugMessage,
                                      list );
        if(!NT_SUCCESS(status)) {

            DbgPrint ("WDF DbgPrint: Unable to expand: %s", DebugMessage);
        }
        else {
            DbgPrint("%s", debugMessageBuffer);
        }
    }
    va_end(list);

#else
    UNREFERENCED_PARAMETER(DebugMessage);
#endif
    return;
}


VOID
Mx::MxGlobalInit(
    VOID
    )
{
    //
    // Global initialization for kernel-mode primitives
    //
}
