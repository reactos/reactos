/***********************************************************************
* Microsoft Disassembler
*
* Microsoft Confidential.  Copyright 1994-1997 Microsoft Corporation.
*
* Component:
*
* File: disx86.h
*
* File Comments:
*
*   This file is a copy of the master version owned by richards.
*   Contact richards for any changes.
*
***********************************************************************/

#pragma pack(push, 8)

class DISX86 : public DIS
{
public:
   enum TRMTA
   {
      trmtaUnknown = DIS::trmtaUnknown,
      trmtaFallThrough = DIS::trmtaFallThrough,
      trmtaTrap,
      trmtaTrapCc,
      trmtaJmpShort,
      trmtaJmpNear,
      trmtaJmpFar,
      trmtaJmpInd,
      trmtaRet,
      trmtaIret,
      trmtaJmpCcShort,
      trmtaJmpCcNear,
      trmtaLoop,
      trmtaJcxz,
      trmtaCallNear16,
      trmtaCallNear32,
      trmtaCallFar,
      trmtaCallInd,
   };


   enum REGA
   {
      regaEax = 0,
      regaEcx = 1,
      regaEdx = 2,
      regaEbx = 3,
      regaEsp = 4,
      regaEbp = 5,
      regaEsi = 6,
      regaEdi = 7,
   };


   class REGMASK : public DIS::REGMASK
   {
   public:
	       DISDLL REGMASK();

	       void And(void *);
	       size_t Cbregmask() const;
	       void Clear();
	       void CopyMask(void *) const;
	       bool FEq(void *) const;
	       bool FRegPresent(int) const;
	       void Minus(void *);
	       void Or(void *);
	       void *PvMask() const;
	       void SetMask(void *);
	       void SetReg(int);

   private:
	       DWORD m_dwMask;

	       static const DWORD mpiregdwMaskx86[];
   };

	    DISX86(DIST);

	    // Methods inherited from DIS

	    ADDR AddrAddress(size_t) const;
	    ADDR AddrJumpTable() const;
	    ADDR AddrOperand(size_t) const;
	    ADDR AddrTarget() const;
	    size_t Cb() const;
	    size_t CbDisassemble(ADDR, const void *, size_t);
	    size_t CbJumpEntry() const;
	    size_t CbGenerateLoadAddress(size_t, void *, size_t, size_t * = NULL) const;
	    size_t CbOperand(size_t) const;
	    size_t CchFormatBytes(char *, size_t) const;
	    size_t CchFormatBytesMax() const;
	    size_t Coperand() const;
	    void FormatAddr(std::ostream&, ADDR) const;
	    void FormatInstr(std::ostream&) const;
	    MEMREFT Memreft(size_t) const;
	    TRMT Trmt() const;
	    DIS::TRMTA Trmta() const;

   // UNDONE: The following are used only by BBRCHK

	    void *PvRegRdMask();
	    void *PvRegWrMask();

//Shri

	    int Ciop(IOP *, size_t) const;
	    size_t CiopMax() const;
	    DIS::REGA RegaMax() const;

//Shri

	    // New methods introduced by DISX86

	    bool FAddress32() const { return(m_fAddress32); }
	    bool FOperand32() const { return(m_fOperand32); }

   // UNDONE: The following types should be private

public:

   enum MODRMT			       // MODRM type
   {
      modrmtNo, 		       // Not present
      modrmtYes,		       // Present
      modrmtMem,		       // Memory only
      modrmtReg,		       // Register only
   };

   enum ICB			       // Immediate byte count
   {
      icbNil,			       // No immediate value
      icbAP,			       // Far pointer
      icbIb,			       // Immediate byte
      icbIb0A,			       // Immediate byte (must be 0Ah)
      icbIv,			       // Immediate operand size value
      icbIw,			       // Immediate word
      icbIw_Ib, 		       // Immediate word and immediate byte
      icbJb,			       // Byte displacement
      icbJv,			       // Address size displacement
      icbO,			       // Address size value
   };

   enum OPRNDT			       // Operand type
   {
      oprndtNil,		       // No operand
      oprndtAP, 		       // Far address
      oprndtCd, 		       // CRx register from MODRM reg
      oprndtConst,		       // Constant from bValue
      oprndtDd, 		       // DRx register from MODRM reg
      oprndtGvOp,		       // General register (operand size) from opcode
      oprndtGvOp2,		       // General register (operand size) from opcode (2nd byte)
      oprndtIb, 		       // Immediate byte
      oprndtIb2,		       // Immediate byte following word
      oprndtIv, 		       // Immediate operand size value
      oprndtIw, 		       // Immediate word
      oprndtJb, 		       // Relative address byte
      oprndtJv, 		       // Relative address operand size value
      oprndtMmModrm,		       // Memory/MM register references from MODRM (MMX)
      oprndtMmModrmReg, 	       // MM register from MODRM reg (MMX)
      oprndtModrm,		       // Memory/register references from MODRM
      oprndtModrmReg,		       // General register from MODRM reg
      oprndtModrmSReg,		       // Segment register from MODRM reg
      oprndtOffset,		       // Address size immediate offset
      oprndtRb, 		       // General register (byte) from bValue
      oprndtRv, 		       // General register (operand size) from bValue
      oprndtRw, 		       // General register (word) from bValue
      oprndtST, 		       // Floating point top of stack
      oprndtSTi,		       // Floating point register from MODRM reg
      oprndtSw, 		       // Segment register (word) from bValue
      oprndtTd, 		       // TRx register from MODRM reg
      oprndtX,			       // DS:[eSI] for string instruction
      oprndtY,			       // ES:[eDI] for string instruction
      oprndtZ,			       // DS:[eBX] for XLAT
   };

   enum OPREFT			       // Operand reference type
   {
      opreftNil,		       // Operand not referenced (e.g. INVLPG)
      opreftRd, 		       // Operand is read
      opreftRw, 		       // Operand is read and written
      opreftWr			       // Operand is written
   };

   struct OPRND 		       // Operand
   {
      OPREFT Opreft() const { return((OPREFT) opreft); };
      OPRNDT Oprndt() const { return((OPRNDT) oprndt); };

      unsigned char  opreft : 2;
      unsigned char  oprndt : 6;
      unsigned char  bValue;
   };

   struct OPS			       // Instruction operands
   {
      MODRMT   modrmt;
      ICB      icb;
      OPRND    rgoprnd[3];
   };

private:

   struct OPCD
   {
      const void     *pvMnemonic;
      const OPS      *pops;
      unsigned char  trmta;
   };

   static   const char rgszReg8[8][4];
   static   const char rgszReg16[8][4];
   static   const char rgszReg32[8][4];
   static   const char rgszSReg[8][4];
   static   const char rgszBase16[8][8];

   static   const DIS::TRMT mptrmtatrmt[];

   static   const OPCD rgopcd[256];
   static   const OPCD rgopcd0F[256];
   static   const OPCD rgopcd0F00[8];
   static   const OPCD rgopcd0F01[8];
   static   const OPCD rgopcd0F0D[8];
   static   const OPCD rgopcd0F0F[256];
   static   const OPCD rgopcd0F71[8];
   static   const OPCD rgopcd0F72[8];
   static   const OPCD rgopcd0F73[8];
   static   const OPCD rgopcd0FBA[8];
   static   const OPCD rgopcd0FC7[8];
   static   const OPCD rgopcd80[8];
   static   const OPCD rgopcd81[8];
   static   const OPCD rgopcd83[8];
   static   const OPCD rgopcdC0[8];
   static   const OPCD rgopcdC1[8];
   static   const OPCD rgopcdD0[8];
   static   const OPCD rgopcdD1[8];
   static   const OPCD rgopcdD2[8];
   static   const OPCD rgopcdD3[8];
   static   const OPCD rgopcdF6[8];
   static   const OPCD rgopcdF7[8];
   static   const OPCD rgopcdFE[8];
   static   const OPCD rgopcdFF[8];
   static   const OPCD rgopcdD8[8];
   static   const OPCD rgopcdD9[8];
   static   const OPCD rgopcdDA[8];
   static   const OPCD rgopcdDB[8];
   static   const OPCD rgopcdDC[8];
   static   const OPCD rgopcdDD[8];
   static   const OPCD rgopcdDE[8];
   static   const OPCD rgopcdDF[8];
   static   const OPCD rgopcdD8_[8];
   static   const OPCD rgopcdD9_[64];
   static   const OPCD opcdDAE9;
   static   const OPCD rgopcdDA_[8];
   static   const OPCD rgopcdDB_[8];
   static   const OPCD rgopcdDB__[8];
   static   const OPCD rgopcdDC_[8];
   static   const OPCD rgopcdDD_[8];
   static   const OPCD rgopcdDE_[8];
   static   const OPCD opcdDED9;
   static   const OPCD rgopcdDF_[8];
   static   const OPCD rgopcdDF__[8];

	    const BYTE *m_pbCur;
	    size_t m_cbMax;

	    size_t m_cb;
	    BYTE m_rgbInstr[16];

	    const struct OPCD *m_popcd;

	    bool m_fAddress32;
	    bool m_fOperand32;
	    BYTE m_bSegOverride;
	    BYTE m_bPrefix;

	    bool m_fAddrOverride;
	    bool m_fOperOverride;

	    size_t m_ibOp;
	    size_t m_ibModrm;
	    size_t m_cbModrm;
	    size_t m_ibImmed;
	    TRMTA m_trmta;

	    REGMASK m_regmaskRd;
	    REGMASK m_regmaskWr;

	    ADDR AddrJumpTable16() const;
	    ADDR AddrJumpTable32() const;
	    ADDR AddrOperandModrm16() const;
	    ADDR AddrOperandModrm32() const;
	    size_t CbGenerateLea(BYTE *, size_t, size_t * = NULL) const;
	    size_t CbGenerateLeaXSp(BYTE *, size_t, size_t) const;
	    size_t CbGenerateMovOffset(BYTE *, size_t, size_t * = NULL) const;
	    size_t CbGenerateMovXDi(BYTE *, size_t) const;
	    size_t CbGenerateMovXSi(BYTE *, size_t) const;
	    size_t CbOperandDefault() const;
	    bool FDisassembleModrm16(BYTE);
	    bool FDisassembleModrm32(BYTE);
	    bool FImplicitFar() const;
	    bool FImplicitPop() const;
	    bool FImplicitPush() const;
	    void FormatHex(std::ostream&, DWORD) const;
	    void FormatModrm16(std::ostream&, unsigned, bool) const;
	    void FormatModrm32(std::ostream&, unsigned, bool) const;
	    void FormatOperand(std::ostream&, OPRNDT, unsigned) const;
	    void FormatOpSize(std::ostream&, unsigned cb, bool) const;
	    void FormatReg32(std::ostream&, REGA) const;
	    void FormatRegister(std::ostream&, int, unsigned) const;
	    void FormatSegOverride(std::ostream&) const;
	    bool FValidOperand(size_t) const;
	    size_t IbDispModrm16() const;
	    size_t IbDispModrm32() const;
	    size_t IbDispOffset() const;
   static   const OPCD *PopcdFloatingPoint(BYTE, BYTE);
	    void UpdateRegs(int, OPREFT, unsigned char);

	    int  CSetIOPImplicit( IOP *) const;
	    bool FSetIOPModrm16(IOPRND &, unsigned, bool) const;
	    bool FSetIOPModrm32(IOPRND &, unsigned, bool) const;
	    bool SetIOPOprnd(IOPRND &, const OPRND &) const;
};

#pragma pack(pop)
