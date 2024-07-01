/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AHCI header file
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#define PORT_STATE_RECOVERING         0
#define PORT_STATE_HARDWARE_RESET     1

#define PORT_ACTION_NONE     0

/* 500 ms */
#define AHCI_CR_STOP_DELAY      500
#define AHCI_CR_START_DELAY     500

#define AHCI_MAX_PORTS       32
#define AHCI_MAX_PM_DEVICES  15

#define AHCI_MAX_COMMAND_SLOTS    32

/* 65535 */
#define AHCI_MAX_PRDT_ENTRIES    0x000FFFFF

/* 4MB */
#define AHCI_MAX_PRD_LENGTH      0x003FFFFF

#define AHCI_COMMAND_TABLE_ALIGNMENT       128
#define AHCI_RECEIVED_FIS_ALIGNMENT        256
#define AHCI_COMMAND_LIST_ALIGNMENT        1024
#define AHCI_RECEIVED_FIS_FBS_ALIGNMENT    4096

#define AHCI_FBS_RECEIVE_AREA_SIZE         4096

#define AHCI_FIS_REGISTER_HOST_TO_DEVICE           0x27
#define AHCI_FIS_REGISTER_DEVICE_TO_HOST           0x34
#define AHCI_FIS_DMA_ACTIVATE_DEVICE_TO_HOST       0x39
#define AHCI_FIS_DMA_SETUP                         0x41
#define AHCI_FIS_DATA                              0x46
#define AHCI_FIS_BIST_ACTIVATE                     0x58
#define AHCI_FIS_PIO_SETUP_DEVICE_TO_HOST          0x5F
#define AHCI_FIS_SET_DEVICE_BITS_DEVICE_TO_HOST    0xA1

typedef enum _AHCI_HOST_BUS_ADAPTER_REGISTER
{
    HbaCapabilities = 0x00,
    HbaGlobalControl = 0x04,
    HbaInterruptStatus = 0x08,
    HbaPortBitmap = 0x0C,
    HbaAhciVersion = 0x10,
    HbaCoalescingControl = 0x14,
    HbaCoalescingPorts = 0x18,
    HbaEnclosureManagementLocation = 0x1C,
    HbaEnclosureManagementControl = 0x20,
    HbaCapabilitiesEx = 0x24,
    HbaBiosHandoffControl = 0x28,
} AHCI_HOST_BUS_ADAPTER_REGISTER;

typedef enum _AHCI_PORT_REGISTER
{
    PxCommandListBaseLow = 0x00,
    PxCommandListBaseHigh = 0x04,
    PxFisBaseLow = 0x08,
    PxFisBaseHigh = 0x0C,
    PxInterruptStatus = 0x10,
    PxInterruptEnable = 0x14,
    PxCmdStatus = 0x18,
    PxTaskFileData = 0x20,
    PxSignature = 0x24,
    PxSataStatus = 0x28,
    PxSataControl = 0x2C,
    PxSataError = 0x30,
    PxSataActive = 0x34,
    PxCommandIssue = 0x38,
    PxSataNotification = 0x3C,
    PxFisSwitchingControl = 0x40,
    PxDeviceSleep = 0x44,
} AHCI_PORT_REGISTER;

/*
 * Interrupt Status/Enable Register
 */
#define AHCI_PXIRQ_DHRS        0x00000001
#define AHCI_PXIRQ_PSS         0x00000002
#define AHCI_PXIRQ_DSS         0x00000004
#define AHCI_PXIRQ_SDBS        0x00000008
#define AHCI_PXIRQ_UFS         0x00000010
#define AHCI_PXIRQ_DPS         0x00000020
#define AHCI_PXIRQ_PCS         0x00000040
#define AHCI_PXIRQ_DMPS        0x00000080
#define AHCI_PXIRQ_RSV1        0x003FFF00
#define AHCI_PXIRQ_PRCS        0x00400000
#define AHCI_PXIRQ_IPMS        0x00800000
#define AHCI_PXIRQ_OFS         0x01000000
#define AHCI_PXIRQ_RSV2        0x02000000
#define AHCI_PXIRQ_INFS        0x04000000
#define AHCI_PXIRQ_IFS         0x08000000
#define AHCI_PXIRQ_HBDS        0x10000000
#define AHCI_PXIRQ_HBFS        0x20000000
#define AHCI_PXIRQ_TFES        0x40000000
#define AHCI_PXIRQ_CPDS        0x80000000

#define AHCI_PXIRQ_FATAL_ERROR \
    (AHCI_PXIRQ_TFES | AHCI_PXIRQ_IFS | AHCI_PXIRQ_HBDS | AHCI_PXIRQ_HBFS)

/*
 * Command and Status Register
 */
#define AHCI_PXCMD_ST       0x00000001
#define AHCI_PXCMD_SUD      0x00000002
#define AHCI_PXCMD_POD      0x00000004
#define AHCI_PXCMD_CLO      0x00000008
#define AHCI_PXCMD_FRE      0x00000010
#define AHCI_PXCMD_RSV      0x000000E0
#define AHCI_PXCMD_CCS_MASK 0x00001F00
#define AHCI_PXCMD_MPSS     0x00002000
#define AHCI_PXCMD_FR       0x00004000
#define AHCI_PXCMD_CR       0x00008000
#define AHCI_PXCMD_CPS      0x00010000
#define AHCI_PXCMD_PMA      0x00020000
#define AHCI_PXCMD_HPCP     0x00040000
#define AHCI_PXCMD_MPSP     0x00080000
#define AHCI_PXCMD_CPD      0x00100000
#define AHCI_PXCMD_ESP      0x00200000
#define AHCI_PXCMD_FBSCP    0x00400000
#define AHCI_PXCMD_APSTE    0x00800000
#define AHCI_PXCMD_ATAPI    0x01000000
#define AHCI_PXCMD_DLAE     0x02000000
#define AHCI_PXCMD_ALPE     0x04000000
#define AHCI_PXCMD_ASP      0x08000000
#define AHCI_PXCMD_ICC_MASK 0xF0000000

#define AHCI_PXCMD_ICC_IDLE        0x00000000
#define AHCI_PXCMD_ICC_ACTIVE      0x10000000
#define AHCI_PXCMD_ICC_PARTIAL     0x20000000
#define AHCI_PXCMD_ICC_SLUMBER     0x60000000
#define AHCI_PXCMD_ICC_DEVSLEEP    0x80000000

#define AHCI_PXCMD_CCS_SHIFT      8

/*
 * Task File Data Register
 */
#define AHCI_PXTFD_STATUS_MASK  0x000000FF
#define AHCI_PXTFD_ERROR_MASK   0x0000FF00

#define AHCI_PXTFD_ERROR_SHIFT  8

#define AHCI_PXTFD_STATUS_ERR  0x01
#define AHCI_PXTFD_STATUS_DRQ  0x08
#define AHCI_PXTFD_STATUS_BUSY 0x80

/*
 * Signature Register
 */
#define AHCI_PXSIG_INVALID     0xFFFFFFFF
#define AHCI_PXSIG_ATAPI       0xEB140101

/*
 * Serial ATA Status Register
 */
#define AHCI_PXSSTS_DET_MASK               0x0000000F
#define AHCI_PXSSTS_SPD_MASK               0x000000F0
#define AHCI_PXSSTS_IPM_MASK               0x00000F00

#define AHCI_PXSSTS_DET_IDLE               0x00000000
#define AHCI_PXSSTS_DET_DEVICE_NO_PHY      0x00000001
#define AHCI_PXSSTS_DET_DEVICE_PHY         0x00000003
#define AHCI_PXSSTS_DET_PHY_OFFLINE        0x00000004

#define AHCI_PXSSTS_IPM_NO_DEVICE          0x00000000
#define AHCI_PXSSTS_IPM_ACTIVE             0x00000100
#define AHCI_PXSSTS_IPM_PARTIAL            0x00000200
#define AHCI_PXSSTS_IPM_SLUMBER            0x00000600
#define AHCI_PXSSTS_IPM_DEVSLEEP           0x00000800

/*
 * Serial ATA Control Register
 */
#define AHCI_PXCTL_DET_MASK                0x0000000F
#define AHCI_PXCTL_SPD_MASK                0x000000F0
#define AHCI_PXCTL_IPM_MASK                0x00000F00

#define AHCI_PXCTL_DET_IDLE                0x00000000
#define AHCI_PXCTL_DET_RESET               0x00000001
#define AHCI_PXCTL_DET_DISABLE_SATA        0x00000004

#define AHCI_PXCTL_IPM_DISABLE_NONE        0x00000000
#define AHCI_PXCTL_IPM_DISABLE_PARTIAL     0x00000100
#define AHCI_PXCTL_IPM_DISABLE_SLUMBER     0x00000200
#define AHCI_PXCTL_IPM_DISABLE_DEVSLEEP    0x00000400

#define AHCI_PXCTL_IPM_DISABLE_ALL         0x00000700

/*
 * Device Sleep Register
 */
#define AHCI_PXDEVSLP_ADSE                 0x00000001
#define AHCI_PXDEVSLP_DSP                  0x00000002
#define AHCI_PXDEVSLP_DETO_MASK            0x000003FC
#define AHCI_PXDEVSLP_MDAT_MASK            0x00007C00
#define AHCI_PXDEVSLP_DITO_MASK            0x01FF8000
#define AHCI_PXDEVSLP_DM_MASK              0x1E000000

#define AHCI_PORT_INTERRUPT_MASK \
  (AHCI_PXIRQ_DHRS | \
  AHCI_PXIRQ_PSS  | \
  AHCI_PXIRQ_DSS  | \
  AHCI_PXIRQ_SDBS | \
  AHCI_PXIRQ_UFS  | \
  AHCI_PXIRQ_DPS  | \
  AHCI_PXIRQ_PCS  | \
  AHCI_PXIRQ_DMPS | \
  AHCI_PXIRQ_PRCS | \
  AHCI_PXIRQ_IPMS | \
  AHCI_PXIRQ_OFS  | \
  AHCI_PXIRQ_INFS | \
  AHCI_PXIRQ_IFS  | \
  AHCI_PXIRQ_HBDS | \
  AHCI_PXIRQ_HBFS | \
  AHCI_PXIRQ_TFES | \
  AHCI_PXIRQ_CPDS)

/*
 * AHCI Version Register
 */
#define AHCI_VERSION_0_95       0x00000905
#define AHCI_VERSION_1_0        0x00010000
#define AHCI_VERSION_1_2        0x00010200
#define AHCI_VERSION_1_3_0      0x00010300
#define AHCI_VERSION_1_3_1      0x00010301

/*
 * Global HBA Control Register
 */
#define AHCI_GHC_HR             0x00000001
#define AHCI_GHC_IE             0x00000002
#define AHCI_GHC_MRSM           0x00000004
#define AHCI_GHC_AE             0x80000000

/*
 * HBA Capabilities Register
 */
#define AHCI_CAP_NP    0x0000001F
#define AHCI_CAP_SXS   0x00000020
#define AHCI_CAP_EMS   0x00000040
#define AHCI_CAP_CCCS  0x00000080
#define AHCI_CAP_NCS   0x00001F00
#define AHCI_CAP_PSC   0x00002000
#define AHCI_CAP_SSC   0x00004000
#define AHCI_CAP_PMD   0x00008000
#define AHCI_CAP_FBSS  0x00010000
#define AHCI_CAP_SPM   0x00020000
#define AHCI_CAP_SAM   0x00040000
#define AHCI_CAP_RSV   0x00080000
#define AHCI_CAP_ISS   0x00F00000
#define AHCI_CAP_SCLO  0x01000000
#define AHCI_CAP_SAL   0x02000000
#define AHCI_CAP_SALP  0x04000000
#define AHCI_CAP_SSS   0x08000000
#define AHCI_CAP_SMPS  0x10000000
#define AHCI_CAP_SSNTF 0x20000000
#define AHCI_CAP_SNCQ  0x40000000
#define AHCI_CAP_S64A  0x80000000

/*
 * HBA Capabilities Extended Register
 */
#define AHCI_CAP2_BOH           0x00000001
#define AHCI_CAP2_NVMP          0x00000002
#define AHCI_CAP2_APST          0x00000004
#define AHCI_CAP2_SDS           0x00000008
#define AHCI_CAP2_SADM          0x00000010
#define AHCI_CAP2_DESO          0x00000020

#define AHCI_BOHC_BIOS_SEMAPHORE             0x00000001
#define AHCI_BOHC_OS_SEMAPHORE               0x00000002
#define AHCI_BOHC_SMI_ON_OS_OWNERSHIP_CHANGE 0x00000004
#define AHCI_BOHC_OS_OWNERSHIP_CHANGE        0x00000008
#define AHCI_BOHC_BIOS_BUSY                  0x00000010

#include <pshpack1.h>

typedef struct _AHCI_FIS_HOST_TO_DEVICE
{
    UCHAR Type;

    UCHAR Flags;
#define UPDATE_COMMAND 0x80
#define PMP_NUMBER     0x0F

    UCHAR Command;
    UCHAR Features;
    UCHAR LbaLow;
    UCHAR LbaMid;
    UCHAR LbaHigh;
    UCHAR Device;
    UCHAR LbaLowEx;
    UCHAR LbaMidEx;
    UCHAR LbaHighEx;
    UCHAR FeaturesEx;
    UCHAR SectorCount;
    UCHAR SectorCountEx;
    UCHAR Icc;
    UCHAR Control;
    ULONG Auxiliary;
} AHCI_FIS_HOST_TO_DEVICE, *PAHCI_FIS_HOST_TO_DEVICE;

C_ASSERT(sizeof(AHCI_FIS_HOST_TO_DEVICE) == 20);

typedef struct _AHCI_FIS_DEVICE_TO_HOST
{
    UCHAR Type;
    UCHAR Flags;
    UCHAR Status;
    UCHAR Error;
    UCHAR LbaLow;
    UCHAR LbaMid;
    UCHAR LbaHigh;
    UCHAR Device;
    UCHAR LbaLowEx;
    UCHAR LbaMidEx;
    UCHAR LbaHighEx;
    UCHAR Reserved;
    UCHAR SectorCount;
    UCHAR SectorCountEx;
    UCHAR Reserved1;
    UCHAR Reserved2;
    ULONG Reserved3;
} AHCI_FIS_DEVICE_TO_HOST, *PAHCI_FIS_DEVICE_TO_HOST;

C_ASSERT(sizeof(AHCI_FIS_DEVICE_TO_HOST) == 20);

typedef struct _AHCI_RECEIVED_FIS
{
    UCHAR DmaSetup[0x1C];
    ULONG Reserved;

    UCHAR PioSetup[0x14];
    ULONG Reserved2[3];

    AHCI_FIS_DEVICE_TO_HOST DeviceToHost;
    ULONG Reserved3;

    UCHAR DeviceBits[0x08];

    UCHAR UnknownFis[0x40];

    UCHAR Reserved4[0x60];
} AHCI_RECEIVED_FIS, *PAHCI_RECEIVED_FIS;

C_ASSERT(sizeof(AHCI_RECEIVED_FIS) == 256);

typedef struct _AHCI_COMMAND_HEADER
{
    ULONG Control;
#define AHCI_COMMAND_HEADER_COMMAND_FIS_LENGTH       0x0000001F
#define AHCI_COMMAND_HEADER_ATAPI                    0x00000020
#define AHCI_COMMAND_HEADER_WRITE                    0x00000040
#define AHCI_COMMAND_HEADER_PREFETCHABLE             0x00000080
#define AHCI_COMMAND_HEADER_RESET                    0x00000100
#define AHCI_COMMAND_HEADER_BIST                     0x00000200
#define AHCI_COMMAND_HEADER_CLEAR_BUSY_UPON_OK       0x00000400
#define AHCI_COMMAND_HEADER_PMP                      0x0000F000
#define AHCI_COMMAND_HEADER_PRDT_LENGTH              0xFFFF0000

#define AHCI_COMMAND_HEADER_PRDT_LENGTH_SHIFT        16

    ULONG PrdByteCount;
    ULONG CommandTableBaseLow;
    ULONG CommandTableBaseHigh;
    ULONG Reserved[4];
} AHCI_COMMAND_HEADER, *PAHCI_COMMAND_HEADER;

C_ASSERT(sizeof(AHCI_COMMAND_HEADER) == 32);

typedef struct _AHCI_COMMAND_LIST
{
    AHCI_COMMAND_HEADER CommandHeader[ANYSIZE_ARRAY];
} AHCI_COMMAND_LIST, *PAHCI_COMMAND_LIST;

typedef struct _AHCI_PRD_TABLE_ENTRY
{
    ULONG DataBaseLow;
    ULONG DataBaseHigh;
    ULONG Reserved;

    ULONG ByteCount;
#define AHCI_PRD_INTERRUPT_ON_COMPLETION 0x80000000

} AHCI_PRD_TABLE_ENTRY, *PAHCI_PRD_TABLE_ENTRY;

C_ASSERT(sizeof(AHCI_PRD_TABLE_ENTRY) == 16);

typedef struct _AHCI_COMMAND_TABLE
{
    union
    {
        AHCI_FIS_HOST_TO_DEVICE HostToDeviceFis;
        UCHAR CommandFis[64];
    };
    UCHAR AtapiCommand[16];
    UCHAR Reserved[48];
    AHCI_PRD_TABLE_ENTRY PrdTable[ANYSIZE_ARRAY];
} AHCI_COMMAND_TABLE, *PAHCI_COMMAND_TABLE;

C_ASSERT(FIELD_OFFSET(AHCI_COMMAND_TABLE, PrdTable) == 128);

#include <poppack.h>

#define AHCI_PORT_BASE(HbaIoBase, PortNumber) \
    (PULONG)((ULONG_PTR)(HbaIoBase) + (PortNumber) * 0x80 + 0x100)

FORCEINLINE
ULONG
AHCI_HBA_READ(
    _In_ PULONG HbaIoBase,
    _In_ AHCI_HOST_BUS_ADAPTER_REGISTER Register)
{
    return READ_REGISTER_ULONG((PULONG)((ULONG_PTR)HbaIoBase + Register));
}

FORCEINLINE
VOID
AHCI_HBA_WRITE(
    _In_ PULONG HbaIoBase,
    _In_ AHCI_HOST_BUS_ADAPTER_REGISTER Register,
    _In_ ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)((ULONG_PTR)HbaIoBase + Register), Value);
}

FORCEINLINE
ULONG
AHCI_PORT_READ(
    _In_ PULONG PortIoBase,
    _In_ AHCI_PORT_REGISTER Register)
{
    return READ_REGISTER_ULONG((PULONG)((ULONG_PTR)PortIoBase + Register));
}

FORCEINLINE
VOID
AHCI_PORT_WRITE(
    _In_ PULONG PortIoBase,
    _In_ AHCI_PORT_REGISTER Register,
    _In_ ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)((ULONG_PTR)PortIoBase + Register), Value);
}
