/***********************************************************************
* Microsoft Disassembler
*
* Microsoft Confidential.  Copyright 1994-1997 Microsoft Corporation.
*
* Component:
*
* File: dissh.h
*
* File Comments:
*
*   This file is a copy of the master version owned by richards.
*   Contact richards for any changes.
*
***********************************************************************/

#pragma pack(push, 8)

class DISSH : public DIS
{
public:
   enum TRMTA
   {
      trmtaUnknown = DIS::trmtaUnknown,
      trmtaFallThrough = DIS::trmtaFallThrough,
      trmtaTrap,
      trmtaBraCc,
      trmtaBraDef,
      trmtaBraIndDef,
      trmtaCallDef,
      trmtaCallIndDef,
   };

   struct IW_N
   {
      WORD op2 : 8;
      WORD Rn  : 4;
      WORD op  : 4;
   };

   struct IW_M
   {
      WORD op2 : 8;
      WORD Rm  : 4;
      WORD op  : 4;
   };

   struct IW_NM
   {
      WORD op2 : 4;
      WORD Rm  : 4;
      WORD Rn  : 4;
      WORD op  : 4;
   };

   struct IW_MD
   {
      WORD d   : 4;
      WORD Rm  : 4;
      WORD op2 : 4;
      WORD op  : 4;
   };

   struct IW_NMD
   {
      WORD d   : 4;
      WORD Rm  : 4;
      WORD Rn  : 4;
      WORD op  : 4;
   };

   struct IW_D
   {
      WORD d   : 8;
      WORD op2 : 4;
      WORD op  : 4;
   };

   struct IW_D12
   {
      WORD d   : 12;
      WORD op  : 4;
   };

   struct IW_ND8
   {
      WORD d   : 8;
      WORD Rn  : 4;
      WORD op  : 4;
   };

   struct IW_I
   {
      WORD i   : 8;
      WORD op2 : 4;
      WORD op  : 4;
   };

   struct IW_NI
   {
      WORD i   : 8;
      WORD Rn  : 4;
      WORD op  : 4;
   };

   union IW			       // Instruction Word
   {
      WORD     w;

      IW_N     N;
      IW_M     M;
      IW_NM    NM;
      IW_MD    MD;
      IW_NMD   NMD;
      IW_D     D;
      IW_D12   D12;
      IW_ND8   ND8;
      IW_I     I;
      IW_NI    NI;

      struct			       //  XXXX R#   opcode
      { 			       //   4	4     8 bits
	 WORD bits3_0 : 4;	       // bits 3-0
	 WORD bits7_4 : 4;	       // bits 7-4
	 WORD reg     : 4;	       // register source/destination
	 WORD opcode  : 4;	       // 4 bit (15-12) opcode
      } instr1;

      struct			       //  XXXX R# immediate | displacement
      { 			       //   4	4	    ( 8 bits)
	 WORD last8  : 8;	       // immediate data/addressing or displacement
	 WORD reg    : 4;	       // register source/destination
	 WORD opcode : 4;	       // 4 bit (15-12) opcode
      } instr2;

      struct			       // XXXX Rn Rm opcode ext | displacement
      { 			       //  4   4  4	      (4 bit)
	 WORD last4  : 4;
	 WORD regM   : 4;	       // Rm source register
	 WORD regN   : 4;	       // Rn destination register
	 WORD opcode : 4;	       // 4 bit (15-12) opcode
      } instr3;

      struct			       // XXXXXXXX Reg# displacement
      { 			       //     8       4       4
	 WORD last4	: 4;	       // 4 bit (3-0) displacement
	 WORD reg	: 4;	       // Rn destination register
	 WORD bits11_8	: 4;	       // bits 11-8
	 WORD bits15_12 : 4;	       // bits 15-12
      } instr4;

      struct			       // XXXXXXXX  immediate | displacement
      { 			       //  8 bits	    8 bits
	 WORD last8	: 8;	       // displacement
	 WORD bits11_8	: 4;	       // 8 bit (15-8) opcode
	 WORD bits15_12 : 4;
      } instr5;

      struct
      {
	 WORD opcode : 16;	       // 16 bit opcode
      } instr6;

      struct			       // XXXX		displacement
      { 			       //  4 bits	 12 bits
	 WORD last12 : 12;	       // displacement
	 WORD opcode : 4;	       // 8 bit (15-8) opcode
      } instr7;

      struct			       //  XXXX Rn Rm   opcode
      { 			       //   4	2 2     8 bits
	 WORD bits3_0 : 4;	       // bits 3-0
	 WORD bits7_4 : 4;	       // bits 7-4
	 WORD bits9_8 : 2;	       // bits 9-8  (sometimes vector regM)
	 WORD bits11_10 : 2;	   // bits 11-10 (sometimes vector regN)
	 WORD opcode  : 4;	       // 4 bit (15-12) opcode
      } instr8;

      struct
      {
	 WORD high8 : 8;	       // bits 7-0
	 WORD low8  : 8;	       // bits 15-8
      } data;
   };


	    DISSH(DIST);

  // Methods inherited from DIS
  
	    // Methods inherited from DIS

	    ADDR AddrOperand(size_t) const;
	    ADDR AddrTarget() const;
	    size_t Cb() const;
	    size_t CbDisassemble(ADDR, const void *, size_t);
	    size_t CbJumpEntry() const;
	    size_t CchFormatBytes(char *, size_t) const;
	    size_t CchFormatBytesMax() const;
	    size_t Coperand() const;
	    void FormatAddr(std::ostream&, ADDR) const;
	    void FormatInstr(std::ostream&) const;
	    MEMREFT Memreft(size_t) const;
	    TRMT Trmt() const;
	    DIS::TRMTA Trmta() const;

private:
   enum OPIT			       // Operand Insert Type
   {
      opitNone, 		       // No operand
      opitRegN, 		       // Destination register
      opitRegM, 		       // Source register
      opitImm,			       // Immediate addressing
      opitDispl,		       // Displacement
   };

   struct OPRND 		       // Operand
   {
      const char *szFormat;	       // Format for sprintf into opcode string
      OPIT	 opit1;
      OPIT	 opit2;
   };
  
   enum SHFT			       // Multiplier for displ/imm in instr
   {
      ZIP,
      DOUB,
      QUAD
   };

   struct OPCD			       // Structure describing the assembly statement
   {
      const char *szMnemonic;	       // mnemonic for opcode
      size_t rgioprnd[2];	       // Index into format array for operands
      size_t instrFormat;	       // instruction format as mapped to field valid
				       // in the m_iw class variable

      // manipulation info for displacement or immediate field
      // there at most 1 displacement or a immediate field, => info
      // can be stored here instead of the rgcls struct

      bool fSigned;		       // Is immediate value signed
      SHFT shift;		       // shift left, 0, 1, 2 (multiply 1, 2, 4)
   };
  

	    DWORD DwExtend(WORD, size_t) const;
	    DWORD DwOpValue(OPIT) const;
	    void getOperandStr(char *, int) const;
   static   const OPCD *PopcdDecode(IW);

   static   const OPRND rgoprnd[];

   static   const OPCD Op_1512[];
   static   const OPCD Op_0_30[];
   static   const OPCD Op_0_74_2[];
   static   const OPCD Op_0_74_3[];
   static   const OPCD Op_0_74_8[];
   static   const OPCD Op_0_74_9[];
   static   const OPCD Op_0_74_10[];
   static   const OPCD Op_0_74_11[];
   static   const OPCD Op_2_30[];
   static   const OPCD Op_3_30[];
   static   const OPCD Op_4_30[];
   static   const OPCD Op_4_74_0[];
   static   const OPCD Op_4_74_1[];
   static   const OPCD Op_4_74_2[];
   static   const OPCD Op_4_74_3[];
   static   const OPCD Op_4_74_4[];
   static   const OPCD Op_4_74_5[];
   static   const OPCD Op_4_74_6[];
   static   const OPCD Op_4_74_7[];
   static   const OPCD Op_4_74_8[];
   static   const OPCD Op_4_74_9[];
   static   const OPCD Op_4_74_10[];
   static   const OPCD Op_4_74_11[];
   static   const OPCD Op_4_74_14[];
   static   const OPCD * const Op_4[15];
   static   const OPCD Op_6_30[];
   static   const OPCD Op_8_118[];
   static   const OPCD Op_12_118[];
   static   const OPCD Op_15_30[];
   static   const OPCD Op_15_74_13[];
   static   const OPCD Op_15_98[];

   static   const long masks[4][2];    // masks for sign extension

	    IW m_iw;		       // instruction word
	    const OPCD *m_popcd;       // pointer into instrucion code array
};

#pragma pack(pop)
