/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    debug.c

Abstract:

    debug output

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    04-Feb-1999:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#ifdef DEBUG
#include "remods.h"

#include "precomp.h"
#include <asm/io.h>
#include <stdarg.h>
#include "serial.h" 
#include "serial_port.h"

#define STANDARD_DEBUG_PREFIX "pICE: "

////////////////////////////////////////////////////
// GLOBALS
////
LONG lDebugLevel = 0;
ULONG ulDebugFlags;
char tempDebug[2048];
USHORT usDebugPortBase;

////////////////////////////////////////////////////
// FUNCTIONS
////
void DebugSendString(LPSTR s);


//************************************************************************* 
// Pice_dprintf() 
// 
// internal debug print
//************************************************************************* 
VOID Pice_dprintf(ULONG DebugLevel, PCHAR DebugMessage, ...)
{	
	va_list         ap;

	va_start(ap, DebugMessage);
	if (DebugLevel <= lDebugLevel)
	{
        save_flags(ulDebugFlags);
        cli();
		PICE_vsprintf(tempDebug, DebugMessage, ap);
        DebugSendString(tempDebug);
        restore_flags(ulDebugFlags);
	}
	va_end(ap);
}

//************************************************************************
// SendByte()
//
// Output a character to the serial port 
//************************************************************************
BOOLEAN DebugSendByte(UCHAR x)
{
    ULONG timeout;

    timeout = 0x00FFFFL;

    // Wait for transmitter to clear 
    while ((inportb((USHORT)(usDebugPortBase + LSR)) & XMTRDY) == 0)
        if (!(--timeout))
        {
			return FALSE;
        }

    outportb((USHORT)(usDebugPortBase + TXR), x);

	return TRUE;
}

///************************************************************************
// DebugSetSpeed()
//
///************************************************************************
void DebugSendString(LPSTR s)
{
    ULONG len = PICE_strlen(s),i;

    for(i=0;i<len;i++)
    {
        DebugSendByte(s[i]);
    }
    DebugSendByte('\r');
}

///************************************************************************
// DebugSetSpeed()
//
///************************************************************************
void DebugSetSpeed(ULONG baudrate)
{
    UCHAR c;
    ULONG divisor;

    divisor = (ULONG) (115200L/baudrate);

    c = inportb((USHORT)(usDebugPortBase + LCR));
    outportb((USHORT)(usDebugPortBase + LCR), (UCHAR)(c | 0x80)); // Set DLAB 
    outportb((USHORT)(usDebugPortBase + DLL), (UCHAR)(divisor & 0x00FF));
    outportb((USHORT)(usDebugPortBase + DLH), (UCHAR)((divisor >> 8) & 0x00FF));
    outportb((USHORT)(usDebugPortBase + LCR), c);          // Reset DLAB 

}

///************************************************************************
// DebugSetOthers()
//
// Set other communications parameters 
//************************************************************************
void DebugSetOthers(ULONG Parity, ULONG Bits, ULONG StopBit)
{
    ULONG setting;
    UCHAR c;

    if (usDebugPortBase == 0)					return ;
    if (Bits < 5 || Bits > 8)				return ;
    if (StopBit != 1 && StopBit != 2)			return ;
    if (Parity != NO_PARITY && Parity != ODD_PARITY && Parity != EVEN_PARITY)
							return;

    setting  = Bits-5;
    setting |= ((StopBit == 1) ? 0x00 : 0x04);
    setting |= Parity;

    c = inportb((USHORT)(usDebugPortBase + LCR));
    outportb((USHORT)(usDebugPortBase + LCR), (UCHAR)(c & ~0x80)); // Reset DLAB 

    // no ints
    outportb((USHORT)(usDebugPortBase + IER), (UCHAR)0);

    outportb((USHORT)(usDebugPortBase + FCR), (UCHAR)0);

    outportb((USHORT)(usDebugPortBase + LCR), (UCHAR)setting);

    outportb((USHORT)(usDebugPortBase + MCR),  DTR | RTS);


    return ;
}

///************************************************************************
// DebugSetupSerial()
//
///************************************************************************
void DebugSetupSerial(ULONG port,ULONG baudrate)
{
	USHORT ports[]={COM1BASE,COM2BASE};

    usDebugPortBase = ports[port-1];
	DebugSetOthers(NO_PARITY,8,1);
	DebugSetSpeed(baudrate);
}
#endif // DEBUG

// EOF
