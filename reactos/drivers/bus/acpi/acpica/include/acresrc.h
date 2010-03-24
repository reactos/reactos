/******************************************************************************
 *
 * Name: acresrc.h - Resource Manager function prototypes
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,

 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

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

#define ACPI_RSC_INITGET                0
#define ACPI_RSC_INITSET                1
#define ACPI_RSC_FLAGINIT               2
#define ACPI_RSC_1BITFLAG               3
#define ACPI_RSC_2BITFLAG               4
#define ACPI_RSC_COUNT                  5
#define ACPI_RSC_COUNT16                6
#define ACPI_RSC_LENGTH                 7
#define ACPI_RSC_MOVE8                  8
#define ACPI_RSC_MOVE16                 9
#define ACPI_RSC_MOVE32                 10
#define ACPI_RSC_MOVE64                 11
#define ACPI_RSC_SET8                   12
#define ACPI_RSC_DATA8                  13
#define ACPI_RSC_ADDRESS                14
#define ACPI_RSC_SOURCE                 15
#define ACPI_RSC_SOURCEX                16
#define ACPI_RSC_BITMASK                17
#define ACPI_RSC_BITMASK16              18
#define ACPI_RSC_EXIT_NE                19
#define ACPI_RSC_EXIT_LE                20
#define ACPI_RSC_EXIT_EQ                21

/* Resource Conversion sub-opcodes */

#define ACPI_RSC_COMPARE_AML_LENGTH     0
#define ACPI_RSC_COMPARE_VALUE          1

#define ACPI_RSC_TABLE_SIZE(d)          (sizeof (d) / sizeof (ACPI_RSCONVERT_INFO))

#define ACPI_RS_OFFSET(f)               (UINT8) ACPI_OFFSET (ACPI_RESOURCE,f)
#define AML_OFFSET(f)                   (UINT8) ACPI_OFFSET (AML_RESOURCE,f)


typedef const struct acpi_rsdump_info
{
    UINT8                   Opcode;
    UINT8                   Offset;
    char                    *Name;
    const char              **Pointer;

} ACPI_RSDUMP_INFO;

/* Values for the Opcode field above */

#define ACPI_RSD_TITLE                  0
#define ACPI_RSD_LITERAL                1
#define ACPI_RSD_STRING                 2
#define ACPI_RSD_UINT8                  3
#define ACPI_RSD_UINT16                 4
#define ACPI_RSD_UINT32                 5
#define ACPI_RSD_UINT64                 6
#define ACPI_RSD_1BITFLAG               7
#define ACPI_RSD_2BITFLAG               8
#define ACPI_RSD_SHORTLIST              9
#define ACPI_RSD_LONGLIST               10
#define ACPI_RSD_DWORDLIST              11
#define ACPI_RSD_ADDRESS                12
#define ACPI_RSD_SOURCE                 13

/* restore default alignment */

#pragma pack()


/* Resource tables indexed by internal resource type */

extern const UINT8              AcpiGbl_AmlResourceSizes[];
extern ACPI_RSCONVERT_INFO      *AcpiGbl_SetResourceDispatch[];

/* Resource tables indexed by raw AML resource descriptor type */

extern const UINT8              AcpiGbl_ResourceStructSizes[];
extern ACPI_RSCONVERT_INFO      *AcpiGbl_GetResourceDispatch[];


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
    ACPI_RESOURCE           *LinkedListBuffer,
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
    char                    *Path,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiRsSetSrsMethodData (
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
    ACPI_RESOURCE           *LinkedListBuffer,
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
    void                    *Context);

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
 * rsdump
 */
void
AcpiRsDumpResourceList (
    ACPI_RESOURCE           *Resource);

void
AcpiRsDumpIrqList (
    UINT8                   *RouteTable);


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

/*
 * rsdump
 */
extern ACPI_RSDUMP_INFO         AcpiRsDumpIrq[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpDma[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpStartDpf[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpEndDpf[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpIo[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpFixedIo[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpVendor[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpEndTag[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpMemory24[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpMemory32[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpFixedMemory32[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpAddress16[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpAddress32[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpAddress64[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpExtAddress64[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpExtIrq[];
extern ACPI_RSDUMP_INFO         AcpiRsDumpGenericReg[];
#endif

#endif  /* __ACRESRC_H__ */
