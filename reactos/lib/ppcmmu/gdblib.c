/****************************************************************************

   THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *  Modified for ReactOS by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *  Modified heavily for PowerPC ReactOS by arty
 *
 *  To enable debugger support, two things need to happen.  One, setting
 *  up a routine so that it is in the exception path, is necessary in order
 *  to allow any breakpoints or error conditions to be properly intercepted
 *  and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.
 ER*
 *  Because gdb will sometimes write to the stack area to execute function
 *  calls, this program cannot rely on using the supervisor stack so it
 *  uses it's own stack area.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU Registers  hex data or ENN
 *    G             set the value of the CPU Registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * Checksum.  A packet consists of
 *
 * $<packet info>#<Checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <Checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#include "ppcmmu/mmu.h"

typedef struct _BREAKPOINT {
    int OldCode;
    int *Address;
} BREAKPOINT, *PBREAKPOINT;

BREAKPOINT BreakPoints[64];
char DataOutBuffer[1024];
volatile int DataOutAddr, DataOutCsum;
char DataInBuffer[128];
volatile int DataInAddr, ParseState = 0, ComputedCsum, ActualCsum;
volatile int PacketSent = 0, SendSignal = 0;
volatile int Continue = 0, Signal = 0;
volatile ppc_trap_frame_t RegisterSaves, *RegisterSaveArea = &RegisterSaves;
char *hex = "0123456789abcdef";

#define RCV 0
#define THR 0
#define BAUDLOW 0
#define BAUDHIGH 1
#define IER 1
#define FCR 2
#define ISR 2
#define LCR 3
#define MCR 4
#define LSR 5
#define MSR 6
#define SPR 7

extern void send(char *serport, char c);
extern char recv(char *serport);
extern void setup(char *serport, int baud);

char *serport = (char *)0x800003f8;

int isxdigit(int ch)
{
    return
        (ch >= 'A' && ch <= 'F') ||
        (ch >= 'a' && ch <= 'f') ||
        (ch >= '0' && ch <= '9');
}

inline void sync() {
    __asm__("eieio\n\t"
	    "sync");
}

inline void send(char *serport, char c) {
	/* Wait for Clear to Send */
    while( !(serport[LSR] & 0x20) ) sync();

    serport[THR] = c;
    sync();
}

inline int rdy(char *serport)
{
    sync();
    return (serport[LSR] & 0x20);
}

inline int chr(char *serport)
{
    sync();
    return serport[LSR] & 1;
}

inline char recv(char *serport) {
    char c;

    while( !chr(serport) ) sync();

    c = serport[RCV];
    sync();

    return c;
}

void setup(char *serport, int baud) {
	int x = 115200 / baud;
	serport[LCR] = 128;
	sync();
	serport[BAUDLOW] = x & 255;
	sync();
	serport[BAUDHIGH] = x >> 8;
	sync();
	serport[LCR] = 3;
	sync();
}

void SerialSetUp(int deviceType, void *deviceAddr, int baud)
{
    int i;
    serport = deviceAddr;
    setup(serport, baud);
}

extern int SerialInterrupt(int signal, ppc_trap_frame_t *tf);

void IntEnable()
{
    serport[IER] |= 1;
}

void SerialWrite(int ch)
{
    send(serport, ch);
}

int SerialRead()
{
    return recv(serport);
}

int hex2int(int ch)
{
    if (ch >= 'a' && ch <= 'f') return ch + 10 - 'a';
    else if (ch >= 'A' && ch <= 'F') return ch + 10 - 'A';
    else return ch - '0';
}

int PacketReadHexNumber(int dig)
{
    int i;
    int result = 0;
    for (i = 0; i < dig && isxdigit(DataInBuffer[DataInAddr]); i++)
    {
        result <<= 4;
        result |= hex2int(DataInBuffer[DataInAddr++]);
    }
    return result;
}

void PacketWriteChar(int ch)
{
    DataOutCsum += ch;
    DataOutBuffer[DataOutAddr++] = ch;
}

int PacketWriteHexNumber(int hnum, int dig)
{
    int i;
    hnum <<= (8 - dig) * 4;
    for (i = 0; i < dig; i++)
    {
        PacketWriteChar(hex[(hnum >> 28) & 15]);
        hnum <<= 4;
    }
    return i;
}

void PacketStart()
{
    DataOutCsum = 0;
    DataOutAddr = 0;
}

void PacketFinish()
{
    int i, ch, count = 0;

    PacketSent = 0;

    SerialWrite('$');
    for (i = 0; i < DataOutAddr; i++)
    {
        SerialWrite(DataOutBuffer[i]);
    }
    SerialWrite('#');
    SerialWrite(hex[(DataOutCsum >> 4) & 15]);
    SerialWrite(hex[DataOutCsum & 15]);

    while(!chr(serport) && ((ch = SerialRead()) != '+') && (ch != '$'));
    if (ch == '$')
    {
        ParseState = 0;
        DataInAddr = 0;
        ComputedCsum = 0;
    }
}


void PacketWriteString(char *str)
{
    while(*str) PacketWriteChar(*str++);
}

void PacketOk()
{
    PacketStart();
    PacketWriteString("OK");
    PacketFinish();
}

void PacketEmpty()
{
    PacketStart();
    PacketFinish();
}

void PacketWriteSignal(int code)
{
    PacketStart();
    PacketWriteChar('S');
    PacketWriteHexNumber(code, 2);
    PacketFinish();
}

void PacketWriteError(int code)
{
    PacketStart();
    PacketWriteChar('E');
    PacketWriteHexNumber(code, 2);
    PacketFinish();
}

void marker() { }

void GotPacket()
{
    int i, memaddr, memsize;

    Continue = 0;
    switch (DataInBuffer[DataInAddr++])
    {
    case 'g':
        PacketStart();
        for (i = 0; i < sizeof(*RegisterSaveArea) / sizeof(int); i++)
        {
            PacketWriteHexNumber(((int *)RegisterSaveArea)[i], 8);
        }
        PacketFinish();
        break;

    case 'G':
        for (i = 0; i < sizeof(*RegisterSaveArea) / sizeof(int); i++)
        {
            ((int *)RegisterSaveArea)[i] = PacketReadHexNumber(8);
        }
        PacketOk();
        break;

    case 'm':
        memaddr = PacketReadHexNumber(8);
        DataInAddr++;
        memsize = PacketReadHexNumber(8);
        PacketStart();
        while(memsize-- > 0)
        {
            PacketWriteHexNumber(*((char *)memaddr++), 2);
        }
        PacketFinish();
        break;

    case 'M':
        memaddr = PacketReadHexNumber(8);
        DataInAddr++;
        memsize = PacketReadHexNumber(8);
        DataInAddr++;
        while(memsize-- > 0)
        {
            *((char *)memaddr++) = PacketReadHexNumber(2);
        }
        PacketOk();
        break;

    case '?':
        PacketWriteSignal(Signal);
        break;

    case 'c':
        PacketOk();
        Continue = 1;
        break;

    case 'S':
        PacketOk();
        Continue = 0;
        break;

    case 's':
        RegisterSaveArea->srr1 |= 0x400;
        PacketOk();
        Continue = 1;
        marker();
        break;

    case 'q':
        switch (DataInBuffer[1])
        {
        case 'S': /*upported => nothing*/
            PacketEmpty();
            break;

        case 'O': /*ffsets*/
            PacketEmpty();
            break;
        }
        break;

    default:
        PacketEmpty();
        break;
    }
}

int SerialInterrupt(int signal, ppc_trap_frame_t *tf)
{
    int ch;

    if (!chr(serport)) return 0;

    Signal = signal;
    RegisterSaveArea = tf;

    do
    {
        ch = SerialRead();

        if (ch == 3) /* Break in - tehe */
        {
            Continue = 0;
            PacketWriteSignal(3);
        }
        else if (ch == '-' || ch == '+')
        {
            /* Nothing */
        }
        else if (ch == '$')
        {
            DataInAddr = 0;
            ParseState = 0;
            ComputedCsum = 0;
            ActualCsum = 0;
        }
        else if (ch == '#' && ParseState == 0)
        {
            ParseState = 2;
        }
        else if (ParseState == 0)
        {
            ComputedCsum += ch;
            DataInBuffer[DataInAddr++] = ch;
        }
        else if (ParseState == 2)
        {
            ActualCsum = ch;
            ParseState++;
        }
        else if (ParseState == 3)
        {
            ActualCsum = hex2int(ch) | (hex2int(ActualCsum) << 4);
            ComputedCsum &= 255;
            ParseState = -1;
            if (ComputedCsum == ActualCsum)
            {
                ComputedCsum = 0;
                DataInBuffer[DataInAddr] = 0;
                DataInAddr = 0;
                Continue = 0;
                SerialWrite('+');
                GotPacket();
            }
            else
                SerialWrite('-');
        }
        else if (ParseState == -1)
            SerialWrite('-');
    }
    while (!Continue);
    return 1;
}

int TakeException(int n, ppc_trap_frame_t *tf)
{
    Signal = n;
    RegisterSaveArea = tf;
    PacketWriteSignal(Signal);
    SendSignal = 0;
    Continue = 0;
    while(!Continue) SerialInterrupt(n, tf);
    return 1;
}

/* EOF */
