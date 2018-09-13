;/*++
;
; Copyright (c) 1997  Microsoft Corporation
;
; Module Name:
;
;   ntacpi.h
;
; Abstract:
;
;
;   This module contains definitions specific to the HAL's
;   ACPI function.
;
; Author:
;
;   Jake Oshins (jakeo) Feb. 18, 1997
;
; Revision History:
;
;-

if 0        ; Begin C only code         */

#ifndef _ACPI_H_
#define _ACPI_H_


#define SLEEP_STATE_FLUSH_CACHE         0x1
#define SLEEP_STATE_FIRMWARE_RESTART    0x2
#define SLEEP_STATE_SAVE_MOTHERBOARD    0x4
#define SLEEP_STATE_OFF                 0x8
#define SLEEP_STATE_RESTART_OTHER_PROCESSORS    0x10

typedef struct {
    union {
        struct {
            ULONG       Pm1aVal:4;
            ULONG       Pm1bVal:4;
            ULONG       Flags:24;
        } bits;
        ULONG   AsULONG;
    };
} SLEEP_STATE_CONTEXT, *PSLEEP_STATE_CONTEXT;


//
// ACPI Register definitions
//

#define P_LVL2          4
#define PBLK_THT_EN                     0x10

//
// Register layout of PM1x_EVT register
// Note also defined in acpiregs.h
//

#define PM1_PWRBTN_STS_BIT      8
#define PM1_PWRBTN_STS          (1 << PM1_PWRBTN_STS_BIT)

//
// Register layout of PM1x_CTL
//

#define SCI_EN              1
#define BM_RLD              2
#define CTL_IGNORE          0x200
#define SLP_TYP_SHIFT       10
#define SLP_EN              0x2000

#define CTL_PRESERVE        (SCI_EN + BM_RLD + CTL_IGNORE)


//
// HAL's table
//

typedef enum {
    HalAcpiTimerInit,
    HalAcpiTimerInterrupt,
    HalAcpiMachineStateInit,
    HalAcpiQueryFlags,
    HalPicStateIntact,
    HalRestorePicState,
    HalPciInterfaceReadConfig,
    HalPciInterfaceWriteConfig,
    HalSetVectorState,
    HalGetIOApicVersion,
    HalSetMaxLegacyPciBusNumber,
    HalAcpiMaxFunction
} HAL_DISPATCH_FUNCTION;

typedef
VOID
(*pHalAcpiTimerInit)(
    IN ULONG    TimerPort,
    IN BOOLEAN  TimerValExt
    );

typedef
VOID
(*pHalAcpiTimerInterrupt)(
    VOID
    );

typedef struct {
    ULONG   Count;
    ULONG   Pblk[1];
} PROCESSOR_INIT, *PPROCESSOR_INIT;

#define HAL_C1_SUPPORTED 0x01
#define HAL_C2_SUPPORTED 0x02
#define HAL_C3_SUPPORTED 0x04
#define HAL_S1_SUPPORTED 0x08
#define HAL_S2_SUPPORTED 0x10
#define HAL_S3_SUPPORTED 0x20
#define HAL_S4_SUPPORTED 0x40
#define HAL_S5_SUPPORTED 0x80

typedef struct {
    BOOLEAN     Supported;
    UCHAR       Pm1aVal;
    UCHAR       Pm1bVal;
} HAL_SLEEP_VAL, *PHAL_SLEEP_VAL;

typedef
VOID
(*pHalAcpiMachineStateInit)(
    IN  PPROCESSOR_INIT ProcInit,
    IN  PHAL_SLEEP_VAL  SleepValues,
    OUT PULONG          PicVal
    );

typedef
ULONG
(*pHalAcpiQueryFlags)(
    VOID
    );

typedef
BOOLEAN
(*pHalPicStateIntact)(
    VOID
    );

typedef
VOID
(*pHalRestorePicState)(
    VOID
    );

typedef
ULONG
(*pHalInterfaceReadWriteConfig)(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

//
// Flags for interrupt vectors
//

#define VECTOR_MODE         1
#define VECTOR_LEVEL        1
#define VECTOR_EDGE         0
#define VECTOR_POLARITY     2
#define VECTOR_ACTIVE_LOW   2
#define VECTOR_ACTIVE_HIGH  0

//
// Vector Type:
//
// VECTOR_SIGNAL = standard edge-triggered or
//		   level-sensitive interrupt vector
//
// VECTOR_MESSAGE = an MSI (Message Signalled Interrupt) vector
//

#define VECTOR_TYPE         4
#define VECTOR_SIGNAL       0
#define VECTOR_MESSAGE      4

#define IS_LEVEL_TRIGGERED(vectorFlags) \
    (vectorFlags & VECTOR_LEVEL)

#define IS_EDGE_TRIGGERED(vectorFlags) \
    !(vectorFlags & VECTOR_LEVEL)

#define IS_ACTIVE_LOW(vectorFlags) \
    (vectorFlags & VECTOR_ACTIVE_LOW)

#define IS_ACTIVE_HIGH(vectorFlags) \
    !(vectorFlags & VECTOR_ACTIVE_LOW)

typedef
VOID
(*pHalSetVectorState)(
    IN ULONG Vector,
    IN ULONG Flags
    );

VOID
HaliSetVectorState(
    IN ULONG Vector,
    IN ULONG Flags
    );

#define HAL_ACPI_PCI_RESOURCES    0x01
#define HAL_ACPI_PRT_SUPPORT      0x02

typedef
ULONG
(*pHalGetIOApicVersion)(
    IN ULONG ApicNo
    );

typedef
VOID
(*pHalSetMaxLegacyPciBusNumber)(
    IN ULONG BusNumber
    );

//
//  typedef struct _PM_DISPATCH_TABLE {
//      ULONG   Signature;
//      ULONG   Version;
//      PVOID   Function[1];
//  } PM_DISPATCH_TABLE, *PPM_DISPATCH_TABLE;
//

typedef struct {
    ULONG   Signature;
    ULONG   Version;
    pHalAcpiTimerInit               HalpAcpiTimerInit;
    pHalAcpiTimerInterrupt          HalpAcpiTimerInterrupt;
    pHalAcpiMachineStateInit        HalpAcpiMachineStateInit;
    pHalAcpiQueryFlags              HalpAcpiQueryFlags;
    pHalPicStateIntact              HalxPicStateIntact;
    pHalRestorePicState             HalxRestorePicState;
    pHalInterfaceReadWriteConfig    HalpPciInterfaceReadConfig;
    pHalInterfaceReadWriteConfig    HalpPciInterfaceWriteConfig;
    pHalSetVectorState              HalpSetVectorState;
    pHalGetIOApicVersion            HalpGetIOApicVersion;
    pHalSetMaxLegacyPciBusNumber    HalpSetMaxLegacyPciBusNumber;
} HAL_ACPI_DISPATCH_TABLE, *PHAL_ACPI_DISPATCH_TABLE;

#define HAL_ACPI_DISPATCH_SIGNATURE   'HAL '
#define HAL_ACPI_DISPATCH_VERSION     1

#define HalAcpiTimerInit            ((pHalAcpiTimerInit)PmHalDispatchTable->Function[HalAcpiTimerInit])
#define HalAcpiTimerInterrupt       ((pHalAcpiTimerInterrupt)PmHalDispatchTable->Function[HalAcpiTimerInterrupt])
#define HalAcpiMachineStateInit     ((pHalAcpiMachineStateInit)PmHalDispatchTable->Function[HalAcpiMachineStateInit])
#define HalPicStateIntact           ((pHalPicStateIntact)PmHalDispatchTable->Function[HalPicStateIntact])
#define HalRestorePicState          ((pHalRestorePicState)PmHalDispatchTable->Function[HalRestorePicState])
#define HalPciInterfaceReadConfig   ((pHalInterfaceReadWriteConfig)PmHalDispatchTable->Function[HalPciInterfaceReadConfig])
#define HalPciInterfaceWriteConfig  ((pHalInterfaceReadWriteConfig)PmHalDispatchTable->Function[HalPciInterfaceWriteConfig])
#define HalSetVectorState           ((pHalSetVectorState)PmHalDispatchTable->Function[HalSetVectorState])
#define HalGetIOApicVersion         ((pHalGetIOApicVersion)PmHalDispatchTable->Function[HalGetIOApicVersion])
#define HalSetMaxLegacyPciBusNumber ((pHalSetMaxLegacyPciBusNumber)PmHalDispatchTable->Function[HalSetMaxLegacyPciBusNumber])

extern PPM_DISPATCH_TABLE PmAcpiDispatchTable;
extern PPM_DISPATCH_TABLE PmHalDispatchTable;

//
// ACPI driver's table
//
typedef enum {
    AcpiEnableDisableGPEvents,
    AcpiInitEnableAcpi,
    AcpiGpeEnableWakeEvents,
    AcpiMaxFunction
} ACPI_DISPATCH_FUNCTION;

typedef
VOID
(*pAcpiEnableDisableGPEvents) (
    IN BOOLEAN Enable
    );

typedef
VOID
(*pAcpiInitEnableAcpi) (
    IN BOOLEAN ReEnable
    );

typedef
VOID
(*pAcpiGpeEnableWakeEvents)(
    VOID
    );

typedef struct {
    ULONG   Signature;
    ULONG   Version;
    pAcpiEnableDisableGPEvents    AcpipEnableDisableGPEvents;
    pAcpiInitEnableAcpi           AcpipInitEnableAcpi;
    pAcpiGpeEnableWakeEvents      AcpipGpeEnableWakeEvents;
} ACPI_HAL_DISPATCH_TABLE, *PACPI_HAL_DISPATCH_TABLE;

#define ACPI_HAL_DISPATCH_SIGNATURE   'ACPI'
#define ACPI_HAL_DISPATCH_VERSION     1

#define AcpiEnableDisableGPEvents       (*(pAcpiEnableDisableGPEvents)PmAcpiDispatchTable->Function[AcpiEnableDisableGPEvents])
#define AcpiInitEnableAcpi              (*(pAcpiInitEnableAcpi)PmAcpiDispatchTable->Function[AcpiInitEnableAcpi])
#define AcpiGpeEnableWakeEvents         (*(pAcpiGpeEnableWakeEvents)PmAcpiDispatchTable->Function[AcpiGpeEnableWakeEvents])

// from detect\i386\acpibios.h
typedef struct {
    PHYSICAL_ADDRESS    Base;
    LARGE_INTEGER       Length;
    ULONGLONG           Type;
} ACPI_E820_ENTRY, *PACPI_E820_ENTRY;

typedef struct _ACPI_BIOS_MULTI_NODE {
    PHYSICAL_ADDRESS    RsdtAddress;    // 64-bit physical address of RSDT
    ULONGLONG           Count;
    ACPI_E820_ENTRY     E820Entry[1];
} ACPI_BIOS_MULTI_NODE, *PACPI_BIOS_MULTI_NODE;

typedef enum {
    AcpiAddressRangeMemory = 1,
    AcpiAddressRangeReserved,
    AcpiAddressRangeACPI,
    AcpiAddressRangeNVS,
    AcpiAddressRangeMaximum,
} ACPI_BIOS_E820_TYPE, *PACPI_BIOS_E820_TYPE;


NTSTATUS
HalpAcpiFindRsdt (
    OUT PACPI_BIOS_MULTI_NODE   *AcpiMulti
    );

#endif //_ACPI_H_

/*
endif
;
;  Begin assembly part of the definitions
;


;
; Register layout of ACPI processor register block
;

P_CNT                   equ     0
P_LVL2                  equ     4
P_LVL3                  equ     5


;
; Register layout of PM1x_EVT register
;

BM_STS              equ       10h
WAK_STS             equ     8000h

;
; Register layout of PM1x_Enable
;

TMR_EN              equ     0001h
GBL_EN              equ     0020h
PWRBTN_EN           equ     0100h
SLPBTN_EN           equ     0200h
RTC_EN              equ     0400h

;
; Register layout of PM1x_CTL
;

SCI_EN              equ     1
BM_RLD              equ     2
CTL_IGNORE          equ     200h
SLP_TYP_SHIFT       equ     10
SLP_EN              equ     2000h

CTL_PRESERVE        equ     (SCI_EN + BM_RLD + CTL_IGNORE)

;
; Register layout of PM2_CNT
;

ARB_DIS             equ     1

;
; ACPI registers, as laid out in HalpFixedAcpiDescTable
;

PM1a_EVT        EQU _HalpFixedAcpiDescTable + 56
PM1b_EVT        EQU _HalpFixedAcpiDescTable + 60
PM1_EVT_LEN     EQU _HalpFixedAcpiDescTable + 88

PM1a_CNT        EQU _HalpFixedAcpiDescTable + 64
PM1b_CNT        EQU _HalpFixedAcpiDescTable + 68

PM2_CNT_BLK     EQU _HalpFixedAcpiDescTable + 72

PM_TMR_BLK      EQU _HalpFixedAcpiDescTable + 76
PM_TMR_FREQ     EQU 3579545

GPE0_BLK        EQU _HalpFixedAcpiDescTable + 80
GPE1_BLK        EQU _HalpFixedAcpiDescTable + 84

GPE0_BLK_LEN    EQU _HalpFixedAcpiDescTable + 92
GPE1_BLK_LEN    EQU _HalpFixedAcpiDescTable + 93

FLUSH_SIZE      EQU _HalpFixedAcpiDescTable + 100
FLUSH_STRIDE    EQU _HalpFixedAcpiDescTable + 102

DUTY_OFFSET     EQU _HalpFixedAcpiDescTable + 104

RTC_DAY_ALRM    EQU _HalpFixedAcpiDescTable + 106
RTC_MON_ALRM    EQU _HalpFixedAcpiDescTable + 107
RTC_CENTURY     EQU _HalpFixedAcpiDescTable + 108
FADT_FLAGS      EQU _HalpFixedAcpiDescTable + 112

;
; FADT flag values
;
WBINVD_SUPPORTED    EQU 1
WBINVD_FLUSH        EQU 2

;
GeneralWakeupEnable EQU 0
RtcWakeupEnable     EQU 1

;
; Constants used in the Context parameter to HaliAcpiSleep
;  (must match C code above)
;
SLEEP_STATE_FLUSH_CACHE         EQU 1
SLEEP_STATE_FIRMWARE_RESTART    EQU 2
CONTEXT_FLAG_SHIFT              EQU 8

;*/
