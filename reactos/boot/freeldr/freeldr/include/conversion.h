/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/windows/conversion.c
 * PURPOSE:         Physical <-> Virtual addressing mode conversions (arch-specific)
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

#pragma once

#ifndef _ZOOM2_
/* Arch-specific addresses translation implementation */
FORCEINLINE
PVOID
VaToPa(PVOID Va)
{
    return (PVOID)((ULONG_PTR)Va & ~KSEG0_BASE);
}

FORCEINLINE
PVOID
PaToVa(PVOID Pa)
{
    return (PVOID)((ULONG_PTR)Pa | KSEG0_BASE);
}
#else
FORCEINLINE
PVOID
VaToPa(PVOID Va)
{
    return Va;
}

FORCEINLINE
PVOID
PaToVa(PVOID Pa)
{
    return Pa;
}
#endif
