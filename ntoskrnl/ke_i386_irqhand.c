/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  Moved to MSVC-compatible inline assembler by Mike Nordell, 2003-12-26
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
 * FILE:            ntoskrnl/ke/i386/vm86_sup.S
 * PURPOSE:         V86 mode support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 09/10/00
 */

/* INCLUDES ******************************************************************/

#pragma hdrstop

#include <ddk/ntddk.h>
#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/i386/fpu.h>
#include <internal/ps.h>
#include <ddk/defines.h>
#include <internal/v86m.h>
#include <ntos/tss.h>
#include <internal/trap.h>
#include <internal/ps.h>

#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>

// no arg-list, but asm doesn't care anyway
void KiInterruptDispatch();


#define DEFINE_INT_HANDLER(N)			\
__declspec(naked)						\
void irq_handler_##N()					\
{										\
	__asm	pushad						\
	__asm	push	ds					\
	__asm	push	es					\
	__asm	push	fs					\
	__asm	mov		eax, 0xceafbeef		\
	__asm	push	eax					\
	__asm	mov		ax, KERNEL_DS		\
	__asm	mov		ds, ax				\
	__asm	mov		es, ax				\
	__asm	mov		ax, PCR_SELECTOR	\
	__asm	mov		fs, ax				\
	__asm	push	esp					\
	__asm	push	N					\
	__asm	call	KiInterruptDispatch	\
	__asm	pop		eax					\
	__asm	pop		eax					\
	__asm	pop		eax					\
	__asm	pop		fs					\
	__asm	pop		es					\
	__asm	pop		ds					\
	__asm	popad						\
	__asm	iretd						
// NOTE: The inline assembler can't deal with having the final brace,
// ending the function, on the same line as an __asm, why there is
// none here and it MUST be added when using the macro!

DEFINE_INT_HANDLER(0)
}
DEFINE_INT_HANDLER(1)
}
DEFINE_INT_HANDLER(2)
}
DEFINE_INT_HANDLER(3)
}
DEFINE_INT_HANDLER(4)
}
DEFINE_INT_HANDLER(5)
}
DEFINE_INT_HANDLER(6)
}
DEFINE_INT_HANDLER(7)
}
DEFINE_INT_HANDLER(8)
}
DEFINE_INT_HANDLER(9)
}
DEFINE_INT_HANDLER(10)
}
DEFINE_INT_HANDLER(11)
}
DEFINE_INT_HANDLER(12)
}
DEFINE_INT_HANDLER(13)
}
DEFINE_INT_HANDLER(14)
}
DEFINE_INT_HANDLER(15)
}

