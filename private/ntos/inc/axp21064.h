/*++ BUILD Version: 0000     Increment this if a change has global effects

Copyright (c) 1993 Digital Euipment Corporation

Module Name:

    axp21064.h

Abstract:

    This module defines the DECchip 21064-specific structures that are
    defined in the PAL but must be visible to the HAL.

Revision History:

--*/

#ifndef _AXP21064_
#define _AXP21064_

//begin_axp21066

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


//
// Bus Interface Unit Status (BIU_STAT) format.
//

typedef union _BIU_STAT_21064{
    struct {
        ULONG BiuHerr: 1;
        ULONG BiuSerr: 1;
        ULONG BcTperr: 1;
        ULONG BcTcperr: 1;
        ULONG BiuCmd: 3;
        ULONG Fatal1: 1;
        ULONG FillEcc: 1;
        ULONG Reserved: 1;
        ULONG FillDperr: 1;
        ULONG FillIrd: 1;
        ULONG FillQw: 2;
        ULONG Fatal2: 1;
    } bits;
    LARGE_INTEGER all;
} BIU_STAT_21064, *PBIU_STAT_21064;

#define BIUSTAT_ALL_21064(biustat) ((biustat).all)
#define BIUSTAT_HERR_21064(biustat) ((biustat).bits.BiuHerr)
#define BIUSTAT_SERR_21064(biustat) ((biustat).bits.BiuSerr)
#define BIUSTAT_TPERR_21064(biustat) ((biustat).bits.BcTperr)
#define BIUSTAT_TCPERR_21064(biustat) ((biustat).bits.BcTcperr)
#define BIUSTAT_CMD_21064(biustat) ((biustat).bits.BiuCmd)
#define BIUSTAT_FATAL1_21064(biustat) ((biustat).bits.Fatal1)
#define BIUSTAT_FILLECC_21064(biustat) ((biustat).bits.FillEcc)
#define BIUSTAT_FILLDPERR_21064(biustat) ((biustat).bits.FillDperr)
#define BIUSTAT_FILLIRD_21064(biustat) ((biustat).bits.FillIrd)
#define BIUSTAT_FILLQW_21064(biustat) ((biustat).bits.FillQw)
#define BIUSTAT_FATAL2_21064(biustat) ((biustat).bits.Fatal2)

//
// Fill Syndrome (FILL_SYNDROME) format.
//

typedef union _FILL_SYNDROME_21064{
    struct {
        ULONG Lo: 7;
        ULONG Hi: 7;
    } bits;
    LARGE_INTEGER all;
} FILL_SYNDROME_21064, *PFILL_SYNDROME_21064;

#define FILLSYNDROME_ALL_21064(fs) ((fs).all)
#define FILLSYNDROME_LO_21064(fs) ((fs).bits.Lo)
#define FILLSYNDROME_HI_21064(fs) ((fs).bits.Hi)

//
// Backup Cache Tag (BC_TAG) format.
//

typedef union _BC_TAG_21064{
    struct {
        ULONG Hit: 1;
        ULONG TagctlP: 1;
        ULONG TagctlD: 1;
        ULONG TagctlS: 1;
        ULONG TagctlV: 1;
        ULONG Tag: 17;
        ULONG TagP: 1;
    } bits;
    LARGE_INTEGER all;
} BC_TAG_21064, *PBC_TAG_21064;

#define BCTAG_ALL_21064(bctag) ((bctag).all)
#define BCTAG_HIT_21064(bctag) ((bctag).bits.Hit)
#define BCTAG_TAGCTLP_21064(bctag) ((bctag).bits.TagctlP)
#define BCTAG_TAGCTLD_21064(bctag) ((bctag).bits.TagctlD)
#define BCTAG_TAGCTLS_21064(bctag) ((bctag).bits.TagctlS)
#define BCTAG_TAGCTLV_21064(bctag) ((bctag).bits.TagctlV)
#define BCTAG_TAG_21064(bctag) ((bctag).bits.Tag)
#define BCTAG_TAGP_21064(bctag) ((bctag).bits.TagP)

//
// Bus Interface Unit Control Register (BIU_CTL) format.
//

typedef LARGE_INTEGER BIU_CTL_21064;
typedef BIU_CTL_21064 *PBIU_CTL_21064;

#define BIUCTL_BCENA_21064_SHIFT 0
#define BIUCTL_ECC_21064_SHIFT 1
#define BIUCTL_OE_21064_SHIFT 2
#define BIUCTL_BCFHIT_21064_SHIFT 3
#define BIUCTL_BCRDSPD_21064_SHIFT 4
#define BIUCTL_BCRDSPD_21064_SHIFTMASK 0xF
#define BIUCTL_BCWRSPD_21064_SHIFT 8
#define BIUCTL_BCWRSPD_21064_SHIFTMASK 0xF
#define BIUCTL_BCWECTL_21064_SHIFT 12
#define BIUCTL_BCWECTL_21064_SHIFTMASK 0xFFFF
#define BIUCTL_BCSIZE_21064_SHIFT 28
#define BIUCTL_BCSIZE_21064_SHIFTMASK 0x7
#define BIUCTL_BADTCP_21064_SHIFT 31
#define BIUCTL_BCPADIS_21064_SHIFT 32
#define BIUCTL_BCPADIS_21064_SHIFTMASK 0xF
#define BIUCTL_BADDP_21064_SHIFT 36

#define BIUCTL_ALL_21064(biuctl) (biuctl)
#define BIUCTL_BCENA_21064(biuctl) ( (biuctl.LowPart >> BIUCTL_BCENA_21064_SHIFT) & 1)
#define BIUCTL_ECC_21064(biuctl) ( (biuctl.LowPart >> BIUCTL_ECC_21064_SHIFT) & 1)
#define BIUCTL_OE_21064(biuctl) ( (biuctl.LowPart >> BIUCTL_OE_21064_SHIFT) & 1)
#define BIUCTL_BCFHIT_21064(biuctl) ( (biuctl.LowPart >> BIUCTL_BCFHIT_21064_SHIFT) & 1)
#define BIUCTL_BCRDSPD_21064(biuctl) \
    ( (biuctl.LowPart >> BIUCTL_BCRDSPD_21064_SHIFT) & BIUCTL_BCRDSPD_21064_SHIFTMASK)
#define BIUCTL_BCWRSPD_21064(biuctl) \
    ( (biuctl.LowPart >> BIUCTL_BCWRSPD_21064_SHIFT) & BIUCTL_BCWRSPD_21064_SHIFTMASK)
#define BIUCTL_BCWECTL_21064(biuctl) \
    ( (biuctl.LowPart >> BIUCTL_BCWECTL_21064_SHIFT) & BIUCTL_BCWECTL_21064_SHIFTMASK)
#define BIUCTL_BCSIZE_21064(biuctl) \
    ( (biuctl.LowPart >> BIUCTL_BCSIZE_21064_SHIFT) & BIUCTL_BCSIZE_21064_SHIFTMASK)
#define BIUCTL_BADTCP_21064(biuctl) \
    ( (biuctl.LowPart >> BIUCTL_BADTCP_21064_SHIFT) & 1)
#define BIUCTL_BCPADIS_21064(biuctl) \
    ( (biuctl.LowPart >> BIUCTL_BCPADIS_21064_SHIFT) & BIUCTL_BCPADIS_21064_SHIFTMASK)
#define BIUCTL_BADDP_21064(biuctl) \
    ( (biuctl.LowPart >> BIUCTL_BADDP_21064_SHIFT) & 1)

//
// Internal Processor State record.
// This is the structure of the data returned by the rdstate call pal.
//

typedef struct _PROCESSOR_STATE_21064{
    ITB_PTE_21064 ItbPte[ ITB_ENTRIES_21064 ];
    ICCSR_21064 Iccsr;
    PS_21064 Ps;
    EXC_SUM_21064 ExcSum;
    LARGE_INTEGER PalBase;
    IRR_21064 Hirr;
    IRR_21064 Sirr;
    IRR_21064 Astrr;
    IER_21064 Hier;
    IER_21064 Sier;
    IER_21064 Aster;
    ABOX_CTL_21064 AboxCtl;
    DTB_PTE_21064 DtbPte[ DTB_ENTRIES_21064 ];
    MMCSR_21064 MmCsr;
    LARGE_INTEGER Va;
    LARGE_INTEGER PalTemp[ PAL_TEMPS_21064 ];
    BIU_CTL_21064 BiuCtl;
    DC_STAT_21064 DcStat;
    BIU_STAT_21064 BiuStat;
    LARGE_INTEGER BiuAddr;
    LARGE_INTEGER FillAddr;
    FILL_SYNDROME_21064 FillSyndrome;
} PROCESSOR_STATE_21064, *PPROCESSOR_STATE_21064;


//
// Machine-check logout frame.
//

typedef struct _LOGOUT_FRAME_21064{
    BIU_STAT_21064 BiuStat;
    LARGE_INTEGER BiuAddr;
    BC_TAG_21064 BcTag;
    LARGE_INTEGER ExcAddr;
    LARGE_INTEGER FillAddr;
    FILL_SYNDROME_21064 FillSyndrome;
    DC_STAT_21064 DcStat;
    ICCSR_21064 Iccsr;
    PS_21064 Ps;
    EXC_SUM_21064 ExcSum;
    LARGE_INTEGER PalBase;
    IRR_21064 Hirr;
    IER_21064 Hier;
    ABOX_CTL_21064 AboxCtl;
    BIU_CTL_21064 BiuCtl;
    MMCSR_21064 MmCsr;
    LARGE_INTEGER Va;
    LARGE_INTEGER PalTemp[ PAL_TEMPS_21064 ];
} LOGOUT_FRAME_21064, *PLOGOUT_FRAME_21064;

//
// Correctable Machine-check logout frame.
//

typedef struct _CORRECTABLE_FRAME_21064{
    BIU_STAT_21064 BiuStat;
    LARGE_INTEGER BiuAddr;
    BC_TAG_21064 BcTag;
    LARGE_INTEGER FillAddr;
    FILL_SYNDROME_21064 FillSyndrome;
    DC_STAT_21064 DcStat;
} CORRECTABLE_FRAME_21064;

//
// Define the physical and virtual address bits
//

#define EV4_PHYSICAL_ADDRESS_BITS     34
#define EV4_VIRTUAL_ADDRESS_BITS      43

#endif //!_AXP21064_  
