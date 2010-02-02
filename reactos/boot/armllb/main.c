/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/main.c
 * PURPOSE:         Main LLB Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

VOID
LlbStartup(VOID)
{
    /* Initialize hardware components */
    LlbHwInitialize();

    /* Clean up the screen */
    LlbVideoClearScreen();

    /* Print header */
    printf("ReactOS ARM Low-Level Boot Loader [" __DATE__ " "__TIME__ "]\n");
    
    /* Boot the OS Loader */
    LlbBoot("");
    while (TRUE);
}

/* EOF */
