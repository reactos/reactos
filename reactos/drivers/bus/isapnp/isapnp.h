#ifndef __ISAPNP_H
#define __ISAPNP_H

#ifdef __cplusplus
extern "C" {
#endif

#define ISAPNP_ADDRESS_PORT   0x0279    // ADDRESS (W)
#define ISAPNP_WRITE_PORT     0x0A79    // WRITE_DATA (W)
#define ISAPNP_MIN_READ_PORT  0x0203    // READ_DATA (R)
#define ISAPNP_MAX_READ_PORT  0x03FF    // READ_DATA (R)

// Card control registers
#define ISAPNP_CARD_READ_DATA_PORT   0x00  // Set READ_DATA port
#define ISAPNP_CARD_ISOLATION        0x01  // Isolation
#define ISAPNP_CARD_CONFIG_COTROL    0x02  // Configuration control
#define ISAPNP_CARD_WAKECSN          0x03  // Wake[CSN]
#define ISAPNP_CARD_RESOUCE_DATA     0x04  // Resource data port
#define ISAPNP_CARD_STATUS           0x05  // Status port
#define ISAPNP_CARD_CSN              0x06  // Card Select Number port
#define ISAPNP_CARD_LOG_DEVICE_NUM   0x07  // Logical Device Number
#define ISAPNP_CARD_RESERVED         0x08  // Card level reserved
#define ISAPNP_CARD_VENDOR_DEFINED   0x20  // Vendor defined

// Logical device control registers
#define ISAPNP_CONTROL_ACTIVATE         0x30  // Activate logical device
#define ISAPNP_CONTROL_IO_RANGE_CHECK   0x31  // I/O range conflict check
#define ISAPNP_CONTROL_LDC_RESERVED     0x32  // Logical Device Control reserved
#define ISAPNP_CONTROL_LDCV_RESERVED    0x38  // Logical Device Control Vendor reserved

// Logical device configuration registers
#define ISAPNP_CONFIG_MEMORY_BASE2    0x00  // Memory base address bits 23-16
#define ISAPNP_CONFIG_MEMORY_BASE1    0x01  // Memory base address bits 15-8
#define ISAPNP_CONFIG_MEMORY_CONTROL  0x02  // Memory control
#define ISAPNP_CONFIG_MEMORY_LIMIT2   0x03  // Memory limit bits 23-16
#define ISAPNP_CONFIG_MEMORY_LIMIT1   0x04  // Memory limit bits 15-8

#define ISAPNP_CONFIG_MEMORY_DESC0    0x40  // Memory descriptor 0
#define ISAPNP_CONFIG_MEMORY_DESC1    0x48  // Memory descriptor 1
#define ISAPNP_CONFIG_MEMORY_DESC2    0x50  // Memory descriptor 2
#define ISAPNP_CONFIG_MEMORY_DESC3    0x58  // Memory descriptor 3

#define ISAPNP_CONFIG_MEMORY32_BASE3    0x00  // 32-bit memory base address bits 31-24
#define ISAPNP_CONFIG_MEMORY32_BASE2    0x01  // 32-bit memory base address bits 23-16
#define ISAPNP_CONFIG_MEMORY32_BASE1    0x01  // 32-bit memory base address bits 15-8
#define ISAPNP_CONFIG_MEMORY32_CONTROL  0x02  // 32-bit memory control
#define ISAPNP_CONFIG_MEMORY32_LIMIT3   0x03  // 32-bit memory limit bits 31-24
#define ISAPNP_CONFIG_MEMORY32_LIMIT2   0x04  // 32-bit memory limit bits 23-16
#define ISAPNP_CONFIG_MEMORY32_LIMIT1   0x05  // 32-bit memory limit bits 15-8

#define ISAPNP_CONFIG_MEMORY32_DESC0  0x76  // 32-bit memory descriptor 0
#define ISAPNP_CONFIG_MEMORY32_DESC1  0x80  // 32-bit memory descriptor 1
#define ISAPNP_CONFIG_MEMORY32_DESC2  0x90  // 32-bit memory descriptor 2
#define ISAPNP_CONFIG_MEMORY32_DESC3  0xA0  // 32-bit memory descriptor 3

#define ISAPNP_CONFIG_IO_BASE1        0x00  // I/O port base address bits 15-8
#define ISAPNP_CONFIG_IO_BASE0        0x01  // I/O port base address bits 7-0

#define ISAPNP_CONFIG_IO_DESC0    0x60  // I/O port descriptor 0
#define ISAPNP_CONFIG_IO_DESC1    0x62  // I/O port descriptor 1
#define ISAPNP_CONFIG_IO_DESC2    0x64  // I/O port descriptor 2
#define ISAPNP_CONFIG_IO_DESC3    0x66  // I/O port descriptor 3
#define ISAPNP_CONFIG_IO_DESC4    0x68  // I/O port descriptor 4
#define ISAPNP_CONFIG_IO_DESC5    0x6A  // I/O port descriptor 5
#define ISAPNP_CONFIG_IO_DESC6    0x6C  // I/O port descriptor 6
#define ISAPNP_CONFIG_IO_DESC7    0x6E  // I/O port descriptor 7

#define ISAPNP_CONFIG_IRQ_LEVEL0    0x70  // Interupt level for descriptor 0
#define ISAPNP_CONFIG_IRQ_TYPE0     0x71  // Type level for descriptor 0
#define ISAPNP_CONFIG_IRQ_LEVEL1    0x72  // Interupt level for descriptor 1
#define ISAPNP_CONFIG_IRQ_TYPE1     0x73  // Type level for descriptor 1

#define ISAPNP_CONFIG_DMA_CHANNEL0    0x74  // DMA channel for descriptor 0
#define ISAPNP_CONFIG_DMA_CHANNEL1    0x75  // DMA channel for descriptor 1


typedef struct _PNPISA_SERIAL_ID
{
  UCHAR VendorId[4];    // Vendor Identifier
  UCHAR SerialId[4];    // Serial number
  UCHAR Checksum;       // Checksum
} PNPISA_SERIAL_ID, *PPNPISA_SERIAL_ID;


#define ISAPNP_RES_PRIORITY_PREFERRED   0
#define ISAPNP_RES_PRIORITY_ACCEPTABLE  1
#define ISAPNP_RES_PRIORITY_FUNCTIONAL  2
#define ISAPNP_RES_PRIORITY_INVALID	    65535


#define ISAPNP_RESOURCE_ITEM_TYPE   0x80      // 0 = small, 1 = large

// Small Resource Item Names (SRINs)
#define ISAPNP_SRIN_VERSION           0x1   // PnP version number
#define ISAPNP_SRIN_LDEVICE_ID        0x2   // Logical device id
#define ISAPNP_SRIN_CDEVICE_ID        0x3   // Compatible device id
#define ISAPNP_SRIN_IRQ_FORMAT        0x4   // IRQ format
#define ISAPNP_SRIN_DMA_FORMAT        0x5   // DMA format
#define ISAPNP_SRIN_START_DFUNCTION   0x6   // Start dependant function
#define ISAPNP_SRIN_END_DFUNCTION     0x7   // End dependant function
#define ISAPNP_SRIN_IO_DESCRIPTOR     0x8   // I/O port descriptor
#define ISAPNP_SRIN_FL_IO_DESCRIPOTOR 0x9   // Fixed location I/O port descriptor
#define ISAPNP_SRIN_VENDOR_DEFINED    0xE   // Vendor defined
#define ISAPNP_SRIN_END_TAG           0xF   // End tag

typedef struct _ISAPNP_SRI_VERSION
{
  UCHAR Header;
  UCHAR Version;        // Packed BCD format version number
  UCHAR VendorVersion;  // Vendor specific version number
} ISAPNP_SRI_VERSION, *PISAPNP_SRI_VERSION;

typedef struct _ISAPNP_SRI_LDEVICE_ID
{
  UCHAR Header;
  USHORT DeviceId;    // Logical device id
  USHORT VendorId;    // Manufacturer id
  UCHAR Flags;        // Flags
} ISAPNP_SRI_LDEVICE_ID, *PISAPNP_SRI_LDEVICE_ID;

typedef struct _ISAPNP_SRI_CDEVICE_ID
{
  UCHAR Header;
  USHORT DeviceId;    // Logical device id
  USHORT VendorId;    // Manufacturer id
} ISAPNP_SRI_CDEVICE_ID, *PISAPNP_SRI_CDEVICE_ID;

typedef struct _ISAPNP_SRI_IRQ_FORMAT
{
  UCHAR Header;
  USHORT Mask;          // IRQ mask (bit 0 = irq 0, etc.)
  UCHAR Information;    // IRQ information
} ISAPNP_SRI_IRQ_FORMAT, *PISAPNP_SRI_IRQ_FORMAT;

typedef struct _ISAPNP_SRI_DMA_FORMAT
{
  UCHAR Header;
  USHORT Mask;          // DMA channel mask (bit 0 = channel 0, etc.)
  UCHAR  Information;   // DMA information
} ISAPNP_SRI_DMA_FORMAT, *PISAPNP_SRI_DMA_FORMAT;

typedef struct _ISAPNP_SRI_START_DFUNCTION
{
  UCHAR Header;
} ISAPNP_SRI_START_DFUNCTION, *PISAPNP_SRI_START_DFUNCTION;

typedef struct _ISAPNP_SRI_END_DFUNCTION
{
  UCHAR Header;
} ISAPNP_SRI_END_DFUNCTION, *PISAPNP_SRI_END_DFUNCTION;

typedef struct _ISAPNP_SRI_IO_DESCRIPTOR
{
  UCHAR Header;
  UCHAR Information;    // Information
  USHORT RangeMinBase;  // Minimum base address
  USHORT RangeMaxBase;  // Maximum base address
  UCHAR Alignment;      // Base alignment
  UCHAR RangeLength;    // Length of range
} ISAPNP_SRI_IO_DESCRIPTOR, *PISAPNP_SRI_IO_DESCRIPTOR;

typedef struct _ISAPNP_SRI_FL_IO_DESCRIPTOR
{
  UCHAR Header;
  USHORT RangeBase;     // Range base address
  UCHAR RangeLength;    // Length of range
} ISAPNP_SRI_FL_IO_DESCRIPTOR, *PISAPNP_SRI_FL_IO_DESCRIPTOR;

typedef struct _PISAPNP_SRI_VENDOR_DEFINED
{
  UCHAR Header;
  UCHAR Reserved[0];  // Vendor defined
} ISAPNP_SRI_VENDOR_DEFINED, *PISAPNP_SRI_VENDOR_DEFINED;

typedef struct _ISAPNP_SRI_END_TAG
{
  UCHAR Header;
  UCHAR Checksum;   // Checksum
} ISAPNP_SRI_END_TAG, *PISAPNP_SRI_END_TAG;


typedef struct _ISAPNP_LRI
{
  UCHAR Header;
  USHORT Length;    // Length of data items
} ISAPNP_LRI, *PISAPNP_LRI;

// Large Resource Item Names (LRINs)
#define ISAPNP_LRIN_MEMORY_RANGE      0x1   // Memory range descriptor
#define ISAPNP_LRIN_ID_STRING_ANSI    0x2   // Identifier string (ANSI)
#define ISAPNP_LRIN_ID_STRING_UNICODE 0x3   // Identifier string (UNICODE)
#define ISAPNP_LRIN_VENDOR_DEFINED    0x4   // Vendor defined
#define ISAPNP_LRIN_MEMORY_RANGE32    0x5   // 32-bit memory range descriptor
#define ISAPNP_LRIN_FL_MEMORY_RANGE32 0x6   // 32-bit fixed location memory range descriptor

typedef struct _ISAPNP_LRI_MEMORY_RANGE
{
  UCHAR Header;
  USHORT Length;        // Length of data items
  UCHAR Information;    // Information
  USHORT RangeMinBase;  // Minimum base address
  USHORT RangeMaxBase;  // Maximum base address
  USHORT Alignment;     // Base alignment
  USHORT RangeLength;   // Length of range
} ISAPNP_LRI_MEMORY_RANGE, *PISAPNP_LRI_MEMORY_RANGE;

typedef struct _ISAPNP_LRI_ID_STRING_ANSI
{
  UCHAR Header;
  USHORT Length;        // Length of data items
  UCHAR String[0];      // Identifier string
} ISAPNP_LRI_ID_STRING_ANSI, *PISAPNP_LRI_ID_STRING_ANSI;

typedef struct _ISAPNP_LRI_ID_STRING_UNICODE
{
  UCHAR Header;
  USHORT Length;        // Length of data items
  USHORT CountryId;     // Country identifier
  USHORT String[0];     // Identifier string
} ISAPNP_LRI_ID_STRING_UNICODE, *PISAPNP_LRI_ID_STRING_UNICODE;

typedef struct _PISAPNP_LRI_VENDOR_DEFINED
{
  UCHAR Header;
  USHORT Length;        // Length of data items
  UCHAR Reserved[0];    // Vendor defined
} ISAPNP_LRI_VENDOR_DEFINED, *PISAPNP_LRI_VENDOR_DEFINED;

typedef struct _ISAPNP_LRI_MEMORY_RANGE32
{
  UCHAR Header;
  USHORT Length;        // Length of data items
  UCHAR Information;    // Information
  ULONG RangeMinBase;   // Minimum base address
  ULONG RangeMaxBase;   // Maximum base address
  ULONG Alignment;      // Base alignment
  ULONG RangeLength;    // Length of range
} ISAPNP_LRI_MEMORY_RANGE32, *PISAPNP_LRI_MEMORY_RANGE32;

typedef struct _ISAPNP_LRI_FL_MEMORY_RANGE32
{
  UCHAR Header;
  USHORT Length;        // Length of data items
  UCHAR Information;    // Information
  ULONG RangeMinBase;   // Minimum base address
  ULONG RangeMaxBase;   // Maximum base address
  ULONG RangeLength;    // Length of range
} ISAPNP_LRI_FL_MEMORY_RANGE32, *PISAPNP_LRI_FL_MEMORY_RANGE32;

typedef struct _ISAPNP_CARD
{
  LIST_ENTRY ListEntry;
  USHORT CardId;
  USHORT VendorId;
  USHORT DeviceId;
  ULONG Serial;
  UCHAR PNPVersion;
  UCHAR ProductVersion;
  UNICODE_STRING Name;
  LIST_ENTRY LogicalDevices;
  KSPIN_LOCK LogicalDevicesLock;
} ISAPNP_CARD, *PISAPNP_CARD;


typedef struct _ISAPNP_DESCRIPTOR
{
  LIST_ENTRY ListEntry;
  IO_RESOURCE_DESCRIPTOR Descriptor;
} ISAPNP_DESCRIPTOR, *PISAPNP_DESCRIPTOR;

typedef struct _ISAPNP_CONFIGURATION_LIST
{
  LIST_ENTRY ListEntry;
  ULONG Priority;
  LIST_ENTRY ListHead;
} ISAPNP_CONFIGURATION_LIST, *PISAPNP_CONFIGURATION_LIST;


#define MAX_COMPATIBLE_ID   32

typedef struct _ISAPNP_LOGICAL_DEVICE
{
  LIST_ENTRY CardListEntry;
  LIST_ENTRY DeviceListEntry;
  USHORT Number;
  USHORT VendorId;
  USHORT DeviceId;
  USHORT CVendorId[MAX_COMPATIBLE_ID];
  USHORT CDeviceId[MAX_COMPATIBLE_ID];
  USHORT Regs;
  PISAPNP_CARD Card;
  UNICODE_STRING Name;
  PDEVICE_OBJECT Pdo;
  PIO_RESOURCE_REQUIREMENTS_LIST ResourceLists;
  LIST_ENTRY Configuration;
  ULONG ConfigurationSize;
  ULONG DescriptorCount;
  ULONG CurrentDescriptorCount;
} ISAPNP_LOGICAL_DEVICE, *PISAPNP_LOGICAL_DEVICE;


typedef enum {
  dsStopped,
  dsStarted
} ISAPNP_DEVICE_STATE;

typedef struct _ISAPNP_DEVICE_EXTENSION
{
  // Physical Device Object
  PDEVICE_OBJECT Pdo;
  // Lower device object
  PDEVICE_OBJECT Ldo;
  // List of ISA PnP cards managed by this driver
  LIST_ENTRY CardListHead;
  // List of devices managed by this driver
  LIST_ENTRY DeviceListHead;
  // Number of devices managed by this driver
  ULONG DeviceListCount;
  // Spinlock for the linked lists
  KSPIN_LOCK GlobalListLock;
  // Current state of the driver
  ISAPNP_DEVICE_STATE State;
} ISAPNP_DEVICE_EXTENSION, *PISAPNP_DEVICE_EXTENSION;

#ifdef __cplusplus
}
#endif

#endif  /*  __ISAPNP_H  */
