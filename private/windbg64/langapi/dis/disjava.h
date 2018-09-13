/***********************************************************************
* Microsoft Disassembler
*
* Microsoft Confidential.  Copyright 1996-1997 Microsoft Corporation.
*
* Component:
*
* File: disjava.h
*
* File Comments:
*
*   This file is a copy of the master version owned by richards.
*   Contact richards for any changes.
*
***********************************************************************/

#pragma pack(push, 8)

class DISJAVA : public DIS
{
public:
   enum TRMTA
   {
      trmtaUnknown = DIS::trmtaUnknown,
      trmtaFallThrough = DIS::trmtaFallThrough,
      trmtaBra,
      trmtaBraCc,
      trmtaBraInd,
      trmtaCall,
   };


	    DISJAVA(DIST);
	    ~DISJAVA();

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

	    // UNDONE: What is this?

	    void SetClass(DWORD cls) { m_class = cls; }
	    DWORD GetClass() const { return m_class; }
	    void SetMethod(DWORD method) { m_method = method; }
	    DWORD GetMethod() const { return m_method; }
	    void SetPC(DWORD offPC) { m_offPC = offPC; }
	    DWORD GetPC() const { return m_offPC; }

private:
   enum OPCLS			       // Operand Class
   {
      opclsNone,		       // No operand
      opclsI1,			       // Signed  8-bit immediate value
      opclsI2,			       // Signed 16-bit immediate value
      opclsVar, 		       // Local variable slot number
      opclsVar0,		       // Local variable slot number 0
      opclsVar1,		       // Local variable slot number 1
      opclsVar2,		       // Local variable slot number 2
      opclsVar3,		       // Local variable slot number 3
      opclsJump,		       // Label offset
      opclsNMTP,		       // 16-bit constant pool index [name and type]
      opclsCPX1,		       //  8-bit constant pool index [numeric constant]
      opclsCPX2,		       // 16-bit constant pool index [numeric constant]
      opclsCLSTP,		       // 16-bit constant pool index [class type]
      opclsImm1,		       //  8-bit unsigned
      opclsInc, 		       //  8-bit signed
      opclsType,		       // Array type
      opclsWide,		       // Wide index upper byte
      opclsQField,		       // An indexed class field
   };

   enum ICLS			       // Instruction Class
   {
	   // Invalid Class

      iclsInvalid,

	   // Memory Class
	   //
	   // Text Format:	   JR	   rs
	   //
	   // Termination Type:    trmtaMipsBraIndDef
	   //
	   // Registers Used:	   Rs
	   // Registers Set:
	   //
	   // Constraints:	   Rd, Rt, and shift ammount must be zero

      iclsNone,
      iclsI1,
      iclsI2,
      iclsLoad,
      iclsLoad0,
      iclsLoad1,
      iclsLoad2,
      iclsLoad3,
      iclsStore,
      iclsStore0,
      iclsStore1,
      iclsStore2,
      iclsStore3,
      iclsBra,
      iclsBraW,
      iclsBraCc,
      iclsCall,
      iclsCallW,
      iclsRet,
      iclsRet2,
      iclsNMTP,
      iclsInvokeInterface,
      iclsCPX1,
      iclsCPX2,
      iclsCLSTP,
      iclsIinc,
      iclsType,
      iclsMultiANew,
      iclsWide,
      iclsSpecial,
      iclsQuickField,
   };

   struct OPCD
   {
      const char  *szMnemonic;
      BYTE	  icls;
   };

   struct CLS
   {
      BYTE	  trmta;
      BYTE	  cb;
      BYTE	  rgopcls[2];	       // Operand class for each operand
   };

   static   const TRMT mptrmtatrmt[];

   static   const CLS rgcls[];

   static   const OPCD rgopcd[];

   static   const char * const rgszType[8];

	    void FormatConst(std::ostream&, WORD) const;
	    void FormatOperand(std::ostream&, OPCLS opcls) const;
	    void FormatQuickField(std::ostream&, WORD) const;
	    void FormatVar(std::ostream&, WORD) const;

	    const BYTE *m_pbCur;
	    size_t m_cbMax;

	    size_t m_cb;
	    BYTE *m_rgbInstr;
	    size_t m_cbAlloc;

	    const struct OPCD *m_popcd;

	    // UNDONE: What is this?

	    DWORD m_class;
	    DWORD m_method;
	    DWORD m_offPC;
};

#pragma pack(pop)
