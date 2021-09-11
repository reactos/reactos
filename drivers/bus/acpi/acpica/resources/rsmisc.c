/*******************************************************************************
 *
 * Module Name: rsmisc - Miscellaneous resource descriptors
 *
 ******************************************************************************/

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

#include "acpi.h"
#include "accommon.h"
#include "acresrc.h"

#define _COMPONENT          ACPI_RESOURCES
        ACPI_MODULE_NAME    ("rsmisc")


#define INIT_RESOURCE_TYPE(i)       i->ResourceOffset
#define INIT_RESOURCE_LENGTH(i)     i->AmlOffset
#define INIT_TABLE_LENGTH(i)        i->Value

#define COMPARE_OPCODE(i)           i->ResourceOffset
#define COMPARE_TARGET(i)           i->AmlOffset
#define COMPARE_VALUE(i)            i->Value


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsConvertAmlToResource
 *
 * PARAMETERS:  Resource            - Pointer to the resource descriptor
 *              Aml                 - Where the AML descriptor is returned
 *              Info                - Pointer to appropriate conversion table
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an external AML resource descriptor to the corresponding
 *              internal resource descriptor
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsConvertAmlToResource (
    ACPI_RESOURCE           *Resource,
    AML_RESOURCE            *Aml,
    ACPI_RSCONVERT_INFO     *Info)
{
    ACPI_RS_LENGTH          AmlResourceLength;
    void                    *Source;
    void                    *Destination;
    char                    *Target;
    UINT8                   Count;
    UINT8                   FlagsMode = FALSE;
    UINT16                  ItemCount = 0;
    UINT16                  Temp16 = 0;


    ACPI_FUNCTION_TRACE (RsConvertAmlToResource);


    if (!Info)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (((ACPI_SIZE) Resource) & 0x3)
    {
        /* Each internal resource struct is expected to be 32-bit aligned */

        ACPI_WARNING ((AE_INFO,
            "Misaligned resource pointer (get): %p Type 0x%2.2X Length %u",
            Resource, Resource->Type, Resource->Length));
    }

    /* Extract the resource Length field (does not include header length) */

    AmlResourceLength = AcpiUtGetResourceLength (Aml);

    /*
     * First table entry must be ACPI_RSC_INITxxx and must contain the
     * table length (# of table entries)
     */
    Count = INIT_TABLE_LENGTH (Info);
    while (Count)
    {
        Target = NULL;

        /*
         * Source is the external AML byte stream buffer,
         * destination is the internal resource descriptor
         */
        Source = ACPI_ADD_PTR (void, Aml, Info->AmlOffset);
        Destination = ACPI_ADD_PTR (void, Resource, Info->ResourceOffset);

        switch (Info->Opcode)
        {
        case ACPI_RSC_INITGET:
            /*
             * Get the resource type and the initial (minimum) length
             */
            memset (Resource, 0, INIT_RESOURCE_LENGTH (Info));
            Resource->Type = INIT_RESOURCE_TYPE (Info);
            Resource->Length = INIT_RESOURCE_LENGTH (Info);
            break;

        case ACPI_RSC_INITSET:
            break;

        case ACPI_RSC_FLAGINIT:

            FlagsMode = TRUE;
            break;

        case ACPI_RSC_1BITFLAG:
            /*
             * Mask and shift the flag bit
             */
            ACPI_SET8 (Destination,
                ((ACPI_GET8 (Source) >> Info->Value) & 0x01));
            break;

        case ACPI_RSC_2BITFLAG:
            /*
             * Mask and shift the flag bits
             */
            ACPI_SET8 (Destination,
                ((ACPI_GET8 (Source) >> Info->Value) & 0x03));
            break;

        case ACPI_RSC_3BITFLAG:
            /*
             * Mask and shift the flag bits
             */
            ACPI_SET8 (Destination,
                ((ACPI_GET8 (Source) >> Info->Value) & 0x07));
            break;

        case ACPI_RSC_6BITFLAG:
            /*
             * Mask and shift the flag bits
             */
            ACPI_SET8 (Destination,
                ((ACPI_GET8 (Source) >> Info->Value) & 0x3F));
            break;

        case ACPI_RSC_COUNT:

            ItemCount = ACPI_GET8 (Source);
            ACPI_SET8 (Destination, ItemCount);

            Resource->Length = Resource->Length +
                (Info->Value * (ItemCount - 1));
            break;

        case ACPI_RSC_COUNT16:

            ItemCount = AmlResourceLength;
            ACPI_SET16 (Destination, ItemCount);

            Resource->Length = Resource->Length +
                (Info->Value * (ItemCount - 1));
            break;

        case ACPI_RSC_COUNT_GPIO_PIN:

            Target = ACPI_ADD_PTR (void, Aml, Info->Value);
            ItemCount = ACPI_GET16 (Target) - ACPI_GET16 (Source);

            Resource->Length = Resource->Length + ItemCount;
            ItemCount = ItemCount / 2;
            ACPI_SET16 (Destination, ItemCount);
            break;

        case ACPI_RSC_COUNT_GPIO_VEN:

            ItemCount = ACPI_GET8 (Source);
            ACPI_SET8 (Destination, ItemCount);

            Resource->Length = Resource->Length + (Info->Value * ItemCount);
            break;

        case ACPI_RSC_COUNT_GPIO_RES:
            /*
             * Vendor data is optional (length/offset may both be zero)
             * Examine vendor data length field first
             */
            Target = ACPI_ADD_PTR (void, Aml, (Info->Value + 2));
            if (ACPI_GET16 (Target))
            {
                /* Use vendor offset to get resource source length */

                Target = ACPI_ADD_PTR (void, Aml, Info->Value);
                ItemCount = ACPI_GET16 (Target) - ACPI_GET16 (Source);
            }
            else
            {
                /* No vendor data to worry about */

                ItemCount = Aml->LargeHeader.ResourceLength +
                    sizeof (AML_RESOURCE_LARGE_HEADER) -
                    ACPI_GET16 (Source);
            }

            Resource->Length = Resource->Length + ItemCount;
            ACPI_SET16 (Destination, ItemCount);
            break;

        case ACPI_RSC_COUNT_SERIAL_VEN:

            ItemCount = ACPI_GET16 (Source) - Info->Value;

            Resource->Length = Resource->Length + ItemCount;
            ACPI_SET16 (Destination, ItemCount);
            break;

        case ACPI_RSC_COUNT_SERIAL_RES:

            ItemCount = (AmlResourceLength +
                sizeof (AML_RESOURCE_LARGE_HEADER)) -
                ACPI_GET16 (Source) - Info->Value;

            Resource->Length = Resource->Length + ItemCount;
            ACPI_SET16 (Destination, ItemCount);
            break;

        case ACPI_RSC_LENGTH:

            Resource->Length = Resource->Length + Info->Value;
            break;

        case ACPI_RSC_MOVE8:
        case ACPI_RSC_MOVE16:
        case ACPI_RSC_MOVE32:
        case ACPI_RSC_MOVE64:
            /*
             * Raw data move. Use the Info value field unless ItemCount has
             * been previously initialized via a COUNT opcode
             */
            if (Info->Value)
            {
                ItemCount = Info->Value;
            }
            AcpiRsMoveData (Destination, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_MOVE_GPIO_PIN:

            /* Generate and set the PIN data pointer */

            Target = (char *) ACPI_ADD_PTR (void, Resource,
                (Resource->Length - ItemCount * 2));
            *(UINT16 **) Destination = ACPI_CAST_PTR (UINT16, Target);

            /* Copy the PIN data */

            Source = ACPI_ADD_PTR (void, Aml, ACPI_GET16 (Source));
            AcpiRsMoveData (Target, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_MOVE_GPIO_RES:

            /* Generate and set the ResourceSource string pointer */

            Target = (char *) ACPI_ADD_PTR (void, Resource,
                (Resource->Length - ItemCount));
            *(UINT8 **) Destination = ACPI_CAST_PTR (UINT8, Target);

            /* Copy the ResourceSource string */

            Source = ACPI_ADD_PTR (void, Aml, ACPI_GET16 (Source));
            AcpiRsMoveData (Target, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_MOVE_SERIAL_VEN:

            /* Generate and set the Vendor Data pointer */

            Target = (char *) ACPI_ADD_PTR (void, Resource,
                (Resource->Length - ItemCount));
            *(UINT8 **) Destination = ACPI_CAST_PTR (UINT8, Target);

            /* Copy the Vendor Data */

            Source = ACPI_ADD_PTR (void, Aml, Info->Value);
            AcpiRsMoveData (Target, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_MOVE_SERIAL_RES:

            /* Generate and set the ResourceSource string pointer */

            Target = (char *) ACPI_ADD_PTR (void, Resource,
                (Resource->Length - ItemCount));
            *(UINT8 **) Destination = ACPI_CAST_PTR (UINT8, Target);

            /* Copy the ResourceSource string */

            Source = ACPI_ADD_PTR (
                void, Aml, (ACPI_GET16 (Source) + Info->Value));
            AcpiRsMoveData (Target, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_SET8:

            memset (Destination, Info->AmlOffset, Info->Value);
            break;

        case ACPI_RSC_DATA8:

            Target = ACPI_ADD_PTR (char, Resource, Info->Value);
            memcpy (Destination, Source,  ACPI_GET16 (Target));
            break;

        case ACPI_RSC_ADDRESS:
            /*
             * Common handler for address descriptor flags
             */
            if (!AcpiRsGetAddressCommon (Resource, Aml))
            {
                return_ACPI_STATUS (AE_AML_INVALID_RESOURCE_TYPE);
            }
            break;

        case ACPI_RSC_SOURCE:
            /*
             * Optional ResourceSource (Index and String)
             */
            Resource->Length +=
                AcpiRsGetResourceSource (AmlResourceLength, Info->Value,
                    Destination, Aml, NULL);
            break;

        case ACPI_RSC_SOURCEX:
            /*
             * Optional ResourceSource (Index and String). This is the more
             * complicated case used by the Interrupt() macro
             */
            Target = ACPI_ADD_PTR (char, Resource,
                Info->AmlOffset + (ItemCount * 4));

            Resource->Length +=
                AcpiRsGetResourceSource (AmlResourceLength, (ACPI_RS_LENGTH)
                    (((ItemCount - 1) * sizeof (UINT32)) + Info->Value),
                    Destination, Aml, Target);
            break;

        case ACPI_RSC_BITMASK:
            /*
             * 8-bit encoded bitmask (DMA macro)
             */
            ItemCount = AcpiRsDecodeBitmask (ACPI_GET8 (Source), Destination);
            if (ItemCount)
            {
                Resource->Length += (ItemCount - 1);
            }

            Target = ACPI_ADD_PTR (char, Resource, Info->Value);
            ACPI_SET8 (Target, ItemCount);
            break;

        case ACPI_RSC_BITMASK16:
            /*
             * 16-bit encoded bitmask (IRQ macro)
             */
            ACPI_MOVE_16_TO_16 (&Temp16, Source);

            ItemCount = AcpiRsDecodeBitmask (Temp16, Destination);
            if (ItemCount)
            {
                Resource->Length += (ItemCount - 1);
            }

            Target = ACPI_ADD_PTR (char, Resource, Info->Value);
            ACPI_SET8 (Target, ItemCount);
            break;

        case ACPI_RSC_EXIT_NE:
            /*
             * Control - Exit conversion if not equal
             */
            switch (Info->ResourceOffset)
            {
            case ACPI_RSC_COMPARE_AML_LENGTH:

                if (AmlResourceLength != Info->Value)
                {
                    goto Exit;
                }
                break;

            case ACPI_RSC_COMPARE_VALUE:

                if (ACPI_GET8 (Source) != Info->Value)
                {
                    goto Exit;
                }
                break;

            default:

                ACPI_ERROR ((AE_INFO, "Invalid conversion sub-opcode"));
                return_ACPI_STATUS (AE_BAD_PARAMETER);
            }
            break;

        default:

            ACPI_ERROR ((AE_INFO, "Invalid conversion opcode"));
            return_ACPI_STATUS (AE_BAD_PARAMETER);
        }

        Count--;
        Info++;
    }

Exit:
    if (!FlagsMode)
    {
        /* Round the resource struct length up to the next boundary (32 or 64) */

        Resource->Length = (UINT32)
            ACPI_ROUND_UP_TO_NATIVE_WORD (Resource->Length);
    }
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsConvertResourceToAml
 *
 * PARAMETERS:  Resource            - Pointer to the resource descriptor
 *              Aml                 - Where the AML descriptor is returned
 *              Info                - Pointer to appropriate conversion table
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an internal resource descriptor to the corresponding
 *              external AML resource descriptor.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsConvertResourceToAml (
    ACPI_RESOURCE           *Resource,
    AML_RESOURCE            *Aml,
    ACPI_RSCONVERT_INFO     *Info)
{
    void                    *Source = NULL;
    void                    *Destination;
    char                    *Target;
    ACPI_RSDESC_SIZE        AmlLength = 0;
    UINT8                   Count;
    UINT16                  Temp16 = 0;
    UINT16                  ItemCount = 0;


    ACPI_FUNCTION_TRACE (RsConvertResourceToAml);


    if (!Info)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /*
     * First table entry must be ACPI_RSC_INITxxx and must contain the
     * table length (# of table entries)
     */
    Count = INIT_TABLE_LENGTH (Info);

    while (Count)
    {
        /*
         * Source is the internal resource descriptor,
         * destination is the external AML byte stream buffer
         */
        Source = ACPI_ADD_PTR (void, Resource, Info->ResourceOffset);
        Destination = ACPI_ADD_PTR (void, Aml, Info->AmlOffset);

        switch (Info->Opcode)
        {
        case ACPI_RSC_INITSET:

            memset (Aml, 0, INIT_RESOURCE_LENGTH (Info));
            AmlLength = INIT_RESOURCE_LENGTH (Info);
            AcpiRsSetResourceHeader (
                INIT_RESOURCE_TYPE (Info), AmlLength, Aml);
            break;

        case ACPI_RSC_INITGET:
            break;

        case ACPI_RSC_FLAGINIT:
            /*
             * Clear the flag byte
             */
            ACPI_SET8 (Destination, 0);
            break;

        case ACPI_RSC_1BITFLAG:
            /*
             * Mask and shift the flag bit
             */
            ACPI_SET_BIT (*ACPI_CAST8 (Destination), (UINT8)
                ((ACPI_GET8 (Source) & 0x01) << Info->Value));
            break;

        case ACPI_RSC_2BITFLAG:
            /*
             * Mask and shift the flag bits
             */
            ACPI_SET_BIT (*ACPI_CAST8 (Destination), (UINT8)
                ((ACPI_GET8 (Source) & 0x03) << Info->Value));
            break;

        case ACPI_RSC_3BITFLAG:
            /*
             * Mask and shift the flag bits
             */
            ACPI_SET_BIT (*ACPI_CAST8 (Destination), (UINT8)
                ((ACPI_GET8 (Source) & 0x07) << Info->Value));
            break;

        case ACPI_RSC_6BITFLAG:
            /*
             * Mask and shift the flag bits
             */
            ACPI_SET_BIT (*ACPI_CAST8 (Destination), (UINT8)
                ((ACPI_GET8 (Source) & 0x3F) << Info->Value));
            break;

        case ACPI_RSC_COUNT:

            ItemCount = ACPI_GET8 (Source);
            ACPI_SET8 (Destination, ItemCount);

            AmlLength = (UINT16)
                (AmlLength + (Info->Value * (ItemCount - 1)));
            break;

        case ACPI_RSC_COUNT16:

            ItemCount = ACPI_GET16 (Source);
            AmlLength = (UINT16) (AmlLength + ItemCount);
            AcpiRsSetResourceLength (AmlLength, Aml);
            break;

        case ACPI_RSC_COUNT_GPIO_PIN:

            ItemCount = ACPI_GET16 (Source);
            ACPI_SET16 (Destination, AmlLength);

            AmlLength = (UINT16) (AmlLength + ItemCount * 2);
            Target = ACPI_ADD_PTR (void, Aml, Info->Value);
            ACPI_SET16 (Target, AmlLength);
            AcpiRsSetResourceLength (AmlLength, Aml);
            break;

        case ACPI_RSC_COUNT_GPIO_VEN:

            ItemCount = ACPI_GET16 (Source);
            ACPI_SET16 (Destination, ItemCount);

            AmlLength = (UINT16) (
                AmlLength + (Info->Value * ItemCount));
            AcpiRsSetResourceLength (AmlLength, Aml);
            break;

        case ACPI_RSC_COUNT_GPIO_RES:

            /* Set resource source string length */

            ItemCount = ACPI_GET16 (Source);
            ACPI_SET16 (Destination, AmlLength);

            /* Compute offset for the Vendor Data */

            AmlLength = (UINT16) (AmlLength + ItemCount);
            Target = ACPI_ADD_PTR (void, Aml, Info->Value);

            /* Set vendor offset only if there is vendor data */

            ACPI_SET16 (Target, AmlLength);

            AcpiRsSetResourceLength (AmlLength, Aml);
            break;

        case ACPI_RSC_COUNT_SERIAL_VEN:

            ItemCount = ACPI_GET16 (Source);
            ACPI_SET16 (Destination, ItemCount + Info->Value);
            AmlLength = (UINT16) (AmlLength + ItemCount);
            AcpiRsSetResourceLength (AmlLength, Aml);
            break;

        case ACPI_RSC_COUNT_SERIAL_RES:

            ItemCount = ACPI_GET16 (Source);
            AmlLength = (UINT16) (AmlLength + ItemCount);
            AcpiRsSetResourceLength (AmlLength, Aml);
            break;

        case ACPI_RSC_LENGTH:

            AcpiRsSetResourceLength (Info->Value, Aml);
            break;

        case ACPI_RSC_MOVE8:
        case ACPI_RSC_MOVE16:
        case ACPI_RSC_MOVE32:
        case ACPI_RSC_MOVE64:

            if (Info->Value)
            {
                ItemCount = Info->Value;
            }
            AcpiRsMoveData (Destination, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_MOVE_GPIO_PIN:

            Destination = (char *) ACPI_ADD_PTR (void, Aml,
                ACPI_GET16 (Destination));
            Source = * (UINT16 **) Source;
            AcpiRsMoveData (Destination, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_MOVE_GPIO_RES:

            /* Used for both ResourceSource string and VendorData */

            Destination = (char *) ACPI_ADD_PTR (void, Aml,
                ACPI_GET16 (Destination));
            Source = * (UINT8 **) Source;
            AcpiRsMoveData (Destination, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_MOVE_SERIAL_VEN:

            Destination = (char *) ACPI_ADD_PTR (void, Aml,
                (AmlLength - ItemCount));
            Source = * (UINT8 **) Source;
            AcpiRsMoveData (Destination, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_MOVE_SERIAL_RES:

            Destination = (char *) ACPI_ADD_PTR (void, Aml,
                (AmlLength - ItemCount));
            Source = * (UINT8 **) Source;
            AcpiRsMoveData (Destination, Source, ItemCount, Info->Opcode);
            break;

        case ACPI_RSC_ADDRESS:

            /* Set the Resource Type, General Flags, and Type-Specific Flags */

            AcpiRsSetAddressCommon (Aml, Resource);
            break;

        case ACPI_RSC_SOURCEX:
            /*
             * Optional ResourceSource (Index and String)
             */
            AmlLength = AcpiRsSetResourceSource (
                Aml, (ACPI_RS_LENGTH) AmlLength, Source);
            AcpiRsSetResourceLength (AmlLength, Aml);
            break;

        case ACPI_RSC_SOURCE:
            /*
             * Optional ResourceSource (Index and String). This is the more
             * complicated case used by the Interrupt() macro
             */
            AmlLength = AcpiRsSetResourceSource (Aml, Info->Value, Source);
            AcpiRsSetResourceLength (AmlLength, Aml);
            break;

        case ACPI_RSC_BITMASK:
            /*
             * 8-bit encoded bitmask (DMA macro)
             */
            ACPI_SET8 (Destination,
                AcpiRsEncodeBitmask (Source,
                    *ACPI_ADD_PTR (UINT8, Resource, Info->Value)));
            break;

        case ACPI_RSC_BITMASK16:
            /*
             * 16-bit encoded bitmask (IRQ macro)
             */
            Temp16 = AcpiRsEncodeBitmask (
                Source, *ACPI_ADD_PTR (UINT8, Resource, Info->Value));
            ACPI_MOVE_16_TO_16 (Destination, &Temp16);
            break;

        case ACPI_RSC_EXIT_LE:
            /*
             * Control - Exit conversion if less than or equal
             */
            if (ItemCount <= Info->Value)
            {
                goto Exit;
            }
            break;

        case ACPI_RSC_EXIT_NE:
            /*
             * Control - Exit conversion if not equal
             */
            switch (COMPARE_OPCODE (Info))
            {
            case ACPI_RSC_COMPARE_VALUE:

                if (*ACPI_ADD_PTR (UINT8, Resource,
                    COMPARE_TARGET (Info)) != COMPARE_VALUE (Info))
                {
                    goto Exit;
                }
                break;

            default:

                ACPI_ERROR ((AE_INFO, "Invalid conversion sub-opcode"));
                return_ACPI_STATUS (AE_BAD_PARAMETER);
            }
            break;

        case ACPI_RSC_EXIT_EQ:
            /*
             * Control - Exit conversion if equal
             */
            if (*ACPI_ADD_PTR (UINT8, Resource,
                COMPARE_TARGET (Info)) == COMPARE_VALUE (Info))
            {
                goto Exit;
            }
            break;

        default:

            ACPI_ERROR ((AE_INFO, "Invalid conversion opcode"));
            return_ACPI_STATUS (AE_BAD_PARAMETER);
        }

        Count--;
        Info++;
    }

Exit:
    return_ACPI_STATUS (AE_OK);
}


#if 0
/* Previous resource validations */

    if (Aml->ExtAddress64.RevisionID !=
        AML_RESOURCE_EXTENDED_ADDRESS_REVISION)
    {
        return_ACPI_STATUS (AE_SUPPORT);
    }

    if (Resource->Data.StartDpf.PerformanceRobustness >= 3)
    {
        return_ACPI_STATUS (AE_AML_BAD_RESOURCE_VALUE);
    }

    if (((Aml->Irq.Flags & 0x09) == 0x00) ||
        ((Aml->Irq.Flags & 0x09) == 0x09))
    {
        /*
         * Only [ActiveHigh, EdgeSensitive] or [ActiveLow, LevelSensitive]
         * polarity/trigger interrupts are allowed (ACPI spec, section
         * "IRQ Format"), so 0x00 and 0x09 are illegal.
         */
        ACPI_ERROR ((AE_INFO,
            "Invalid interrupt polarity/trigger in resource list, 0x%X",
            Aml->Irq.Flags));
        return_ACPI_STATUS (AE_BAD_DATA);
    }

    Resource->Data.ExtendedIrq.InterruptCount = Temp8;
    if (Temp8 < 1)
    {
        /* Must have at least one IRQ */

        return_ACPI_STATUS (AE_AML_BAD_RESOURCE_LENGTH);
    }

    if (Resource->Data.Dma.Transfer == 0x03)
    {
        ACPI_ERROR ((AE_INFO,
            "Invalid DMA.Transfer preference (3)"));
        return_ACPI_STATUS (AE_BAD_DATA);
    }
#endif
