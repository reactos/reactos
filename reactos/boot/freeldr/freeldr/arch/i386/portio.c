/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/portio.c
 * PURPOSE:         Port I/O functions
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  Created 18/10/99
 */

//#include <ntddk.h>
#include <freeldr.h>


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

#ifdef SLOW_IO_BY_JUMPING
#define __SLOW_DOWN_IO __asm__ __volatile__("jmp 1f\n1:\tjmp 1f\n1:")
#else
#define __SLOW_DOWN_IO __asm__ __volatile__("outb %al,$0x80")
#endif

#ifdef REALLY_SLOW_IO
#define SLOW_DOWN_IO { __SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; __SLOW_DOWN_IO; }
#else
#define SLOW_DOWN_IO __SLOW_DOWN_IO
#endif

#undef READ_PORT_BUFFER_UCHAR
NTHALAPI
VOID
DDKAPI
READ_PORT_BUFFER_UCHAR (PUCHAR Port,
                        PUCHAR Buffer,
                        ULONG Count)
{
   __asm__ __volatile__ ("cld ; rep ; insb\n\t"
			 : "=D" (Buffer), "=c" (Count)
			 : "d" (Port),"0" (Buffer),"1" (Count));
}

#undef READ_PORT_BUFFER_USHORT
NTHALAPI
VOID
DDKAPI
READ_PORT_BUFFER_USHORT (USHORT* Port,
                         USHORT* Buffer,
                         ULONG Count)
{
   __asm__ __volatile__ ("cld ; rep ; insw"
			 : "=D" (Buffer), "=c" (Count)
			 : "d" (Port),"0" (Buffer),"1" (Count));
}

#undef READ_PORT_BUFFER_ULONG
NTHALAPI
VOID
DDKAPI
READ_PORT_BUFFER_ULONG (ULONG* Port,
                        ULONG* Buffer,
                        ULONG Count)
{
   __asm__ __volatile__ ("cld ; rep ; insl"
			 : "=D" (Buffer), "=c" (Count)
			 : "d" (Port),"0" (Buffer),"1" (Count));
}

#undef READ_PORT_UCHAR
NTHALAPI
UCHAR
DDKAPI
READ_PORT_UCHAR (PUCHAR Port)
{
   UCHAR Value;

   __asm__("inb %w1, %0\n\t"
	   : "=a" (Value)
	   : "d" (Port));
   SLOW_DOWN_IO;
   return(Value);
}

#undef READ_PORT_USHORT
NTHALAPI
USHORT
DDKAPI
READ_PORT_USHORT (USHORT* Port)
{
   USHORT Value;

   __asm__("inw %w1, %0\n\t"
	   : "=a" (Value)
	   : "d" (Port));
   SLOW_DOWN_IO;
   return(Value);
}

#undef READ_PORT_ULONG
NTHALAPI
ULONG
DDKAPI
READ_PORT_ULONG (ULONG* Port)
{
   ULONG Value;

   __asm__("inl %w1, %0\n\t"
	   : "=a" (Value)
	   : "d" (Port));
   SLOW_DOWN_IO;
   return(Value);
}

#undef WRITE_PORT_BUFFER_UCHAR
NTHALAPI
VOID
DDKAPI
WRITE_PORT_BUFFER_UCHAR (PUCHAR Port,
                         PUCHAR Buffer,
                         ULONG Count)
{
   __asm__ __volatile__ ("cld ; rep ; outsb"
			 : "=S" (Buffer), "=c" (Count)
			 : "d" (Port),"0" (Buffer),"1" (Count));
}

#undef WRITE_PORT_BUFFER_USHORT
NTHALAPI
VOID
DDKAPI
WRITE_PORT_BUFFER_USHORT (USHORT* Port,
                          USHORT* Buffer,
                          ULONG Count)
{
   __asm__ __volatile__ ("cld ; rep ; outsw"
			 : "=S" (Buffer), "=c" (Count)
			 : "d" (Port),"0" (Buffer),"1" (Count));
}

#undef WRITE_PORT_BUFFER_ULONG
NTHALAPI
VOID
DDKAPI
WRITE_PORT_BUFFER_ULONG (ULONG* Port,
                         ULONG* Buffer,
                         ULONG Count)
{
   __asm__ __volatile__ ("cld ; rep ; outsl"
			 : "=S" (Buffer), "=c" (Count)
			 : "d" (Port),"0" (Buffer),"1" (Count));
}

#undef WRITE_PORT_UCHAR
NTHALAPI
VOID
DDKAPI
WRITE_PORT_UCHAR (PUCHAR Port,
                  UCHAR Value)
{
   __asm__("outb %0, %w1\n\t"
	   :
	   : "a" (Value),
	     "d" (Port));
   SLOW_DOWN_IO;
}

#undef WRITE_PORT_USHORT
NTHALAPI
VOID
DDKAPI
WRITE_PORT_USHORT (USHORT* Port,
                   USHORT Value)
{
   __asm__("outw %0, %w1\n\t"
	   :
	   : "a" (Value),
	     "d" (Port));
   SLOW_DOWN_IO;
}

#undef WRITE_PORT_ULONG
NTHALAPI
VOID
DDKAPI
WRITE_PORT_ULONG (ULONG* Port,
                  ULONG Value)
{
   __asm__("outl %0, %w1\n\t"
	   :
	   : "a" (Value),
	     "d" (Port));
   SLOW_DOWN_IO;
}

/* EOF */
