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

#define TF_EDI          (0x0)
#define TF_ESI          (0x4)
#define TF_EBP          (0x8)
#define TF_ESP          (0xC)
#define TF_EBX          (0x10)
#define TF_EDX          (0x14)
#define TF_ECX          (0x18)
#define TF_EAX          (0x1C)
#define TF_TYPE         (0x20)
#define TF_DS           (0x24)
#define TF_ES           (0x28)
#define TF_FS           (0x2C)
#define TF_GS           (0x30)
#define TF_ERROR_CODE   (0x34)
#define TF_EIP          (0x38)
#define TF_CS           (0x3C)
#define TF_EFLAGS       (0x40)
#define TF_ESP0         (0x44)
#define TF_SS0          (0x48)
#define TF_V86_ES       (0x4C)
#define TF_V86_DS       (0x50)
#define TF_V86_FS       (0x54)
#define TF_V86_GS       (0x58)
#define TF_REGS         (0x5C)
#define TF_ORIG_EBP     (0x60)

#ifndef __ASM__

struct trap_frame
{
  ULONG edi;
  ULONG esi; 
  ULONG ebp;
  ULONG esp; 
  ULONG ebx;
  ULONG edx; 
  ULONG ecx;
  ULONG eax;
  ULONG type;
  ULONG ds;
  ULONG es;
  ULONG fs;
  ULONG gs;
  ULONG error_code;
  ULONG eip;
  ULONG cs;
  ULONG eflags;
  ULONG esp0;
  ULONG ss0;
   
   /*
    * These members are only valid in v86 mode
    */
  ULONG v86_es;
  ULONG v86_ds;
  ULONG v86_fs;
  ULONG v86_gs;
  
  /*
   * These are put on the top of the stack by the routine that entered
   * v86 mode so the exception handlers can find the control information
   */
  struct _KV86M_REGISTERS* regs;     
  ULONG orig_ebp;
};

ULONG
KeV86Exception(struct trap_frame* tf, ULONG address);

#endif /* not __ASM__ */

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_TRAP_H */
