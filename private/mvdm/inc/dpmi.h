/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dpmi.h

Abstract:

    This file contains code to implement support for the DPMI bops

Author:

    Dave Hastings (daveh) 27-Jun-1991

Revision History:


--*/

/* ASM
ifdef WOW_x86
include vint.inc
endif
include bop.inc
*/
#define LDT_SIZE 0x1FFF

// DPMI Bop Sub Functions

#define InitDosxRM                  0
#define InitDosx                    1
#define InitLDT                     2
#define GetFastBopAddress           3
#define InitIDT                     4
#define InitExceptionHandlers       5
#define InitApp                     6
#define TerminateApp                7
#define DpmiInUse                   8
#define DpmiNoLongerInUse           9

#define DPMISwitchToProtectedMode   10 /* prefix necessary */
#define DPMISwitchToRealMode        11
#define SetAltRegs                  12

#define IntHandlerIret              13
#define IntHandlerIretd             14
#define FaultHandlerIret            15
#define FaultHandlerIretd           16
#define DpmiUnhandledException      17

#define RMCallBackCall              18
#define ReflectIntrToPM             19
#define ReflectIntrToV86            20

#define InitPmStackInfo             21
#define VcdPmSvcCall32              22
#define SetDescriptorTableEntries   23
#define ResetLDTUserBase            24

#define XlatInt21Call               25
#define Int31Entry                  26
#define Int31Call                   27

#define HungAppIretAndExit          28

#define MAX_DPMI_BOP_FUNC HungAppIretAndExit + 1

/* ASM
DPMIBOP macro SubFun
    BOP BOP_DPMI
    db SubFun
    endm
*/


//
// Definitions for real mode call backs
//

/* XLATOFF */
typedef struct _RMCB_INFO {
    BOOL bInUse;
    USHORT StackSel;
    USHORT StrucSeg;
    ULONG  StrucOffset;
    USHORT ProcSeg;
    ULONG  ProcOffset;
} RMCB_INFO;

// 16 is the minimum defined in the dpmi spec
#define MAX_RMCBS 16


typedef struct _MEM_DPMI {
    PVOID Address;
    ULONG Length;
    struct _MEM_DPMI * Prev;
    struct _MEM_DPMI * Next;
    WORD Owner;
    WORD Sel;
    WORD SelCount;
} MEM_DPMI, *PMEM_DPMI;

/* XLATON */

