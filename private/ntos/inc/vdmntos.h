/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmntos.h

Abstract:

    This is the include file for the vdm component.  It describes the kernel
    mode visible portions of the vdm component.  The \nt\private\inc\vdm.h
    file describes the portions that are usermode visible.

Author:

    Dave Hastings (daveh) 02-Feb-1992

Revision History:

--*/

#ifndef _VDMNTOS_
#define _VDMNTOS_

//
// Need this #include here because non-x86 ntos\vdm\vdm.c
// references structures defined there.
//

#include "..\..\inc\vdm.h"

#if defined(i386)

typedef struct _VDM_IO_LISTHEAD {
    PVDM_IO_HANDLER VdmIoHandlerList;
    ERESOURCE       VdmIoResource;
    ULONG           Context;
} VDM_IO_LISTHEAD, *PVDM_IO_LISTHEAD;


typedef struct _VDM_PROCESS_OBJECTS {
    PVDM_IO_LISTHEAD VdmIoListHead;
    KAPC             QueuedIntApc;
    KAPC             QueuedIntUserApc;
    FAST_MUTEX       DelayIntFastMutex;
    KSPIN_LOCK       DelayIntSpinLock;
    LIST_ENTRY       DelayIntListHead;
    PVDMICAUSERDATA  pIcaUserData;
    PETHREAD         MainThread;
    PVDM_TIB         VdmTib;
    PUCHAR           PrinterState;
    PUCHAR           PrinterControl;
    PUCHAR           PrinterStatus;
    PUCHAR           PrinterHostState;
} VDM_PROCESS_OBJECTS, *PVDM_PROCESS_OBJECTS;


typedef struct _DelayInterruptsIrq {
    LIST_ENTRY  DelayIntListEntry;
    ULONG       IrqLine;
    PETHREAD    Thread;
    KDPC        Dpc;
    KAPC        Apc;
    KTIMER      Timer;
    BOOLEAN     InUse;
    PETHREAD    MainThread;
} DELAYINTIRQ, *PDELAYINTIRQ;

#define VDMDELAY_NOTINUSE 0
#define VDMDELAY_KTIMER   1
#define VDMDELAY_PTIMER   2
#define VDMDELAY_KAPC     3


BOOLEAN
Ps386GetVdmIoHandler(
    IN PEPROCESS Process,
    IN ULONG PortNumber,
    OUT PVDM_IO_HANDLER VdmIoHandler,
    OUT PULONG Context
    );

#define SEL_TYPE_READ       0x00000001
#define SEL_TYPE_WRITE      0x00000002
#define SEL_TYPE_EXECUTE    0x00000004
#define SEL_TYPE_BIG        0x00000008
#define SEL_TYPE_ED         0x00000010
#define SEL_TYPE_2GIG       0x00000020
#define SEL_TYPE_NP         0x00000040

// NPX error exception dispatcher
BOOLEAN
VdmDispatchIRQ13(
    PKTRAP_FRAME TrapFrame
    );

BOOLEAN
VdmSkipNpxInstruction(
    PKTRAP_FRAME TrapFrame,
    ULONG        Address32Bits,
    PUCHAR       istream,
    ULONG        InstructionSize
    );

VOID
VdmEndExecution(
    PKTRAP_FRAME TrapFrame,
    PVDM_TIB VdmTib
    );

NTSTATUS
VdmDispatchInterrupts(
    PKTRAP_FRAME TrapFrame,
    PVDM_TIB     VdmTib
    );

VOID
VdmDispatchException(
     PKTRAP_FRAME TrapFrame,
     NTSTATUS     ExcepCode,
     PVOID        ExcepAddress,
     ULONG        NumParms,
     ULONG        Parm1,
     ULONG        Parm2,
     ULONG        Parm3
     );

#define VdmGetTrapFrame(pKThread) \
        ((PKTRAP_FRAME)( (PUCHAR)(pKThread)->InitialStack -              \
                         sizeof(FX_SAVE_AREA) -                          \
                         ((ULONG)(sizeof(KTRAP_FRAME)+KTRAP_FRAME_ROUND) \
                           & ~(KTRAP_FRAME_ROUND))                       \
                        )                                                \
         )

//
// These values are defined here to describe the structure of an array
// containing running counts of v86 opcode emulation. The array lives in
// ke\i386, but is referenced in ex.
//
#define VDM_INDEX_Invalid             0
#define VDM_INDEX_0F                  1
#define VDM_INDEX_ESPrefix            2
#define VDM_INDEX_CSPrefix            3
#define VDM_INDEX_SSPrefix            4
#define VDM_INDEX_DSPrefix            5
#define VDM_INDEX_FSPrefix            6
#define VDM_INDEX_GSPrefix            7
#define VDM_INDEX_OPER32Prefix        8
#define VDM_INDEX_ADDR32Prefix        9
#define VDM_INDEX_INSB               10
#define VDM_INDEX_INSW               11
#define VDM_INDEX_OUTSB              12
#define VDM_INDEX_OUTSW              13
#define VDM_INDEX_PUSHF              14
#define VDM_INDEX_POPF               15
#define VDM_INDEX_INTnn              16
#define VDM_INDEX_INTO               17
#define VDM_INDEX_IRET               18
#define VDM_INDEX_NPX                19
#define VDM_INDEX_INBimm             20
#define VDM_INDEX_INWimm             21
#define VDM_INDEX_OUTBimm            22
#define VDM_INDEX_OUTWimm            23
#define VDM_INDEX_INB                24
#define VDM_INDEX_INW                25
#define VDM_INDEX_OUTB               26
#define VDM_INDEX_OUTW               27
#define VDM_INDEX_LOCKPrefix         28
#define VDM_INDEX_REPNEPrefix        29
#define VDM_INDEX_REPPrefix          30
#define VDM_INDEX_CLI                31
#define VDM_INDEX_STI                32
#define VDM_INDEX_HLT                33

// The following value must be 1 more than the last defined index value
#define MAX_VDM_INDEX                34

//
// This is the address of the Vdm communication area.  It's in ke\i386\vdm.c
//

extern ULONG VdmFixedStateLinear;

#define FIXED_NTVDMSTATE_LINEAR_PC_AT   0x714
#define FIXED_NTVDMSTATE_LINEAR_PC_98   0x614

#endif // i386
#endif // _VDMNTOS_
