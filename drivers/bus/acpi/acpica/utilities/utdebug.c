/******************************************************************************
 *
 * Module Name: utdebug - Debug print routines
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2011, Intel Corp.
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

#define __UTDEBUG_C__

#include "acpi.h"
#include "accommon.h"

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utdebug")


#ifdef ACPI_DEBUG_OUTPUT

static ACPI_THREAD_ID       AcpiGbl_PrevThreadId = (ACPI_THREAD_ID) 0xFFFFFFFF;
static char                 *AcpiGbl_FnEntryStr = "----Entry";
static char                 *AcpiGbl_FnExitStr  = "----Exit-";

/* Local prototypes */

static const char *
AcpiUtTrimFunctionName (
    const char              *FunctionName);


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtInitStackPtrTrace
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Save the current CPU stack pointer at subsystem startup
 *
 ******************************************************************************/

void
AcpiUtInitStackPtrTrace (
    void)
{
    ACPI_SIZE               CurrentSp;


    AcpiGbl_EntryStackPointer = &CurrentSp;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtTrackStackPtr
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Save the current CPU stack pointer
 *
 ******************************************************************************/

void
AcpiUtTrackStackPtr (
    void)
{
    ACPI_SIZE               CurrentSp;


    if (&CurrentSp < AcpiGbl_LowestStackPointer)
    {
        AcpiGbl_LowestStackPointer = &CurrentSp;
    }

    if (AcpiGbl_NestingLevel > AcpiGbl_DeepestNesting)
    {
        AcpiGbl_DeepestNesting = AcpiGbl_NestingLevel;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtTrimFunctionName
 *
 * PARAMETERS:  FunctionName        - Ascii string containing a procedure name
 *
 * RETURN:      Updated pointer to the function name
 *
 * DESCRIPTION: Remove the "Acpi" prefix from the function name, if present.
 *              This allows compiler macros such as __FUNCTION__ to be used
 *              with no change to the debug output.
 *
 ******************************************************************************/

static const char *
AcpiUtTrimFunctionName (
    const char              *FunctionName)
{

    /* All Function names are longer than 4 chars, check is safe */

    if (*(ACPI_CAST_PTR (UINT32, FunctionName)) == ACPI_PREFIX_MIXED)
    {
        /* This is the case where the original source has not been modified */

        return (FunctionName + 4);
    }

    if (*(ACPI_CAST_PTR (UINT32, FunctionName)) == ACPI_PREFIX_LOWER)
    {
        /* This is the case where the source has been 'linuxized' */

        return (FunctionName + 5);
    }

    return (FunctionName);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDebugPrint
 *
 * PARAMETERS:  RequestedDebugLevel - Requested debug print level
 *              LineNumber          - Caller's line number (for error output)
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *              Format              - Printf format field
 *              ...                 - Optional printf arguments
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print error message with prefix consisting of the module name,
 *              line number, and component ID.
 *
 ******************************************************************************/

void  ACPI_INTERNAL_VAR_XFACE
AcpiDebugPrint (
    UINT32                  RequestedDebugLevel,
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    const char              *Format,
    ...)
{
    ACPI_THREAD_ID          ThreadId;
    va_list                 args;


    /*
     * Stay silent if the debug level or component ID is disabled
     */
    if (!(RequestedDebugLevel & AcpiDbgLevel) ||
        !(ComponentId & AcpiDbgLayer))
    {
        return;
    }

    /*
     * Thread tracking and context switch notification
     */
    ThreadId = AcpiOsGetThreadId ();
    if (ThreadId != AcpiGbl_PrevThreadId)
    {
        if (ACPI_LV_THREADS & AcpiDbgLevel)
        {
            AcpiOsPrintf (
                "\n**** Context Switch from TID %u to TID %u ****\n\n",
                (UINT32) AcpiGbl_PrevThreadId, (UINT32) ThreadId);
        }

        AcpiGbl_PrevThreadId = ThreadId;
    }

    /*
     * Display the module name, current line number, thread ID (if requested),
     * current procedure nesting level, and the current procedure name
     */
    AcpiOsPrintf ("%8s-%04ld ", ModuleName, LineNumber);

    if (ACPI_LV_THREADS & AcpiDbgLevel)
    {
        AcpiOsPrintf ("[%u] ", (UINT32) ThreadId);
    }

    AcpiOsPrintf ("[%02ld] %-22.22s: ",
        AcpiGbl_NestingLevel, AcpiUtTrimFunctionName (FunctionName));

    va_start (args, Format);
    AcpiOsVprintf (Format, args);
    va_end (args);
}

ACPI_EXPORT_SYMBOL (AcpiDebugPrint)


/*******************************************************************************
 *
 * FUNCTION:    AcpiDebugPrintRaw
 *
 * PARAMETERS:  RequestedDebugLevel - Requested debug print level
 *              LineNumber          - Caller's line number
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *              Format              - Printf format field
 *              ...                 - Optional printf arguments
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print message with no headers.  Has same interface as
 *              DebugPrint so that the same macros can be used.
 *
 ******************************************************************************/

void  ACPI_INTERNAL_VAR_XFACE
AcpiDebugPrintRaw (
    UINT32                  RequestedDebugLevel,
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    const char              *Format,
    ...)
{
    va_list                 args;


    if (!(RequestedDebugLevel & AcpiDbgLevel) ||
        !(ComponentId & AcpiDbgLayer))
    {
        return;
    }

    va_start (args, Format);
    AcpiOsVprintf (Format, args);
    va_end (args);
}

ACPI_EXPORT_SYMBOL (AcpiDebugPrintRaw)


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtTrace
 *
 * PARAMETERS:  LineNumber          - Caller's line number
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function entry trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in DebugLevel
 *
 ******************************************************************************/

void
AcpiUtTrace (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId)
{

    AcpiGbl_NestingLevel++;
    AcpiUtTrackStackPtr ();

    AcpiDebugPrint (ACPI_LV_FUNCTIONS,
        LineNumber, FunctionName, ModuleName, ComponentId,
        "%s\n", AcpiGbl_FnEntryStr);
}

ACPI_EXPORT_SYMBOL (AcpiUtTrace)


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtTracePtr
 *
 * PARAMETERS:  LineNumber          - Caller's line number
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *              Pointer             - Pointer to display
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function entry trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in DebugLevel
 *
 ******************************************************************************/

void
AcpiUtTracePtr (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    void                    *Pointer)
{
    AcpiGbl_NestingLevel++;
    AcpiUtTrackStackPtr ();

    AcpiDebugPrint (ACPI_LV_FUNCTIONS,
        LineNumber, FunctionName, ModuleName, ComponentId,
        "%s %p\n", AcpiGbl_FnEntryStr, Pointer);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtTraceStr
 *
 * PARAMETERS:  LineNumber          - Caller's line number
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *              String              - Additional string to display
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function entry trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in DebugLevel
 *
 ******************************************************************************/

void
AcpiUtTraceStr (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    char                    *String)
{

    AcpiGbl_NestingLevel++;
    AcpiUtTrackStackPtr ();

    AcpiDebugPrint (ACPI_LV_FUNCTIONS,
        LineNumber, FunctionName, ModuleName, ComponentId,
        "%s %s\n", AcpiGbl_FnEntryStr, String);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtTraceU32
 *
 * PARAMETERS:  LineNumber          - Caller's line number
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *              Integer             - Integer to display
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function entry trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in DebugLevel
 *
 ******************************************************************************/

void
AcpiUtTraceU32 (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    UINT32                  Integer)
{

    AcpiGbl_NestingLevel++;
    AcpiUtTrackStackPtr ();

    AcpiDebugPrint (ACPI_LV_FUNCTIONS,
        LineNumber, FunctionName, ModuleName, ComponentId,
        "%s %08X\n", AcpiGbl_FnEntryStr, Integer);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtExit
 *
 * PARAMETERS:  LineNumber          - Caller's line number
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function exit trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in DebugLevel
 *
 ******************************************************************************/

void
AcpiUtExit (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId)
{

    AcpiDebugPrint (ACPI_LV_FUNCTIONS,
        LineNumber, FunctionName, ModuleName, ComponentId,
        "%s\n", AcpiGbl_FnExitStr);

    AcpiGbl_NestingLevel--;
}

ACPI_EXPORT_SYMBOL (AcpiUtExit)


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStatusExit
 *
 * PARAMETERS:  LineNumber          - Caller's line number
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *              Status              - Exit status code
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function exit trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in DebugLevel.  Prints exit status also.
 *
 ******************************************************************************/

void
AcpiUtStatusExit (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    ACPI_STATUS             Status)
{

    if (ACPI_SUCCESS (Status))
    {
        AcpiDebugPrint (ACPI_LV_FUNCTIONS,
            LineNumber, FunctionName, ModuleName, ComponentId,
            "%s %s\n", AcpiGbl_FnExitStr,
            AcpiFormatException (Status));
    }
    else
    {
        AcpiDebugPrint (ACPI_LV_FUNCTIONS,
            LineNumber, FunctionName, ModuleName, ComponentId,
            "%s ****Exception****: %s\n", AcpiGbl_FnExitStr,
            AcpiFormatException (Status));
    }

    AcpiGbl_NestingLevel--;
}

ACPI_EXPORT_SYMBOL (AcpiUtStatusExit)


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtValueExit
 *
 * PARAMETERS:  LineNumber          - Caller's line number
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *              Value               - Value to be printed with exit msg
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function exit trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in DebugLevel.  Prints exit value also.
 *
 ******************************************************************************/

void
AcpiUtValueExit (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    UINT64                  Value)
{

    AcpiDebugPrint (ACPI_LV_FUNCTIONS,
        LineNumber, FunctionName, ModuleName, ComponentId,
        "%s %8.8X%8.8X\n", AcpiGbl_FnExitStr,
        ACPI_FORMAT_UINT64 (Value));

    AcpiGbl_NestingLevel--;
}

ACPI_EXPORT_SYMBOL (AcpiUtValueExit)


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtPtrExit
 *
 * PARAMETERS:  LineNumber          - Caller's line number
 *              FunctionName        - Caller's procedure name
 *              ModuleName          - Caller's module name
 *              ComponentId         - Caller's component ID
 *              Ptr                 - Pointer to display
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function exit trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in DebugLevel.  Prints exit value also.
 *
 ******************************************************************************/

void
AcpiUtPtrExit (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    UINT8                   *Ptr)
{

    AcpiDebugPrint (ACPI_LV_FUNCTIONS,
        LineNumber, FunctionName, ModuleName, ComponentId,
        "%s %p\n", AcpiGbl_FnExitStr, Ptr);

    AcpiGbl_NestingLevel--;
}

#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDumpBuffer
 *
 * PARAMETERS:  Buffer              - Buffer to dump
 *              Count               - Amount to dump, in bytes
 *              Display             - BYTE, WORD, DWORD, or QWORD display
 *              ComponentID         - Caller's component ID
 *
 * RETURN:      None
 *
 * DESCRIPTION: Generic dump buffer in both hex and ascii.
 *
 ******************************************************************************/

void
AcpiUtDumpBuffer2 (
    UINT8                   *Buffer,
    UINT32                  Count,
    UINT32                  Display)
{
    UINT32                  i = 0;
    UINT32                  j;
    UINT32                  Temp32;
    UINT8                   BufChar;


    if (!Buffer)
    {
        AcpiOsPrintf ("Null Buffer Pointer in DumpBuffer!\n");
        return;
    }

    if ((Count < 4) || (Count & 0x01))
    {
        Display = DB_BYTE_DISPLAY;
    }

    /* Nasty little dump buffer routine! */

    while (i < Count)
    {
        /* Print current offset */

        AcpiOsPrintf ("%6.4X: ", i);

        /* Print 16 hex chars */

        for (j = 0; j < 16;)
        {
            if (i + j >= Count)
            {
                /* Dump fill spaces */

                AcpiOsPrintf ("%*s", ((Display * 2) + 1), " ");
                j += Display;
                continue;
            }

            switch (Display)
            {
            case DB_BYTE_DISPLAY:
            default:    /* Default is BYTE display */

                AcpiOsPrintf ("%02X ", Buffer[(ACPI_SIZE) i + j]);
                break;


            case DB_WORD_DISPLAY:

                ACPI_MOVE_16_TO_32 (&Temp32, &Buffer[(ACPI_SIZE) i + j]);
                AcpiOsPrintf ("%04X ", Temp32);
                break;


            case DB_DWORD_DISPLAY:

                ACPI_MOVE_32_TO_32 (&Temp32, &Buffer[(ACPI_SIZE) i + j]);
                AcpiOsPrintf ("%08X ", Temp32);
                break;


            case DB_QWORD_DISPLAY:

                ACPI_MOVE_32_TO_32 (&Temp32, &Buffer[(ACPI_SIZE) i + j]);
                AcpiOsPrintf ("%08X", Temp32);

                ACPI_MOVE_32_TO_32 (&Temp32, &Buffer[(ACPI_SIZE) i + j + 4]);
                AcpiOsPrintf ("%08X ", Temp32);
                break;
            }

            j += Display;
        }

        /*
         * Print the ASCII equivalent characters but watch out for the bad
         * unprintable ones (printable chars are 0x20 through 0x7E)
         */
        AcpiOsPrintf (" ");
        for (j = 0; j < 16; j++)
        {
            if (i + j >= Count)
            {
                AcpiOsPrintf ("\n");
                return;
            }

            BufChar = Buffer[(ACPI_SIZE) i + j];
            if (ACPI_IS_PRINT (BufChar))
            {
                AcpiOsPrintf ("%c", BufChar);
            }
            else
            {
                AcpiOsPrintf (".");
            }
        }

        /* Done with that line. */

        AcpiOsPrintf ("\n");
        i += 16;
    }

    return;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDumpBuffer
 *
 * PARAMETERS:  Buffer              - Buffer to dump
 *              Count               - Amount to dump, in bytes
 *              Display             - BYTE, WORD, DWORD, or QWORD display
 *              ComponentID         - Caller's component ID
 *
 * RETURN:      None
 *
 * DESCRIPTION: Generic dump buffer in both hex and ascii.
 *
 ******************************************************************************/

void
AcpiUtDumpBuffer (
    UINT8                   *Buffer,
    UINT32                  Count,
    UINT32                  Display,
    UINT32                  ComponentId)
{

    /* Only dump the buffer if tracing is enabled */

    if (!((ACPI_LV_TABLES & AcpiDbgLevel) &&
        (ComponentId & AcpiDbgLayer)))
    {
        return;
    }

    AcpiUtDumpBuffer2 (Buffer, Count, Display);
}


