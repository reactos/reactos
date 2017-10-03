/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/portio.c
 * PURPOSE:         I/O Functions for access to ports
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#undef READ_PORT_UCHAR
#undef READ_PORT_USHORT
#undef READ_PORT_ULONG
#undef WRITE_PORT_UCHAR
#undef WRITE_PORT_USHORT
#undef WRITE_PORT_ULONG

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
READ_PORT_BUFFER_UCHAR(IN PUCHAR Port,
                       OUT PUCHAR Buffer,
                       IN ULONG Count)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
READ_PORT_BUFFER_USHORT(IN PUSHORT Port,
                        OUT PUSHORT Buffer,
                        IN ULONG Count)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
READ_PORT_BUFFER_ULONG(IN PULONG Port,
                       OUT PULONG Buffer,
                       IN ULONG Count)
{
    UNIMPLEMENTED;
    while (TRUE);
}

UCHAR
NTAPI
READ_PORT_UCHAR(IN PUCHAR Port)
{
    return READ_REGISTER_UCHAR(Port);
}

USHORT
NTAPI
READ_PORT_USHORT(IN PUSHORT Port)
{
    return READ_REGISTER_USHORT(Port);
}

ULONG
NTAPI
READ_PORT_ULONG(IN PULONG Port)
{
    return READ_REGISTER_ULONG(Port);
}

VOID
NTAPI
WRITE_PORT_BUFFER_UCHAR(IN PUCHAR Port,
                        IN PUCHAR Buffer,
                        IN ULONG Count)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
WRITE_PORT_BUFFER_USHORT(IN PUSHORT Port,
                         IN PUSHORT Buffer,
                         IN ULONG Count)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
WRITE_PORT_BUFFER_ULONG(IN PULONG Port,
                        IN PULONG Buffer,
                        IN ULONG Count)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
WRITE_PORT_UCHAR(IN PUCHAR Port,
                 IN UCHAR Value)
{
    WRITE_REGISTER_UCHAR(Port, Value);
}

VOID
NTAPI
WRITE_PORT_USHORT(IN PUSHORT Port,
                  IN USHORT Value)
{
    WRITE_REGISTER_USHORT(Port, Value);
}

VOID
NTAPI
WRITE_PORT_ULONG(IN PULONG Port,
                 IN ULONG Value)
{
    WRITE_REGISTER_ULONG(Port, Value);
}

/* EOF */
