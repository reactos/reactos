/***********************************************************************
* Microsoft Disassembler
*
* Microsoft Confidential.  Copyright 1994-1997 Microsoft Corporation.
*
* Component:
*
* File: dismips.h
*
* File Comments:
*
*   This file is a copy of the master version owned by richards.
*   Contact richards for any changes.
*
***********************************************************************/

#pragma pack(push, 8)

class DISMIPS : public DIS
{
public:
   enum TRMTA
   {
      trmtaUnknown = DIS::trmtaUnknown,
      trmtaFallThrough = DIS::trmtaFallThrough,
      trmtaBraInd,
      trmtaCallInd,
      trmtaTrap,
      trmtaTrapCc,
      trmtaBraDef,
      trmtaBraIndDef,
      trmtaBraCcDef,
      trmtaBraCcLikely,
      trmtaCallDef,
      trmtaCallIndDef,
      trmtaCallCcDef,
      trmtaCallCcLikely,
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

   union IW                // Instruction Word
   {
      DWORD    dw;

      struct
      {
     DWORD Target : 26;
     DWORD Opcode : 6;
      } j_format;

      struct
      {
     DWORD Uimmediate : 16;
     DWORD Rt : 5;
     DWORD Rs : 5;
     DWORD Opcode : 6;
      } u_format;

      struct
      {
     DWORD Function : 6;
     DWORD Re : 5;
     DWORD Rd : 5;
     DWORD Rt : 5;
     DWORD Rs : 5;
     DWORD Opcode : 6;
      } r_format;

      struct
      {
     DWORD Function : 6;
     DWORD Re : 5;
     DWORD Rd : 5;
     DWORD Rt : 5;
     DWORD Format : 4;
     DWORD Fill1 : 1;
     DWORD Opcode : 6;
      } f_format;

      struct
      {
     DWORD Function : 6;
     DWORD Fd : 5;
     DWORD Fs : 5;
     DWORD Ft : 5;
     DWORD Format : 4;
     DWORD Fill1 : 1;
     DWORD Opcode : 6;
      } c_format;

      struct
      {
     DWORD Function : 6;
     DWORD Vd : 5;
     DWORD Vs : 5;
     DWORD Vt : 5;
     DWORD Fmt_sel : 5;
     DWORD Opcode : 6;
      } v_format;
   };


        DISMIPS(DIST);

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
   enum OPCLS                  // Operand Class
   {
      opclsNone,               // No operand
      opclsRegRs,              // General purpose register Rs
      opclsRegRt,              // General purpose register Rt
      opclsRegRd,              // General purpose register Rd
      opclsImmRt,              // Immediate value of Rt
      opclsImmRe,              // Immediate value of Re
      opclsImm,                // Immediate value
      opclsMem,                // Memory reference
      opclsMem_w,              // Memory reference
      opclsMem_r,              // Memory reference
      opclsCc1,                // Floating point condition code
      opclsCc2,                // Floating point condition code
      opclsAddrBra,            // Branch instruction target
      opclsAddrJmp,            // Jump instruction target
      opclsCprRt,              // Coprocessor general register Rt
      opclsCprRd,              // Coprocessor general register Rd
      opclsRegFr,              // Floating point general register Fr
      opclsRegFs,              // Floating point general register Fs
      opclsRegFt,              // Floating point general register Ft
      opclsRegFd,              // Floating point general register Fd
      opclsIndex,              // Index based reference
      opclsRegVs,              // Vector register Vs
      opclsRegVt,              // Vector register Vt
      opclsRegVd,              // Vector register Vd
      opclsImmV,               // Immediate value low three bits of Format
   };

   enum ICLS                   // Instruction Class
   {
       // Invalid Class

      iclsInvalid,

       // Immediate Class
       //
       // Text Format:     ADDIU   rt,rs,immediate
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs
       // Registers Set:       Rt

      iclsImmediate,

       // Immediate Class
       //
       // Text Format:     ADDI    rt,rs,immediate
       //
       // Termination Type:    trmtaTrapCc
       //
       // Registers Used:      Rs
       // Registers Set:       Rt

      iclsImmTrapCc,

       // Immediate (BraCc-1) Class
       //
       // Text Format:     BEQ     rt,rs,Target
       //
       // Termination Type:    trmtaBraCcDef
       //
       // Registers Used:      Rs, Rt
       // Registers Set:

      iclsImmBraCc1,

       // Immediate (BraCc-2) Class
       //
       // Text Format:     BEQL    rt,rs,Target
       //
       // Termination Type:    trmtaBraCcLikely
       //
       // Registers Used:      Rs, Rt
       // Registers Set:

      iclsImmBraCc2,

       // Immediate (BraCc-3) Class
       //
       // Text Format:     BGTZ    rs,Target
       //
       // Termination Type:    trmtaBraCcDef
       //
       // Registers Used:      Rs
       // Registers Set:
       //
       // Constraints:     Rt must be zero

      iclsImmBraCc3,

       // Immediate (BraCc-4) Class
       //
       // Text Format:     BGTZL   rs,Target
       //
       // Termination Type:    trmtaBraCcLikely
       //
       // Registers Used:      Rs
       // Registers Set:
       //
       // Constraints:     Rt must be zero

      iclsImmBraCc4,

       // Immediate (BraCc-5) Class
       //
       // Text Format:     BGEZ    rs,Target
       //
       // Termination Type:    trmtaBraCcDef
       //
       // Registers Used:      Rs
       // Registers Set:
       //
       // Note:        The Rt field is a function code

      iclsImmBraCc5,

       // Immediate (BraCc-6) Class
       //
       // Text Format:     BGEZL   rs,Target
       //
       // Termination Type:    trmtaBraCcLikely
       //
       // Registers Used:      Rs
       // Registers Set:
       //
       // Note:        The Rt field is a function code

      iclsImmBraCc6,

       // Immediate (CallCc-1) Class
       //
       // Text Format:     BGEZAL  rs,Target
       //
       // Termination Type:    trmtaCallCcDef
       //
       // Registers Used:      Rs
       // Registers Set:       R31
       //
       // Note:        The Rt field is a function code

      iclsImmCallCc1,

       // Immediate (CallCc-2) Class
       //
       // Text Format:     BGEZALL rs,Target
       //
       // Termination Type:    trmtaCallCcLikely
       //
       // Registers Used:      Rs
       // Registers Set:       R31
       //
       // Note:        The Rt field is a function code

      iclsImmCallCc2,

       // Immediate (Performance) Class
       //
       // Text Format:     CACHE   op,offset(rs)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs
       // Registers Set:
       //
       // Note:        The Rt field stores the op parm

      iclsImmPerf,

       // Immediate (Load) Class
       //
       // Text Format:     LB      rt,offset(rs)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs
       // Registers Set:       Rt

      iclsImmLoad,

       // Immediate (Load Coprocessor) Class
       //
       // Text Format:     LDC0    rt,offset(rs)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs
       // Registers Set:       Coprocessor general register Rt

      iclsImmLoadCp,

       // Immediate (LUI) Class
       //
       // Text Format:     LUI     rt,immediate
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:
       // Registers Set:       Rt
       //
       // Note:        The Rs field is unused (UNDONE: Must be zero?)

      iclsImmLui,

       // Immediate (Store) Class
       //
       // Text Format:     SB      rt,offset(rs)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs, Rt
       // Registers Set:

      iclsImmStore,

       // Immediate (SC) Class
       //
       // Text Format:     SC      rt,offset(rs)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs, Rt
       // Registers Set:       Rt

      iclsImmSc,

       // Immediate (Store Coprocessor) Class
       //
       // Text Format:     SDC0    rt,offset(rs)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs, Coprocessor general register Rt
       // Registers Set:

      iclsImmStoreCp,

       // Immediate (Trap) Class
       //
       // Text Format:     TEQI    rs,immediate
       //
       // Termination Type:    trmtaTrapCc
       //
       // Registers Used:      Rs
       // Registers Set:
       //
       // Note:        The Rt field is a function code

      iclsImmTrap,

       // Jump Class
       //
       // Text Format:     J       Target
       //
       // Termination Type:    trmtaBraDef
       //
       // Registers Used:
       // Registers Set:

      iclsJump,

       // Jump (JAL) Class
       //
       // Text Format:     JAL     Target
       //
       // Termination Type:    trmtaCallDef
       //
       // Registers Used:
       // Registers Set:       R31

      iclsJumpJal,

       // Register Class
       //
       // Text Format:     ADDU    rd,rs,rt
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs, Rt
       // Registers Set:       Rd
       //
       // Constraints:     Shift ammount must be zero

      iclsRegister,

       // Register Class with Condition Code
       //
       // Text Format:     MOVF    rd,rs,cc
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs
       // Registers Set:       Rd
       //
       // Constraints:     cc represents

      iclsRegisterCc,

       // Register Class
       //
       // Text Format:     ADD     rd,rs,rt
       //
       // Termination Type:    trmtaTrapCc
       //
       // Registers Used:      Rs, Rt
       // Registers Set:       Rd
       //
       // Constraints:     Shift ammount must be zero

      iclsRegTrapCc,

       // Register (BREAK) Class
       //
       // Text Format:     BREAK   immediate
       //
       // Termination Type:    trmtaTrap
       //
       // Registers Used:
       // Registers Set:
       //
       // Note:        MIPS does not use an operand for the immediate

      iclsRegBreak,

       // Register (JALR) Class
       //
       // Text Format:     JALR    rd,rs
       //
       // Termination Type:    trmtaCallInd
       //
       // Registers Used:      Rs
       // Registers Set:       Rd
       //
       // Constraints:     Rt and shift ammount must be zero

      iclsRegJalr,

       // Register (JR) Class
       //
       // Text Format:     JR      rs
       //
       // Termination Type:    trmtaBraIndDef
       //
       // Registers Used:      Rs
       // Registers Set:
       //
       // Constraints:     Rd, Rt, and shift ammount must be zero

      iclsRegJr,

       // Register (MFHI) Class
       //
       // Text Format:     MFHI    rd
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      HI
       // Registers Set:       Rd
       //
       // Constraints:     Rs, Rt, and shift ammount must be zero

      iclsRegMfhi,

       // Register (MFLO) Class
       //
       // Text Format:     MFLO    rd
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      LO
       // Registers Set:       Rd
       //
       // Constraints:     Rs, Rt, and shift ammount must be zero

      iclsRegMflo,

       // Register (MTHI) Class
       //
       // Text Format:     MTHI    rs
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs
       // Registers Set:       HI
       //
       // Constraints:     Rt, Rd, and shift ammount must be zero

      iclsRegMthi,

       // Register (MTLO) Class
       //
       // Text Format:     MTLO    rs
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs
       // Registers Set:       LO
       //
       // Constraints:     Rt, Rd, and shift ammount must be zero

      iclsRegMtlo,

       // Register (Multiply-Divide) Class
       //
       // Text Format:     DDIV    rs,rt
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs, Rt
       // Registers Set:       HI, LO
       //
       // Constraints:     Rd and shift ammount must be zero

      iclsRegMulDiv,

       // Register (Shift) Class
       //
       // Text Format:     DSLL    rd,rt,sa
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rt
       // Registers Set:       Rd
       //
       // Constraints:     The Rs field must be zero

      iclsRegShift,

       // Register (Shift Variable) Class
       //
       // Text Format:     DSLLV   rd,rt,rs
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs, Rt
       // Registers Set:       Rd
       //
       // Constraints:     Shift ammount must be zero

      iclsRegShiftVar,

       // Register (SYNC) Class
       //
       // Text Format:     SYNC
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:
       // Registers Set:
       //
       // Constraints:     Rs, Rt, Rd, and shift ammount must be zero

      iclsRegSync,

       // Register (SYSCALL) Class
       //
       // Text Format:     SYSCALL
       //
       // Termination Type:    trmtaTrap
       //
       // Registers Used:
       // Registers Set:
       //
       // Constraints:     Rs, Rt, Rd, and shift ammount must be zero

      iclsRegSyscall,

       // Register (Trap) Class
       //
       // Text Format:     TEQ     rs,rt,immediate
       //
       // Termination Type:    trmtaTrapCc
       //
       // Registers Used:      Rs, Rt
       // Registers Set:
       //
       // Note:        Rd and shift ammount contain the immediate
       // Note:        MIPS does not use an operand for the immediate

      iclsRegTrap,

       // Immediate (BraCc-7) Class
       //
       // Coprocessor
       //
       // Text Format:     BCzF    cc,Target
       //
       // Termination Type:    trmtaBraCcDef
       //
       // Registers Used:
       // Registers Set:
       //
       // Note:        The coprocessor z condition is referenced
       // Note:        The Rs and Rt fields are function codes
       // Note:        The coprocessor must be set in the mnemonic

      iclsImmBraCc7,

       // Immediate (BraCc-8) Class
       //
       // Coprocessor
       //
       // Text Format:     BCzF    Target
       //
       // Termination Type:    trmtaBraCcDef
       //
       // Registers Used:
       // Registers Set:
       //
       // Note:        The coprocessor z condition is referenced
       // Note:        The Rs and Rt fields are function codes
       // Note:        The coprocessor must be set in the mnemonic

      iclsImmBraCc8,

       // Register (CFCz) Class
       //
       // Coprocessor
       //
       // Text Format:     CFCz    rt,rd
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor control register Rd
       // Registers Set:       Rt
       //
       // Constraints:     Shift ammount and function must be zero
       //
       // Note:        The coprocessor must be set in the mnemonic

      iclsRegCfc,

       // Register (CTCz) Class
       //
       // Coprocessor
       //
       // Text Format:     CTCz    rt,rd
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rt
       // Registers Set:       Coprocessor control register Rd
       //
       // Constraints:     Shift ammount and function must be zero
       //
       // Note:        The coprocessor must be set in the mnemonic

      iclsRegCtc,

       // Register (MFCz) Class
       //
       // Coprocessor
       //
       // Text Format:     DMFCz   rt,rd
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor general register Rd
       // Registers Set:       Rt
       //
       // Constraints:     Shift ammount and function must be zero
       //
       // Note:        The coprocessor must be set in the mnemonic

      iclsRegMfc,

       // Register (MTCz) Class
       //
       // Coprocessor
       //
       // Text Format:     DMTCz   rt,rd
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rt
       // Registers Set:       Coprocessor general register Rd
       //
       // Constraints:     Shift ammount and function must be zero
       //
       // Note:        The coprocessor must be set in the mnemonic

      iclsRegMtc,

       // Register (Cp0) Class
       //
       // Coprocessor
       //
       // Text Format:     TLBP
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:
       // Registers Set:
       //
       // Constraints:     This is valid for coprocessor 0 only
       // Constraints:     Rs must be 10000b
       // Constraints:     Rt, Rd, and shift ammount must be zero
       //
       // Note:        The coprocessor must be set in the mnemonic

      iclsRegCp0,

       // Register (ERET) Class
       //
       // Coprocessor
       //
       // Text Format:     ERET
       //
       // Termination Type:    trmtaBraInd
       //
       // Registers Used:
       // Registers Set:
       //
       // Constraints:     This is valid for coprocessor 0 only
       // Constraints:     Rs must be 10000b
       // Constraints:     Rt, Rd, and shift ammount must be zero
       //
       // Note:        The coprocessor must be set in the mnemonic

      iclsRegEret,

       // Register (Float-1) Class
       //
       // Coprocessor
       //
       // Text Format:     ADD.S  fd,fs,ft
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor general registers Fs and Ft
       // Registers Set:       Coprocessor general register Fd
       //
       // Constraints:     Format must be Single or Double

      iclsRegFloat1,

       // Register (Float-2) Class
       //
       // Coprocessor
       //
       // Text Format:     SQRT.S fd,fs
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor general register Fs
       // Registers Set:       Coprocessor general register Fd
       //
       // Constraints:     Format must be Single or Double
       // Constraints:     Ft must be zero

      iclsRegFloat2,

       // Register (Float-3) Class
       //
       // Coprocessor
       //
       // Text Format:     MOV.S  fd,fs
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor general registers Fs
       // Registers Set:       Coprocessor general register Fd
       //
       // Constraints:     Format must be Single or Double or Word
       // Constraints:     Ft must be zero

      iclsRegFloat3,

       // Register (Float-4) Class
       //
       // Coprocessor
       //
       // Text Format:     CVT.S  fd,fs
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor general registers Fs
       // Registers Set:       Coprocessor general register Fd
       //
       // Constraints:     Format must be Double or Word
       // Constraints:     Ft must be zero

      iclsRegFloat4,

       // Register (Float-5) Class
       //
       // Coprocessor
       //
       // Text Format:     CVT.D  fd,fs
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor general registers Fs
       // Registers Set:       Coprocessor general register Fd
       //
       // Constraints:     Format must be Single or Word
       // Constraints:     Ft must be zero

      iclsRegFloat5,

       // Register (Float-6) Class
       //
       // Coprocessor
       //
       // Text Format:     C.F.S  cc,fs,ft
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor general register Fs
       // Registers Set:       Coprocessor general register Fd
       //
       // Constraints:     Format must be Single or Double
       // Constraints:     Fd must be zero

      iclsRegFloat6,

       // Register (Float-7) Class
       //
       // Coprocessor
       //
       // Text Format:     MOVN.S fd,fs,rt
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rt, Coprocessor general register Fs
       // Registers Set:       Coprocessor general register Fd
       //
       // Constraints:     UNDONE

      iclsRegFloat7,

       // Register (Float-8) Class
       //
       // Coprocessor
       //
       // Text Format:     MOVF.S fd,fs,cc
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor general registers Fs
       // Registers Set:       Coprocessor general register Fd
       //
       // Constraints:     UNDONE

      iclsRegFloat8,

       // Register (Float-9) Class
       //
       // Coprocessor
       //
       // Text Format:     C.F.S  fs,ft
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Coprocessor general register Fs
       // Registers Set:       Coprocessor general register Fd
       //
       // Constraints:     Format must be Single or Double
       // Constraints:     Fd must be zero

      iclsRegFloat9,

       // Register (Float) Class with Cc Trap termination
       //
       // Text Format:     MADD   fd,fr,fs,ft
       //
       // Termination Type:    trmtaTrapCc
       //
       // Registers Used:      Fs, Ft, Fr
       // Registers Set:       Fd
       //
       // Constraints:

      iclsRegFloat10,

       // Register (Float) Class with Cc Trap termination
       //
       // Text Format:     ALNV   rs,ft,fs,fd
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Fs, Ft, Rs
       // Registers Set:       Fd
       //
       // Constraints:

      iclsRegAlnv,

       // Index Prefetched Class
       //
       // Text Format:     PREFX  hint,index(rs)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      Rs
       // Registers Set:
       //
       // Note:        The Rd field stores the hint parm
       //              The Rt field stores the index parm

      iclsIndexPref,

       // Index Load Class
       //
       // Text Format:     LDXC1  fd,index(rs)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      rs, fd
       // Registers Set:
       //
       // Note:        The Rt field stores the index parm

      iclsIndexLoad,

       // Index Store Class
       //
       // Text Format:     SDXC1  fs,index(rs)
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      rs, fs
       // Registers Set:
       //
       // Note:        The Rt field stores the index parm

      iclsIndexStore,

       // Vector Class
       //
       // Text Format:     ADD.QH vd,vs,vt
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      vs, vt
       // Registers Set:       vd

      iclsVector,

       // Vector Immediate Class
       //
       // Text Format:     ALNI.OB vd,vs,vt,imm
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      vs, vt
       // Registers Set:       vd

      iclsVectorImm,

       // Vector Class
       //
       // Text Format:     C.EQ.QH vs,vt
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      vs, vt
       // Registers Set:

      iclsVectorVsVt,

       // Vector Class
       //
       // Text Format:     RZU.QH vd,vt
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      vs
       // Registers Set:       vd

      iclsVectorVdVt,

       // Vector Class
       //
       // Text Format:     WACH.QH vs
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:      vs
       // Registers Set:

      iclsVectorVs,

       // Vector Class
       //
       // Text Format:     RACH.QH vd
       //
       // Termination Type:    trmtaFallThrough
       //
       // Registers Used:
       // Registers Set:       vd

      iclsVectorVd,
   };

   struct CLS
   {
      BYTE    trmta;
      BYTE    rgopcls[4];         // Operand class for each operand
   };

   struct OPCD
   {
      const char  *szMnemonic;
      BYTE    icls;
   };

   static   const TRMT mptrmtatrmt[];

   static   const CLS rgcls[];

   static   const OPCD rgopcd[];
   static   const OPCD rgopcdSpecial[];
   static   const OPCD rgopcdRegimm[];
   static   const OPCD rgopcdBc[];
   static   const OPCD rgopcdCop[];
   static   const OPCD rgopcdCp0[];
   static   const OPCD rgopcdCp1[];
   static   const OPCD rgopcdCp2[];
   static   const OPCD rgopcdCop1x[];
   static   const OPCD rgopcdMadd[];

   static   const char rgszFormat[16][4];
   static   const char * const rgszGpr[32];

   static   const OPCD opcdB;
   static   const OPCD opcdNop;

        void FormatOperand(std::ostream&, OPCLS opcls) const;
        void FormatRegRel(std::ostream&, REGA, DWORD) const;
        bool FValidOperand(size_t) const;
   static   const OPCD *PopcdDecode(IW);
        const OPCD *PopcdPseudoOp(OPCD *, char *) const;

        IW m_iw;
        const OPCD *m_popcd;
};

#pragma pack(pop)
