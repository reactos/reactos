/*++

Copyright (c) 2000-2001 Goran Devic                                                   
Modified (c) 2001 Klaus P. Gerlicher

Module Name:

    disassembler.c

Abstract:

    line disassembler

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Goran Devic                                                   

Revision History:

  17-Mar-2000:  Original                                        (Goran Devic)                                             
  26-Apr-2000:  Major rewrite, added coprocessor instructions   (Goran Devic)
  04-Nov-2000:  Modified for LinIce                             (Goran Devic)                                    
  05-Jan-2001:  Modified for pICE                               (Klaus P. Gerlicher)


Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

/*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include "remods.h"
#include "precomp.h"

#include "disassemblerdata.h"           // Include its own data

/******************************************************************************
*
*   This structure is used to pass parameters and options to the
*   line disassembler.
*
******************************************************************************/
typedef struct
{
    ULONG dwFlags;              // Generic flags (described below)
    USHORT wSel;                  // Selector to use to fetch code
    UCHAR *bpTarget;             // Target pointer to disassemble
    UCHAR *szDisasm;             // String where to put ascii result
    UCHAR Codes[20];             // Buffer where to store code UCHARs

    UCHAR bAsciiLen;             // Length of the ascii result
    UCHAR bInstrLen;             // Instruction lenght in UCHARs

    int nDisplacement;          // Scanner: possible constant displacement
    int nScanEnum;              // Scanner: specific flags SCAN_*

} TDisassembler;

// dwFlags contains a set of boolean flags with the following functionality

#define DIS_DATA32          0x0001  // Data size 16/32 bits (0/1)
#define   DIS_GETDATASIZE(flags) ((flags)&DIS_DATA32)
#define DIS_ADDRESS32       0x0002  // Address size 16/32 bits (0/1)
#define   DIS_GETADDRSIZE(flags) (((flags)&DIS_ADDRESS32)?1:0)

#define DIS_SEGOVERRIDE     0x0004  // Default segment has been overriden

#define DIS_REP             0x0100  // Return: REP prefix found (followed by..)
#define DIS_REPNE           0x0200  // Return: REPNE prefix found
#define   DIS_GETREPENUM(flags)  (((flags)>>8)&3)
#define DIS_ILLEGALOP       0x8000  // Return: illegal opcode


/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/


/******************************************************************************
*                                                                             *
*   External functions (optional)                                             *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/
UCHAR GetUCHAR(ULONG addr)
{
    if(IsAddressValid(addr))
        return *(PUCHAR)addr;
    else
        return 0x82; // INVALID OPCODE
}

static UCHAR GetNextUCHAR(USHORT sel, UCHAR *offset, UCHAR *pCode)
{
    pCode[0] = GetUCHAR((ULONG) offset + 0) & 0xFF;

    return( pCode[0] );
}    

static USHORT GetNextUSHORT(USHORT sel, UCHAR *offset, UCHAR *pCode)
{
    pCode[0] = GetUCHAR((ULONG) offset + 0) & 0xFF;
    pCode[1] = GetUCHAR((ULONG) offset + 1) & 0xFF;

    return( *(USHORT *) pCode );
}    

static ULONG GetNextULONG(USHORT sel, UCHAR *offset, UCHAR *pCode)
{
    pCode[0] = GetUCHAR((ULONG) offset + 0) & 0xFF;
    pCode[1] = GetUCHAR((ULONG) offset + 1) & 0xFF;
    pCode[2] = GetUCHAR((ULONG) offset + 2) & 0xFF;
    pCode[3] = GetUCHAR((ULONG) offset + 3) & 0xFF;

    return( *(ULONG *) pCode );
}    


#define NEXTUCHAR    GetNextUCHAR( pDis->wSel, bpTarget, bpCode); bpCode += 1; bpTarget += 1; bInstrLen += 1

#define NEXTUSHORT    GetNextUSHORT( pDis->wSel, bpTarget, bpCode); bpCode += 2; bpTarget += 2; bInstrLen += 2

#define NEXTULONG   GetNextULONG(pDis->wSel, bpTarget, bpCode); bpCode += 4; bpTarget += 4; bInstrLen += 4


/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
*   UCHAR Disassembler( TDisassembler *pDis );                                 *
*                                                                             *
*******************************************************************************
*
*   This is a generic Intel line disassembler.
*
*   Where:
*       TDisassembler:
*           bpTarget is the address of instruction to disassemble
*           szDisasm is the address of the buffer to print a line into
*           dwFlags contains the default operand and address bits
*           pCode is the address to store code UCHARs (up to 16)
*
*   Disassembled instruction is stored as an ASCIIZ string pointed by
*   szDisasm pointer (from the pDis structure).
*
*   Returns:
*       TDisassembler:
*           *szDisasm contains the disassembled instruction string
*           bAsciiLen is set to the length of the printed string
*           bInstrLen is set to instruction length in UCHARs
*           dwFlags - has operand and address size flags adjusted
*                   - DIS_ILLEGALOP set if that was illegal instruction
*       UCHAR - instruction length in UCHARs
*
******************************************************************************/
UCHAR Disassembler( TDisassembler *pDis )
{
    TOpcodeData *p;             // Pointer to a current instruction record
    UCHAR   *bpTarget;           // Pointer to the target code to be disassembled
    UCHAR   *bpCode;             // Pointer to code UCHARs
    ULONG   arg;                // Argument counter
    char   *sPtr;               // Message selection pointer
    int     nPos;               // Printing position in the output string
    UCHAR   *pArg;               // Pointer to record where instruction arguments are
    ULONG   dwULONG;            // Temporary ULONG storage
    USHORT    wUSHORT;              // Temporary USHORT storage
    UCHAR    bUCHAR;              // Temporary UCHAR storage
    UCHAR    bInstrLen;          // Current instruction lenght in UCHARs
    UCHAR    bOpcode;            // Current opcode that is being disassembled
    UCHAR    bSegOverride;       // 0 default segment. >0, segment index
    UCHAR    bMod=0;             // Mod field of the instruction
    UCHAR    bReg=0;             // Register field of the instruction
    UCHAR    bRm=0;                // R/M field of the instruction
    UCHAR    bW;                 // Width bit for the register selection

    UCHAR    bSib;               // S-I-B UCHAR for the instruction
    UCHAR    bSs;                // SS field of the s-i-b UCHAR
    UCHAR    bIndex;             // Index field of the s-i-b UCHAR
    UCHAR    bBase;              // Base field of the s-i-b UCHAR
    LPSTR    pSymbolName;        // used to symbolic name of value

    bInstrLen = 0;              // Reset instruction lenght to zero
    bSegOverride = 0;           // Set default segment (no override)
    nPos = 0;                   // Reset printing position
    sPtr = NULL;                // Points to no message by default
    bpTarget = pDis->bpTarget;  // Set internal pointer to a target address
    bpCode = pDis->Codes;       // Set internal pointer to code UCHARs

    do
    {
        bOpcode = NEXTUCHAR;     // Get the first opcode UCHAR from the target address
        p = &Op1[bOpcode];      // Get the address of the instruction record

        if( p->flags & DIS_SPECIAL )
        {
            // Opcode is one of the special ones, so do what needs to be done there

            switch( p->name )
            {
                case _EscD8:
                case _EscD9:
                case _EscDA:
                case _EscDB:
                case _EscDC:
                case _EscDD:
                case _EscDE:
                case _EscDF:        // Coprocessor escape: UCHARs D8 - DF
                    bOpcode = NEXTUCHAR;             // Get the modRM UCHAR of the instruction

                    if( bOpcode < 0xC0 )
                    {
                        // Opcodes 00-BF use Coproc1 table

                        bReg = (bOpcode >> 3) & 7;
                        p = &Coproc1[ p->name - _EscD8 ][ bReg ];

                        goto StartInstructionParseMODRM;
                    }
                    // Opcodes C0-FF use Coproc2 table

                    p = &Coproc2[ p->name - _EscD8 ][ bOpcode - 0xC0 ];

                goto StartInstructionNoMODRM;

                case _S_ES:         // Segment override
                case _S_CS:
                case _S_SS:
                case _S_DS:
                case _S_FS:
                case _S_GS:
                    bSegOverride = p->name - _S_ES + 1;
                continue;

                case _OPSIZ:        // Operand size override - toggle
                    pDis->dwFlags ^= DIS_DATA32;
                continue;

                case _ADSIZ:        // Address size override - toggle
                    pDis->dwFlags ^= DIS_ADDRESS32;
                continue;

                case _REPNE:        // REPNE/REPNZ prefix
                    pDis->dwFlags |= DIS_REPNE;
                continue;

                case _REP:          // REP/REPE/REPZ prefix
                    pDis->dwFlags |= DIS_REP;
                continue;

                case _2BESC:        // 2 UCHAR escape code 0x0F
                    bOpcode = NEXTUCHAR;             // Get the second UCHAR of the instruction
                    p = &Op2[bOpcode];              // Get the address of the instruction record

                    if( !(p->flags & DIS_SPECIAL) ) goto StartInstruction;
                    if( p->name < _GRP6 ) goto IllegalOpcode;

                case _GRP1a:        // Additional groups of instructions
                case _GRP1b:
                case _GRP1c:
                case _GRP2a:
                case _GRP2b:
                case _GRP2c:
                case _GRP2d:
                case _GRP2e:
                case _GRP2f:
                case _GRP3a:
                case _GRP3b:
                case _GRP4:
                case _GRP5:
                case _GRP6:
                case _GRP7:
                case _GRP8:
                case _GRP9:

                    bOpcode = NEXTUCHAR;             // Get the Mod R/M UCHAR whose...
                                                    // bits 3,4,5 select instruction

                    bReg = (bOpcode >> 3) & 7;
                    p = &Groups[p->name - _GRP1a][ bReg ];

                    if( !(p->flags & DIS_SPECIAL) ) goto StartInstructionParseMODRM;

                case _NDEF :        // Not defined or illegal opcode
                    goto IllegalOpcode;

                default :;          // Should not happen
            }
        }
        else
            goto StartInstruction;
    }
    while( bInstrLen < 15 );

IllegalOpcode:

    nPos += PICE_sprintf( pDis->szDisasm+nPos, "invalid");
    pDis->dwFlags |= DIS_ILLEGALOP;

    goto DisEnd;

StartInstruction:

    // If this instruction needs additional Mod R/M UCHAR, fetch it

    if( p->flags & DIS_MODRM )
    {
        // Get the next UCHAR (modR/M bit field)
        bOpcode = NEXTUCHAR;

        bReg = (bOpcode >> 3) & 7;

StartInstructionParseMODRM:

        // Parse that UCHAR and get mod, reg and rm fields
        bMod = bOpcode >> 6;
        bRm  = bOpcode & 7;
    }

StartInstructionNoMODRM:

    // Print the possible repeat prefix followed by the instruction

    if( p->flags & DIS_COPROC )
        nPos += PICE_sprintf( pDis->szDisasm+nPos, "%-6s ", sCoprocNames[ p->name ]);
    else
        nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s%-6s ",
                sRep[DIS_GETREPENUM(pDis->dwFlags)],
                sNames[ p->name + (DIS_GETNAMEFLAG(p->flags) & DIS_GETDATASIZE(pDis->dwFlags)) ] );

    // Do instruction argument processing, up to 3 times

    pArg = &p->dest;

    for( arg=p->args; arg!=0; arg--, pArg++, arg? nPos += PICE_sprintf( pDis->szDisasm+nPos,", ") : 0 )
    {
        switch( *pArg )
        {
             case _Eb :                                         // modR/M used - bW = 0
                bW = 0;
                goto _E;

             case _Ev :                                         // modR/M used - bW = 1
                bW = 1;
                goto _E;

             case _Ew :                                         // always USHORT size
                pDis->dwFlags &= ~DIS_DATA32;
                bW = 1;
                goto _E;

             case _Ms :                                         // fword ptr (sgdt,sidt,lgdt,lidt)
                sPtr = sFwordPtr;
                goto _E1;

             case _Mq :                                         // qword ptr (cmpxchg8b)
                sPtr = sQwordPtr;
                goto _E1;

             case _Mp :                                         // 32 or 48 bit pointer (les,lds,lfs,lss,lgs)
             case _Ep :                                         // Always a memory pointer (call, jmp)
                if( pDis->dwFlags & DIS_DATA32 )
                    sPtr = sFwordPtr;
                else
                    sPtr = sDwordPtr;
                goto _E1;

             _E:
                 // Do registers first so that the rest may be done together
                 if( bMod == 3 )
                 {
                      // Registers depending on the w field and data size
                      nPos+=PICE_sprintf(pDis->szDisasm+nPos, "%s", sRegs1[DIS_GETDATASIZE(pDis->dwFlags)][bW][bRm] );

                      break;
                 }

                 if( bW==0 )
                     sPtr = sBytePtr;
                 else
                     if( pDis->dwFlags & DIS_DATA32 )
                         sPtr = sDwordPtr;
                     else
                         sPtr = sWordPtr;

             case _M  :                                         // Pure memory pointer (lea,invlpg,floats)
                if( bMod == 3 ) goto IllegalOpcode;

             _E1:

                 if( sPtr )
                     nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sPtr );

             case _Ma :                                         // Used by bound instruction, skip the pointer info

                 // Print the segment if it is overriden
                 //
                 nPos += PICE_sprintf( pDis->szDisasm+nPos,"%s", sSegOverride[ bSegOverride ] );

                 //
                 // Special case when sib UCHAR is present in 32 address encoding
                 //
                 if( (bRm==4) && (pDis->dwFlags & DIS_ADDRESS32) )
                 {
                      //
                      // Get the s-i-b UCHAR and parse it
                      //
                      bSib = NEXTUCHAR;

                      bSs = bSib >> 6;
                      bIndex = (bSib >> 3) & 7;
                      bBase = bSib & 7;

                      // Special case for base=5 && mod==0 -> fetch 32 bit offset
                      if( (bBase==5) && (bMod==0) )
                      {
                          dwULONG = NEXTULONG;
                          if(ScanExportsByAddress(&pSymbolName,dwULONG))
                          {
                              nPos += PICE_sprintf( pDis->szDisasm+nPos,"[%s", pSymbolName );
                          }
                          else
                          {
                              nPos += PICE_sprintf( pDis->szDisasm+nPos,"[%08X", (unsigned int) dwULONG );
                          }
                      }
                      else
                          nPos += PICE_sprintf( pDis->szDisasm+nPos,"[%s", sGenReg16_32[ 1 ][ bBase ] );

                      // Scaled index, no index if bIndex is 4
                      if( bIndex != 4 )
                          nPos += PICE_sprintf( pDis->szDisasm+nPos,"+%s%s", sScale[ bSs ], sGenReg16_32[ 1 ][ bIndex ] );
                      else
                          if(bSs != 0)
                              nPos += PICE_sprintf( pDis->szDisasm+nPos,"<INVALID MODE>" );

                      // Offset 8 bit or 32 bit
                      if( bMod == 1 )
                      {
                          bUCHAR = NEXTUCHAR;
                          if( (signed char)bUCHAR < 0 )
                                 nPos += PICE_sprintf( pDis->szDisasm+nPos,"-%02X", 0-(signed char)bUCHAR );
                          else
                                 nPos += PICE_sprintf( pDis->szDisasm+nPos,"+%02X", bUCHAR );
                      }

                      if( bMod == 2 )
                      {
                          dwULONG = NEXTULONG;
                          nPos += PICE_sprintf( pDis->szDisasm+nPos,"+%08X", (unsigned int) dwULONG );
                      }

                      // Wrap up the instruction
                      nPos += PICE_sprintf( pDis->szDisasm+nPos,"]" );
                      break;
                 }

                 //
                 // 16 or 32 address bit cases with mod zero, one or two
                 //
                 // Special cases when r/m is 5 and mod is 0, immediate d16 or d32
                 if( bMod==0 && ((bRm==6 && !(pDis->dwFlags & DIS_ADDRESS32)) || (bRm==5 && (pDis->dwFlags & DIS_ADDRESS32))) )
                 {
                      if( pDis->dwFlags & DIS_ADDRESS32 )
                      {
                          dwULONG = NEXTULONG;
                          if(ScanExportsByAddress(&pSymbolName,dwULONG))
                              nPos += PICE_sprintf( pDis->szDisasm+nPos,"[%s]", pSymbolName );
                          else
                              nPos += PICE_sprintf( pDis->szDisasm+nPos,"[%08X]", (unsigned int) dwULONG );
                      }
                      else
                      {
                          wUSHORT = NEXTUSHORT;
                          nPos += PICE_sprintf( pDis->szDisasm+nPos,"[%04X]", wUSHORT );
                      }

                      break;
                 }

                 // Print the start of the line
                 nPos += PICE_sprintf( pDis->szDisasm+nPos,"[%s", sAdr1[DIS_GETADDRSIZE(pDis->dwFlags)][ bRm ] );

                 // Offset (8 or 16) or (8 or 32) bit - 16, 32 bits are unsigned
                 if( bMod==1 )
                 {
                      bUCHAR = NEXTUCHAR;
                      if( (signed char)bUCHAR < 0 )
                             nPos += PICE_sprintf( pDis->szDisasm+nPos,"-%02X", 0-(signed char)bUCHAR );
                      else
                             nPos += PICE_sprintf( pDis->szDisasm+nPos,"+%02X", bUCHAR );
                 }

                 if( bMod==2 )
                 {
                      if( pDis->dwFlags & DIS_ADDRESS32 )
                      {
                          dwULONG = NEXTULONG;
                          nPos += PICE_sprintf( pDis->szDisasm+nPos,"+%08X", (unsigned int) dwULONG );
                      }
                      else
                      {
                          wUSHORT = NEXTUSHORT;
                          nPos += PICE_sprintf( pDis->szDisasm+nPos,"+%04X", wUSHORT );
                      }
                 }

                 // Wrap up the instruction
                 nPos += PICE_sprintf( pDis->szDisasm+nPos,"]" );

             break;

             case _Gb :                                         // general, UCHAR register
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sRegs1[0][0][ bReg ] );
             break;

             case _Gv :                                         // general, (d)USHORT register
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sGenReg16_32[DIS_GETDATASIZE(pDis->dwFlags)][ bReg ] );
             break;

             case _Yb :                                         // ES:(E)DI pointer
             case _Yv :
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s%s", sSegOverrideDefaultES[ bSegOverride ], sYptr[DIS_GETADDRSIZE(pDis->dwFlags)] );
             break;

             case _Xb :                                         // DS:(E)SI pointer
             case _Xv :
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s%s", sSegOverrideDefaultDS[ bSegOverride ], sXptr[DIS_GETADDRSIZE(pDis->dwFlags)] );
             break;

             case _Rd :                                         // general register double USHORT
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sGenReg16_32[ 1 ][ bRm ] );
             break;

             case _Rw :                                         // register USHORT
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sGenReg16_32[ 0 ][ bMod ] );
             break;

             case _Sw :                                         // segment register
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sSeg[ bReg ] );
             break;

             case _Cd :                                         // control register
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sControl[ bReg ] );
             break;

             case _Dd :                                         // debug register
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sDebug[ bReg ] );
             break;

             case _Td :                                         // test register
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sTest[ bReg ] );
             break;


             case _Jb :                                         // immediate UCHAR, relative offset
                 bUCHAR = NEXTUCHAR;
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "short %08X", (unsigned int)(pDis->bpTarget + (signed char)bUCHAR + bInstrLen) );
             break;

             case _Jv :                                         // immediate USHORT or ULONG, relative offset
                 if( pDis->dwFlags & DIS_DATA32 )
                 {
                      dwULONG = NEXTULONG;
                      if(ScanExportsByAddress(&pSymbolName,(unsigned int)(pDis->bpTarget + (signed long)dwULONG + bInstrLen)))
                        nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", pSymbolName );
                      else
                        nPos += PICE_sprintf( pDis->szDisasm+nPos, "%08X", (unsigned int)(pDis->bpTarget + (signed long)dwULONG + bInstrLen) );
                 }
                 else
                 {
                     wUSHORT = NEXTUSHORT;
                     if(ScanExportsByAddress(&pSymbolName,(unsigned int)(pDis->bpTarget + (signed short)wUSHORT + bInstrLen)))
                        nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", pSymbolName );
                     else
                        nPos += PICE_sprintf( pDis->szDisasm+nPos, "%08X", (unsigned int)(pDis->bpTarget + (signed short)wUSHORT + bInstrLen) );
                 }
             break;

             case _O  :                                         // Simple USHORT or ULONG offset
                  if( pDis->dwFlags & DIS_ADDRESS32 )           // depending on the address size
                  {
                      dwULONG = NEXTULONG;
                      nPos += PICE_sprintf( pDis->szDisasm+nPos,"%s[%08X]", sSegOverride[ bSegOverride ], (unsigned int) dwULONG );
                  }
                  else
                  {
                      wUSHORT = NEXTUSHORT;
                      nPos += PICE_sprintf( pDis->szDisasm+nPos,"%s[%04X]", sSegOverride[ bSegOverride ], wUSHORT );
                  }
             break;

             case _Ib :                                         // immediate UCHAR
                 bUCHAR = NEXTUCHAR;
                 nPos += PICE_sprintf( pDis->szDisasm+nPos,"%02X", bUCHAR );
             break;

             case _Iv :                                         // immediate USHORT or ULONG
                 if( pDis->dwFlags & DIS_DATA32 )
                 {
                      dwULONG = NEXTULONG;
                      nPos += PICE_sprintf( pDis->szDisasm+nPos, "%08X", (unsigned int) dwULONG );
                 }
                 else
                 {
                     wUSHORT = NEXTUSHORT;
                     nPos += PICE_sprintf( pDis->szDisasm+nPos, "%04X", wUSHORT );
                 }
             break;

             case _Iw :                                         // Immediate USHORT
                 wUSHORT = NEXTUSHORT;
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%04X", wUSHORT );
             break;

             case _Ap :                                         // 32 bit or 48 bit pointer (call far, jump far)
                 if( pDis->dwFlags & DIS_DATA32 )
                 {
                      dwULONG = NEXTULONG;
                      wUSHORT = NEXTUSHORT;
                      nPos += PICE_sprintf( pDis->szDisasm+nPos, "%04X:%08X", wUSHORT, (unsigned int) dwULONG );
                 }
                 else
                 {
                     dwULONG = NEXTULONG;
                     nPos += PICE_sprintf( pDis->szDisasm+nPos, "%08X", (unsigned int) dwULONG );
                 }
             break;

             case _1 :                                          // numerical 1
                 nPos += PICE_sprintf( pDis->szDisasm+nPos,"1" );
             break;

             case _3 :                                          // numerical 3
                 nPos += PICE_sprintf( pDis->szDisasm+nPos,"3" );
             break;

                                                                // Hard coded registers
             case _DX: case _AL: case _AH: case _BL: case _BH: case _CL: case _CH:
             case _DL: case _DH: case _CS: case _DS: case _ES: case _SS: case _FS:
             case _GS:
                 nPos += PICE_sprintf( pDis->szDisasm+nPos,"%s", sRegs2[ *pArg - _DX ] );
             break;

             case _eAX: case _eBX: case _eCX: case _eDX:
             case _eSP: case _eBP: case _eSI: case _eDI:
                 nPos += PICE_sprintf( pDis->szDisasm+nPos, "%s", sGenReg16_32[DIS_GETDATASIZE(pDis->dwFlags)][ *pArg - _eAX ]);
             break;

             case _ST:                                          // Coprocessor ST
                nPos += PICE_sprintf( pDis->szDisasm+nPos,"%s", sST[9] );
             break;

            case _ST0:                                         // Coprocessor ST(0) - ST(7)
            case _ST1:
            case _ST2:
            case _ST3:
            case _ST4:
            case _ST5:
            case _ST6:
            case _ST7:
               nPos += PICE_sprintf( pDis->szDisasm+nPos,"%s", sST[ *pArg - _ST0 ] );
            break;

            case _AX:                                           // Coprocessor AX
                nPos += PICE_sprintf( pDis->szDisasm+nPos,"%s", sGenReg16_32[0][0] );
            break;
        }
    }

DisEnd:

    // Set the returning values and return with the bInstrLen field

    pDis->bAsciiLen = (UCHAR) nPos;
    pDis->bInstrLen = bInstrLen;

    return bInstrLen;
}

/******************************************************************************
*                                                                             *
*   BOOLEAN Disasm(PULONG pOffset,PUCHAR pchDst)                              *
*                                                                             *
*   entry point for disassembly from other modules                            *
******************************************************************************/
BOOLEAN Disasm(PULONG pOffset,PUCHAR pchDst)
{
    TDisassembler dis;
 
    dis.dwFlags  = DIS_DATA32 | DIS_ADDRESS32;
    dis.bpTarget = (UCHAR*)*pOffset;
    dis.szDisasm = pchDst;
    dis.wSel = CurrentCS;

    *pOffset += (ULONG)Disassembler( &dis);
    return TRUE;
}