/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * FILE:            ntoskrnl/include/internal/trap.h
 * PURPOSE:         Trap definitions
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 10/12/00
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_TRAP_H
#define __NTOSKRNL_INCLUDE_INTERNAL_TRAP_H

#define TF_SAVED_ORIG_STACK (0x8C)
#define TF_REGS         (0x90)
#define TF_ORIG_EBP     (0x94)

#ifndef AS_INVOKED

#include <internal/ke.h>

typedef struct _KV86M_TRAP_FRAME
{
  KTRAP_FRAME Tf;
  
  ULONG SavedInitialStack;

  /*
   * These are put on the top of the stack by the routine that entered
   * v86 mode so the exception handlers can find the control information
   */
  struct _KV86M_REGISTERS* regs;     
  ULONG orig_ebp;
} KV86M_TRAP_FRAME, *PKV86M_TRAP_FRAME;

ULONG
KeV86Exception(ULONG ExceptionNr, PKTRAP_FRAME Tf, ULONG address);

#endif /* !AS_INVOKED */

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_TRAP_H */
