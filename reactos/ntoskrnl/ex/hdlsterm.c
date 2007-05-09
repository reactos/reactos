/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/hdlsterm.c
 * PURPOSE:         Headless Terminal Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HeadlessInit(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PHEADLESS_LOADER_BLOCK HeadlessBlock;

    /* Get the headless loader block */
    HeadlessBlock = LoaderBlock->Extension->HeadlessLoaderBlock;
    if (HeadlessBlock)
    {
        DPRINT1("ReactOS does not currently have Headless Terminal support!\n");
    }
}

/* EOF */
