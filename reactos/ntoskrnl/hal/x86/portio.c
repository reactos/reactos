/* $Id: portio.c,v 1.3 2000/03/04 20:45:34 ea Exp $
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
#include <internal/halio.h>


/* FUNCTIONS ****************************************************************/

VOID
STDCALL
READ_PORT_BUFFER_UCHAR (PUCHAR Port,
                        PUCHAR Buffer,
                        ULONG Count)
{
        insb ((ULONG)Port, Buffer, Count);
}


VOID
STDCALL
READ_PORT_BUFFER_ULONG (PULONG Port,
                        PULONG Buffer,
                        ULONG Count)
{
        insl ((ULONG)Port, Buffer, Count);
}


VOID
STDCALL
READ_PORT_BUFFER_USHORT (PUSHORT Port,
                         PUSHORT Buffer,
                         ULONG Count)
{
        insw ((ULONG)Port, Buffer, Count);
}


UCHAR
STDCALL
READ_PORT_UCHAR (PUCHAR Port)
{
        return inb_p ((ULONG)Port);
}


ULONG
STDCALL
READ_PORT_ULONG (PULONG Port)
{
        return inl_p ((ULONG)Port);
}


USHORT
STDCALL
READ_PORT_USHORT (PUSHORT Port)
{
        return inw_p ((ULONG)Port);
}


VOID
STDCALL
WRITE_PORT_BUFFER_UCHAR (PUCHAR Port,
                         PUCHAR Buffer,
                         ULONG Count)
{
        outsb ((ULONG)Port, Buffer, Count);
}


VOID
STDCALL
WRITE_PORT_BUFFER_ULONG (PULONG Port,
                         PULONG Buffer,
                         ULONG Count)
{
        outsl ((ULONG)Port, Buffer, Count);
}


VOID
STDCALL
WRITE_PORT_BUFFER_USHORT (PUSHORT Port,
                          PUSHORT Buffer,
                          ULONG Count)
{
        outsw ((ULONG)Port, Buffer, Count);
}


VOID
STDCALL
WRITE_PORT_UCHAR (PUCHAR Port,
                  UCHAR Value)
{
        outb_p ((ULONG)Port, Value);
}


VOID
STDCALL
WRITE_PORT_ULONG (PULONG Port,
                  ULONG Value)
{
        outl_p ((ULONG)Port, Value);
}


VOID
STDCALL
WRITE_PORT_USHORT (PUSHORT Port,
                   USHORT Value)
{
        outw_p ((ULONG)Port, Value);
}

/* EOF */
