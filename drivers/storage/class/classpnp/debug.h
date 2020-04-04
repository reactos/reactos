/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    debug.h

Abstract:


Author:

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#pragma once

VOID ClassDebugPrint(CLASS_DEBUG_LEVEL DebugPrintLevel, PCCHAR DebugMessage, ...);

#if DBG

    typedef struct _CLASSPNP_GLOBALS {

        //
        // whether or not to ASSERT for lost irps
        //

        ULONG BreakOnLostIrps;
        ULONG SecondsToWaitForIrps;

        //
        // use a buffered debug print to help
        // catch timing issues that do not
        // reproduce with std debugprints enabled
        //

        ULONG UseBufferedDebugPrint;
        ULONG UseDelayedRetry;

        //
        // the next four are the buffered printing support
        // (currently unimplemented) and require the spinlock
        // to use
        //

        ULONG Index;                // index into buffer
        KSPIN_LOCK SpinLock;
        PSTR Buffer;                // requires spinlock to access
        ULONG NumberOfBuffers;      // number of buffers available
        SIZE_T EachBufferSize;      // size of each buffer

        //
        // interlocked variables to initialize
        // this data only once
        //

        LONG Initializing;
        LONG Initialized;

    } CLASSPNP_GLOBALS, *PCLASSPNP_GLOBALS;

    #define DBGTRACE(dbgTraceLevel, args_in_parens)                                \
        if (ClassDebug & (1 << (dbgTraceLevel+15))){                                               \
            DbgPrint("CLASSPNP> *** TRACE *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("    >  "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            if (DebugTrapOnWarn && (dbgTraceLevel == ClassDebugWarning)){ \
                DbgBreakPoint();  \
            } \
        }
    #define DBGWARN(args_in_parens)                                \
        {                                               \
            DbgPrint("CLASSPNP> *** WARNING *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("    >  "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            if (DebugTrapOnWarn){ \
                DbgBreakPoint();  \
            } \
        }
    #define DBGERR(args_in_parens)                                \
        {                                               \
            DbgPrint("CLASSPNP> *** ERROR *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("    >  "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            DbgBreakPoint();                            \
        }
    #define DBGTRAP(args_in_parens)                                \
        {                                               \
            DbgPrint("CLASSPNP> *** COVERAGE TRAP *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("    >  "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            DbgBreakPoint();                            \
        }


    #define DBGGETIOCTLSTR(_ioctl) DbgGetIoctlStr(_ioctl)
    #define DBGGETSCSIOPSTR(_pSrb) DbgGetScsiOpStr(_pSrb)
    #define DBGGETSENSECODESTR(_pSrb) DbgGetSenseCodeStr(_pSrb)
    #define DBGGETADSENSECODESTR(_pSrb) DbgGetAdditionalSenseCodeStr(_pSrb)
    #define DBGGETADSENSEQUALIFIERSTR(_pSrb) DbgGetAdditionalSenseCodeQualifierStr(_pSrb)
    #define DBGCHECKRETURNEDPKT(_pkt) DbgCheckReturnedPkt(_pkt)
    #define DBGGETSRBSTATUSSTR(_pSrb) DbgGetSrbStatusStr(_pSrb)
    
    VOID ClasspInitializeDebugGlobals(VOID);
    char *DbgGetIoctlStr(ULONG ioctl);
    char *DbgGetScsiOpStr(PSCSI_REQUEST_BLOCK Srb);
    char *DbgGetSenseCodeStr(PSCSI_REQUEST_BLOCK Srb);
    char *DbgGetAdditionalSenseCodeStr(PSCSI_REQUEST_BLOCK Srb);
    char *DbgGetAdditionalSenseCodeQualifierStr(PSCSI_REQUEST_BLOCK Srb);
    VOID DbgCheckReturnedPkt(TRANSFER_PACKET *Pkt);
    char *DbgGetSrbStatusStr(PSCSI_REQUEST_BLOCK Srb);


    extern CLASSPNP_GLOBALS ClasspnpGlobals;
    extern LONG ClassDebug;
    extern BOOLEAN DebugTrapOnWarn;

#else

    #define ClasspInitializeDebugGlobals()
    #define DBGWARN(args_in_parens)                                
    #define DBGERR(args_in_parens)                                
    #define DBGTRACE(dbgTraceLevel, args_in_parens)                                
    #define DBGTRAP(args_in_parens)
    
    #define DBGGETIOCTLSTR(_ioctl)
    #define DBGGETSCSIOPSTR(_pSrb)
    #define DBGGETSENSECODESTR(_pSrb)    
    #define DBGGETADSENSECODESTR(_pSrb)
    #define DBGGETADSENSEQUALIFIERSTR(_pSrb)
    #define DBGCHECKRETURNEDPKT(_pkt)
    #define DBGGETSRBSTATUSSTR(_pSrb)
    
#endif
