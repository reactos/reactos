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
#define KdPrintEx(_x_) DbgPrintEx _x_

#ifdef DBG
#include "kdgdb.h"
#include "kdbochs.h"
#endif

/* SYMBOL ROUTINES **********************************************************/

#if defined(KDBG) || defined(DBG)

VOID
KdbSymLoadUserModuleSymbols(IN PLDR_MODULE LdrModule);

VOID
KdbSymFreeProcessSymbols(IN PEPROCESS Process);

VOID
KdbSymLoadDriverSymbols(IN PUNICODE_STRING Filename,
                        IN PMODULE_OBJECT Module);

VOID
KdbSymUnloadDriverSymbols(IN PMODULE_OBJECT ModuleObject);

VOID
KdbSymProcessBootSymbols(IN PCHAR FileName);

VOID
KdbSymInit(IN PMODULE_TEXT_SECTION NtoskrnlTextSection,
           IN PMODULE_TEXT_SECTION LdrHalTextSection);

BOOLEAN 
KdbSymPrintAddress(IN PVOID Address);

VOID
KdbDeleteProcessHook(IN PEPROCESS Process);

NTSTATUS
KdbSymGetAddressInformation(IN PROSSYM_INFO  RosSymInfo,
                            IN ULONG_PTR  RelativeAddress,
                            OUT PULONG LineNumber  OPTIONAL,
                            OUT PCH FileName  OPTIONAL,
                            OUT PCH FunctionName  OPTIONAL);

typedef struct _KDB_MODULE_INFO
{
    WCHAR        Name[256];
    ULONG_PTR    Base;
    ULONG        Size;
    PROSSYM_INFO RosSymInfo;
} KDB_MODULE_INFO, *PKDB_MODULE_INFO;

/* MACROS FOR NON-KDBG BUILDS ************************************************/

# define KDB_LOADUSERMODULE_HOOK(LDRMOD)	KdbSymLoadUserModuleSymbols(LDRMOD)
# define KDB_LOADDRIVER_HOOK(FILENAME, MODULE)	KdbSymLoadDriverSymbols(FILENAME, MODULE)
# define KDB_UNLOADDRIVER_HOOK(MODULE)		KdbSymUnloadDriverSymbols(MODULE)
# define KDB_LOADERINIT_HOOK(NTOS, HAL)		KdbSymInit(NTOS, HAL)
# define KDB_SYMBOLFILE_HOOK(FILENAME)		KdbSymProcessBootSymbols(FILENAME)
#else
# define KDB_LOADUSERMODULE_HOOK(LDRMOD)	do { } while (0)
# define KDB_LOADDRIVER_HOOK(FILENAME, MODULE)	do { } while (0)
# define KDB_UNLOADDRIVER_HOOK(MODULE)		do { } while (0)
# define KDB_LOADERINIT_HOOK(NTOS, HAL)		do { } while (0)
# define KDB_SYMBOLFILE_HOOK(FILENAME)		do { } while (0)
# define KDB_CREATE_THREAD_HOOK(CONTEXT)	do { } while (0)
#endif

#if defined(KDBG) || defined(DBG)
# define KeRosPrintAddress(ADDRESS)         KdbSymPrintAddress(ADDRESS)
#else
# define KeRosPrintAddress(ADDRESS)         KiRosPrintAddress(ADDRESS)
#endif

#ifdef KDBG
# define KdbInit()                          KdbpCliInit()
# define KdbModuleLoaded(FILENAME)          KdbpCliModuleLoaded(FILENAME)
# define KDB_DELETEPROCESS_HOOK(PROCESS)	KdbDeleteProcessHook(PROCESS)
#else
# define KdbEnterDebuggerException(ER, PM, C, TF, F)  kdHandleException
# define KdbInit()                          do { } while (0)
# define KdbEnter()                         do { } while (0)
# define KdbModuleLoaded(X)                 do { } while (0)
# define KDB_DELETEPROCESS_HOOK(PROCESS)	do { } while (0)
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
    DumpNonPagedPool		= 0x1e, /* a */
    ManualBugCheck		= 0x30, /* b */
    DumpNonPagedPoolStats	= 0x2e, /* c */
    DumpNewNonPagedPool		= 0x20, /* d */
    DumpNewNonPagedPoolStats	= 0x12, /* e */
    DumpAllThreads		= 0x21, /* f */
    DumpUserThreads		= 0x22, /* g */
    KdSpare1			= 0x23, /* h */
    KdSpare2			= 0x17, /* i */
    KdSpare3			= 0x24, /* j */
    EnterDebugger		= 0x25  /* k */
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
