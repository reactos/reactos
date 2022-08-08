/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/include/ntoskrnl.h
 * PURPOSE:         NTOS glue routines for the MINIHAL library
 * PROGRAMMERS:     Hervé Poussineau  <hpoussin@reactos.org>
 */

#include <ntdef.h>
#undef _NTHAL_
#undef NTSYSAPI
#define NTSYSAPI

/* Windows Device Driver Kit */
#include <ntddk.h>
#include <ndk/haltypes.h>

/* Disk stuff */
#include <arc/arc.h>
#include <ntdddisk.h>
#include <internal/hal.h>
