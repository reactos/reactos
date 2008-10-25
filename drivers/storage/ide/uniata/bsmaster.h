/*++

Copyright (c) 2002-2005 Alexandr A. Telyatnikov (Alter)

Module Name:
    bsmaster.h

Abstract:
    This file contains DMA/UltraDMA and IDE BusMastering related definitions,
    internal structures and useful macros

Author:
    Alexander A. Telyatnikov (Alter)

Environment:
    kernel mode only

Notes:

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Revision History:

    Code was created by
         Alter, Copyright (c) 2002-2007

    Some definitions were taken from FreeBSD 4.3-4.6 ATA driver by
         Søren Schmidt, Copyright (c) 1998,1999,2000,2001

--*/

#ifndef __IDE_BUSMASTER_H__
#define __IDE_BUSMASTER_H__

#include "config.h"

#include "tools.h"

//
//
//
#define         ATA_IDLE                0x0
#define         ATA_IMMEDIATE           0x1
#define         ATA_WAIT_INTR           0x2
#define         ATA_WAIT_READY          0x3
#define         ATA_ACTIVE              0x4
#define         ATA_ACTIVE_ATA          0x5
#define         ATA_ACTIVE_ATAPI        0x6
#define         ATA_REINITING           0x7
#define         ATA_WAIT_BASE_READY     0x8
#define         ATA_WAIT_IDLE           0x9


#include "bm_devs.h"

#include "uata_ctl.h"

#define MAX_RETRIES                     6
#define RETRY_UDMA2                     1
#define RETRY_WDMA                      2
#define RETRY_PIO                       3


#define	IO_WD1	        0x1F0		/* Primary Fixed Disk Controller */
#define	IO_WD2	        0x170		/* Secondary Fixed Disk Controller */
#define IP_PC98_BANK    0x432

#define PCI_ADDRESS_IOMASK              0xfffffff0

#define ATA_BM_OFFSET1			0x08
#define ATA_IOSIZE			0x08
#define ATA_ALTOFFSET			0x206	/* alternate registers offset */
#define ATA_PCCARD_ALTOFFSET		0x0e	/* do for PCCARD devices */
#define ATA_ALTIOSIZE			0x01	/* alternate registers size */
#define ATA_BMIOSIZE			0x20
#define ATA_PC98_BANKIOSIZE             0x01
#define ATA_MAX_LBA28                   DEF_U64(0x0fffffff)

#define ATA_DMA_ENTRIES			256     /* PAGESIZE/2/sizeof(BM_DMA_ENTRY)*/
#define ATA_DMA_EOT			0x80000000

#define DEV_BSIZE                       512

#define ATAPI_MAGIC_LSB			0x14
#define ATAPI_MAGIC_MSB			0xeb

#define AHCI_MAX_PORT                   32

typedef struct _BUSMASTER_CTX {
    PBUSMASTER_CONTROLLER_INFORMATION* BMListPtr;
    ULONG* BMListLen;
} BUSMASTER_CTX, *PBUSMASTER_CTX;

#define PCI_DEV_CLASS_STORAGE           0x01

#define PCI_DEV_SUBCLASS_IDE            0x01
#define PCI_DEV_SUBCLASS_RAID           0x04
#define PCI_DEV_SUBCLASS_ATA            0x05
#define PCI_DEV_SUBCLASS_SATA           0x06

/* structure for holding DMA address data */
typedef struct BM_DMA_ENTRY {
    ULONG base;
    ULONG count;
} BM_DMA_ENTRY, *PBM_DMA_ENTRY;

typedef struct _IDE_BUSMASTER_REGISTERS {
    UCHAR Command;
    UCHAR DeviceSpecific0;
    UCHAR Status;
    UCHAR DeviceSpecific1;
    ULONG PRD_Table;
} IDE_BUSMASTER_REGISTERS, *PIDE_BUSMASTER_REGISTERS;

#define BM_STATUS_ACTIVE                0x01
#define BM_STATUS_ERR                   0x02
#define BM_STATUS_INTR                  0x04
#define BM_STATUS_MASK                  0x07
#define BM_STATUS_DRIVE_0_DMA           0x20
#define BM_STATUS_DRIVE_1_DMA           0x40
#define BM_STATUS_SIMPLEX_ONLY          0x80

#define BM_COMMAND_START_STOP		0x01
/*#define BM_COMMAND_WRITE		0x08
#define BM_COMMAND_READ		        0x00*/
#define BM_COMMAND_WRITE		0x00
#define BM_COMMAND_READ		        0x08

#define BM_DS0_SII_DMA_ENABLE           (1 << 0)  /* DMA run switch */
#define BM_DS0_SII_IRQ	                (1 << 3)  /* ??? */
#define BM_DS0_SII_DMA_SATA_IRQ	        (1 << 4)  /* OR of all SATA IRQs */
#define BM_DS0_SII_DMA_ERROR	        (1 << 17) /* PCI bus error */
#define BM_DS0_SII_DMA_COMPLETE	        (1 << 18) /* cmd complete / IRQ pending */


#define IDX_BM_IO                       (IDX_IO2_o+IDX_IO2_o_SZ)
//#define IDX_BM_IO_SZ                    sizeof(IDE_BUSMASTER_REGISTERS)
#define IDX_BM_IO_SZ                    5

#define IDX_BM_Command                  (FIELD_OFFSET(IDE_BUSMASTER_REGISTERS, Command        )+IDX_BM_IO)
#define IDX_BM_DeviceSpecific0          (FIELD_OFFSET(IDE_BUSMASTER_REGISTERS, DeviceSpecific0)+IDX_BM_IO)
#define IDX_BM_Status                   (FIELD_OFFSET(IDE_BUSMASTER_REGISTERS, Status         )+IDX_BM_IO)
#define IDX_BM_DeviceSpecific1          (FIELD_OFFSET(IDE_BUSMASTER_REGISTERS, DeviceSpecific1)+IDX_BM_IO)
#define IDX_BM_PRD_Table                (FIELD_OFFSET(IDE_BUSMASTER_REGISTERS, PRD_Table      )+IDX_BM_IO)

typedef struct _IDE_AHCI_REGISTERS {
    // HBA Capabilities
    struct {
        ULONG NOP:5;   // number of ports
        ULONG Reserved5_7:1;
        ULONG NCS:5;   // number of command slots
        ULONG PSC:1;   // partial state capable
        ULONG SSC:1;   // slumber state capable
        ULONG PMD:1;   // PIO multiple DRQ block
        ULONG Reserved16:1;

        ULONG SPM:1;   // port multiplier
        ULONG SAM:1;   // AHCI mode only
        ULONG SNZO:1;  // non-zero DMA offset
        ULONG ISS:4;   // interface speed
        ULONG SCLO:1;  // command list override
        ULONG SAL:1;   // activity LED
        ULONG SALP:1;  // aggressive link power management
        ULONG SSS:1;   // staggered spin-up
        ULONG SIS:1;   // interlock switch
        ULONG Reserved29:1;
        ULONG SNCQ:1;  // native command queue
        ULONG S64A:1;  // 64bit addr
    } CAP;

#define AHCI_CAP_NOP_MASK    0x0000001f
#define AHCI_CAP_S64A        0x80000000

    // Global HBA Control
    struct {
        ULONG HR:1;    // HBA Reset
        ULONG IE:1;    // interrupt enable
        ULONG Reserved2_30:1;
        ULONG AE:1;    // AHCI enable
    } GHC;

#define AHCI_GHC_HR    0x00000001
#define AHCI_GHC_IE    0x00000002
#define AHCI_GHC_AE    0x80000000

    // Interrupt status (bit mask)
    ULONG IS;
    // Ports implemented (bit mask)
    ULONG PI;
    // AHCI Version
    ULONG VS;
    ULONG Reserved[3];

    UCHAR Reserved2[0x80];

    UCHAR VendorSpec[0x60];
} IDE_AHCI_REGISTERS, *PIDE_AHCI_REGISTERS;

#define IDX_AHCI_CAP                    (FIELD_OFFSET(IDE_AHCI_REGISTERS, CAP))
#define IDX_AHCI_GHC                    (FIELD_OFFSET(IDE_AHCI_REGISTERS, GHC))
#define IDX_AHCI_IS                     (FIELD_OFFSET(IDE_AHCI_REGISTERS, IS))
#define IDX_AHCI_VS                     (FIELD_OFFSET(IDE_AHCI_REGISTERS, VS))
#define IDX_AHCI_PI                     (FIELD_OFFSET(IDE_AHCI_REGISTERS, PI))


typedef union _SATA_SSTATUS_REG {

    struct {
        ULONG DET:4; // Device Detection

#define SStatus_DET_NoDev        0x00
#define SStatus_DET_Dev_NoPhy    0x01
#define SStatus_DET_Dev_Ok       0x03
#define SStatus_DET_Offline      0x04

        ULONG SPD:4; // Current Interface Speed

#define SStatus_SPD_NoDev        0x00
#define SStatus_SPD_Gen1         0x01
#define SStatus_SPD_Gen2         0x02

        ULONG IPM:4; // Interface Power Management

#define SStatus_IPM_NoDev        0x00
#define SStatus_IPM_Active       0x01
#define SStatus_IPM_Partial      0x02
#define SStatus_IPM_Slumber      0x06

        ULONG Reserved:20;
    };
    ULONG Reg;

} SATA_SSTATUS_REG, *PSATA_SSTATUS_REG;


typedef union _SATA_SCONTROL_REG {

    struct {
        ULONG DET:4; // Device Detection Init

#define SControl_DET_DoNothing   0x00
#define SControl_DET_Idle        0x00
#define SControl_DET_Init        0x01
#define SControl_DET_Disable     0x04

        ULONG SPD:4; // Speed Allowed

#define SControl_SPD_NoRestrict  0x00
#define SControl_SPD_LimGen1     0x01
#define SControl_SPD_LimGen2     0x02

        ULONG IPM:4; // Interface Power Management Transitions Allowed

#define SControl_IPM_NoRestrict  0x00
#define SControl_IPM_NoPartial   0x01
#define SControl_IPM_NoSlumber   0x02
#define SControl_IPM_NoPartialSlumber 0x03

        ULONG SPM:4; // Select Power Management, unused by AHCI
        ULONG PMP:4; // Port Multiplier Port, unused by AHCI
        ULONG Reserved:12;
    };
    ULONG Reg;

} SATA_SCONTROL_REG, *PSATA_SCONTROL_REG;


typedef union _SATA_SERROR_REG {

    struct {
        struct {
            UCHAR I:1; // Recovered Data Integrity Error
            UCHAR M:1; // Recovered Communications Error
            UCHAR Reserved_2_7:6;

            UCHAR T:1; // Transient Data Integrity Error
            UCHAR C:1; // Persistent Communication or Data Integrity Error
            UCHAR P:1; // Protocol Error
            UCHAR E:1; // Internal Error
            UCHAR Reserved_12_15:4;
        } ERR;

        struct {
            UCHAR N:1; // PhyRdy Change, PIS.PRCS
            UCHAR I:1; // Phy Internal Error
            UCHAR W:1; // Comm Wake
            UCHAR B:1; // 10B to 8B Decode Error
            UCHAR D:1; // Disparity Error, not used by AHCI
            UCHAR C:1; // CRC Error
            UCHAR H:1; // Handshake Error
            UCHAR S:1; // Link Sequence Error

            UCHAR T:1; // Transport state transition error
            UCHAR F:1; // Unknown FIS Type
            UCHAR X:1; // Exchanged
            UCHAR Reserved_27_31:5;
        } DIAG;
    };
    ULONG Reg;

} SATA_SERROR_REG, *PSATA_SERROR_REG;


typedef struct _IDE_SATA_REGISTERS {
    union {
        SATA_SSTATUS_REG   SStatus;
        ULONG              SStatus_Reg;
    };
    union {
        SATA_SERROR_REG    SError;
        ULONG              SError_Reg;
    };
    union {
        SATA_SCONTROL_REG  SControl;
        ULONG              SControl_Reg;
    };

    // SATA 1.2

    ULONG                  SActive;
    union {
        ULONG Reg;
        struct {
            USHORT PMN;     // PM Notify, bitmask
            USHORT Reserved;
        };
    } SNTF;
    ULONG                  SReserved[11];
} IDE_SATA_REGISTERS, *PIDE_SATA_REGISTERS;

#define IDX_SATA_IO                     (IDX_BM_IO+IDX_BM_IO_SZ)
//#define IDX_SATA_IO_SZ                  sizeof(IDE_SATA_REGISTERS)
#define IDX_SATA_IO_SZ                  5

#define IDX_SATA_SStatus                (0+IDX_SATA_IO)
#define IDX_SATA_SError                 (1+IDX_SATA_IO)
#define IDX_SATA_SControl               (2+IDX_SATA_IO)
#define IDX_SATA_SActive                (3+IDX_SATA_IO)
#define IDX_SATA_SNTF_PMN               (4+IDX_SATA_IO)

#define IDX_MAX_REG                     (IDX_SATA_IO+IDX_SATA_IO_SZ)

typedef union _AHCI_IS_REG {
    struct {
        ULONG DHRS:1;// Device to Host Register FIS Interrupt
        ULONG PSS:1; // PIO Setup FIS Interrupt
        ULONG DSS:1; // DMA Setup FIS Interrupt
        ULONG SDBS:1;// Set Device Bits Interrupt
        ULONG UFS:1; // Unknown FIS Interrupt
        ULONG DPS:1; // Descriptor Processed
        ULONG PCS:1; // Port Connect Change Status
        ULONG DMPS:1;// Device Mechanical Presence Status

        ULONG Reserved_8_21:14;
        ULONG PRCS:1;// PhyRdy Change Status
        ULONG IPMS:1;// Incorrect Port Multiplier Status

        ULONG OFS:1; // Overflow Status
        ULONG Reserved_25:1;
        ULONG INFS:1;// Interface Non-fatal Error Status
        ULONG IFS:1; // Interface Fatal Error Status
        ULONG HBDS:1;// Host Bus Data Error Status
        ULONG HBFS:1;// Host Bus Fatal Error Status
        ULONG TFES:1;// Task File Error Status
        ULONG CPDS:1;// Cold Port Detect Status
    };
    ULONG Reg;
} AHCI_IS_REG, *PAHCI_IS_REG;


typedef struct _IDE_AHCI_PORT_REGISTERS {
    union {
        struct {
            ULONG  CLB;   // command list base address
            ULONG  CLBU;  // command list base address (upper 32bits)
        };
        ULONGLONG CLB64;
    };

    union {
        struct {
            ULONG  FB;   // FIS base address
            ULONG  FBU;  // FIS base address (upper 32bits)
        };
        ULONGLONG FB64;
    };

    union {
        ULONG       IS_Reg;            // interrupt status
        AHCI_IS_REG IS;
    };

    union {
        ULONG Reg;            // interrupt enable
        struct {
            ULONG DHRE:1;// Device to Host Register FIS Interrupt Enable
            ULONG PSE:1; // PIO Setup FIS Interrupt Enable
            ULONG DSE:1; // DMA Setup FIS Interrupt Enable
            ULONG SDBE:1;// Set Device Bits FIS Interrupt Enable
            ULONG UFE:1; // Unknown FIS Interrupt Enable
            ULONG DPE:1; // Descriptor Processed Interrupt Enable
            ULONG PCE:1; // Port Change Interrupt Enable
            ULONG DPME:1;// Device Mechanical Presence Enable

            ULONG Reserved_8_21:14;
            ULONG PRCE:1;// PhyRdy Change Interrupt Enable
            ULONG IPME:1;// Incorrect Port Multiplier Enable
            ULONG OFE:1; // Overflow Enable
            ULONG Reserved_25:1;
            ULONG INFE:1;// Interface Non-fatal Error Enable
            ULONG IFE:1; // Interface Fatal Error Enable
            ULONG HBDE:1;// Host Bus Data Error Enable
            ULONG HBFE:1;// Host Bus Fatal Error Enable
            ULONG TFEE:1;// Task File Error Enable
            ULONG CPDE:1;// Cold Port Detect Enable
        };
    } IE;

    union {
        ULONG Reg;           // command register
        struct {

            ULONG ST:1;  // Start
            ULONG SUD:1; // Spin-Up Device
            ULONG POD:1; // Power On Device
            ULONG CLO:1; // Command List Override
            ULONG FRE:1; // FIS Receive Enable
            ULONG Reserved_5_7:3;

            ULONG CCS:5; // Current Command Slot
            ULONG MPSS:1;// Mechanical Presence Switch State
            ULONG FR:1;  // FIS Receive Running
            ULONG CR:1;  // Command List Running

            ULONG CPS:1; // Cold Presence State
            ULONG PMA:1; // Port Multiplier Attached
            ULONG HPCP:1;// Hot Plug Capable Port
            ULONG MPSP:1;// Mechanical Presence Switch Attached to Port
            ULONG CPD:1; // Cold Presence Detection
            ULONG ESP:1; // External SATA Port
            ULONG Reserved_22_23:2;

            ULONG ATAPI:1; // Device is ATAPI
            ULONG DLAE:1;// Drive LED on ATAPI Enable
            ULONG ALPE:1;// Aggressive Link Power Management Enable
            ULONG ASP:1; // Aggressive Slumber / Partial
            ULONG ICC:4; // Interface Communication Control

#define SATA_CMD_ICC_Idle    0x00
#define SATA_CMD_ICC_NoOp    0x00
#define SATA_CMD_ICC_Active  0x01
#define SATA_CMD_ICC_Partial 0x02
#define SATA_CMD_ICC_Slumber 0x06
        };
    } CMD;

    ULONG Reserved;

    union {
        ULONG Reg;           // Task File Data
        struct {
            struct {
                UCHAR ERR:1;
                UCHAR cs1:2;// command-specific
                UCHAR DRQ:1;
                UCHAR cs2:3;// command-specific
                UCHAR BSY:1;
            } STS;
            UCHAR ERR; // Contains the latest copy of the task file error register.
            UCHAR Reserved[2];
        };
    } TFD;

    union {
        ULONG Reg;           // signature
        struct {
            UCHAR SectorCount;
            UCHAR LbaLow;
            UCHAR LbaMid;
            UCHAR LbaHigh;
        };
    } SIG;
    union {
        ULONG             SStatus;      // SCR0
        SATA_SSTATUS_REG  SSTS;
    };
    union {
        ULONG             SControl;     // SCR2
        SATA_SCONTROL_REG SCTL;
    };
    union {
        ULONG             SError;       // SCR1
        SATA_SERROR_REG   SERR;
    };
    union {
        ULONG SACT;      // SCR3
        ULONG SActive;   // bitmask
    };
    ULONG CI;            // Command issue, bitmask

    // AHCI 1.1
    union {
        ULONG Reg;
        struct {
            USHORT PMN;     // PM Notify, bitmask
            USHORT Reserved;
        };
    } SNTF;
    ULONG FIS_Switching_Reserved[12];
    UCHAR VendorSpec[16];

} IDE_AHCI_PORT_REGISTERS, *PIDE_AHCI_PORT_REGISTERS;

#define IDX_AHCI_P_IS                     (FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, IS))
#define IDX_AHCI_P_CI                     (FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, CI))

typedef struct _IDE_AHCI_PRD_ENTRY {
    union {
        ULONG base;
        ULONGLONG base64;
        struct {
            ULONG DBA;
            union {
                ULONG DBAU;
                ULONG baseu;
            };
        };
    };
    ULONG Reserved1;

    ULONG DBC:22;
    ULONG Reserved2:9;
    ULONG I:1;

} IDE_AHCI_PRD_ENTRY, *PIDE_AHCI_PRD_ENTRY;

#define ATA_AHCI_DMA_ENTRIES		(PAGE_SIZE/2/sizeof(IDE_AHCI_PRD_ENTRY))   /* 128 */

typedef struct _IDE_AHCI_CMD {
    UCHAR              cfis[64];
    UCHAR              acmd[32];
    UCHAR              Reserved[32];
    IDE_AHCI_PRD_ENTRY prd_tab[ATA_AHCI_DMA_ENTRIES];
} IDE_AHCI_CMD, *PIDE_AHCI_CMD;

typedef struct _IDE_AHCI_CMD_LIST {
    USHORT             cmd_flags;
    USHORT             prd_length;     /* PRD entries */
    ULONG              bytecount;
    ULONGLONG          cmd_table_phys; /* 128byte aligned */
} IDE_AHCI_CMD_LIST, *PIDE_AHCI_CMD_LIST;

#define IsBusMaster(pciData) \
    ( ((pciData)->Command & (PCI_ENABLE_BUS_MASTER/* | PCI_ENABLE_IO_SPACE*/)) == \
          (PCI_ENABLE_BUS_MASTER/* | PCI_ENABLE_IO_SPACE*/))

#define PCI_IDE_PROGIF_NATIVE_1         0x01
#define PCI_IDE_PROGIF_NATIVE_2         0x04
#define PCI_IDE_PROGIF_NATIVE_ALL       0x05

#define IsMasterDev(pciData) \
    ( ((pciData)->ProgIf & 0x80) && \
      ((pciData)->ProgIf & PCI_IDE_PROGIF_NATIVE_ALL) != PCI_IDE_PROGIF_NATIVE_ALL )

//#define INT_Q_SIZE 32
#define MIN_REQ_TTL 4

union _ATA_REQ;

typedef union _ATA_REQ {
//    ULONG               reqId;          // serial
    struct {

        union {

            struct {
                union _ATA_REQ*     next_req;
                union _ATA_REQ*     prev_req;

                PSCSI_REQUEST_BLOCK Srb;            // Current request on controller.

                PUSHORT             DataBuffer;     // Data buffer pointer.
                ULONG               WordsLeft;      // Data words left.
                ULONG               TransferLength; // Originally requested transfer length
                LONGLONG            lba;
                ULONG               WordsTransfered;// Data words already transfered.
                ULONG               bcount;

                UCHAR               retry;
                UCHAR               ttl;
            //    UCHAR               tag;
                UCHAR               Flags;
                UCHAR               ReqState;

                PSCSI_REQUEST_BLOCK OriginalSrb;    // Mechanism Status Srb Data

                ULONG               dma_entries;
                union {
                    ULONG           dma_base;
                    ULONGLONG       ahci_base64;    // for AHCI
                };
            };
            UCHAR padding_128b[128];
        };
        struct {
            union {
                BM_DMA_ENTRY    dma_tab[ATA_DMA_ENTRIES];
                IDE_AHCI_CMD    ahci_cmd;       // for AHCI
            };
        };
    };

    UCHAR padding_4kb[PAGE_SIZE];

} ATA_REQ, *PATA_REQ;

#define REQ_FLAG_FORCE_DOWNRATE         0x01
#define REQ_FLAG_DMA_OPERATION          0x02
#define REQ_FLAG_REORDERABLE_CMD        0x04
#define REQ_FLAG_RW_MASK                0x08
#define     REQ_FLAG_READ                   0x08
#define     REQ_FLAG_WRITE                  0x00
#define REQ_FLAG_FORCE_DOWNRATE_LBA48   0x10
#define REQ_FLAG_DMA_DBUF               0x20
#define REQ_FLAG_DMA_DBUF_PRD           0x40

// Request states
#define REQ_STATE_NONE                  0x00
#define REQ_STATE_QUEUED                0x10

#define REQ_STATE_PREPARE_TO_TRANSFER   0x20
#define REQ_STATE_PREPARE_TO_NEXT       0x21
#define REQ_STATE_READY_TO_TRANSFER     0x30

#define REQ_STATE_EXPECTING_INTR        0x40
#define REQ_STATE_ATAPI_EXPECTING_CMD_INTR     0x41
#define REQ_STATE_ATAPI_EXPECTING_DATA_INTR    0x42
#define REQ_STATE_ATAPI_DO_NOTHING_INTR        0x43

#define REQ_STATE_EARLY_INTR            0x48

#define REQ_STATE_PROCESSING_INTR       0x50

#define REQ_STATE_DPC_INTR_REQ          0x51
#define REQ_STATE_DPC_RESET_REQ         0x52
#define REQ_STATE_DPC_COMPLETE_REQ      0x53

#define REQ_STATE_DPC_WAIT_BUSY0        0x57
#define REQ_STATE_DPC_WAIT_BUSY1        0x58
#define REQ_STATE_DPC_WAIT_BUSY         0x59
#define REQ_STATE_DPC_WAIT_DRQ          0x5a
#define REQ_STATE_DPC_WAIT_DRQ0         0x5b
#define REQ_STATE_DPC_WAIT_DRQ_ERR      0x5c

#define REQ_STATE_TRANSFER_COMPLETE     0x7f

// Command actions:
#define CMD_ACTION_PREPARE              0x01
#define CMD_ACTION_EXEC                 0x02
#define CMD_ACTION_ALL                  (CMD_ACTION_PREPARE | CMD_ACTION_EXEC)

// predefined Reorder costs
#define REORDER_COST_MAX               ((DEF_I64(0x1) << 60) - 1)
#define REORDER_COST_TTL               (REORDER_COST_MAX - 1)
#define REORDER_COST_INTERSECT         (REORDER_COST_MAX - 2)
#define REORDER_COST_DENIED            (REORDER_COST_MAX - 3)
#define REORDER_COST_RESELECT          (REORDER_COST_MAX/4)

#define REORDER_COST_SWITCH_RW_CD      (REORDER_COST_MAX/8)
#define REORDER_MCOST_SWITCH_RW_CD     (0)
#define REORDER_MCOST_SEEK_BACK_CD     (16)

#define REORDER_COST_SWITCH_RW_HDD     (0)
#define REORDER_MCOST_SWITCH_RW_HDD    (4)
#define REORDER_MCOST_SEEK_BACK_HDD    (2)

/*typedef struct _ATA_QUEUE {
    struct _ATA_REQ*    head_req; // index
    struct _ATA_REQ*    tail_req; // index
    ULONG               req_count;
    ULONG               dma_base;
    BM_DMA_ENTRY        dma_tab[ATA_DMA_ENTRIES];
} ATA_QUEUE, *PATA_QUEUE;*/

struct _HW_DEVICE_EXTENSION;
struct _HW_LU_EXTENSION;

typedef struct _IORES {
    ULONG Addr;
    ULONG MemIo:1;
    ULONG Reserved:31;
} IORES, *PIORES;

// Channel extension
typedef struct _HW_CHANNEL {

    PATA_REQ            cur_req;
    ULONG               cur_cdev;
/*    PATA_REQ            first_req;
    PATA_REQ            last_req;*/
    ULONG               queue_depth;
    ULONG               ChannelSelectWaitCount;

    UCHAR               DpcState;

    BOOLEAN             ExpectingInterrupt;    // Indicates expecting an interrupt
    BOOLEAN             RDP;    // Indicate last tape command was DSC Restrictive.
    // Indicates whether '0x1f0' is the base address. Used
    // in SMART Ioctl calls.
    BOOLEAN             PrimaryAddress;
    // Placeholder for the sub-command value of the last
    // SMART command.
    UCHAR               SmartCommand;
    // Reorder anabled
    BOOLEAN             UseReorder;
    // Placeholder for status register after a GET_MEDIA_STATUS command
    UCHAR               ReturningMediaStatus;

    BOOLEAN             CopyDmaBuffer;
    //BOOLEAN             MemIo;
    BOOLEAN             AltRegMap;

    //UCHAR               Reserved[3];

    MECHANICAL_STATUS_INFORMATION_HEADER MechStatusData;
    SENSE_DATA          MechStatusSense;
    ULONG               MechStatusRetryCount;
    SCSI_REQUEST_BLOCK  InternalSrb;

    ULONG               MaxTransferMode; // may differ from Controller's value due to 40-pin cable

    ULONG               ChannelCtrlFlags;
    ULONG               ResetInProgress; // flag
    LONG                DisableIntr;
    LONG                CheckIntr;

    ULONG               lChannel;

#define CHECK_INTR_ACTIVE               0x03
#define CHECK_INTR_DETECTED             0x02
#define CHECK_INTR_CHECK                0x01
#define CHECK_INTR_IDLE                 0x00

    ULONG       NextDpcChan;
    PHW_TIMER   HwScsiTimer;
    LONGLONG    DpcTime;
#if 0
    PHW_TIMER   HwScsiTimer1;
    PHW_TIMER   HwScsiTimer2;
    LONGLONG    DpcTime1;
//    PHW_TIMER           CurDpc;
//    LARGE_INTEGER       ActivationTime;

//    KDPC                Dpc;
//    KTIMER              Timer;
//    PHW_TIMER           HwScsiTimer;
//    KSPIN_LOCK          QueueSpinLock;
//    KIRQL               QueueOldIrql;
#endif
    struct _HW_DEVICE_EXTENSION* DeviceExtension;
    struct _HW_LU_EXTENSION* lun[2];

    // Double-buffering support
    PVOID   DB_PRD;
    ULONG   DB_PRD_PhAddr;
    PVOID   DB_IO;
    ULONG   DB_IO_PhAddr;

    PUCHAR  DmaBuffer;

    // 
    PIDE_AHCI_CMD_LIST       AHCI_CL;
    ULONGLONG                AHCI_CL_PhAddr;
    PVOID                    AHCI_FIS;  // is not actually used by UniATA now, but is required by AHCI controller
    ULONGLONG                AHCI_FIS_PhAddr;
    // Note: in contrast to FBSD, we keep PRD and CMD item in AtaReq structure 

#ifdef QUEUE_STATISTICS
    LONGLONG QueueStat[MAX_QUEUE_STAT];
    LONGLONG ReorderCount;
    LONGLONG IntersectCount;
    LONGLONG TryReorderCount;
    LONGLONG TryReorderHeadCount;
    LONGLONG TryReorderTailCount; /* in-order requests */
#endif //QUEUE_STATISTICS

    //ULONG BaseMemAddress;
    //ULONG BaseMemAddressOffset;
    IORES RegTranslation[IDX_MAX_REG];

} HW_CHANNEL, *PHW_CHANNEL;

#define CTRFLAGS_DMA_ACTIVE             0x0001
#define CTRFLAGS_DMA_RO                 0x0002
#define CTRFLAGS_DMA_OPERATION          0x0004
#define CTRFLAGS_INTR_DISABLED          0x0008
#define CTRFLAGS_DPC_REQ                0x0010
#define CTRFLAGS_ENABLE_INTR_REQ        0x0020
#define CTRFLAGS_LBA48                  0x0040
#define CTRFLAGS_DSC_BSY                0x0080
#define CTRFLAGS_NO_SLAVE               0x0100

#define GEOM_AUTO                       0xffffffff
#define GEOM_STD                        0x0000
#define GEOM_UNIATA                     0x0001
#define GEOM_ORIG                       0x0002
#define GEOM_MANUAL                     0x0003

#define DPC_STATE_NONE                  0x00
#define DPC_STATE_ISR                   0x10
#define DPC_STATE_DPC                   0x20
#define DPC_STATE_TIMER                 0x30
#define DPC_STATE_COMPLETE              0x40

// Logical unit extension
typedef struct _HW_LU_EXTENSION {
    IDENTIFY_DATA2 IdentifyData;
    ULONGLONG      NumOfSectors;
    ULONG          DeviceFlags;    // Flags word for each possible device. DFLAGS_XXX
    ULONG          DiscsPresent;   // Indicates number of platters on changer-ish devices.
    BOOLEAN        DWordIO;        // Indicates use of 32-bit PIO
    UCHAR          ReturningMediaStatus;

    UCHAR          TransferMode;          // current transfer mode
    UCHAR          LimitedTransferMode;   // user-defined or IDE cable limitation
    UCHAR          OrigTransferMode;      // transfer mode, returned by device IDENTIFY (can be changed via IOCTL)

    UCHAR          MaximumBlockXfer;
    UCHAR          Padding0[2];    // padding
    ULONG          ErrorCount;     // Count of errors. Used to turn off features.
 //   ATA_QUEUE      cmd_queue;
    LONGLONG       ReadCmdCost;
    LONGLONG       WriteCmdCost;
    LONGLONG       OtherCmdCost;
    LONGLONG       RwSwitchCost;
    LONGLONG       RwSwitchMCost;
    LONGLONG       SeekBackMCost;
    //
    PATA_REQ       first_req;
    PATA_REQ       last_req;
    ULONG          queue_depth;
    ULONG          last_write;

    ULONG          LunSelectWaitCount;

    // tuning options
    ULONG          opt_GeomType;
    ULONG          opt_MaxTransferMode;
    ULONG          opt_PreferedTransferMode;
    BOOLEAN        opt_ReadCacheEnable;
    BOOLEAN        opt_WriteCacheEnable;
    UCHAR          opt_ReadOnly;
    // padding
    BOOLEAN        opt_reserved[1];

    struct _SBadBlockListItem* bbListDescr;
    struct _SBadBlockRange* arrBadBlocks;
    ULONG           nBadBlocks;

    struct _HW_DEVICE_EXTENSION* DeviceExtension;

#ifdef IO_STATISTICS

    LONGLONG ModeErrorCount[MAX_RETRIES];
    LONGLONG RecoverCount[MAX_RETRIES];
    LONGLONG IoCount;

#endif//IO_STATISTICS
} HW_LU_EXTENSION, *PHW_LU_EXTENSION;

// Device extension
typedef struct _HW_DEVICE_EXTENSION {
    CHAR Signature[32];
    //PIDE_REGISTERS_1 BaseIoAddress1[IDE_MAX_CHAN];    // Base register locations
    //PIDE_REGISTERS_2 BaseIoAddress2[IDE_MAX_CHAN];
    ULONG BusInterruptLevel;    // Interrupt level
    ULONG InterruptMode;        // Interrupt Mode (Level or Edge)
    ULONG BusInterruptVector;
    // Number of channels being supported by one instantiation
    // of the device extension. Normally (and correctly) one, but
    // with so many broken PCI IDE controllers being sold, we have
    // to support them.
    ULONG NumberChannels;
    ULONG NumberLuns;
    ULONG FirstChannelToCheck;
#if 1
    HW_LU_EXTENSION lun[IDE_MAX_LUN];
    HW_CHANNEL chan[AHCI_MAX_PORT/*IDE_MAX_CHAN*/];
#else
    PHW_LU_EXTENSION lun;
    PHW_CHANNEL chan;
#endif
    UCHAR LastInterruptedChannel;
    // Indicates the number of blocks transferred per int. according to the
    // identify data.
    BOOLEAN DriverMustPoll;    // Driver is being used by the crash dump utility or ntldr.
    BOOLEAN BusMaster;
    BOOLEAN UseDpc;            // Indicates use of DPC on long waits
    IDENTIFY_DATA FullIdentifyData;    // Identify data for device
    // BusMaster specific data
//    PBM_DMA_ENTRY dma_tab_0;
    //KSPIN_LOCK  DpcSpinLock;

    ULONG       ActiveDpcChan;
    ULONG       FirstDpcChan;
/*
    PHW_TIMER   HwScsiTimer1;
    PHW_TIMER   HwScsiTimer2;
    LONGLONG    DpcTime1;
    LONGLONG    DpcTime2;
*/
    ULONG          queue_depth;

    PDEVICE_OBJECT Isr2DevObj;

    //PIDE_BUSMASTER_REGISTERS  BaseIoAddressBM_0;
    IORES                     BaseIoAddressBM_0;
    //PIDE_BUSMASTER_REGISTERS  BaseIoAddressBM[IDE_MAX_CHAN];

    // Device identification
    ULONG DevID;
    ULONG RevID;
    ULONG slotNumber;
    ULONG SystemIoBusNumber;
    ULONG DevIndex;

    ULONG InitMethod; // vendor specific

    ULONG Channel;

    ULONG HbaCtrlFlags;
    BOOLEAN simplexOnly;
    //BOOLEAN MemIo;
    BOOLEAN AltRegMap;
    BOOLEAN UnknownDev;
    BOOLEAN MasterDev;
    BOOLEAN Host64;
    BOOLEAN DWordIO;         // Indicates use of 32-bit PIO
    UCHAR   Reserved1[2];

    LONG  ReCheckIntr;

    ULONG MaxTransferMode;  // max transfer mode supported by controller
    ULONG HwFlags;
    INTERFACE_TYPE OrigAdapterInterfaceType;
    INTERFACE_TYPE AdapterInterfaceType;
    ULONG MaximumDmaTransferLength;
    ULONG AlignmentMask;

    //ULONG BaseMemAddress;

    //PIDE_SATA_REGISTERS       BaseIoAddressSATA_0;
    IORES                     BaseIoAddressSATA_0;
    //PIDE_SATA_REGISTERS       BaseIoAddressSATA[IDE_MAX_CHAN];

    IORES                     BaseIoAHCI_0;
    //PIDE_AHCI_PORT_REGISTERS  BaseIoAHCIPort[AHCI_MAX_PORT];

    BOOLEAN        opt_AtapiDmaZeroTransfer; // default FALSE
    BOOLEAN        opt_AtapiDmaControlCmd;   // default FALSE
    BOOLEAN        opt_AtapiDmaRawRead;      // default TRUE
    BOOLEAN        opt_AtapiDmaReadWrite;    // default TRUE

    PCHAR          FullDevName;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

typedef struct _ISR2_DEVICE_EXTENSION {
    PHW_DEVICE_EXTENSION HwDeviceExtension;
    ULONG DevIndex;
} ISR2_DEVICE_EXTENSION, *PISR2_DEVICE_EXTENSION;

#define HBAFLAGS_DMA_DISABLED           0x01
#define HBAFLAGS_DMA_DISABLED_LBA48     0x02

extern UCHAR         pciBuffer[256];
extern PBUSMASTER_CONTROLLER_INFORMATION BMList;
extern ULONG         BMListLen;
extern ULONG         IsaCount;
extern ULONG         MCACount;

//extern const CHAR retry_Wdma[MAX_RETRIES+1];
//extern const CHAR retry_Udma[MAX_RETRIES+1];

extern VOID
UniataEnumBusMasterController(
    IN PVOID DriverObject,
    PVOID Argument2
    );

extern ULONG DDKAPI
UniataFindCompatBusMasterController1(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

extern ULONG DDKAPI
UniataFindCompatBusMasterController2(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

extern ULONG DDKAPI
UniataFindBusMasterController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

extern ULONG DDKAPI
UniataFindFakeBusMasterController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

extern NTSTATUS
UniataConnectIntr2(
    IN PVOID HwDeviceExtension
    );

extern NTSTATUS
UniataDisconnectIntr2(
    IN PVOID HwDeviceExtension
    );

extern ULONG
ScsiPortGetBusDataByOffset(
    IN PVOID  HwDeviceExtension,
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG  BusNumber,
    IN ULONG  SlotNumber,
    IN PVOID  Buffer,
    IN ULONG  Offset,
    IN ULONG  Length
    );

#define PCIBUSNUM_NOT_SPECIFIED    (0xffffffffL)
#define PCISLOTNUM_NOT_SPECIFIED   (0xffffffffL)

extern ULONG
AtapiFindListedDev(
    PBUSMASTER_CONTROLLER_INFORMATION BusMasterAdapters,
    ULONG     lim,
    IN PVOID  HwDeviceExtension,
    IN ULONG  BusNumber,
    IN ULONG  SlotNumber,
    OUT PCI_SLOT_NUMBER* _slotData // optional
    );

extern ULONG
AtapiFindDev(
    IN PVOID  HwDeviceExtension,
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG  BusNumber,
    IN ULONG  SlotNumber,
    IN ULONG  dev_id,
    IN ULONG  RevID
    );

extern VOID
AtapiDmaAlloc(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG lChannel          // logical channel,
    );

extern BOOLEAN
AtapiDmaSetup(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,          // logical channel,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PUCHAR data,
    IN ULONG count
    );

extern BOOLEAN
AtapiDmaPioSync(
    PVOID  HwDeviceExtension,
    PSCSI_REQUEST_BLOCK Srb,
    PUCHAR data,
    ULONG  count
    );

extern BOOLEAN
AtapiDmaDBSync(
    PHW_CHANNEL chan,
    PSCSI_REQUEST_BLOCK Srb
    );

extern VOID
AtapiDmaStart(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,          // logical channel,
    IN PSCSI_REQUEST_BLOCK Srb
    );

extern UCHAR
AtapiDmaDone(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,          // logical channel,
    IN PSCSI_REQUEST_BLOCK Srb
    );

extern VOID
AtapiDmaReinit(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG ldev,
    IN PATA_REQ AtaReq
    );

extern VOID
AtapiDmaInit__(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG ldev
    );

extern VOID
AtapiDmaInit(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,          // logical channel,
                               // is always 0 except simplex-only controllers
    IN CHAR apiomode,
    IN CHAR wdmamode,
    IN CHAR udmamode
    );

extern BOOLEAN NTAPI
AtapiInterrupt2(
    IN PKINTERRUPT Interrupt,
    IN PVOID HwDeviceExtension
    );

extern PDRIVER_OBJECT SavedDriverObject;

extern BOOLEAN
UniataChipDetect(
    IN PVOID HwDeviceExtension,
    IN PPCI_COMMON_CONFIG pciData, // optional
    IN ULONG DeviceNumber,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN BOOLEAN* simplexOnly
    );

extern BOOLEAN
AtapiChipInit(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG c
    );

extern ULONG
AtapiGetIoRange(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN PPCI_COMMON_CONFIG pciData,
    IN ULONG SystemIoBusNumber,
    IN ULONG rid,
    IN ULONG offset,
    IN ULONG length //range id
    );

/****************** 1 *****************/
#define GetPciConfig1(offs, op) {                                \
    ScsiPortGetBusDataByOffset(HwDeviceExtension,                \
                               PCIConfiguration,                 \
                               SystemIoBusNumber,                \
                               slotNumber,                       \
                               &op,                              \
                               offs,                             \
                               1);                               \
}

#define SetPciConfig1(offs, op) {                                \
    UCHAR _a = op;                                              \
    ScsiPortSetBusDataByOffset(HwDeviceExtension,                \
                               PCIConfiguration,                 \
                               SystemIoBusNumber,                \
                               slotNumber,                       \
                               &_a,                              \
                               offs,                             \
                               1);                               \
}

#define ChangePciConfig1(offs, _op) {                            \
    UCHAR a = 0;                                                 \
    GetPciConfig1(offs, a);                                      \
    a = (UCHAR)(_op);                                            \
    SetPciConfig1(offs, a);                                      \
}

/****************** 2 *****************/
#define GetPciConfig2(offs, op) {                                \
    ScsiPortGetBusDataByOffset(HwDeviceExtension,                \
                               PCIConfiguration,                 \
                               SystemIoBusNumber,                \
                               slotNumber,                       \
                               &op,                              \
                               offs,                             \
                               2);                               \
}

#define SetPciConfig2(offs, op) {                                \
    USHORT _a = op;                                              \
    ScsiPortSetBusDataByOffset(HwDeviceExtension,                \
                               PCIConfiguration,                 \
                               SystemIoBusNumber,                \
                               slotNumber,                       \
                               &_a,                              \
                               offs,                             \
                               2);                               \
}

#define ChangePciConfig2(offs, _op) {                            \
    USHORT a = 0;                                                \
    GetPciConfig2(offs, a);                                      \
    a = (USHORT)(_op);                                           \
    SetPciConfig2(offs, a);                                      \
}

/****************** 4 *****************/
#define GetPciConfig4(offs, op) {                                \
    ScsiPortGetBusDataByOffset(HwDeviceExtension,                \
                               PCIConfiguration,                 \
                               SystemIoBusNumber,                \
                               slotNumber,                       \
                               &op,                              \
                               offs,                             \
                               4);                               \
}

#define SetPciConfig4(offs, op) {                                \
    ULONG _a = op;                                               \
    ScsiPortSetBusDataByOffset(HwDeviceExtension,                \
                               PCIConfiguration,                 \
                               SystemIoBusNumber,                \
                               slotNumber,                       \
                               &_a,                              \
                               offs,                             \
                               4);                               \
}

#define ChangePciConfig4(offs, _op) {                            \
    ULONG a = 0;                                                 \
    GetPciConfig4(offs, a);                                      \
    a = _op;                                                     \
    SetPciConfig4(offs, a);                                      \
}

#ifndef GetDmaStatus
#define GetDmaStatus(de, c) \
    (((de)->BusMaster) ? AtapiReadPort1(&((de)->chan[c]), IDX_BM_Status) : 0)
#endif //GetDmaStatus

#ifdef USE_OWN_DMA
#define AtapiVirtToPhysAddr(hwde, srb, phaddr, plen, phaddru) \
    AtapiVirtToPhysAddr_(hwde, srb, phaddr, plen, phaddru);
#else
#define AtapiVirtToPhysAddr(hwde, srb, phaddr, plen, phaddru) \
    (ScsiPortConvertPhysicalAddressToUlong/*(ULONG)ScsiPortGetVirtualAddress*/(/*hwde,*/  \
                    ScsiPortGetPhysicalAddress(hwde, srb, phaddr, plen)))
#endif //USE_OWN_DMA

VOID
DDKFASTAPI
AtapiWritePort4(
    IN PHW_CHANNEL chan,
    IN ULONG port,
    IN ULONG data
    );

VOID
DDKFASTAPI
AtapiWritePort2(
    IN PHW_CHANNEL chan,
    IN ULONG port,
    IN USHORT data
    );

VOID
DDKFASTAPI
AtapiWritePort1(
    IN PHW_CHANNEL chan,
    IN ULONG port,
    IN UCHAR data
    );

VOID
DDKFASTAPI
AtapiWritePortEx4(
    IN PHW_CHANNEL chan,
    IN ULONG port,
    IN ULONG offs,
    IN ULONG data
    );

VOID
DDKFASTAPI
AtapiWritePortEx1(
    IN PHW_CHANNEL chan,
    IN ULONG port,
    IN ULONG offs,
    IN UCHAR data
    );

ULONG
DDKFASTAPI
AtapiReadPort4(
    IN PHW_CHANNEL chan,
    IN ULONG port
    );

USHORT
DDKFASTAPI
AtapiReadPort2(
    IN PHW_CHANNEL chan,
    IN ULONG port
    );

UCHAR
DDKFASTAPI
AtapiReadPort1(
    IN PHW_CHANNEL chan,
    IN ULONG port
    );

ULONG
DDKFASTAPI
AtapiReadPortEx4(
    IN PHW_CHANNEL chan,
    IN ULONG port,
    IN ULONG offs
    );

UCHAR
DDKFASTAPI
AtapiReadPortEx1(
    IN PHW_CHANNEL chan,
    IN ULONG port,
    IN ULONG offs
    );

VOID
DDKFASTAPI
AtapiWriteBuffer4(
    IN PHW_CHANNEL chan,
    IN ULONG _port,
    IN PVOID Buffer,
    IN ULONG Count,
    IN ULONG Timing
    );

VOID
DDKFASTAPI
AtapiWriteBuffer2(
    IN PHW_CHANNEL chan,
    IN ULONG _port,
    IN PVOID Buffer,
    IN ULONG Count,
    IN ULONG Timing
    );

VOID
DDKFASTAPI
AtapiReadBuffer4(
    IN PHW_CHANNEL chan,
    IN ULONG _port,
    IN PVOID Buffer,
    IN ULONG Count,
    IN ULONG Timing
    );

VOID
DDKFASTAPI
AtapiReadBuffer2(
    IN PHW_CHANNEL chan,
    IN ULONG _port,
    IN PVOID Buffer,
    IN ULONG Count,
    IN ULONG Timing
    );

/*#define GET_CHANNEL(Srb)  (Srb->TargetId >> 1)
#define GET_LDEV(Srb)  (Srb->TargetId)
#define GET_LDEV2(P, T, L)  (T)*/

#define GET_CHANNEL(Srb)  (Srb->PathId)
#define GET_LDEV(Srb)  (Srb->TargetId | (Srb->PathId << 1))
#define GET_LDEV2(P, T, L)  (T | ((P)<<1))
#define GET_CDEV(Srb)  (Srb->TargetId)

#define AtapiSetupLunPtrs(chan, deviceExtension, c) \
{ \
        chan->DeviceExtension = deviceExtension; \
        chan->lChannel        = c; \
        chan->lun[0] = &(deviceExtension->lun[c*2+0]); \
        chan->lun[1] = &(deviceExtension->lun[c*2+1]); \
        chan->AltRegMap       = deviceExtension->AltRegMap; \
        chan->NextDpcChan     = -1; \
        chan->lun[0]->DeviceExtension = deviceExtension; \
        chan->lun[1]->DeviceExtension = deviceExtension; \
}

BOOLEAN
AtapiReadChipConfig(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG channel // physical channel
    );

VOID
UniataForgetDevice(
    PHW_LU_EXTENSION   LunExt
    );

extern ULONG SkipRaids;
extern ULONG ForceSimplex;

extern BOOLEAN InDriverEntry;

extern BOOLEAN g_opt_Verbose;

extern BOOLEAN WinVer_WDM_Model;

#endif //__IDE_BUSMASTER_H__
