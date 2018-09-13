/*++ BUILD Version: 0000     Increment this if a change has global effects

Copyright (c) 1994 Digital Euipment Corporation

Module Name:

    axp21164.h

Abstract:

    This module defines the DECchip 21164-specific structures that are
    defined in the PAL but must be visible to the HAL.

Revision History:

--*/

#ifndef _AXP21164_
#define _AXP21164_


//
// Define the "special" processor bus used by all machines that run a
// DECchip 21164.  The processor bus is used to access the internal
// performance counters.
//

#define PROCESSOR_BUS_21164 21164

//
// Define the physical address bit that turns on user-mode access
// to I/O space in the pfn of a pte.  This bit is required because of
// the current 36 bit physical address space limit on NT.
//

#define EV5_USER_IO_ADDRESS_SPACE (ULONGLONG)(0x800000000)

#define EV5_IO_BASE_PHYSICAL 0x8000000000

//
// Define the number of entries for repeated internal processor registers.
//

#define ITB_ENTRIES_21164 32
#define DTB_ENTRIES_21164 64
#define PAL_TEMPS_21164   24

//
// Define the Ibox Internal Processor Register formats.
//

//
// Define the ITB_PTE - write format.
//

typedef union _ITB_PTE_21164{
    struct {
        ULONG Ignore1: 4;
        ULONG Asm: 1;
        ULONG Gh: 2;
        ULONG Ignore2: 1;
        ULONG Kre: 1;
        ULONG Ere: 1;
        ULONG Sre: 1;
        ULONG Ure: 1;
        ULONG Ignore3: 20;
        ULONG Pfn: 27;
        ULONG Ignore4: 5;
    } ;
    ULONGLONG all;
} ITB_PTE_21164, *PITB_PTE_21164;

//
// Define the ITB_PTE_TEMP - read format.
//

typedef union _ITB_PTE_TEMP_21164{
    struct {
        ULONG Raz1: 13;
        ULONG Asm: 1;
        ULONG Raz2: 4;
        ULONG Kre: 1;
        ULONG Ere: 1;
        ULONG Sre: 1;
        ULONG Ure: 1;
        ULONG Raz3: 7;
        ULONG Ghd: 3;
        ULONG Pfn: 27;
        ULONG Raz4: 5;
    } ;
    ULONGLONG all;
} ITB_PTE_TEMP_21164, *PITB_PTE_TEMP_21164;

//
// Define the ITB_ASN.
//

typedef union _ITB_ASN_21164{
    struct {
        ULONG Raz1: 4;
        ULONG Asn: 7;
        ULONG Raz2: 21;
        ULONG Raz3: 32;
    } ;
    ULONGLONG all;
} ITB_ASN_21164, *PITB_ASN_21164;

//
// Define the ICPERR_STAT.
//

typedef union _ICPERR_STAT_21164{
    struct {
        ULONG Raz1: 11;
        ULONG Dpe: 1;
        ULONG Tpe: 1;
        ULONG Tmr: 1;
        ULONG Raz2: 18;
        ULONG Raz3: 32;
    } ;
    ULONGLONG all;
} ICPERR_STAT_21164, *PICPERR_STAT_21164;

//
// Define the EXC_SUM.
//

typedef union _EXC_SUM_21164{
    struct {
        ULONG Raz1: 10;
        ULONG Swc: 1;
        ULONG Inv: 1;
        ULONG Dze: 1;
        ULONG Fov: 1;
        ULONG Unf: 1;
        ULONG Ine: 1;
        ULONG Iov: 1;
        ULONG Raz2: 15;
        ULONG Raz3: 32;
    } ;
    ULONGLONG all;
} EXC_SUM_21164, *PEXC_SUM_21164;

//
// Define the PS.
//

typedef union _PS_21164{
    struct {
        ULONG Raz1: 3;
        ULONG Cm0: 1;
        ULONG Cm1: 1;
        ULONG Raz2: 27;
        ULONG Raz3: 32;
    } ;
    ULONGLONG all;
} PS_21164, *PPS_21164;

//
// Define the ICSR.
//

typedef union _ICSR_21164{
    struct {
        ULONG Raz1: 8;
        ULONG Pme: 2;
        ULONG Raz2: 7;
        ULONG Byte: 1;
        ULONG Raz3: 1;
        ULONG Mve: 1;   // PCA56
        ULONG Imsk: 4;
        ULONG Tmm: 1;
        ULONG Tmd: 1;
        ULONG Fpe: 1;
        ULONG Hwe: 1;
        ULONG Sp32: 1;
        ULONG Sp43: 1;
        ULONG Sde: 1;
        ULONG Raz4: 1;
        ULONG Crde: 1;
        ULONG Sle: 1;
        ULONG Fms: 1;
        ULONG Fbt: 1;
        ULONG Fbd: 1;
        ULONG Dbs: 1;
        ULONG Ista: 1;
        ULONG Tst: 1;
        ULONG Raz5: 24;
    } ;
    ULONGLONG all;
} ICSR_21164, *PICSR_21164;


//
// Define the SIRR.
//

typedef union _SIRR_21164{
    struct {
        ULONG Raz1: 4;
        ULONG Sir: 15;
        ULONG Raz2: 13;
        ULONG Raz3: 32;
    } ;
    ULONGLONG all;
} SIRR_21164, *PSIRR_21164;

//
// Define the HWINT_CLR.
//

typedef union _HWINT_CLR_21164{
    struct {
        ULONG Raz1: 27;
        ULONG Pc0c: 1;
        ULONG Pc1c: 1;
        ULONG Pc2c: 1;
        ULONG Raz2: 2;
        ULONG Crdc: 1;
        ULONG Slc: 1;
        ULONG Raz3: 30;
    } ;
    ULONGLONG all;
} HWINT_CLR_21164, *PHW_INTCLR_21164;

//
// Define the ISR.
//

typedef union _ISR_21164{
    struct {
        ULONG Ast: 4;
        ULONG Sirr: 15;
        ULONG Atr: 1;
        ULONG I20: 1;
        ULONG I21: 1;
        ULONG I22: 1;
        ULONG I23: 1;
        ULONG Raz1: 3;
        ULONG Pc0: 1;
        ULONG Pc1: 1;
        ULONG Pc2: 1;
        ULONG Pfl: 1;
        ULONG Mck: 1;
        ULONG Crd: 1;
        ULONG Sli: 1;
        ULONG Hlt: 1;
        ULONG Raz2: 29;
    } ;
    ULONGLONG all;
} ISR_21164, *PISR_21164;

//
// Define the PMCTR.
//

typedef union _PMCTR_21164{
    struct {
        ULONG Sel2: 4;
        ULONG Sel1: 4;
        ULONG Kk: 1;
        ULONG Kp: 1;
        ULONG Ctl2: 2;
        ULONG Ctl1: 2;
        ULONG Ctl0: 2;
        ULONG Ctr2: 14;
        ULONG Ku: 1;
        ULONG Sel0: 1;
        ULONG Ctr1: 16;
        ULONG Ctr0: 16;
    } ;
    ULONGLONG all;
} PMCTR_21164, *PPMCTR_21164;

//
// Define the Mbox and Dcache Internal Processor Register formats.
//

//
// Define the DTB_ASN.
//

typedef union _DTB_ASN_21164{
    struct {
        ULONG Raz1: 32;
        ULONG Raz2: 25;
        ULONG Asn: 7;
    } ;
    ULONGLONG all;
} DTB_ASN_21164, *PDTB_ASN_21164;

//
// Define the DTB_CM.
//

typedef union _DTB_CM_21164{
    struct {
        ULONG Raz1: 3;
        ULONG Cm0: 1;
        ULONG Cm1: 1;
        ULONG Raz2: 27;
        ULONG Raz3: 32;
    } ;
    ULONGLONG all;
} DTB_CM_21164, *PDTB_CM_21164;

//
// Define the DTB_PTE.
//

typedef union _DTB_PTE_21164{
    struct {
        ULONG Ignore1: 1;
        ULONG For: 1;
        ULONG Fow: 1;
        ULONG Ignore2: 1;
        ULONG Asm: 1;
        ULONG Gh: 2;
        ULONG Ignore3: 1;
        ULONG Kre: 1;
        ULONG Ere: 1;
        ULONG Sre: 1;
        ULONG Ure: 1;
        ULONG Kwe: 1;
        ULONG Ewe: 1;
        ULONG Swe: 1;
        ULONG Uwe: 1;
        ULONG Ignore4: 16;
        ULONG Pfn: 27;
        ULONG Ignore5: 5;
    } ;
    ULONGLONG all;
} DTB_PTE_21164, *PDTB_PTE_21164;

//
// Define the DTB_PTE_TEMP.
//

typedef union _DTB_PTE_TEMP_21164{
    struct {
        ULONG For: 1;
        ULONG Fow: 1;
        ULONG Kre: 1;
        ULONG Ere: 1;
        ULONG Sre: 1;
        ULONG Ure: 1;
        ULONG Kwe: 1;
        ULONG Ewe: 1;
        ULONG Swe: 1;
        ULONG Uwe: 1;
        ULONG Raz1: 3;
        ULONG Pfn31_13: 19;
        ULONG Pfn39_32: 8;
        ULONG Raz2: 24;
    } ;
    ULONGLONG all;
} DTB_PTE_TEMP_21164, *PDTB_PTE_TEMP_21164;

//
// Define the MM_STAT.
//

typedef union _MM_STAT_21164{
    struct {
        ULONG Wr: 1;
        ULONG Acv: 1;
        ULONG For: 1;
        ULONG Fow: 1;
        ULONG DtbMiss: 1;
        ULONG BadVa: 1;
        ULONG Ra: 5;
        ULONG Opcode: 6;
        ULONG Raz1: 15;
        ULONG Raz2: 32;
    } ;
    ULONGLONG all;
} MM_STAT_21164, *PMM_STAT_21164;

//
// Define the DC_PERR_STAT.
//

typedef union _DC_PERR_STAT_21164{
    struct {
        ULONG Seo: 1;
        ULONG Lock: 1;
        ULONG Dp0: 1;
        ULONG Dp1: 1;
        ULONG Tp0: 1;
        ULONG Tp1: 1;
        ULONG Raz1: 26;
        ULONG Raz2: 32;
    } ;
    ULONGLONG all;
} DC_PERR_STAT_21164, *PDC_PERR_STAT_21164;

//
// Define the MCSR.
//

typedef union _MCSR_21164{
    struct {
        ULONG MBigEndian: 1;
        ULONG Sp32: 1;
        ULONG Sp43: 1;
        ULONG DbgTestSel0: 1;
        ULONG EBigEndian: 1;
        ULONG DbgTestSel1: 1;
        ULONG Raz1: 26;
        ULONG Raz2: 32; 
    } ;
    ULONGLONG all;
} MCSR_21164, *PMCSR_21164;

//
// Define the DC_MODE.
//

typedef union _DC_MODE_21164{
    struct {
        ULONG DcEna: 1;
        ULONG DcFhit: 1;
        ULONG DcBadParity: 1;
        ULONG DcPerrDisable: 1;
        ULONG DcDoa: 1;
        ULONG Raz1: 27;
        ULONG Raz2: 32;
    } ;
    ULONGLONG all;
} DC_MODE_21164, *PDC_MODE_21164;

//
// Define the MAF_MODE.
//

typedef union _MAF_MODE_21164{
    struct {
        ULONG DreadNomerge: 1;
        ULONG WbFlushAlways: 1;
        ULONG WbNomerge: 1;
        ULONG IoNomerge: 1;
        ULONG WbCntDisable: 1;
        ULONG MafArbDisable: 1;
        ULONG DreadPending: 1;
        ULONG WbPending: 1;
        ULONG Raz1: 24;
        ULONG Raz2: 32;
    } ;
    ULONGLONG all;
} MAF_MODE_21164, *PMAF_MODE_21164;

//
// Define the ALT_MODE.
//

typedef union _ALT_MODE_21164{
    struct {
        ULONG Ignore1: 3;
        ULONG Am: 2;
        ULONG Ignore2: 27;
        ULONG Ignore3: 32;
    } ;
    ULONGLONG all;
} ALT_MODE_21164, *PALT_MODE_21164;

//
// Define the CC_CTL.
//

typedef union _CC_CTL_21164{
    struct {
        ULONG Count;
        ULONG CcEna: 1;
        ULONG Ignore: 31;
    } ;
    ULONGLONG all;
} CC_CTL_21164, *PCC_CTL_21164;

//
// Define Cbox Internal Processor Registers.
// (These IPRs are accessed via ld/st rather than mf/mt.)
//

//
// Define physical and superpage addresses for the CBOX registers.
//

#define BASE_SUPERVA (ULONGLONG)(0xfffffc0000000000)

#define SC_CTL_PA      (ULONGLONG)(0xfffff000a8)
#define SC_STAT_PA     (ULONGLONG)(0xfffff000e8)
#define SC_ADDR_PA     (ULONGLONG)(0xfffff00188)
#define BC_CONTROL_PA  (ULONGLONG)(0xfffff00128)
#define BC_CONFIG_PA   (ULONGLONG)(0xfffff001c8)
#define BC_TAG_ADDR_PA (ULONGLONG)(0xfffff00108)
#define EI_STAT_PA     (ULONGLONG)(0xfffff00168)
#define EI_ADDR_PA     (ULONGLONG)(0xfffff00148)
#define FILL_SYN_PA    (ULONGLONG)(0xfffff00068)
#define LD_LOCK_PA     (ULONGLONG)(0xfffff001e8)

#define SC_CTL_SVA      (ULONGLONG)( BASE_SUPERVA | SC_CTL_PA )
#define SC_STAT_SVA     (ULONGLONG)( BASE_SUPERVA | SC_STAT_PA )
#define SC_ADDR_SVA     (ULONGLONG)( BASE_SUPERVA | SC_ADDR_PA )
#define BC_CONTROL_SVA  (ULONGLONG)( BASE_SUPERVA | BC_CONTROL_PA )
#define BC_CONFIG_SVA   (ULONGLONG)( BASE_SUPERVA | BC_CONFIG_PA )
#define BC_TAG_ADDR_SVA (ULONGLONG)( BASE_SUPERVA | BC_TAG_ADDR_PA )
#define EI_STAT_SVA     (ULONGLONG)( BASE_SUPERVA | EI_STAT_PA )
#define EI_ADDR_SVA     (ULONGLONG)( BASE_SUPERVA | EI_ADDR_PA )
#define FILL_SYN_SVA    (ULONGLONG)( BASE_SUPERVA | FILL_SYN_PA )
#define LD_LOCK_SVA     (ULONGLONG)( BASE_SUPERVA | LD_LOCK_PA )

//
// Define the offsets for the Cbox IPRs to be used with specialized
// read/write ipr routines for EV5.
//

typedef struct _CBOX_IPRS_21164{
    UCHAR FillSyn;
    UCHAR Unused1;
    UCHAR ScCtl;
    UCHAR Unused2;
    UCHAR ScStat;
    UCHAR BcTagAddr;
    UCHAR BcControl;
    UCHAR EiAddr;
    UCHAR EiStat;
    UCHAR ScAddr;
    UCHAR Unused3;
    UCHAR BcConfig;
    UCHAR LdLock;
} CBOX_IPRS_21164, *PCBOX_IPRS_21164;

//
// Define the SC_CTL.
//

typedef union _SC_CTL_21164{
    struct {
        ULONG ScFhit: 1;
        ULONG ScFlush: 1;
        ULONG ScTagStat: 6;
        ULONG ScFbDp: 4;
        ULONG ScBlkSize: 1;
        ULONG ScSetEn: 3;
        ULONG Raz1: 16;
        ULONG Raz2: 32;
    } ;
    ULONGLONG all;
} SC_CTL_21164, *PSC_CTL_21164;

//
// Define the SC_ADDR.
//

typedef union _SC_ADDR_21164{
    struct {
        ULONGLONG Rao1: 4;
        ULONGLONG ScAddr: 35;
        ULONGLONG Raz1: 1;
        ULONGLONG Rao2: 24;
    };
    ULONGLONG all;
} SC_ADDR_21164, *PSC_ADDR_21164;

//
// Define the SC_STAT.
//

typedef union _SC_STAT_21164{
    struct {
        ULONG ScTperr: 3;
        ULONG ScDperr: 8;
        ULONG CboxCmd: 5;
        ULONG ScScndErr: 1;
        ULONG Raz1: 15;
        ULONG Raz2: 32;
    } ;
    ULONGLONG all;
} SC_STAT_21164, *PSC_STAT_21164;

//
// Define the BC_CONTROL.
//

typedef union _BC_CONTROL_21164{
    struct {
        ULONG BcEnabled: 1;
        ULONG AllocCyc: 1;
        ULONG EiCmdGrp1: 1;
        ULONG EiCmdGrp2: 1;
        ULONG CorrFillDat: 1;
        ULONG VtmFirst: 1;
        ULONG EiEccOrParity: 1;
        ULONG BcFhit: 1;
        ULONG BcTagStat: 5;
        ULONG BcBadDat: 2;
        ULONG EiDisErr: 1;
        ULONG TlPipeLatch: 1;
        ULONG BcWave: 2;
        ULONG PmMuxSel1: 3;
        ULONG PmMuxSel2: 3;
        ULONG Mbz1: 1;
        ULONG FlushScVtm: 1;
        ULONG Mbz2: 1;
        ULONG DisSysPar: 1;
        ULONG Mbz3: 3;
        ULONG Raz1: 1;
        ULONG NoByteIo: 1;
        ULONG Raz2: 30;
    } ;
    ULONGLONG all;
} BC_CONTROL_21164, *PBC_CONTROL_21164;

//
// Define the BC_CONFIG.
//

typedef union _BC_CONFIG_21164{
    struct {
        ULONG BcSize: 3;
        ULONG Reserved1: 1;
        ULONG BcRdSpd: 4;
        ULONG BcWrSpd: 4;
        ULONG BcRdWrSpc: 3;
        ULONG Reserved2: 1;
        ULONG FillWeOffset: 3;
        ULONG Reserved3: 1;
        ULONG BcWeCtl: 9;
        ULONG Reserved4: 3;
        ULONG Reserved5: 32;
    } ;
    ULONGLONG all;
} BC_CONFIG_21164, *PBC_CONFIG_21164;

//
// Define the EI_STAT.
//

typedef union _EI_STAT_21164{
    struct {
        ULONG Ra01: 24;
        ULONG ChipId: 4;
        ULONG BcTperr: 1;
        ULONG BcTcperr: 1;
        ULONG EiEs: 1;
        ULONG CorEccErr: 1;
        ULONG UncEccErr: 1;
        ULONG EiParErr: 1;
        ULONG FilIrd: 1;
        ULONG SeoHrdErr: 1;
        ULONG Ra02: 28;
    } ;
    ULONGLONG all;
} EI_STAT_21164, *PEI_STAT_21164;

//
// Define the EI_ADDR.
//

typedef union _EI_ADDR_21164{
    struct {
        ULONGLONG Rao1: 4;
        ULONGLONG EiAddr: 36;
        ULONGLONG Rao2: 24;
    };
    ULONGLONG all;
} EI_ADDR_21164, *PEI_ADDR_21164;

//
// Define the BC_TAG_ADDR.
//

typedef union _BC_TAG_ADDR_21164{
    struct {
        ULONG Ra01: 12;
        ULONG Hit: 1;
        ULONG TagCtlP: 1;
        ULONG TagCtlD: 1;
        ULONG TagCtlS: 1;
        ULONG TagCtlV: 1;
        ULONG TagP: 1;
        ULONG Ra02: 2;
        ULONG Tag0: 12;
        ULONG Tag1: 7;
        ULONG Ra03: 25;
    } ;
    ULONGLONG all;
} BC_TAG_ADDR_21164, *PBC_TAG_ADDR_21164;

//
// Define the FILL_SYN.
//

typedef union _FILL_SYN_21164{
    struct {
        ULONG Lo: 8;
        ULONG Hi: 8;
        ULONG Raz1: 16;
        ULONG Raz2: 32;
    } ;
    ULONGLONG all;
} FILL_SYN_21164, *PFILL_SYN_21164;

//++
// 21164PC Definitions
//--

//
// CBOX Register addresses
//

#define CBOX_CONFIG_PA          (ULONGLONG)(0xfffff00008)
#define CBOX_ADDRESS_PA         (ULONGLONG)(0xfffff00088)
#define CBOX_STATUS_PA          (ULONGLONG)(0xfffff00108)
#define CBOX_CONFIG2_PA         (ULONGLONG)(0xfffff00188)

#define CBOX_CONFIG_SVA         (ULONGLONG)( BASE_SUPERVA | CBOX_CONFIG_PA )
#define CBOX_ADDRESS_SVA        (ULONGLONG)( BASE_SUPERVA | CBOX_ADDRESS_PA )
#define CBOX_STATUS_SVA         (ULONGLONG)( BASE_SUPERVA | CBOX_STATUS_PA )
#define CBOX_CONFIG2_SVA        (ULONGLONG)( BASE_SUPERVA | CBOX_CONFIG2_PA )

//
// Define the offsets for the Cbox IPRs to be used with specialized
// read/write ipr routines for PCA56.
//

typedef struct _CBOX_IPRS_21164PC{
  UCHAR CboxConfig;
  UCHAR CboxAddress;
  UCHAR CboxStatus;
  UCHAR CboxConfig2;
} CBOX_IPRS_21164PC, *PCBOX_IPRS_21164PC;

//
// Define CBOX_CONFIG
//

typedef union _CBOX_CONFIG_21164PC{
  struct {
    ULONG Mbz1: 4;
    ULONG BcClkRatio: 4;
    ULONG BcLatencyOff: 4;
    ULONG BcSize: 2;
    ULONG BcClkDelay: 2;
    ULONG BcRwOff: 3;
    ULONG BcProbeDuringFill: 1;
    ULONG BcFillDelay: 3;
    ULONG IoParityEnable: 1;
    ULONG MemParityEnable: 1;
    ULONG BcForceHit: 1;
    ULONG BcForceErr: 1;
    ULONG BcBigDrv: 1;
    ULONG BcTagData: 3;
    ULONG BcEnable: 1;
    ULONG Mbz2: 32;
  };
  ULONGLONG all;
} CBOX_CONFIG_21164PC, *PCBOX_CONFIG_21164PC;

//
// Define CBOX_ADDRESS
//

typedef union _CBOX_ADDRESS_21164PC{
  struct {
    ULONGLONG Mbz1: 4;
    ULONGLONG Address36_4: 33;
    ULONGLONG Mbz2: 2;
    ULONGLONG Address39: 1;
    ULONGLONG Mbz3: 24;
  };
  ULONGLONG all;
} CBOX_ADDRESS_21164PC, *PCBOX_ADDRESS_21164PC;

//
// Define CBOX_STATUS
//

typedef union _CBOX_STATUS_21164PC{
  struct {
    ULONGLONG Mbz1: 4;
    ULONGLONG SysClkRatio: 4;
    ULONGLONG ChipRev: 4;
    ULONGLONG DataParErr: 4;
    ULONGLONG TagParErr: 1;
    ULONGLONG TagDirty: 1;
    ULONGLONG Memory: 1;
    ULONGLONG MultiErr: 1;
    ULONGLONG Mbz2: 44;
  };
  ULONGLONG all;
} CBOX_STATUS_21164PC, *PCBOX_STATUS_21164PC;

//
// Define CBOX_CONFIG2
//

typedef union _CBOX_CONFIG2_21164PC{
  struct {
    ULONGLONG Mbz1: 4;
    ULONGLONG BcRegReg: 1;
    ULONGLONG DbgSel: 1;
    ULONGLONG BcThreeMiss: 1;
    ULONGLONG Mbz2: 1;
    ULONGLONG Pm0Mux: 3;
    ULONGLONG Pm1Mux: 3;
    ULONGLONG Mbz3: 50;
  };
  ULONGLONG all;
} CBOX_CONFIG2_21164PC, *PCBOX_CONFIG2_21164PC;

//++
// End of 21164PC definitions
//--


//
// Define EV5 IPLs (interrupt priority levels.
//

#define EV5_IPL0   (0)
#define EV5_IPL1   (1)
#define EV5_IPL2   (2)
#define EV5_IPL3   (3)
#define EV5_IPL4   (4)
#define EV5_IPL5   (5)
#define EV5_IPL6   (6)
#define EV5_IPL7   (7)
#define EV5_IPL8   (8)
#define EV5_IPL9   (9)
#define EV5_IPL10 (10)
#define EV5_IPL11 (11)
#define EV5_IPL12 (12)
#define EV5_IPL13 (13)
#define EV5_IPL14 (14)
#define EV5_IPL15 (15)
#define EV5_IPL16 (16)
#define EV5_IPL17 (17)
#define EV5_IPL18 (18)
#define EV5_IPL19 (19)
#define EV5_IPL20 (20)
#define EV5_IPL21 (21)
#define EV5_IPL22 (22)
#define EV5_IPL23 (23)
#define EV5_IPL24 (24)
#define EV5_IPL25 (25)
#define EV5_IPL26 (26)
#define EV5_IPL27 (27)
#define EV5_IPL28 (28)
#define EV5_IPL29 (29)
#define EV5_IPL30 (30)
#define EV5_IPL31 (31)

//
// Define interrupt vector values for EV5.
//

#define EV5_IPL20_VECTOR (20)
#define EV5_IPL21_VECTOR (21)
#define EV5_IPL22_VECTOR (22)
#define EV5_IPL23_VECTOR (23)

#define EV5_IRQ0_VECTOR EV5_IPL20_VECTOR
#define EV5_IRQ1_VECTOR EV5_IPL21_VECTOR
#define EV5_IRQ2_VECTOR EV5_IPL22_VECTOR
#define EV5_IRQ3_VECTOR EV5_IPL23_VECTOR

#define EV5_HALT_VECTOR (14)
#define EV5_PFL_VECTOR (24)
#define EV5_MCHK_VECTOR (12)
#define EV5_CRD_VECTOR (25)
#define EV5_PC0_VECTOR (6)
#define EV5_PC1_VECTOR (8)
#define EV5_PC2_VECTOR (15)

//
// Define the Interrupt Mask structure communicated between the 
// HAL and PALcode.
//

typedef union _IMSK_21164{
    struct{
        ULONG Irq0Mask: 1;
        ULONG Irq1Mask: 1;
        ULONG Irq2Mask: 1;
        ULONG Irq3Mask: 1;
        ULONG Reserved: 28;
    };
    ULONG all;
} IMSK_21164, *PIMSK_21164;

//
// PALcode Event Counters for the 21164
// This is the structure of the data returned by the rdcounters call pal.
//

typedef struct _COUNTERS_21164{
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
    ULONGLONG Misc18Count;
    ULONGLONG Misc19Count;
    ULONGLONG Misc20Count;
    ULONGLONG SleepCount;
    ULONGLONG EalnfixCount;
    ULONGLONG DalnfixCount;
} COUNTERS_21164, *PCOUNTERS_21164;

//
// Types of performance counters.
//

typedef enum _AXP21164_PCCOUNTER{
	Ev5PerformanceCounter0 = 0,
	Ev5PerformanceCounter1 = 1,
	Ev5PerformanceCounter2 = 2
} AXP21164_PCCOUNTER, *PAXP21164_PCCOUNTER;

//
// Mux control values
//

typedef enum _AXP21164_PCMUXCONTROL{
	Ev5Cycles = 0x0,
	Ev5Instructions = 0x1,
	Ev5NonIssue = 0x0,
	Ev5SplitIssue = 0x1,
	Ev5PipeDry = 0x2,
	Ev5ReplayTrap = 0x3,
	Ev5SingleIssue = 0x4,
	Ev5DualIssue = 0x5,
	Ev5TripleIssue = 0x6,
	Ev5QuadIssue = 0x7,
	Ev5FlowChangeInst = 0x8,
	Ev5IntOpsIssued = 0x9,
	Ev5FPOpsIssued = 0xa,
	Ev5LoadsIssued = 0xb,
	Ev5StoresIssued = 0xc,
	Ev5IcacheIssued = 0xd,
	Ev5DcacheAccesses = 0xe,
	Ev5CBOXInput1 = 0xf,
	Ev5LongStalls = 0x0,
	Ev5PCMispredicts = 0x2,
	Ev5BRMispredicts = 0x3,
	Ev5IcacheRFBMisses = 0x4,
	Ev5ITBMisses = 0x5,
	Ev5DcacheLDMisses = 0x6,
	Ev5DTBMisses = 0x7,
	Ev5LDMergedMAF = 0x8,
	Ev5LDUReplayTraps = 0x9,
	Ev5WBMAFReplayTraps = 0xa,
	Ev5ExternPerfmonhInput = 0xb,
	Ev5CPUCycles = 0xc,
	Ev5MBStallCycles = 0xd,
	Ev5LDxLInstIssued = 0xe,
	Ev5CBOXInput2 = 0xf,

	//
	// Special MUX controls
	//

    Ev5PcSpecial = 0x10,

	Ev5JsrRetIssued = 0x10,
	Ev5CondBrIssued = 0x11,
	Ev5AllFlowIssued = 0x12,

    Ev5ScMux1 = 0x20,
    Ev5ScAccesses = 0x20,
    Ev5ScReads = 0x21,
    Ev5ScWrites = 0x22,
    Ev5ScVictims = 0x23,
    Ev5ScUndefined = 0x24,
    Ev5ScBcacheAccesses = 0x25,
    Ev5ScBcacheVictims = 0x26,
    Ev5ScSystemCmdReq = 0x27,
    Ev5ScMux2 = 0x28,
    Ev5ScMisses = 0x28,
    Ev5ScReadMisses = 0x29,
    Ev5ScWriteMisses = 0x2a,
    Ev5ScSharedWrites = 0x2b,
    Ev5ScWrites2 = 0x2c,
    Ev5ScBcacheMisses = 0x2d,
    Ev5ScSysInvalidate = 0x2e,
    Ev5ScSysReadReq = 0x2f,

} AXP21164_PCMUXCONTROL, *PAXP21164_PCMUXCONTROL;

//
// Counter control values.
//

typedef enum _AXP21164_PCEVENTCOUNT{
	Ev5CountEvents2xx8 = 0x100,
	Ev5CountEvents2xx14 = 0x4000,
	Ev5CountEvents2xx16 = 0x10000
} AXP21164_PCEVENTCOUNT, *PAXP21164_PCEVENTCOUNT;

//
//  Event count selection values
//

typedef enum _COUNTER_CONTROL{
    Ev5CounterDisable = 0x0,
    Ev5InterruptDisable = 0x1,
    Ev5EventCountLow=0x2,
    Ev5EventCountHigh=0x3
} COUNTER_CONTROL, *PCOUNTER_CONTROL;

//
// Internal processor state record.
// This is the structure of the data returned by the rdstate call pal.
//

typedef struct _PROCESSOR_STATE_21164{
    ITB_PTE_TEMP_21164 ItbPte[ ITB_ENTRIES_21164 ];
    ITB_ASN_21164 ItbAsn;
    ULONGLONG Ivptbr;
    ICPERR_STAT_21164 IcPerrStat;
    EXC_SUM_21164 ExcSum;
    ULONGLONG ExcMask;
    ULONGLONG PalBase;
    PS_21164 Ps;
    ICSR_21164 Icsr;
    ULONGLONG Ipl;
    ULONGLONG IntId;
    ULONGLONG Astrr;
    ULONGLONG Aster;
    SIRR_21164 Sirr;
    ISR_21164 Isr;
    PMCTR_21164 Pmctr;
    ULONGLONG PalTemp[ PAL_TEMPS_21164 ];
    DTB_PTE_TEMP_21164 DtbPte[ DTB_ENTRIES_21164 ];
    MM_STAT_21164 MmStat;
    ULONGLONG Va;
    DC_PERR_STAT_21164 DcPerrStat;
    MCSR_21164 Mcsr;
    DC_MODE_21164 DcMode;
    MAF_MODE_21164 MafMode;
    union {
      struct {  // EV5
        SC_CTL_21164 ScCtl;
        SC_ADDR_21164 ScAddr;
        SC_STAT_21164 ScStat;
        BC_CONTROL_21164 BcControl;
        BC_CONFIG_21164 BcConfig;
        EI_STAT_21164 EiStat;
        EI_ADDR_21164 EiAddr;
        BC_TAG_ADDR_21164 BcTagAddr;
        FILL_SYN_21164 FillSyn;
      };
      struct { // PCA56
        CBOX_CONFIG_21164PC CboxConfig;
        CBOX_ADDRESS_21164PC CboxAddress;
        CBOX_STATUS_21164PC CboxStatus;
        CBOX_CONFIG2_21164PC CboxConfig2;
        ULONGLONG Reserved1;
        ULONGLONG Reserved2;
        ULONGLONG Reserved3;
        ULONGLONG Reserved4;
        ULONGLONG Reserved5;
      };
    };
} PROCESSOR_STATE_21164, *PPROCESSOR_STATE_21164;

//
// Machine-check logout frame.
//

typedef struct _LOGOUT_FRAME_21164{
    ULONGLONG           ExcAddr;
    ULONGLONG           PalBase;
    ULONGLONG           Ps;
    ULONGLONG           Va;
    ULONGLONG           VaForm;
    ICSR_21164          Icsr;
    ICPERR_STAT_21164   IcPerrStat;
    ISR_21164           Isr;
    ULONGLONG           Ipl;
    ULONGLONG           IntId;
    MM_STAT_21164       MmStat;
    MCSR_21164          Mcsr;
    DC_PERR_STAT_21164  DcPerrStat;
    union {
      struct {  // EV5
        SC_CTL_21164        ScCtl;
        SC_STAT_21164       ScStat;
        SC_ADDR_21164       ScAddr;
        BC_CONTROL_21164    BcControl;
        BC_CONFIG_21164     BcConfig;
        BC_TAG_ADDR_21164   BcTagAddr;
        EI_STAT_21164       EiStat;
        EI_ADDR_21164       EiAddr;
        FILL_SYN_21164      FillSyn;
      };
      struct { // PCA56
        CBOX_CONFIG_21164PC CboxConfig;
        CBOX_ADDRESS_21164PC CboxAddress;
        CBOX_STATUS_21164PC CboxStatus;
        CBOX_CONFIG2_21164PC CboxConfig2;
        ULONGLONG Reserved1;
        ULONGLONG Reserved2;
        ULONGLONG Reserved3;
        ULONGLONG Reserved4;
        ULONGLONG Reserved5;
      };
    };
    ULONGLONG PalTemp[ PAL_TEMPS_21164 ];
} LOGOUT_FRAME_21164, *PLOGOUT_FRAME_21164;

//
// Correctable Machine-check logout frame.
//

typedef struct _CORRECTABLE_FRAME_21164{
    union {
      struct {  // EV5
        EI_STAT_21164       EiStat;
        EI_ADDR_21164       EiAddr;
        FILL_SYN_21164      FillSyn;
      };
      struct { // PCA56
        CBOX_STATUS_21164PC CboxStatus;
        CBOX_ADDRESS_21164PC CboxAddress;
        ULONGLONG Reserved;
      };
    };
    ISR_21164           Isr;
} CORRECTABLE_FRAME_21164;

//
// Define the number of physical and virtual address bits
//

#define EV5_PHYSICAL_ADDRESS_BITS     40
#define EV5_VIRTUAL_ADDRESS_BITS      43

#endif //!_AXP21164_  
