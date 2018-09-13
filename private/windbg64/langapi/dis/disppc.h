/***********************************************************************
* Microsoft Disassembler
*
* Microsoft Confidential.  Copyright 1994-1997 Microsoft Corporation.
*
* Component:
*
* File: disppc.h
*
* File Comments:
*
*   This file is a copy of the master version owned by richards.
*   Contact richards for any changes.
*
***********************************************************************/

#pragma pack(push, 8)

class DISPPC : public DIS
{
public:
   enum TRMTA
   {
      trmtaUnknown = DIS::trmtaUnknown,
      trmtaFallThrough = DIS::trmtaFallThrough,
      trmtaBra,
      trmtaBraInd,
      trmtaBraCc,
      trmtaBraCcR,
      trmtaBraCcInd,
      trmtaCall,
      trmtaCallCc,
      trmtaCallInd,
      trmtaCallCcInd,
      trmtaTrap,
      trmtaTrapCc,
      trmtaBraCase,
      trmtaAfterCatch,
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

      regaF0   = 32,
      regaF1   = 33,
      regaF2   = 34,
      regaF3   = 35,
      regaF4   = 36,
      regaF5   = 37,
      regaF6   = 38,
      regaF7   = 39,
      regaF8   = 40,
      regaF9   = 41,
      regaF10  = 42,
      regaF11  = 43,
      regaF12  = 44,
      regaF13  = 45,
      regaF14  = 46,
      regaF15  = 47,
      regaF16  = 48,
      regaF17  = 49,
      regaF18  = 50,
      regaF19  = 51,
      regaF20  = 52,
      regaF21  = 53,
      regaF22  = 54,
      regaF23  = 55,
      regaF24  = 56,
      regaF25  = 57,
      regaF26  = 58,
      regaF27  = 59,
      regaF28  = 60,
      regaF29  = 61,
      regaF30  = 62,
      regaF31  = 63,
   };


   struct IW_I
   {
      DWORD LK : 1;
      DWORD AA : 1;
      DWORD LI : 24;
      DWORD opcd : 6;
   };

   struct IW_B
   {
      DWORD LK : 1;
      DWORD AA : 1;
      DWORD BD : 14;
      DWORD BI : 5;
      DWORD BO : 5;
      DWORD opcd : 6;
   };

   struct IW_SC
   {
      DWORD mbz1 : 1;
      DWORD XO : 1;
      DWORD mbz2 : 14;
      DWORD mbz3 : 5;
      DWORD mbz4 : 5;
      DWORD opcd : 6;
   };

   struct IW_D
   {
      DWORD d : 16;		       // Also SIMM, UIMM
      DWORD rA : 5;
      DWORD rD : 5;		       // Also rS, frD, frS, TO, (crfD, 0, L)
      DWORD opcd : 6;
   };

   struct IW_DS
   {
      DWORD XO : 2;
      DWORD ds : 14;
      DWORD rA : 5;
      DWORD rD : 5;		       // Also rS
      DWORD opcd : 6;
   };

   struct IW_X
   {
      DWORD Rc : 1;
      DWORD XO : 10;
      DWORD rB : 5;		       // Also frB, SH, NB, (IMM, 0)
      DWORD rA : 5;		       // Also frA, (crfS, 0), (0, SR)
      DWORD rD : 5;		       // Also rS, frD, frS, (crfD, 0, L), TO, crbD
      DWORD opcd : 6;
   };

   struct IW_XL
   {
      DWORD LK : 1;
      DWORD XO : 10;
      DWORD crbB : 5;
      DWORD crbA : 5;		       // Also BI, (crfS, 0)
      DWORD crbD : 5;		       // Also BO, (crfD, 0)
      DWORD opcd : 6;
   };

   struct IW_XFX
   {
      DWORD Rc : 1;
      DWORD XO : 10;
      DWORD SPR : 10;		       // Also (0, CRM, 0), TBR
      DWORD rD : 5;		       // Also rS
      DWORD opcd : 6;
   };

   struct IW_XFL
   {
      DWORD Rc : 1;
      DWORD XO : 10;
      DWORD frB : 5;
      DWORD mbz1 : 1;
      DWORD FM : 8;
      DWORD mbz2 : 1;
      DWORD opcd : 6;
   };

   struct IW_XS
   {
      DWORD Rc : 1;
      DWORD SH5 : 1;
      DWORD xo : 9;
      DWORD SH : 5;
      DWORD rA : 5;
      DWORD rS : 5;
      DWORD opcd : 6;
   };

   struct IW_XO
   {
      DWORD Rc : 1;
      DWORD XO : 9;
      DWORD OE : 1;
      DWORD rB : 5;
      DWORD rA : 5;
      DWORD rD : 5;
      DWORD opcd : 6;
   };

   struct IW_A
   {
      DWORD Rc : 1;
      DWORD XO : 5;
      DWORD frC : 5;
      DWORD frB : 5;
      DWORD frA : 5;
      DWORD frD : 5;
      DWORD opcd : 6;
   };

   struct IW_M
   {
      DWORD Rc : 1;
      DWORD ME : 5;
      DWORD MB : 5;
      DWORD rB : 5;		       // Also SH
      DWORD rA : 5;
      DWORD rS : 5;
      DWORD opcd : 6;
   };

   struct IW_MD
   {
      DWORD Rc : 1;
      DWORD SH5 : 1;
      DWORD XO : 3;
      DWORD MB : 6;		       // Also ME
      DWORD SH : 5;
      DWORD rA : 5;
      DWORD rS : 5;
      DWORD opcd : 6;
   };

   struct IW_MDS
   {
      DWORD Rc : 1;
      DWORD XO : 4;
      DWORD MB : 6;		       // Also ME
      DWORD rB : 5;
      DWORD rA : 5;
      DWORD rS : 5;
      DWORD opcd : 6;
   };


   union IW			       // Instruction Word
   {
      DWORD    dw;

      IW_I     I;
      IW_B     B;
      IW_SC    SC;
      IW_D     D;
      IW_DS    DS;
      IW_X     X;
      IW_XL    XL;
      IW_XFX   XFX;
      IW_XFL   XFL;
      IW_XS    XS;
      IW_XO    XO;
      IW_A     A;
      IW_M     M;
      IW_MD    MD;
      IW_MDS   MDS;
   };


	    DISPPC(DIST);

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
   enum OPCLS			       // Operand Class
   {
      opclsNone,		       // No operand
      opcls_crfD,		       // cmpi	   crfD,L,rA,SIMM    (op 1)
      opclsI_LI,		       // b	   target	     (op 1)
      opclsB_BD,		       // bc	   BO,BI,target      (op 3)
      opclsB_BI,		       // "                          (op 2)
      opclsB_BO,		       // "                          (op 1)
      opclsB_CR,		       // ble	   cr,Target	     (op 1)
      opclsD_d_r,		       // lmw	   rD,d(rA)	     (op 2)
      opclsD_d_w,		       // stmw	   rS,d(rA)	     (op 2)
      opclsD_SIMM,		       // addi	   rD,rA,SIMM	     (op 3)
      opclsD_UIMM,		       // andi.    rA,rS,UIMM	     (op 3)
      opclsD_rA,		       // "                          (op 1)
      opclsD_rD,		       // lmw	   rD,d(rA)	     (op 1)
      opclsD_rS,		       // andi.    rA,rS,UIMM	     (op 2)
      opclsD_frS,		       // stfd	   frS,d(rA)	     (op 1)
      opclsD_frD,		       // lfd	   frD,d(rA)	     (op 1)
      opclsD_TO,		       // twi	   TO,rA,SIMM	     (op 1)
      opclsD_L, 		       // "                          (op 2)
      opclsDS_ds_r,		       // ld	   rD,ds(rA)	     (op 2)
      opclsDS_ds_w,		       // std	   rS,ds(rA)	     (op 2)
      opclsDS_rD,		       // "                          (op 1)
      opclsDS_rS,		       // std	   rS,ds(rA)	     (op 1)
      opclsX_rB,		       // cmp	   crfD,L,rA,rB      (op 4)
      opclsX_frB,		       // fcmpu    crfD,frA,frB      (op 3)
      opclsX_SH,		       // srawi    rA,rS,SH	     (op 3)
      opclsX_NB,		       // lswi	   rD,rA,NB	     (op 3)
      opclsX_IMM,		       // mtfsfi   crfD,IMM	     (op 2)
      opclsX_rA,		       // cmp	   crfD,L,rA,rB      (op 3)
      opclsX_frA,		       // fcmpu    crfD,frA,frB      (op 2)
      opclsX_crfS,		       // mcrfs    crfD,crfS	     (op 2)
      opclsX_SR,		       // mtsr	   SR,rS	     (op 1)
      opclsX_rD,		       // lbzux    rD,rA,rB	     (op 1)
      opclsX_rS,		       // and	   rA,rS,rB	     (op 2)
      opclsX_frD,		       // lfdux    frD,rA,rB	     (op 1)
      opclsX_frS,		       // stfdux   frS,rA,rB	     (op 1)
      opclsX_L, 		       // cmp	   crfD,L,rA,rB      (op 2)
      opclsX_TO,		       // tw	   TO,rA,rB	     (op 1)
      opclsX_crbD,		       // mtfsb0   crbD 	     (op 1)
      opclsXL_crbB,		       // crand    crbD,crbA,crbB    (op 3)
      opclsXL_crbA,		       // "                          (op 2)
      opclsXL_BI,		       // bcctr    BO,BI	     (op 2)
      opclsXL_crfS,		       // mcrf	   crfD,crfS	     (op 2)
      opclsXL_crbD,		       // crand    crbD,crbA,crbB    (op 1)
      opclsXL_BO,		       // bcctr    BO,BI	     (op 1)
      opclsXL_CR,		       // blectr   cr		     (op 1)
      opclsXFX_SPR,		       // mfspr    rD,SPR	     (op 2)
      opclsXFX_CRM,		       // mtcrf    CRM,rS	     (op 1)
      opclsXFX_TBR,		       // mftb	   rD,TBR	     (op 2)
      opclsXFX_rD,		       // mfspr    rD,SPR	     (op 1)
      opclsXFX_rS,		       // mtcrf    CRM,rS	     (op 2)
      opclsXFL_frB,		       // mtfsf    FM,frB	     (op 2)
      opclsXFL_FM,		       // "                          (op 1)
      opclsXS_SH,		       // sradi    rA,rS,SH	     (op 3)
      opclsXS_rA,		       // "                          (op 1)
      opclsXS_rS,		       // "                          (op 2)
      opclsXO_rB,		       // add	   rD,rA,rB	     (op 3)
      opclsXO_rA,		       // "                          (op 2)
      opclsXO_rD,		       // "                          (op 1)
      opclsA_frC,		       // fmadd    frD,frA,frC,frB   (op 3)
      opclsA_frB,		       // "                          (op 4)
      opclsA_frA,		       // "                          (op 2)
      opclsA_frD,		       // "                          (op 1)
      opclsM_ME,		       // rlwnm    rA,rS,rB,MB,ME    (op 5)
      opclsM_MB,		       // "                          (op 4)
      opclsM_rB,		       // "                          (op 3)
      opclsM_SH,		       // rlwimi   rA,rS,SH,MB,ME    (op 3)
      opclsM_rA,		       // "                          (op 1)
      opclsM_rS,		       // "                          (op 2)
      opclsMD_MB,		       // rldic    rA,rS,SH,MB	     (op 4)
      opclsMD_ME,		       // rldicr   rA,rS,SH,MB	     (op 4)
      opclsMD_SH,		       // "                          (op 3)
      opclsMD_rS,		       // "                          (op 2)
      opclsMD_rA,		       // "                          (op 1)
      opclsMDS_MB,		       // rldcl    rA,rS,rB,MB	     (op 4)
      opclsMDS_ME,		       // rldcr    rA,rS,rB,ME	     (op 4)
      opclsMDS_rB,		       // "                          (op 3)
      opclsMDS_rS,		       // "                          (op 2)
      opclsMDS_rA,		       // "                          (op 1)
   };

   enum ICLS			       // Instruction Class
   {
	   // Invalid Class

      iclsInvalid,

	   // Memory Class
	   //
	   // Text Format:	   JR	   rs
	   //
	   // Termination Type:    trmtaBraIndDef
	   //
	   // Registers Used:	   Rs
	   // Registers Set:
	   //
	   // Constraints:	   Rd, Rt, and shift ammount must be zero

      iclsA_1,
      iclsA_2,
      iclsA_3,
      iclsA_4,
      iclsD_1,
      iclsD_2,
      iclsD_3,
      iclsD_4,
      iclsD_5,
      iclsD_6,
      iclsD_7,
      iclsD_8,
      iclsD_9,
      iclsD_10,
      iclsD_11,
      iclsD_12,
      iclsD_13,
      iclsD_14,
      iclsD_15,
      iclsD_16,
      iclsDS_1,
      iclsDS_2,
      iclsDS_3,
      iclsDS_4,
      iclsBc,
      iclsSc,
      iclsB,
      iclsM_1,
      iclsMD_1,
      iclsMD_2,
      iclsX_1,
      iclsX_2,
      iclsX_3,
      iclsX_4,
      iclsX_5,
      iclsX_6,
      iclsX_7,
      iclsX_8,
      iclsX_9,
      iclsX_10,
      iclsX_11,
      iclsX_12,
      iclsX_13,
      iclsX_14,
      iclsX_15,
      iclsX_16,
      iclsX_17,
      iclsX_18,
      iclsX_19,
      iclsX_20,
      iclsX_21,
      iclsX_22,
      iclsX_23,
      iclsX_24,
      iclsX_25,
      iclsX_26,
      iclsX_27,
      iclsX_28,
      iclsX_29,
      iclsX_30,
      iclsX_31,
      iclsX_32,
      iclsX_33,
      iclsX_34,
      iclsX_35,
      iclsX_36,
      iclsX_37,
      iclsX_38,
      iclsXFL_1,
      iclsXFX_1,
      iclsXFX_2,
      iclsXFX_3,
      iclsXFX_4,
      iclsXFX_5,
      iclsXO_1,
      iclsXO_2,
      iclsXL_1,
      iclsBclr,
      iclsXL_3,
      iclsXL_4,
      iclsXL_5,
      iclsXL_6,
      iclsBcctr,
      iclsXS_1,
      iclsBc2,
      iclsBc3,
      iclsBc4,
      iclsBc5,
      iclsBc6,
      iclsTwi2,
      iclsTw2,
   };

   struct OPCD
   {
      const char  *szMnemonic;
      BYTE	  icls;
   };

   struct CLS
   {
      BYTE	  trmta;
      BYTE	  rgopcls[5];	       // Operand class for each operand
   };

   enum SPRREG
   {
      sprregMq	     = 0,
      sprregXer      = 1,
      sprregRtcu     = 4,
      sprregRtcl     = 5,
      sprregLr	     = 8,
      sprregCtr      = 9,
      sprregDsisr    = 18,
      sprregDar      = 19,
      sprregDec      = 22,
      sprregSdr1     = 25,
      sprregSrr0     = 26,
      sprregSrr1     = 27,
      sprregSprg0    = 272,
      sprregSprg1    = 273,
      sprregSprg2    = 274,
      sprregSprg3    = 275,
      sprregAsr      = 280,
      sprregEar      = 282,
      sprregTbl      = 284,	       // 604 (UNDONE: 603?)
      sprregTbu      = 285,	       // 604 (UNDONE: 603?)
      sprregPvr      = 287,
      sprregIbat0u   = 528,
      sprregIbat0l   = 529,
      sprregIbat1u   = 530,
      sprregIbat1l   = 531,
      sprregIbat2u   = 532,
      sprregIbat2l   = 533,
      sprregIbat3u   = 534,
      sprregIbat3l   = 535,
      sprregDbat0u   = 536,	       // 604
      sprregDbat0l   = 537,	       // 604
      sprregDbat1u   = 538,	       // 604
      sprregDbat1l   = 539,	       // 604
      sprregDbat2u   = 540,	       // 604
      sprregDbat2l   = 541,	       // 604
      sprregDbat3u   = 542,	       // 604
      sprregDbat3l   = 543,	       // 604
      sprregMmcr0    = 952,	       // 604
      sprregPmc1     = 953,	       // 604
      sprregPmc2     = 954,	       // 604
      sprregSia      = 955,	       // 604
      sprregSda      = 959,	       // 604
      sprregDmiss    = 976,	       // 603
      sprregImiss    = 980,	       // 603
      sprregIcmp     = 981,	       // 603
      sprregRpa      = 982,	       // 603
      sprregHid0     = 1008,	       // 601, 603 and 604
      sprregHid1     = 1009,
      sprregHid2     = 1010,	       // 601, 603 and 604 (iabr)
      sprregHid5     = 1013,	       // 601 and 604 (dabr)
      sprregHid15    = 1023,	       // 604 (pir)
   };

   struct SPRMAP
   {
      SPRREG	  sprreg;
      const char  *szName;
   };

   static   const TRMT mptrmtatrmt[];

   static   const CLS rgcls[];

   static   const OPCD rgopcd[];
   static   const OPCD * const rgrgopcd13[];
   static   const OPCD rgopcd13_00[];
   static   const OPCD rgopcd13_01[];
   static   const OPCD rgopcd13_10[];
   static   const OPCD rgopcd13_12[];
   static   const OPCD rgopcd13_16[];
   static   const OPCD rgopcd1E[];
   static   const OPCD * const rgrgopcd1F[];
   static   const OPCD rgopcd1F_00[];
   static   const OPCD rgopcd1F_04[];
   static   const OPCD rgopcd1F_08[];
   static   const OPCD rgopcd1F_09[];
   static   const OPCD rgopcd1F_0A[];
   static   const OPCD rgopcd1F_0B[];
   static   const OPCD rgopcd1F_10[];
   static   const OPCD rgopcd1F_12[];
   static   const OPCD rgopcd1F_13[];
   static   const OPCD rgopcd1F_14[];
   static   const OPCD rgopcd1F_15[];
   static   const OPCD rgopcd1F_16[];
   static   const OPCD rgopcd1F_17[];
   static   const OPCD rgopcd1F_18[];
   static   const OPCD rgopcd1F_19[];
   static   const OPCD rgopcd1F_1A[];
   static   const OPCD rgopcd1F_1B[];
   static   const OPCD rgopcd1F_1C[];
   static   const OPCD rgopcd1F_1D[];
   static   const OPCD rgopcd3A[];
   static   const OPCD rgopcd3B[];
   static   const OPCD rgopcd3E[];
   static   const OPCD rgopcd3F[];
   static   const OPCD * const rgrgopcd3F[];
   static   const OPCD rgopcd3F_00[];
   static   const OPCD rgopcd3F_06[];
   static   const OPCD rgopcd3F_07[];
   static   const OPCD rgopcd3F_08[];
   static   const OPCD rgopcd3F_0C[];
   static   const OPCD rgopcd3F_0E[];
   static   const OPCD rgopcd3F_0F[];

   static   const char * const szBIFalse[4];
   static   const char * const szBITrue[4];

   static   const char * const szBO[];

   static   const char * const szTO[];

   static   const OPCD opcdLi;
   static   const OPCD opcdLis;
   static   const OPCD opcdNop;
   static   const OPCD opcdNot;
   static   const OPCD opcdMr;
   static   const OPCD opcdTrap;

   static   const DWORD dwValidBO;
   static   const DWORD dwValidBO_CTR;

   static   const SPRMAP rgsprmap[];
   static   const size_t csprmap;

	    void FormatHex(std::ostream&, DWORD) const;
	    void FormatOperand(std::ostream&, OPCLS opcls) const;
	    void FormatRegRel(std::ostream&, REGA, DWORD) const;
	    bool FValidOperand(size_t) const;
   static   const OPCD *PopcdDecode(IW);
	    const OPCD *PopcdPseudoOp(OPCD *, char *) const;

	    IW m_iw;
	    const OPCD *m_popcd;
};

#pragma pack(pop)
