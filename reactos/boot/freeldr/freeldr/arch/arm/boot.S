/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/arm/boot.s
 * PURPOSE:         Implements the entry point for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include <ksarm.h>

    NESTED_ENTRY _start
    PROLOG_END _start

    __assertfail

    NESTED_END _start

//L_ArmInit:
//    .long ArmInit

    END
/* EOF */
