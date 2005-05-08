/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/idt.c
 * PURPOSE:         IDT managment
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

IDT_DESCRIPTOR KiIdt[256];

#include <pshpack1.h>

struct
{
  USHORT Length;
  ULONG Base;
} KiIdtDescriptor = {256 * 8, (ULONG)KiIdt};

#include <poppack.h>

/* FUNCTIONS *****************************************************************/


