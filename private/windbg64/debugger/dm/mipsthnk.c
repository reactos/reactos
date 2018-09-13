#include "precomp.h"
#pragma hdrstop

#define OPCODE(x) ((WORD)((x & 0xFFFF0000) >>16))
#define ADDRESS(x) ((WORD)(x & 0x0000FFFF))

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
    // MIPS direct (ilink) thunk looks like this:
    //  0x03e04025          or      t0,ra,zero
    //  0x04110001          bgezal  zero,0xc
    //  0x00000000          nop
    //  0x8fe90014          lw      t1,20(ra)
    //  0x013f4821          addu    t1,t1,ra
    //  0x25290018          addiu   t1,t1,24
    //  0x01200008          jr      t1
    //  0x0100f825          or      ra,t0,zero
    //  0xdeadbeef          ld      t5,-16657(s5)

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
    // MIPS indirect (dll import) thunk either looks like this:
    // R4k
    //  0x00, 0x00, 0x08, 0x3C,               // lui $8,IAT
    //  0x00, 0x00, 0x08, 0x8D,               // lw  $8,IAT($8)
    //  0x08, 0x00, 0x00, 0x01,               // jr $8
    //  0x00, 0x00, 0x00, 0x00,               // nop (delay slot)
    //
    // or like this:
    // R3k
    //  0x00, 0x00, 0x08, 0x3C,               // lui $8,IAT
    //  0x00, 0x00, 0x08, 0x8D,               // lw  $8,IAT($8)
    //  0x00, 0x00, 0x00, 0x00,               // nop (Required for R3000)
    //  0x08, 0x00, 0x00, 0x01,               // jr $8
    //  0x00, 0x00, 0x00, 0x00,               // nop (delay slot)

    if (cbBuff >= 16) {
        DWORD *Inst = (DWORD *)rgbBuffer;
        DWORD ThunkSize = 0;
        if ((OPCODE(*(Inst+0)) == 0x3C08) &&
            (OPCODE(*(Inst+1)) == 0x8D08) &&
            (*(Inst+2) == 0x01000008) &&
            (*(Inst+3) == 0x00000000))
        {
            // We've got a R4K thunk.
            ThunkSize = 16;
        } else {
            if (cbBuff >= 20) {
                if ((OPCODE(*(Inst+0)) == 0x3C08) &&
                    (OPCODE(*(Inst+1)) == 0x8D08) &&
                    (*(Inst+2) == 0x00000000) &&
                    (*(Inst+3) == 0x01000008) &&
                    (*(Inst+4) == 0x00000000))
                {
                    // We've got a R3K thunk.
                    ThunkSize = 20;
                }
            }
        }

        if (ThunkSize) {
            DWORD Address;
            Address = ADDRESS(*(Inst+0)) << 16 + ADDRESS(*(Inst+1));
            if ( DbgReadMemory (
                    hthd->hprc,
                    (LPCVOID)Address,
                    lpuoffThunkDest,
                    sizeof(UOFFSET),
                    NULL)
            ) {
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
/*
   !!! HACK ALERT !!!
   This is a quick fix for V2
   V3 should do the following or some variant
   1) SAPI should return a non-null hproc when sitting at an import thunk. windbg's SH does this
   but dolphin returns null.
   2) x86 and mips backend should mark adjustor thunks as S_THUNK32 rather than S_GPROC32 and S_GPROCMIPS
   3) Then we could return CANSTEP_THUNK to DM and it would handle this nasty pseudo-emulation
   4) Dolphin EE does some thunk manipulation to print out function pointers symbolicly -- Perhaps use
   a thunkhlp.lib to deal with all this in one place
 */

class REGISTERS
{
   UOFFSET m_regs[32];
   BOOL m_fValid[32];

       public:
       REGISTERS ()
   {
      int i;
          memset (m_regs, 0, sizeof (m_regs));
      for (i = 0; i < sizeof (m_fValid) / sizeof (m_fValid[0]); i++)
      {
         m_fValid[i] = FALSE;
      }
   }

   UOFFSET & operator[] (int i)
   {
      if (!m_fValid[i])
      {                 // initialize on first access.

         SHREG shreg;

         shreg.hReg = (unsigned short) (CV_M4_IntZERO + i);
         DHGetReg (&shreg, NULL);
         m_regs[i] = shreg.Byte4;
         m_fValid[i] = TRUE;
      }
      return (m_regs[i]);
   }
};

#include <mipsinst.h>         // for PMIPS_INSTRUCTION
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * ThunkTarget
 * Input
 * hpid,htid
 * lpThunkAddr: Address of thunk
 * Output
 * BOOL: True if address was thunk and target was updated
 *  target: Destination of thunk
 * In/Out:
 *  regs: registers used by thunk [updated -- in case the next thunk uses them]
 *
 * Factor out common code for dbcCanStep and dbcExitedFunction.
 *--------------------------------------------------------------------------------
 */
static
BOOL
ThunkTarget (
    HPID hpid,
    HTID htid,
    LPADDR lpThunkAddr,
    DWORD& target,
    REGISTERS& regs
    )
{
   const int MIPS_THUNK_SIZE = 10; // Largest thunk I can think of
   DWORD cbRead;
   MIPS_INSTRUCTION buffer[MIPS_THUNK_SIZE];
   PMIPS_INSTRUCTION pInst = buffer;
   PMIPS_INSTRUCTION pInstMax;
   UOFFSET source;
   BOOL isThunk = FALSE;
   source = GetAddrOff (*lpThunkAddr);

   cbRead = DHGetDebuggeeBytes (*lpThunkAddr, sizeof (buffer), buffer);
   if (cbRead < sizeof(buffer)) {
      return(FALSE);
   }
   pInstMax = pInst + (cbRead / sizeof (buffer[0]));

   for (; !isThunk && (pInst < pInstMax); pInst++)
   {
      switch (pInst->j_format.Opcode)
      {
      case J_OP:
         target = (source & 0xf0000000) | (pInst->j_format.Target << 2);
         isThunk = TRUE;
         break;

      case LUI_OP:
         regs[pInst->i_format.Rt] = (pInst->i_format.Simmediate << 16);
         break;

      case LW_OP:
         {
            if (pInst->i_format.Rs == 29)
            {           // SP

               pInst = pInstMax; // leave loop

               break;
            }

            UOFFSET uoff = regs[pInst->i_format.Rs];

            uoff += pInst->i_format.Simmediate;
            ADDR addr2 = *lpThunkAddr;

            SE_SetAddrOff (&addr2, uoff);
            if (DHGetDebuggeeBytes (addr2, sizeof (regs[0]), &regs[pInst->i_format.Rt]) == 0)
            {
               ASSERT (FALSE);
            }
         }
         break;

      case ADDI_OP:
      case ADDIU_OP:
         regs[pInst->i_format.Rt] = regs[pInst->i_format.Rs] + pInst->i_format.Simmediate;
         break;

      case SPEC_OP:
         switch (pInst->r_format.Function)
         {
         case JR_OP:
            if (pInst->r_format.Rs != 31)
            {
               target = regs[pInst->r_format.Rs];
               isThunk = TRUE;
            }
            pInst = pInstMax; // leave loop

            break;

         case OR_OP:
            regs[pInst->r_format.Rd] = regs[pInst->r_format.Rs] | regs[pInst->r_format.Rt];
            break;

         case ADD_OP:
         case ADDU_OP:
            regs[pInst->r_format.Rd] = regs[pInst->r_format.Rs] + regs[pInst->r_format.Rt];
            break;

         case SLL_OP:
            if (pInst->Long == 0 || pInst->Long == 0x21)
            {           // nop?

               break;
            }
            // fall-through

         default:
            pInst = pInstMax; // leave loop

            break;
         }
         break;

      case BCOND_OP:
         switch (pInst->u_format.Rt)
         {
         case BGEZAL_OP:
            if (pInst->r_format.Rs == 0)
            {
               regs[31] = source + (pInst - buffer + 2) * sizeof (MIPS_INSTRUCTION);
               pInst += pInst->i_format.Simmediate;
            }
            else
            {
               pInst = pInstMax;
            }
            break;

         default:
            pInst = pInstMax;
            break;
         }
         break;

      default:
         pInst = pInstMax; // leave loop

         break;
      }
    }
   return(isThunk);
}

/****************************************************************************
 SHThunkStep

 Purpose  : disassemble/emulate mips thunks to find final destination and test
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
   ADDR destAddr;
   BOOL fStep = FALSE;

   if (lpDestAddr == NULL)
   {
      destAddr = *lpSrcAddr;
      lpDestAddr = &destAddr;
   }
   if (ADDR_IS_LI (*lpDestAddr))
   {
      SYFixupAddr (lpDestAddr);
   }
   if (ADDR_IS_LI (*lpSrcAddr))
   {
      SYFixupAddr (lpSrcAddr);
   }

   CanStep->Flags = CANSTEP_NO;
   int ThunkLimit = 8;
   while (ThunkTarget(hpid, htid, lpDestAddr, target, regs) && --ThunkLimit)
   {
      ASSERT(target != 0);
      ADDR addrTarget = {0};

      ADDR_IS_OFF32 (addrTarget) = TRUE;
      ADDR_IS_FLAT (addrTarget) = TRUE;
      SE_SetAddrOff (&addrTarget, target);
       fStep = SYFHasSource (hpid, &addrTarget);
      CanStep->Flags = fStep ? CANSTEP_YES : CANSTEP_NO;
      if (CanStep->Flags == CANSTEP_YES)
      {
         UOFF32 uoff;
         CXF Cxf = {0};
         ADDR_IS_OFF32 (addrTarget) = TRUE;
         ADDR_IS_FLAT (addrTarget) = TRUE;
         SE_SetAddrOff (&addrTarget, target);
         SYUnFixupAddr (&addrTarget);
         SHSetCxt (&addrTarget, SHpCXTFrompCXF (&Cxf));
         if (SHIsInProlog (&Cxf.cxt))
         {
            uoff = SHGetDebugStart (SHHPROCFrompCXT (SHpCXTFrompCXF (&Cxf)));
            if (uoff > Cxf.cxt.addr.addr.off)
            {
               SE_SetAddrOff (&addrTarget, uoff);
               SYFixupAddr (&addrTarget);
               target = GetAddrOff (addrTarget);
            }
         }
         CanStep->PrologOffset = target - GetAddrOff (*lpSrcAddr);
         break;
      }
      else
      {
         // check for thunk to thunk by continuing
         target = 0;
         *lpDestAddr = addrTarget;
      }
   }
   return fStep;
}


#endif
