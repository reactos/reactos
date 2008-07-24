/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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

#ifndef __AMD64_AMD64_H_
#define __AMD64_AMD64_H_

#define STACK64ADDR	0x74000	/* The 64-bit stack top will be at 0x74000 */

#define LMODE_CS	0x08
#define LMODE_DS	0x10

/* Where we put out initial page tables */
#define PML4_PAGENUM 60 // Put it high enough so it doesn't interfere with freeldr

#define PAGESIZE 4096
#define PDP_PAGENUM (PML4_PAGENUM + 1)
#define PD_PAGENUM (PDP_PAGENUM + 1)
#define PML4_ADDRESS (PML4_PAGENUM * PAGESIZE)
#define PDP_ADDRESS (PDP_PAGENUM * PAGESIZE)
#define PML4_SEG (PML4_ADDRESS / 16)
#define PML4_PAGES 3
#define PAGETABLE_SIZE PML4_PAGES * PAGESIZE

#if 0
#ifndef ASM
typedef struct
{
	unsigned long long	rax;
	unsigned long long	rbx;
	unsigned long long	rcx;
	unsigned long long	rdx;

	unsigned long long	rsi;
	unsigned long long	rdi;

	unsigned long long	r8;
	unsigned long long	r9;
	unsigned long long	r10;
	unsigned long long	r11;
	unsigned long long	r12;
	unsigned long long	r13;
	unsigned long long	r14;
	unsigned long long	r15;

	unsigned short	ds;
	unsigned short	es;
	unsigned short	fs;
	unsigned short	gs;

	unsigned long long	rflags;

} QWORDREGS;
#endif /* ! ASM */
#endif

#endif /* __AMD64_AMD64_H_ */

/* EOF */
