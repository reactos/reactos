/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_PS_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_PS_H

/*
 * Defines for accessing KPCR and KTHREAD structure members
 */
#define KTHREAD_INITIAL_STACK     0x18
#define KTHREAD_STACK_LIMIT       0x1C
#define KTHREAD_TEB               0x20
#define KTHREAD_KERNEL_STACK      0x28
#define KTHREAD_PREVIOUS_MODE     0x13B
#define KTHREAD_TRAP_FRAME        0x12C
#define KTHREAD_CALLBACK_STACK    0x124

#define ETHREAD_THREADS_PROCESS   0x26C

#define KPROCESS_DIRECTORY_TABLE_BASE 0x18

#define KPCR_BASE                 0xFF000000

#define KPCR_EXCEPTION_LIST       0x0
#define KPCR_SELF                 0x18
#define KPCR_TSS                  0x3C
#define KPCR_CURRENT_THREAD       0x5C

#ifndef AS_INVOKED

/*
 * Processor Control Region
 */
typedef struct _IKPCR
{
  KPCR KPCR;                      /* 00 */
  UCHAR Reserved[0x0C];           /* 50 */
  struct _KTHREAD* CurrentThread; /* 5C */
} IKPCR, *PIKPCR;                 /* 60 */

VOID
Ki386ContextSwitch(struct _KTHREAD* NewThread, 
		   struct _KTHREAD* OldThread);
NTSTATUS 
Ke386InitThread(struct _KTHREAD* Thread, PKSTART_ROUTINE fn, 
		PVOID StartContext);
NTSTATUS 
Ke386InitThreadWithContext(struct _KTHREAD* Thread, PCONTEXT Context);
NTSTATUS
Ki386ValidateUserContext(PCONTEXT Context);

#endif /* AS_INVOKED */

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_PS_H */

/* EOF */
