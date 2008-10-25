/* $Id: portio.c 23907 2006-09-04 05:52:23Z arty $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/portio.c
 * PURPOSE:         Port I/O functions
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 18/10/99
 */

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * This file contains the definitions for the x86 IO instructions
 * inb/inw/inl/outb/outw/outl and the "string versions" of the same
 * (insb/insw/insl/outsb/outsw/outsl). You can also use "pausing"
 * versions of the single-IO instructions (inb_p/inw_p/..).
 *
 * This file is not meant to be obfuscating: it's just complicated
 * to (a) handle it all in a way that makes gcc able to optimize it
 * as well as possible and (b) trying to avoid writing the same thing
 * over and over again with slight variations and possibly making a
 * mistake somewhere.
 */

/*
 * Thanks to James van Artsdalen for a better timing-fix than
 * the two short jumps: using outb's to a nonexistent port seems
 * to guarantee better timings even on fast machines.
 *
 * On the other hand, I'd like to be sure of a non-existent port:
 * I feel a bit unsafe about using 0x80 (should be safe, though)
 *
 *		Linus
 */

#if defined(__GNUC__)

#ifdef SLOW_IO_BY_JUMPING
#define __SLOW_DOWN_IO __asm__ __volatile__("jmp 1f\n1:\tjmp 1f\n1:")
#else
#define __SLOW_DOWN_IO __asm__ __volatile__("outb %al,$0x80")
#endif

#elif defined(_MSC_VER)

#ifdef SLOW_IO_BY_JUMPING
#define __SLOW_DOWN_IO __asm jmp 1f  __asm jmp 1f  1f:
#else
#define __SLOW_DOWN_IO __asm out 0x80, al
#endif

#else
#error Unknown compiler for inline assembler
#endif


#ifdef REALLY_SLOW_IO
#define SLOW_DOWN_IO { __SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; }
#else
#define SLOW_DOWN_IO __SLOW_DOWN_IO
#endif

extern int GetPhysByte(int Addr);
extern void SetPhysByte(int Addr, int Val);
extern int GetPhysWord(int Addr);
extern void SetPhysWord(int Addr, int Val);
extern int GetPhys(int Addr);
extern void SetPhys(int Addr, int Val);

__asm__("\t.globl GetPhys\n"
	"GetPhys:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   3,0(3)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl GetPhysWord\n"
	"GetPhysWord:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lhz   3,0(3)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl GetPhysByte\n"
	"GetPhysByte:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lbz   3,0(3)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl SetPhys\n"
	"SetPhys:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"sync\n\t"
	"eieio\n\t"
	"stw   4,0(3)\n\t"    /* Set actual value at phys addr r3 */
	"dcbst 0,3\n\t"
	"mtmsr 5\n\t"
	"sync\n\t"
	"eieio\n\t"
	"mr    3,4\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl SetPhysWord\n"
	"SetPhysWord:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"sync\n\t"
	"eieio\n\t"
	"sth   4,0(3)\n\t"    /* Set actual value at phys addr r3 */
	"dcbst 0,3\n\t"
	"mtmsr 5\n\t"
	"sync\n\t"
	"eieio\n\t"
	"mr    3,4\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl SetPhysByte\n"
	"SetPhysByte:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"sync\n\t"
	"eieio\n\t"
	"stb   4,0(3)\n\t"    /* Set actual value at phys addr r3 */
	"dcbst 0,3\n\t"
	"mtmsr 5\n\t"
	"sync\n\t"
	"eieio\n\t"
	"mr    3,4\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

VOID STDCALL
READ_PORT_BUFFER_UCHAR (PUCHAR Port,
                        PUCHAR Buffer,
                        ULONG Count)
{
    while(Count--) { *Buffer++ = GetPhysByte((ULONG)Port); }
}

VOID STDCALL
READ_PORT_BUFFER_USHORT (PUSHORT Port,
                         PUSHORT Buffer,
                         ULONG Count)
{
    while(Count--) { *Buffer++ = GetPhysWord((ULONG)Port); }
}

VOID STDCALL
READ_PORT_BUFFER_ULONG (PULONG Port,
                        PULONG Buffer,
                        ULONG Count)
{
    while(Count--) { *Buffer++ = GetPhys((ULONG)Port); }
}

UCHAR STDCALL
READ_PORT_UCHAR (PUCHAR Port)
{
    return GetPhys((ULONG)Port);
}

USHORT STDCALL
READ_PORT_USHORT (PUSHORT Port)
{
    return GetPhysWord((ULONG)Port);
}

ULONG STDCALL
READ_PORT_ULONG (PULONG Port)
{
    return GetPhys((ULONG)Port);
}

VOID STDCALL
WRITE_PORT_BUFFER_UCHAR (PUCHAR Port,
                         PUCHAR Buffer,
                         ULONG Count)
{
    while(Count--) { SetPhysByte((ULONG)Port, *Buffer++); }
}

VOID STDCALL
WRITE_PORT_BUFFER_USHORT (PUSHORT Port,
                          PUSHORT Buffer,
                          ULONG Count)
{
    while(Count--) { SetPhysWord((ULONG)Port, *Buffer++); }
}

VOID STDCALL
WRITE_PORT_BUFFER_ULONG (PULONG Port,
                         PULONG Buffer,
                         ULONG Count)
{
    while(Count--) { SetPhys((ULONG)Port, *Buffer++); }
}

VOID STDCALL
WRITE_PORT_UCHAR (PUCHAR Port,
                  UCHAR Value)
{
    SetPhysByte((ULONG)Port, Value);
}

VOID STDCALL
WRITE_PORT_USHORT (PUSHORT Port,
                   USHORT Value)
{
    SetPhysWord((ULONG)Port, Value);
}

VOID STDCALL
WRITE_PORT_ULONG (PULONG Port,
                  ULONG Value)
{
    SetPhys((ULONG)Port, Value);
}

/* EOF */
