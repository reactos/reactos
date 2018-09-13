/*++ BUILD Version: 0000     Increment this if a change has global effects

Copyright (c) 1993 Digital Euipment Corporation

Module Name:

    axp21066.h

Abstract:

    This module defines the DECchip 21066-specific structures that are
    defined in the PAL but must be visible to the HAL.

Revision History:

--*/

#ifndef _AXP21066_
#define _AXP21066_


#ifndef CORE_21064
#define CORE_21064

//
// Define the "special" processor bus used by all machines that run a
// DECchip 21064.  The processor bus is used to access the internal
// performance counters.
//

#define PROCESSOR_BUS_21064 21064

//
// Define the number of entries for repeated internal processor registers.
//

#define ITB_ENTRIES_21064 12
#define DTB_ENTRIES_21064 32
#define PAL_TEMPS_21064   32

//
// Define an interrupt enable table entry.
//

typedef struct _IETEntry_21064{
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
} IETEntry_21064, *PIETEntry_21064;


//
// Define the offsets and sizes of the mask sub-tables within the interrupt
// mask table in the PCR.
//

#define IRQLMASK_HDW_SUBTABLE_21064 (8)
#define IRQLMASK_HDW_SUBTABLE_21064_ENTRIES (64)

#define IRQLMASK_SFW_SUBTABLE_21064 (0)
#define IRQLMASK_SFW_SUBTABLE_21064_ENTRIES (4)

#define IRQLMASK_PC_SUBTABLE_21064  (4)
#define IRQLMASK_PC_SUBTABLE_21064_ENTRIES (4)

//
// PALcode Event Counters for the 21064
// This is the structure of the data returned by the rdcounters call pal.
//

typedef struct _COUNTERS_21064{
    LARGE_INTEGER MachineCheckCount;
    LARGE_INTEGER ArithmeticExceptionCount;
    LARGE_INTEGER InterruptCount;
    LARGE_INTEGER ItbMissCount;
    LARGE_INTEGER NativeDtbMissCount;
    LARGE_INTEGER PalDtbMissCount;
    LARGE_INTEGER ItbAcvCount;
    LARGE_INTEGER DtbAcvCount;
    LARGE_INTEGER UnalignedCount;
    LARGE_INTEGER OpcdecCount;
    LARGE_INTEGER FenCount;
    LARGE_INTEGER ItbTnvCount;
    LARGE_INTEGER DtbTnvCount;
    LARGE_INTEGER PteMissCount;
    LARGE_INTEGER KspMissCount;
    LARGE_INTEGER PdeTnvCount;
    LARGE_INTEGER HaltCount;
    LARGE_INTEGER RestartCount;
    LARGE_INTEGER DrainaCount;
    LARGE_INTEGER InitpalCount;
    LARGE_INTEGER WrentryCount;
    LARGE_INTEGER SwpirqlCount;
    LARGE_INTEGER RdirqlCount;
    LARGE_INTEGER DiCount;
    LARGE_INTEGER EiCount;
    LARGE_INTEGER SwppalCount;
    LARGE_INTEGER SsirCount;
    LARGE_INTEGER CsirCount;
    LARGE_INTEGER RfeCount;
    LARGE_INTEGER RetsysCount;
    LARGE_INTEGER SwpctxCount;
    LARGE_INTEGER SwpprocessCount;
    LARGE_INTEGER RdmcesCount;
    LARGE_INTEGER WrmcesCount;
    LARGE_INTEGER TbiaCount;
    LARGE_INTEGER TbisCount;
    LARGE_INTEGER DtbisCount;
    LARGE_INTEGER RdkspCount;
    LARGE_INTEGER SwpkspCount;
    LARGE_INTEGER RdpsrCount;
    LARGE_INTEGER RdpcrCount;
    LARGE_INTEGER RdthreadCount;
    LARGE_INTEGER RdcountersCount;
    LARGE_INTEGER RdstateCount;
    LARGE_INTEGER WrperfmonCount;
    LARGE_INTEGER InitpcrCount;
    LARGE_INTEGER BptCount;
    LARGE_INTEGER CallsysCount;
    LARGE_INTEGER ImbCount;
    LARGE_INTEGER GentrapCount;
    LARGE_INTEGER RdtebCount;
    LARGE_INTEGER KbptCount;
    LARGE_INTEGER CallkdCount;
    LARGE_INTEGER TbisasnCount;
    LARGE_INTEGER Misc1Count;
    LARGE_INTEGER Misc2Count;
    LARGE_INTEGER Misc3Count;
} COUNTERS_21064, *PCOUNTERS_21064;

typedef enum _AXP21064_PCCOUNTER{
    Ev4PerformanceCounter0 = 0,
    Ev4PerformanceCounter1 = 1
} AXP21064_PCCOUNTER, *PAXP21064_PCCOUNTER;

typedef enum _AXP21064_PCMUXCONTROL{
    Ev4TotalIssues = 0x0,
    Ev4PipelineDry = 0x2,
    Ev4LoadInstruction = 0x4,
    Ev4PipelineFrozen = 0x6,
    Ev4BranchInstructions = 0x8,
    Ev4PalMode = 0xb,
    Ev4TotalCycles = 0xa,
    Ev4TotalNonIssues = 0xc,
    Ev4ExternalCounter0 = 0xe,
    Ev4DcacheMiss = 0x0,
    Ev4IcacheMiss = 0x1,
    Ev4DualIssues = 0x2,
    Ev4BranchMispredicts = 0x3,
    Ev4FPInstructions = 0x4,
    Ev4IntegerOperate = 0x5,
    Ev4StoreInstructions = 0x6,
    Ev4ExternalCounter1 = 0x7
} AXP21064_PCMUXCONTROL, *PAXP21064_PCMUXCONTROL;

typedef enum _AXP21064_PCEVENTCOUNT{
    Ev4CountEvents2xx8 = 0x100,
    Ev4CountEvents2xx12 = 0x1000,
    Ev4CountEvents2xx16 = 0x10000
} AXP21064_PCEVENTCOUNT, *PAXP21064_PCEVENTCOUNT;

typedef enum _AXP21064_EVENTCOUNT{
    Ev4EventCountHigh = 1,
    Ev4EventCountLow = 0
} AXP21064_EVENTCOUNT, *PAXP21064_EVENTCOUNT;

//
// Internal Processor Register definitions (read format).
//

//
// Pte formats
//

typedef LARGE_INTEGER ITB_PTE_21064;
typedef ITB_PTE_21064 *PITB_PTE_21064;
typedef LARGE_INTEGER DTB_PTE_21064;
typedef DTB_PTE_21064 *PDTB_PTE_21064;

#define PTE_FOR_21064_SHIFT 3
#define PTE_FOW_21064_SHIFT 4
#define PTE_KWE_21064_SHIFT 5
#define PTE_EWE_21064_SHIFT 6
#define PTE_SWE_21064_SHIFT 7
#define PTE_UWE_21064_SHIFT 8
#define PTE_KRE_21064_SHIFT 9
#define PTE_ERE_21064_SHIFT 10
#define PTE_SRE_21064_SHIFT 11
#define PTE_URE_21064_SHIFT 12 
#define PTE_PFN_21064_SHIFT 13
#define PTE_PFN_21064_SHIFTMASK  0x1FFFF
#define PTE_ASM_21064_SHIFT 34

#define PTE_ALL_21064(itbpte) (itbpte)
#define PTE_FOR_21064(itbpte) ( (itbpte.LowPart >> PTE_FOR_21064_SHIFT) & 1)
#define PTE_FOW_21064(itbpte) ( (itbpte.LowPart >> PTE_FOW_21064_SHIFT) & 1)
#define PTE_KWE_21064(itbpte) ( (itbpte.LowPart >> PTE_KWE_21064_SHIFT) & 1)
#define PTE_EWE_21064(itbpte) ( (itbpte.LowPart >> PTE_EWE_21064_SHIFT) & 1)
#define PTE_SWE_21064(itbpte) ( (itbpte.LowPart >> PTE_SWE_21064_SHIFT) & 1)
#define PTE_UWE_21064(itbpte) ( (itbpte.LowPart >> PTE_UWE_21064_SHIFT) & 1)
#define PTE_KRE_21064(itbpte) ( (itbpte.LowPart >> PTE_KRE_21064_SHIFT) & 1)
#define PTE_ERE_21064(itbpte) ( (itbpte.LowPart >> PTE_ERE_21064_SHIFT) & 1)
#define PTE_SRE_21064(itbpte) ( (itbpte.LowPart >> PTE_SRE_21064_SHIFT) & 1)
#define PTE_URE_21064(itbpte) ( (itbpte.LowPart >> PTE_URE_21064_SHIFT) & 1)
#define PTE_ASM_21064(itbpte) ( (itbpte.LowPart >> PTE_ASM_21064_SHIFT) & 1)
#define PTE_PFN_21064(itbpte) ( (itbpte.LowPart >> PTE_PFN_21064_SHIFT) & PTE_PFN_21064_SHIFTMASK)

//
// Instruction Cache Control and Status Register format
//

typedef LARGE_INTEGER ICCSR_21064;
typedef ICCSR_21064 *PICCSR_21064;

#define ICCSR_PC0_21064_SHIFT   1
#define ICCSR_PC1_21064_SHIFT   2
#define ICCSR_PCMUX0_21064_SHIFT 9
#define ICCSR_PCMUX0_21064_SHIFTMASK 0xF
#define ICCSR_PCMUX1_21064_SHIFT 13
#define ICCSR_PCMUX1_21064_SHIFTMASK 0x7
#define ICCSR_PIPE_21064_SHIFT 16
#define ICCSR_BPE_21064_SHIFT  17
#define ICCSR_JSE_21064_SHIFT  18
#define ICCSR_BHE_21064_SHIFT  19
#define ICCSR_DI_21064_SHIFT   20
#define ICCSR_HWE_21064_SHIFT  21
#define ICCSR_MAP_21064_SHIFT  22
#define ICCSR_FPE_21064_SHIFT  23
#define ICCSR_ASN_21064_SHIFT  28
#define ICCSR_ASN_21064_SHIFTMASK 0x3F

#define ICCSR_ALL_21064(iccsr) (iccsr)
#define ICCSR_PC0_21064(iccsr) ( (iccsr.LowPart >> ICCSR_PC0_21064_SHIFT) & 1)
#define ICCSR_PC1_21064(iccsr) ( (iccsr.LowPart >> ICCSR_PC1_21064_SHIFT) & 1)
#define ICCSR_PCMUX0_21064(iccsr) \
    ( (iccsr.LowPart >> ICCSR_PCMUX0_21064_SHIFT) & ICCSR_PCMUX0_21064_SHIFTMASK)
#define ICCSR_PCMUX1_21064(iccsr) \
    ( (iccsr.LowPart >> ICCSR_PCMUX1_21064_SHIFT) & ICCSR_PCMUX1_21064_SHIFTMASK)
#define ICCSR_PIPE_21064(iccsr) ( (iccsr.LowPart >> ICCSR_PIPE_21064_SHIFT) & 1)
#define ICCSR_BPE_21064(iccsr) ( (iccsr.LowPart >> ICCSR_BPE_21064_SHIFT) & 1)
#define ICCSR_JSE_21064(iccsr) ( (iccsr.LowPart >> ICCSR_JSE_21064_SHIFT) & 1)
#define ICCSR_BHE_21064(iccsr) ( (iccsr.LowPart >> ICCSR_BHE_21064_SHIFT) & 1)
#define ICCSR_DI_21064(iccsr)  ( (iccsr.LowPart >> ICCSR_DI_21064_SHIFT) & 1)
#define ICCSR_HWE_21064(iccsr) ( (iccsr.LowPart >> ICCSR_HWE_21064_SHIFT) & 1)
#define ICCSR_MAP_21064(iccsr) ( (iccsr.LowPart >> ICCSR_MAP_21064_SHIFT) & 1)
#define ICCSR_FPE_21064(iccsr) ( (iccsr.LowPart >> ICCSR_FPE_21064_SHIFT) & 1)
#define ICCSR_ASN_21064(iccsr) \
    (ULONG)( (iccsr.LowPart >> ICCSR_ASN_21064_SHIFT) & ICCSR_ASN_21064_SHIFTMASK)

//
// Processor Status (PS) format.
//

typedef LARGE_INTEGER PS_21064;
typedef PS_21064 *PPS_21064;

#define PS_CM0_21064_SHIFT 1
#define PS_CM1_21064_SHIFT 34

#define PS_ALL_21064(ps) (ps)
#define PS_CM_21064(ps) \
        ( (((ps).LowPart >> PS_CM0_21064_SHIFT) & 1) || \
          (((ps).LowPart >> (PS_CM1_21064_SHIFT-1)) & 1) )

//
// Exception Summary (EXC_SUM) format.
//

typedef LARGE_INTEGER EXC_SUM_21064;
typedef EXC_SUM_21064 *PEXC_SUM_21064;

#define EXCSUM_SWC_21064_SHIFT 2
#define EXCSUM_INV_21064_SHIFT 3
#define EXCSUM_DZE_21064_SHIFT 4
#define EXCSUM_FOV_21064_SHIFT 5
#define EXCSUM_UNF_21064_SHIFT 6
#define EXCSUM_INE_21064_SHIFT 7
#define EXCSUM_IOV_21064_SHIFT 8
#define EXCSUM_MSK_21064_SHIFT 33

#define EXCSUM_ALL_21064(excsum) (excsum)
#define EXCSUM_SWC_21064(excsum) ((excsum.LowPart >> EXCSUM_SWC_21064_SHIFT) & 0x1)
#define EXCSUM_INV_21064(excsum) ( (excsum.LowPart >> EXCSUM_INV_21064_SHIFT) & 0x1)
#define EXCSUM_DZE_21064(excsum) ( (excsum.LowPart >> EXCSUM_DZE_21064_SHIFT) & 0x1)
#define EXCSUM_FOV_21064(excsum) ( (excsum.LowPart >> EXCSUM_FOV_21064_SHIFT) & 0x1)
#define EXCSUM_UNF_21064(excsum) ( (excsum.LowPart >> EXCSUM_UNF_21064_SHIFT) & 0x1)
#define EXCSUM_INE_21064(excsum) ( (excsum.LowPart >> EXCSUM_INE_21064_SHIFT) & 0x1)
#define EXCSUM_IOV_21064(excsum) ( (excsum.LowPart >> EXCSUM_IOV_21064_SHIFT) & 0x1)
#define EXCSUM_MSK_21064(excsum) ( (excsum.LowPart >> EXCSUM_MSK_21064_SHIFT) & 0x1)

//
// Interrupt Request (HIRR, SIRR, ASTRR) format.
//

typedef LARGE_INTEGER IRR_21064;
typedef IRR_21064 *PIRR_21064;

#define IRR_HWR_21064_SHIFT 1
#define IRR_SWR_21064_SHIFT 2
#define IRR_ATR_21064_SHIFT 3
#define IRR_CRR_21064_SHIFT 4
#define IRR_HIRR53_21064_SHIFT 5
#define IRR_HIRR53_21064_SHIFTMASK 0x7
#define IRR_PC1_21064_SHIFT 8
#define IRR_PC0_21064_SHIFT 9
#define IRR_HIRR20_21064_SHIFT 10
#define IRR_HIRR20_21064_SHIFTMASK 0x7
#define IRR_SLR_21064_SHIFT 13
#define IRR_SIRR_21064_SHIFT 14
#define IRR_SIRR_21064_SHIFTMASK 0x7FFF
#define IRR_ASTRR_21064_SHIFT 29
#define IRR_ASTRR_21064_SHIFTMASK 0xF

#define IRR_ALL_21064(irr) (irr)
#define IRR_HWR_21064(irr) ( (irr.LowPart >> IRR_HWR_21064_SHIFT) & 0x1)
#define IRR_SWR_21064(irr) ( (irr.LowPart >> IRR_SWR_21064_SHIFT) & 0x1)
#define IRR_ATR_21064(irr) ( (irr.LowPart >> IRR_ATR_21064_SHIFT) & 0x1)
#define IRR_CRR_21064(irr) ( (irr.LowPart >> IRR_CRR_21064_SHIFT) & 0x1)
#define IRR_HIRR_21064(irr) \
    ( ((irr.LowPart >> (IRR_HIRR53_21064_SHIFT-3)) & IRR_HIRR53_21064_SHIFTMASK) || \
    ( (irr.LowPart >> IRR_HIRR20_21064_SHIFT) & IRR_HIRR20_21064_SHIFTMASK) )
#define IRR_PC1_21064(irr) ( (irr.LowPart >> IRR_PC1_21064_SHIFT) & 0x1)
#define IRR_PC0_21064(irr) ( (irr.LowPart >> IRR_PC0_21064_SHIFT) & 0x1)
#define IRR_SLR_21064(irr) ( (irr.LowPart >> IRR_SLR_21064_SHIFT) & 0x1)
#define IRR_SIRR_21064(irr) \
    ( (irr.LowPart >> IRR_SIRR_21064_SHIFT) & IRR_SIRR_21064_SHIFTMASK)
#define IRR_ASTRR_21064(irr) \
    ( (irr.LowPart >> IRR_ASTRR_21064_SHIFT) & IRR_ASTRR_21064_SHIFTMASK)

//
// Interrupt Enable (HIER, SIER, ASTER) format.
//

typedef LARGE_INTEGER IER_21064;
typedef IER_21064 *PIER_21064;

#define IER_CRR_21064_SHIFT 4
#define IER_HIER53_21064_SHIFT 5
#define IER_HIER53_21064_SHIFTMASK 0x7
#define IER_PC1_21064_SHIFT 8
#define IER_PC0_21064_SHIFT 9
#define IER_HIER20_21064_SHIFT 10
#define IER_HIER20_21064_SHIFTMASK 0x7
#define IER_SLR_21064_SHIFT 13
#define IER_SIER_21064_SHIFT 14
#define IER_SIER_21064_SHIFTMASK 0x7FFF
#define IER_ASTER_21064_SHIFT 29
#define IER_ASTER_21064_SHIFTMASK 0xF

#define IER_ALL_21064(ier) (ier)
#define IER_CRR_21064(ier) ( (ier.LowPart >> IER_CRR_21064_SHIFT) & 0x1)
#define IER_HIER_21064(ier) \
    ( ( (ier.LowPart >> (IER_HIER53_21064_SHIFT-3)) & IER_HIER53_21064_SHIFTMASK) || \
      ( (ier.LowPart >> IER_HIER20_21064_SHIFT) & IER_HIER20_21064_SHIFTMASK) )
#define IER_PC1_21064(ier) ( (ier.LowPart >> IER_PC1_21064_SHIFT) & 0x1)
#define IER_PC0_21064(ier) ( (ier.LowPart >> IER_PC0_21064_SHIFT) & 0x1)
#define IER_SLR_21064(ier) ( (ier.LowPart >> IER_SLR_21064_SHIFT) & 0x1)
#define IER_SIER_21064(ier) \
    ( (ier.LowPart >> IER_SIER_21064_SHIFT) & IER_SIER_21064_SHIFTMASK)
#define IER_ASTER_21064(ier) \
    ( (ier.LowPart >> IER_ASTER_21064_SHIFT) & IER_ASTER_21064_SHIFTMASK)

//
// Abox Control Register (ABOX_CTL) format.
//

typedef union _ABOX_CTL_21064{
    struct {
        ULONG wb_dis: 1;
        ULONG mchk_en: 1;
        ULONG crd_en: 1;
        ULONG ic_sbuf_en: 1;
        ULONG spe_1: 1;
        ULONG spe_2: 1;
        ULONG emd_en: 1;
        ULONG mbz1: 3;
        ULONG dc_ena: 1;
        ULONG dc_fhit: 1;
    } bits;
    LARGE_INTEGER all;
} ABOX_CTL_21064, *PABOX_CTL_21064;

#define ABOXCTL_ALL_21064(aboxctl) ((aboxctl).all)
#define ABOXCTL_WBDIS_21064(aboxctl) ((aboxctl).bits.wb_dis)
#define ABOXCTL_MCHKEN_21064(aboxctl) ((aboxctl).bits.mchk_en)
#define ABOXCTL_CRDEN_21064(aboxctl) ((aboxctl).bits.crd_en)
#define ABOXCTL_ICSBUFEN_21064(aboxctl) ((aboxctl).bits.ic_sbuf_en)
#define ABOXCTL_SPE1_21064(aboxctl) ((aboxctl).bits.spe_1)
#define ABOXCTL_SPE2_21064(aboxctl) ((aboxctl).bits.spe_2)
#define ABOXCTL_EMDEN_21064(aboxctl) ((aboxctl).bits.emd_en)
#define ABOXCTL_DCENA_21064(aboxctl) ((aboxctl).bits.dc_ena)
#define ABOXCTL_DCFHIT_21064(aboxctl) ((aboxctl).bits.dc_fhit)

//
// Memory Management Control and Status Register (MMCSR) format.
//

typedef union _MMCSR_21064{
    struct {
        ULONG Wr: 1;
        ULONG Acv: 1;
        ULONG For: 1;
        ULONG Fow: 1;
        ULONG Ra: 5;
        ULONG Opcode: 6;
    } bits;
    LARGE_INTEGER all;
} MMCSR_21064, *PMMCSR_21064;

#define MMCSR_ALL_21064(mmcsr) ((mmcsr).all)
#define MMCSR_WR_21064(mmcsr) ((mmcsr).bits.Wr)
#define MMCSR_ACV_21064(mmcsr) ((mmcsr).bits.Acv)
#define MMCSR_FOR_21064(mmcsr) ((mmcsr).bits.For)
#define MMCSR_FOW_21064(mmcsr) ((mmcsr).bits.Fow)
#define MMCSR_RA_21064(mmcsr) ((mmcsr).bits.Ra)
#define MMCSR_OPCODE_21064(mmcsr) ((mmcsr).bits.Opcode)

//
// Dcache Status (DC_STAT) format.
//
typedef union _DC_STAT_21064{
    struct {
        ULONG Reserved: 3;
        ULONG DcHit: 1;
        ULONG DCacheParityError: 1;
        ULONG ICacheParityError: 1;
    } bits;
    LARGE_INTEGER all;
} DC_STAT_21064, *PDC_STAT_21064;

#define DCSTAT_ALL_21064(dcstat) ((dcstat).all)
#define DCSTAT_DCHIT_21064(dcstat) ((dcstat).bits.DcHit)
#define DCSTAT_DCPARITY_ERROR_21064(dcstat) ((dcstat).bits.DCacheParityError)
#define DCSTAT_ICPARITY_ERROR_21064(dcstat) ((dcstat).bits.ICacheParityError)

#endif //!CORE_21064

//end_axp21064

//
// Define the number of banks in the memory controller.
//

#define MEMORY_BANKS_21066 (4)

//
// Define the base physical addresses for the integrated
// memory and I/O controllers.
//

#define MEMORY_CONTROLLER_PHYSICAL_21066 (0x120000000)
#define IO_CONTROLLER_PHYSICAL_21066 (0x180000000)

//
// Define the memory controller CSR structure.
//

typedef struct _MEMC_CSRS_21066{
    LARGE_INTEGER Bcr0;
    LARGE_INTEGER Bcr1;
    LARGE_INTEGER Bcr2;
    LARGE_INTEGER Bcr3;
    LARGE_INTEGER Bmr0;
    LARGE_INTEGER Bmr1;
    LARGE_INTEGER Bmr2;
    LARGE_INTEGER Bmr3;
    LARGE_INTEGER Btr0;
    LARGE_INTEGER Btr1;
    LARGE_INTEGER Btr2;
    LARGE_INTEGER Btr3;
    LARGE_INTEGER Gtr;
    LARGE_INTEGER Esr;
    LARGE_INTEGER Ear;
    LARGE_INTEGER Car;
    LARGE_INTEGER Vgr;
    LARGE_INTEGER Plm;
    LARGE_INTEGER For;
} MEMC_CSRS_21066, *PMEMC_CSRS_21066;

//
// Define the i/o controller CSR structure.
//

typedef struct _IOC_CSRS_21066{
    LARGE_INTEGER Hae;
    LARGE_INTEGER Filler1[3];
    LARGE_INTEGER Cct;
    LARGE_INTEGER Filler2[3];
    LARGE_INTEGER IoStat0;
    LARGE_INTEGER Filler3[3];
    LARGE_INTEGER IoStat1;
    LARGE_INTEGER Filler4[3];
    LARGE_INTEGER Tbia;
    LARGE_INTEGER Filler5[3];
    LARGE_INTEGER Tben;
    LARGE_INTEGER Filler6[3];
    LARGE_INTEGER PciSoftReset;
    LARGE_INTEGER Filler7[3];
    LARGE_INTEGER PciParityDisable;
    LARGE_INTEGER Filler8[3];
    LARGE_INTEGER Wbase0;
    LARGE_INTEGER Filler9[3];
    LARGE_INTEGER Wbase1;
    LARGE_INTEGER Filler10[3];
    LARGE_INTEGER Wmask0;
    LARGE_INTEGER Filler11[3];
    LARGE_INTEGER Wmask1;
    LARGE_INTEGER Filler12[3];
    LARGE_INTEGER Tbase0;
    LARGE_INTEGER Filler13[3];
    LARGE_INTEGER Tbase1;
    LARGE_INTEGER Filler14[3];
} IOC_CSRS_21066, *PIOC_CSRS_21066;


//
// Bank Configuration Registers (BCR0 - BCR3)
//

typedef union _BCR_21066{
    struct {
        ULONG Reserved1: 6;
        ULONG Ras: 4;
        ULONG Erm: 1;
        ULONG Wrm: 1;
        ULONG Bwe: 1;
        ULONG Sbe: 1;
        ULONG Bav: 1;
        ULONG Reserved2: 5;
        ULONG BankBase: 9;
        ULONG Reserved3: 3;
    } ;
    LARGE_INTEGER all;
} BCR_21066, *PBCR_21066;

//
// Bank Address Mask Registers (BMR0 - BMR3)
//

typedef union _BMR_21066{
    struct {
        ULONG Reserved1: 20;
        ULONG BankAddressMask: 9;
        ULONG Reserved2: 3;
    } ;
    LARGE_INTEGER all;
} BMR_21066, *PBMR_21066;

//
// Global Timing Register (GTR)
//

typedef union _GTR_21066{
    struct {
        ULONG Precharge: 5;
        ULONG MinimumRas: 5;
        ULONG MaximumRas: 8;
        ULONG RefreshEnable: 1;
        ULONG RefreshInterval: 8;
        ULONG RefreshDivideSelect: 1;
        ULONG Setup: 4;
    } ;
    LARGE_INTEGER all;
} GTR_21066, *PGTR_21066;

//
// Error Status Register (ESR)
//

typedef union _ESR_21066{
    struct {
        ULONG Eav: 1;
        ULONG Cee: 1;
        ULONG Uee: 1;
        ULONG Wre: 1;
        ULONG Sor: 1;
        ULONG Reserved1: 2;
        ULONG Cte: 1;
        ULONG Reserved2: 1;
        ULONG Mse: 1;
        ULONG Mhe: 1;
        ULONG Ice: 1;
        ULONG Nxm: 1;
        ULONG Reserved3: 19;
        ULONG Ecc0: 1;       
        ULONG Wec6: 1;
        ULONG Wec3: 1;
        ULONG Reserved4: 1;
        ULONG Ecc1: 1;
        ULONG Reserved5: 3;
        ULONG Wec7: 1;
        ULONG Ecc2: 1;
        ULONG Wec2: 1;
        ULONG Reserved6: 2;
        ULONG Ecc3: 1;
        ULONG Reserved7: 2;
        ULONG Wec4: 1;
        ULONG Wec1: 1;
        ULONG Ecc4: 1;
        ULONG Wec0: 1;
        ULONG Reserved8: 2;
        ULONG Ecc5: 1;
        ULONG Reserved9: 4;
        ULONG Ecc6: 1;
        ULONG Wec5: 1;
        ULONG Reserved10: 2;
        ULONG Ecc7: 1;       
    } ;
    LARGE_INTEGER all;
} ESR_21066, *PESR_21066;

//
// Error Address Register (EAR)
//

typedef union _EAR_21066{
    struct {
        ULONG PerfCntMux0: 3;
        ULONG ErrorAddress: 26;
        ULONG PerfCntMux1: 3;
    } ;
    LARGE_INTEGER all;
} EAR_21066, *PEAR_21066;


//
// Cache Register (CAR)
//

typedef union _CAR_21066{
    struct {
        ULONG Bce: 1;
        ULONG Reserved1: 1;
        ULONG Etp: 1;
        ULONG Wwp: 1;
        ULONG Ece: 1;
        ULONG BCacheSize: 3;
        ULONG ReadCycles: 3;
        ULONG WriteCycles: 3;
        ULONG Whd: 1;
        ULONG Pwr: 1;
        ULONG Tag: 15;
        ULONG Hit: 1;
    } ;
    LARGE_INTEGER all;
} CAR_21066, *PCAR_21066;

       
//
// IOC Status 0 Registers (IOC_STAT0)
//

typedef union _IOC_STAT0_21066{
    struct {
        ULONG Cmd: 4;
        ULONG Err: 1;
        ULONG Lost: 1;
        ULONG Thit: 1;
        ULONG Tref: 1;
        ULONG Code: 3;
        ULONG Reserved1: 2;
        ULONG PageNumber: 19;
    } ;
    LARGE_INTEGER all;
} IOC_STAT0_21066, *PIOC_STAT0_21066;


//
// IOC Status 1 Register (IOC_STAT1)
//

typedef union _IOC_STAT1_21066{
    struct {
        ULONG Address: 32;
    } ;
    LARGE_INTEGER all;
} IOC_STAT1_21066, *PIOC_STAT1_21066;


//
// Internal Processor State record.
// This is the structure of the data returned by the rdstate call pal.
//

typedef struct _PROCESSOR_STATE_21066{
    ABOX_CTL_21064 AboxCtl;
    IER_21064 Aster;
    IRR_21064 Astrr;
    BCR_21066 BankConfig[ MEMORY_BANKS_21066 ];
    BMR_21066 BankMask[ MEMORY_BANKS_21066 ];
    DC_STAT_21064 DcStat;
    DTB_PTE_21064 DtbPte[ DTB_ENTRIES_21064 ];
    EXC_SUM_21064 ExcSum;
    IER_21064 Hier;
    IRR_21064 Hirr;
    ICCSR_21064 Iccsr;
    ITB_PTE_21064 ItbPte[ ITB_ENTRIES_21064 ];
    MMCSR_21064 MmCsr;
    LARGE_INTEGER PalBase;
    LARGE_INTEGER PalTemp[ PAL_TEMPS_21064 ];
    PS_21064 Ps;
    IER_21064 Sier;
    IRR_21064 Sirr;
    LARGE_INTEGER Va;
} PROCESSOR_STATE_21066, *PPROCESSOR_STATE_21066;

//
// Machine-check logout frame.
//

typedef struct _LOGOUT_FRAME_21066{
    ABOX_CTL_21064 AboxCtl;
    BCR_21066 BankConfig[ MEMORY_BANKS_21066 ];
    BMR_21066 BankMask[ MEMORY_BANKS_21066 ];
    DC_STAT_21064 DcStat;
    LARGE_INTEGER ExcAddr;
    EXC_SUM_21064 ExcSum;
    IER_21064 Hier;
    IRR_21064 Hirr;
    ICCSR_21064 Iccsr;
    MMCSR_21064 MmCsr;
    LARGE_INTEGER PalBase;
    LARGE_INTEGER PalTemp[ PAL_TEMPS_21064 ];
    PS_21064 Ps;
    LARGE_INTEGER Va;
} LOGOUT_FRAME_21066, *PLOGOUT_FRAME_21066;

//
// Correctable Machine-check logout frame.
//

typedef struct _CORRECTABLE_FRAME_21066{
    BCR_21066 BankConfig[ MEMORY_BANKS_21066 ];
    BMR_21066 BankMask[ MEMORY_BANKS_21066 ];
    DC_STAT_21064 DcStat;
} CORRECTABLE_FRAME_21066;

//
// Define the physical and virtual address bits
//

#define LCA_PHYSICAL_ADDRESS_BITS     34
#define LCA_VIRTUAL_ADDRESS_BITS      43

#endif //!_AXP21066_  
