/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/envir.c
 * PURPOSE:         LLB Environment Variable Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

PCHAR
NTAPI
LlbEnvRead(IN PCHAR ValueName)
{
    /* FIXME: HACK */
    return "RAMDISK";
}

/* EOF */

