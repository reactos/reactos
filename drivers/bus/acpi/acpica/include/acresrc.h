/******************************************************************************
 *
 * Name: acresrc.h - Resource Manager function prototypes
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __ACRESRC_H__
#define __ACRESRC_H__

/* Need the AML resource descriptor structs */

#include "amlresrc.h"


/*
 * If possible, pack the following structures to byte alignment, since we
 * don't care about performance for debug output. Two cases where we cannot
 * pack the structures:
 *
 * 1) Hardware does not support misaligned memory transfers
 * 2) Compiler does not support pointers within packed structures
 */
#if (!defined(ACPI_MISALIGNMENT_NOT_SUPPORTED) && !defined(ACPI_PACKED_POINTERS_NOT_SUPPORTED))
#pragma pack(1)
#endif

/*
 * Individual entry for the resource conversion tables
 */
typedef const struct acpi_rsconvert_info
{
    UINT8                   Opcode;
    UINT8                   ResourceOffset;
    UINT8                   AmlOffset;
    UINT8                   Value;

} ACPI_RSCONVERT_INFO;

/* Resource conversion opcodes */

typedef enum
{
    ACPI_RSC_INITGET        = 0,
    ACPI_RSC_INITSET,
    ACPI_RSC_FLAGINIT,
    ACPI_RSC_1BITFLAG,
    ACPI_RSC_2BITFLAG,
    ACPI_RSC_3BITFLAG,
    ACPI_RSC_6BITFLAG,
    ACPI_RSC_ADDRESS,
    ACPI_RSC_BITMASK,
    ACPI_RSC_BITMASK16,
    ACPI_RSC_COUNT,
    ACPI_RSC_COUNT16,
    ACPI_RSC_COUNT_GPIO_PIN,
    ACPI_RSC_COUNT_GPIO_RES,
    ACPI_RSC_COUNT_GPIO_VEN,
    ACPI_RSC_COUNT_SERIAL_RES,
    ACPI_RSC_COUNT_SERIAL_VEN,
    ACPI_RSC_DATA8,
    ACPI_RSC_EXIT_EQ,
    ACPI_RSC_EXIT_LE,
    ACPI_RSC_EXIT_NE,
    ACPI_RSC_LENGTH,
    ACPI_RSC_MOVE_GPIO_PIN,
    ACPI_RSC_MOVE_GPIO_RES,
    ACPI_RSC_MOVE_SERIAL_RES,
    ACPI_RSC_MOVE_SERIAL_VEN,
    ACPI_RSC_MOVE8,
    ACPI_RSC_MOVE16,
    ACPI_RSC_MOVE32,
    ACPI_RSC_MOVE64,
    ACPI_RSC_SET8,
    ACPI_RSC_SOURCE,
    ACPI_RSC_SOURCEX

} ACPI_RSCONVERT_OPCODES;

/* Resource Conversion sub-opcodes */

#define ACPI_RSC_COMPARE_AML_LENGTH     0
#define ACPI_RSC_COMPARE_VALUE          1

#define ACPI_RSC_TABLE_SIZE(d)          (sizeof (d) / sizeof (ACPI_RSCONVERT_INFO))

#define ACPI_RS_OFFSET(f)               (UINT8) ACPI_OFFSET (ACPI_RESOURCE,f)
#define AML_OFFSET(f)                   (UINT8) ACPI_OFFSET (AML_RESOURCE,f)


/*
 * Individual entry for the resource dump tables
 */
typedef const struct acpi_rsdump_info
{
    UINT8                   Opcode;
    UINT8                   Offset;
    const char              *Name;
    const char              **Pointer;

} ACPI_RSDUMP_INFO;

/* Values for the Opcode field above */

typedef enum
{
    ACPI_RSD_TITLE          = 0,
    ACPI_RSD_1BITFLAG,
    ACPI_RSD_2BITFLAG,
    ACPI_RSD_3BITFLAG,
    ACPI_RSD_6BITFLAG,
    ACPI_RSD_ADDRESS,
    ACPI_RSD_DWORDLIST,
    ACPI_RSD_LITERAL,
    ACPI_RSD_LONGLIST,
    ACPI_RSD_SHORTLIST,
    ACPI_RSD_SHORTLISTX,
    ACPI_RSD_SOURCE,
    ACPI_RSD_STRING,
    ACPI_RSD_UINT8,
    ACPI_RSD_UINT16,
    ACPI_RSD_UINT32,
    ACPI_RSD_UINT64,
    ACPI_RSD_WORDLIST,
    ACPI_RSD_LABEL,
    ACPI_RSD_SOURCE_LABEL,

} ACPI_RSDUMP_OPCODES;

/* restore default alignment */

#pragma pack()


/* Resource tables indexed by internal resource type */

extern const UINT8              AcpiGbl_AmlResourceSizes[];
extern const UINT8              AcpiGbl_AmlResourceSerialBusSizes[];
extern ACPI_RSCONVERT_INFO      *AcpiGbl_SetResourceDispatch[];

/* Resource tables indexed by raw AML resource descriptor type */

extern const UINT8              AcpiGbl_ResourceStructSizes[];
extern const UINT8              AcpiGbl_ResourceStructSerialBusSizes[];
extern ACPI_RSCONVERT_INFO      *AcpiGbl_GetResourceDispatch[];

extern ACPI_RSCONVERT_INFO      *AcpiGbl_ConvertResourceSerialBusDispatch[];

typedef struct acpi_vendor_walk_info
{
    ACPI_VENDOR_UUID        *Uuid;
    ACPI_BUFFER             *Buffer;
    ACPI_STATUS             Status;

} ACPI_VENDOR_WALK_INFO;


/*
 * rscreate
 */
ACPI_STATUS
AcpiRsCreateResourceList (
    ACPI_OPERAND_OBJECT     *AmlBuffer,
    ACPI_BUFFER             *OutputBuffer);

ACPI_STATUS
AcpiRsCreateAmlResources (
    ACPI_BUFFER             *ResourceList,
    ACPI_BUFFER             *OutputBuffer);

ACPI_STATUS
AcpiRsCreatePciRoutingTable (
    ACPI_OPERAND_OBJECT     *PackageObject,
    ACPI_BUFFER             *OutputBuffer);


/*
 * rsutils
 */
ACPI_STATUS
AcpiRsGetPrtMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiRsGetCrsMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiRsGetPrsMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiRsGetMethodData (
    ACPI_HANDLE             Handle,
    const char              *Path,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiRsSetSrsMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiRsGetAeiMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *RetBuffer);

/*
 * rscalc
 */
ACPI_STATUS
AcpiRsGetListLength (
    UINT8                   *AmlBuffer,
    UINT32                  AmlBufferLength,
    ACPI_SIZE               *SizeNeeded);

ACPI_STATUS
AcpiRsGetAmlLength (
    ACPI_RESOURCE           *ResourceList,
    ACPI_SIZE               ResourceListSize,
    ACPI_SIZE               *SizeNeeded);

ACPI_STATUS
AcpiRsGetPciRoutingTableLength (
    ACPI_OPERAND_OBJECT     *PackageObject,
    ACPI_SIZE               *BufferSizeNeeded);

ACPI_STATUS
AcpiRsConvertAmlToResources (
    UINT8                   *Aml,
    UINT32                  Length,
    UINT32                  Offset,
    UINT8                   ResourceIndex,
    void                    **Context);

ACPI_STATUS
AcpiRsConvertResourcesToAml (
    ACPI_RESOURCE           *Resource,
    ACPI_SIZE               AmlSizeNeeded,
    UINT8                   *OutputBuffer);


/*
 * rsaddr
 */
void
AcpiRsSetAddressCommon (
    AML_RESOURCE            *Aml,
    ACPI_RESOURCE           *Resource);

BOOLEAN
AcpiRsGetAddressCommon (
    ACPI_RESOURCE           *Resource,
    AML_RESOURCE            *Aml);


/*
 * rsmisc
 */
ACPI_STATUS
AcpiRsConvertAmlToResource (
    ACPI_RESOURCE           *Resource,
    AML_RESOURCE            *Aml,
    ACPI_RSCONVERT_INFO     *Info);

ACPI_STATUS
AcpiRsConvertResourceToAml (
    ACPI_RESOURCE           *Resource,
    AML_RESOURCE            *Aml,
    ACPI_RSCONVERT_INFO     *Info);


/*
 * rsutils
 */
void
AcpiRsMoveData (
    void                    *Destination,
    void                    *Source,
    UINT16                  ItemCount,
    UINT8                   MoveType);

UINT8
AcpiRsDecodeBitmask (
    UINT16                  Mask,
    UINT8                   *List);

UINT16
AcpiRsEncodeBitmask (
    UINT8                   *List,
    UINT8                   Count);

ACPI_RS_LENGTH
AcpiRsGetResourceSource (
    ACPI_RS_LENGTH          ResourceLength,
    ACPI_RS_LENGTH          MinimumLength,
    ACPI_RESOURCE_SOURCE    *ResourceSource,
    AML_RESOURCE            *Aml,
    char                    *StringPtr);

ACPI_RSDESC_SIZE
AcpiRsSetResourceSource (
    AML_RESOURCE            *Aml,
    ACPI_RS_LENGTH          MinimumLength,
    ACPI_RESOURCE_SOURCE    *ResourceSource);

void
AcpiRsSetResourceHeader (
    UINT8                   DescriptorType,
    ACPI_RSDESC_SIZE        TotalLength,
    AML_RESOURCE            *Aml);

void
AcpiRsSetResourceLength (
    ACPI_RSDESC_SIZE        TotalLength,
    AML_RESOURCE            *Aml);


/*
 * rsdump - Debugger support
 */
#ifdef ACPI_DEBUGGER
void
AcpiRsDumpResourceList (
    ACPI_RESOURCE           *Resource);

void
AcpiRsDumpIrqList (
    UINT8                   *RouteTable);
#endif


/*
 * Resource conversion tables
 */
extern ACPI_RSCONVERT_INFO      AcpiRsConvertDma[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertEndDpf[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertIo[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertFixedIo[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertEndTag[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertMemory24[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertGenericReg[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertMemory32[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertFixedMemory32[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertAddress32[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertAddress16[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertExtIrq[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertAddress64[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertExtAddress64[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertGpio[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertFixedDma[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertCsi2SerialBus[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertI2cSerialBus[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertSpiSerialBus[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertUartSerialBus[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertPinFunction[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertPinConfig[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertPinGroup[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertPinGroupFunction[];
extern ACPI_RSCONVERT_INFO      AcpiRsConvertPinGroupConfig[];

/* These resources require separate get/set tables */

extern ACPI_RSCONVERT_INFO      AcpiRsGetIrq[];
extern ACPI_RSCONVERT_INFO      AcpiRsGetStartDpf[];
extern ACPI_RSCONVERT_INFO      AcpiRsGetVendorSmall[];
extern ACPI_RSCONVERT_INFO      AcpiRsGetVendorLarge[];

extern ACPI_RSCONVERT_INFO      AcpiRsSetIrq[];
extern ACPI_RSCONVERT_INFO      AcpiRsSetStartDpf[];
extern ACPI_RSCONVERT_INFO      AcpiRsSetVendor[];


#if defined(ACPI_DEBUG_OUTPUT) || defined(ACPI_DEBUGGER)
/*
 * rsinfo
 */
extern ACPI_RSDUMP_INFO         *AcpiGbl_DumpResourceDispatch[];
extern ACPI_RSDUMP_INFO         *AcpiGbl_DumpSerialBusDispatch[];

/*
 * rsdumpinfo
 */
extern ACPI_RSDUMP_INFO         AcpiRsDumpIrq[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpPrt[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpDma[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpStartDpf[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpEndDpf[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpIo[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpIoFlags[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpFixedIo[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpVendor[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpEndTag[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpMemory24[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpMemory32[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpMemoryFlags[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpFixedMemory32[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpAddress16[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpAddress32[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpAddress64[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpExtAddress64[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpExtIrq[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpGenericReg[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpGpio[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpPinFunction[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpFixedDma[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpCommonSerialBus[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpCsi2SerialBus[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpI2cSerialBus[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpSpiSerialBus[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpUartSerialBus[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpGeneralFlags[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpPinConfig[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpPinGroup[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpPinGroupFunction[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpPinGroupConfig[];
#endif

#endif  /* __ACRESRC_H__ */
