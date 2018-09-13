/*++

Module Name:

    pci.h

Abstract:

    This is the PCI bus specific header file used by device drivers.

Author:

Revision History:

--*/

#ifndef _PCI_
#define _PCI_

// begin_ntddk

//
// A PCI driver can read the complete 256 bytes of configuration
// information for any PCI device by calling:
//
//      ULONG
//      HalGetBusData (
//          IN BUS_DATA_TYPE        PCIConfiguration,
//          IN ULONG                PciBusNumber,
//          IN PCI_SLOT_NUMBER      VirtualSlotNumber,
//          IN PPCI_COMMON_CONFIG   &PCIDeviceConfig,
//          IN ULONG                sizeof (PCIDeviceConfig)
//      );
//
//      A return value of 0 means that the specified PCI bus does not exist.
//
//      A return value of 2, with a VendorID of PCI_INVALID_VENDORID means
//      that the PCI bus does exist, but there is no device at the specified
//      VirtualSlotNumber (PCI Device/Function number).
//
//

// begin_wdm begin_ntminiport begin_ntndis

typedef struct _PCI_SLOT_NUMBER {
    union {
        struct {
            ULONG   DeviceNumber:5;
            ULONG   FunctionNumber:3;
            ULONG   Reserved:24;
        } bits;
        ULONG   AsULONG;
    } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;


#define PCI_TYPE0_ADDRESSES             6
#define PCI_TYPE1_ADDRESSES             2
#define PCI_TYPE2_ADDRESSES             5

typedef struct _PCI_COMMON_CONFIG {
    USHORT  VendorID;                   // (ro)
    USHORT  DeviceID;                   // (ro)
    USHORT  Command;                    // Device control
    USHORT  Status;
    UCHAR   RevisionID;                 // (ro)
    UCHAR   ProgIf;                     // (ro)
    UCHAR   SubClass;                   // (ro)
    UCHAR   BaseClass;                  // (ro)
    UCHAR   CacheLineSize;              // (ro+)
    UCHAR   LatencyTimer;               // (ro+)
    UCHAR   HeaderType;                 // (ro)
    UCHAR   BIST;                       // Built in self test

    union {
        struct _PCI_HEADER_TYPE_0 {
            ULONG   BaseAddresses[PCI_TYPE0_ADDRESSES];
            ULONG   CIS;
            USHORT  SubVendorID;
            USHORT  SubSystemID;
            ULONG   ROMBaseAddress;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved1[3];
            ULONG   Reserved2;
            UCHAR   InterruptLine;      //
            UCHAR   InterruptPin;       // (ro)
            UCHAR   MinimumGrant;       // (ro)
            UCHAR   MaximumLatency;     // (ro)
        } type0;

// end_wdm end_ntminiport end_ntndis

        //
        // PCI to PCI Bridge
        //

        struct _PCI_HEADER_TYPE_1 {
            ULONG   BaseAddresses[PCI_TYPE1_ADDRESSES];
            UCHAR   PrimaryBus;
            UCHAR   SecondaryBus;
            UCHAR   SubordinateBus;
            UCHAR   SecondaryLatency;
            UCHAR   IOBase;
            UCHAR   IOLimit;
            USHORT  SecondaryStatus;
            USHORT  MemoryBase;
            USHORT  MemoryLimit;
            USHORT  PrefetchBase;
            USHORT  PrefetchLimit;
            ULONG   PrefetchBaseUpper32;
            ULONG   PrefetchLimitUpper32;
            USHORT  IOBaseUpper16;
            USHORT  IOLimitUpper16;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved1[3];
            ULONG   ROMBaseAddress;
            UCHAR   InterruptLine;
            UCHAR   InterruptPin;
            USHORT  BridgeControl;
        } type1;

        //
        // PCI to CARDBUS Bridge
        //

        struct _PCI_HEADER_TYPE_2 {
            ULONG   SocketRegistersBaseAddress;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved;
            USHORT  SecondaryStatus;
            UCHAR   PrimaryBus;
            UCHAR   SecondaryBus;
            UCHAR   SubordinateBus;
            UCHAR   SecondaryLatency;
            struct  {
                ULONG   Base;
                ULONG   Limit;
            }       Range[PCI_TYPE2_ADDRESSES-1];
            UCHAR   InterruptLine;
            UCHAR   InterruptPin;
            USHORT  BridgeControl;
        } type2;

// begin_wdm begin_ntminiport begin_ntndis

    } u;

    UCHAR   DeviceSpecific[192];

} PCI_COMMON_CONFIG, *PPCI_COMMON_CONFIG;


#define PCI_COMMON_HDR_LENGTH (FIELD_OFFSET (PCI_COMMON_CONFIG, DeviceSpecific))

#define PCI_MAX_DEVICES                     32
#define PCI_MAX_FUNCTION                    8
#define PCI_MAX_BRIDGE_NUMBER               0xFF

#define PCI_INVALID_VENDORID                0xFFFF

//
// Bit encodings for  PCI_COMMON_CONFIG.HeaderType
//

#define PCI_MULTIFUNCTION                   0x80
#define PCI_DEVICE_TYPE                     0x00
#define PCI_BRIDGE_TYPE                     0x01
#define PCI_CARDBUS_BRIDGE_TYPE             0x02

#define PCI_CONFIGURATION_TYPE(PciData) \
    (((PPCI_COMMON_CONFIG)(PciData))->HeaderType & ~PCI_MULTIFUNCTION)

#define PCI_MULTIFUNCTION_DEVICE(PciData) \
    ((((PPCI_COMMON_CONFIG)(PciData))->HeaderType & PCI_MULTIFUNCTION) != 0)

//
// Bit encodings for PCI_COMMON_CONFIG.Command
//

#define PCI_ENABLE_IO_SPACE                 0x0001
#define PCI_ENABLE_MEMORY_SPACE             0x0002
#define PCI_ENABLE_BUS_MASTER               0x0004
#define PCI_ENABLE_SPECIAL_CYCLES           0x0008
#define PCI_ENABLE_WRITE_AND_INVALIDATE     0x0010
#define PCI_ENABLE_VGA_COMPATIBLE_PALETTE   0x0020
#define PCI_ENABLE_PARITY                   0x0040  // (ro+)
#define PCI_ENABLE_WAIT_CYCLE               0x0080  // (ro+)
#define PCI_ENABLE_SERR                     0x0100  // (ro+)
#define PCI_ENABLE_FAST_BACK_TO_BACK        0x0200  // (ro)

//
// Bit encodings for PCI_COMMON_CONFIG.Status
//

#define PCI_STATUS_CAPABILITIES_LIST        0x0010  // (ro)
#define PCI_STATUS_66MHZ_CAPABLE            0x0020  // (ro)
#define PCI_STATUS_UDF_SUPPORTED            0x0040  // (ro)
#define PCI_STATUS_FAST_BACK_TO_BACK        0x0080  // (ro)
#define PCI_STATUS_DATA_PARITY_DETECTED     0x0100
#define PCI_STATUS_DEVSEL                   0x0600  // 2 bits wide
#define PCI_STATUS_SIGNALED_TARGET_ABORT    0x0800
#define PCI_STATUS_RECEIVED_TARGET_ABORT    0x1000
#define PCI_STATUS_RECEIVED_MASTER_ABORT    0x2000
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR    0x4000
#define PCI_STATUS_DETECTED_PARITY_ERROR    0x8000

//
// The NT PCI Driver uses a WhichSpace parameter on its CONFIG_READ/WRITE
// routines.   The following values are defined-
//

#define PCI_WHICHSPACE_CONFIG               0x0
#define PCI_WHICHSPACE_ROM                  0x52696350

// end_wdm
//
// PCI Capability IDs
//

#define PCI_CAPABILITY_ID_POWER_MANAGEMENT  0x01
#define PCI_CAPABILITY_ID_AGP               0x02
#define PCI_CAPABILITY_ID_MSI               0x05

//
// All PCI Capability structures have the following header.
//
// CapabilityID is used to identify the type of the structure (is
// one of the PCI_CAPABILITY_ID values above.
//
// Next is the offset in PCI Configuration space (0x40 - 0xfc) of the
// next capability structure in the list, or 0x00 if there are no more
// entries.
//
typedef struct _PCI_CAPABILITIES_HEADER {
    UCHAR   CapabilityID;
    UCHAR   Next;
} PCI_CAPABILITIES_HEADER, *PPCI_CAPABILITIES_HEADER;

//
// Power Management Capability
//

typedef struct _PCI_PMC {
    UCHAR       Version:3;
    UCHAR       PMEClock:1;
    UCHAR       Rsvd1:1;
    UCHAR       DeviceSpecificInitialization:1;
    UCHAR       Rsvd2:2;
    struct _PM_SUPPORT {
        UCHAR   Rsvd2:1;
        UCHAR   D1:1;
        UCHAR   D2:1;
        UCHAR   PMED0:1;
        UCHAR   PMED1:1;
        UCHAR   PMED2:1;
        UCHAR   PMED3Hot:1;
        UCHAR   PMED3Cold:1;
    } Support;
} PCI_PMC, *PPCI_PMC;

typedef struct _PCI_PMCSR {
    USHORT      PowerState:2;
    USHORT      Rsvd1:6;
    USHORT      PMEEnable:1;
    USHORT      DataSelect:4;
    USHORT      DataScale:2;
    USHORT      PMEStatus:1;
} PCI_PMCSR, *PPCI_PMCSR;


typedef struct _PCI_PMCSR_BSE {
    UCHAR       Rsvd1:6;
    UCHAR       D3HotSupportsStopClock:1;       // B2_B3#
    UCHAR       BusPowerClockControlEnabled:1;  // BPCC_EN
} PCI_PMCSR_BSE, *PPCI_PMCSR_BSE;


typedef struct _PCI_PM_CAPABILITY {

    PCI_CAPABILITIES_HEADER Header;

    //
    // Power Management Capabilities (Offset = 2)
    //

    union {
        PCI_PMC         Capabilities;
        USHORT          AsUSHORT;
    } PMC;

    //
    // Power Management Control/Status (Offset = 4)
    //

    union {
        PCI_PMCSR       ControlStatus;
        USHORT          AsUSHORT;
    } PMCSR;

    //
    // PMCSR PCI-PCI Bridge Support Extensions
    //

    union {
        PCI_PMCSR_BSE   BridgeSupport;
        UCHAR           AsUCHAR;
    } PMCSR_BSE;

    //
    // Optional read only 8 bit Data register.  Contents controlled by
    // DataSelect and DataScale in ControlStatus.
    //

    UCHAR   Data;

} PCI_PM_CAPABILITY, *PPCI_PM_CAPABILITY;

//
// AGP Capability
//

typedef struct _PCI_AGP_CAPABILITY {

    PCI_CAPABILITIES_HEADER Header;

    USHORT  Minor:4;
    USHORT  Major:4;
    USHORT  Rsvd1:8;

    struct  _PCI_AGP_STATUS {
        ULONG   Rate:3;
        ULONG   Rsvd1:1;
        ULONG   FastWrite:1;
        ULONG   FourGB:1;
        ULONG   Rsvd2:3;
        ULONG   SideBandAddressing:1;                   // SBA
        ULONG   Rsvd3:14;
        ULONG   RequestQueueDepthMaximum:8;             // RQ
    } AGPStatus;

    struct  _PCI_AGP_COMMAND {
        ULONG   Rate:3;
        ULONG   Rsvd1:1;
        ULONG   FastWriteEnable:1;
        ULONG   FourGBEnable:1;
        ULONG   Rsvd2:2;
        ULONG   AGPEnable:1;
        ULONG   SBAEnable:1;
        ULONG   Rsvd3:14;
        ULONG   RequestQueueDepth:8;
    } AGPCommand;

} PCI_AGP_CAPABILITY, *PPCI_AGP_CAPABILITY;

#define PCI_AGP_RATE_1X     0x1
#define PCI_AGP_RATE_2X     0x2
#define PCI_AGP_RATE_4X     0x4

//
// MSI (Message Signalled Interrupts) Capability
//

typedef struct _PCI_MSI_CAPABILITY {

      PCI_CAPABILITIES_HEADER Header;

      struct _PCI_MSI_MESSAGE_CONTROL {
         USHORT  MSIEnable:1;
         USHORT  MultipleMessageCapable:3;
         USHORT  MultipleMessageEnable:3;
         USHORT  CapableOf64Bits:1;
         USHORT  Reserved:8;
      } MessageControl;

      union {
            struct _PCI_MSI_MESSAGE_ADDRESS {
               ULONG_PTR Reserved:2;              // always zero, DWORD aligned address
               ULONG_PTR Address:30;
            } Register;
            ULONG_PTR Raw;
      } MessageAddress;

      //
      // The rest of the Capability structure differs depending on whether
      // 32bit or 64bit addressing is being used.
      //
      // (The CapableOf64Bits bit above determines this)
      //

      union {

         // For 64 bit devices

         struct _PCI_MSI_64BIT_DATA {
            ULONG MessageUpperAddress;
            USHORT MessageData;
         } Bit64;

         // For 32 bit devices

         struct _PCI_MSI_32BIT_DATA {
            USHORT MessageData;
            ULONG Unused;
         } Bit32;
      } Data;

} PCI_MSI_CAPABILITY, *PPCI_PCI_CAPABILITY;

// begin_wdm
//
// Base Class Code encodings for Base Class (from PCI spec rev 2.1).
//

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

// 0d thru fe reserved

#define PCI_CLASS_NOT_DEFINED               0xff

//
// Sub Class Code encodings (PCI rev 2.1).
//

// Class 00 - PCI_CLASS_PRE_20

#define PCI_SUBCLASS_PRE_20_NON_VGA         0x00
#define PCI_SUBCLASS_PRE_20_VGA             0x01

// Class 01 - PCI_CLASS_MASS_STORAGE_CTLR

#define PCI_SUBCLASS_MSC_SCSI_BUS_CTLR      0x00
#define PCI_SUBCLASS_MSC_IDE_CTLR           0x01
#define PCI_SUBCLASS_MSC_FLOPPY_CTLR        0x02
#define PCI_SUBCLASS_MSC_IPI_CTLR           0x03
#define PCI_SUBCLASS_MSC_RAID_CTLR          0x04
#define PCI_SUBCLASS_MSC_OTHER              0x80

// Class 02 - PCI_CLASS_NETWORK_CTLR

#define PCI_SUBCLASS_NET_ETHERNET_CTLR      0x00
#define PCI_SUBCLASS_NET_TOKEN_RING_CTLR    0x01
#define PCI_SUBCLASS_NET_FDDI_CTLR          0x02
#define PCI_SUBCLASS_NET_ATM_CTLR           0x03
#define PCI_SUBCLASS_NET_OTHER              0x80

// Class 03 - PCI_CLASS_DISPLAY_CTLR

// N.B. Sub Class 00 could be VGA or 8514 depending on Interface byte

#define PCI_SUBCLASS_VID_VGA_CTLR           0x00
#define PCI_SUBCLASS_VID_XGA_CTLR           0x01
#define PCI_SUBCLASS_VID_OTHER              0x80

// Class 04 - PCI_CLASS_MULTIMEDIA_DEV

#define PCI_SUBCLASS_MM_VIDEO_DEV           0x00
#define PCI_SUBCLASS_MM_AUDIO_DEV           0x01
#define PCI_SUBCLASS_MM_OTHER               0x80

// Class 05 - PCI_CLASS_MEMORY_CTLR

#define PCI_SUBCLASS_MEM_RAM                0x00
#define PCI_SUBCLASS_MEM_FLASH              0x01
#define PCI_SUBCLASS_MEM_OTHER              0x80

// Class 06 - PCI_CLASS_BRIDGE_DEV

#define PCI_SUBCLASS_BR_HOST                0x00
#define PCI_SUBCLASS_BR_ISA                 0x01
#define PCI_SUBCLASS_BR_EISA                0x02
#define PCI_SUBCLASS_BR_MCA                 0x03
#define PCI_SUBCLASS_BR_PCI_TO_PCI          0x04
#define PCI_SUBCLASS_BR_PCMCIA              0x05
#define PCI_SUBCLASS_BR_NUBUS               0x06
#define PCI_SUBCLASS_BR_CARDBUS             0x07
#define PCI_SUBCLASS_BR_OTHER               0x80

// Class 07 - PCI_CLASS_SIMPLE_COMMS_CTLR

// N.B. Sub Class 00 and 01 additional info in Interface byte

#define PCI_SUBCLASS_COM_SERIAL             0x00
#define PCI_SUBCLASS_COM_PARALLEL           0x01
#define PCI_SUBCLASS_COM_OTHER              0x80

// Class 08 - PCI_CLASS_BASE_SYSTEM_DEV

// N.B. See Interface byte for additional info.

#define PCI_SUBCLASS_SYS_INTERRUPT_CTLR     0x00
#define PCI_SUBCLASS_SYS_DMA_CTLR           0x01
#define PCI_SUBCLASS_SYS_SYSTEM_TIMER       0x02
#define PCI_SUBCLASS_SYS_REAL_TIME_CLOCK    0x03
#define PCI_SUBCLASS_SYS_OTHER              0x80

// Class 09 - PCI_CLASS_INPUT_DEV

#define PCI_SUBCLASS_INP_KEYBOARD           0x00
#define PCI_SUBCLASS_INP_DIGITIZER          0x01
#define PCI_SUBCLASS_INP_MOUSE              0x02
#define PCI_SUBCLASS_INP_OTHER              0x80

// Class 0a - PCI_CLASS_DOCKING_STATION

#define PCI_SUBCLASS_DOC_GENERIC            0x00
#define PCI_SUBCLASS_DOC_OTHER              0x80

// Class 0b - PCI_CLASS_PROCESSOR

#define PCI_SUBCLASS_PROC_386               0x00
#define PCI_SUBCLASS_PROC_486               0x01
#define PCI_SUBCLASS_PROC_PENTIUM           0x02
#define PCI_SUBCLASS_PROC_ALPHA             0x10
#define PCI_SUBCLASS_PROC_POWERPC           0x20
#define PCI_SUBCLASS_PROC_COPROCESSOR       0x40

// Class 0c - PCI_CLASS_SERIAL_BUS_CTLR

#define PCI_SUBCLASS_SB_IEEE1394            0x00
#define PCI_SUBCLASS_SB_ACCESS              0x01
#define PCI_SUBCLASS_SB_SSA                 0x02
#define PCI_SUBCLASS_SB_USB                 0x03
#define PCI_SUBCLASS_SB_FIBRE_CHANNEL       0x04


// end_ntndis

//
// Bit encodes for PCI_COMMON_CONFIG.u.type0.BaseAddresses
//

#define PCI_ADDRESS_IO_SPACE                0x00000001  // (ro)
#define PCI_ADDRESS_MEMORY_TYPE_MASK        0x00000006  // (ro)
#define PCI_ADDRESS_MEMORY_PREFETCHABLE     0x00000008  // (ro)

#define PCI_ADDRESS_IO_ADDRESS_MASK         0xfffffffc
#define PCI_ADDRESS_MEMORY_ADDRESS_MASK     0xfffffff0
#define PCI_ADDRESS_ROM_ADDRESS_MASK        0xfffff800

#define PCI_TYPE_32BIT      0
#define PCI_TYPE_20BIT      2
#define PCI_TYPE_64BIT      4

//
// Bit encodes for PCI_COMMON_CONFIG.u.type0.ROMBaseAddresses
//

#define PCI_ROMADDRESS_ENABLED              0x00000001


//
// Reference notes for PCI configuration fields:
//
// ro   these field are read only.  changes to these fields are ignored
//
// ro+  these field are intended to be read only and should be initialized
//      by the system to their proper values.  However, driver may change
//      these settings.
//
// ---
//
//      All resources comsumed by a PCI device start as unitialized
//      under NT.  An uninitialized memory or I/O base address can be
//      determined by checking it's corrisponding enabled bit in the
//      PCI_COMMON_CONFIG.Command value.  An InterruptLine is unitialized
//      if it contains the value of -1.
//

// end_ntminiport

// end_ntddk end_wdm

//
// PCI_REGISTRY_INFO - this structure is passed into the HAL from
// the firmware.  It signifies how many PCI bus(es) are present and
// what style of access the PCI bus(es) support.
//

typedef struct _PCI_REGISTRY_INFO {
    UCHAR       MajorRevision;
    UCHAR       MinorRevision;
    UCHAR       NoBuses;
    UCHAR       HardwareMechanism;
} PCI_REGISTRY_INFO, *PPCI_REGISTRY_INFO;

//
// PCI definitions for IOBase & IOLimit
// PCIBridgeIO2Base(a,b)  - convert IOBase  & IOBaseUpper16 to ULONG IOBase
// PCIBridgeIO2Limit(a,b) - convert IOLimit & IOLimitUpper6 to ULONG IOLimit
//

#define PciBridgeIO2Base(a,b)   \
        ( ((a >> 4) << 12) + (((a & 0xf) == 1) ? (b << 16) : 0) )

#define PciBridgeIO2Limit(a,b)  (PciBridgeIO2Base(a,b) | 0xfff)

#define PciBridgeMemory2Base(a)  (ULONG) ((a & 0xfff0) << 16)
#define PciBridgeMemory2Limit(a) (PciBridgeMemory2Base(a) | 0xfffff)

//
// Bit encodes for PCI_COMMON_CONFIG.u.type1/2.BridgeControl
//

#define PCI_ENABLE_BRIDGE_PARITY_ERROR        0x0001
#define PCI_ENABLE_BRIDGE_SERR                0x0002
#define PCI_ENABLE_BRIDGE_ISA                 0x0004
#define PCI_ENABLE_BRIDGE_VGA                 0x0008
#define PCI_ENABLE_BRIDGE_MASTER_ABORT_SERR   0x0020
#define PCI_ASSERT_BRIDGE_RESET               0x0040

//
// Bit encodes for PCI_COMMON_CONFIG.u.type1.BridgeControl
//

#define PCI_ENABLE_BRIDGE_FAST_BACK_TO_BACK   0x0080

//
// Bit encodes for PCI_COMMON_CONFIG.u.type2.BridgeControl
//

#define PCI_ENABLE_CARDBUS_IRQ_ROUTING        0x0080
#define PCI_ENABLE_CARDBUS_MEM0_PREFETCH      0x0100
#define PCI_ENABLE_CARDBUS_MEM1_PREFETCH      0x0200
#define PCI_ENABLE_CARDBUS_WRITE_POSTING      0x0400

//
//  Definitions needed for Access to Hardware Type 1
//

#define PCI_TYPE1_ADDR_PORT     ((PULONG) 0xCF8)
#define PCI_TYPE1_DATA_PORT     0xCFC

typedef struct _PCI_TYPE1_CFG_BITS {
    union {
        struct {
            ULONG   Reserved1:2;
            ULONG   RegisterNumber:6;
            ULONG   FunctionNumber:3;
            ULONG   DeviceNumber:5;
            ULONG   BusNumber:8;
            ULONG   Reserved2:7;
            ULONG   Enable:1;
        } bits;

        ULONG   AsULONG;
    } u;
} PCI_TYPE1_CFG_BITS, *PPCI_TYPE1_CFG_BITS;


//
//  Definitions needed for Access to Hardware Type 2
//

#define PCI_TYPE2_CSE_PORT              ((PUCHAR) 0xCF8)
#define PCI_TYPE2_FORWARD_PORT          ((PUCHAR) 0xCFA)
#define PCI_TYPE2_ADDRESS_BASE          0xC


typedef struct _PCI_TYPE2_CSE_BITS {
    union {
        struct {
            UCHAR   Enable:1;
            UCHAR   FunctionNumber:3;
            UCHAR   Key:4;
        } bits;
        UCHAR   AsUCHAR;
    } u;
} PCI_TYPE2_CSE_BITS, PPCI_TYPE2_CSE_BITS;


typedef struct _PCI_TYPE2_ADDRESS_BITS {
    union {
        struct {
            USHORT  RegisterNumber:8;
            USHORT  Agent:4;
            USHORT  AddressBase:4;
        } bits;
        USHORT  AsUSHORT;
    } u;
} PCI_TYPE2_ADDRESS_BITS, *PPCI_TYPE2_ADDRESS_BITS;


//
// Definitions for the config cycle format on the PCI bus.
//

typedef struct _PCI_TYPE0_CFG_CYCLE_BITS {
    union {
        struct {
            ULONG   Reserved1:2;
            ULONG   RegisterNumber:6;
            ULONG   FunctionNumber:3;
            ULONG   Reserved2:21;
        } bits;
        ULONG   AsULONG;
    } u;
} PCI_TYPE0_CFG_CYCLE_BITS, *PPCI_TYPE0_CFG_CYCLE_BITS;

typedef struct _PCI_TYPE1_CFG_CYCLE_BITS {
    union {
        struct {
            ULONG   Reserved1:2;
            ULONG   RegisterNumber:6;
            ULONG   FunctionNumber:3;
            ULONG   DeviceNumber:5;
            ULONG   BusNumber:8;
            ULONG   Reserved2:8;
        } bits;
        ULONG   AsULONG;
    } u;
} PCI_TYPE1_CFG_CYCLE_BITS, *PPCI_TYPE1_CFG_CYCLE_BITS;


// begin_ntddk

//
// Portable portion of HAL & HAL bus extender definitions for BUSHANDLER
// BusData for installed PCI buses.
//

typedef VOID
(*PciPin2Line) (
    IN struct _BUS_HANDLER  *BusHandler,
    IN struct _BUS_HANDLER  *RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    );

typedef VOID
(*PciLine2Pin) (
    IN struct _BUS_HANDLER  *BusHandler,
    IN struct _BUS_HANDLER  *RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    );

typedef VOID
(*PciReadWriteConfig) (
    IN struct _BUS_HANDLER *BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

#define PCI_DATA_TAG            ' ICP'
#define PCI_DATA_VERSION        1

typedef struct _PCIBUSDATA {
    ULONG                   Tag;
    ULONG                   Version;
    PciReadWriteConfig      ReadConfig;
    PciReadWriteConfig      WriteConfig;
    PciPin2Line             Pin2Line;
    PciLine2Pin             Line2Pin;
    PCI_SLOT_NUMBER         ParentSlot;
    PVOID                   Reserved[4];
} PCIBUSDATA, *PPCIBUSDATA;

typedef ULONG (*PCI_READ_WRITE_CONFIG)(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

typedef VOID (*PCI_PIN_TO_LINE)(
    IN PVOID Context,
    IN PPCI_COMMON_CONFIG PciData
    );

typedef VOID (*PCI_LINE_TO_PIN)(
    IN PVOID Context,
    IN PPCI_COMMON_CONFIG PciNewData,
    IN PPCI_COMMON_CONFIG PciOldData
    );

typedef struct _PCI_BUS_INTERFACE_STANDARD {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    //
    // standard PCI bus interfaces
    //
    PCI_READ_WRITE_CONFIG ReadConfig;
    PCI_READ_WRITE_CONFIG WriteConfig;
    PCI_PIN_TO_LINE PinToLine;
    PCI_LINE_TO_PIN LineToPin;
} PCI_BUS_INTERFACE_STANDARD, *PPCI_BUS_INTERFACE_STANDARD;

#define PCI_BUS_INTERFACE_STANDARD_VERSION 1

#define PCI_DEVICE_PRESENT_INTERFACE_VERSION 1

typedef
BOOLEAN
(*PPCI_IS_DEVICE_PRESENT) (
    IN USHORT VendorID,
    IN USHORT DeviceID,
    IN UCHAR RevisionID,
    IN USHORT SubVendorID,
    IN USHORT SubSystemID,
    IN ULONG Flags
);

#define PCI_USE_SUBSYSTEM_IDS   0x00000001
#define PCI_USE_REVISION        0x00000002


typedef struct _PCI_DEVICE_PRESENT_INTERFACE {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    //
    // pci device info
    //
    PPCI_IS_DEVICE_PRESENT IsDevicePresent;
} PCI_DEVICE_PRESENT_INTERFACE, *PPCI_DEVICE_PRESENT_INTERFACE;


// end_ntddk
#endif
