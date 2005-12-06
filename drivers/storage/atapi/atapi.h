//
//  ATAPI.H - defines and typedefs for the IDE Driver module.
//

#ifndef __ATAPI_H
#define __ATAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#define  IDE_MAXIMUM_DEVICES    8

#define IDE_MAX_NAME_LENGTH     50

#define  IDE_SECTOR_BUF_SZ         512
#define  IDE_MAX_SECTORS_PER_XFER  256
#define  IDE_MAX_RESET_RETRIES     10000
#define  IDE_MAX_POLL_RETRIES      100000
#define  IDE_MAX_WRITE_RETRIES     1000
#define  IDE_MAX_BUSY_RETRIES      50000
#define  IDE_MAX_DRQ_RETRIES       10000
//#define  IDE_MAX_CMD_RETRIES       1
#define  IDE_MAX_CMD_RETRIES       0
#define  IDE_CMD_TIMEOUT           5
#define  IDE_RESET_PULSE_LENGTH    500  /* maybe a little too long */
#define  IDE_RESET_BUSY_TIMEOUT    120
#define  IDE_RESET_DRDY_TIMEOUT    120

// Control Block offsets and masks
#define  IDE_REG_ALT_STATUS     0x0000
#define  IDE_REG_DEV_CNTRL      0x0000  /* device control register */
#define    IDE_DC_SRST            0x04  /* drive reset (both drives) */
#define    IDE_DC_nIEN            0x02  /* IRQ enable (active low) */
#define  IDE_REG_DRV_ADDR       0x0001

// Command Block offsets and masks
#define  IDE_REG_DATA_PORT      0x0000
#define  IDE_REG_ERROR          0x0001  /* error register */
#define    IDE_ER_AMNF            0x01  /* addr mark not found */
#define    IDE_ER_TK0NF           0x02  /* track 0 not found */
#define    IDE_ER_ABRT            0x04  /* command aborted */
#define    IDE_ER_MCR             0x08  /* media change requested */
#define    IDE_ER_IDNF            0x10  /* ID not found */
#define    IDE_ER_MC              0x20  /* Media changed */
#define    IDE_ER_UNC             0x40  /* Uncorrectable data error */
#define  IDE_REG_PRECOMP        0x0001
#define  IDE_REG_SECTOR_CNT     0x0002
#define  IDE_REG_SECTOR_NUM     0x0003
#define  IDE_REG_CYL_LOW        0x0004
#define  IDE_REG_CYL_HIGH       0x0005
#define  IDE_REG_DRV_HEAD       0x0006
#define    IDE_DH_FIXED           0xA0
#define    IDE_DH_LBA             0x40
#define    IDE_DH_HDMASK          0x0F
#define    IDE_DH_DRV0            0x00
#define    IDE_DH_DRV1            0x10
#define  IDE_REG_STATUS           0x0007
#define    IDE_SR_BUSY              0x80
#define    IDE_SR_DRDY              0x40
#define    IDE_SR_WERR              0x20
#define    IDE_SR_DRQ               0x08
#define    IDE_SR_ERR               0x01
#define  IDE_REG_COMMAND          0x0007

/* IDE/ATA commands */
#define    IDE_CMD_RESET		0x08
#define    IDE_CMD_READ			0x20
#define    IDE_CMD_READ_ONCE		0x21
#define    IDE_CMD_READ_EXT		0x24	/* 48 bit */
#define    IDE_CMD_READ_DMA_EXT		0x25	/* 48 bit */
#define    IDE_CMD_READ_MULTIPLE_EXT	0x29	/* 48 bit */
#define    IDE_CMD_WRITE		0x30
#define    IDE_CMD_WRITE_ONCE		0x31
#define    IDE_CMD_WRITE_EXT		0x34	/* 48 bit */
#define    IDE_CMD_WRITE_DMA_EXT	0x35	/* 48 bit */
#define    IDE_CMD_WRITE_MULTIPLE_EXT	0x39	/* 48 bit */
#define    IDE_CMD_PACKET		0xA0
#define    IDE_CMD_READ_MULTIPLE	0xC4
#define    IDE_CMD_WRITE_MULTIPLE	0xC5
#define    IDE_CMD_READ_DMA		0xC8
#define    IDE_CMD_WRITE_DMA		0xCA
#define    IDE_CMD_FLUSH_CACHE		0xE7
#define    IDE_CMD_FLUSH_CACHE_EXT	0xEA	/* 48 bit */
#define    IDE_CMD_IDENT_ATA_DRV	0xEC
#define    IDE_CMD_IDENT_ATAPI_DRV	0xA1
#define    IDE_CMD_GET_MEDIA_STATUS	0xDA

//
//  Access macros for command registers
//  Each macro takes an address of the command port block, and data
//
#define IDEReadError(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address) + IDE_REG_ERROR)))
#define IDEWritePrecomp(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address) + IDE_REG_PRECOMP), (Data)))
#define IDEReadSectorCount(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address) + IDE_REG_SECTOR_CNT)))
#define IDEWriteSectorCount(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address) + IDE_REG_SECTOR_CNT), (Data)))
#define IDEReadSectorNum(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address) + IDE_REG_SECTOR_NUM)))
#define IDEWriteSectorNum(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address) + IDE_REG_SECTOR_NUM), (Data)))
#define IDEReadCylinderLow(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address) + IDE_REG_CYL_LOW)))
#define IDEWriteCylinderLow(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address) + IDE_REG_CYL_LOW), (Data)))
#define IDEReadCylinderHigh(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address) + IDE_REG_CYL_HIGH)))
#define IDEWriteCylinderHigh(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address) + IDE_REG_CYL_HIGH), (Data)))
#define IDEReadDriveHead(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address) + IDE_REG_DRV_HEAD)))
#define IDEWriteDriveHead(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address) + IDE_REG_DRV_HEAD), (Data)))
#define IDEReadStatus(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address) + IDE_REG_STATUS)))
#define IDEWriteCommand(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address) + IDE_REG_COMMAND), (Data)))
#define IDEReadDMACommand(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address))))
#define IDEWriteDMACommand(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address)), (Data)))
#define IDEReadDMAStatus(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address) + 2)))
#define IDEWriteDMAStatus(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address) + 2), (Data)))
#define IDEWritePRDTable(Address, Data) \
  (ScsiPortWritePortUlong((PULONG)((Address) + 4), (Data)))


//
//  Data block read and write commands
//
#define IDEReadBlock(Address, Buffer, Count) \
  (ScsiPortReadPortBufferUshort((PUSHORT)((Address) + IDE_REG_DATA_PORT), (PUSHORT)(Buffer), (Count) / 2))
#define IDEWriteBlock(Address, Buffer, Count) \
  (ScsiPortWritePortBufferUshort((PUSHORT)((Address) + IDE_REG_DATA_PORT), (PUSHORT)(Buffer), (Count) / 2))

#define IDEReadBlock32(Address, Buffer, Count) \
  (ScsiPortReadPortBufferUlong((PULONG)((Address) + IDE_REG_DATA_PORT), (PULONG)(Buffer), (Count) / 4))
#define IDEWriteBlock32(Address, Buffer, Count) \
  (ScsiPortWritePortBufferUlong((PULONG)((Address) + IDE_REG_DATA_PORT), (PULONG)(Buffer), (Count) / 4))

#define IDEReadWord(Address) \
  (ScsiPortReadPortUshort((PUSHORT)((Address) + IDE_REG_DATA_PORT)))

//
//  Access macros for control registers
//  Each macro takes an address of the control port blank and data
//
#define IDEReadAltStatus(Address) \
  (ScsiPortReadPortUchar((PUCHAR)((Address) + IDE_REG_ALT_STATUS)))
#define IDEWriteDriveControl(Address, Data) \
  (ScsiPortWritePortUchar((PUCHAR)((Address) + IDE_REG_DEV_CNTRL), (Data)))



//    IDE_DRIVE_IDENTIFY

typedef struct _IDE_DRIVE_IDENTIFY
{
  USHORT ConfigBits;          /*00*/
  USHORT LogicalCyls;         /*01*/
  USHORT Reserved02;          /*02*/
  USHORT LogicalHeads;        /*03*/
  USHORT BytesPerTrack;       /*04*/
  USHORT BytesPerSector;      /*05*/
  USHORT SectorsPerTrack;     /*06*/
  UCHAR  InterSectorGap;      /*07*/
  UCHAR  InterSectorGapSize;
  UCHAR  Reserved08H;         /*08*/
  UCHAR  BytesInPLO;
  USHORT VendorUniqueCnt;     /*09*/
  UCHAR  SerialNumber[20];    /*10*/
  USHORT ControllerType;      /*20*/
  USHORT BufferSize;          /*21*/
  USHORT ECCByteCnt;          /*22*/
  UCHAR  FirmwareRev[8];      /*23*/
  UCHAR  ModelNumber[40];     /*27*/
  USHORT RWMultImplemented;   /*47*/
  USHORT DWordIo;             /*48*/
  USHORT Capabilities;        /*49*/
#define IDE_DRID_STBY_SUPPORTED   0x2000
#define IDE_DRID_IORDY_SUPPORTED  0x0800
#define IDE_DRID_IORDY_DISABLE    0x0400
#define IDE_DRID_LBA_SUPPORTED    0x0200
#define IDE_DRID_DMA_SUPPORTED    0x0100
  USHORT Reserved50;          /*50*/
  USHORT MinPIOTransTime;     /*51*/
  USHORT MinDMATransTime;     /*52*/
  USHORT TMFieldsValid;       /*53*/
  USHORT TMCylinders;         /*54*/
  USHORT TMHeads;             /*55*/
  USHORT TMSectorsPerTrk;     /*56*/
  USHORT TMCapacityLo;        /*57*/
  USHORT TMCapacityHi;        /*58*/
  USHORT RWMultCurrent;       /*59*/
  USHORT TMSectorCountLo;     /*60*/
  USHORT TMSectorCountHi;     /*61*/
  USHORT DmaModes;            /*62*/
  USHORT MultiDmaModes;       /*63*/
  USHORT Reserved64[5];       /*64*/
  USHORT Reserved69[2];       /*69*/
  USHORT Reserved71[4];       /*71*/
  USHORT MaxQueueDepth;       /*75*/
  USHORT Reserved76[4];       /*76*/
  USHORT MajorRevision;       /*80*/
  USHORT MinorRevision;       /*81*/
  USHORT SupportedFeatures82; /*82*/
  USHORT SupportedFeatures83; /*83*/
  USHORT SupportedFeatures84; /*84*/
  USHORT EnabledFeatures85;   /*85*/
  USHORT EnabledFeatures86;   /*86*/
  USHORT EnabledFeatures87;   /*87*/
  USHORT UltraDmaModes;       /*88*/
  USHORT Reserved89[11];      /*89*/
  USHORT Max48BitAddress[4];  /*100*/
  USHORT Reserved104[151];    /*104*/
  USHORT Checksum;            /*255*/
} IDE_DRIVE_IDENTIFY, *PIDE_DRIVE_IDENTIFY;


/* Special ATAPI commands */

#define ATAPI_FORMAT_UNIT	0x24
#define ATAPI_MODE_SELECT	0x55
#define ATAPI_MODE_SENSE	0x5A


/* Special ATAPI_MODE_SELECT (12 bytes) command block */

typedef struct _ATAPI_MODE_SELECT12
{
  UCHAR OperationCode;
  UCHAR Reserved1:4;
  UCHAR PFBit:1;
  UCHAR Reserved2:3;
  UCHAR Reserved3[5];
  UCHAR ParameterListLengthMsb;
  UCHAR ParameterListLengthLsb;
  UCHAR Reserved4[3];
} ATAPI_MODE_SELECT12, *PATAPI_MODE_SELECT12;


/* Special ATAPI_MODE_SENSE (12 bytes) command block */

typedef struct _ATAPI_MODE_SENSE12
{
  UCHAR OperationCode;
  UCHAR Reserved1;
  UCHAR PageCode:6;
  UCHAR Pc:2;
  UCHAR Reserved2[4];
  UCHAR ParameterListLengthMsb;
  UCHAR ParameterListLengthLsb;
  UCHAR Reserved3[3];
} ATAPI_MODE_SENSE12, *PATAPI_MODE_SENSE12;

#ifdef __cplusplus
}
#endif

#endif  /*  __ATAPT_H  */


