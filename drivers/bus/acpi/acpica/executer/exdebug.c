/******************************************************************************
 *
 * Module Name: exdebug - Support for stores to the AML Debug Object
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

#include "acpi.h"
#include "accommon.h"
#include "acinterp.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exdebug")


#ifndef ACPI_NO_ERROR_MESSAGES
/*******************************************************************************
 *
 * FUNCTION:    AcpiExDoDebugObject
 *
 * PARAMETERS:  SourceDesc          - Object to be output to "Debug Object"
 *              Level               - Indentation level (used for packages)
 *              Index               - Current package element, zero if not pkg
 *
 * RETURN:      None
 *
 * DESCRIPTION: Handles stores to the AML Debug Object. For example:
 *              Store(INT1, Debug)
 *
 * This function is not compiled if ACPI_NO_ERROR_MESSAGES is set.
 *
 * This function is only enabled if AcpiGbl_EnableAmlDebugObject is set, or
 * if ACPI_LV_DEBUG_OBJECT is set in the AcpiDbgLevel. Thus, in the normal
 * operational case, stores to the debug object are ignored but can be easily
 * enabled if necessary.
 *
 ******************************************************************************/

void
AcpiExDoDebugObject (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    UINT32                  Level,
    UINT32                  Index)
{
    UINT32                  i;
    UINT32                  Timer;
    ACPI_OPERAND_OBJECT     *ObjectDesc;
    UINT32                  Value;


    ACPI_FUNCTION_TRACE_PTR (ExDoDebugObject, SourceDesc);


    /* Output must be enabled via the DebugObject global or the DbgLevel */

    if (!AcpiGbl_EnableAmlDebugObject &&
        !(AcpiDbgLevel & ACPI_LV_DEBUG_OBJECT))
    {
        return_VOID;
    }

    /* Null string or newline -- don't emit the line header */

    if (SourceDesc &&
        (ACPI_GET_DESCRIPTOR_TYPE (SourceDesc) == ACPI_DESC_TYPE_OPERAND) &&
        (SourceDesc->Common.Type == ACPI_TYPE_STRING))
    {
        if ((SourceDesc->String.Length == 0) ||
                ((SourceDesc->String.Length == 1) &&
                (*SourceDesc->String.Pointer == '\n')))
        {
            AcpiOsPrintf ("\n");
            return_VOID;
        }
    }

    /*
     * Print line header as long as we are not in the middle of an
     * object display
     */
    if (!((Level > 0) && Index == 0))
    {
        if (AcpiGbl_DisplayDebugTimer)
        {
            /*
             * We will emit the current timer value (in microseconds) with each
             * debug output. Only need the lower 26 bits. This allows for 67
             * million microseconds or 67 seconds before rollover.
             *
             * Convert 100 nanosecond units to microseconds
             */
            Timer = ((UINT32) AcpiOsGetTimer () / 10);
            Timer &= 0x03FFFFFF;

            AcpiOsPrintf ("[ACPI Debug T=0x%8.8X] %*s", Timer, Level, " ");
        }
        else
        {
            AcpiOsPrintf ("[ACPI Debug] %*s", Level, " ");
        }
    }

    /* Display the index for package output only */

    if (Index > 0)
    {
       AcpiOsPrintf ("(%.2u) ", Index - 1);
    }

    if (!SourceDesc)
    {
        AcpiOsPrintf ("[Null Object]\n");
        return_VOID;
    }

    if (ACPI_GET_DESCRIPTOR_TYPE (SourceDesc) == ACPI_DESC_TYPE_OPERAND)
    {
        /* No object type prefix needed for integers and strings */

        if ((SourceDesc->Common.Type != ACPI_TYPE_INTEGER) &&
            (SourceDesc->Common.Type != ACPI_TYPE_STRING))
        {
            AcpiOsPrintf ("%s  ", AcpiUtGetObjectTypeName (SourceDesc));
        }

        if (!AcpiUtValidInternalObject (SourceDesc))
        {
           AcpiOsPrintf ("%p, Invalid Internal Object!\n", SourceDesc);
           return_VOID;
        }
    }
    else if (ACPI_GET_DESCRIPTOR_TYPE (SourceDesc) == ACPI_DESC_TYPE_NAMED)
    {
        AcpiOsPrintf ("%s  (Node %p)\n",
            AcpiUtGetTypeName (((ACPI_NAMESPACE_NODE *) SourceDesc)->Type),
                SourceDesc);
        return_VOID;
    }
    else
    {
        return_VOID;
    }

    /* SourceDesc is of type ACPI_DESC_TYPE_OPERAND */

    switch (SourceDesc->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        /* Output correct integer width */

        if (AcpiGbl_IntegerByteWidth == 4)
        {
            AcpiOsPrintf ("0x%8.8X\n",
                (UINT32) SourceDesc->Integer.Value);
        }
        else
        {
            AcpiOsPrintf ("0x%8.8X%8.8X\n",
                ACPI_FORMAT_UINT64 (SourceDesc->Integer.Value));
        }
        break;

    case ACPI_TYPE_BUFFER:

        AcpiOsPrintf ("[0x%.2X]\n", (UINT32) SourceDesc->Buffer.Length);
        AcpiUtDumpBuffer (SourceDesc->Buffer.Pointer,
            (SourceDesc->Buffer.Length < 256) ?
                SourceDesc->Buffer.Length : 256, DB_BYTE_DISPLAY, 0);
        break;

    case ACPI_TYPE_STRING:

        AcpiOsPrintf ("\"%s\"\n", SourceDesc->String.Pointer);
        break;

    case ACPI_TYPE_PACKAGE:

        AcpiOsPrintf ("(Contains 0x%.2X Elements):\n",
            SourceDesc->Package.Count);

        /* Output the entire contents of the package */

        for (i = 0; i < SourceDesc->Package.Count; i++)
        {
            AcpiExDoDebugObject (SourceDesc->Package.Elements[i],
                Level + 4, i + 1);
        }
        break;

    case ACPI_TYPE_LOCAL_REFERENCE:

        AcpiOsPrintf ("[%s] ", AcpiUtGetReferenceName (SourceDesc));

        /* Decode the reference */

        switch (SourceDesc->Reference.Class)
        {
        case ACPI_REFCLASS_INDEX:

            AcpiOsPrintf ("0x%X\n", SourceDesc->Reference.Value);
            break;

        case ACPI_REFCLASS_TABLE:

            /* Case for DdbHandle */

            AcpiOsPrintf ("Table Index 0x%X\n", SourceDesc->Reference.Value);
            return_VOID;

        default:

            break;
        }

        AcpiOsPrintf ("  ");

        /* Check for valid node first, then valid object */

        if (SourceDesc->Reference.Node)
        {
            if (ACPI_GET_DESCRIPTOR_TYPE (SourceDesc->Reference.Node) !=
                ACPI_DESC_TYPE_NAMED)
            {
                AcpiOsPrintf (" %p - Not a valid namespace node\n",
                    SourceDesc->Reference.Node);
            }
            else
            {
                AcpiOsPrintf ("Node %p [%4.4s] ", SourceDesc->Reference.Node,
                    (SourceDesc->Reference.Node)->Name.Ascii);

                switch ((SourceDesc->Reference.Node)->Type)
                {
                /* These types have no attached object */

                case ACPI_TYPE_DEVICE:
                    AcpiOsPrintf ("Device\n");
                    break;

                case ACPI_TYPE_THERMAL:
                    AcpiOsPrintf ("Thermal Zone\n");
                    break;

                default:

                    AcpiExDoDebugObject ((SourceDesc->Reference.Node)->Object,
                        Level + 4, 0);
                    break;
                }
            }
        }
        else if (SourceDesc->Reference.Object)
        {
            if (ACPI_GET_DESCRIPTOR_TYPE (SourceDesc->Reference.Object) ==
                ACPI_DESC_TYPE_NAMED)
            {
                /* Reference object is a namespace node */

                AcpiExDoDebugObject (ACPI_CAST_PTR (ACPI_OPERAND_OBJECT,
                    SourceDesc->Reference.Object),
                    Level + 4, 0);
            }
            else
            {
                ObjectDesc = SourceDesc->Reference.Object;
                Value = SourceDesc->Reference.Value;

                switch (ObjectDesc->Common.Type)
                {
                case ACPI_TYPE_BUFFER:

                    AcpiOsPrintf ("Buffer[%u] = 0x%2.2X\n",
                        Value, *SourceDesc->Reference.IndexPointer);
                    break;

                case ACPI_TYPE_STRING:

                    AcpiOsPrintf ("String[%u] = \"%c\" (0x%2.2X)\n",
                        Value, *SourceDesc->Reference.IndexPointer,
                        *SourceDesc->Reference.IndexPointer);
                    break;

                case ACPI_TYPE_PACKAGE:

                    AcpiOsPrintf ("Package[%u] = ", Value);
                    if (!(*SourceDesc->Reference.Where))
                    {
                        AcpiOsPrintf ("[Uninitialized Package Element]\n");
                    }
                    else
                    {
                        AcpiExDoDebugObject (*SourceDesc->Reference.Where,
                            Level+4, 0);
                    }
                    break;

                default:

                    AcpiOsPrintf ("Unknown Reference object type %X\n",
                        ObjectDesc->Common.Type);
                    break;
                }
            }
        }
        break;

    default:

        AcpiOsPrintf ("(Descriptor %p)\n", SourceDesc);
        break;
    }

    ACPI_DEBUG_PRINT_RAW ((ACPI_DB_EXEC, "\n"));
    return_VOID;
}
#endif
