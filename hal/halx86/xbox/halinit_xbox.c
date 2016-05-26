/*
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          hal/halx86/xbox/halinit_xbox.c
 * PURPOSE:       Initialize the x86 hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include "halxbox.h"

#define NDEBUG
#include <debug.h>

const USHORT HalpBuildType = HAL_BUILD_TYPE;

/* FUNCTIONS ***************************************************************/

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
HalpInitPhase0(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    HalpXboxInitPartIo();
}

VOID
HalpInitPhase1(VOID)
{
}

/* EOF */
