/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/internal/vdm.h
* PURPOSE:         Internal header for V86 and VDM Support
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Define this if you want debugging support
//
#define _VM_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define VM_EXEC_DEBUG                                   0x01

//
// Debug/Tracing support
//
#if _VM_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define VMTRACE DbgPrintEx
#else
#define VMTRACE(x, ...)                                 \
    if (x & VdmpTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define VMTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

//
// Memory addresses inside CSRSS for V86 Support
//
#define TRAMPOLINE_BASE                                 0x10000
#define TRAMPOLINE_TIB                                  0x12000
#define TRAMPOLINE_TEB                                  0x13000

//
// BOP (Magic Opcode) to exit V86 Mode
//
#define TRAMPOLINE_BOP                                  0xFEC4C4

//
// VDM State Pointer
//
#define VdmState                                        \
    (PULONG)FIXED_NTVDMSTATE_LINEAR_PC_AT

//
// VDM Event Types
//
typedef enum _VdmEventClass
{
    VdmIO,
    VdmStringIO,
    VdmMemAccess,
    VdmIntAck,
    VdmBop,
    VdmError,
    VdmIrq13
} VDMEVENTCLASS, *PVDMEVENTCLASS;

//
// VDM Interrupt and Fault Handler Definitions
//
typedef struct _Vdm_InterruptHandler
{
    USHORT CsSelector;
    USHORT Flags;
    ULONG Eip;
} VDM_INTERRUPTHANDLER, *PVDM_INTERRUPTHANDLER;

typedef struct _Vdm_FaultHandler
{
    USHORT CsSelector;
    USHORT SsSelector;
    ULONG Eip;
    ULONG Esp;
    ULONG Flags;
} VDM_FAULTHANDLER, *PVDM_FAULTHANDLER;

//
// VDM Event Information
//
typedef struct _VdmEventInfo
{
    ULONG Size;
    VDMEVENTCLASS Event;
    ULONG InstructionSize;
    union
    {
        //VDMIOINFO IoInfo;
        //VDMSTRINGIOINFO StringIoInfo;
        ULONG BopNumber;
        //VDMFAULTINFO FaultInfo;
        LONG ErrorStatus;
        ULONG IntAckInfo;
    };
} VDMEVENTINFO, *PVDMEVENTINFO;

//
// VDM Printer Information
//
typedef struct _Vdm_Printer_Info
{
    PUCHAR prt_state;
    // TODO
} VDM_PRINTER_INFO, *PVDM_PRINTER_INFO;

//
// VDM Trace Information
//
typedef struct _VdmTraceInfo
{
    // TODO
    UCHAR Flags;
    // TODO
} VDMTRACEINFO, *PVDMTRACEINFO;

//
// VDM Family Table
//
typedef struct _tagFAMILY_TABLE
{
    INT numHookedAPIs;
    // TODO
} FAMILY_TABLE, *PFAMILY_TABLE;

//
// Thread Information Block for VDM Threads
//
typedef struct _Vdm_Tib
{
    ULONG Size;
    PVDM_INTERRUPTHANDLER VdmInterruptTable;
    PVDM_FAULTHANDLER VdmFaultTable;
    CONTEXT MonitorContext;
    CONTEXT VdmContext;
    VDMEVENTINFO EventInfo;
    VDM_PRINTER_INFO PrinterInfo;
    ULONG TempArea1[2];
    ULONG TempArea2[2];
    VDMTRACEINFO TraceInfo;
    ULONG IntelMSW;
    LONG NumTasks;
    PFAMILY_TABLE *pDpmFamTbls;
    BOOLEAN ContinueExecution;
} VDM_TIB, *PVDM_TIB;

//
// Process Information Block for VDM Processes
//
typedef struct _VDM_PROCESS_OBJECTS
{
    PVOID VdmIoListHead; // PVDM_IO_LISTHEAD
    KAPC QueuedIntApc;
    KAPC QueuedIntUserApc;
    FAST_MUTEX DelayIntFastMutex;
    KSPIN_LOCK DelayIntSpinLock;
    LIST_ENTRY DelayIntListHead;
    PVOID pIcaUserData; // VDMICAUSERDATA
    PETHREAD MainThread;
    PVDM_TIB VdmTib;
    UCHAR PrinterState;
    UCHAR PrinterControl;
    UCHAR PrinterStatus;
    UCHAR PrinterHostState;
    USHORT AdlibStatus;
    USHORT AdlibIndexRegister;
    USHORT AdlibPhysPortStart;
    USHORT AdlibPhysPortEnd;
    USHORT AdlibVirtPortStart;
    USHORT AdlibVirtPortEnd;
    USHORT AdlibAction;
    USHORT VdmControl;
    ULONG PMCliTimeStamp;
} VDM_PROCESS_OBJECTS, *PVDM_PROCESS_OBJECTS;

//
// Functions
//
NTSTATUS
NTAPI
VdmpStartExecution(
    VOID
);

//
// Global data inside the VDM
//


