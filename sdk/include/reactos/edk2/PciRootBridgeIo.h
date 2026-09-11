/** @file
  PCI Root Bridge I/O protocol as defined in the UEFI 2.0 specification.

  PCI Root Bridge I/O protocol is used by PCI Bus Driver to perform PCI Memory, PCI I/O,
  and PCI Configuration cycles on a PCI Root Bridge. It also provides services to perform
  different types of bus mastering DMA.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PCI_ROOT_BRIDGE_IO_H__
#define __PCI_ROOT_BRIDGE_IO_H__

#define EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GUID \
  { \
    0x2f707ebb, 0x4a1a, 0x11d4, {0x9a, 0x38, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

typedef struct _EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL;

///
/// ACPI address space descriptors, returned by
/// EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.Configuration().
/// (Normally defined in IndustryStandard/Acpi10.h in EDK2.)
///
#pragma pack(1)
typedef struct {
  UINT8   Desc;
  UINT16  Len;
  UINT8   ResType;
  UINT8   GenFlag;
  UINT8   SpecificFlag;
  UINT64  AddrSpaceGranularity;
  UINT64  AddrRangeMin;
  UINT64  AddrRangeMax;
  UINT64  AddrTranslationOffset;
  UINT64  AddrLen;
} EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR;

typedef struct {
  UINT8 Desc;
  UINT8 Checksum;
} EFI_ACPI_END_TAG_DESCRIPTOR;
#pragma pack()

#define ACPI_ADDRESS_SPACE_DESCRIPTOR   0x8A
#define ACPI_END_TAG_DESCRIPTOR         0x79

#define ACPI_ADDRESS_SPACE_TYPE_MEM     0x00
#define ACPI_ADDRESS_SPACE_TYPE_IO      0x01
#define ACPI_ADDRESS_SPACE_TYPE_BUS     0x02

typedef enum {
  EfiPciWidthUint8,
  EfiPciWidthUint16,
  EfiPciWidthUint32,
  EfiPciWidthUint64,
  EfiPciWidthFifoUint8,
  EfiPciWidthFifoUint16,
  EfiPciWidthFifoUint32,
  EfiPciWidthFifoUint64,
  EfiPciWidthFillUint8,
  EfiPciWidthFillUint16,
  EfiPciWidthFillUint32,
  EfiPciWidthFillUint64,
  EfiPciWidthMaximum
} EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH;

typedef enum {
  ///
  /// A read operation from system memory by a bus master that is not capable of producing
  /// PCI dual address cycles.
  ///
  EfiPciOperationBusMasterRead,
  ///
  /// A write operation from system memory by a bus master that is not capable of producing
  /// PCI dual address cycles.
  ///
  EfiPciOperationBusMasterWrite,
  ///
  /// Provides both read and write access to system memory by both the processor and a bus
  /// master that is not capable of producing PCI dual address cycles.
  ///
  EfiPciOperationBusMasterCommonBuffer,
  ///
  /// A read operation from system memory by a bus master that is capable of producing PCI
  /// dual address cycles.
  ///
  EfiPciOperationBusMasterRead64,
  ///
  /// A write operation to system memory by a bus master that is capable of producing PCI
  /// dual address cycles.
  ///
  EfiPciOperationBusMasterWrite64,
  ///
  /// Provides both read and write access to system memory by both the processor and a bus
  /// master that is capable of producing PCI dual address cycles.
  ///
  EfiPciOperationBusMasterCommonBuffer64,
  EfiPciOperationMaximum
} EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION;

#define EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO    0x0001
#define EFI_PCI_ATTRIBUTE_ISA_IO                0x0002
#define EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO        0x0004
#define EFI_PCI_ATTRIBUTE_VGA_MEMORY            0x0008
#define EFI_PCI_ATTRIBUTE_VGA_IO                0x0010
#define EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO        0x0020
#define EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO      0x0040
#define EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE  0x0080
#define EFI_PCI_ATTRIBUTE_MEMORY_CACHED         0x0800
#define EFI_PCI_ATTRIBUTE_MEMORY_DISABLE        0x1000
#define EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE    0x8000
#define EFI_PCI_ATTRIBUTE_ISA_IO_16             0x10000
#define EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16     0x20000
#define EFI_PCI_ATTRIBUTE_VGA_IO_16             0x40000

/**
  Reads from the I/O space of a PCI Root Bridge. Returns when either the polling exit criteria is
  satisfied or after a defined duration.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  Address               The base address of the memory or I/O operations.
  @param  Mask                  Mask used for the polling criteria.
  @param  Value                 The comparison value used for the polling exit criteria.
  @param  Delay                 The number of 100 ns units to poll.
  @param  Result                Pointer to the last value read from the memory location.

  @retval EFI_SUCCESS           The last data returned from the access matched the poll exit criteria.
  @retval EFI_TIMEOUT           Delay expired before a match occurred.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_POLL_IO_MEM)(
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL         *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  );

/**
  Enables a PCI driver to access PCI controller registers in the PCI root bridge memory space.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Width                 Signifies the width of the memory operations.
  @param  Address               The base address of the memory operations.
  @param  Count                 The number of memory operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For
                                write operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI root bridge.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_IO_MEM)(
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  );

typedef struct {
  ///
  /// Read PCI controller registers in the PCI root bridge memory space.
  ///
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_IO_MEM  Read;
  ///
  /// Write PCI controller registers in the PCI root bridge memory space.
  ///
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_IO_MEM  Write;
} EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_ACCESS;

/**
  Enables a PCI driver to copy one region of PCI root bridge memory space to another region of PCI
  root bridge memory space.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  DestAddress           The destination address of the memory operation.
  @param  SrcAddress            The source address of the memory operation.
  @param  Count                 The number of memory operations to perform.

  @retval EFI_SUCCESS           The data was copied from one memory region to another memory region.
  @retval EFI_INVALID_PARAMETER Width is invalid for this PCI root bridge.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_COPY_MEM)(
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL         *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH   Width,
  IN UINT64                                  DestAddress,
  IN UINT64                                  SrcAddress,
  IN UINTN                                   Count
  );

/**
  Provides the PCI controller-specific addresses required to access system memory from a
  DMA bus master.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_MAP)(
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL            *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_UNMAP)(
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN VOID                             *Mapping
  );

/**
  Allocates pages that are suitable for an EfiPciOperationBusMasterCommonBuffer or
  EfiPciOperationBusMasterCommonBuffer64 mapping.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Type                  This parameter is not used and must be ignored.
  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  Attributes            The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_ALLOCATE_BUFFER)(
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE                Type,
  IN  EFI_MEMORY_TYPE                  MemoryType,
  IN  UINTN                            Pages,
  OUT VOID                             **HostAddress,
  IN  UINT64                           Attributes
  );

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_FREE_BUFFER)(
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN UINTN                            Pages,
  IN VOID                             *HostAddress
  );

/**
  Flushes all PCI posted write transactions from a PCI host bridge to system memory.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.

  @retval EFI_SUCCESS           The PCI posted write transactions were flushed from the PCI host
                                bridge to system memory.
  @retval EFI_DEVICE_ERROR      The PCI posted write transactions were not flushed from the PCI
                                host bridge due to a hardware error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_FLUSH)(
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This
  );

/**
  Gets the attributes that a PCI root bridge supports setting with SetAttributes(), and the
  attributes that a PCI root bridge is currently using.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Supports              A pointer to the mask of attributes that this PCI root bridge supports
                                setting with SetAttributes().
  @param  Attributes            A pointer to the mask of attributes that this PCI root bridge is currently
                                using.

  @retval EFI_SUCCESS           If Supports is not NULL, then the attributes that the PCI root
                                bridge supports is returned in Supports. If Attributes is
                                not NULL, then the attributes that the PCI root bridge is currently
                                using is returned in Attributes.
  @retval EFI_INVALID_PARAMETER Both Supports and Attributes are NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GET_ATTRIBUTES)(
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT UINT64                           *Supports,
  OUT UINT64                           *Attributes
  );

/**
  Sets attributes for a resource range on a PCI root bridge.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Attributes            The mask of attributes to set.
  @param  ResourceBase          A pointer to the base address of the resource range to be modified by the
                                attributes specified by Attributes.
  @param  ResourceLength        A pointer to the length of the resource range to be modified by the
                                attributes specified by Attributes.

  @retval EFI_SUCCESS           The current configuration of this PCI root bridge was returned in
                                Resources.
  @retval EFI_UNSUPPORTED       The current configuration of this PCI root bridge could not be
                                retrieved.
  @retval EFI_INVALID_PARAMETER Invalid pointer of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_SET_ATTRIBUTES)(
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN     UINT64                           Attributes,
  IN OUT UINT64                           *ResourceBase,
  IN OUT UINT64                           *ResourceLength
  );

/**
  Retrieves the current resource settings of this PCI root bridge in the form of a set of ACPI
  resource descriptors.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Resources             A pointer to the resource descriptors that describe the current
                                configuration of this PCI root bridge.

  @retval EFI_SUCCESS           The current configuration of this PCI root bridge was returned in
                                Resources.
  @retval EFI_UNSUPPORTED       The current configuration of this PCI root bridge could not be
                                retrieved.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_CONFIGURATION)(
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT VOID                             **Resources
  );

///
/// Provides the basic Memory, I/O, PCI configuration, and DMA interfaces that are
/// used to abstract accesses to PCI controllers behind a PCI Root Bridge Controller.
///
struct _EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL {
  ///
  /// The EFI_HANDLE of the PCI Host Bridge of which this PCI Root Bridge is a member.
  ///
  EFI_HANDLE                                       ParentHandle;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_POLL_IO_MEM      PollMem;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_POLL_IO_MEM      PollIo;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_ACCESS           Mem;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_ACCESS           Io;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_ACCESS           Pci;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_COPY_MEM         CopyMem;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_MAP              Map;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_UNMAP            Unmap;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_ALLOCATE_BUFFER  AllocateBuffer;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_FREE_BUFFER      FreeBuffer;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_FLUSH            Flush;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GET_ATTRIBUTES   GetAttributes;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_SET_ATTRIBUTES   SetAttributes;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_CONFIGURATION    Configuration;
  ///
  /// The segment number that this PCI root bridge resides.
  ///
  UINT32                                           SegmentNumber;
};

extern EFI_GUID gEfiPciRootBridgeIoProtocolGuid;

#endif
