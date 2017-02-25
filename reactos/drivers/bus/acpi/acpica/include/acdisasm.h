/******************************************************************************
 *
 * Name: acdisasm.h - AML disassembler
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
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

#ifndef __ACDISASM_H__
#define __ACDISASM_H__

#include "amlresrc.h"


#define BLOCK_NONE              0
#define BLOCK_PAREN             1
#define BLOCK_BRACE             2
#define BLOCK_COMMA_LIST        4
#define ACPI_DEFAULT_RESNAME    *(UINT32 *) "__RD"

/*
 * Raw table data header. Used by disassembler and data table compiler.
 * Do not change.
 */
#define ACPI_RAW_TABLE_DATA_HEADER      "Raw Table Data"


typedef struct acpi_dmtable_info
{
    UINT8                       Opcode;
    UINT16                      Offset;
    char                        *Name;
    UINT8                       Flags;

} ACPI_DMTABLE_INFO;

/* Values for Flags field above */

#define DT_LENGTH                       0x01    /* Field is a subtable length */
#define DT_FLAG                         0x02    /* Field is a flag value */
#define DT_NON_ZERO                     0x04    /* Field must be non-zero */
#define DT_OPTIONAL                     0x08    /* Field is optional */
#define DT_DESCRIBES_OPTIONAL           0x10    /* Field describes an optional field (length, etc.) */
#define DT_COUNT                        0x20    /* Currently not used */

/*
 * Values for Opcode above.
 * Note: 0-7 must not change, they are used as a flag shift value. Other
 * than those, new values can be added wherever appropriate.
 */
typedef enum
{
    /* Simple Data Types */

    ACPI_DMT_FLAG0          = 0,
    ACPI_DMT_FLAG1          = 1,
    ACPI_DMT_FLAG2          = 2,
    ACPI_DMT_FLAG3          = 3,
    ACPI_DMT_FLAG4          = 4,
    ACPI_DMT_FLAG5          = 5,
    ACPI_DMT_FLAG6          = 6,
    ACPI_DMT_FLAG7          = 7,
    ACPI_DMT_FLAGS0,
    ACPI_DMT_FLAGS1,
    ACPI_DMT_FLAGS2,
    ACPI_DMT_FLAGS4,
    ACPI_DMT_UINT8,
    ACPI_DMT_UINT16,
    ACPI_DMT_UINT24,
    ACPI_DMT_UINT32,
    ACPI_DMT_UINT40,
    ACPI_DMT_UINT48,
    ACPI_DMT_UINT56,
    ACPI_DMT_UINT64,
    ACPI_DMT_BUF7,
    ACPI_DMT_BUF10,
    ACPI_DMT_BUF16,
    ACPI_DMT_BUF128,
    ACPI_DMT_SIG,
    ACPI_DMT_STRING,
    ACPI_DMT_NAME4,
    ACPI_DMT_NAME6,
    ACPI_DMT_NAME8,

    /* Types that are decoded to strings and miscellaneous */

    ACPI_DMT_ACCWIDTH,
    ACPI_DMT_CHKSUM,
    ACPI_DMT_GAS,
    ACPI_DMT_SPACEID,
    ACPI_DMT_UNICODE,
    ACPI_DMT_UUID,

    /* Types used only for the Data Table Compiler */

    ACPI_DMT_BUFFER,
    ACPI_DMT_RAW_BUFFER,  /* Large, multiple line buffer */
    ACPI_DMT_DEVICE_PATH,
    ACPI_DMT_LABEL,
    ACPI_DMT_PCI_PATH,

    /* Types that are specific to particular ACPI tables */

    ACPI_DMT_ASF,
    ACPI_DMT_DMAR,
    ACPI_DMT_DMAR_SCOPE,
    ACPI_DMT_EINJACT,
    ACPI_DMT_EINJINST,
    ACPI_DMT_ERSTACT,
    ACPI_DMT_ERSTINST,
    ACPI_DMT_FADTPM,
    ACPI_DMT_GTDT,
    ACPI_DMT_HEST,
    ACPI_DMT_HESTNTFY,
    ACPI_DMT_HESTNTYP,
    ACPI_DMT_IORTMEM,
    ACPI_DMT_IVRS,
    ACPI_DMT_LPIT,
    ACPI_DMT_MADT,
    ACPI_DMT_NFIT,
    ACPI_DMT_PCCT,
    ACPI_DMT_PMTT,
    ACPI_DMT_SLIC,
    ACPI_DMT_SRAT,

    /* Special opcodes */

    ACPI_DMT_EXTRA_TEXT,
    ACPI_DMT_EXIT

} ACPI_ENTRY_TYPES;

typedef
void (*ACPI_DMTABLE_HANDLER) (
    ACPI_TABLE_HEADER       *Table);

typedef
ACPI_STATUS (*ACPI_CMTABLE_HANDLER) (
    void                    **PFieldList);

typedef struct acpi_dmtable_data
{
    char                    *Signature;
    ACPI_DMTABLE_INFO       *TableInfo;
    ACPI_DMTABLE_HANDLER    TableHandler;
    ACPI_CMTABLE_HANDLER    CmTableHandler;
    const unsigned char     *Template;

} ACPI_DMTABLE_DATA;


typedef struct acpi_op_walk_info
{
    ACPI_WALK_STATE         *WalkState;
    ACPI_PARSE_OBJECT       *MappingOp;
    UINT8                   *PreviousAml;
    UINT8                   *StartAml;
    UINT32                  Level;
    UINT32                  LastLevel;
    UINT32                  Count;
    UINT32                  BitOffset;
    UINT32                  Flags;
    UINT32                  AmlOffset;

} ACPI_OP_WALK_INFO;

/*
 * TBD - another copy of this is in asltypes.h, fix
 */
#ifndef ASL_WALK_CALLBACK_DEFINED
typedef
ACPI_STATUS (*ASL_WALK_CALLBACK) (
    ACPI_PARSE_OBJECT           *Op,
    UINT32                      Level,
    void                        *Context);
#define ASL_WALK_CALLBACK_DEFINED
#endif

typedef
void (*ACPI_RESOURCE_HANDLER) (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

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
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoBgrt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoCpep[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoCpep0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoCsrt0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoCsrt1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoCsrt2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoCsrt2a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDbg2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDbg2Device[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDbg2Addr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDbg2Size[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDbg2Name[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDbg2OemData[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDbgp[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmarHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmarScope[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar3[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDmar4[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDrtm[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDrtm0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDrtm0a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDrtm1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDrtm1a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoDrtm2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoEcdt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoEinj[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoEinj0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoErst[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoErst0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFacs[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFadt1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFadt2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFadt3[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFadt5[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFadt6[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFpdt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFpdtHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFpdt0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoFpdt1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoGas[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoGtdt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoGtdtHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoGtdt0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoGtdt0a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoGtdt1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHeader[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest6[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest7[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest8[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest9[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHest10[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHestNotify[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHestBank[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoHpet[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoLpitHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoLpit0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoLpit1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort0a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort1a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort3[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort3a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort3b[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort3c[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIort4[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIortAcc[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIortHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIortMap[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoIortPad[];
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
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt11[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt12[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt13[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt14[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadt15[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMadtHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMcfg[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMcfg0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMchi[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMpst[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMpst0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMpst0A[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMpst0B[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMpst1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMpst2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMsct[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMsct0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMtmr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoMtmr0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfitHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit2a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit3[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit3a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit4[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit5[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit6[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoNfit6a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPmtt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPmtt0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPmtt1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPmtt1a[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPmtt2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPmttHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPcct[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPcctHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPcct0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPcct1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoPcct2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoRsdp1[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoRsdp2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoS3pt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoS3ptHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoS3pt0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoS3pt1[];
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
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoSrat3[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoStao[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoStaoStr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoTcpaHdr[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoTcpaClient[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoTcpaServer[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoTpm2[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoUefi[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoVrtc[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoVrtc0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWaet[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWdat[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWdat0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWddt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWdrt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWpbt[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoWpbt0[];
extern ACPI_DMTABLE_INFO        AcpiDmTableInfoXenv[];

extern ACPI_DMTABLE_INFO        AcpiDmTableInfoGeneric[][2];

/*
 * dmtable and ahtable
 */
extern const ACPI_DMTABLE_DATA  AcpiDmTableData[];
extern const AH_TABLE           AcpiSupportedTables[];

UINT8
AcpiDmGenerateChecksum (
    void                    *Table,
    UINT32                  Length,
    UINT8                   OriginalChecksum);

const ACPI_DMTABLE_DATA *
AcpiDmGetTableData (
    char                    *Signature);

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
AcpiDmDumpBuffer (
    void                    *Table,
    UINT32                  BufferOffset,
    UINT32                  Length,
    UINT32                  AbsoluteOffset,
    char                    *Header);

void
AcpiDmDumpUnicode (
    void                    *Table,
    UINT32                  BufferOffset,
    UINT32                  ByteLength);

void
AcpiDmDumpAsf (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpCpep (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpCsrt (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpDbg2 (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpDmar (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpDrtm (
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
AcpiDmDumpFpdt (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpGtdt (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpHest (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpIort (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpIvrs (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpLpit (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpMadt (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpMcfg (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpMpst (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpMsct (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpMtmr (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpNfit (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpPcct (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpPmtt (
    ACPI_TABLE_HEADER       *Table);

UINT32
AcpiDmDumpRsdp (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpRsdt (
    ACPI_TABLE_HEADER       *Table);

UINT32
AcpiDmDumpS3pt (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpSlic (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpSlit (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpSrat (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpStao (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpTcpa (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpVrtc (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpWdat (
    ACPI_TABLE_HEADER       *Table);

void
AcpiDmDumpWpbt (
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

UINT32
AcpiDmListType (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmMethodFlags (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmDisplayTargetPathname (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmNotifyDescription (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmPredefinedDescription (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmFieldPredefinedDescription (
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
AcpiDmCheckForHardwareId (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDmDecompressEisaId (
    UINT32                  EncodedId);

BOOLEAN
AcpiDmIsUuidBuffer (
    ACPI_PARSE_OBJECT       *Op);

BOOLEAN
AcpiDmIsUnicodeBuffer (
    ACPI_PARSE_OBJECT       *Op);

BOOLEAN
AcpiDmIsStringBuffer (
    ACPI_PARSE_OBJECT       *Op);

BOOLEAN
AcpiDmIsPldBuffer (
    ACPI_PARSE_OBJECT       *Op);


/*
 * dmdeferred
 */
ACPI_STATUS
AcpiDmParseDeferredOps (
    ACPI_PARSE_OBJECT       *Root);


/*
 * dmextern
 */
ACPI_STATUS
AcpiDmAddToExternalFileList (
    char                    *PathList);

void
AcpiDmClearExternalFileList (
    void);

void
AcpiDmAddOpToExternalList (
    ACPI_PARSE_OBJECT       *Op,
    char                    *Path,
    UINT8                   Type,
    UINT32                  Value,
    UINT16                  Flags);

void
AcpiDmAddNodeToExternalList (
    ACPI_NAMESPACE_NODE     *Node,
    UINT8                   Type,
    UINT32                  Value,
    UINT16                  Flags);

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

void
AcpiDmUnresolvedWarning (
    UINT8                   Type);

void
AcpiDmGetExternalsFromFile (
    void);

/*
 * dmresrc
 */
void
AcpiDmDumpInteger8 (
    UINT8                   Value,
    const char              *Name);

void
AcpiDmDumpInteger16 (
    UINT16                  Value,
    const char              *Name);

void
AcpiDmDumpInteger32 (
    UINT32                  Value,
    const char              *Name);

void
AcpiDmDumpInteger64 (
    UINT64                  Value,
    const char              *Name);

void
AcpiDmResourceTemplate (
    ACPI_OP_WALK_INFO       *Info,
    ACPI_PARSE_OBJECT       *Op,
    UINT8                   *ByteData,
    UINT32                  ByteCount);

ACPI_STATUS
AcpiDmIsResourceTemplate (
    ACPI_WALK_STATE         *WalkState,
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
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmDwordDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmExtendedDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmQwordDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmMemory24Descriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmMemory32Descriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmFixedMemory32Descriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmGenericRegisterDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmInterruptDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmVendorLargeDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmGpioDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmSerialBusDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmVendorCommon (
    const char              *Name,
    UINT8                   *ByteData,
    UINT32                  Length,
    UINT32                  Level);


/*
 * dmresrcs
 */
void
AcpiDmIrqDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmDmaDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmFixedDmaDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmIoDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmFixedIoDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmStartDependentDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmEndDependentDescriptor (
    ACPI_OP_WALK_INFO       *Info,
    AML_RESOURCE            *Resource,
    UINT32                  Length,
    UINT32                  Level);

void
AcpiDmVendorSmallDescriptor (
    ACPI_OP_WALK_INFO       *Info,
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


/*
 * dmcstyle
 */
BOOLEAN
AcpiDmCheckForSymbolicOpcode (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_OP_WALK_INFO       *Info);

void
AcpiDmCloseOperator (
    ACPI_PARSE_OBJECT       *Op);


/*
 * dmtables
 */
void
AdDisassemblerHeader (
    char                    *Filename,
    UINT8                   TableType);

#define ACPI_IS_AML_TABLE   0
#define ACPI_IS_DATA_TABLE  1


/*
 * adisasm
 */
ACPI_STATUS
AdAmlDisassemble (
    BOOLEAN                 OutToFile,
    char                    *Filename,
    char                    *Prefix,
    char                    **OutFilename);

ACPI_STATUS
AdGetLocalTables (
    void);

ACPI_STATUS
AdParseTable (
    ACPI_TABLE_HEADER       *Table,
    ACPI_OWNER_ID           *OwnerId,
    BOOLEAN                 LoadTable,
    BOOLEAN                 External);

ACPI_STATUS
AdDisplayTables (
    char                    *Filename,
    ACPI_TABLE_HEADER       *Table);

ACPI_STATUS
AdDisplayStatistics (
    void);

#endif  /* __ACDISASM_H__ */
