/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/portio.c
 * PURPOSE:         Port I/O functions
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 18/10/99
 */

#include <ddk/ntddk.h>


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

VOID STDCALL
READ_PORT_BUFFER_UCHAR (PUCHAR Port,
                        PUCHAR Buffer,
                        ULONG Count)
{
#if defined(__GNUC__)
   __asm__ __volatile__ ("cld ; rep ; insb\n\t" 
			 : "=D" (Buffer), "=c" (Count) 
			 : "d" (Port),"0" (Buffer),"1" (Count));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		mov edi, Buffer
		mov ecx, Count
		cld
		rep ins byte ptr[edi], dx
	}
#else
#error Unknown compiler for inline assembler
#endif
}

VOID STDCALL
READ_PORT_BUFFER_USHORT (PUSHORT Port,
                         PUSHORT Buffer,
                         ULONG Count)
{
#if defined(__GNUC__)
   __asm__ __volatile__ ("cld ; rep ; insw"
			 : "=D" (Buffer), "=c" (Count) 
			 : "d" (Port),"0" (Buffer),"1" (Count));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		mov edi, Buffer
		mov ecx, Count
		cld
		rep ins word ptr[edi], dx
	}
#else
#error Unknown compiler for inline assembler
#endif
}

VOID STDCALL
READ_PORT_BUFFER_ULONG (PULONG Port,
                        PULONG Buffer,
                        ULONG Count)
{
#if defined(__GNUC__)
   __asm__ __volatile__ ("cld ; rep ; insl"
			 : "=D" (Buffer), "=c" (Count) 
			 : "d" (Port),"0" (Buffer),"1" (Count));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		mov edi, Buffer
		mov ecx, Count
		cld
		rep ins dword ptr[edi], dx
	}
#else
#error Unknown compiler for inline assembler
#endif
}

UCHAR STDCALL
READ_PORT_UCHAR (PUCHAR Port)
{
   UCHAR Value;

#if defined(__GNUC__)
   __asm__("inb %w1, %0\n\t"
	   : "=a" (Value)
	   : "d" (Port));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		in al, dx
		mov Value, al
	}
#else
#error Unknown compiler for inline assembler
#endif

   SLOW_DOWN_IO;
   return(Value);
}

USHORT STDCALL
READ_PORT_USHORT (PUSHORT Port)
{
   USHORT Value;

#if defined(__GNUC__)
   __asm__("inw %w1, %0\n\t"
	   : "=a" (Value)
	   : "d" (Port));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		in ax, dx
		mov Value, ax
	}
#else
#error Unknown compiler for inline assembler
#endif
   SLOW_DOWN_IO;
   return(Value);
}

ULONG STDCALL
READ_PORT_ULONG (PULONG Port)
{
   ULONG Value;

#if defined(__GNUC__)
   __asm__("inl %w1, %0\n\t"
	   : "=a" (Value)
	   : "d" (Port));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		in eax, dx
		mov Value, eax
	}
#else
#error Unknown compiler for inline assembler
#endif
   SLOW_DOWN_IO;
   return(Value);
}

VOID STDCALL
WRITE_PORT_BUFFER_UCHAR (PUCHAR Port,
                         PUCHAR Buffer,
                         ULONG Count)
{
#if defined(__GNUC__)
   __asm__ __volatile__ ("cld ; rep ; outsb" 
			 : "=S" (Buffer), "=c" (Count) 
			 : "d" (Port),"0" (Buffer),"1" (Count));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		mov esi, Buffer
		mov ecx, Count
		cld
		rep outs
	}
#else
#error Unknown compiler for inline assembler
#endif
}

VOID STDCALL
WRITE_PORT_BUFFER_USHORT (PUSHORT Port,
                          PUSHORT Buffer,
                          ULONG Count)
{
#if defined(__GNUC__)
   __asm__ __volatile__ ("cld ; rep ; outsw"
			 : "=S" (Buffer), "=c" (Count) 
			 : "d" (Port),"0" (Buffer),"1" (Count));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		mov esi, Buffer
		mov ecx, Count
		cld
		rep outsw
	}
#else
#error Unknown compiler for inline assembler
#endif
}

VOID STDCALL
WRITE_PORT_BUFFER_ULONG (PULONG Port,
                         PULONG Buffer,
                         ULONG Count)
{
#if defined(__GNUC__)
   __asm__ __volatile__ ("cld ; rep ; outsl" 
			 : "=S" (Buffer), "=c" (Count) 
			 : "d" (Port),"0" (Buffer),"1" (Count));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		mov esi, Buffer
		mov ecx, Count
		cld
		rep outsd
	}
#else
#error Unknown compiler for inline assembler
#endif
}

VOID STDCALL
WRITE_PORT_UCHAR (PUCHAR Port,
                  UCHAR Value)
{
#if defined(__GNUC__)
   __asm__("outb %0, %w1\n\t"
	   : 
	   : "a" (Value),
	     "d" (Port));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		mov al, Value
		out dx,al
	}
#else
#error Unknown compiler for inline assembler
#endif
   SLOW_DOWN_IO;
}

VOID STDCALL
WRITE_PORT_USHORT (PUSHORT Port,
                   USHORT Value)
{
#if defined(__GNUC__)
   __asm__("outw %0, %w1\n\t"
	   : 
	   : "a" (Value),
	     "d" (Port));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		mov ax, Value
		out dx,ax
	}
#else
#error Unknown compiler for inline assembler
#endif
   SLOW_DOWN_IO;
}

VOID STDCALL
WRITE_PORT_ULONG (PULONG Port,
                  ULONG Value)
{
#if defined(__GNUC__)
   __asm__("outl %0, %w1\n\t"
	   : 
	   : "a" (Value),
	     "d" (Port));
#elif defined(_MSC_VER)
	__asm
	{
		mov edx, Port
		mov eax, Value
		out dx,eax
	}
#else
#error Unknown compiler for inline assembler
#endif
   SLOW_DOWN_IO;
}

/* EOF */
