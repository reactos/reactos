/*******************************************************************************
 *
 * Module Name: rsirq - IRQ resource descriptors
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
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
#include "acresrc.h"

#define _COMPONENT          ACPI_RESOURCES
        ACPI_MODULE_NAME    ("rsirq")


/*******************************************************************************
 *
 * AcpiRsGetIrq
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsGetIrq[9] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_IRQ,
                        ACPI_RS_SIZE (ACPI_RESOURCE_IRQ),
                        ACPI_RSC_TABLE_SIZE (AcpiRsGetIrq)},

    /* Get the IRQ mask (bytes 1:2) */

    {ACPI_RSC_BITMASK16,ACPI_RS_OFFSET (Data.Irq.Interrupts[0]),
                        AML_OFFSET (Irq.IrqMask),
                        ACPI_RS_OFFSET (Data.Irq.InterruptCount)},

    /* Set default flags (others are zero) */

    {ACPI_RSC_SET8,     ACPI_RS_OFFSET (Data.Irq.Triggering),
                        ACPI_EDGE_SENSITIVE,
                        1},

    /* Get the descriptor length (2 or 3 for IRQ descriptor) */

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.Irq.DescriptorLength),
                        AML_OFFSET (Irq.DescriptorType),
                        0},

    /* All done if no flag byte present in descriptor */

    {ACPI_RSC_EXIT_NE,  ACPI_RSC_COMPARE_AML_LENGTH, 0, 3},

    /* Get flags: Triggering[0], Polarity[3], Sharing[4], Wake[5] */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Irq.Triggering),
                        AML_OFFSET (Irq.Flags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Irq.Polarity),
                        AML_OFFSET (Irq.Flags),
                        3},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Irq.Sharable),
                        AML_OFFSET (Irq.Flags),
                        4},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Irq.WakeCapable),
                        AML_OFFSET (Irq.Flags),
                        5}
};


/*******************************************************************************
 *
 * AcpiRsSetIrq
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsSetIrq[14] =
{
    /* Start with a default descriptor of length 3 */

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_IRQ,
                        sizeof (AML_RESOURCE_IRQ),
                        ACPI_RSC_TABLE_SIZE (AcpiRsSetIrq)},

    /* Convert interrupt list to 16-bit IRQ bitmask */

    {ACPI_RSC_BITMASK16,ACPI_RS_OFFSET (Data.Irq.Interrupts[0]),
                        AML_OFFSET (Irq.IrqMask),
                        ACPI_RS_OFFSET (Data.Irq.InterruptCount)},

    /* Set flags: Triggering[0], Polarity[3], Sharing[4], Wake[5] */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Irq.Triggering),
                        AML_OFFSET (Irq.Flags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Irq.Polarity),
                        AML_OFFSET (Irq.Flags),
                        3},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Irq.Sharable),
                        AML_OFFSET (Irq.Flags),
                        4},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Irq.WakeCapable),
                        AML_OFFSET (Irq.Flags),
                        5},

    /*
     * All done if the output descriptor length is required to be 3
     * (i.e., optimization to 2 bytes cannot be attempted)
     */
    {ACPI_RSC_EXIT_EQ,  ACPI_RSC_COMPARE_VALUE,
                        ACPI_RS_OFFSET(Data.Irq.DescriptorLength),
                        3},

    /* Set length to 2 bytes (no flags byte) */

    {ACPI_RSC_LENGTH,   0, 0, sizeof (AML_RESOURCE_IRQ_NOFLAGS)},

    /*
     * All done if the output descriptor length is required to be 2.
     *
     * TBD: Perhaps we should check for error if input flags are not
     * compatible with a 2-byte descriptor.
     */
    {ACPI_RSC_EXIT_EQ,  ACPI_RSC_COMPARE_VALUE,
                        ACPI_RS_OFFSET(Data.Irq.DescriptorLength),
                        2},

    /* Reset length to 3 bytes (descriptor with flags byte) */

    {ACPI_RSC_LENGTH,   0, 0, sizeof (AML_RESOURCE_IRQ)},

    /*
     * Check if the flags byte is necessary. Not needed if the flags are:
     * ACPI_EDGE_SENSITIVE, ACPI_ACTIVE_HIGH, ACPI_EXCLUSIVE
     */
    {ACPI_RSC_EXIT_NE,  ACPI_RSC_COMPARE_VALUE,
                        ACPI_RS_OFFSET (Data.Irq.Triggering),
                        ACPI_EDGE_SENSITIVE},

    {ACPI_RSC_EXIT_NE,  ACPI_RSC_COMPARE_VALUE,
                        ACPI_RS_OFFSET (Data.Irq.Polarity),
                        ACPI_ACTIVE_HIGH},

    {ACPI_RSC_EXIT_NE,  ACPI_RSC_COMPARE_VALUE,
                        ACPI_RS_OFFSET (Data.Irq.Sharable),
                        ACPI_EXCLUSIVE},

    /* We can optimize to a 2-byte IrqNoFlags() descriptor */

    {ACPI_RSC_LENGTH,   0, 0, sizeof (AML_RESOURCE_IRQ_NOFLAGS)}
};


/*******************************************************************************
 *
 * AcpiRsConvertExtIrq
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertExtIrq[10] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_EXTENDED_IRQ,
                        ACPI_RS_SIZE (ACPI_RESOURCE_EXTENDED_IRQ),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertExtIrq)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_EXTENDED_IRQ,
                        sizeof (AML_RESOURCE_EXTENDED_IRQ),
                        0},

    /*
     * Flags: Producer/Consumer[0], Triggering[1], Polarity[2],
     *        Sharing[3], Wake[4]
     */
    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.ExtendedIrq.ProducerConsumer),
                        AML_OFFSET (ExtendedIrq.Flags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.ExtendedIrq.Triggering),
                        AML_OFFSET (ExtendedIrq.Flags),
                        1},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.ExtendedIrq.Polarity),
                        AML_OFFSET (ExtendedIrq.Flags),
                        2},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.ExtendedIrq.Sharable),
                        AML_OFFSET (ExtendedIrq.Flags),
                        3},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.ExtendedIrq.WakeCapable),
                        AML_OFFSET (ExtendedIrq.Flags),
                        4},

    /* IRQ Table length (Byte4) */

    {ACPI_RSC_COUNT,    ACPI_RS_OFFSET (Data.ExtendedIrq.InterruptCount),
                        AML_OFFSET (ExtendedIrq.InterruptCount),
                        sizeof (UINT32)},

    /* Copy every IRQ in the table, each is 32 bits */

    {ACPI_RSC_MOVE32,   ACPI_RS_OFFSET (Data.ExtendedIrq.Interrupts[0]),
                        AML_OFFSET (ExtendedIrq.Interrupts[0]),
                        0},

    /* Optional ResourceSource (Index and String) */

    {ACPI_RSC_SOURCEX,  ACPI_RS_OFFSET (Data.ExtendedIrq.ResourceSource),
                        ACPI_RS_OFFSET (Data.ExtendedIrq.Interrupts[0]),
                        sizeof (AML_RESOURCE_EXTENDED_IRQ)}
};


/*******************************************************************************
 *
 * AcpiRsConvertDma
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertDma[6] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_DMA,
                        ACPI_RS_SIZE (ACPI_RESOURCE_DMA),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertDma)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_DMA,
                        sizeof (AML_RESOURCE_DMA),
                        0},

    /* Flags: transfer preference, bus mastering, channel speed */

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.Dma.Transfer),
                        AML_OFFSET (Dma.Flags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Dma.BusMaster),
                        AML_OFFSET (Dma.Flags),
                        2},

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.Dma.Type),
                        AML_OFFSET (Dma.Flags),
                        5},

    /* DMA channel mask bits */

    {ACPI_RSC_BITMASK,  ACPI_RS_OFFSET (Data.Dma.Channels[0]),
                        AML_OFFSET (Dma.DmaChannelMask),
                        ACPI_RS_OFFSET (Data.Dma.ChannelCount)}
};


/*******************************************************************************
 *
 * AcpiRsConvertFixedDma
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertFixedDma[4] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_FIXED_DMA,
                        ACPI_RS_SIZE (ACPI_RESOURCE_FIXED_DMA),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertFixedDma)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_FIXED_DMA,
                        sizeof (AML_RESOURCE_FIXED_DMA),
                        0},

    /*
     * These fields are contiguous in both the source and destination:
     * RequestLines
     * Channels
     */
    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.FixedDma.RequestLines),
                        AML_OFFSET (FixedDma.RequestLines),
                        2},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.FixedDma.Width),
                        AML_OFFSET (FixedDma.Width),
                        1},
};
