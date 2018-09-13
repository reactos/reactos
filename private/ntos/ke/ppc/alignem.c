/*++

Copyright (c) 1993   IBM Corporation and Microsoft Corporation

Module Name:

    alignem.c

Abstract:

    This module implements the code necessary to emulate unaligned data
    references.

Author:

    Rick Simpson  4-Aug-1993

    Based on MIPS version by David N. Cutler (davec) 17-Jun-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KiSetFloatRegisterValue (
    IN ULONG,
    IN DOUBLE,
    OUT PKEXCEPTION_FRAME,
    OUT PKTRAP_FRAME
    );

DOUBLE
KiGetFloatRegisterValue (
    IN ULONG,
    IN PKEXCEPTION_FRAME,
    IN PKTRAP_FRAME
    );

/*++
    When PowerPC takes an Alignment Interrupt, the hardware loads the following:
        SRR 0 <- Address of instruction causing the interrupt
        SRR 1 <- MSR
        DAR   <- Effective address of the misaligned reference as computed
                   by the instruction that caused the interrupt
        DSISR <- Several fields relevant to the failing instruction:
                   Bits 12..13 <- Extended op-code (XO) if instr is DS-form
                   Bits 15..21 <- Index into the table below, identifying
                                    (for the most part) the failing instr
                   Bits 22..26 <- RT/RS/FRT/FRS field (reg no.) of instr
                   Bits 27..31 <- RA (reg no.) field for update-form instrs

    For the most part, it is not necessary to retrieve the actual instruction
    in order to emulate the effects of an unaligned load or store.  Enough
    information is in the DSISR to distinguish most cases.  Special processing
    is required for certain instructions -- the DSISR does not have enough
    information for them.

    It is unnecessary to compute the failing effective address by emulating
    the instruction's addressing arithmetic, because the value required is
    contained in the DAR.

    The table here is indexed by bits 15..21 of the DSISR.

    The "escape" flag indicates that some sort of special handling is needed,
    for one of the following reasons:
         1) More than one instruction maps to the same DSISR value
              (ld/ldu/lwa, std/stdu)
         2) The instruction is load-and-reserve or store-conditional,
              and misalignment should not be "fixed up"
         3) The instruction is a byte-reversed load or store
         4) The instruction is "ecowx" or "eciwx"
         5) The instruction is "dcbz"
         6) The instruction is "stfiwx"

    NOTE:  Even though lwz and lwarx share the same DSISR value (0), the
    table entry for position 0 is used only for lwz.  This is so that the
    most likely case (load word from unaligned address) can take the
    mainline path.  The less likely case (load word and reserve from
    unaligned address) is ignored and treated as if it were simply load
    word.  Unaligned addresses are not supported for lwarx/stwcx. in the
    PowerPC architecture.  The implementation here (allowing lwarx to
    proceed as if it were lwx, without establishing a reservation) is
    allowable according to the architecture; a matching store conditional
    (stwcx.) to the same unaligned address will fail (return FALSE from
    this routine), so the incorrect reservation address will be caught
    then.
--*/

typedef struct _ALFAULT {
    ULONG Valid    : 1;  // Valid DSISR value (1) vs. Should not occur (0)
    ULONG Load     : 1;  // Load (1) vs. Store (0)
    ULONG Length   : 2;  // Length:  2 bytes (1), 4 bytes (2), 8 bytes (3)
    ULONG Signed   : 1;  // Sign-extended (1) vs. Zero-extended (0)
    ULONG Fixed    : 1;  // Fixed point (1) vs. Floating point (0)
    ULONG Update   : 1;  // Update-form (1) vs. Non-Update-form (0)
    ULONG Escape   : 1;  // Needs special processing (1) vs. Regular (0)
} ALFAULT, *PALFAULT;

// Table indices for instructions needing special handling

#define LDARX_INDEX_VALUE     1
#define LD_INDEX_VALUE       13
#define STD_INDEX_VALUE      15
#define STWCX_INDEX_VALUE    66
#define STDCX_INDEX_VALUE    67
#define LWBRX_INDEX_VALUE    72
#define STWBRX_INDEX_VALUE   74
#define LHBRX_INDEX_VALUE    76
#define STHBRX_INDEX_VALUE   78
#define ECIWX_INDEX_VALUE    84
#define ECOWX_INDEX_VALUE    86
#define DCBZ_INDEX_VALUE     95
#define STFIWX_INDEX_VALUE  111

static ALFAULT AlFault[128] = {

//   Valid  Load  Length  Signed  Fixed  Update  Escape
    {  1,     1,     2,      0,     1,      0,      0  },  //   0   lwz, lwarx
    {  1,     1,     3,      0,     1,      0,      1  },  //   1   ldarx
    {  1,     0,     2,      0,     1,      0,      0  },  //   2   stw
    {  0,     0,     0,      0,     0,      0,      0  },  //   3
    {  1,     1,     1,      0,     1,      0,      0  },  //   4   lhz
    {  1,     1,     1,      1,     1,      0,      0  },  //   5   lha
    {  1,     0,     1,      0,     1,      0,      0  },  //   6   sth
    {  0,     0,     0,      0,     0,      0,      0  },  //   7
    {  1,     1,     2,      0,     0,      0,      0  },  //   8   lfs
    {  1,     1,     3,      0,     0,      0,      0  },  //   9   lfd
    {  1,     0,     2,      0,     0,      0,      0  },  //  10   stfs
    {  1,     0,     3,      0,     0,      0,      0  },  //  11   stfd
    {  0,     0,     0,      0,     0,      0,      0  },  //  12
    {  1,     1,     0,      0,     0,      0,      1  },  //  13   ld, ldu, lwa
    {  0,     0,     0,      0,     0,      0,      0  },  //  14
    {  1,     0,     0,      0,     0,      0,      1  },  //  15   std, stdu
    {  1,     1,     2,      0,     1,      1,      0  },  //  16   lwzu
    {  0,     0,     0,      0,     0,      0,      0  },  //  17
    {  1,     0,     2,      0,     1,      1,      0  },  //  18   stwu
    {  0,     0,     0,      0,     0,      0,      0  },  //  19
    {  1,     1,     1,      0,     1,      1,      0  },  //  20   lhzu
    {  1,     1,     1,      1,     1,      1,      0  },  //  21   lhau
    {  1,     0,     1,      0,     1,      1,      0  },  //  22   sthu
    {  0,     0,     0,      0,     0,      0,      0  },  //  23
    {  1,     1,     2,      0,     0,      1,      0  },  //  24   lfsu
    {  1,     1,     3,      0,     0,      1,      0  },  //  25   lfdu
    {  1,     0,     2,      0,     0,      1,      0  },  //  26   stfsu
    {  1,     0,     3,      0,     0,      1,      0  },  //  27   stfdu
    {  0,     0,     0,      0,     0,      0,      0  },  //  28
    {  0,     0,     0,      0,     0,      0,      0  },  //  29
    {  0,     0,     0,      0,     0,      0,      0  },  //  30
    {  0,     0,     0,      0,     0,      0,      0  },  //  31
    {  1,     1,     3,      0,     1,      0,      0  },  //  32   ldx
    {  0,     0,     0,      0,     0,      0,      0  },  //  33
    {  1,     0,     3,      0,     1,      0,      0  },  //  34   stdx
    {  0,     0,     0,      0,     0,      0,      0  },  //  35
    {  0,     0,     0,      0,     0,      0,      0  },  //  36
    {  1,     1,     2,      1,     1,      0,      0  },  //  37   lwax
    {  0,     0,     0,      0,     0,      0,      0  },  //  38
    {  0,     0,     0,      0,     0,      0,      0  },  //  39
    {  0,     0,     0,      0,     0,      0,      0  },  //  40
    {  0,     0,     0,      0,     0,      0,      0  },  //  41
    {  0,     0,     0,      0,     0,      0,      0  },  //  42
    {  0,     0,     0,      0,     0,      0,      0  },  //  43
    {  0,     0,     0,      0,     0,      0,      0  },  //  44
    {  0,     0,     0,      0,     0,      0,      0  },  //  45
    {  0,     0,     0,      0,     0,      0,      0  },  //  46
    {  0,     0,     0,      0,     0,      0,      0  },  //  47
    {  1,     1,     3,      0,     1,      1,      0  },  //  48   ldux
    {  0,     0,     0,      0,     0,      0,      0  },  //  49
    {  1,     0,     3,      0,     1,      1,      0  },  //  50   stdux
    {  0,     0,     0,      0,     0,      0,      0  },  //  51
    {  0,     0,     0,      0,     0,      0,      0  },  //  52
    {  1,     1,     2,      1,     1,      1,      0  },  //  53   lwaux
    {  0,     0,     0,      0,     0,      0,      0  },  //  54
    {  0,     0,     0,      0,     0,      0,      0  },  //  55
    {  0,     0,     0,      0,     0,      0,      0  },  //  56
    {  0,     0,     0,      0,     0,      0,      0  },  //  57
    {  0,     0,     0,      0,     0,      0,      0  },  //  58
    {  0,     0,     0,      0,     0,      0,      0  },  //  59
    {  0,     0,     0,      0,     0,      0,      0  },  //  60
    {  0,     0,     0,      0,     0,      0,      0  },  //  61
    {  0,     0,     0,      0,     0,      0,      0  },  //  62
    {  0,     0,     0,      0,     0,      0,      0  },  //  63
    {  0,     0,     0,      0,     0,      0,      0  },  //  64
    {  0,     0,     0,      0,     0,      0,      0  },  //  65
    {  1,     0,     2,      0,     1,      0,      1  },  //  66   stwcx.
    {  1,     0,     3,      0,     1,      0,      1  },  //  67   stdcx.
    {  0,     0,     0,      0,     0,      0,      0  },  //  68
    {  0,     0,     0,      0,     0,      0,      0  },  //  69
    {  0,     0,     0,      0,     0,      0,      0  },  //  70
    {  0,     0,     0,      0,     0,      0,      0  },  //  71
    {  1,     1,     2,      0,     1,      0,      1  },  //  72   lwbrx
    {  0,     0,     0,      0,     0,      0,      0  },  //  73
    {  1,     0,     2,      0,     1,      0,      1  },  //  74   stwbrx
    {  0,     0,     0,      0,     0,      0,      0  },  //  75
    {  1,     1,     1,      0,     1,      0,      1  },  //  76   lhbrx
    {  0,     0,     0,      0,     0,      0,      0  },  //  77
    {  1,     0,     1,      0,     1,      0,      1  },  //  78   sthbrx
    {  0,     0,     0,      0,     0,      0,      0  },  //  79
    {  0,     0,     0,      0,     0,      0,      0  },  //  80
    {  0,     0,     0,      0,     0,      0,      0  },  //  81
    {  0,     0,     0,      0,     0,      0,      0  },  //  82
    {  0,     0,     0,      0,     0,      0,      0  },  //  83
    {  1,     1,     2,      0,     1,      0,      1  },  //  84   eciwx
    {  0,     0,     0,      0,     0,      0,      0  },  //  85
    {  1,     0,     2,      0,     1,      0,      1  },  //  86   ecowx
    {  0,     0,     0,      0,     0,      0,      0  },  //  87
    {  0,     0,     0,      0,     0,      0,      0  },  //  88
    {  0,     0,     0,      0,     0,      0,      0  },  //  89
    {  0,     0,     0,      0,     0,      0,      0  },  //  90
    {  0,     0,     0,      0,     0,      0,      0  },  //  91
    {  0,     0,     0,      0,     0,      0,      0  },  //  92
    {  0,     0,     0,      0,     0,      0,      0  },  //  93
    {  0,     0,     0,      0,     0,      0,      0  },  //  94
    {  1,     0,     0,      0,     0,      0,      1  },  //  95   dcbz
    {  1,     1,     2,      0,     1,      0,      0  },  //  96   lwzx
    {  0,     0,     0,      0,     0,      0,      0  },  //  97
    {  1,     0,     2,      0,     1,      0,      0  },  //  98   stwzx
    {  0,     0,     0,      0,     0,      0,      0  },  //  99
    {  1,     1,     1,      0,     1,      0,      0  },  // 100   lhzx
    {  1,     1,     1,      1,     1,      0,      0  },  // 101   lhax
    {  1,     0,     1,      0,     1,      0,      0  },  // 102   sthx
    {  0,     0,     0,      0,     0,      0,      0  },  // 103
    {  1,     1,     2,      0,     0,      0,      0  },  // 104   lfsx
    {  1,     1,     3,      0,     0,      0,      0  },  // 105   lfdx
    {  1,     0,     2,      0,     0,      0,      0  },  // 106   stfsx
    {  1,     0,     3,      0,     0,      0,      0  },  // 107   stfdx
    {  0,     0,     0,      0,     0,      0,      0  },  // 108
    {  0,     0,     0,      0,     0,      0,      0  },  // 109
    {  0,     0,     0,      0,     0,      0,      0  },  // 110
    {  1,     0,     2,      0,     1,      0,      1  },  // 111   stfiwx
    {  1,     1,     2,      0,     1,      1,      0  },  // 112   lwzux
    {  0,     0,     0,      0,     0,      0,      0  },  // 113
    {  1,     0,     2,      0,     1,      1,      0  },  // 114   stwux
    {  0,     0,     0,      0,     0,      0,      0  },  // 115
    {  1,     1,     1,      0,     1,      1,      0  },  // 116   lhzux
    {  1,     1,     1,      1,     1,      1,      0  },  // 117   lhaux
    {  1,     0,     1,      0,     1,      1,      0  },  // 118   sthux
    {  0,     0,     0,      0,     0,      0,      0  },  // 119
    {  1,     1,     2,      0,     0,      1,      0  },  // 120   lfsux
    {  1,     1,     3,      0,     0,      1,      0  },  // 121   lfdux
    {  1,     0,     2,      0,     0,      1,      0  },  // 122   stfsux
    {  1,     0,     3,      0,     0,      1,      0  },  // 123   stfdux
    {  0,     0,     0,      0,     0,      0,      0  },  // 124
    {  0,     0,     0,      0,     0,      0,      0  },  // 125
    {  0,     0,     0,      0,     0,      0,      0  },  // 126
    {  0,     0,     0,      0,     0,      0,      0  }   // 127
};

BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to emulate an unaligned data reference to an
    address in the user part of the address space.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    A value of TRUE is returned if the data reference is successfully
    emulated. Otherwise, a value of FALSE is returned.

--*/

{

    ULONG BranchAddress;
    PUCHAR DataAddress;

    union {
        DOUBLE Double;
        float  Float;
        ULONG  Long;
        SHORT  Short;
    } DataReference;
    PUCHAR DataValue = (PUCHAR) &DataReference;

    PVOID ExceptionAddress;
    DSISR DsisrValue;
    ULONG TableIndex;
    ULONG DataRegNum;
    ALFAULT Info;
    KIRQL OldIrql;

    //
    // Call out to profile interrupt if alignment profiling is active
    //
    if (KiProfileAlignmentFixup) {

        if (++KiProfileAlignmentFixupCount >= KiProfileAlignmentFixupInterval) {

            KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
            KiProfileAlignmentFixupCount = 0;
            KeProfileInterruptWithSource(TrapFrame, ProfileAlignmentFixup);
            KeLowerIrql(OldIrql);

        }
    }

    //
    // Save the original exception address in case another exception
    // occurs.
    //

    ExceptionAddress = ExceptionRecord->ExceptionAddress;

    //
    // Any exception that occurs during the attempted emulation of the
    // unaligned reference causes the emulation to be aborted. The new
    // exception code and information is copied to the original exception
    // record and a value of FALSE is returned.
    //

    try {

        //
        // PowerPC has no branch-delay-slot complexities like MIPS
        //

        BranchAddress = TrapFrame->Iar + 4;

        //
        // The effective address of the reference from the DAR was saved
        // in the exception record. Check to make sure it is within the
        // user part of the address space. Alignment exceptions take
        // precedence over memory management exceptions (this is true
        // for PowerPC as well as MIPS) and the address could be a
        // system address.
        //

        DataAddress = (PUCHAR) (ExceptionRecord->ExceptionInformation[1]);

        if ((ULONG)DataAddress < MM_USER_PROBE_ADDRESS) {

            //
            // Get information about the failing instruction from saved DSISR.
            //

            DsisrValue = *(DSISR*) &(ExceptionRecord->ExceptionInformation[2]);
            TableIndex = DsisrValue.Index;
            DataRegNum = DsisrValue.DataReg;
            Info = AlFault[TableIndex];

            //
            // If table entry is marked invalid, we have some sort of logic error.
            //

            if (!Info.Valid)
                return FALSE;

            //
            // If table entry does not indicate special processing needed,
            //   emulate the execution of the instruction
            //

            if (!Info.Escape) {

                    //
                    // Integer or float load or store
                    //

                if (Info.Fixed) {

                        //
                        // Integer register
                        //

                    if (Info.Load) {

                            //
                            // Integer load
                            //

                        switch (Info.Length) {

                                //
                                // Halfword integer load
                                //

                          case 1:
                            DataValue[0] = DataAddress[0];
                            DataValue[1] = DataAddress[1];
                            KiSetRegisterValue
                                (DataRegNum,
                                 Info.Signed ?   // sign extension ...
                                     (ULONG) ((LONG) DataReference.Short) :
                                     (ULONG) ((USHORT) DataReference.Short),
                                 ExceptionFrame,
                                 TrapFrame);
                            break;

                                //
                                // Fullword integer load
                                //

                          case 2:
                            DataValue[0] = DataAddress[0];
                            DataValue[1] = DataAddress[1];
                            DataValue[2] = DataAddress[2];
                            DataValue[3] = DataAddress[3];
                            KiSetRegisterValue
                                (DataRegNum,
                                 DataReference.Long,
                                 ExceptionFrame,
                                 TrapFrame);
                            break;

                                //
                                // Doubleword integer load
                                //

                          case 3:
                            return FALSE; // Have no 8-byte integer regs yet

                        }
                    } else {

                            //
                            // Integer store
                            //

                        switch (Info.Length) {

                                //
                                // Halfword integer store
                                //

                          case 1:
                            DataReference.Short = (SHORT)
                                KiGetRegisterValue
                                    (DataRegNum,
                                     ExceptionFrame,
                                     TrapFrame);
                            DataAddress[0] = DataValue[0];
                            DataAddress[1] = DataValue[1];
                            break;

                                //
                                // Fullword integer store
                                //

                          case 2:       // Word
                            DataReference.Long =
                                KiGetRegisterValue
                                    (DataRegNum,
                                     ExceptionFrame,
                                     TrapFrame);
                            DataAddress[0] = DataValue[0];
                            DataAddress[1] = DataValue[1];
                            DataAddress[2] = DataValue[2];
                            DataAddress[3] = DataValue[3];
                            break;

                                //
                                // Doubleword integer store
                                //

                          case 3:

                            return FALSE; // Have no 8-byte integer regs yet
                        }
                    }
                } else {                // Floating point

                        //
                        // Floating-point register
                        //

                    if (Info.Load) {    // Floating point load

                            //
                            // Floating-point load
                            //

                        if (Info.Length == 2) {

                                //
                                // Floating-point single precision load
                                //
                            DataValue[0] = DataAddress[0];
                            DataValue[1] = DataAddress[1];
                            DataValue[2] = DataAddress[2];
                            DataValue[3] = DataAddress[3];
                            KiSetFloatRegisterValue
                                (DataRegNum,
                                 (DOUBLE) DataReference.Float,
                                 ExceptionFrame,
                                 TrapFrame);

                        } else {

                                //
                                // Floating-point double precision load
                                //
                            DataValue[0] = DataAddress[0];
                            DataValue[1] = DataAddress[1];
                            DataValue[2] = DataAddress[2];
                            DataValue[3] = DataAddress[3];
                            DataValue[4] = DataAddress[4];
                            DataValue[5] = DataAddress[5];
                            DataValue[6] = DataAddress[6];
                            DataValue[7] = DataAddress[7];
                            KiSetFloatRegisterValue
                                (DataRegNum,
                                 DataReference.Double,
                                 ExceptionFrame,
                                 TrapFrame);
                        }
                    } else {

                            //
                            // Floating-point store
                            //

                        if (Info.Length == 2) {

                                //
                                // Floating-point single precision store
                                //

                            DataReference.Float = (float)
                                KiGetFloatRegisterValue
                                    (DataRegNum,
                                     ExceptionFrame,
                                     TrapFrame);
                            DataAddress[0] = DataValue[0];
                            DataAddress[1] = DataValue[1];
                            DataAddress[2] = DataValue[2];
                            DataAddress[3] = DataValue[3];

                        } else {

                                //
                                // Floating-point double precision store
                                //
                            DataReference.Double =
                                KiGetFloatRegisterValue
                                    (DataRegNum,
                                     ExceptionFrame,
                                     TrapFrame);
                            DataAddress[0] = DataValue[0];
                            DataAddress[1] = DataValue[1];
                            DataAddress[2] = DataValue[2];
                            DataAddress[3] = DataValue[3];
                            DataAddress[4] = DataValue[4];
                            DataAddress[5] = DataValue[5];
                            DataAddress[6] = DataValue[6];
                            DataAddress[7] = DataValue[7];
                        }
                    }
                }

                //
                // See if "update" (post-increment) form of addressing
                //

                if (Info.Update)
                    KiSetRegisterValue  // Store effective addr back into base reg
                        (DsisrValue.UpdateReg,
                         (ULONG) DataAddress,
                         ExceptionFrame,
                         TrapFrame);

            }

            //
            // Table indicates that special processing is needed, either because
            // the DSISR does not contain enough information to disambiguate the
            // failing instruction, or the instruction is not a load or store,
            // or the instruction has some other unusual requirement.
            //

            else {                      // Info.Escape == 1
                switch (TableIndex) {

                //
                // Doubleword integers not yet supported
                //

                  case LD_INDEX_VALUE:
                  case STD_INDEX_VALUE:
                    return FALSE;

                //
                // Load-and-reserve, store-conditional not supported
                //   for misaligned addresses
                //

                  case LDARX_INDEX_VALUE:
                  case STWCX_INDEX_VALUE:
                  case STDCX_INDEX_VALUE:
                    return FALSE;

                //
                // Integer byte-reversed fullword load
                //

                  case LWBRX_INDEX_VALUE:
                    DataValue[0] = DataAddress[3];
                    DataValue[1] = DataAddress[2];
                    DataValue[2] = DataAddress[1];
                    DataValue[3] = DataAddress[0];
                    KiSetRegisterValue
                        (DataRegNum,
                         DataReference.Long,
                         ExceptionFrame,
                         TrapFrame);
                    break;

                //
                // Integer byte-reversed fullword store
                //

                  case STWBRX_INDEX_VALUE:
                    DataReference.Long =
                        KiGetRegisterValue
                            (DataRegNum,
                             ExceptionFrame,
                             TrapFrame);
                    DataAddress[0] = DataValue[3];
                    DataAddress[1] = DataValue[2];
                    DataAddress[2] = DataValue[1];
                    DataAddress[3] = DataValue[0];
                    break;

                //
                // Integer byte-reversed halfword load
                //

                  case LHBRX_INDEX_VALUE:
                    DataValue[0] = DataAddress[1];
                    DataValue[1] = DataAddress[0];
                    KiSetRegisterValue
                        (DataRegNum,
                         Info.Signed ?   // sign extension ...
                             (ULONG) ((LONG) DataReference.Short) :
                             (ULONG) ((USHORT) DataReference.Short),
                         ExceptionFrame,
                         TrapFrame);
                    break;

                //
                // Integer byte-reversed halfword store
                //

                  case STHBRX_INDEX_VALUE:
                    DataReference.Short = (SHORT)
                        KiGetRegisterValue
                            (DataRegNum,
                             ExceptionFrame,
                             TrapFrame);
                    DataAddress[0] = DataValue[1];
                    DataAddress[1] = DataValue[0];
                    break;

                //
                // Special I/O instructions not supported yet
                //

                  case ECIWX_INDEX_VALUE:
                  case ECOWX_INDEX_VALUE:
                    return FALSE;

                //
                // Data Cache Block Zero
                //
                //   dcbz causes an alignment fault if cache is disabled
                //   for the address range covered by the block.
                //
                //   A data cache block is 32 bytes long, we emulate this
                //   instruction by storing 8 zero integers a the address
                //   specified.
                //
                //   Note, dcbz zeros the block "containing" the address
                //   so we round down first.
                //

                  case DCBZ_INDEX_VALUE: {
                    PULONG DcbAddress = (PULONG)((ULONG)DataAddress & ~0x1f);

                    *DcbAddress++ = 0;
                    *DcbAddress++ = 0;
                    *DcbAddress++ = 0;
                    *DcbAddress++ = 0;
                    *DcbAddress++ = 0;
                    *DcbAddress++ = 0;
                    *DcbAddress++ = 0;
                    *DcbAddress++ = 0;
                    break;
                  }

                //
                // Store Floating as Integer
                //

                  case STFIWX_INDEX_VALUE:
                    DataReference.Double =
                        KiGetFloatRegisterValue
                            (DataRegNum,
                             ExceptionFrame,
                             TrapFrame);
                    DataAddress[0] = DataValue[0];
                    DataAddress[1] = DataValue[1];
                    DataAddress[2] = DataValue[2];
                    DataAddress[3] = DataValue[3];
                }
            }

            TrapFrame->Iar = BranchAddress;
            return TRUE;
        }

    //
    // If an exception occurs, then copy the new exception information to the
    // original exception record and handle the exception.
    //

    } except (KiCopyInformation(ExceptionRecord,
                               (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Preserve the original exception address.
        //

        ExceptionRecord->ExceptionAddress = ExceptionAddress;
    }

    //
    // Return a value of FALSE.
    //

    return FALSE;
}

BOOLEAN
KiEmulateDcbz (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to emulate a Data Cache Block Zero instruction.
    The PowerPC hardware will raise an alignment exception if a DCBZ is
    attempted on non-cached memory.   We need to emulate this even in kernel
    mode so we can debug h/w problems by disabling the data cache.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    A value of TRUE is returned if the data reference is successfully
    emulated. Otherwise, a value of FALSE is returned.

--*/

{

    PUCHAR DataAddress;
    PVOID ExceptionAddress;
    DSISR DsisrValue;
    ULONG TableIndex;
    ULONG DataRegNum;
    ALFAULT Info;

    //
    // Save the original exception address in case another exception
    // occurs.
    //

    ExceptionAddress = ExceptionRecord->ExceptionAddress;

    //
    // Any exception that occurs during the attempted emulation of the
    // unaligned reference causes the emulation to be aborted. The new
    // exception code and information is copied to the original exception
    // record and a value of FALSE is returned.
    //

    try {

        //
        // The effective address of the reference from the DAR was saved
        // in the exception record. Check to make sure it is within the
        // user part of the address space. Alignment exceptions take
        // precedence over memory management exceptions (this is true
        // for PowerPC as well as MIPS) and the address could be a
        // system address.
        //

        DataAddress = (PUCHAR) (ExceptionRecord->ExceptionInformation[1]);

        //
        // Get information about the failing instruction from saved DSISR.
        //

        DsisrValue = *(DSISR*) &(ExceptionRecord->ExceptionInformation[2]);
        TableIndex = DsisrValue.Index;
        DataRegNum = DsisrValue.DataReg;
        Info = AlFault[TableIndex];

        //
        // If table entry is valid and does not indicate special processing
        // needed, and is a DCBZ instruction, emulate the execution of the 
        // instruction
        //

        if (Info.Valid && Info.Escape && (TableIndex == DCBZ_INDEX_VALUE)) {

            //
            // Data Cache Block Zero
            //
            //   A data cache block is 32 bytes long, we emulate this
            //   instruction by storing 8 zero integers a the address
            //   specified.
            //
            //   Note, dcbz zeros the block "containing" the address
            //   so we round down first.
            //

            PULONG DcbAddress = (PULONG)((ULONG)DataAddress & ~0x1f);

            *DcbAddress++ = 0;
            *DcbAddress++ = 0;
            *DcbAddress++ = 0;
            *DcbAddress++ = 0;
            *DcbAddress++ = 0;
            *DcbAddress++ = 0;
            *DcbAddress++ = 0;
            *DcbAddress++ = 0;

            //
            // Bump instruction address to next instruction.
            //

            TrapFrame->Iar += 4;

            return TRUE;
        }

    //
    // If an exception occurs, then copy the new exception information to the
    // original exception record and handle the exception.
    //

    } except (KiCopyInformation(ExceptionRecord,
                               (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Preserve the original exception address.
        //

        ExceptionRecord->ExceptionAddress = ExceptionAddress;
    }

    //
    // Return a value of FALSE.
    //

    return FALSE;
}
