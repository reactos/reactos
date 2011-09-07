/* $Id: halinit_up.c 53529 2011-09-02 14:45:19Z tkreuzer $
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/hal/x86/halinit.c
 * PURPOSE:       Initalize the x86 hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

VOID
NTAPI
ApicInitializeLocalApic(ULONG Cpu);

/* FUNCTIONS ***************************************************************/

VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Initialize the local APIC for this cpu */
    ApicInitializeLocalApic(ProcessorNumber);

    /* Initialize the timer */
    //ApicInitializeTimer(ProcessorNumber);

}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{

}

VOID
HalpInitPhase1(VOID)
{
    /* Initialize DMA. NT does this in Phase 0 */
    HalpInitDma();
}

/* EOF */
