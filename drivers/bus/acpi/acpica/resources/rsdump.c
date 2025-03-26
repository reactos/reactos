/*******************************************************************************
 *
 * Module Name: rsdump - AML debugger support for resource structures.
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2022, Intel Corp.
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

#include "acpi.h"
#include "accommon.h"
#include "acresrc.h"

#define _COMPONENT          ACPI_RESOURCES
        ACPI_MODULE_NAME    ("rsdump")

/*
 * All functions in this module are used by the AML Debugger only
 */

/* Local prototypes */

static void
AcpiRsOutString (
    const char              *Title,
    const char              *Value);

static void
AcpiRsOutInteger8 (
    const char              *Title,
    UINT8                   Value);

static void
AcpiRsOutInteger16 (
    const char              *Title,
    UINT16                  Value);

static void
AcpiRsOutInteger32 (
    const char              *Title,
    UINT32                  Value);

static void
AcpiRsOutInteger64 (
    const char              *Title,
    UINT64                  Value);

static void
AcpiRsOutTitle (
    const char              *Title);

static void
AcpiRsDumpByteList (
    UINT16                  Length,
    UINT8                   *Data);

static void
AcpiRsDumpWordList (
    UINT16                  Length,
    UINT16                  *Data);

static void
AcpiRsDumpDwordList (
    UINT8                   Length,
    UINT32                  *Data);

static void
AcpiRsDumpShortByteList (
    UINT8                   Length,
    UINT8                   *Data);

static void
AcpiRsDumpResourceSource (
    ACPI_RESOURCE_SOURCE    *ResourceSource);

static void
AcpiRsDumpResourceLabel (
    char                   *Title,
    ACPI_RESOURCE_LABEL    *ResourceLabel);

static void
AcpiRsDumpAddressCommon (
    ACPI_RESOURCE_DATA      *Resource);

static void
AcpiRsDumpDescriptor (
    void                    *Resource,
    ACPI_RSDUMP_INFO        *Table);


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsDumpResourceList
 *
 * PARAMETERS:  ResourceList        - Pointer to a resource descriptor list
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dispatches the structure to the correct dump routine.
 *
 ******************************************************************************/

void
AcpiRsDumpResourceList (
    ACPI_RESOURCE           *ResourceList)
{
    UINT32                  Count = 0;
    UINT32                  Type;


    ACPI_FUNCTION_ENTRY ();


    /* Check if debug output enabled */

    if (!ACPI_IS_DEBUG_ENABLED (ACPI_LV_RESOURCES, _COMPONENT))
    {
        return;
    }

    /* Walk list and dump all resource descriptors (END_TAG terminates) */

    do
    {
        AcpiOsPrintf ("\n[%02X] ", Count);
        Count++;

        /* Validate Type before dispatch */

        Type = ResourceList->Type;
        if (Type > ACPI_RESOURCE_TYPE_MAX)
        {
            AcpiOsPrintf (
                "Invalid descriptor type (%X) in resource list\n",
                ResourceList->Type);
            return;
        }
        else if (!ResourceList->Type)
        {
            ACPI_ERROR ((AE_INFO, "Invalid Zero Resource Type"));
            return;
        }

        /* Sanity check the length. It must not be zero, or we loop forever */

        if (!ResourceList->Length)
        {
            AcpiOsPrintf (
                "Invalid zero length descriptor in resource list\n");
            return;
        }

        /* Dump the resource descriptor */

        if (Type == ACPI_RESOURCE_TYPE_SERIAL_BUS)
        {
            AcpiRsDumpDescriptor (&ResourceList->Data,
                AcpiGbl_DumpSerialBusDispatch[
                    ResourceList->Data.CommonSerialBus.Type]);
        }
        else
        {
            AcpiRsDumpDescriptor (&ResourceList->Data,
                AcpiGbl_DumpResourceDispatch[Type]);
        }

        /* Point to the next resource structure */

        ResourceList = ACPI_NEXT_RESOURCE (ResourceList);

        /* Exit when END_TAG descriptor is reached */

    } while (Type != ACPI_RESOURCE_TYPE_END_TAG);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsDumpIrqList
 *
 * PARAMETERS:  RouteTable      - Pointer to the routing table to dump.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print IRQ routing table
 *
 ******************************************************************************/

void
AcpiRsDumpIrqList (
    UINT8                   *RouteTable)
{
    ACPI_PCI_ROUTING_TABLE  *PrtElement;
    UINT8                   Count;


    ACPI_FUNCTION_ENTRY ();


    /* Check if debug output enabled */

    if (!ACPI_IS_DEBUG_ENABLED (ACPI_LV_RESOURCES, _COMPONENT))
    {
        return;
    }

    PrtElement = ACPI_CAST_PTR (ACPI_PCI_ROUTING_TABLE, RouteTable);

    /* Dump all table elements, Exit on zero length element */

    for (Count = 0; PrtElement->Length; Count++)
    {
        AcpiOsPrintf ("\n[%02X] PCI IRQ Routing Table Package\n", Count);
        AcpiRsDumpDescriptor (PrtElement, AcpiRsDumpPrt);

        PrtElement = ACPI_ADD_PTR (ACPI_PCI_ROUTING_TABLE,
            PrtElement, PrtElement->Length);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsDumpDescriptor
 *
 * PARAMETERS:  Resource            - Buffer containing the resource
 *              Table               - Table entry to decode the resource
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump a resource descriptor based on a dump table entry.
 *
 ******************************************************************************/

static void
AcpiRsDumpDescriptor (
    void                    *Resource,
    ACPI_RSDUMP_INFO        *Table)
{
    UINT8                   *Target = NULL;
    UINT8                   *PreviousTarget;
    const char              *Name;
    UINT8                   Count;


    /* First table entry must contain the table length (# of table entries) */

    Count = Table->Offset;

    while (Count)
    {
        PreviousTarget = Target;
        Target = ACPI_ADD_PTR (UINT8, Resource, Table->Offset);
        Name = Table->Name;

        switch (Table->Opcode)
        {
        case ACPI_RSD_TITLE:
            /*
             * Optional resource title
             */
            if (Table->Name)
            {
                AcpiOsPrintf ("%s Resource\n", Name);
            }
            break;

        /* Strings */

        case ACPI_RSD_LITERAL:

            AcpiRsOutString (Name, ACPI_CAST_PTR (char, Table->Pointer));
            break;

        case ACPI_RSD_STRING:

            AcpiRsOutString (Name, ACPI_CAST_PTR (char, Target));
            break;

        /* Data items, 8/16/32/64 bit */

        case ACPI_RSD_UINT8:

            if (Table->Pointer)
            {
                AcpiRsOutString (Name, Table->Pointer [*Target]);
            }
            else
            {
                AcpiRsOutInteger8 (Name, ACPI_GET8 (Target));
            }
            break;

        case ACPI_RSD_UINT16:

            AcpiRsOutInteger16 (Name, ACPI_GET16 (Target));
            break;

        case ACPI_RSD_UINT32:

            AcpiRsOutInteger32 (Name, ACPI_GET32 (Target));
            break;

        case ACPI_RSD_UINT64:

            AcpiRsOutInteger64 (Name, ACPI_GET64 (Target));
            break;

        /* Flags: 1-bit and 2-bit flags supported */

        case ACPI_RSD_1BITFLAG:

            AcpiRsOutString (Name, Table->Pointer [*Target & 0x01]);
            break;

        case ACPI_RSD_2BITFLAG:

            AcpiRsOutString (Name, Table->Pointer [*Target & 0x03]);
            break;

        case ACPI_RSD_3BITFLAG:

            AcpiRsOutString (Name, Table->Pointer [*Target & 0x07]);
            break;

        case ACPI_RSD_6BITFLAG:

            AcpiRsOutInteger8 (Name, (ACPI_GET8 (Target) & 0x3F));
            break;

        case ACPI_RSD_SHORTLIST:
            /*
             * Short byte list (single line output) for DMA and IRQ resources
             * Note: The list length is obtained from the previous table entry
             */
            if (PreviousTarget)
            {
                AcpiRsOutTitle (Name);
                AcpiRsDumpShortByteList (*PreviousTarget, Target);
            }
            break;

        case ACPI_RSD_SHORTLISTX:
            /*
             * Short byte list (single line output) for GPIO vendor data
             * Note: The list length is obtained from the previous table entry
             */
            if (PreviousTarget)
            {
                AcpiRsOutTitle (Name);
                AcpiRsDumpShortByteList (*PreviousTarget,
                    *(ACPI_CAST_INDIRECT_PTR (UINT8, Target)));
            }
            break;

        case ACPI_RSD_LONGLIST:
            /*
             * Long byte list for Vendor resource data
             * Note: The list length is obtained from the previous table entry
             */
            if (PreviousTarget)
            {
                AcpiRsDumpByteList (ACPI_GET16 (PreviousTarget), Target);
            }
            break;

        case ACPI_RSD_DWORDLIST:
            /*
             * Dword list for Extended Interrupt resources
             * Note: The list length is obtained from the previous table entry
             */
            if (PreviousTarget)
            {
                AcpiRsDumpDwordList (*PreviousTarget,
                    ACPI_CAST_PTR (UINT32, Target));
            }
            break;

        case ACPI_RSD_WORDLIST:
            /*
             * Word list for GPIO Pin Table
             * Note: The list length is obtained from the previous table entry
             */
            if (PreviousTarget)
            {
                AcpiRsDumpWordList (*PreviousTarget,
                    *(ACPI_CAST_INDIRECT_PTR (UINT16, Target)));
            }
            break;

        case ACPI_RSD_ADDRESS:
            /*
             * Common flags for all Address resources
             */
            AcpiRsDumpAddressCommon (ACPI_CAST_PTR (
                ACPI_RESOURCE_DATA, Target));
            break;

        case ACPI_RSD_SOURCE:
            /*
             * Optional ResourceSource for Address resources
             */
            AcpiRsDumpResourceSource (ACPI_CAST_PTR (
                ACPI_RESOURCE_SOURCE, Target));
            break;

        case ACPI_RSD_LABEL:
            /*
             * ResourceLabel
             */
            AcpiRsDumpResourceLabel ("Resource Label", ACPI_CAST_PTR (
                ACPI_RESOURCE_LABEL, Target));
            break;

        case ACPI_RSD_SOURCE_LABEL:
            /*
             * ResourceSourceLabel
             */
            AcpiRsDumpResourceLabel ("Resource Source Label", ACPI_CAST_PTR (
                ACPI_RESOURCE_LABEL, Target));
            break;

        default:

            AcpiOsPrintf ("**** Invalid table opcode [%X] ****\n",
                Table->Opcode);
            return;
        }

        Table++;
        Count--;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsDumpResourceSource
 *
 * PARAMETERS:  ResourceSource      - Pointer to a Resource Source struct
 *
 * RETURN:      None
 *
 * DESCRIPTION: Common routine for dumping the optional ResourceSource and the
 *              corresponding ResourceSourceIndex.
 *
 ******************************************************************************/

static void
AcpiRsDumpResourceSource (
    ACPI_RESOURCE_SOURCE    *ResourceSource)
{
    ACPI_FUNCTION_ENTRY ();


    if (ResourceSource->Index == 0xFF)
    {
        return;
    }

    AcpiRsOutInteger8 ("Resource Source Index",
        ResourceSource->Index);

    AcpiRsOutString ("Resource Source",
        ResourceSource->StringPtr ?
            ResourceSource->StringPtr : "[Not Specified]");
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsDumpResourceLabel
 *
 * PARAMETERS:  Title              - Title of the dumped resource field
 *              ResourceLabel      - Pointer to a Resource Label struct
 *
 * RETURN:      None
 *
 * DESCRIPTION: Common routine for dumping the ResourceLabel
 *
 ******************************************************************************/

static void
AcpiRsDumpResourceLabel (
    char                   *Title,
    ACPI_RESOURCE_LABEL    *ResourceLabel)
{
    ACPI_FUNCTION_ENTRY ();

    AcpiRsOutString (Title,
        ResourceLabel->StringPtr ?
            ResourceLabel->StringPtr : "[Not Specified]");
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsDumpAddressCommon
 *
 * PARAMETERS:  Resource        - Pointer to an internal resource descriptor
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump the fields that are common to all Address resource
 *              descriptors
 *
 ******************************************************************************/

static void
AcpiRsDumpAddressCommon (
    ACPI_RESOURCE_DATA      *Resource)
{
    ACPI_FUNCTION_ENTRY ();


   /* Decode the type-specific flags */

    switch (Resource->Address.ResourceType)
    {
    case ACPI_MEMORY_RANGE:

        AcpiRsDumpDescriptor (Resource, AcpiRsDumpMemoryFlags);
        break;

    case ACPI_IO_RANGE:

        AcpiRsDumpDescriptor (Resource, AcpiRsDumpIoFlags);
        break;

    case ACPI_BUS_NUMBER_RANGE:

        AcpiRsOutString ("Resource Type", "Bus Number Range");
        break;

    default:

        AcpiRsOutInteger8 ("Resource Type",
            (UINT8) Resource->Address.ResourceType);
        break;
    }

    /* Decode the general flags */

    AcpiRsDumpDescriptor (Resource, AcpiRsDumpGeneralFlags);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsOut*
 *
 * PARAMETERS:  Title       - Name of the resource field
 *              Value       - Value of the resource field
 *
 * RETURN:      None
 *
 * DESCRIPTION: Miscellaneous helper functions to consistently format the
 *              output of the resource dump routines
 *
 ******************************************************************************/

static void
AcpiRsOutString (
    const char              *Title,
    const char              *Value)
{

    AcpiOsPrintf ("%27s : %s", Title, Value);
    if (!*Value)
    {
        AcpiOsPrintf ("[NULL NAMESTRING]");
    }
    AcpiOsPrintf ("\n");
}

static void
AcpiRsOutInteger8 (
    const char              *Title,
    UINT8                   Value)
{
    AcpiOsPrintf ("%27s : %2.2X\n", Title, Value);
}

static void
AcpiRsOutInteger16 (
    const char              *Title,
    UINT16                  Value)
{

    AcpiOsPrintf ("%27s : %4.4X\n", Title, Value);
}

static void
AcpiRsOutInteger32 (
    const char              *Title,
    UINT32                  Value)
{

    AcpiOsPrintf ("%27s : %8.8X\n", Title, Value);
}

static void
AcpiRsOutInteger64 (
    const char              *Title,
    UINT64                  Value)
{

    AcpiOsPrintf ("%27s : %8.8X%8.8X\n", Title,
        ACPI_FORMAT_UINT64 (Value));
}

static void
AcpiRsOutTitle (
    const char              *Title)
{

    AcpiOsPrintf ("%27s : ", Title);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsDump*List
 *
 * PARAMETERS:  Length      - Number of elements in the list
 *              Data        - Start of the list
 *
 * RETURN:      None
 *
 * DESCRIPTION: Miscellaneous functions to dump lists of raw data
 *
 ******************************************************************************/

static void
AcpiRsDumpByteList (
    UINT16                  Length,
    UINT8                   *Data)
{
    UINT16                  i;


    for (i = 0; i < Length; i++)
    {
        AcpiOsPrintf ("%25s%2.2X : %2.2X\n", "Byte", i, Data[i]);
    }
}

static void
AcpiRsDumpShortByteList (
    UINT8                   Length,
    UINT8                   *Data)
{
    UINT8                   i;


    for (i = 0; i < Length; i++)
    {
        AcpiOsPrintf ("%X ", Data[i]);
    }

    AcpiOsPrintf ("\n");
}

static void
AcpiRsDumpDwordList (
    UINT8                   Length,
    UINT32                  *Data)
{
    UINT8                   i;


    for (i = 0; i < Length; i++)
    {
        AcpiOsPrintf ("%25s%2.2X : %8.8X\n", "Dword", i, Data[i]);
    }
}

static void
AcpiRsDumpWordList (
    UINT16                  Length,
    UINT16                  *Data)
{
    UINT16                  i;


    for (i = 0; i < Length; i++)
    {
        AcpiOsPrintf ("%25s%2.2X : %4.4X\n", "Word", i, Data[i]);
    }
}
