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
/* $Id: bthread.S,v 1.1 2000/12/10 23:42:00 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/bthread.s
 * PURPOSE:         Trap handlers
 * PROGRAMMER:      David Welch (david.welch@seh.ox.ac.uk)
 * UPDATE HISTORY:
 *                  ???
 */

#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <ddk/defines.h>

/*
 *
 */

.globl _PsBeginThreadWithContextInternal

_PsBeginThreadWithContextInternal:
     call _PiBeforeBeginThread
     popl %eax
     popl %eax
     popl %eax
     popl %eax
     popl %eax
     popl %eax
     popl %eax
     addl $112,%esp
     popl %gs
;     popl %fs
     movl $TEB_SELECTOR, %ax
     movl %ax, %fs
     popl %es
     popl %ds
     popl %edi
     popl %esi
     popl %ebx
     popl %edx
     popl %ecx
     popl %eax
     popl %ebp
     iret
