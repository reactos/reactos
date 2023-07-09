/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    genvdmtb.m4

Abstract:

    This module implements a source code which generates a source .obj
    for genxx.exe.  This results in definitions for kernel structures that
    are accessed in x86 assembly.

    This version is preprocessed with M4, then compiled with the 386 compiler.
    To build, go to vdm\up and do "nmake UMAPPL=genvdmtb" to generate
    genvdmtb.obj.  Then run "genxx -vdm" to create \nt\private\vdmtb.inc.

Author:

    Bryan M. Willman (bryanwi) 16-Oct-90

Revision History:

    Dave Hastings (daveh) 27-Mar-93
        Stolen from the ke directory to correct a maintinence problem

    Forrest Foltz (forrestf) 24-Jan-1998
        Modified format to use new obj-based procedure.

--*/

#include "crt\excpt.h"
#include "crt\stdarg.h"
#include "ntdef.h"
#include "ntstatus.h"
#include "ntkeapi.h"
#include "nti386.h"
#include "ntseapi.h"
#include "ntobapi.h"
#include "ntimage.h"
#include "ntldr.h"
#include "ntpsapi.h"
#include "ntxcapi.h"
#include "ntlpcapi.h"
#include "ntioapi.h"
#include "ntexapi.h"
#include "ntmmapi.h"
#include "ntnls.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntconfig.h"

#include "ntcsrsrv.h"

#include "ntdef.h"
#include "vdm.h"
#include "stdio.h"

include(`..\..\ke\genxx.h')

//
// Element description array is here
//

STRUC_ELEMENT ElementList[] = {

    START_LIST

    genTxt("IFDEF NEC_98\n")
    genVal(FIXED_NTVDMSTATE_SEGMENT, 0x60)
    genTxt("else\n")
    genVal(FIXED_NTVDMSTATE_SEGMENT, 0x70)
    genTxt("endif\n")

    genNam(FIXED_NTVDMSTATE_OFFSET)
    genTxt("FIXED_NTVDMSTATE_LINEAR EQU     ((FIXED_NTVDMSTATE_SEGMENT  SHL  4 ) + FIXED_NTVDMSTATE_OFFSET)\n")
    genNam(FIXED_NTVDMSTATE_SIZE)

    genCom("VdmFlags")

    genNam(VDM_INT_HARDWARE)
    genNam(VDM_INT_TIMER)

    genNam(VDM_INTERRUPT_PENDING)
    genVal(VDM_INTS_HOOKED_IN_PM, 0x4)

    genNam(VDM_BREAK_EXCEPTIONS)
    genNam(VDM_BREAK_DEBUGGER)

    genNam(VDM_PROFILE)
    genNam(VDM_ANALYZE_PROFILE)
    genNam(VDM_TRACE_HISTORY)
    genNam(VDM_32BIT_APP)

    genVal(VDM_VIRTUAL_INTERRUPTS, EFLAGS_IF_MASK)
    genVal(VDM_VIRTUAL_AC, EFLAGS_AC_MASK)
    genVal(VDM_VIRTUAL_NT, EFLAGS_NT_MASK)
    genVal(MIPS_BIT_MASK, VDM_ON_MIPS)

    genNam(VDM_ON_MIPS)
    genNam(VDM_EXEC)
    genNam(VDM_RM)
    genNam(VDM_USE_DBG_VDMEVENT)

    genNam(VDM_WOWBLOCKED)
    genNam(VDM_IDLEACTIVITY)

    genNam(VDM_WOWHUNGAPP)
    genNam(VDM_PE_MASK)

    genCom("Interrupt handler flags")

    genNam(VDM_INT_INT_GATE)
    genNam(VDM_INT_TRAP_GATE)
    genNam(VDM_INT_32)
    genNam(VDM_INT_16)
    genNam(VDM_INT_HOOKED)

    genCom("EFlags values")

    genNam(EFLAGS_TF_MASK)
    genVal(EFLAGS_INTERRUPT_MASK, EFLAGS_IF_MASK)
    genVal(EFLAGS_IOPL_MASK, EFLAGS_PL_MASK)
    genNam(EFLAGS_NT_MASK)

    genCom("Selector Flags")

    genVal(SEL_TYPE_READ, 0x00000001)
    genVal(SEL_TYPE_WRITE, 0x00000002)
    genVal(SEL_TYPE_EXECUTE, 0x00000004)
    genVal(SEL_TYPE_BIG, 0x00000008)
    genVal(SEL_TYPE_ED, 0x00000010)
    genVal(SEL_TYPE_2GIG, 0x00000020)

    genCom("VdmEvent Enumerations")

    genNam(VdmIO)
    genNam(VdmStringIO)
    genNam(VdmMemAccess)
    genNam(VdmIntAck)
    genNam(VdmBop)
    genNam(VdmError)
    genNam(VdmIrq13)
    genNam(VdmMaxEvent)

    genCom("VdmTib offsets")

    genDef(Vt, VDM_TIB,MonitorContext)
    genDef(Vt, VDM_TIB,VdmContext)
    genAlt(VtInterruptHandlers, VDM_TIB,VdmInterruptHandlers)
    genAlt(VtFaultHandlers, VDM_TIB,VdmFaultHandlers)
    genDef(Vt, VDM_TIB,EventInfo)

    genVal(VtEIEvent,
           OFFSET(VDM_TIB,EventInfo) + OFFSET(VDMEVENTINFO,Event))

    genVal(VtEIInstSize,
           OFFSET(VDM_TIB,EventInfo) + OFFSET(VDMEVENTINFO,InstructionSize))

    genVal(VtEIBopNumber,
           OFFSET(VDM_TIB,EventInfo) + OFFSET(VDMEVENTINFO, BopNumber))

    genVal(VtEIIntAckInfo,
           OFFSET(VDM_TIB,EventInfo) + OFFSET(VDMEVENTINFO, IntAckInfo))

    genDef(Vt, VDM_TIB,DpmiInfo)
    genDef(Ei, VDMEVENTINFO,Event)
    genDef(Ei, VDMEVENTINFO,InstructionSize)
    genDef(Ei, VDMEVENTINFO,BopNumber)
    genDef(Ei, VDMEVENTINFO,IntAckInfo)

    genCom("WOW TD offsets")
    genVal(WtdFastZWowEsp, 0x8)

    genCom("VdmInterrupHandler offsets")

    genDef(Vi, VDM_INTERRUPTHANDLER,CsSelector)
    genDef(Vi, VDM_INTERRUPTHANDLER,Eip)
    genDef(Vi, VDM_INTERRUPTHANDLER,Flags)
    genVal(VDM_INTERRUPT_HANDLER_SIZE, sizeof(VDM_INTERRUPTHANDLER))

    genCom("VdmFaultHandler offsets")

    genDef(Vf, VDM_FAULTHANDLER,CsSelector)
    genDef(Vf, VDM_FAULTHANDLER,Eip)
    genDef(Vf, VDM_FAULTHANDLER,SsSelector)
    genDef(Vf, VDM_FAULTHANDLER,Esp)
    genDef(Vf, VDM_FAULTHANDLER,Flags)
    genVal(VDM_FAULT_HANDLER_SIZE, sizeof(VDM_FAULTHANDLER))

    genCom("VdmDpmiInfo offsets")

    genDef(Vp, VDM_DPMIINFO,LockCount)
    genDef(Vp, VDM_DPMIINFO,Flags)
    genDef(Vp, VDM_DPMIINFO,SsSelector)
    genDef(Vp, VDM_DPMIINFO,SaveSsSelector)
    genDef(Vp, VDM_DPMIINFO,SaveEsp)
    genDef(Vp, VDM_DPMIINFO,SaveEip)
    genDef(Vp, VDM_DPMIINFO,DosxIntIret)
    genDef(Vp, VDM_DPMIINFO,DosxIntIretD)
    genDef(Vp, VDM_DPMIINFO,DosxFaultIret)
    genDef(Vp, VDM_DPMIINFO,DosxFaultIretD)
    genDef(Vp, VDM_DPMIINFO,DosxRmReflector)

    genCom("VdmTrace codes")

    genNam(VDMTR_KERNEL_OP_PM)
    genNam(VDMTR_KERNEL_OP_V86)
    genNam(VDMTR_KERNEL_HW_INT)

    genCom("Misc defines")

    genVal(DBG_SINGLESTEP, 0x5)
    genVal(DBG_BREAK, 0x6)
    genVal(DBG_GPFAULT, 0x7)
    genVal(DBG_STACKFAULT, 0x10)
    genVal(STATUS_VDM_EVENT, 0x40000005)

    END_LIST
};
