/***********************************************************************
* Microsoft Disassembler
*
* Microsoft Confidential.  Copyright 1994-1997 Microsoft Corporation.
*
* Component:
*
* File: dismips16.h
*
* File Comments:
*
*   This file is a copy of the master version owned by richards.
*   Contact richards for any changes.
*
***********************************************************************/

#pragma pack(push, 8)

class DISMIPS16 : public DIS
{
public:
   enum TRMTA
   {
      trmtaUnknown = DIS::trmtaUnknown,
      trmtaFallThrough = DIS::trmtaFallThrough,
      trmtaTrap,
      trmtaBra,
      trmtaBraCc,
      trmtaBraIndDef,
      trmtaCallDef,
      trmtaCallIndDef,
   };


   enum REGA
   {
      regaR0   = 0,
      regaR1   = 1,
      regaR2   = 2,
      regaR3   = 3,
      regaR4   = 4,
      regaR5   = 5,
      regaR6   = 6,
      regaR7   = 7,
      regaR8   = 8,
      regaR9   = 9,
      regaR10  = 10,
      regaR11  = 11,
      regaR12  = 12,
      regaR13  = 13,
      regaR14  = 14,
      regaR15  = 15,
      regaR16  = 16,
      regaR17  = 17,
      regaR18  = 18,
      regaR19  = 19,
      regaR20  = 20,
      regaR21  = 21,
      regaR22  = 22,
      regaR23  = 23,
      regaR24  = 24,
      regaR25  = 25,
      regaR26  = 26,
      regaR27  = 27,
      regaR28  = 28,
      regaR29  = 29,
      regaR30  = 30,
      regaR31  = 31,

      regaZero = 0,
      regaAt   = 1,
      regaV0   = 2,
      regaV1   = 3,
      regaA0   = 4,
      regaA1   = 5,
      regaA2   = 6,
      regaA3   = 7,
      regaT0   = 8,
      regaT1   = 9,
      regaT2   = 10,
      regaT3   = 11,
      regaT4   = 12,
      regaT5   = 13,
      regaT6   = 14,
      regaT7   = 15,
      regaS0   = 16,
      regaS1   = 17,
      regaS2   = 18,
      regaS3   = 19,
      regaS4   = 20,
      regaS5   = 21,
      regaS6   = 22,
      regaS7   = 23,
      regaT8   = 24,
      regaT9   = 25,
      regaK0   = 26,
      regaK1   = 27,
      regaGp   = 28,
      regaSp   = 29,
      regaS8   = 30,
      regaRa   = 31,

      regaPc   = 32,

   };


        DISDLL DISMIPS16(DIST);

        // Methods inherited from DIS

        ADDR AddrOperand(size_t) const;
        ADDR AddrTarget() const;
        size_t Cb() const;
        size_t CbDisassemble(ADDR, const void *, size_t);
        size_t CbJumpEntry() const;
        size_t CbOperand(size_t) const;
        size_t CchFormatBytes(char *, size_t) const;
        size_t CchFormatBytesMax() const;
        size_t Coperand() const;
        void FormatAddr(std::ostream&, ADDR) const;
        void FormatInstr(std::ostream&) const;
        MEMREFT Memreft(size_t) const;
        TRMT Trmt() const;

        DIS::TRMTA Trmta() const;

private:
   union IW                // Instruction Word
   {
      WORD    w;

      struct
      {
     WORD Immediate : 11;
     WORD Opcode : 5;
      } i_format;

      struct
      {
     WORD Immediate : 8;
     WORD Rx : 3;
     WORD Opcode : 5;
      } ri_format;

      struct
      {
     WORD Func : 5;
     WORD Ry : 3;
     WORD Rx : 3;
     WORD Opcode : 5;
      } rr_format;

      struct
      {
     WORD Immediate : 5;
     WORD Ry : 3;
     WORD Rx : 3;
     WORD Opcode : 5;
      } rri_format;

      struct
      {
     WORD Func : 2;
     WORD Rz : 3;
     WORD Ry : 3;
     WORD Rx : 3;
     WORD Opcode : 5;
      } rrr_format;

      struct
      {
     WORD Immediate : 4;
     WORD Func : 1;
     WORD Ry : 3;
     WORD Rx : 3;
     WORD Opcode : 5;
      } rria_format;

      struct
      {
     WORD Func : 2;
     WORD Shift : 3;
     WORD Ry : 3;
     WORD Rx : 3;
     WORD Opcode : 5;
      } shift_format;

      struct
      {
     WORD Immediate : 8;
     WORD Func : 3;
     WORD Opcode : 5;
      } i8_format;

      struct
      {
     WORD R32 : 5;
     WORD Ry : 3;
     WORD Func : 3;
     WORD Opcode : 5;
      } mover32_format;

      struct
      {
     WORD Rw : 3;
     WORD R323 : 2;        // mangled
     WORD R320 : 3;
     WORD Func : 3;
     WORD Opcode : 5;
      } move32r_format;

      struct
      {
     WORD Immediate : 8;
     WORD Func : 3;
     WORD Opcode : 5;
      } i64_format;

      struct
      {
     WORD Immediate : 5;
     WORD Ry : 3;
     WORD Func : 3;
     WORD Opcode : 5;
      } ri64_format;

      struct
      {
     WORD Target21 : 5;
     WORD Target16 : 5;
     WORD Ext : 1;
     WORD Opcode : 5;
      } j_format;

      struct
      {
     WORD Func : 5;
     WORD Ry : 3;
     WORD Shift : 3;
     WORD Opcode : 5;
      } shift64_format;

      struct
      {
     WORD Zero1 : 5;
     WORD Func : 3;
     WORD Rx : 3;
     WORD Opcode : 5;
      } jr_format;

      struct
      {
     WORD Break : 5;
     WORD Code : 6;
     WORD Opcode : 5;
      } break_format;
   };

   union EW                // Extend word
   {
      WORD w;

      struct
      {
     WORD Immediate11 : 5;
     WORD Immediate5 : 6;
     WORD Opcode : 5;
      } ei_format;

      struct
      {
     WORD Immediate11 : 4;
     WORD Immediate4 : 7;
     WORD Opcode : 5;
      } erria_format;

      struct
      {
     WORD Zero : 5;
     WORD Shift5 : 1;
     WORD Shift0 : 5;
     WORD Opcode : 5;
      } eshift_format;
   };

   enum OPCLS                  // Operand Class
   {
      opclsNone,               // No operand

      opclsSp,                 // SP register
      opclsPc,                 // PC register
      opclsRa,                 // RA register

      opclsRx,                 // MIPS16 general register Rx
      opclsRy,                 // MIPS16 general register Ry
      opclsRz,                 // MIPS16 general register Rz
      opclsRw,                 // MIPS16 general register Rw

      opclsMemRxB_r,               // Memory Rx with 5bit byte offset (read)
      opclsMemRxH_r,               // Memory Rx with 5bit halfword offset (read)
      opclsMemRxW_r,               // Memory Rx with 5bit word offset (read)
      opclsMemRxD_r,               // Memory Rx with 5bit dword offset (read)

      opclsMemPcW_r,               // Memory PC with 8bit word offset (read)
      opclsMemPcD_r,               // Memory PC with 5bit dword offset (read)

      opclsMemSpW_r,               // Memory SP with 8bit word offset (read)
      opclsMemSpD5_r,              // Memory SP with 5bit dword offset (read)
      opclsMemSpD8_r,              // Memory SP with 8bit dword offset (read)

      opclsMemRxB_w,               // Memory Rx with 5bit byte offset (write)
      opclsMemRxH_w,               // Memory Rx with 5bit halfword offset (write)
      opclsMemRxW_w,               // Memory Rx with 5bit word offset (write)
      opclsMemRxD_w,               // Memory Rx with 5bit dword offset (write)

      opclsMemSpW_w,               // Memory SP with 8bit word offset (write)
      opclsMemSpD5_w,              // Memory SP with 5bit dword offset (write)
      opclsMemSpD8_w,              // Memory SP with 8bit dword offset (write)

      opclsImmSB4,             // Signed 4bit byte immediate
      opclsImmSB5,             // Signed 5bit byte immediate
      opclsImmSB8,             // Signed 8bit byte immediate
      opclsImmSD8,             // Signed 8bit dword immediate
      opclsImmUB8,             // Unsigned 8bit byte immediate
      opclsImmUB8U,            // Unsigned 8bit byte immediate (unsigned even on extend)
      opclsImmUW5,             // Unsigned 5bit word immediate
      opclsImmUW8,             // Unsigned 8bit word immediate

      opclsShift0,             // Shift amount type 0
      opclsShift1,             // Shift amount type 1
      opclsShift2,             // Shift amount type 2

      opclsR32Src,             // MIPS32 general register source
      opclsR32Dst,             // MIPS32 general register destination

      opclsAddrJmp,            // Jump instruction target
      opclsAddrBraC,               // Conditional branch instruction target
      opclsAddrBraU,               // Unconditional branch instruction target

      opclsBrkImm,             // Immediate value for break instruction
   };

   enum ICLS                   // Instruction Class
   {
       // Invalid Class

      iclsInvalid,

       // Rx-offset Load Byte Class
       //
       // Text Format:     LB   ry,offset(rx)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx
       // Registers Set:       Ry

      iclsRxLoadByte,

       // Rx-offset Load Halfword Class
       //
       // Text Format:     LH   ry,offset(rx)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx
       // Registers Set:       Ry

      iclsRxLoadHalfword,

       // Rx-offset Load Word Class
       //
       // Text Format:     LW   ry,offset(rx)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx
       // Registers Set:       Ry

      iclsRxLoadWord,

       // PC-offset Load Word Class
       //
       // Text Format:     LW   rx,offset(pc)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      PC
       // Registers Set:       Rx

      iclsPcLoadWord,

       // SP-offset Load Word Class
       //
       // Text Format:     LW   rx,offset(sp)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      SP
       // Registers Set:       Rx

      iclsSpLoadWord,

       // Rx-offset Load DWord Class
       //
       // Text Format:     LD   ry,offset(rx)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx
       // Registers Set:       Ry

      iclsRxLoadDword,

       // PC-offset Load DWord Class
       //
       // Text Format:     LD   ry,offset(pc)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      PC
       // Registers Set:       Ry

      iclsPcLoadDword,

       // SP-offset Load DWord Class
       //
       // Text Format:     LD   ry,offset(sp)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      SP
       // Registers Set:       Ry

      iclsSpLoadDword,

       // Rx-offset Store Byte Class
       //
       // Text Format:     SB   ry,offset(rx)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx, Ry
       // Registers Set:

      iclsRxStoreByte,

       // Rx-offset Store Halfword Class
       //
       // Text Format:     SH   ry,offset(rx)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx, Ry
       // Registers Set:

      iclsRxStoreHalfword,

       // Rx-offset Store Word Class
       //
       // Text Format:     SW   ry,offset(rx)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx, Ry
       // Registers Set:

      iclsRxStoreWord,

       // SP-offset Store Word Class
       //
       // Text Format:     SW   ry,offset(sp)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      SP, Rx
       // Registers Set:

      iclsSpStoreWord,

       // SP-offset Store RA Word Class
       //
       // Text Format:     SW   ra,offset(sp)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      SP, RA
       // Registers Set:

      iclsSpStoreRaWord,

       // Rx-offset Store DWord Class
       //
       // Text Format:     SD   ry,offset(rx)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx, Ry
       // Registers Set:

      iclsRxStoreDword,

       // SP-offset Store DWord Class
       //
       // Text Format:     SD   ry,offset(sp)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      SP, Ry
       // Registers Set:

      iclsSpStoreDword,

       // SP-offset Store RA DWord Class
       //
       // Text Format:     SD   ra,offset(sp)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      SP, RA
       // Registers Set:

      iclsSpStoreRaDword,

       // Immediate Type 3 Class
       //
       // Text Format:     SLTI   rx,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:
       // Registers Set:       Rx

      iclsImmediate3,

       // Immediate Type 0 Class
       //
       // Text Format:     ADDIU   ry,rx,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx
       // Registers Set:       Ry

      iclsImmediate0,

       // Immediate Type 1 Class
       //
       // Text Format:     ADDIU   rx,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx
       // Registers Set:       Rx

      iclsImmediate1,

       // Immediate Type 4 Class
       //
       // Text Format:     LI   rx,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx
       // Registers Set:       Rx

      iclsImmediate4,

       // Immediate SP Class
       //
       // Text Format:     ADDIU   sp,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      SP
       // Registers Set:       SP


      iclsImmSp,

       // Immediate Rx-PC Class
       //
       // Text Format:     ADDIU   rx,pc,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx, PC
       // Registers Set:       Rx

      iclsImmRxPc,

       // Immediate Ry-PC Class
       //
       // Text Format:     DADDIU   ry,pc,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Ry, PC
       // Registers Set:       Ry

      iclsImmRyPc,

       // Immediate Rx-SP Class
       //
       // Text Format:     ADDIU   rx,sp,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx, SP
       // Registers Set:       Rx

      iclsImmRxSp,

       // Immediate Type 2 Class
       //
       // Text Format:     DADDIU   ry,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Ry
       // Registers Set:       Ry

      iclsImmediate2,

       // Immediate Ry-SP Class
       //
       // Text Format:     DADDIU   ry,sp,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Ry, SP
       // Registers Set:       Ry

      iclsImmRySp,

       // Two Operand Class
       //
       // Text Format:     CMP   rx,ry
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx, Ry
       // Registers Set:

      iclsOp2,

       // Three Operand Class
       //
       // Text Format:     ADDU   rz,rx,ry
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx, Ry
       // Registers Set:       Rz

      iclsOp3,

       // Move Ry,R32 Class
       //
       // Text Format:     MOVE   Ry,R32
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      R32
       // Registers Set:       Ry

      iclsMoveR32,

       // Move R32,Rw Class
       //
       // Text Format:     MOVE   R32,Rw
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rw
       // Registers Set:       R32

      iclsMove32R,

       // Shift Type 0 Class
       //
       // Text Format:     SLL    ry,rx,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Ry
       // Registers Set:       Rx

      iclsShift0,

       // Shift Variable Class
       //
       // Text Format:     SLLV    ry,rx
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rx, Ry
       // Registers Set:       Ry

      iclsShiftVar,

       // Shift Type 1 Class
       //
       // Text Format:     DSRL    ry,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Ry
       // Registers Set:       Ry

      iclsShift1,

       // Shift Type 2 Class
       //
       // Text Format:     DSLL    rx,ry,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Ry
       // Registers Set:       Rx

      iclsShift2,

       // Move From HI/LO Class
       //
       // Text Format:     MFHI    rx
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:
       // Registers Set:       Rx

      iclsMoveHiLo,

       // Call Class
       //
       // Text Format:     JAL    target
       //
       // Termination Type:    trmtaCallDef
       //
       // Registers Used:
       // Registers Set:       RA

      iclsCall,

       // Jump Register Rx Class
       //
       // Text Format:     JR    rx
       //
       // Termination Type:    trmtaBraInd
       //
       // Registers Used:      Rx
       // Registers Set:

      iclsJumpReg,

       // Jump Register RA Class
       //
       // Text Format:     JR    ra
       //
       // Termination Type:    trmtaBraInd
       //
       // Registers Used:      RA
       // Registers Set:

      iclsJumpRegRa,

       // Call Register Class
       //
       // Text Format:     JALR    ra,rx
       //
       // Termination Type:    trmtaCallInd
       //
       // Registers Used:      Rx
       // Registers Set:       RA

      iclsCallReg,

       // Branch Conditional Type 0
       //
       // Text Format:     BEQZ    rx,target
       //
       // Termination Type:    trmtaBraCc
       //
       // Registers Used:      Rx
       // Registers Set:       RA

      iclsBraCc0,

       // Branch Conditional Type 1
       //
       // Text Format:     BTEQZ    target
       //
       // Termination Type:    trmtaBraCc
       //
       // Registers Used:      T
       // Registers Set:

      iclsBraCc1,

       // Branch Unconditional
       //
       // Text Format:     B     target
       //
       // Termination Type:    trmtaBra
       //
       // Registers Used:
       // Registers Set:

      iclsBra,

       // Trap
       //
       // Text Format:     BREAK  imm
       //
       // Termination Type:    trmtaTrap
       //
       // Registers Used:
       // Registers Set:

      iclsTrap,

       // Nop
       //
       // Text Format:     NOP
       //
       // Termination Type:    trmtaTrap
       //
       // Registers Used:
       // Registers Set:

      iclsNop,
   };

   struct CLS
   {
      TRMTA   trmta;
      OPCLS   rgopcls[3];         // Operand class for each operand
   };

   struct OPCD
   {
      const char  *szMnemonic;
      ICLS    icls;
   };

   static   const TRMT mptrmtatrmt[];

   static   const CLS rgcls[];

   static   const OPCD rgopcd[];
   static   const OPCD rgopcdJal[];
   static   const OPCD rgopcdShift[];
   static   const OPCD rgopcdRria[];
   static   const OPCD rgopcdI8[];
   static   const OPCD rgopcdRrr[];
   static   const OPCD rgopcdRr[];
   static   const OPCD rgopcdI64[];
   static   const OPCD rgopcdJr[];
   static   const OPCD rgopcdNop;

   static   const char * const rgszGpr[];

   static   const REGA rgiMap[];

        bool FCalcOffsetReg(OPCLS, DWORD *, REGA *) const;
        void FormatImmediate (std::ostream&, DWORD) const;
        void FormatOffsetReg(std::ostream&, DWORD, REGA) const;
        void FormatOperand(std::ostream&, OPCLS) const;
        bool FValidOperand(size_t) const;
   static   const OPCD *PopcdDecode(IW);

        IW    m_iw;         // current instruction word
        EW    m_ew;         // current extend word
        WORD  m_nw;         // next word
        DWORD m_imm4;       // sign extended 4-bit immediate
        DWORD m_imm5;       // sign extended 5-bit immediate
        DWORD m_imm8;       // sign extended 8-bit immediate
        DWORD m_imm11;      // sign extended 11-bit immediate
        ADDR  m_basePc;     // value of PC for this instruction
        bool  m_fExtend;        // this instruction was extended
        const OPCD *m_popcd;
};

#pragma pack(pop)
