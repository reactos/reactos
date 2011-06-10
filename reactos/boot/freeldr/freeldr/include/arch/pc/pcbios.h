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
