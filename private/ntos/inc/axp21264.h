/*++ BUILD Version: 0000     Increment this if a change has global effects

Copyright (c) 1996 Digital Euipment Corporation

Module Name:

    axp21264.h

Abstract:

    This module defines the DECchip 21264-specific structures that are
    defined in the PAL but must be visible to the HAL.

Revision History:

--*/

#ifndef _AXP21264_
#define _AXP21264_


//
// Define the "special" processor bus used by all machines that run a
// DECchip 21264.  The processor bus is used to access the internal
// performance counters.
//

#define PROCESSOR_BUS_21264 21264

//
// Define the physical address bit that turns on user-mode access
// to I/O space in the pfn of a pte.  This bit is required because of
// the current 36 bit physical address space limit on NT.
//

#define EV6_USER_IO_ADDRESS_SPACE (ULONGLONG)(0x800000000)

//
// Define the Ebox Internal Processor Register formats.
//

//
// Define the CC_CTL.
//

typedef union _CC_CTL_21264{
    struct {
        ULONGLONG Count : 32;
        ULONGLONG CcEna : 1;
        ULONGLONG Ignore : 31;
    } ;
    ULONGLONG all;
} CC_CTL_21264, *PCC_CTL_21264;

//
//  Define VA_CTL.
//

typedef union _VA_CTL_21264{
    struct {
        ULONGLONG BigEndian : 1;
        ULONGLONG Va48 : 1;
        ULONGLONG VaForm32 : 1;
        ULONGLONG Mbz : 27;
        ULONGLONG VPtb : 34;
    };
    ULONGLONG all;
} VA_CTL_21264, *PVA_CTL_21264;

//
// Define the Ibox Internal Processor Register formats.
//

//
//  Define ITB_PTE.
//

typedef union _ITB_PTE_21264{
    struct {
        ULONGLONG Ignore1 : 4;
        ULONGLONG Asm : 1;
        ULONGLONG Gh : 2;
        ULONGLONG Ignore2 : 1;
        ULONGLONG Kre : 1;
        ULONGLONG Ere : 1;
        ULONGLONG Sre : 1;
        ULONGLONG Ure : 1;
        ULONGLONG Ignore3 : 1;
        ULONGLONG Pfn : 31;
        ULONGLONG Ignore4 : 20;
    };
    ULONGLONG all;
} ITB_PTE_21264, *PITB_PTE_21264;

//
// Define EXC_ADDR
//

typedef union _EXC_ADDR_21264{
	  struct{
		  ULONGLONG Pal : 1;
		  ULONGLONG Raz : 1;
		  ULONGLONG Pc  : 62;
	  };
	  ULONGLONG all;
}  EXC_ADDR_21264, *PEXC_ADDR_21264;



//
//  Define IER_CM - Interrupt Enable/Current Mode Register
//  Note that this can be also be written as two independant registers.
//

typedef union _IER_CM_21264{
    struct {
        ULONGLONG Raz1 : 3;
        ULONGLONG Cm : 2;
        ULONGLONG Raz2 : 8;
        ULONGLONG AstEn : 1;
        ULONGLONG SiEn : 15;
        ULONGLONG PcEn : 2;
        ULONGLONG CrEn : 1;
        ULONGLONG SlEn : 1;
        ULONGLONG EiEn : 6;
        ULONGLONG Raz3 : 25;
    };
    ULONGLONG all;
} IER_CM_21264, *PIER_CM_21264;

//
//  Define SIRR - Software Interrupt Request Register
//

typedef union _SIRR_21264{
    struct{
        ULONGLONG Raz1 : 14;
        ULONGLONG Sir : 15;
        ULONGLONG Raz2 : 35;
    };
    ULONGLONG all;
} SIRR_21264, *PSIRR_21264;

//
//  Define ISUM - Interrupt Summary register
//

typedef union _ISUM_21264{
    struct{
        ULONGLONG Raz1 : 3;
        ULONGLONG AstK : 1;
        ULONGLONG AstE : 1;
        ULONGLONG Raz2 : 4;
        ULONGLONG AstS : 1;
        ULONGLONG AstU : 1;
        ULONGLONG Raz3 : 3;
        ULONGLONG Si : 15;
        ULONGLONG Pc : 2;
        ULONGLONG Cr : 1;
        ULONGLONG Sl : 1;
        ULONGLONG Ei : 6;
        ULONGLONG Raz4 : 25;
    };
    ULONGLONG all;
} ISUM_21264, *PISUM_21264;

//
//  Define HW_INT_CLR - Hardware Interrupt Clear Register
//

typedef union _HW_INT_CLR_21264{
    struct{
        ULONGLONG Ign1 : 26;
        ULONGLONG Fbtp : 1;
        ULONGLONG Fbdp : 1;
        ULONGLONG MchkD : 1;
        ULONGLONG Pc : 2;
        ULONGLONG Cr : 1;
        ULONGLONG Sl : 1;
        ULONGLONG Ign2 : 31;
    };
    ULONGLONG all;
} HW_INT_CLR_21264, *PHW_INT_CLR_21264;

//
//  Define EXC_SUM - Exception Summary Register
//

typedef union _EXC_SUM_21264{
    struct{
        ULONGLONG Swc : 1;
        ULONGLONG Inv : 1;
        ULONGLONG Dze : 1;
        ULONGLONG Fov : 1;
        ULONGLONG Unf : 1;
        ULONGLONG Ine : 1;
        ULONGLONG Iov : 1;
        ULONGLONG Int : 1;
        ULONGLONG Reg : 5;
        ULONGLONG BadIva : 1;
        ULONGLONG Ignore1 : 27;
        ULONGLONG PcOvf	 : 1;
        ULONGLONG SetInv : 1;
        ULONGLONG SetDze : 1;
        ULONGLONG SetOvf : 1;
        ULONGLONG SetUnf : 1;
        ULONGLONG SetIne : 1;
        ULONGLONG SetIov : 1;
        ULONGLONG Ignore2 : 16;
    };
    ULONGLONG all;
} EXC_SUM_21264, *PEXC_SUM_21264;

//
//  Define I_CTL - Ibox Control Register
//

typedef union _I_CTL_21264{
    struct{
        ULONGLONG PcEn : 1;
        ULONGLONG IcEnable : 2;
        ULONGLONG Sp32 : 1;
        ULONGLONG Sp43 : 1;
        ULONGLONG Sp48 : 1;
        ULONGLONG Raz1 : 1;
        ULONGLONG Sde : 1;
        ULONGLONG Sbe : 2;
        ULONGLONG BpMode : 2;
        ULONGLONG Hwe : 1;
        ULONGLONG Fbtp : 1;
        ULONGLONG Fbdp : 1;
        ULONGLONG Va48 : 1;
        ULONGLONG VaForm32 : 1;
        ULONGLONG SingleIssue : 1;
        ULONGLONG Pct0En : 1;
        ULONGLONG Pct1En : 1;
        ULONGLONG CallPalR23 : 1;
        ULONGLONG MchkEn : 1;
        ULONGLONG TbMbEn : 1;
        ULONGLONG BistFail : 1;
        ULONGLONG ChipId : 6;
        ULONGLONG Vptb : 18;
        ULONGLONG Sext : 16;
    };
    ULONGLONG all;
} I_CTL_21264, *PI_CTL_21264;

//
//  Define I_STAT - Ibox Status Register
//

typedef union _I_STAT_21264{
    struct{
        ULONGLONG Raz1 : 29;
        ULONGLONG Tpe : 1;
        ULONGLONG Dpe : 1;
        ULONGLONG Raz2 : 33;
    };
    ULONGLONG all;
} I_STAT_21264, *PI_STAT_21264;

//
//  Define PCTX - Ibox Process Context Register
//  Note that this can be also be written as five independant registers.
//  (ASN, ASTER, ASTRR, PPCE, FPE)
//

typedef union _PCTX_21264{
    struct{
        ULONGLONG Raz1 : 1;
        ULONGLONG Ppce : 1;
        ULONGLONG Fpe : 1;
        ULONGLONG Raz2 : 2;
        ULONGLONG AstEr : 4;
        ULONGLONG AstRr : 4;
        ULONGLONG Raz3 : 26;
        ULONGLONG Asn : 8;
        ULONGLONG Raz4 : 17;
    };
    ULONGLONG all;
} PCTX_21264, *PPCTX_21264;

//
//  Define PCTR_CTL - Performance Counter Control Register
//

typedef union _PCTR_CTL_21264{
    struct{
        ULONGLONG Sel1 : 4;
        ULONGLONG Sel0 : 1;
        ULONGLONG Raz1 : 1;
        ULONGLONG Pctr1 : 20;
        ULONGLONG Raz2 : 2;
        ULONGLONG Pctr0 : 20;
        ULONGLONG Raz3 : 16;
    };
    ULONGLONG all;
} PCTR_CTL_21264, *PPCTR_CTL_21264;


//
// Define the Mbox and Dcache Internal Processor Register formats.
//

//
//  Define DTB_PTE
//

typedef union _DTB_PTE_21264{
    struct{
        ULONGLONG Ignore1 : 1;
        ULONGLONG For : 1;
        ULONGLONG Fow : 1;
        ULONGLONG Ignore2 : 1;
        ULONGLONG Asm : 1;
        ULONGLONG Gh : 2;
        ULONGLONG Ignore3 : 1;
        ULONGLONG Kre : 1;
        ULONGLONG Ere : 1;
        ULONGLONG Sre : 1;
        ULONGLONG Ure : 1;
        ULONGLONG Kwe : 1;
        ULONGLONG Ewe : 1;
        ULONGLONG Swe : 1;
        ULONGLONG Uwe : 1;
        ULONGLONG Ignore4 : 16;
        ULONGLONG Pfn : 31;
        ULONGLONG Ignore5 : 1;
    };
    ULONGLONG all;
} DTB_PTE_21264, *PDTB_PTE_21264;

//
//  Define DTB_ASN
//

typedef union _DTB_ASN_21264{
    struct{
        ULONGLONG Ignore1 : 56;
        ULONGLONG Asn : 8;
    };
    ULONGLONG all;
} DTB_ASN_21264, *PDTB_ASN_21264;

//
//  Define MM_STAT - MBOX Status Register
//

typedef union _MM_STAT_21264{
    struct{
        ULONGLONG Wr : 1;
        ULONGLONG Acv : 1;
        ULONGLONG For : 1;
        ULONGLONG Fow : 1;
        ULONGLONG Opcode : 6;
        ULONGLONG DcTagPerr : 1;
        ULONGLONG Ignore1 : 53;
    };
    ULONGLONG all;
} MM_STAT_21264, *PMM_STAT_21264;

//
//  Define M_CTL - MBOX Control Register
//

typedef union _M_CTL_21264{
    struct{
        ULONGLONG Mbz1	: 1;
        ULONGLONG sp32	: 1;
		ULONGLONG sp43	: 1;
		ULONGLONG sp48	: 1;
        ULONGLONG Mbz2	: 60;
    };
    ULONGLONG all;
} M_CTL_21264, *PM_CTL_21264;

//
//  Define DC_CTL - Dcache Control Register
//

typedef union _DC_CTL_21264{
    struct{
        ULONGLONG SetEn : 2;
        ULONGLONG Fhit : 1;
        ULONGLONG Flush : 1;
        ULONGLONG FBadTpar : 1;
        ULONGLONG FBadDecc : 1;
        ULONGLONG DcTagParEn : 1;
        ULONGLONG DcDatErrEn : 1;
        ULONGLONG Mbz1 : 56;
    };
    ULONGLONG all;
} DC_CTL_21264, *PDC_CTL_21264;

//
//  Define DC_STAT - Dcache Status Register
//

typedef union _DC_STAT_21264{
    struct{
        ULONGLONG TPerrP0 : 1;
        ULONGLONG TPerrP1 : 1;
        ULONGLONG EccErrSt : 1;
        ULONGLONG EccErrLd : 1;
        ULONGLONG Seo : 1;
        ULONGLONG Raz1 : 59;
    };
    ULONGLONG all;
} DC_STAT_21264, *PDC_STAT_21264;

        
//
// Define Cbox Internal Processor Registers.
//

//
// Define CSTAT field in CBOX read IPR
//

typedef union _C_STAT_21264 {
    struct {
        ULONGLONG       ErrorQualifier  :3;
        ULONGLONG       IstreamError    :1;
        ULONGLONG       DoubleBitError  :1;
        ULONGLONG       Reserved        :59;
    };
    ULONGLONG all;
} C_STAT_21264, *PC_STAT_21264;


// SjBfix. CBOX Register chain not defined yet.



//
// Define the Interrupt Mask structure communicated between the 
// HAL and PALcode.
//
// This is the bit defintion for the IRQL fields that are stored
// in the PCR IrqlTable. Keep it the same as on EV4.
//

typedef struct _IETEntry_21264{
    ULONG ApcEnable: 1;
    ULONG DispatchEnable: 1;
    ULONG PerformanceCounter0Enable: 1;
    ULONG PerformanceCounter1Enable: 1;
    ULONG CorrectableReadEnable: 1;
    ULONG Irq0Enable: 1;
    ULONG Irq1Enable: 1;
    ULONG Irq2Enable: 1;
    ULONG Irq3Enable: 1;
    ULONG Irq4Enable: 1;
    ULONG Irq5Enable: 1;
    ULONG Reserved: 21;
} IETEntry_21264, *PIETEntry_21264;

//
// Define the offsets and sizes of the mask sub-tables within the interrupt
// mask table in the PCR.
//

#define IRQLMASK_HDW_SUBTABLE_21264 (8)
#define IRQLMASK_HDW_SUBTABLE_21264_ENTRIES (64)

#define IRQLMASK_SFW_SUBTABLE_21264 (0)
#define IRQLMASK_SFW_SUBTABLE_21264_ENTRIES (4)

#define IRQLMASK_PC_SUBTABLE_21264  (4)
#define IRQLMASK_PC_SUBTABLE_21264_ENTRIES (4)

//
// HACKHACK - this should probably be in a table
//
#define EV6_CRD_VECTOR (25)

//
// PALcode Event Counters for the 21264
// This is the structure of the data returned by the rdcounters call pal.
//

typedef struct _COUNTERS_21264{
    ULONGLONG MachineCheckCount;
    ULONGLONG ArithmeticExceptionCount;
    ULONGLONG InterruptCount;
    ULONGLONG ItbMissCount;
    ULONGLONG DtbMissSingleCount;
    ULONGLONG DtbMissDoubleCount;
    ULONGLONG IAccvioCount;
    ULONGLONG DfaultCount;
    ULONGLONG UnalignedCount;
    ULONGLONG OpcdecCount;
    ULONGLONG FenCount;
    ULONGLONG ItbTnvCount;
    ULONGLONG DtbTnvCount;
    ULONGLONG PdeTnvCount;
    ULONGLONG FPCRCount;
    ULONGLONG RestCount;
    ULONGLONG DtbMissDouble4Count;
    ULONGLONG HardwareInterruptCount;
    ULONGLONG SoftwareInterruptCount;
    ULONGLONG SpecialInterruptCount;
    ULONGLONG HaltCount;
    ULONGLONG RestartCount;
    ULONGLONG DrainaCount;
    ULONGLONG RebootCount;
    ULONGLONG InitpalCount;
    ULONGLONG WrentryCount;
    ULONGLONG SwpirqlCount;
    ULONGLONG RdirqlCount;
    ULONGLONG DiCount;
    ULONGLONG EiCount;
    ULONGLONG SwppalCount;
    ULONGLONG SsirCount;
    ULONGLONG CsirCount;
    ULONGLONG RfeCount;
    ULONGLONG RetsysCount;
    ULONGLONG SwpctxCount;
    ULONGLONG SwpprocessCount;
    ULONGLONG RdmcesCount;
    ULONGLONG WrmcesCount;
    ULONGLONG TbiaCount;
    ULONGLONG TbisCount;
    ULONGLONG TbisasnCount;
    ULONGLONG DtbisCount;
    ULONGLONG RdkspCount;
    ULONGLONG SwpkspCount;
    ULONGLONG RdpsrCount;
    ULONGLONG RdpcrCount;
    ULONGLONG RdthreadCount;
    ULONGLONG TbimCount;
    ULONGLONG TbimasnCount;
    ULONGLONG RdcountersCount;
    ULONGLONG RdstateCount;
    ULONGLONG WrperfmonCount;
    ULONGLONG InitpcrCount;
    ULONGLONG BptCount;
    ULONGLONG CallsysCount;
    ULONGLONG ImbCount;
    ULONGLONG GentrapCount;
    ULONGLONG RdtebCount;
    ULONGLONG KbptCount;
    ULONGLONG CallkdCount;
    ULONGLONG AddressSpaceSwapCount;
    ULONGLONG AsnWrapCount;
    ULONGLONG EalnfixCount;
    ULONGLONG DalnfixCount;
    ULONGLONG SleepCount;
    ULONGLONG Misc1Count;
    ULONGLONG Misc2Count;
    ULONGLONG Misc3Count;
    ULONGLONG Misc4Count;
    ULONGLONG Misc5Count;
    ULONGLONG Misc6Count;
    ULONGLONG Misc7Count;
    ULONGLONG Misc8Count;
    ULONGLONG Misc9Count;
    ULONGLONG Misc10Count;
    ULONGLONG Misc11Count;
    ULONGLONG Misc12Count;
    ULONGLONG Misc13Count;
    ULONGLONG Misc14Count;
    ULONGLONG Misc15Count;
    ULONGLONG Misc16Count;
    ULONGLONG Misc17Count;
} COUNTERS_21264, *PCOUNTERS_21264;

//
// Types of performance counters.
//

typedef enum _AXP21264_PCCOUNTER{
	Ev6PerformanceCounter0 = 0,
	Ev6PerformanceCounter1 = 1,
} AXP21264_PCCOUNTER, *PAXP21264_PCCOUNTER;

//
// Mux control values
//

typedef enum _AXP21264_PCMUXCONTROL{
    //
    //  Mux values for PCTR1:
    //
    Ev6Instructions = 0x00,
    Ev6CondBranches = 0x01,
    Ev6Mispredicts = 0x02,
    Ev6ITBMisses = 0x03,
    Ev6DTBMisses = 0x04,
    Ev6Unaligned = 0x05,
    Ev6IcacheMisses = 0x06,
    Ev6ReplayTraps = 0x07,
    Ev6LoadMisses = 0x08,
    Ev6DcacheMisses = 0x09,
    Ev6BcacheReads = 0x0a,
    Ev6BcacheWrites = 0x0b,
    Ev6SysPortReads = 0x0c,
    Ev6SysPortWrites = 0x0d,
    Ev6MBStalls = 0x0e,             // SjBfix. Not documented
    Ev6StcStalls = 0x0f,            // SjBfix. Not documented

    //
    //  Mux values for PCTR0:
    //
    Ev6Cycles = 0x00,
    Ev6RetiredInstructions = 0x01

} AXP21264_PCMUXCONTROL, *PAXP21264_PCMUXCONTROL;


//
// Internal processor state record.
// This is the structure of the data returned by the rdstate call pal.
//

typedef struct _PROCESSOR_STATE_21264{
    IER_CM_21264 IerCm;
    SIRR_21264 Sirr;
    ISUM_21264 Isum;
    EXC_SUM_21264 ExcSum;
    ULONGLONG PalBase;
    I_CTL_21264 ICtl;
    I_STAT_21264 IStat;
    PCTX_21264 PCtx;
    PCTR_CTL_21264 PCtr;
    MM_STAT_21264 MmStat;
    DC_STAT_21264 DcStat;

} PROCESSOR_STATE_21264, *PPROCESSOR_STATE_21264;

//
// Machine-check logout frame.
//

typedef struct _LOGOUT_FRAME_21264{
    ULONG               FrameSize;
    ULONG               RSDC;
    ULONG               CpuAreaOffset;
    ULONG               SystemAreaOffset;
    ULONG               MchkCode;
    ULONG               MchkFrameRev;
    I_STAT_21264        IStat;
    DC_STAT_21264       DcStat;
    ULONGLONG           CAddr;
    ULONGLONG           Dc1Syndrome;
    ULONGLONG           Dc0Syndrome;
    ULONGLONG           CStat;
    ULONGLONG           CSts;
    ULONGLONG           Va;
    ULONGLONG           ExcAddr;
    IER_CM_21264        IerCm;
    ISUM_21264          ISum;
    MM_STAT_21264       MmStat;
    ULONGLONG           PalBase;
    I_CTL_21264         ICtl;
    PCTX_21264          PCtx;
    VA_CTL_21264        VaCtl;
    ULONGLONG           Ps;
} LOGOUT_FRAME_21264, *PLOGOUT_FRAME_21264;

//
// Correctable logout frame
//

typedef struct _CORRECTABLE_FRAME_21264 {
    ULONG               FrameSize;
    ULONG               RSDC;
    ULONG               CpuAreaOffset;
    ULONG               SystemAreaOffset;
    ULONG               MchkCode;
    ULONG               MchkFrameRev;
    I_STAT_21264        IStat;
    DC_STAT_21264       DCStat;
    ULONGLONG           CAddr;
    ULONGLONG           Dc1Syndrome;
    ULONGLONG           Dc0Syndrome;
    ULONGLONG           CStat;
    ULONGLONG           CSts;
    ULONGLONG           MmStat;
} CORRECTABLE_FRAME_21264, *PCORRECTABLE_FRAME_21264;

//
// Define the number of physical and virtual address bits
//

#define EV6_PHYSICAL_ADDRESS_BITS       44
#define EV6_VIRTUAL_ADDRESS_BITS        43

#endif //!_AXP21264_  
