/***
* vmemory.h - Virtual Memory (VM) Management Routines
*
*	Copyright (c) 1989-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This include file provides prototypes and definitions for
*	the virtual memory management routines.
*
*******************************************************************************/

#ifndef _INC_VMEMORY

#if (_MSC_VER <= 600)
#define __far       _far
#define __pascal    _pascal
#endif

/* virtual memory handle type */
typedef unsigned long _vmhnd_t;

/* null handle value */
#define _VM_NULL	((_vmhnd_t) 0)

/* use all available DOS memory for virtual heap */
#define _VM_ALLDOS	0

/* swap areas */
#define _VM_EMS 	1
#define _VM_XMS 	2
#define _VM_DISK	4
#define _VM_ALLSWAP	(_VM_EMS | _VM_XMS | _VM_DISK)

/* clean/dirty flags */
#define _VM_CLEAN	0
#define _VM_DIRTY	1

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void __far __pascal _vfree(_vmhnd_t);
int __far __pascal _vheapinit(unsigned int, unsigned int, unsigned int);
void __far __pascal _vheapterm(void);
void __far * __far __pascal _vload(_vmhnd_t, int);
void __far * __far __pascal _vlock(_vmhnd_t);
unsigned int __far __pascal _vlockcnt(_vmhnd_t);
_vmhnd_t __far __pascal _vmalloc(unsigned long);
unsigned long __far __pascal _vmsize(_vmhnd_t);
_vmhnd_t __far __pascal _vrealloc(_vmhnd_t , unsigned long);
void __far __pascal _vunlock(_vmhnd_t, int);

#ifdef __cplusplus
}
#endif

#define _INC_VMEMORY
#endif /* _INC_VMEMORY */
