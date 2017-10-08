/*
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          hal/halx86/up/halinit_up.c
 * PURPOSE:       Initialize the x86 hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

const USHORT HalpBuildType = HAL_BUILD_TYPE;

/* FUNCTIONS ****************************************************************/

VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Set default IDR */
    KeGetPcr()->IDR = 0xFFFFFFFB;
}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{

}

VOID
HalpInitPhase1(VOID)
{
        /* Enable IRQ 0 */
        HalpEnableInterruptHandler(IDT_DEVICE,
                                   0,
                                   PRIMARY_VECTOR_BASE,
                                   CLOCK2_LEVEL,
                                   HalpClockInterrupt,
                                   Latched);

        /* Enable IRQ 8 */
        HalpEnableInterruptHandler(IDT_DEVICE,
                                   0,
                                   PRIMARY_VECTOR_BASE + 8,
                                   PROFILE_LEVEL,
                                   HalpProfileInterrupt,
                                   Latched);

        /* Initialize DMA. NT does this in Phase 0 */
        HalpInitDma();
}

/* EOF */
