/* $Id: stdlib.h,v 1.5 2003/01/05 18:27:20 robd Exp $
 */
/*
 * psx/stdlib.h
 *
 * internal stdlib.h
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __PSX_STDLIB_H_INCLUDED__
#define __PSX_STDLIB_H_INCLUDED__

/* INCLUDES */
#include <ddk/ntddk.h>
#include <ntos/heap.h>
#include <stdlib.h>

/* OBJECTS */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */
/* FIXME? Windows NT's ntdll doesn't export RtlGetProcessHeap() */
//#define RtlGetProcessHeap() ((HANDLE)NtCurrentPeb()->ProcessHeap)
#ifndef _RTLGETPROCESSHEAP_DEFINED_
#define _RTLGETPROCESSHEAP_DEFINED_
#define RtlGetProcessHeap() (NtCurrentPeb()->ProcessHeap)
#endif

#define __malloc(SIZE)      (RtlAllocateHeap(RtlGetProcessHeap(), 0, (SIZE)))
#define __realloc(PTR,SIZE) (RtlReAllocateHeap(RtlGetProcessHeap(), 0, (PTR), (SIZE)))
#define __free(PTR)         (RtlFreeHeap(RtlGetProcessHeap(), 0, (PTR)))

#endif /* __PSX_STDLIB_H_INCLUDED__ */

/* EOF */

