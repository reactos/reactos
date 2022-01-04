/******************************************************************************
 *
 * Module Name: acapps - common include for ACPI applications/tools
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

#ifndef _ACAPPS
#define _ACAPPS

#ifdef ACPI_USE_STANDARD_HEADERS
#include <sys/stat.h>
#endif /* ACPI_USE_STANDARD_HEADERS */

/* Common info for tool signons */

#define ACPICA_NAME                 "Intel ACPI Component Architecture"
#define ACPICA_COPYRIGHT            "Copyright (c) 2000 - 2021 Intel Corporation"

#if ACPI_MACHINE_WIDTH == 64
#define ACPI_WIDTH          " (64-bit version)"

#elif ACPI_MACHINE_WIDTH == 32
#define ACPI_WIDTH          " (32-bit version)"

#else
#error unknown ACPI_MACHINE_WIDTH
#define ACPI_WIDTH          " (unknown bit width, not 32 or 64)"

#endif

/* Macros for signons and file headers */

#define ACPI_COMMON_SIGNON(UtilityName) \
    "\n%s\n%s version %8.8X\n%s\n\n", \
    ACPICA_NAME, \
    UtilityName, ((UINT32) ACPI_CA_VERSION), \
    ACPICA_COPYRIGHT

#define ACPI_COMMON_HEADER(UtilityName, Prefix) \
    "%s%s\n%s%s version %8.8X%s\n%s%s\n%s\n", \
    Prefix, ACPICA_NAME, \
    Prefix, UtilityName, ((UINT32) ACPI_CA_VERSION), ACPI_WIDTH, \
    Prefix, ACPICA_COPYRIGHT, \
    Prefix

#define ACPI_COMMON_BUILD_TIME \
    "Build date/time: %s %s\n", __DATE__, __TIME__

/* Macros for usage messages */

#define ACPI_USAGE_HEADER(Usage) \
    printf ("Usage: %s\nOptions:\n", Usage);

#define ACPI_USAGE_TEXT(Description) \
    printf (Description);

#define ACPI_OPTION(Name, Description) \
    printf ("  %-20s%s\n", Name, Description);


/* Check for unexpected exceptions */

#define ACPI_CHECK_STATUS(Name, Status, Expected) \
    if (Status != Expected) \
    { \
        AcpiOsPrintf ("Unexpected %s from %s (%s-%d)\n", \
            AcpiFormatException (Status), #Name, _AcpiModuleName, __LINE__); \
    }

/* Check for unexpected non-AE_OK errors */


#define ACPI_CHECK_OK(Name, Status)   ACPI_CHECK_STATUS (Name, Status, AE_OK);

#define FILE_SUFFIX_DISASSEMBLY     "dsl"
#define FILE_SUFFIX_BINARY_TABLE    ".dat" /* Needs the dot */


/* acfileio */

ACPI_STATUS
AcGetAllTablesFromFile (
    char                    *Filename,
    UINT8                   GetOnlyAmlTables,
    ACPI_NEW_TABLE_DESC     **ReturnListHead);

void
AcDeleteTableList (
    ACPI_NEW_TABLE_DESC     *ListHead);

BOOLEAN
AcIsFileBinary (
    FILE                    *File);

ACPI_STATUS
AcValidateTableHeader (
    FILE                    *File,
    long                    TableOffset);


/* Values for GetOnlyAmlTables */

#define ACPI_GET_ONLY_AML_TABLES    TRUE
#define ACPI_GET_ALL_TABLES         FALSE


/*
 * getopt
 */
int
AcpiGetopt(
    int                     argc,
    char                    **argv,
    char                    *opts);

int
AcpiGetoptArgument (
    int                     argc,
    char                    **argv);

extern int                  AcpiGbl_Optind;
extern int                  AcpiGbl_Opterr;
extern int                  AcpiGbl_SubOptChar;
extern char                 *AcpiGbl_Optarg;


/*
 * cmfsize - Common get file size function
 */
UINT32
CmGetFileSize (
    ACPI_FILE               File);


/*
 * adwalk
 */
void
AcpiDmCrossReferenceNamespace (
    ACPI_PARSE_OBJECT       *ParseTreeRoot,
    ACPI_NAMESPACE_NODE     *NamespaceRoot,
    ACPI_OWNER_ID           OwnerId);

void
AcpiDmDumpTree (
    ACPI_PARSE_OBJECT       *Origin);

void
AcpiDmFindOrphanMethods (
    ACPI_PARSE_OBJECT       *Origin);

void
AcpiDmFinishNamespaceLoad (
    ACPI_PARSE_OBJECT       *ParseTreeRoot,
    ACPI_NAMESPACE_NODE     *NamespaceRoot,
    ACPI_OWNER_ID           OwnerId);

void
AcpiDmConvertParseObjects (
    ACPI_PARSE_OBJECT       *ParseTreeRoot,
    ACPI_NAMESPACE_NODE     *NamespaceRoot);


/*
 * adfile
 */
ACPI_STATUS
AdInitialize (
    void);

char *
FlGenerateFilename (
    char                    *InputFilename,
    char                    *Suffix);

ACPI_STATUS
FlSplitInputPathname (
    char                    *InputPath,
    char                    **OutDirectoryPath,
    char                    **OutFilename);

char *
FlGetFileBasename (
    char                    *FilePathname);

char *
AdGenerateFilename (
    char                    *Prefix,
    char                    *TableId);

void
AdWriteTable (
    ACPI_TABLE_HEADER       *Table,
    UINT32                  Length,
    char                    *TableName,
    char                    *OemTableId);

#endif /* _ACAPPS */
