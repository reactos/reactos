#include "precomp.h"
#pragma hdrstop

#include "alpdis.h"


BOOL
FIsDirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    // Alpha direct (ilink) thunk looks like this:
    //  0xc0000000          br    $0,  zero Pick up PC value
    //  0x20600014          lda   20($3), zero
    //  0xa0a3fffc          ldl   $5, -4($3)
    //  0x40a30003          addl  $5, $3, $3
    //  0x68030000          jmp   zero, $3
    //  0x00000000          halt (maintain 16 byte align and puke if execute)

    return FALSE;
}


BOOL
FIsIndirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    // Alpha indirect (Dll Import) thunk looks like this:
    //  0x277f0000          ldah t12, IAT(zero) // t12=r27
    //  0xa37b0000          ldl  t12, IAT(pv)
    //  0x6bfb0000          jmp  $31, (t12)
    //
    // Alpha Long BSR thunk looks like this:
    //  0x279f0000,         ldah at, hi_addr(zero)
    //  0x239c0000,         lda  at, lo_addr(at)
    //  0x6bfc0000,         jmp  $31, (at)
    //  0x00000000          halt (maintain 16 byte align and puke if execute)


    if (cbBuff >= 16) {
        DWORD *Inst = (DWORD *)rgbBuffer;
        DWORD ThunkSize = 0;

        if ((OPCODE(*(Inst+0)) == 0x277f) &&
            (OPCODE(*(Inst+1)) == 0xa37b) &&
            (*(Inst+2) == 0x6bfb0000))
        {
            /* DLL Import case */
            ThunkSize = 12;
        }
        else
        if ((OPCODE(*(Inst+0)) == 0x279f) &&
            (OPCODE(*(Inst+1)) == 0xa39c) &&
            (*(Inst+2) == 0x6bfc0000) &&
            (*(Inst+3) == 0x00000000))
        {
            /* Long BSR Case */
            ThunkSize = 16;
        }

        if (ThunkSize) {

            DWORD64 Address;
            Address = (MEM_DISP(*(Inst+0)) << 16) + MEM_DISP(*(Inst+1));
            if ( DbgReadMemory (
                    hthd->hprc,
                    Address,
                    lpuoffThunkDest,
                    sizeof(UOFFSET),
                    NULL) )
            {
                *lpdwThunkSize = ThunkSize;
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL
FIsVCallThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    return FALSE;
}


BOOL
FIsVTDispAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    return FALSE;
}


BOOL
FIsAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    return FALSE;
}

BOOL
GetPMFDest(
    HTHDX hthd,
    UOFFSET uThis,
    UOFFSET uPMF,
    UOFFSET *lpuOffDest
    )
{
    return FALSE;
}

#if 0
class REGISTERS
{
    UOFFSET m_regs[32];
    BOOL m_fValid[32];

    public:
        REGISTERS ()
        {
            int i;
            memset (m_regs, 0, sizeof (m_regs));
            for (i = 0; i < sizeof (m_fValid) / sizeof (m_fValid[0]); i++) {
                m_fValid[i] = FALSE;
            }
        }

        UOFFSET & operator[] (int i)
        {
            if (!m_fValid[i]) {

                // initialize on first access.

                SHREG shreg;

                shreg.hReg = (unsigned short) (CV_ALPHA_IntV0 + i);
                DHGetReg (&shreg, NULL);
                m_regs[i] = shreg.Byte4;
                m_fValid[i] = TRUE;
            }
            return (m_regs[i]);
        }
};

#include <alphaops.h>         // for PALPHA_INSTRUCTION
/****************************************************************************
 * ThunkTarget
 *
 * Input:
 *    hpid, htid
 *    lpThunkAddr: Address of Thunk
 * Output:
 *    BOOL: true if address was thunk and target was updated
 *    target: Destination of Thunk
 * In/Out
 *    regs: registers used by thunk [updated in case next thunk uses them]
 *
 * Factor out common code for dbcCanStep and dbcExitedFunction.
 ****************************************************************************/
static
BOOL
ThunkTarget(
    HPID hpid,
    HTID htid,
    LPADDR lpThunkAddr,
    UOFFSET &target,
    REGISTERS &regs
    )
{
    const int ALPHA_THUNK_SIZE = 10;  // Largest thunk I can think of
    DWORD cbRead;
    ALPHA_INSTRUCTION buffer[ALPHA_THUNK_SIZE];
    PALPHA_INSTRUCTION pInst = buffer;
    PALPHA_INSTRUCTION pInstMax;
    UOFFSET source;
    BOOL isThunk = FALSE;
    source = GetAddrOff (*lpThunkAddr);

    cbRead = DHGetDebuggeeBytes (*lpThunkAddr, sizeof (buffer), buffer);
    if(cbRead < sizeof(buffer)) {
        return FALSE;
    }
    pInstMax = pInst + (cbRead / sizeof (buffer[0]));

    for (; !isThunk && (pInst < pInstMax); pInst++)
    {
        switch (pInst->Jump.Opcode)
        {
            case JMP_OP:
                target = regs[pInst->Jump.Rb];
                if (sizeof(DWORD) == sizeof(regs[pInst->Jump.Rb])) {
                    target = SE32To64(target);
                }
                isThunk = TRUE;
                break;

            case LDAH_OP:
                regs[pInst->Memory.Ra] = (pInst->Memory.MemDisp << 16) + regs[pInst->Memory.Rb];
                break;

            case LDA_OP:
                regs[pInst->Memory.Ra] = (pInst->Memory.MemDisp) + regs[pInst->Memory.Rb];
                break;

            case LDL_OP:
            {
                UOFFSET uoff = regs[pInst->Memory.Rb];
                if (sizeof(DWORD) == sizeof(regs[pInst->Memory.Rb])) {
                    uoff = SE32To64(uoff);
                }

                uoff += pInst->Memory.MemDisp;
                ADDR addr2 = *lpThunkAddr;

                SE_SetAddrOff (&addr2, uoff);
                if (DHGetDebuggeeBytes (addr2, sizeof (regs[0]), &regs[pInst->Memory.Ra]) == 0)
                {
                    ASSERT (FALSE);
                }
            }
                break;

            case BR_OP:
// pStart not defined: ASSERT(pInst == pStart);
                regs[pInst->Branch.Ra] = source+4;
                pInst=pInst + (4*(pInst->Branch.BranchDisp));
                break;

            case ARITH_OP:
                switch(pInst->OpReg.Function)
                {
                    case ADDL_FUNC:
                        regs[pInst->OpReg.Rc] = (regs[pInst->OpReg.Ra]+regs[pInst->OpReg.Rb]);
                        break;
                    default:
                        ASSERT(FALSE);
                        break;
                }
                break;

            default:
                pInst = pInstMax; // leave loop

                break;
        }
    }
    return (isThunk);
}

/****************************************************************************
SHThunkStep

Purpose  : disassemble/emulate ALPHA thunks to find final destination and test
for source code availability.

Arguments:
hpid,htid - current process/thread
srcAddr   - original thunk address for the sake of prolog fudging
CanStep   - returned to DM to indicate how to continue
destAddr  - current thunk address for disassembly/emulation

Returns  : TRUE if a destination with source code is found
****************************************************************************/
static
BOOL
SHThunkStep (
    HPID hpid,
    HTID htid,
    LPADDR lpSrcAddr,
    CANSTEP * CanStep,
    LPADDR lpDestAddr
    )
{
    REGISTERS regs;
    UOFFSET target = 0;
    BOOL fStep = FALSE;

    if (lpDestAddr == NULL) {
        lpDestAddr = lpSrcAddr;
    }

    if (ADDR_IS_LI (*lpDestAddr)) {
        SYFixupAddr (lpDestAddr);
    }

    CanStep->Flags = CANSTEP_NO;
    int ThunkLimit = 8;
    while (ThunkTarget(hpid, htid, lpDestAddr, target, regs) && --ThunkLimit) {
        ASSERT(target != 0);

        ADDR addrTarget = {0};

        ADDR_IS_OFF32 (addrTarget) = TRUE;
        ADDR_IS_FLAT (addrTarget) = TRUE;
        SE_SetAddrOff (&addrTarget, target);
        fStep = SYFHasSource(hpid, &addrTarget);
        CanStep->Flags = fStep ? CANSTEP_YES : CANSTEP_NO;
        if (CanStep->Flags == CANSTEP_YES) {
            UOFFSET uoff;
            CXF Cxf = {0};

            ADDR_IS_OFF32 (addrTarget) = TRUE;
            ADDR_IS_FLAT (addrTarget) = TRUE;
            SE_SetAddrOff(&addrTarget, target);
            SYUnFixupAddr(&addrTarget);
            SHSetCxt(&addrTarget, SHpCXTFrompCXF (&Cxf));
            if (SHIsInProlog (&Cxf.cxt)) {
                uoff = SHGetDebugStart (SHHPROCFrompCXT (SHpCXTFrompCXF (&Cxf)));

                if(uoff > Cxf.cxt.addr.addr.off) {
                    SE_SetAddrOff(&addrTarget, uoff);
                    SYFixupAddr(&addrTarget);
                    target = GetAddrOff(addrTarget);
                }
            }
            CanStep->PrologOffset = target - GetAddrOff (*lpSrcAddr);
            break;

        } else {

            // check for thunk to thunk by continuing
            target = 0;
            *lpDestAddr = addrTarget;
        }
    }
    return fStep;
}

#endif
