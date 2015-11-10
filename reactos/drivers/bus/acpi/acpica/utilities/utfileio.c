/*******************************************************************************
 *
 * Module Name: utfileio - simple file I/O routines
 *
 ******************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2015, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
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
 * to or modifications of the Original Intel Code. No other license or right
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
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
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
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
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

#include "acpi.h"
#include "accommon.h"
#include "actables.h"
#include "acapps.h"

#ifdef ACPI_ASL_COMPILER
#include "aslcompiler.h"
#endif


#define _COMPONENT          ACPI_CA_DEBUGGER
        ACPI_MODULE_NAME    ("utfileio")


#ifdef ACPI_APPLICATION

/* Local prototypes */

static ACPI_STATUS
AcpiUtCheckTextModeCorruption (
    UINT8                   *Table,
    UINT32                  TableLength,
    UINT32                  FileLength);

static ACPI_STATUS
AcpiUtReadTable (
    FILE                    *fp,
    ACPI_TABLE_HEADER       **Table,
    UINT32                  *TableLength);


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtCheckTextModeCorruption
 *
 * PARAMETERS:  Table           - Table buffer
 *              TableLength     - Length of table from the table header
 *              FileLength      - Length of the file that contains the table
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check table for text mode file corruption where all linefeed
 *              characters (LF) have been replaced by carriage return linefeed
 *              pairs (CR/LF).
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiUtCheckTextModeCorruption (
    UINT8                   *Table,
    UINT32                  TableLength,
    UINT32                  FileLength)
{
    UINT32                  i;
    UINT32                  Pairs = 0;


    if (TableLength != FileLength)
    {
        ACPI_WARNING ((AE_INFO,
            "File length (0x%X) is not the same as the table length (0x%X)",
            FileLength, TableLength));
    }

    /* Scan entire table to determine if each LF has been prefixed with a CR */

    for (i = 1; i < FileLength; i++)
    {
        if (Table[i] == 0x0A)
        {
            if (Table[i - 1] != 0x0D)
            {
                /* The LF does not have a preceding CR, table not corrupted */

                return (AE_OK);
            }
            else
            {
                /* Found a CR/LF pair */

                Pairs++;
            }
            i++;
        }
    }

    if (!Pairs)
    {
        return (AE_OK);
    }

    /*
     * Entire table scanned, each CR is part of a CR/LF pair --
     * meaning that the table was treated as a text file somewhere.
     *
     * NOTE: We can't "fix" the table, because any existing CR/LF pairs in the
     * original table are left untouched by the text conversion process --
     * meaning that we cannot simply replace CR/LF pairs with LFs.
     */
    AcpiOsPrintf ("Table has been corrupted by text mode conversion\n");
    AcpiOsPrintf ("All LFs (%u) were changed to CR/LF pairs\n", Pairs);
    AcpiOsPrintf ("Table cannot be repaired!\n");
    return (AE_BAD_VALUE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtReadTable
 *
 * PARAMETERS:  fp              - File that contains table
 *              Table           - Return value, buffer with table
 *              TableLength     - Return value, length of table
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load the DSDT from the file pointer
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiUtReadTable (
    FILE                    *fp,
    ACPI_TABLE_HEADER       **Table,
    UINT32                  *TableLength)
{
    ACPI_TABLE_HEADER       TableHeader;
    UINT32                  Actual;
    ACPI_STATUS             Status;
    UINT32                  FileSize;
    BOOLEAN                 StandardHeader = TRUE;
    INT32                   Count;

    /* Get the file size */

    FileSize = CmGetFileSize (fp);
    if (FileSize == ACPI_UINT32_MAX)
    {
        return (AE_ERROR);
    }

    if (FileSize < 4)
    {
        return (AE_BAD_HEADER);
    }

    /* Read the signature */

    fseek (fp, 0, SEEK_SET);

    Count = fread (&TableHeader, 1, sizeof (ACPI_TABLE_HEADER), fp);
    if (Count != sizeof (ACPI_TABLE_HEADER))
    {
        AcpiOsPrintf ("Could not read the table header\n");
        return (AE_BAD_HEADER);
    }

    /* The RSDP table does not have standard ACPI header */

    if (ACPI_VALIDATE_RSDP_SIG (TableHeader.Signature))
    {
        *TableLength = FileSize;
        StandardHeader = FALSE;
    }
    else
    {

#if 0
        /* Validate the table header/length */

        Status = AcpiTbValidateTableHeader (&TableHeader);
        if (ACPI_FAILURE (Status))
        {
            AcpiOsPrintf ("Table header is invalid!\n");
            return (Status);
        }
#endif

        /* File size must be at least as long as the Header-specified length */

        if (TableHeader.Length > FileSize)
        {
            AcpiOsPrintf (
                "TableHeader length [0x%X] greater than the input file size [0x%X]\n",
                TableHeader.Length, FileSize);

#ifdef ACPI_ASL_COMPILER
            Status = FlCheckForAscii (fp, NULL, FALSE);
            if (ACPI_SUCCESS (Status))
            {
                AcpiOsPrintf ("File appears to be ASCII only, must be binary\n");
            }
#endif
            return (AE_BAD_HEADER);
        }

#ifdef ACPI_OBSOLETE_CODE
        /* We only support a limited number of table types */

        if (!ACPI_COMPARE_NAME ((char *) TableHeader.Signature, ACPI_SIG_DSDT) &&
            !ACPI_COMPARE_NAME ((char *) TableHeader.Signature, ACPI_SIG_PSDT) &&
            !ACPI_COMPARE_NAME ((char *) TableHeader.Signature, ACPI_SIG_SSDT))
        {
            AcpiOsPrintf ("Table signature [%4.4s] is invalid or not supported\n",
                (char *) TableHeader.Signature);
            ACPI_DUMP_BUFFER (&TableHeader, sizeof (ACPI_TABLE_HEADER));
            return (AE_ERROR);
        }
#endif

        *TableLength = TableHeader.Length;
    }

    /* Allocate a buffer for the table */

    *Table = AcpiOsAllocate ((size_t) FileSize);
    if (!*Table)
    {
        AcpiOsPrintf (
            "Could not allocate memory for ACPI table %4.4s (size=0x%X)\n",
            TableHeader.Signature, *TableLength);
        return (AE_NO_MEMORY);
    }

    /* Get the rest of the table */

    fseek (fp, 0, SEEK_SET);
    Actual = fread (*Table, 1, (size_t) FileSize, fp);
    if (Actual == FileSize)
    {
        if (StandardHeader)
        {
            /* Now validate the checksum */

            Status = AcpiTbVerifyChecksum ((void *) *Table,
                        ACPI_CAST_PTR (ACPI_TABLE_HEADER, *Table)->Length);

            if (Status == AE_BAD_CHECKSUM)
            {
                Status = AcpiUtCheckTextModeCorruption ((UINT8 *) *Table,
                            FileSize, (*Table)->Length);
                return (Status);
            }
        }
        return (AE_OK);
    }

    if (Actual > 0)
    {
        AcpiOsPrintf ("Warning - reading table, asked for %X got %X\n",
            FileSize, Actual);
        return (AE_OK);
    }

    AcpiOsPrintf ("Error - could not read the table file\n");
    AcpiOsFree (*Table);
    *Table = NULL;
    *TableLength = 0;
    return (AE_ERROR);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtReadTableFromFile
 *
 * PARAMETERS:  Filename         - File where table is located
 *              Table            - Where a pointer to the table is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get an ACPI table from a file
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtReadTableFromFile (
    char                    *Filename,
    ACPI_TABLE_HEADER       **Table)
{
    FILE                    *File;
    UINT32                  FileSize;
    UINT32                  TableLength;
    ACPI_STATUS             Status = AE_ERROR;


    /* Open the file, get current size */

    File = fopen (Filename, "rb");
    if (!File)
    {
        perror ("Could not open input file");
        return (Status);
    }

    FileSize = CmGetFileSize (File);
    if (FileSize == ACPI_UINT32_MAX)
    {
        goto Exit;
    }

    /* Get the entire file */

    fprintf (stderr, "Loading Acpi table from file %10s - Length %.8u (%06X)\n",
        Filename, FileSize, FileSize);

    Status = AcpiUtReadTable (File, Table, &TableLength);
    if (ACPI_FAILURE (Status))
    {
        AcpiOsPrintf ("Could not get table from the file\n");
    }

Exit:
    fclose(File);
    return (Status);
}

#endif
