/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/arm/boot.s
 * PURPOSE:         Implements the entry point for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

    .title "ARM FreeLDR Entry Point"
    .include "ntoskrnl/include/internal/arm/kxarm.h"
    .include "ntoskrnl/include/internal/arm/ksarm.h"

    NESTED_ENTRY _start
    PROLOG_END _start
    
    b ArmInit
    
    ENTRY_END _start

L_ArmInit:
    .long ArmInit

.global PageDirectoryStart, PageDirectoryEnd
.global startup_pagedirectory
.global kernel_pagetable

.bss
PageDirectoryStart:
kernel_pagetable:
    .fill 2*4096, 1, 0
    .space 4096
startup_pagedirectory:
    .fill 4*4096, 1, 0
    

.global PageDirectoryEnd
PageDirectoryEnd:
