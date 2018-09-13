/**** DMDISASM.C - EM Lego disassembler interface                          *
 *                                                                         *
 *                                                                         *
 *  Copyright <C> 1995, Microsoft Corp                                     *
 *                                                                         *
 *  Created: January 1, 1996 by Kent Forschmiedt
 *                                                                         *
 *  Revision History:                                                      *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <simpldis.h>


#define MAXL     20
#define CCHMAX   256

#if defined(TARGET_i386)
#define SIMPLE_ARCH_CURRENT Simple_Arch_X86
#elif defined(TARGET_ALPHA) || defined(TARGET_AXP64)
#define SIMPLE_ARCH_CURRENT Simple_Arch_AlphaAxp
#elif defined(TARGET_IA64)
#define SIMPLE_ARCH_CURRENT Simple_Arch_IA64
#else
#error "Undefined processor"
#endif


int
CvRegFromSimpleReg(
    MPT     mpt,
    int     regInstr
    )
{
    switch (mpt) {
    case mptix86:
        switch(regInstr) {
            case SimpleRegEax: return CV_REG_EAX;
            case SimpleRegEcx: return CV_REG_ECX;
            case SimpleRegEdx: return CV_REG_EDX;
            case SimpleRegEbx: return CV_REG_EBX;
            case SimpleRegEsp: return CV_REG_ESP;
            case SimpleRegEbp: return CV_REG_EBP;
            case SimpleRegEsi: return CV_REG_ESI;
            case SimpleRegEdi: return CV_REG_EDI;
        }
        break;

    case mptdaxp:
        return (regInstr + CV_ALPHA_IntV0);

    case mptia64:
        assert(!"Will need code for IA64"); //v-vadimp when msdis is ready

    }
    return (0);
}



#if !defined(TARGET_IA64) //v-vadimp we are so far using falcon
ULONGLONG
QwGetreg(
    PVOID   pv,
    int     regInstr
    )
{
    HTHDX       hthdx = (HTHDX)pv;
    XOSD        xosd;
    ULONGLONG   retVal;

    retVal = GetRegValue(&hthdx->context,
                         CvRegFromSimpleReg(MPT_CURRENT, regInstr)
                         );

    return retVal;
}

XOSD
disasm (
    HTHDX  hthd,
    LPSDI  lpsdi,
    PVOID  Memory,
    int    Size
    )
{
    XOSD        xosd      = xosdNone;
    int         cbUsed;
    int         Bytes;
    SIMPLEDIS   Sdis;

    Bytes = SimplyDisassemble(
        Memory,                    // code ptr
        Size,                      // bytes
//BUGBUG 64 bits
        (ULONG)GetAddrOff(lpsdi->addr),
        SIMPLE_ARCH_CURRENT,
        &Sdis,
        NULL,
        NULL,
        NULL,
        QwGetreg,
        (PVOID)hthd
        );

    if (Bytes < 0) {
        cbUsed = -Bytes;
        xosd = xosdGeneral;
    } else {
        cbUsed = Bytes;
    }

    return xosd;
}

DWORD
BranchUnassemble(
    HTHDX   hthd,
    PVOID   Memory,
    DWORD   Size,
    LPADDR  Addr,
    BOOL   *IsBranch,
    BOOL   *TargetKnown,
    BOOL   *IsCall,
    BOOL   *IsTable,
    LPADDR  Target
    )
{
    int         cbUsed;
    int         Bytes;
    SIMPLEDIS   Sdis;
    DWORD64     dwTarget = 0;


    Bytes = SimplyDisassemble(
        Memory,                    // code ptr
        Size,                      // bytes
        (ULONG)GetAddrOff(*Addr),
        SIMPLE_ARCH_CURRENT,
        &Sdis,
        NULL,
        NULL,
        NULL,
        QwGetreg,
        (PVOID)hthd
        );

    if (Bytes < 0) {
        *IsBranch = FALSE;
        *IsTable = FALSE;
        *IsCall = FALSE;
        *TargetKnown = FALSE;
        return 0;
    }

    *IsBranch = Sdis.IsBranch;
    *IsCall = Sdis.IsCall;
    *IsTable = FALSE;

    if (*IsBranch) {
        //
        // when do we know the branch target, and when do we not?
        //
        if (Sdis.dwJumpTable && !Sdis.dwBranchTarget) {
            *IsTable = TRUE;
            dwTarget = SE32To64( Sdis.dwJumpTable );
        } else {
            *IsTable = FALSE;
            dwTarget = SE32To64( Sdis.dwBranchTarget );
        }
    }

    if (*IsCall) {
        *IsCall = TRUE;
        dwTarget = SE32To64( Sdis.dwBranchTarget );
    }

    *TargetKnown = !!dwTarget;

    if (*TargetKnown) {
        AddrInit( Target, 
                  0, 
                  0, 
                  dwTarget, 
                  TRUE, 
                  TRUE, 
                  FALSE, 
                  FALSE 
                  );
    }

    return Bytes;

}
#endif

ULONG64
GetNextOffset (
    HTHDX hthd,
    BOOL fStep
    )

/*++

Routine Description:


    From a limited disassembly of the instruction pointed
    by the FIR register, compute the offset of the next
    instruction for either a trace or step operation.

        trace -> the next instruction to execute
        step -> the instruction in the next memory location or the
                next instruction executed due to a branch (step
                over subroutine calls).

Arguments:

    hthd  - Supplies the handle to the thread to get the next offset for

    fStep - Supplies TRUE for STEP offset and FALSE for trace offset

Returns:
    step or trace offset if input is TRUE or FALSE, respectively

Version Information:
    This copy of GetNextOffset is from ntsd\alpha\ntdis.c@v16 (11/14/92)

--*/

{
    int         Bytes;
    PUCHAR      Memory[MAX_INSTRUCTION_SIZE];
    DWORD       Size;
    BOOL        r;
    ADDR        TargetAddr;
    ADDR        IpAddr;
    BOOL        IsBranch;
    BOOL        TargetKnown;
    BOOL        IsCall;
    BOOL        IsTable;
    DWORD64     NewOffset;

#if defined (TARGET_ALPHA) || defined (TARGET_AXP64)
ULONG64
AlpGetNextOffset (
    HTHDX hthd,
    BOOL fStep
    );

    DWORD64 AnswerX = AlpGetNextOffset(hthd, fStep);
    return AnswerX;
#endif

#ifdef TARGET_IA64
ULONG64
IA64GetNextOffset (
    HTHDX hthd,
    BOOL fStep
    );

    DWORD64 AnswerX = IA64GetNextOffset(hthd, fStep);
    return AnswerX;
#endif

    AddrFromHthdx(&IpAddr, hthd);

    //
    // relative addressing updates PC first
    // Assume next sequential instruction is next offset
    //


    r = AddrReadMemory(hthd->hprc,
                       hthd,
                       &IpAddr,
                       Memory,
                       MAX_INSTRUCTION_SIZE,
                       &Size );

    if (!r) {
        DPRINT(1, ("GetNextOffset: failed AddrReadMemory %I64x\n", GetAddrOff(IpAddr)));
        assert(FALSE);
        return 4;
    }

    Bytes = BranchUnassemble( hthd,
                              Memory,
                              Size,
                              &IpAddr,
                              &IsBranch,
                              &TargetKnown,
                              &IsCall,
                              &IsTable,
                              &TargetAddr
                              );

    NewOffset = IpAddr.addr.off + Bytes;

    //
    // use the target address if it is a trace, or if it is a step and
    // the instruction is not a call.
    //

    if (!fStep && IsCall && TargetKnown) {
        NewOffset = GetAddrOff(TargetAddr);
    }

    if (IsBranch && TargetKnown) {
        NewOffset = GetAddrOff(TargetAddr);
    }

    return NewOffset;
}
