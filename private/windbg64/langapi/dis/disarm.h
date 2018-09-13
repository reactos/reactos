/***********************************************************************
* Microsoft Disassembler
*
* Microsoft Confidential.  Copyright 1994-1997 Microsoft Corporation.
*
* Component:
*
* File: disarm.h
*
* File Comments:
*
*   This file is a copy of the master version owned by richards.
*   Contact richards for any changes.
*
***********************************************************************/

#pragma pack(push, 8)

class DISARM : public DIS
{
public:
   enum TRMTA
   {
      trmtaUnknown = DIS::trmtaUnknown,
      trmtaFallThrough = DIS::trmtaFallThrough,
      trmtaBra,
      trmtaBraInd,
      trmtaBraCc,
      trmtaBraCcInd,
      trmtaCall,
      trmtaCallCc,
      trmtaCallInd,
      trmtaCallCcInd,
      trmtaTrap,
      trmtaTrapCc,
      trmtaBraCase,
      trmtaAfterCatch,
      trmtaBraIndMaybe_15_12,
      trmtaBraIndMaybe_19_16,
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
   };


   struct IW_MFS		       // move_from_status
   {
      DWORD mbz : 12;
      DWORD Rd : 4;
      DWORD mbo : 4;
      DWORD SL : 1; // = 0
      DWORD AW : 1; // = 0
      DWORD R : 1;
      DWORD sub : 5; // = 2
      DWORD cond : 4;
   };

   struct IW_MRTS		       // move_reg_to_status
   {
      DWORD Rm : 4;
      DWORD mbz : 8;
      DWORD mbo : 4;
      DWORD mask : 4;
      DWORD SL : 1; // = 0
      DWORD AW : 1; // = 1
      DWORD R : 1;
      DWORD sub : 5; // = 2
      DWORD cond : 4;
   };

   struct IW_BX 		       // branch_exch_instrset
   {
      DWORD Rm : 4;
      DWORD sub1 : 4; // = 1
      DWORD mbo : 12;
      DWORD sub : 8; // = 0x12
      DWORD cond : 4;
   };

   struct IW_SSB		       // swap_swap_byte
   {
      DWORD Rm : 4;
      DWORD sub1 : 4; // = 9
      DWORD mbz : 4;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD SL : 1; // = 0
      DWORD AW : 1; // = 0
      DWORD B : 1;
      DWORD sub2 : 2; // = 2
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_DPIS		       // data_proc_imm_shift
   {
      DWORD Rm : 4;
      DWORD mbz : 1;
      DWORD shifttype : 2;
      DWORD shiftamt : 5;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD S : 1;
      DWORD opcode : 4;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_DPISM		       // data_proc_imm_shift_move
   {
      DWORD Rm : 4;
      DWORD mbz : 1;
      DWORD shifttype : 2;
      DWORD shiftamt : 5;
      DWORD Rd : 4;
      DWORD mbzRn : 4;
      DWORD S : 1;
      DWORD opcode : 4;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_DPIST		       // data_proc_imm_shift_test
   {
      DWORD Rm : 4;
      DWORD mbz : 1;
      DWORD shifttype : 2;
      DWORD shiftamt : 5;
      DWORD mbzRd : 4;
      DWORD Rn : 4;
      DWORD mboS : 1;
      DWORD opcode : 4;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_DPRS		       // data_proc_reg_shift
   {
      DWORD Rm : 4;
      DWORD mbo : 1;
      DWORD shifttype : 2;
      DWORD mbz : 1;
      DWORD Rs : 4;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD S : 1;
      DWORD opcode : 4;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_DPRSM		       // data_proc_reg_shift_move
   {
      DWORD Rm : 4;
      DWORD mbo : 1;
      DWORD shifttype : 2;
      DWORD mbz : 1;
      DWORD Rs : 4;
      DWORD Rd : 4;
      DWORD mbzRn : 4;
      DWORD S : 1;
      DWORD opcode : 4;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_DPRST		       // data_proc_reg_shift_test
   {
      DWORD Rm : 4;
      DWORD mbo : 1;
      DWORD shifttype : 2;
      DWORD mbz : 1;
      DWORD Rs : 4;
      DWORD mbzRd : 4;
      DWORD Rn : 4;
      DWORD mboS : 1;
      DWORD opcode : 4;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_M			       // multiply
   {
      DWORD Rm : 4;
      DWORD sub1 : 4; // = 9
      DWORD Rs : 4;
      DWORD mbz : 4;
      DWORD Rd : 4;
      DWORD S : 1;
      DWORD A : 1; // = 0
      DWORD mbz1 : 1;
      DWORD sub2 : 2; // = 0
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_MA 		       // multiply_accum
   {
      DWORD Rm : 4;
      DWORD sub1 : 4; // = 9
      DWORD Rs : 4;
      DWORD Rn : 4;
      DWORD Rd : 4;
      DWORD S : 1;
      DWORD A : 1; // = 1
      DWORD mbz1 : 1;
      DWORD sub2 : 2; // = 0
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_ML 		       // multiply_long
   {
      DWORD Rm : 4;
      DWORD sub1 : 4; // = 9
      DWORD Rs : 4;
      DWORD RdLo : 4;
      DWORD RdHi : 4;
      DWORD S : 1;
      DWORD A : 1;
      DWORD U : 1;
      DWORD sub2 : 2; // = 1
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_LHBI		       // load_half_byte_imm
   {
      DWORD LoOff : 4;
      DWORD mbo1 : 1;
      DWORD H : 1;
      DWORD S : 1;
      DWORD mbo2 : 1;
      DWORD HiOff : 4;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD L : 1; // = 1
      DWORD W : 1;
      DWORD I : 1; // = 1
      DWORD U : 1;
      DWORD P : 1;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_LHBR		       // load_half_byte_reg
   {
      DWORD Rm : 4;
      DWORD mbo1 : 1;
      DWORD H : 1;
      DWORD S : 1;
      DWORD mbo2 : 1;
      DWORD mbz : 4;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD L : 1; // = 1
      DWORD W : 1;
      DWORD I : 1; // = 0
      DWORD U : 1;
      DWORD P : 1;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_SHBI		       // store_half_byte_imm
   {
      DWORD LoOff : 4;
      DWORD mbo1 : 1;
      DWORD H : 1;
      DWORD S : 1;
      DWORD mbo2 : 1;
      DWORD HiOff : 4;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD L : 1; // = 0
      DWORD W : 1;
      DWORD I : 1; // = 1
      DWORD U : 1;
      DWORD P : 1;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_SHBR		       // store_half_byte_reg
   {
      DWORD Rm : 4;
      DWORD mbo1 : 1;
      DWORD H : 1;
      DWORD S : 1;
      DWORD mbo2 : 1;
      DWORD mbz : 4;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD L : 1; // = 1
      DWORD W : 1;
      DWORD I : 1; // = 0
      DWORD U : 1;
      DWORD P : 1;
      DWORD sub : 3; // = 0
      DWORD cond : 4;
   };

   struct IW_MITS		       // move_imm_to_status
   {
      DWORD imm : 8;
      DWORD rot : 4;
      DWORD mbo : 4;
      DWORD mbz : 3;
      DWORD mbo1 : 1;
      DWORD SL : 1; // = 0
      DWORD AW : 1; // = 1
      DWORD R : 1;
      DWORD sub1 : 2; // = 2
      DWORD sub : 3; // = 1
      DWORD cond : 4;
   };

   struct IW_DPI		       // data_proc_immediate
   {
      DWORD imm : 8;
      DWORD rot : 4;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD S : 1;
      DWORD opcode : 4;
      DWORD sub : 3; // = 1
      DWORD cond : 4;
   };

   struct IW_DPIM		       // data_proc_immediate_move
   {
      DWORD imm : 8;
      DWORD rot : 4;
      DWORD Rd : 4;
      DWORD mbzRn : 4;
      DWORD S : 1;
      DWORD opcode : 4;
      DWORD sub : 3; // = 1
      DWORD cond : 4;
   };

   struct IW_DPIT		       // data_proc_immediate_test
   {
      DWORD imm : 8;
      DWORD rot : 4;
      DWORD mbzRd : 4;
      DWORD Rn : 4;
      DWORD mboS : 1;
      DWORD opcode : 4;
      DWORD sub : 3; // = 1
      DWORD cond : 4;
   };

   struct IW_LIO		       // ldst_imm_offset
   {
      DWORD imm : 12;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD L : 1;
      DWORD W : 1;
      DWORD B : 1;
      DWORD U : 1;
      DWORD P : 1;
      DWORD sub : 3; // = 2
      DWORD cond : 4;
   };

   struct IW_LRO		       // ldst_reg_offset
   {
      DWORD Rm : 4;
      DWORD mbz : 1;
      DWORD shifttype : 2;
      DWORD shiftamt : 5;
      DWORD Rd : 4;
      DWORD Rn : 4;
      DWORD L : 1;
      DWORD W : 1;
      DWORD B : 1;
      DWORD U : 1;
      DWORD P : 1;
      DWORD sub : 3; // = 3
      DWORD cond : 4;
   };

   struct IW_UI 		       // undefined_instr
   {
      DWORD any1 : 4;
      DWORD mbo : 1;
      DWORD any2 : 20;
      DWORD sub : 3; // = 3
      DWORD cond : 4;
   };

   struct IW_LM 		       // load_multiple
   {
      DWORD reglist : 16;
      DWORD Rn : 4;
      DWORD L : 1; // = 1
      DWORD W : 1;
      DWORD S : 1;
      DWORD U : 1;
      DWORD P : 1;
      DWORD sub : 3; // = 4
      DWORD cond : 4;
   };

   struct IW_SM 		       // store_multiple
   {
      DWORD reglist : 16;
      DWORD Rn : 4;
      DWORD L : 1; // = 0
      DWORD W : 1;
      DWORD S : 1;
      DWORD U : 1;
      DWORD P : 1;
      DWORD sub : 3; // = 4
      DWORD cond : 4;
   };

   struct IW_BB 		       // b_bl
   {
      DWORD offset : 24;
      DWORD L : 1;
      DWORD sub : 3; // = 5
      DWORD cond : 4;
   };

   struct IW_CL 		       // cp_ldst
   {
      DWORD imm : 8;
      DWORD cpnum : 4;
      DWORD CRd : 4;
      DWORD Rn : 4;
      DWORD L : 1;
      DWORD W : 1;
      DWORD N : 1;
      DWORD U : 1;
      DWORD P : 1;
      DWORD sub : 3; // = 6
      DWORD cond : 4;
   };

   struct IW_CDP		       // cp_data_proc
   {
      DWORD CRm : 4;
      DWORD mbz : 1;
      DWORD opcode2 : 3;
      DWORD cpnum : 4;
      DWORD CRd : 4;
      DWORD CRn : 4;
      DWORD opcode1 : 4;
      DWORD mbz1 : 1;
      DWORD sub : 3; // = 7
      DWORD cond : 4;
   };

   struct IW_CRX		       // cp_reg_xfer
   {
      DWORD CRm : 4;
      DWORD mbo : 1;
      DWORD opcode2 : 3;
      DWORD cpnum : 4;
      DWORD Rd : 4;
      DWORD CRn : 4;
      DWORD L : 1;
      DWORD opcode1 : 3;
      DWORD mbz : 1;
      DWORD sub : 3; // = 7
      DWORD cond : 4;
   };

   struct IW_S			       // swi
   {
      DWORD swinum : 24;
      DWORD mbo : 1;
      DWORD sub : 3; // = 7
      DWORD cond : 4;
   };

   union IW			       // Instruction Word
   {
      DWORD	dw;
      IW_MFS	MFS;		       // move_from_status
      IW_MRTS	MRTS;		       // move_reg_to_status
      IW_BX	BX;		       // branch_exch_instrset
      IW_SSB	SSB;		       // swap_swap_byte
      IW_DPIS	DPIS;		       // data_proc_imm_shift
      IW_DPISM	DPISM;		       // data_proc_imm_shift_move
      IW_DPIST	DPIST;		       // data_proc_imm_shift_test
      IW_DPRS	DPRS;		       // data_proc_reg_shift
      IW_DPRSM	DPRSM;		       // data_proc_reg_shift_move
      IW_DPRST	DPRST;		       // data_proc_reg_shift_test
      IW_M	M;		       // multiply
      IW_MA	MA;		       // multiply_accum
      IW_ML	ML;		       // multiply_long
      IW_LHBI	LHBI;		       // load_half_byte_imm
      IW_LHBR	LHBR;		       // load_half_byte_reg
      IW_SHBI	SHBI;		       // store_half_byte_imm
      IW_SHBR	SHBR;		       // store_half_byte_reg
      IW_MITS	MITS;		       // move_imm_to_status
      IW_DPI	DPI;		       // data_proc_immediate
      IW_DPIM	DPIM;		       // data_proc_immediate_move
      IW_DPIT	DPIT;		       // data_proc_immediate_test
      IW_LIO	LIO;		       // ldst_imm_offset
      IW_LRO	LRO;		       // ldst_reg_offset
      IW_UI	UI;		       // undefined_instr
      IW_LM	LM;		       // load_multiple
      IW_SM	SM;		       // store_multiple
      IW_BB	BB;		       // b_bl
      IW_CL	CL;		       // cp_ldst
      IW_CDP	CDP;		       // cp_data_proc
      IW_CRX	CRX;		       // cp_reg_xfer
      IW_S	S;		       // swi
   };


	    DISARM(DIST);

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
   enum ICLS		// Instruction Class
   {
      iclsInvalid,	// Invalid Class

      iclsMFS,		// move_from_status
      iclsMRTS, 	// move_reg_to_status
      iclsBX,		// branch_exch_instrset
      iclsSSB,		// swap_swap_byte
      iclsDPIS, 	// data_proc_imm_shift
      iclsDPISM,	// data_proc_imm_shift_move
      iclsDPIST,	// data_proc_imm_shift_test
      iclsDPRS, 	// data_proc_reg_shift
      iclsDPRSM,	// data_proc_reg_shift_move
      iclsDPRST,	// data_proc_reg_shift_test
      iclsM,		// multiply
      iclsMA,		// multiply_accum
      iclsML,		// multiply_long
      iclsLHBI, 	// load_half_byte_imm
      iclsLHBR, 	// load_half_byte_reg
      iclsSHBI, 	// store_half_byte_imm
      iclsSHBR, 	// store_half_byte_reg
      iclsMITS, 	// move_imm_to_status
      iclsDPI,		// data_proc_immediate
      iclsDPIM, 	// data_proc_immediate_move
      iclsDPIT, 	// data_proc_immediate_test
      iclsLIO,		// ldst_imm_offset
      iclsLRO,		// ldst_reg_offset
      iclsUI,		// undefined_instr
      iclsLM,		// load_multiple
      iclsSM,		// store_multiple
      iclsBB,		// b_bl
      iclsCL,		// cp_ldst
      iclsCDP,		// cp_data_proc
      iclsCRX,		// cp_reg_xfer
      iclsS,		// swi
   };

   enum FormatFlags
   {
      FMT_NO_FLAGS = 0,
      FMT_DOING_COMMENT = 1,
      FMT_NEXT = 2,
   };


   enum Regs
   {
      REG_PC = 15
   };

   static   const char * const cond_name[];
   static   const char * const coproc_regs[];
   static   const char * const data_proc_opcodes[];
   static   const char * const regs[];

   static   void do_comma(std::ostream&, int&);
   static   void do_semi(std::ostream&, int&, unsigned&);
	    DWORD DwBitfield(unsigned, unsigned) const;
	    const char *FormatCondText(std::ostream&, const char *, unsigned, int&, unsigned&) const;
	    const char *PchFormatPart(std::ostream&, const char *, int&, unsigned&) const;
	    const char *SzFindFormat() const;

	    IW m_iw;
	    const char *m_szFormat;
};

#pragma pack(pop)
