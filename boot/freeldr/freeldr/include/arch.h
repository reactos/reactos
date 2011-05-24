/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __ASM__
#pragma once
#endif

#ifndef HEX
#define HEX(y) 0x##y
#endif

#ifdef _M_AMD64
#include <arch/amd64/amd64.h>
#endif

#if defined (_M_IX86)
/* Defines needed for switching between real and protected mode */
#define NULL_DESC	HEX(00)	/* NULL descriptor */
#define PMODE_CS	HEX(08)	/* PMode code selector, base 0 limit 4g */
#define PMODE_DS	HEX(10)	/* PMode data selector, base 0 limit 4g */
#define RMODE_CS	HEX(18)	/* RMode code selector, base 0 limit 64k */
#define RMODE_DS	HEX(20)	/* RMode data selector, base 0 limit 64k */
#endif

#define CR0_PE_SET	HEX(00000001)	/* OR this value with CR0 to enable pmode */
#define CR0_PE_CLR	HEX(FFFFFFFE)	/* AND this value with CR0 to disable pmode */

#define STACK16ADDR	HEX(7000)	/* The 16-bit stack top will be at 0000:7000 */
#define STACK32ADDR	HEX(78000)	/* The 32-bit stack top will be at 7000:8000, or 0x78000 */

#if defined (_M_IX86) || defined (_M_AMD64)
#define BIOSCALLBUFFER		0x78000	/* Buffer to store temporary data for any Int386() call */
#define BIOSCALLBUFSEGMENT	0x7800	/* Buffer to store temporary data for any Int386() call */
#define BIOSCALLBUFOFFSET	0x0000	/* Buffer to store temporary data for any Int386() call */
#define FILESYSBUFFER		0x80000	/* Buffer to store file system data (e.g. cluster buffer for FAT) */
#define DISKREADBUFFER		0x90000	/* Buffer to store data read in from the disk via the BIOS */
#define DISKREADBUFFER_SIZE 512
#elif defined(_M_PPC) || defined(_M_MIPS)
#define DISKREADBUFFER		    0x80000000
#define FILESYSBUFFER           0x80000000
#elif defined(_M_ARM)
extern ULONG gDiskReadBuffer, gFileSysBuffer;
#define DISKREADBUFFER gDiskReadBuffer
#define FILESYSBUFFER  gFileSysBuffer
#endif

/* Makes "x" a global variable or label */
#define EXTERN(x)	.global x; x:


// Flag Masks
#define I386FLAG_CF		HEX(0001)		// Carry Flag
#define I386FLAG_RESV1	HEX(0002)		// Reserved - Must be 1
#define I386FLAG_PF		HEX(0004)		// Parity Flag
#define I386FLAG_RESV2	HEX(0008)		// Reserved - Must be 0
#define I386FLAG_AF		HEX(0010)		// Auxiliary Flag
#define I386FLAG_RESV3	HEX(0020)		// Reserved - Must be 0
#define I386FLAG_ZF		HEX(0040)		// Zero Flag
#define I386FLAG_SF		HEX(0080)		// Sign Flag
#define I386FLAG_TF		HEX(0100)		// Trap Flag (Single Step)
#define I386FLAG_IF		HEX(0200)		// Interrupt Flag
#define I386FLAG_DF		HEX(0400)		// Direction Flag
#define I386FLAG_OF		HEX(0800)		// Overflow Flag


#ifndef __ASM__

#include <pshpack1.h>
typedef struct
{
	unsigned long	eax;
	unsigned long	ebx;
	unsigned long	ecx;
	unsigned long	edx;

	unsigned long	esi;
	unsigned long	edi;

	unsigned short	ds;
	unsigned short	es;
	unsigned short	fs;
	unsigned short	gs;

	unsigned long	eflags;

} DWORDREGS;

typedef struct
{
	unsigned short	ax, _upper_ax;
	unsigned short	bx, _upper_bx;
	unsigned short	cx, _upper_cx;
	unsigned short	dx, _upper_dx;

	unsigned short	si, _upper_si;
	unsigned short	di, _upper_di;

	unsigned short	ds;
	unsigned short	es;
	unsigned short	fs;
	unsigned short	gs;

	unsigned short	flags, _upper_flags;

} WORDREGS;

typedef struct
{
	unsigned char	al;
	unsigned char	ah;
	unsigned short	_upper_ax;
	unsigned char	bl;
	unsigned char	bh;
	unsigned short	_upper_bx;
	unsigned char	cl;
	unsigned char	ch;
	unsigned short	_upper_cx;
	unsigned char	dl;
	unsigned char	dh;
	unsigned short	_upper_dx;

	unsigned short	si, _upper_si;
	unsigned short	di, _upper_di;

	unsigned short	ds;
	unsigned short	es;
	unsigned short	fs;
	unsigned short	gs;

	unsigned short	flags, _upper_flags;

} BYTEREGS;


typedef union
{
	DWORDREGS	x;
	DWORDREGS	d;
	WORDREGS	w;
	BYTEREGS	b;
} REGS;
#include <poppack.h>

// Int386()
//
// Real mode interrupt vector interface
//
// (E)FLAGS can *only* be returned by this function, not set.
// Make sure all memory pointers are in SEG:OFFS format and
// not linear addresses, unless the interrupt handler
// specifically handles linear addresses.
int		Int386(int ivec, REGS* in, REGS* out);

// This macro tests the Carry Flag
// If CF is set then the call failed (usually)
#define INT386_SUCCESS(regs)	((regs.x.eflags & I386FLAG_CF) == 0)

void	EnableA20(void);

VOID	ChainLoadBiosBootSectorCode(VOID);	// Implemented in boot.S
VOID	SoftReboot(VOID);					// Implemented in boot.S

VOID	DetectHardware(VOID);		// Implemented in hardware.c

#endif /* ! __ASM__ */
