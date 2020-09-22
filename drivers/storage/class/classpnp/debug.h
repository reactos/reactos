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

#define DBGGETIOCTLSTR(_ioctl)              DbgGetIoctlStr(_ioctl)
#define DBGGETSCSIOPSTR(_pSrb)              DbgGetScsiOpStr(_pSrb)
#define DBGGETSRBSTATUSSTR(_pSrb)           DbgGetSrbStatusStr(_pSrb)
#define DBGGETSENSECODESTR(_pSrb)           DbgGetSenseCodeStr(_pSrb)
#define DBGGETADSENSECODESTR(_pSrb)         DbgGetAdditionalSenseCodeStr(_pSrb)
#define DBGGETADSENSEQUALIFIERSTR(_pSrb)    DbgGetAdditionalSenseCodeQualifierStr(_pSrb)

char *DbgGetIoctlStr(ULONG ioctl);
char *DbgGetScsiOpStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb);
char *DbgGetSrbStatusStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb);
char *DbgGetSenseCodeStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb);
char *DbgGetAdditionalSenseCodeStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb);
char *DbgGetAdditionalSenseCodeQualifierStr(PSTORAGE_REQUEST_BLOCK_HEADER Srb);

#if DBG

    typedef struct _CLASSPNP_GLOBALS {

        //
        // whether or not to NT_ASSERT for lost irps
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
        PUCHAR Buffer;              // requires spinlock to access
        ULONG NumberOfBuffers;      // number of buffers available
        SIZE_T EachBufferSize;      // size of each buffer

        //
        // interlocked variables to initialize
        // this data only once
        //

        LONG Initializing;
        LONG Initialized;

    } CLASSPNP_GLOBALS, *PCLASSPNP_GLOBALS;


    //
    // Define a structure used to capture stack traces when we
    // get an access request and the disks are powered off.  This
    // will help us determine who's causing disk respins.
    //

    //
    // How many stack frames to capture each time?
    //
    #define DISK_SPINUP_BACKTRACE_LENGTH    (0x18)

    //
    // How many stack traces can we capture before
    // out buffer wraps? (needs to be power of 2)
    //
    #define NUMBER_OF_DISK_SPINUP_TRACES    (0x10)

    typedef struct _DISK_SPINUP_TRACES {

        LARGE_INTEGER   TimeStamp;  // timestamp of the spinup event.
        PVOID   StackTrace[DISK_SPINUP_BACKTRACE_LENGTH]; // Holds stack trace
    } DISK_SPINUP_TRACES, *PDISK_SPINUP_TRACES;


    #define DBGCHECKRETURNEDPKT(_pkt) DbgCheckReturnedPkt(_pkt)
    #define DBGLOGSENDPACKET(_pkt) DbgLogSendPacket(_pkt)
    #define DBGLOGRETURNPACKET(_pkt) DbgLogReturnPacket(_pkt)
    #define DBGLOGFLUSHINFO(_fdoData, _isIO, _isFUA, _isFlush) DbgLogFlushInfo(_fdoData, _isIO, _isFUA, _isFlush)

    VOID ClasspInitializeDebugGlobals();
    VOID DbgCheckReturnedPkt(TRANSFER_PACKET *Pkt);
    VOID DbgLogSendPacket(TRANSFER_PACKET *Pkt);
    VOID DbgLogReturnPacket(TRANSFER_PACKET *Pkt);
    VOID DbgLogFlushInfo(PCLASS_PRIVATE_FDO_DATA FdoData, BOOLEAN IsIO, BOOLEAN IsFUA, BOOLEAN IsFlush);
    VOID SnapDiskStartup(VOID);
    extern CLASSPNP_GLOBALS ClasspnpGlobals;
    extern LONG ClassDebug;
    extern BOOLEAN DebugTrapOnWarn;

#else

    #define ClasspInitializeDebugGlobals()
    #define SnapDiskStartup()

    #define DBGCHECKRETURNEDPKT(_pkt)
    #define DBGLOGSENDPACKET(_pkt)
    #define DBGLOGRETURNPACKET(_pkt)
    #define DBGLOGFLUSHINFO(_fdoData, _isIO, _isFUA, _isFlush)

#endif

