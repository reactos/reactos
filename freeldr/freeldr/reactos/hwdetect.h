/*
 *  FreeLoader
 *
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2003  Casper S. Hornstrup  <chorns@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __HWDETECT_H
#define __HWDETECT_H

typedef U64 PHYSICAL_ADDRESS;

typedef enum _INTERFACE_TYPE
{
  InterfaceTypeUndefined = -1,
  Internal,
  Isa,
  Eisa,
  MicroChannel,
  TurboChannel,
  PCIBus,
  VMEBus,
  NuBus,
  PCMCIABus,
  CBus,
  MPIBus,
  MPSABus,
  ProcessorInternal,
  InternalPowerBus,
  PNPISABus,
  MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;

typedef enum _BUS_DATA_TYPE
{
  ConfigurationSpaceUndefined = -1,
  Cmos,
  EisaConfiguration,
  Pos,
  CbusConfiguration,
  PCIConfiguration,
  VMEConfiguration,
  NuBusConfiguration,
  PCMCIAConfiguration,
  MPIConfiguration,
  MPSAConfiguration,
  PNPISAConfiguration,
  MaximumBusDataType,
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

typedef struct _CM_INT13_DRIVE_PARAMETER {
  U16 DriveSelect;
  U32 MaxCylinders;
  U16 SectorsPerTrack;
  U16 MaxHeads;
  U16 NumberDrives;
} CM_INT13_DRIVE_PARAMETER, *PCM_INT13_DRIVE_PARAMETER;

typedef struct _CM_DISK_GEOMETRY_DEVICE_DATA {
  U32 BytesPerSector;
  U32 NumberOfCylinders;
  U32 SectorsPerTrack;
  U32 NumberOfHeads;
} CM_DISK_GEOMETRY_DEVICE_DATA, *PCM_DISK_GEOMETRY_DEVICE_DATA;

typedef struct {
  U8 Type;
  U8 ShareDisposition;
  U16 Flags;
  union {
    struct {
      PHYSICAL_ADDRESS Start;
      U32 Length;
    } __attribute__((packed)) Port;
    struct {
      U32 Level;
      U32 Vector;
      U32 Affinity;
    } __attribute__((packed)) Interrupt;
    struct {
      PHYSICAL_ADDRESS Start;
      U32 Length;
    } __attribute__((packed)) Memory;
    struct {
      U32 Channel;
      U32 Port;
      U32 Reserved1;
    } __attribute__((packed)) Dma;
    struct {
      U32 DataSize;
      U32 Reserved1;
      U32 Reserved2;
    } __attribute__((packed)) DeviceSpecificData;
  } __attribute__((packed)) u;
} __attribute__((packed)) CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;

typedef struct {
  U16 Version;
  U16 Revision;
  U32 Count;
  CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} __attribute__((packed))CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

typedef struct {
  INTERFACE_TYPE InterfaceType;
  U32 BusNumber;
  CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} __attribute__((packed)) CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

/* PCI bus definitions */

#define PCI_TYPE0_ADDRESSES	6
#define PCI_TYPE1_ADDRESSES	2
#define PCI_TYPE2_ADDRESSES	5

typedef struct _PCI_COMMON_CONFIG
{
  U16 VendorID;		/* read-only */
  U16 DeviceID;		/* read-only */
  U16 Command;
  U16 Status;
  U8  RevisionID;		/* read-only */
  U8  ProgIf;		/* read-only */
  U8  SubClass;		/* read-only */
  U8  BaseClass;		/* read-only */
  U8  CacheLineSize;		/* read-only */
  U8  LatencyTimer;		/* read-only */
  U8  HeaderType;		/* read-only */
  U8  BIST;
  union
    {
      struct _PCI_HEADER_TYPE_0
      	{
      	  U32  BaseAddresses[PCI_TYPE0_ADDRESSES];
      	  U32  CIS;
      	  U16 SubVendorID;
      	  U16 SubSystemID;
      	  U32  ROMBaseAddress;
      	  U32  Reserved2[2];
      
      	  U8  InterruptLine;
      	  U8  InterruptPin;		/* read-only */
      	  U8  MinimumGrant;		/* read-only */
      	  U8  MaximumLatency;	/* read-only */
      	} type0;

      /* PCI to PCI Bridge */
      struct _PCI_HEADER_TYPE_1
      	{
      	  U32  BaseAddresses[PCI_TYPE1_ADDRESSES];
      	  U8  PrimaryBus;
      	  U8  SecondaryBus;
      	  U8  SubordinateBus;
      	  U8  SecondaryLatency;
      	  U8  IOBase;
      	  U8  IOLimit;
      	  U16 SecondaryStatus;
      	  U16 MemoryBase;
      	  U16 MemoryLimit;
      	  U16 PrefetchBase;
      	  U16 PrefetchLimit;
      	  U32  PrefetchBaseUpper32;
      	  U32  PrefetchLimitUpper32;
      	  U16 IOBaseUpper16;
      	  U16 IOLimitUpper16;
      	  U8  CapabilitiesPtr;
      	  U8  Reserved1[3];
      	  U32  ROMBaseAddress;
      	  U8  InterruptLine;
      	  U8  InterruptPin;
      	  U16 BridgeControl;
      	} type1;

      /* PCI to CARDBUS Bridge */
      struct _PCI_HEADER_TYPE_2
      	{
      	  U32  SocketRegistersBaseAddress;
      	  U8  CapabilitiesPtr;
      	  U8  Reserved;
      	  U16 SecondaryStatus;
      	  U8  PrimaryBus;
      	  U8  SecondaryBus;
      	  U8  SubordinateBus;
      	  U8  SecondaryLatency;
      	  struct
      	    {
      	      U32 Base;
      	      U32 Limit;
      	    } Range[PCI_TYPE2_ADDRESSES-1];
      	  U8  InterruptLine;
      	  U8  InterruptPin;
      	  U16 BridgeControl;
      	} type2;
    } u;
  U8 DeviceSpecific[192];
} PCI_COMMON_CONFIG, *PPCI_COMMON_CONFIG;

#define PCI_COMMON_HDR_LENGTH (FIELD_OFFSET (PCI_COMMON_CONFIG, DeviceSpecific))

#define PCI_MAX_DEVICES                     32
#define PCI_MAX_FUNCTION                    8

#define PCI_INVALID_VENDORID                0xFFFF

/* Bit encodings for PCI_COMMON_CONFIG.HeaderType */

#define PCI_MULTIFUNCTION                   0x80
#define PCI_DEVICE_TYPE                     0x00
#define PCI_BRIDGE_TYPE                     0x01


/* Bit encodings for PCI_COMMON_CONFIG.Command */

#define PCI_ENABLE_IO_SPACE                 0x0001
#define PCI_ENABLE_MEMORY_SPACE             0x0002
#define PCI_ENABLE_BUS_MASTER               0x0004
#define PCI_ENABLE_SPECIAL_CYCLES           0x0008
#define PCI_ENABLE_WRITE_AND_INVALIDATE     0x0010
#define PCI_ENABLE_VGA_COMPATIBLE_PALETTE   0x0020
#define PCI_ENABLE_PARITY                   0x0040
#define PCI_ENABLE_WAIT_CYCLE               0x0080
#define PCI_ENABLE_SERR                     0x0100
#define PCI_ENABLE_FAST_BACK_TO_BACK        0x0200


/* Bit encodings for PCI_COMMON_CONFIG.Status */

#define PCI_STATUS_FAST_BACK_TO_BACK        0x0080
#define PCI_STATUS_DATA_PARITY_DETECTED     0x0100
#define PCI_STATUS_DEVSEL                   0x0600  /* 2 bits wide */
#define PCI_STATUS_SIGNALED_TARGET_ABORT    0x0800
#define PCI_STATUS_RECEIVED_TARGET_ABORT    0x1000
#define PCI_STATUS_RECEIVED_MASTER_ABORT    0x2000
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR    0x4000
#define PCI_STATUS_DETECTED_PARITY_ERROR    0x8000


/* PCI device classes */

#define PCI_CLASS_PRE_20                    0x00
#define PCI_CLASS_MASS_STORAGE_CTLR         0x01
#define PCI_CLASS_NETWORK_CTLR              0x02
#define PCI_CLASS_DISPLAY_CTLR              0x03
#define PCI_CLASS_MULTIMEDIA_DEV            0x04
#define PCI_CLASS_MEMORY_CTLR               0x05
#define PCI_CLASS_BRIDGE_DEV                0x06
#define PCI_CLASS_SIMPLE_COMMS_CTLR         0x07
#define PCI_CLASS_BASE_SYSTEM_DEV           0x08
#define PCI_CLASS_INPUT_DEV                 0x09
#define PCI_CLASS_DOCKING_STATION           0x0a
#define PCI_CLASS_PROCESSOR                 0x0b
#define PCI_CLASS_SERIAL_BUS_CTLR           0x0c


/* PCI device subclasses for class 1 (mass storage controllers)*/

#define PCI_SUBCLASS_MSC_SCSI_BUS_CTLR      0x00
#define PCI_SUBCLASS_MSC_IDE_CTLR           0x01
#define PCI_SUBCLASS_MSC_FLOPPY_CTLR        0x02
#define PCI_SUBCLASS_MSC_IPI_CTLR           0x03
#define PCI_SUBCLASS_MSC_RAID_CTLR          0x04
#define PCI_SUBCLASS_MSC_OTHER              0x80


/* Bit encodes for PCI_COMMON_CONFIG.u.type0.BaseAddresses */

#define PCI_ADDRESS_IO_SPACE                0x00000001
#define PCI_ADDRESS_MEMORY_TYPE_MASK        0x00000006
#define PCI_ADDRESS_MEMORY_PREFETCHABLE     0x00000008

#define PCI_ADDRESS_IO_ADDRESS_MASK         0xfffffffc
#define PCI_ADDRESS_MEMORY_ADDRESS_MASK     0xfffffff0
#define PCI_ADDRESS_ROM_ADDRESS_MASK        0xfffff800

#define PCI_TYPE_32BIT      0
#define PCI_TYPE_20BIT      2
#define PCI_TYPE_64BIT      4


/* Bit encodes for PCI_COMMON_CONFIG.u.type0.ROMBaseAddresses */

#define PCI_ROMADDRESS_ENABLED              0x00000001

typedef struct _PCI_SLOT_NUMBER
{
  union
    {
      struct
        {
          U32 DeviceNumber:5;
          U32 FunctionNumber:3;
          U32 Reserved:24;
        } bits;
      U32 AsULONG;
    } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;



/* ***** BEGIN ATA ***** */

#define  IDE_SECTOR_BUF_SZ      512
#define  IDE_MAX_POLL_RETRIES   100000
#define  IDE_MAX_BUSY_RETRIES   50000

// Control Block offsets and masks
#define  IDE_REG_DEV_CNTRL      0x0000  /* device control register */
#define    IDE_DC_nIEN            0x02  /* IRQ enable (active low) */

// Command Block offsets and masks
#define  IDE_REG_DATA_PORT      0x0000
#define  IDE_REG_ERROR          0x0001  /* error register */
#define    IDE_ER_AMNF            0x01  /* address mark not found */
#define    IDE_ER_TK0NF           0x02  /* Track 0 not found */
#define    IDE_ER_ABRT            0x04  /* Command aborted */
#define    IDE_ER_MCR             0x08  /* Media change requested */
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
#define  IDE_REG_STATUS         0x0007
#define    IDE_SR_BUSY            0x80
#define    IDE_SR_DRQ             0x08
#define    IDE_SR_ERR             0x01
#define  IDE_REG_COMMAND        0x0007

/* IDE/ATA commands */
#define    IDE_CMD_RESET            0x08
#define    IDE_CMD_IDENT_ATA_DRV    0xEC
#define    IDE_CMD_IDENT_ATAPI_DRV  0xA1

/* Access macros for command registers
   Each macro takes an address of the command port block, and data */
#define IDEWritePrecomp(Address, Data) \
  (WRITE_PORT_UCHAR((PU8)((Address) + IDE_REG_PRECOMP), (Data)))
#define IDEWriteSectorCount(Address, Data) \
  (WRITE_PORT_UCHAR((PU8)((Address) + IDE_REG_SECTOR_CNT), (Data)))
#define IDEWriteSectorNum(Address, Data) \
  (WRITE_PORT_UCHAR((PU8)((Address) + IDE_REG_SECTOR_NUM), (Data)))
#define IDEReadCylinderLow(Address) \
  (READ_PORT_UCHAR((PU8)((Address) + IDE_REG_CYL_LOW)))
#define IDEWriteCylinderLow(Address, Data) \
  (WRITE_PORT_UCHAR((PU8)((Address) + IDE_REG_CYL_LOW), (Data)))
#define IDEReadCylinderHigh(Address) \
  (READ_PORT_UCHAR((PU8)((Address) + IDE_REG_CYL_HIGH)))
#define IDEWriteCylinderHigh(Address, Data) \
  (WRITE_PORT_UCHAR((PU8)((Address) + IDE_REG_CYL_HIGH), (Data)))
#define IDEWriteDriveHead(Address, Data) \
  (WRITE_PORT_UCHAR((PU8)((Address) + IDE_REG_DRV_HEAD), (Data)))
#define IDEWriteDriveControl(Address, Data) \
  (WRITE_PORT_UCHAR((PU8)((Address) + IDE_REG_DEV_CNTRL), (Data)))
#define IDEReadStatus(Address) \
  (READ_PORT_UCHAR((PU8)((Address) + IDE_REG_STATUS)))
#define IDEWriteCommand(Address, Data) \
  (WRITE_PORT_UCHAR((PU8)((Address) + IDE_REG_COMMAND), (Data)))

/* Data block read and write commands */
#define IDEReadBlock(Address, Buffer, Count) \
  (READ_PORT_BUFFER_USHORT((PU16)((Address) + IDE_REG_DATA_PORT), \
  (PU16)(Buffer), (Count) / 2))

typedef struct _IDE_DRIVE_IDENTIFY
{
  U16  ConfigBits;             /*00*/
  U16  LogicalCyls;            /*01*/
  U16  Reserved02;             /*02*/
  U16  LogicalHeads;           /*03*/
  U16  BytesPerTrack;          /*04*/
  U16  BytesPerSector;         /*05*/
  U16  SectorsPerTrack;        /*06*/
  U8   InterSectorGap;         /*07*/
  U8   InterSectorGapSize;
  U8   Reserved08H;            /*08*/
  U8   BytesInPLO;
  U16  VendorUniqueCnt;        /*09*/
  CHAR SerialNumber[20];       /*10*/
  U16  ControllerType;         /*20*/
  U16  BufferSize;             /*21*/
  U16  ECCByteCnt;             /*22*/
  CHAR FirmwareRev[8];         /*23*/
  CHAR ModelNumber[40];        /*27*/
  U16  RWMultImplemented;      /*47*/
  U16  DWordIo;	               /*48*/
  U16  Capabilities;           /*49*/
#define IDE_DRID_STBY_SUPPORTED   0x2000
#define IDE_DRID_IORDY_SUPPORTED  0x0800
#define IDE_DRID_IORDY_DISABLE    0x0400
#define IDE_DRID_LBA_SUPPORTED    0x0200
#define IDE_DRID_DMA_SUPPORTED    0x0100
  U16  Reserved50;             /*50*/
  U16  MinPIOTransTime;        /*51*/
  U16  MinDMATransTime;        /*52*/
  U16  TMFieldsValid;          /*53*/
  U16  TMCylinders;            /*54*/
  U16  TMHeads;                /*55*/
  U16  TMSectorsPerTrk;        /*56*/
  U16  TMCapacityLo;           /*57*/
  U16  TMCapacityHi;           /*58*/
  U16  RWMultCurrent;          /*59*/
  U16  TMSectorCountLo;        /*60*/
  U16  TMSectorCountHi;        /*61*/
  U16  Reserved62[193];        /*62*/
  U16  Checksum;               /*255*/
} IDE_DRIVE_IDENTIFY, *PIDE_DRIVE_IDENTIFY;

/* ***** END ATA ***** */

typedef struct _DETECTED_BUS
{
  LIST_ENTRY ListEntry;
  INTERFACE_TYPE BusType;
  U32 BusNumber;
  CHAR Identifier[20];
} DETECTED_BUS, *PDETECTED_BUS;

typedef struct _DETECTED_BUSSES
{
  LIST_ENTRY Busses; /* DETECTED_BUS */
} DETECTED_BUSSES, *PDETECTED_BUSSES;

#if 0
typedef struct _DETECTED_STORAGE_CONTROLLER
{
  LIST_ENTRY ListEntry;
  INTERFACE_TYPE BusType;
  U32 BusNumber;
  U32 DriveCount;
  IDE_DRIVE_IDENTIFY IdeDriveIdentify[2];
} DETECTED_STORAGE_CONTROLLER, *PDETECTED_STORAGE_CONTROLLER;
#endif

typedef struct _DETECTED_DISK
{
  LIST_ENTRY ListEntry;
  INTERFACE_TYPE BusType;
  U32 BusNumber;
  CM_INT13_DRIVE_PARAMETER Int13DriveParameter;
  CM_DISK_GEOMETRY_DEVICE_DATA DiskGeometryDeviceData;
} DETECTED_DISK, *PDETECTED_DISK;

typedef struct _DETECTED_STORAGE
{
  LIST_ENTRY Disks; /* DETECTED_DISK */
} DETECTED_STORAGE, *PDETECTED_STORAGE;


typedef struct _REGISTRY_BUS_INFORMATION
{
  LIST_ENTRY ListEntry;
  HKEY BusKey;
  INTERFACE_TYPE BusType;
  U32 BusNumber;
} REGISTRY_BUS_INFORMATION, *PREGISTRY_BUS_INFORMATION;

VOID DetectHardware(VOID);

#endif /* __HWDETECT_H */
