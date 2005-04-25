/* $Id$
 *
 * kernel debugger prototypes
 */

#ifndef __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H
#define __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H

#include <internal/ke.h>
#include <internal/ldr.h>
#include <ntdll/ldr.h>

struct _KD_DISPATCH_TABLE;

#ifdef GDB
#include "kdgdb.h"
#endif
#ifdef BOCHS
#include "kdbochs.h"
#endif

/* KD ROUTINES ***************************************************************/

typedef enum _KD_CONTINUE_TYPE
{
    kdContinue = 0,
    kdDoNotHandleException,
    kdHandleException
} KD_CONTINUE_TYPE;

typedef
VOID
STDCALL
(*PKDP_INIT_ROUTINE)(struct _KD_DISPATCH_TABLE *DispatchTable,
                     ULONG BootPhase);

typedef
VOID
STDCALL
(*PKDP_PRINT_ROUTINE)(PCH String);

typedef
VOID
STDCALL
(*PKDP_PROMPT_ROUTINE)(PCH String);

typedef
KD_CONTINUE_TYPE
STDCALL
(*PKDP_EXCEPTION_ROUTINE)(PEXCEPTION_RECORD ExceptionRecord,
                          PCONTEXT Context,
                          PKTRAP_FRAME TrapFrame);

/* INIT ROUTINES *************************************************************/

VOID
STDCALL
KdpScreenInit(struct _KD_DISPATCH_TABLE *DispatchTable,
              ULONG BootPhase);
              
VOID
STDCALL
KdpSerialInit(struct _KD_DISPATCH_TABLE *DispatchTable,
              ULONG BootPhase);
              
VOID
STDCALL
KdpInitDebugLog(struct _KD_DISPATCH_TABLE *DispatchTable,
                ULONG BootPhase);
                
/* KD ROUTINES ***************************************************************/

KD_CONTINUE_TYPE
STDCALL
KdpEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
                          KPROCESSOR_MODE PreviousMode,
                          PCONTEXT Context,
                          PKTRAP_FRAME TrapFrame,
                          BOOLEAN FirstChance,
                          BOOLEAN Gdb);
                          
ULONG
STDCALL
KdpPrintString(PANSI_STRING String);

BOOLEAN
STDCALL
KdpDetectConflicts(PCM_RESOURCE_LIST DriverList);
              
/* KD GLOBALS  ***************************************************************/

/* serial debug connection */
#define DEFAULT_DEBUG_PORT      2 /* COM2 */
#define DEFAULT_DEBUG_COM1_IRQ  4 /* COM1 IRQ */
#define DEFAULT_DEBUG_COM2_IRQ  3 /* COM2 IRQ */
#define DEFAULT_DEBUG_BAUD_RATE 115200 /* 115200 Baud */

/* KD Native Modes */
#define KdScreen 0
#define KdSerial 1
#define KdFile 2
#define KdMax 3

/* KD Private Debug Modes */
typedef struct _KDP_DEBUG_MODE
{
    union {
        struct {
            /* Native Modes */
            UCHAR Screen :1;
            UCHAR Serial :1;
            UCHAR File   :1;
    
            /* Currently Supported Wrappers */
            UCHAR Pice   :1;
            UCHAR Gdb    :1;
            UCHAR Bochs  :1;
        };
        
        /* Generic Value */
        ULONG Value;
    };
} KDP_DEBUG_MODE;

/* KD Internal Debug Services */
typedef enum _KDP_DEBUG_SERVICE
{
    DumpNonPagedPool = 0,
    ManualBugCheck,
    DumpNonPagedPoolStats,
    DumpNewNonPagedPool,
    DumpNewNonPagedPoolStats,
    DumpAllThreads,
    DumpUserThreads,
    KdSpare1,
    KdSpare2,
    KdSpare3,
    EnterDebugger
} KDP_DEBUG_SERVICE;

/* Dispatch Table for Wrapper Functions */
typedef struct _KD_DISPATCH_TABLE
{
    LIST_ENTRY KdProvidersList;
    PKDP_INIT_ROUTINE KdpInitRoutine;
    PKDP_PRINT_ROUTINE KdpPrintRoutine;
    PKDP_PROMPT_ROUTINE KdpPromptRoutine;  
    PKDP_EXCEPTION_ROUTINE KdpExceptionRoutine;
} KD_DISPATCH_TABLE, *PKD_DISPATCH_TABLE;

/* The current Debugging Mode */
extern KDP_DEBUG_MODE KdpDebugMode;

/* The current Port IRQ */
extern ULONG KdpPortIrq;

/* The current Port */
extern ULONG KdpPort;

/* Port Information for the Serial Native Mode */
extern KD_PORT_INFORMATION SerialPortInfo;

/* Init Functions for Native Providers */
extern PKDP_INIT_ROUTINE InitRoutines[KdMax];

/* Wrapper Init Function */
extern PKDP_INIT_ROUTINE WrapperInitRoutine;
                                                        
/* Dispatch Tables for Native Providers */
extern KD_DISPATCH_TABLE DispatchTable[KdMax];

/* Dispatch Table for the Wrapper */
extern KD_DISPATCH_TABLE WrapperTable;
                                     
/* The KD Native Provider List */
extern LIST_ENTRY KdProviders;

#endif /* __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H */
