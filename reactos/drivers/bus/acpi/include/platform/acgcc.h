/******************************************************************************
 *
 * Name: acgcc.h - GCC specific defines, etc.
 *       $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 R. Byron Moore
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ACGCC_H__
#define __ACGCC_H__


#ifdef __ia64__
#define _IA64

#define COMPILER_DEPENDENT_UINT64   unsigned long
/* Single threaded */
#define ACPI_APPLICATION

#define ACPI_ASM_MACROS
#define causeinterrupt(level)
#define BREAKPOINT3
#define disable() __cli()
#define enable()  __sti()
#define wbinvd()

/*! [Begin] no source code translation */

#include <asm/pal.h>

#define halt()              ia64_pal_halt_light()           /* PAL_HALT[_LIGHT] */
#define safe_halt()         ia64_pal_halt(1)                /* PAL_HALT */


#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq) \
	do { \
	__asm__ volatile ("1:  ld4      r29=%1\n"  \
		";;\n"                  \
		"mov    ar.ccv=r29\n"   \
		"mov    r2=r29\n"       \
		"shr.u  r30=r29,1\n"    \
		"and    r29=-4,r29\n"   \
		";;\n"                  \
		"add    r29=2,r29\n"    \
		"and    r30=1,r30\n"    \
		";;\n"                  \
		"add    r29=r29,r30\n"  \
		";;\n"                  \
		"cmpxchg4.acq   r30=%1,r29,ar.ccv\n" \
		";;\n"                  \
		"cmp.eq p6,p7=r2,r30\n" \
		"(p7) br.dpnt.few 1b\n" \
		"cmp.gt p8,p9=3,r29\n"  \
		";;\n"                  \
		"(p8) mov %0=-1\n"      \
		"(p9) mov %0=r0\n"      \
		:"=r"(Acq):"m"(GLptr):"r2","r29","r30","memory"); \
	} while (0)

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Acq) \
	do { \
	__asm__ volatile ("1:  ld4      r29=%1\n" \
		";;\n"                  \
		"mov    ar.ccv=r29\n"   \
		"mov    r2=r29\n"       \
		"and    r29=-4,r29\n"   \
		";;\n"                  \
		"cmpxchg4.acq   r30=%1,r29,ar.ccv\n" \
		";;\n"                  \
		"cmp.eq p6,p7=r2,r30\n" \
		"(p7) br.dpnt.few 1b\n" \
		"and    %0=1,r2\n"      \
		";;\n"                  \
		:"=r"(Acq):"m"(GLptr):"r2","r29","r30","memory"); \
	} while (0)
/*! [End] no source code translation !*/


#else /* DO IA32 */
#define COMPILER_DEPENDENT_UINT64   unsigned long long
#define ACPI_ASM_MACROS
#define causeinterrupt(level)
#define BREAKPOINT3
#define disable() __cli()
#define enable()  __sti()
#define halt()    __asm__ __volatile__ ("sti; hlt":::"memory")
#define wbinvd()

/*! [Begin] no source code translation
 *
 * A brief explanation as GNU inline assembly is a bit hairy
 *  %0 is the output parameter in EAX ("=a")
 *  %1 and %2 are the input parameters in ECX ("c")
 *  and an immediate value ("i") respectively
 *  All actual register references are preceded with "%%" as in "%%edx"
 *  Immediate values in the assembly are preceded by "$" as in "$0x1"
 *  The final asm parameter are the operation altered non-output registers.
 */
#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq) \
	do { \
		int dummy; \
		asm("1:     movl (%1),%%eax;" \
			"movl   %%eax,%%edx;" \
			"andl   %2,%%edx;" \
			"btsl   $0x1,%%edx;" \
			"adcl   $0x0,%%edx;" \
			"lock;  cmpxchgl %%edx,(%1);" \
			"jnz    1b;" \
			"cmpb   $0x3,%%dl;" \
			"sbbl   %%eax,%%eax" \
			:"=a"(Acq),"=c"(dummy):"c"(GLptr),"i"(~1L):"dx"); \
	} while(0)

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Acq) \
	do { \
		int dummy; \
		asm("1:     movl (%1),%%eax;" \
			"movl   %%eax,%%edx;" \
			"andl   %2,%%edx;" \
			"lock;  cmpxchgl %%edx,(%1);" \
			"jnz    1b;" \
			"andl   $0x1,%%eax" \
			:"=a"(Acq),"=c"(dummy):"c"(GLptr),"i"(~3L):"dx"); \
	} while(0)

/*! [End] no source code translation !*/

#endif /* IA 32 */

#endif /* __ACGCC_H__ */
