/******************************************************************************
 *
 * Name: acrestyp.h - Defines, types, and structures for resource descriptors
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __ACRESTYP_H__
#define __ACRESTYP_H__


/*
 * Definitions for Resource Attributes
 */
typedef UINT16                          ACPI_RS_LENGTH;    /* Resource Length field is fixed at 16 bits */
typedef UINT32                          ACPI_RSDESC_SIZE;  /* Max Resource Descriptor size is (Length+3) = (64K-1)+3 */

/*
 * Memory Attributes
 */
#define ACPI_READ_ONLY_MEMORY           (UINT8) 0x00
#define ACPI_READ_WRITE_MEMORY          (UINT8) 0x01

#define ACPI_NON_CACHEABLE_MEMORY       (UINT8) 0x00
#define ACPI_CACHABLE_MEMORY            (UINT8) 0x01
#define ACPI_WRITE_COMBINING_MEMORY     (UINT8) 0x02
#define ACPI_PREFETCHABLE_MEMORY        (UINT8) 0x03

/*! [Begin] no source code translation */
/*
 * IO Attributes
 * The ISA IO ranges are:     n000-n0FFh,  n400-n4FFh, n800-n8FFh, nC00-nCFFh.
 * The non-ISA IO ranges are: n100-n3FFh,  n500-n7FFh, n900-nBFFh, nCD0-nFFFh.
 */
/*! [End] no source code translation !*/

#define ACPI_NON_ISA_ONLY_RANGES        (UINT8) 0x01
#define ACPI_ISA_ONLY_RANGES            (UINT8) 0x02
#define ACPI_ENTIRE_RANGE               (ACPI_NON_ISA_ONLY_RANGES | ACPI_ISA_ONLY_RANGES)

/* Type of translation - 1=Sparse, 0=Dense */

#define ACPI_SPARSE_TRANSLATION         (UINT8) 0x01

/*
 * IO Port Descriptor Decode
 */
#define ACPI_DECODE_10                  (UINT8) 0x00    /* 10-bit IO address decode */
#define ACPI_DECODE_16                  (UINT8) 0x01    /* 16-bit IO address decode */

/*
 * Interrupt attributes - used in multiple descriptors
 */

/* Triggering */

#define ACPI_LEVEL_SENSITIVE            (UINT8) 0x00
#define ACPI_EDGE_SENSITIVE             (UINT8) 0x01

/* Polarity */

#define ACPI_ACTIVE_HIGH                (UINT8) 0x00
#define ACPI_ACTIVE_LOW                 (UINT8) 0x01
#define ACPI_ACTIVE_BOTH                (UINT8) 0x02

/* Sharing */

#define ACPI_EXCLUSIVE                  (UINT8) 0x00
#define ACPI_SHARED                     (UINT8) 0x01

/* Wake */

#define ACPI_NOT_WAKE_CAPABLE           (UINT8) 0x00
#define ACPI_WAKE_CAPABLE               (UINT8) 0x01

/*
 * DMA Attributes
 */
#define ACPI_COMPATIBILITY              (UINT8) 0x00
#define ACPI_TYPE_A                     (UINT8) 0x01
#define ACPI_TYPE_B                     (UINT8) 0x02
#define ACPI_TYPE_F                     (UINT8) 0x03

#define ACPI_NOT_BUS_MASTER             (UINT8) 0x00
#define ACPI_BUS_MASTER                 (UINT8) 0x01

#define ACPI_TRANSFER_8                 (UINT8) 0x00
#define ACPI_TRANSFER_8_16              (UINT8) 0x01
#define ACPI_TRANSFER_16                (UINT8) 0x02

/*
 * Start Dependent Functions Priority definitions
 */
#define ACPI_GOOD_CONFIGURATION         (UINT8) 0x00
#define ACPI_ACCEPTABLE_CONFIGURATION   (UINT8) 0x01
#define ACPI_SUB_OPTIMAL_CONFIGURATION  (UINT8) 0x02

/*
 * 16, 32 and 64-bit Address Descriptor resource types
 */
#define ACPI_MEMORY_RANGE               (UINT8) 0x00
#define ACPI_IO_RANGE                   (UINT8) 0x01
#define ACPI_BUS_NUMBER_RANGE           (UINT8) 0x02

#define ACPI_ADDRESS_NOT_FIXED          (UINT8) 0x00
#define ACPI_ADDRESS_FIXED              (UINT8) 0x01

#define ACPI_POS_DECODE                 (UINT8) 0x00
#define ACPI_SUB_DECODE                 (UINT8) 0x01

/* Producer/Consumer */

#define ACPI_PRODUCER                   (UINT8) 0x00
#define ACPI_CONSUMER                   (UINT8) 0x01


/*
 * If possible, pack the following structures to byte alignment
 */
#ifndef ACPI_MISALIGNMENT_NOT_SUPPORTED
#pragma pack(1)
#endif

/* UUID data structures for use in vendor-defined resource descriptors */

typedef struct acpi_uuid
{
    UINT8                           Data[ACPI_UUID_LENGTH];
} ACPI_UUID;

typedef struct acpi_vendor_uuid
{
    UINT8                           Subtype;
    UINT8                           Data[ACPI_UUID_LENGTH];

} ACPI_VENDOR_UUID;

/*
 * Structures used to describe device resources
 */
typedef struct acpi_resource_irq
{
    UINT8                           DescriptorLength;
    UINT8                           Triggering;
    UINT8                           Polarity;
    UINT8                           Sharable;
    UINT8                           WakeCapable;
    UINT8                           InterruptCount;
    UINT8                           Interrupts[1];

} ACPI_RESOURCE_IRQ;

typedef struct acpi_resource_dma
{
    UINT8                           Type;
    UINT8                           BusMaster;
    UINT8                           Transfer;
    UINT8                           ChannelCount;
    UINT8                           Channels[1];

} ACPI_RESOURCE_DMA;

typedef struct acpi_resource_start_dependent
{
    UINT8                           DescriptorLength;
    UINT8                           CompatibilityPriority;
    UINT8                           PerformanceRobustness;

} ACPI_RESOURCE_START_DEPENDENT;


/*
 * The END_DEPENDENT_FUNCTIONS_RESOURCE struct is not
 * needed because it has no fields
 */


typedef struct acpi_resource_io
{
    UINT8                           IoDecode;
    UINT8                           Alignment;
    UINT8                           AddressLength;
    UINT16                          Minimum;
    UINT16                          Maximum;

} ACPI_RESOURCE_IO;

typedef struct acpi_resource_fixed_io
{
    UINT16                          Address;
    UINT8                           AddressLength;

} ACPI_RESOURCE_FIXED_IO;

typedef struct acpi_resource_fixed_dma
{
    UINT16                          RequestLines;
    UINT16                          Channels;
    UINT8                           Width;

} ACPI_RESOURCE_FIXED_DMA;

/* Values for Width field above */

#define ACPI_DMA_WIDTH8                         0
#define ACPI_DMA_WIDTH16                        1
#define ACPI_DMA_WIDTH32                        2
#define ACPI_DMA_WIDTH64                        3
#define ACPI_DMA_WIDTH128                       4
#define ACPI_DMA_WIDTH256                       5


typedef struct acpi_resource_vendor
{
    UINT16                          ByteLength;
    UINT8                           ByteData[1];

} ACPI_RESOURCE_VENDOR;

/* Vendor resource with UUID info (introduced in ACPI 3.0) */

typedef struct acpi_resource_vendor_typed
{
    UINT16                          ByteLength;
    UINT8                           UuidSubtype;
    UINT8                           Uuid[ACPI_UUID_LENGTH];
    UINT8                           ByteData[1];

} ACPI_RESOURCE_VENDOR_TYPED;

typedef struct acpi_resource_end_tag
{
    UINT8                           Checksum;

} ACPI_RESOURCE_END_TAG;

typedef struct acpi_resource_memory24
{
    UINT8                           WriteProtect;
    UINT16                          Minimum;
    UINT16                          Maximum;
    UINT16                          Alignment;
    UINT16                          AddressLength;

} ACPI_RESOURCE_MEMORY24;

typedef struct acpi_resource_memory32
{
    UINT8                           WriteProtect;
    UINT32                          Minimum;
    UINT32                          Maximum;
    UINT32                          Alignment;
    UINT32                          AddressLength;

} ACPI_RESOURCE_MEMORY32;

typedef struct acpi_resource_fixed_memory32
{
    UINT8                           WriteProtect;
    UINT32                          Address;
    UINT32                          AddressLength;

} ACPI_RESOURCE_FIXED_MEMORY32;

typedef struct acpi_memory_attribute
{
    UINT8                           WriteProtect;
    UINT8                           Caching;
    UINT8                           RangeType;
    UINT8                           Translation;

} ACPI_MEMORY_ATTRIBUTE;

typedef struct acpi_io_attribute
{
    UINT8                           RangeType;
    UINT8                           Translation;
    UINT8                           TranslationType;
    UINT8                           Reserved1;

} ACPI_IO_ATTRIBUTE;

typedef union acpi_resource_attribute
{
    ACPI_MEMORY_ATTRIBUTE           Mem;
    ACPI_IO_ATTRIBUTE               Io;

    /* Used for the *WordSpace macros */

    UINT8                           TypeSpecific;

} ACPI_RESOURCE_ATTRIBUTE;

typedef struct acpi_resource_source
{
    UINT8                           Index;
    UINT16                          StringLength;
    char                            *StringPtr;

} ACPI_RESOURCE_SOURCE;

/* Fields common to all address descriptors, 16/32/64 bit */

#define ACPI_RESOURCE_ADDRESS_COMMON \
    UINT8                           ResourceType; \
    UINT8                           ProducerConsumer; \
    UINT8                           Decode; \
    UINT8                           MinAddressFixed; \
    UINT8                           MaxAddressFixed; \
    ACPI_RESOURCE_ATTRIBUTE         Info;

typedef struct acpi_address16_attribute
{
    UINT16                          Granularity;
    UINT16                          Minimum;
    UINT16                          Maximum;
    UINT16                          TranslationOffset;
    UINT16                          AddressLength;

} ACPI_ADDRESS16_ATTRIBUTE;

typedef struct acpi_address32_attribute
{
    UINT32                          Granularity;
    UINT32                          Minimum;
    UINT32                          Maximum;
    UINT32                          TranslationOffset;
    UINT32                          AddressLength;

} ACPI_ADDRESS32_ATTRIBUTE;

typedef struct acpi_address64_attribute
{
    UINT64                          Granularity;
    UINT64                          Minimum;
    UINT64                          Maximum;
    UINT64                          TranslationOffset;
    UINT64                          AddressLength;

} ACPI_ADDRESS64_ATTRIBUTE;

typedef struct acpi_resource_address
{
    ACPI_RESOURCE_ADDRESS_COMMON

} ACPI_RESOURCE_ADDRESS;

typedef struct acpi_resource_address16
{
    ACPI_RESOURCE_ADDRESS_COMMON
    ACPI_ADDRESS16_ATTRIBUTE        Address;
    ACPI_RESOURCE_SOURCE            ResourceSource;

} ACPI_RESOURCE_ADDRESS16;

typedef struct acpi_resource_address32
{
    ACPI_RESOURCE_ADDRESS_COMMON
    ACPI_ADDRESS32_ATTRIBUTE        Address;
    ACPI_RESOURCE_SOURCE            ResourceSource;

} ACPI_RESOURCE_ADDRESS32;

typedef struct acpi_resource_address64
{
    ACPI_RESOURCE_ADDRESS_COMMON
    ACPI_ADDRESS64_ATTRIBUTE        Address;
    ACPI_RESOURCE_SOURCE            ResourceSource;

} ACPI_RESOURCE_ADDRESS64;

typedef struct acpi_resource_extended_address64
{
    ACPI_RESOURCE_ADDRESS_COMMON
    UINT8                           RevisionID;
    ACPI_ADDRESS64_ATTRIBUTE        Address;
    UINT64                          TypeSpecific;

} ACPI_RESOURCE_EXTENDED_ADDRESS64;

typedef struct acpi_resource_extended_irq
{
    UINT8                           ProducerConsumer;
    UINT8                           Triggering;
    UINT8                           Polarity;
    UINT8                           Sharable;
    UINT8                           WakeCapable;
    UINT8                           InterruptCount;
    ACPI_RESOURCE_SOURCE            ResourceSource;
    UINT32                          Interrupts[1];

} ACPI_RESOURCE_EXTENDED_IRQ;

typedef struct acpi_resource_generic_register
{
    UINT8                           SpaceId;
    UINT8                           BitWidth;
    UINT8                           BitOffset;
    UINT8                           AccessSize;
    UINT64                          Address;

} ACPI_RESOURCE_GENERIC_REGISTER;

typedef struct acpi_resource_gpio
{
    UINT8                           RevisionId;
    UINT8                           ConnectionType;
    UINT8                           ProducerConsumer;   /* For values, see Producer/Consumer above */
    UINT8                           PinConfig;
    UINT8                           Sharable;           /* For values, see Interrupt Attributes above */
    UINT8                           WakeCapable;        /* For values, see Interrupt Attributes above */
    UINT8                           IoRestriction;
    UINT8                           Triggering;         /* For values, see Interrupt Attributes above */
    UINT8                           Polarity;           /* For values, see Interrupt Attributes above */
    UINT16                          DriveStrength;
    UINT16                          DebounceTimeout;
    UINT16                          PinTableLength;
    UINT16                          VendorLength;
    ACPI_RESOURCE_SOURCE            ResourceSource;
    UINT16                          *PinTable;
    UINT8                           *VendorData;

} ACPI_RESOURCE_GPIO;

/* Values for GPIO ConnectionType field above */

#define ACPI_RESOURCE_GPIO_TYPE_INT             0
#define ACPI_RESOURCE_GPIO_TYPE_IO              1

/* Values for PinConfig field above */

#define ACPI_PIN_CONFIG_DEFAULT                 0
#define ACPI_PIN_CONFIG_PULLUP                  1
#define ACPI_PIN_CONFIG_PULLDOWN                2
#define ACPI_PIN_CONFIG_NOPULL                  3

/* Values for IoRestriction field above */

#define ACPI_IO_RESTRICT_NONE                   0
#define ACPI_IO_RESTRICT_INPUT                  1
#define ACPI_IO_RESTRICT_OUTPUT                 2
#define ACPI_IO_RESTRICT_NONE_PRESERVE          3


/* Common structure for I2C, SPI, and UART serial descriptors */

#define ACPI_RESOURCE_SERIAL_COMMON \
    UINT8                           RevisionId; \
    UINT8                           Type; \
    UINT8                           ProducerConsumer;    /* For values, see Producer/Consumer above */\
    UINT8                           SlaveMode; \
    UINT8                           TypeRevisionId; \
    UINT16                          TypeDataLength; \
    UINT16                          VendorLength; \
    ACPI_RESOURCE_SOURCE            ResourceSource; \
    UINT8                           *VendorData;

typedef struct acpi_resource_common_serialbus
{
    ACPI_RESOURCE_SERIAL_COMMON

} ACPI_RESOURCE_COMMON_SERIALBUS;

/* Values for the Type field above */

#define ACPI_RESOURCE_SERIAL_TYPE_I2C           1
#define ACPI_RESOURCE_SERIAL_TYPE_SPI           2
#define ACPI_RESOURCE_SERIAL_TYPE_UART          3

/* Values for SlaveMode field above */

#define ACPI_CONTROLLER_INITIATED               0
#define ACPI_DEVICE_INITIATED                   1


typedef struct acpi_resource_i2c_serialbus
{
    ACPI_RESOURCE_SERIAL_COMMON
    UINT8                           AccessMode;
    UINT16                          SlaveAddress;
    UINT32                          ConnectionSpeed;

} ACPI_RESOURCE_I2C_SERIALBUS;

/* Values for AccessMode field above */

#define ACPI_I2C_7BIT_MODE                      0
#define ACPI_I2C_10BIT_MODE                     1


typedef struct acpi_resource_spi_serialbus
{
    ACPI_RESOURCE_SERIAL_COMMON
    UINT8                           WireMode;
    UINT8                           DevicePolarity;
    UINT8                           DataBitLength;
    UINT8                           ClockPhase;
    UINT8                           ClockPolarity;
    UINT16                          DeviceSelection;
    UINT32                          ConnectionSpeed;

} ACPI_RESOURCE_SPI_SERIALBUS;

/* Values for WireMode field above */

#define ACPI_SPI_4WIRE_MODE                     0
#define ACPI_SPI_3WIRE_MODE                     1

/* Values for DevicePolarity field above */

#define ACPI_SPI_ACTIVE_LOW                     0
#define ACPI_SPI_ACTIVE_HIGH                    1

/* Values for ClockPhase field above */

#define ACPI_SPI_FIRST_PHASE                    0
#define ACPI_SPI_SECOND_PHASE                   1

/* Values for ClockPolarity field above */

#define ACPI_SPI_START_LOW                      0
#define ACPI_SPI_START_HIGH                     1


typedef struct acpi_resource_uart_serialbus
{
    ACPI_RESOURCE_SERIAL_COMMON
    UINT8                           Endian;
    UINT8                           DataBits;
    UINT8                           StopBits;
    UINT8                           FlowControl;
    UINT8                           Parity;
    UINT8                           LinesEnabled;
    UINT16                          RxFifoSize;
    UINT16                          TxFifoSize;
    UINT32                          DefaultBaudRate;

} ACPI_RESOURCE_UART_SERIALBUS;

/* Values for Endian field above */

#define ACPI_UART_LITTLE_ENDIAN                 0
#define ACPI_UART_BIG_ENDIAN                    1

/* Values for DataBits field above */

#define ACPI_UART_5_DATA_BITS                   0
#define ACPI_UART_6_DATA_BITS                   1
#define ACPI_UART_7_DATA_BITS                   2
#define ACPI_UART_8_DATA_BITS                   3
#define ACPI_UART_9_DATA_BITS                   4

/* Values for StopBits field above */

#define ACPI_UART_NO_STOP_BITS                  0
#define ACPI_UART_1_STOP_BIT                    1
#define ACPI_UART_1P5_STOP_BITS                 2
#define ACPI_UART_2_STOP_BITS                   3

/* Values for FlowControl field above */

#define ACPI_UART_FLOW_CONTROL_NONE             0
#define ACPI_UART_FLOW_CONTROL_HW               1
#define ACPI_UART_FLOW_CONTROL_XON_XOFF         2

/* Values for Parity field above */

#define ACPI_UART_PARITY_NONE                   0
#define ACPI_UART_PARITY_EVEN                   1
#define ACPI_UART_PARITY_ODD                    2
#define ACPI_UART_PARITY_MARK                   3
#define ACPI_UART_PARITY_SPACE                  4

/* Values for LinesEnabled bitfield above */

#define ACPI_UART_CARRIER_DETECT                (1<<2)
#define ACPI_UART_RING_INDICATOR                (1<<3)
#define ACPI_UART_DATA_SET_READY                (1<<4)
#define ACPI_UART_DATA_TERMINAL_READY           (1<<5)
#define ACPI_UART_CLEAR_TO_SEND                 (1<<6)
#define ACPI_UART_REQUEST_TO_SEND               (1<<7)


/* ACPI_RESOURCE_TYPEs */

#define ACPI_RESOURCE_TYPE_IRQ                  0
#define ACPI_RESOURCE_TYPE_DMA                  1
#define ACPI_RESOURCE_TYPE_START_DEPENDENT      2
#define ACPI_RESOURCE_TYPE_END_DEPENDENT        3
#define ACPI_RESOURCE_TYPE_IO                   4
#define ACPI_RESOURCE_TYPE_FIXED_IO             5
#define ACPI_RESOURCE_TYPE_VENDOR               6
#define ACPI_RESOURCE_TYPE_END_TAG              7
#define ACPI_RESOURCE_TYPE_MEMORY24             8
#define ACPI_RESOURCE_TYPE_MEMORY32             9
#define ACPI_RESOURCE_TYPE_FIXED_MEMORY32       10
#define ACPI_RESOURCE_TYPE_ADDRESS16            11
#define ACPI_RESOURCE_TYPE_ADDRESS32            12
#define ACPI_RESOURCE_TYPE_ADDRESS64            13
#define ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64   14  /* ACPI 3.0 */
#define ACPI_RESOURCE_TYPE_EXTENDED_IRQ         15
#define ACPI_RESOURCE_TYPE_GENERIC_REGISTER     16
#define ACPI_RESOURCE_TYPE_GPIO                 17  /* ACPI 5.0 */
#define ACPI_RESOURCE_TYPE_FIXED_DMA            18  /* ACPI 5.0 */
#define ACPI_RESOURCE_TYPE_SERIAL_BUS           19  /* ACPI 5.0 */
#define ACPI_RESOURCE_TYPE_MAX                  19

/* Master union for resource descriptors */

typedef union acpi_resource_data
{
    ACPI_RESOURCE_IRQ                       Irq;
    ACPI_RESOURCE_DMA                       Dma;
    ACPI_RESOURCE_START_DEPENDENT           StartDpf;
    ACPI_RESOURCE_IO                        Io;
    ACPI_RESOURCE_FIXED_IO                  FixedIo;
    ACPI_RESOURCE_FIXED_DMA                 FixedDma;
    ACPI_RESOURCE_VENDOR                    Vendor;
    ACPI_RESOURCE_VENDOR_TYPED              VendorTyped;
    ACPI_RESOURCE_END_TAG                   EndTag;
    ACPI_RESOURCE_MEMORY24                  Memory24;
    ACPI_RESOURCE_MEMORY32                  Memory32;
    ACPI_RESOURCE_FIXED_MEMORY32            FixedMemory32;
    ACPI_RESOURCE_ADDRESS16                 Address16;
    ACPI_RESOURCE_ADDRESS32                 Address32;
    ACPI_RESOURCE_ADDRESS64                 Address64;
    ACPI_RESOURCE_EXTENDED_ADDRESS64        ExtAddress64;
    ACPI_RESOURCE_EXTENDED_IRQ              ExtendedIrq;
    ACPI_RESOURCE_GENERIC_REGISTER          GenericReg;
    ACPI_RESOURCE_GPIO                      Gpio;
    ACPI_RESOURCE_I2C_SERIALBUS             I2cSerialBus;
    ACPI_RESOURCE_SPI_SERIALBUS             SpiSerialBus;
    ACPI_RESOURCE_UART_SERIALBUS            UartSerialBus;
    ACPI_RESOURCE_COMMON_SERIALBUS          CommonSerialBus;

    /* Common fields */

    ACPI_RESOURCE_ADDRESS                   Address;        /* Common 16/32/64 address fields */

} ACPI_RESOURCE_DATA;


/* Common resource header */

typedef struct acpi_resource
{
    UINT32                          Type;
    UINT32                          Length;
    ACPI_RESOURCE_DATA              Data;

} ACPI_RESOURCE;

/* restore default alignment */

#pragma pack()


#define ACPI_RS_SIZE_NO_DATA                8       /* Id + Length fields */
#define ACPI_RS_SIZE_MIN                    (UINT32) ACPI_ROUND_UP_TO_NATIVE_WORD (12)
#define ACPI_RS_SIZE(Type)                  (UINT32) (ACPI_RS_SIZE_NO_DATA + sizeof (Type))

/* Macro for walking resource templates with multiple descriptors */

#define ACPI_NEXT_RESOURCE(Res) \
    ACPI_ADD_PTR (ACPI_RESOURCE, (Res), (Res)->Length)


typedef struct acpi_pci_routing_table
{
    UINT32                          Length;
    UINT32                          Pin;
    UINT64                          Address;        /* here for 64-bit alignment */
    UINT32                          SourceIndex;
    char                            Source[4];      /* pad to 64 bits so sizeof() works in all cases */

} ACPI_PCI_ROUTING_TABLE;

#endif /* __ACRESTYP_H__ */
