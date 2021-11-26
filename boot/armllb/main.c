/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/main.c
 * PURPOSE:         Main LLB Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

VOID
LlbStartup(IN ULONG Reserved,
           IN ULONG BoardInfo,
           IN PATAG Arguments)
{
    /* Make sure we are booting on the correct kind of machine */
    if (BoardInfo != LlbHwGetBoardType()) while (TRUE);

    /* Initialize hardware components */
    LlbHwInitialize();

    /* Either QEMU or U-Boot itself should send this information */
    LlbEnvParseArguments(Arguments);

    /* Clean up the screen */
    LlbVideoClearScreen(FALSE);

    /* Print header */
    printf("\nReactOS ARM Low-Level Boot Loader [" __DATE__ " "__TIME__ "]\n");

    /* Boot the OS Loader */
    LlbBoot();
    while (TRUE);
}

/* EOF */
