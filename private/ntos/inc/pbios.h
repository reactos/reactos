/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pbiosp.h

Abstract:

    PnP BIOS/ISA configuration data definitions

Author:

    Shie-Lin Tzong (shielint) April 12, 1995

Revision History:

--*/

//#include "nthal.h"
//#include "hal.h"

//
// Constants
//

#define SMALL_RESOURCE_TAG          (UCHAR)(0x00)
#define LARGE_RESOURCE_TAG          (UCHAR)(0x80)
#define SMALL_TAG_MASK              0xf8
#define SMALL_TAG_SIZE_MASK         7

//
// Small Resouce Tags with length bits stripped off
//

#define TAG_VERSION                 0x08
#define TAG_LOGICAL_ID              0x10
#define TAG_COMPATIBLE_ID           0x18
#define TAG_IRQ                     0x20
#define TAG_DMA                     0x28
#define TAG_START_DEPEND            0x30
#define TAG_END_DEPEND              0x38
#define TAG_IO                      0x40
#define TAG_IO_FIXED                0x48
#define TAG_VENDOR                  0x70
#define TAG_END                     0x78

//
// Large Resouce Tags
//

#define TAG_MEMORY                  0x81
#define TAG_ANSI_ID                 0x82
#define TAG_UNICODE_ID              0x83
#define TAG_LVENDOR                 0x84
#define TAG_MEMORY32                0x85
#define TAG_MEMORY32_FIXED          0x86

//
// Complete TAG if applicable.
//

#define TAG_COMPLETE_COMPATIBLE_ID  0x1C
#define TAG_COMPLETE_END            0x79

#include "pshpack1.h"

//
// PNP ISA Port descriptor definition
//

typedef struct _PNP_PORT_DESCRIPTOR_ {
    UCHAR   Tag;                    // 01000111B, small item name = 08, length = 7
    UCHAR   Information;            // bit [0] = 1 device decodes full 16 bit addr
                                    //         = 0 device decodes ISA addr bits[9-0]
    USHORT  MinimumAddress;
    USHORT  MaximumAddress;
    UCHAR   Alignment;              // Increment in 1 byte blocks
    UCHAR   Length;                 // # contiguous Port requested
} PNP_PORT_DESCRIPTOR, *PPNP_PORT_DESCRIPTOR;

//
// PNP ISA fixed Port descriptor definition
//

typedef struct _PNP_FIXED_PORT_DESCRIPTOR_ {
    UCHAR   Tag;                    // 01001011B, small item name = 09, length = 3
    USHORT  MinimumAddress;
    UCHAR   Length;                 // # contiguous Port requested
} PNP_FIXED_PORT_DESCRIPTOR, *PPNP_FIXED_PORT_DESCRIPTOR;

//
// PNP ISA IRQ descriptor definition
//

typedef struct _PNP_IRQ_DESCRIPTOR_ {
    UCHAR   Tag;                    // 0010001XB small item name = 4 length = 2/3
    USHORT  IrqMask;                // bit 0 is irq 0
    UCHAR   Information;            // Optional
} PNP_IRQ_DESCRIPTOR, *PPNP_IRQ_DESCRIPTOR;

//
// Masks for PNP_IRQ_DESCRIPTOR Information byte
//

#define PNP_IRQ_LEVEL_MASK          0xC
#define PNP_IRQ_EDGE_MASK           0x3

//
// PNP ISA DMA descriptor definition
//

typedef struct _PNP_DMA_DESCRIPTOR_ {
    UCHAR   Tag;                    // 00101010B, small item name = 05, length = 2
    UCHAR   ChannelMask;            // bit 0 is channel 0
    UCHAR   Flags;                  // see spec
} PNP_DMA_DESCRIPTOR, *PPNP_DMA_DESCRIPTOR;

//
// PNP ISA MEMORY descriptor
//

typedef struct _PNP_MEMORY_DESCRIPTOR_ {
    UCHAR   Tag;                    // 10000001B, Large item name = 1
    USHORT  Length;                 // Length of the descriptor = 9
    UCHAR   Information;            // See def below
    USHORT  MinimumAddress;         // address bit [8-23]
    USHORT  MaximumAddress;         // address bit [8-23]
    USHORT  Alignment;              // 0x0000 = 64KB
    USHORT  MemorySize;             // In 256 byte blocks
} PNP_MEMORY_DESCRIPTOR, *PPNP_MEMORY_DESCRIPTOR;

//
// PNP ISA MEMORY32 descriptor
//

typedef struct _PNP_MEMORY32_DESCRIPTOR_ {
    UCHAR   Tag;                    // 10000101B, Large item name = 5
    USHORT  Length;                 // Length of the descriptor = 17
    UCHAR   Information;            // See def below
    ULONG   MinimumAddress;         // 32 bit addr
    ULONG   MaximumAddress;         // 32 bit addr
    ULONG   Alignment;              // 32 bit alignment
    ULONG   MemorySize;             // 32 bit length
} PNP_MEMORY32_DESCRIPTOR, *PPNP_MEMORY32_DESCRIPTOR;

//
// PNP ISA FIXED MEMORY32 descriptor
//

typedef struct _PNP_FIXED_MEMORY32_DESCRIPTOR_ {
    UCHAR   Tag;                    // 10000110B, Large item name = 6
    USHORT  Length;                 // Length of the descriptor = 9
    UCHAR   Information;            // See def below
    ULONG   BaseAddress;            // 32 bit addr
    ULONG   MemorySize;             // 32 bit length
} PNP_FIXED_MEMORY32_DESCRIPTOR, *PPNP_FIXED_MEMORY32_DESCRIPTOR;

#define PNP_MEMORY_ROM_MASK            0x40
#define PNP_MEMORY_SHADOWABLE_MASK     0x20
#define PNP_MEMORY_CONTROL_MASK        0x18
    #define PNP_MEMORY_CONTROL_8BIT       00
    #define PNP_MEMORY_CONTROL_16BIT      01
    #define PNP_MEMORY_CONTROL_8AND16BIT  02
    #define PNP_MEMORY_CONTROL_32BIT      03
#define PNP_MEMORY_SUPPORT_TYPE_MASK   04
#define PNP_MEMORY_CACHE_SUPPORT_MASK  02
#define PNP_MEMORY_WRITE_STATUS_MASK   01

#define UNKNOWN_DOCKING_IDENTIFIER     0xffffffff
#define UNABLE_TO_DETERMINE_DOCK_CAPABILITIES 0x89
#define FUNCTION_NOT_SUPPORTED         0x82
#define SYSTEM_NOT_DOCKED              0x87

//
// Pnp BIOS device node structure
//

typedef struct _PNP_BIOS_DEVICE_NODE {
    USHORT  Size;
    UCHAR   Node;
    ULONG   ProductId;
    UCHAR   DeviceType[3];
    USHORT  DeviceAttributes;
    // followed by AllocatedResourceBlock, PossibleResourceBlock
    // and CompatibleDeviceId
} PNP_BIOS_DEVICE_NODE, *PPNP_BIOS_DEVICE_NODE;

//
// DeviceType definition
//

#define BASE_TYPE_DOCKING_STATION      0xA

//
// Device attributes definitions
//

#define DEVICE_DISABLEABLE             0x0001
#define DEVICE_CONFIGURABLE            0x0002
#define DEVICE_DOCKING                 0x0020
#define DEVICE_REMOVABLE               0x0040

#define DEVICE_CONFIGURABILITY(x)      ((x)&0x180)
#define DEVICE_CONFIG_STATIC_ONLY      0x0000
#define DEVICE_CONFIG_STATIC_DYNAMIC   0x0080
#define DEVICE_CONFIG_DYNAMIC_ONLY     0x0100

//
// Pnp BIOS Installation check
//

typedef struct _PNP_BIOS_INSTALLATION_CHECK {
    UCHAR   Signature[4];              // $PnP (ascii)
    UCHAR   Revision;
    UCHAR   Length;
    USHORT  ControlField;
    UCHAR   Checksum;
    ULONG   EventFlagAddress;          // Physical address
    USHORT  RealModeEntryOffset;
    USHORT  RealModeEntrySegment;
    USHORT  ProtectedModeEntryOffset;
    ULONG   ProtectedModeCodeBaseAddress;
    ULONG   OemDeviceId;
    USHORT  RealModeDataBaseAddress;
    ULONG  ProtectedModeDataBaseAddress;
} PNP_BIOS_INSTALLATION_CHECK, *PPNP_BIOS_INSTALLATION_CHECK;

#include "poppack.h"

//
// Pnp BIOS ControlField masks
//

#define PNP_BIOS_CONTROL_MASK          0x3
#define PNP_BIOS_EVENT_NOT_SUPPORTED   0
#define PNP_BIOS_EVENT_POLLING         1
#define PNP_BIOS_EVENT_ASYNC           2

//
// Pnp Bios event
//

#define ABOUT_TO_CHANGE_CONFIG         1
#define DOCK_CHANGED                   2
#define SYSTEM_DEVICE_CHANGED          3
#define CONFIG_CHANGE_FAILED           4
