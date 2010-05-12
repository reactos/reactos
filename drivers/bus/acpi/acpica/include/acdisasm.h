/******************************************************************************
 *
 * Name: acdisasm.h - AML disassembler
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

#ifndef __ACDISASM_H__
#define __ACDISASM_H__

#include "amlresrc.h"


#define BLOCK_NONE              0
#define BLOCK_PAREN             1
#define BLOCK_BRACE             2
#define BLOCK_COMMA_LIST        4
#define ACPI_DEFAULT_RESNAME    *(UINT32 *) "__RD"


typedef const struct acpi_dmtable_info
{
    UINT8                       Opcode;
    UINT8                       Offset;
    char                        *Name;

} ACPI_DMTABLE_INFO;

/*
 * Values for Opcode above.
 * Note: 0-7 must not change, used as a flag shift value
 */
#define ACPI_DMT_FLAG0                  0
#define ACPI_DMT_FLAG1                  1
#define ACPI_DMT_FLAG2                  2
#define ACPI_DMT_FLAG3                  3
#define ACPI_DMT_FLAG4                  4
#define ACPI_DMT_FLAG5                  5
#define ACPI_DMT_FLAG6                  6
#define ACPI_DMT_FLAG7                  7
#define ACPI_DMT_FLAGS0                 8
#define ACPI_DMT_FLAGS2                 9
#define ACPI_DMT_UINT8                  10
#define ACPI_DMT_UINT16                 11
#define ACPI_DMT_UINT24                 12
#define ACPI_DMT_UINT32                 13
#define ACPI_DMT_UINT56                 14
#define ACPI_DMT_UINT64                 15
#define ACPI_DMT_STRING                 16
#define ACPI_DMT_NAME4                  17
#define ACPI_DMT_NAME6                  18
#define ACPI_DMT_NAME8                  19
#define ACPI_DMT_CHKSUM                 20
#define ACPI_DMT_SPACEID                21
#define ACPI_DMT_GAS                    22
#define ACPI_DMT_ASF                    23
#define ACPI_DMT_DMAR                   24
#define ACPI_DMT_HEST                   25
#define ACPI_DMT_HESTNTFY               26
#define ACPI_DMT_HESTNTYP               27
#define ACPI_DMT_MADT                   28
#define ACPI_DMT_SRAT                   29
#define ACPI_DMT_EXIT                   30
#define ACPI_DMT_SIG                    31
#define ACPI_DMT_FADTPM                 32
#define ACPI_DMT_BUF16                  33
#define ACPI_DMT_IVRS                   34


typedef
void (*ACPI_DMTABLE_HANDLER) (
    ACPI_TABLE_HEADER       *Table);

typedef struct acpi_dmtable_data
{
    char                    *Signature;
    ACPI_DMTABLE_INFO       *TableInfo;
    ACPI_DMTABLE_HANDLER    TableHandler;
    char                    *Name;

} ACPI_DMTABLE_DATA;


typedef struct acpi_op_walk_info
{
    UINT32                  Level;
    UINT32                  LastLevel;
    UINT32                  Count;
    UINT32                  BitOffset;
    UINT32                  Flags;
    ACPI_WALK_STATE         *WalkState;

} ACPI_OP_WALK_INFO;

typedef
ACPI_STATUS (*ASL_WALK_CALLBACK) (
    ACPI_PARSE_OBJECT           *Op,
    UINT32                      Level,
    void                        *Context);

typedef struct acpi_resource_tag
{
    UINT32                  BitIndex;
    char                    *Tag;

} ACPI_RESOURCE_TAG;

/* Strings used for decoding flags to ASL keywords */

extern const char               *AcpiGbl_WordDecode[];
extern const char               *AcpiGbl_IrqDecode[];
extern const char               *AcpiGbl_LockRule[];
extern const char               *AcpiGbl_AccessTypes[];
extern const char               *AcpiGbl_UpdateRules[];
extern const char               *AcpiGbl_MatchOps[];

extern ACPI_DMTABLE_INFO        AcpiDmTableInfoAsf0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoAsf1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoAsf1a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoAsf2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoAsf2a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoAsf3[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoAsf4[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoAsfHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoBoot[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoBert[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoCpep[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoCpep0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDbgp[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmarHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmarScope[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar3[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoEcdt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoEinj[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoEinj0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoErst[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFacs[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFadt1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFadt2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFadt3[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoGas[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHeader[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest6[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest7[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest8[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest9[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHestNotify[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHestBank[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHpet[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIvrs[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIvrs0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIvrs1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIvrs4[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIvrs8a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIvrs8b[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIvrs8c[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIvrsHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt3[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt4[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt5[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt6[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt7[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt8[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt9[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt10[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadtHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMcfg[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMcfg0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMsct[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMsct0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoRsdp1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoRsdp2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSbst[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSlic[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSlit[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSpcr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSpmi[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSrat[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSratHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSrat0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSrat1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSrat2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoTcpa[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoUefi[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWaet[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWdat[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWdat0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWdrt[];


/*
 * dmtable
 */
void
AcpiDmDumpDataTable (
    ACPI_TABLE_HEADER       *Table);

ACPI_STATUS
AcpiDmDumpTable (
    UINT32                  TableLength,
    UINT32                  TableOffset,
    void                    *Table,
    UINT32                  SubTableLength,
    ACPI_DMTABLE_INFO        *Info);

void
AcpiDmLineHeader (
    UINT32                  Offset,
    UINT32                  ByteLength,
    char                    *Name);

void
AcpiDmLineHeader2 (
    UINT32                  Offset,
    UINT32                  ByteLength,
    char                    *Name,
    UINT32                  Value);


/*
 * dmtbdump
 */
void
AcpiDmDumpAsf (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpCpep (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpDmar (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpEinj (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpErst (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpFadt (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpHest (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpIvrs (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpMcfg (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpMadt (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpMsct (
    ACPI_TABLE_HEADER       *Table);

UINT32
AcpiDmDumpRsdp (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpRsdt (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpSlit (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpSrat (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpWdat (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpXsdt (
    ACPI_TABLE_HEADER       *Table);


/*
 * dmwalk
 */
void
AcpiDmDisassemble (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Origin,
    UINT32                  NumOpcodes);

void
AcpiDmWalkParseTree (
    ACPI_PARSE_OBJECT       *Op,
    ASL_WALK_CALLBACK       DescendingCallback,
    ASL_WALK_CALLBACK       AscendingCallback,
    void                    *Context);


/*
 * dmopcode
 */
void
AcpiDmDisassembleOneOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OP_WALK_INFO       *Info,
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmDecodeInternalObject (
    ACPI_OPERAND_OBJECT     *ObjDesc);

UINT32
AcpiDmListType (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmMethodFlags (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmFieldFlags (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmAddressSpace (
    UINT8                   SpaceId);

void
AcpiDmRegionFlags (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmMatchOp (
    ACPI_PARSE_OBJECT       *Op);


/*
 * dmnames
 */
UINT32
AcpiDmDumpName (
    UINT32                  Name);

ACPI_STATUS
AcpiPsDisplayObjectPathname (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmNamestring (
    char                    *Name);


/*
 * dmobject
 */
void
AcpiDmDisplayInternalObject (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState);

void
AcpiDmDisplayArguments (
    ACPI_WALK_STATE         *WalkState);

void
AcpiDmDisplayLocals (
    ACPI_WALK_STATE         *WalkState);

void
AcpiDmDumpMethodInfo (
    ACPI_STATUS             Status,
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op);


/*
 * dmbuffer
 */
void
AcpiDmDisasmByteList (
    UINT32                  Level,
    UINT8                   *ByteData,
    UINT32                  ByteCount);

void
AcpiDmByteList (
    ACPI_OP_WALK_INFO       *Info,
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmIsEisaId (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmEisaId (
    UINT32                  EncodedId);

BOOLEAN
AcpiDmIsUnicodeBuffer (
    ACPI_PARSE_OBJECT       *Op);

BOOLEAN
AcpiDmIsStringBuffer (
    ACPI_PARSE_OBJECT       *Op);


/*
 * dmextern
 */
void
AcpiDmAddToExternalList (
    ACPI_PARSE_OBJECT       *Op,
    char                    *Path,
    UINT8                   Type,
    UINT32                  Value);

void
AcpiDmAddExternalsToNamespace (
    void);

UINT32
AcpiDmGetExternalMethodCount (
    void);

void
AcpiDmClearExternalList (
    void);

void
AcpiDmEmitExternals (
    void);


/*
 * dmresrc
 */
void
AcpiDmDumpInteger8 (
    UINT8                   Value,
    char                    *Name);

void
AcpiDmDumpInteger16 (
    UINT16                  Value,
    char                    *Name);

void
AcpiDmDumpInteger32 (
    UINT32                  Value,
    char                    *Name);

void
AcpiDmDumpInteger64 (
    UINT64                  Value,
    char                    *Name);

void
AcpiDmResourceTemplate (
    ACPI_OP_WALK_INFO       *Info,
    ACPI_PARSE_OBJECT       *Op,
    UINT8                   *ByteData,
    UINT32                  ByteCount);

ACPI_STATUS
AcpiDmIsResourceTemplate (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmBitList (
    UINT16                  Mask);

void
AcpiDmDescriptorName (
    void);


/*
 * dmresrcl
 */
void
AcpiDmWordDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmDwordDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmExtendedDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmQwordDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmMemory24Descriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmMemory32Descriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmFixedMemory32Descriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmGenericRegisterDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmInterruptDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmVendorLargeDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmVendorCommon (
    char                    *Name,
    UINT8                   *ByteData,
    UINT32                  Length,
    UINT32                  Level);


/*
 * dmresrcs
 */
void
AcpiDmIrqDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmDmaDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmIoDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmFixedIoDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmStartDependentDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmEndDependentDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmVendorSmallDescriptor (
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);


/*
 * dmutils
 */
void
AcpiDmDecodeAttribute (
    UINT8                   Attribute);

void
AcpiDmIndent (
    UINT32                  Level);

BOOLEAN
AcpiDmCommaIfListMember (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmCommaIfFieldMember (
    ACPI_PARSE_OBJECT       *Op);


/*
 * dmrestag
 */
void
AcpiDmFindResources (
    ACPI_PARSE_OBJECT       *Root);

void
AcpiDmCheckResourceReference (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_WALK_STATE         *WalkState);

#endif  /* __ACDISASM_H__ */
