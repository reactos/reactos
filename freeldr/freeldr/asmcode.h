/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000  Brian Palmer  <brianp@sginet.com>
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
	

/* Defines needed for switching between real and protected mode */
#define NULL_DESC	0x00	/* NULL descriptor */
#define PMODE_CS	0x08	/* PMode code selector, base 0 limit 4g */
#define PMODE_DS	0x10	/* PMode data selector, base 0 limit 4g */
#define RMODE_CS	0x18	/* RMode code selector, base 0 limit 64k */
#define RMODE_DS	0x20	/* RMode data selector, base 0 limit 64k */

#define	KERNEL_BASE	0xC0000000
//#define	USER_CS		0x08
//#define	USER_DS		0x10
//#define	KERNEL_CS	0x20
//#define	KERNEL_DS	0x28
#define	KERNEL_CS	0x08
#define	KERNEL_DS	0x10

#define CR0_PE_SET	0x00000001	/* OR this value with CR0 to enable pmode */
#define CR0_PE_CLR	0xFFFFFFFE	/* AND this value with CR0 to disable pmode */

#define	NR_TASKS	128		/* Space reserved in the GDT for TSS descriptors */

#define STACK16ADDR	0x7000	/* The 16-bit stack top will be at 0000:7000 */
#define STACK32ADDR	0x60000	/* The 32-bit stack top will be at 6000:0000, or 0x60000 */

#define FILESYSADDR	0x80000	/* The filesystem data address will be at 8000:0000, or 0x80000 */

#define FATCLUSTERBUF	0x60000	/* The fat filesystem's cluster buffer */

#define SCREENBUFFER	0x68000	/* The screen contents will be saved here */
#define FREELDRINIADDR	0x6C000	/* The freeldr.ini load address will be at 6000:C000, or 0x6C000 */

#define	SCRATCHSEG	0x7000	/* The 512-byte fixed scratch area will be at 7000:0000, or 0x70000 */
#define	SCRATCHOFF	0x0000	/* The 512-byte fixed scratch area will be at 7000:0000, or 0x70000 */
#define	SCRATCHAREA	0x70000	/* The 512-byte fixed scratch area will be at 7000:0000, or 0x70000 */

/* Makes "x" a global variable or label */
#define EXTERN(x)	.global x; x:




#ifndef ASM

void	enable_a20(void);

#endif /* ! ASM */
