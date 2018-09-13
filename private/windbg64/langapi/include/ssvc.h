/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    ssvc.h

Abstract:

    This header contains the enumeration of system services provided
    by OSDebug 4.

Author:

    Kent Forschmiedt (kentf) 09-Oct-1996

Environment:

    Win32, User Mode

--*/



typedef enum {
        ssvcNull = 0,
        ssvcDumpLocalHeap,
        ssvcDumpGlobalHeap,
        ssvcDumpModuleList,
        ssvcCrackLocalHmem,
        ssvcCrackGlobalHmem,
        ssvcKillApplication,
        ssvcFreeLibrary,
        ssvcInput,
        ssvcOutput,
        ssvcOleRpc,             // Enable/disable OLE Remote Procedure Call tracing
                                    // Pass cb = 1, rgb[0] = fEnable.  Before this is
                                    // called the first time, OLE RPC debugging is
                                    // disabled.  Also see mtrcOleRpc.
        ssvcHackFlipScreen,     // Hack for testing: toggle switching previous
                                    // foreground window back to foreground on F8/F10.
        ssvcNativeDebugger,     // Activate remote debugger
        ssvcSetETS,
        ssvcCvtRez2Seg,
        ssvcSqlDebug,
        ssvcFiberDebug,
} SSVC;
