/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/idt.c
 * PURPOSE:         IDT managment
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

IDT_DESCRIPTOR KiIdt[256];

struct
{
  USHORT Length;
  ULONG Base;
} __attribute__((packed)) KiIdtDescriptor = {256 * 8, (ULONG)KiIdt};


/* FUNCTIONS *****************************************************************/


