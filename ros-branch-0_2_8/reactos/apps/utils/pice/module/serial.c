/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    serial.c

Abstract:

    serial debugger connection

Environment:

    LINUX 2.2.X
    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    19-Aug-2000:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
#include "remods.h"
#include "precomp.h"
#include "serial_port.h"

BOOLEAN SerialReadByte(PUCHAR px);


// used for SERIAL window creation
// NB: at the moment the terminal is 60 lines high.
WINDOW wWindowSerial[4]=
{
	{1,3,1,0,FALSE},
	{5,8,1,0,FALSE},
	{14,26,1,0,FALSE},
	{41,18,1,0,FALSE}
};

PUCHAR pScreenBufferSerial;

USHORT usSerialPortBase;

UCHAR packet[_PAGE_SIZE];
UCHAR assemble_packet[_PAGE_SIZE];

UCHAR flush_buffer[_PAGE_SIZE],g_x,g_y;
ULONG ulFlushBufferPos = 0;

UCHAR ucLastKeyRead;
ECOLORS eForegroundColor=WHITE,eBackgroundColor=BLACK;

///************************************************************************
// SerialSetSpeed()
//
///************************************************************************
void SerialSetSpeed(ULONG baudrate)
{
    UCHAR c;
    ULONG divisor;

    divisor = (ULONG) (115200L/baudrate);

    c = inportb((USHORT)(usSerialPortBase + LCR));
    outportb((USHORT)(usSerialPortBase + LCR), (UCHAR)(c | 0x80)); // Set DLAB
    outportb((USHORT)(usSerialPortBase + DLL), (UCHAR)(divisor & 0x00FF));
    outportb((USHORT)(usSerialPortBase + DLH), (UCHAR)((divisor >> 8) & 0x00FF));
    outportb((USHORT)(usSerialPortBase + LCR), c);          // Reset DLAB

}

///************************************************************************
// SerialSetOthers()
//
// Set other communications parameters
//************************************************************************
void SerialSetOthers(ULONG Parity, ULONG Bits, ULONG StopBit)
{
    ULONG setting;
    UCHAR c;

    if (usSerialPortBase == 0)					return ;
    if (Bits < 5 || Bits > 8)				return ;
    if (StopBit != 1 && StopBit != 2)			return ;
    if (Parity != NO_PARITY && Parity != ODD_PARITY && Parity != EVEN_PARITY)
							return;

    setting  = Bits-5;
    setting |= ((StopBit == 1) ? 0x00 : 0x04);
    setting |= Parity;

    c = inportb((USHORT)(usSerialPortBase + LCR));
    outportb((USHORT)(usSerialPortBase + LCR), (UCHAR)(c & ~0x80)); // Reset DLAB

    // no ints
    outportb((USHORT)(usSerialPortBase + IER), (UCHAR)0);

    // clear FIFO and disable them
    outportb((USHORT)(usSerialPortBase + FCR), (UCHAR)0);

    outportb((USHORT)(usSerialPortBase + LCR), (UCHAR)setting);

    outportb((USHORT)(usSerialPortBase + MCR),  DTR | RTS);


    return ;
}

///************************************************************************
// FlushSerialBuffer()
//
///************************************************************************
void FlushSerialBuffer(void)
{
    UCHAR c;

    while(SerialReadByte(&c));
}

///************************************************************************
// SetupSerial()
//
///************************************************************************
void SetupSerial(ULONG port,ULONG baudrate)
{
	USHORT ports[]={COM1BASE,COM2BASE,COM3BASE,COM4BASE};

    usSerialPortBase = ports[port-1];
	SerialSetOthers(NO_PARITY,8,1);
	SerialSetSpeed(baudrate);

    // clear out received bytes
    // else we would think there's a terminal connected
    FlushSerialBuffer();
}


///************************************************************************
// SerialReadByte()
//
// Output a character to the serial port
//************************************************************************
BOOLEAN SerialReadByte(PUCHAR px)
{
    ULONG timeout;

    timeout = 0x00FFFFL;

    // Wait for transmitter to clear
    while ((inportb((USHORT)(usSerialPortBase + LSR)) & RCVRDY) == 0)
        if (!(--timeout))
        {
			return FALSE;
        }

    *px = inportb((USHORT)(usSerialPortBase + RXR));

    return TRUE;
}

///************************************************************************
// SerialSendByte()
//
// Output a character to the serial port
//************************************************************************
BOOLEAN SerialSendByte(UCHAR x)
{
    ULONG timeout;

    timeout = 0x00FFFFL;

    // Wait for transmitter to clear
    while ((inportb((USHORT)(usSerialPortBase + LSR)) & XMTRDY) == 0)
        if (!(--timeout))
        {
			return FALSE;
        }

    outportb((USHORT)(usSerialPortBase + TXR), x);

	return TRUE;
}

//************************************************************************
// CheckSum()
//
//************************************************************************
UCHAR CheckSum(LPSTR p,ULONG Len)
{
	UCHAR ucCheckSum = 0;
	ULONG i;
	for(i=0;i<Len;i++)
	{
		ucCheckSum ^= *p++;
        ucCheckSum += 1;
	}

	return ucCheckSum;
}


///************************************************************************
// ReadPacket()
//
///************************************************************************
BOOLEAN ReadPacket(PSERIAL_PACKET p)
{
    return TRUE;
}


///************************************************************************
// SendPacket()
//
///************************************************************************
BOOLEAN SendPacket(PSERIAL_PACKET p)
{
    PUCHAR pHeader = (PUCHAR)&p->header;
    ULONG i;
    UCHAR c;
    ULONG timeout;

    do
    {
        timeout = 10;
        pHeader = (PUCHAR)&p->header;
        for(i=0;i<(sizeof(SERIAL_PACKET_HEADER)+p->header.packet_size);i++)
        {
            if(!SerialSendByte(*pHeader++))
            {
                return FALSE;
            }
        }

        do
        {
            c = 0;
            SerialReadByte(&c);
            if(c != ACK)
                ucLastKeyRead = c;
        }while(c != ACK && timeout--);

    }while(c != ACK);

    return TRUE;
}

///************************************************************************
// SendPacketTimeout()
//
///************************************************************************
BOOLEAN SendPacketTimeout(PSERIAL_PACKET p)
{
    PUCHAR pHeader = (PUCHAR)&p->header;
    ULONG i;
    UCHAR c;
    ULONG timeout = 20;
    BOOLEAN bResult = TRUE;

    pHeader = (PUCHAR)&p->header;
    for(i=0;i<(sizeof(SERIAL_PACKET_HEADER)+p->header.packet_size);i++)
    {
        if(!SerialSendByte(*pHeader++))
        {
            return FALSE;
        }
    }

    do
    {
        c = 0xFF;
        SerialReadByte(&c);
    }while(c != ACK && timeout--);

    if(c != ACK)
        bResult = FALSE;

    return bResult;
}


//************************************************************************
// AssemblePacket()
//
//************************************************************************
PSERIAL_PACKET AssemblePacket(PUCHAR pData,ULONG ulSize)
{
    PSERIAL_PACKET p;
    ULONG ulCheckSum;

    p = (PSERIAL_PACKET)assemble_packet;

    // fill in header
    p->header.packet_chksum = CheckSum(pData,ulSize);
    p->header.packet_size = ulSize;
    p->header.packet_header_chksum = 0;
    ulCheckSum = (ULONG)CheckSum((PUCHAR)p,sizeof(SERIAL_PACKET_HEADER));
    p->header.packet_header_chksum = ulCheckSum;
    // attach data to packet
    PICE_memcpy(p->data,pData,ulSize);

    return p;
}


// OUTPUT handlers

//*************************************************************************
// SetForegroundColorVga()
//
//*************************************************************************
void SetForegroundColorSerial(ECOLORS col)
{
    eForegroundColor = col;
}

//*************************************************************************
// SetBackgroundColorVga()
//
//*************************************************************************
void SetBackgroundColorSerial(ECOLORS col)
{
    eBackgroundColor = col;
}


//*************************************************************************
// PrintGrafSerial()
//
//*************************************************************************
void PrintGrafSerial(ULONG x,ULONG y,UCHAR c)
{
    // put this into memory
    pScreenBufferSerial[y*GLOBAL_SCREEN_WIDTH + x] = c;

    // put this into cache
    if(ulFlushBufferPos == 0)
    {
        g_x = x;
        g_y = y;
    }

    flush_buffer[ulFlushBufferPos++] = c;
}

//*************************************************************************
// FlushSerial()
//
//*************************************************************************
void FlushSerial(void)
{
    PSERIAL_DATA_PACKET_PRINT pPrint;
    PSERIAL_PACKET p;

    pPrint = (PSERIAL_DATA_PACKET_PRINT)packet;
    pPrint->type = PACKET_TYPE_PRINT;
    pPrint->x = g_x;
    pPrint->y = g_y;
    pPrint->fgcol = eForegroundColor;
    pPrint->bkcol = eBackgroundColor;
    flush_buffer[ulFlushBufferPos++] = 0;
    PICE_strcpy(pPrint->string,flush_buffer);
    ulFlushBufferPos = 0;

    p = AssemblePacket((PUCHAR)pPrint,sizeof(SERIAL_DATA_PACKET_PRINT)+PICE_strlen(flush_buffer));
    SendPacket(p);
}

//*************************************************************************
// ShowCursorSerial()
//
// show hardware cursor
//*************************************************************************
void ShowCursorSerial(void)
{
    PSERIAL_DATA_PACKET_CURSOR pCursor;
    PSERIAL_PACKET p;

    ENTER_FUNC();

    bCursorEnabled = TRUE;

    pCursor = (PSERIAL_DATA_PACKET_CURSOR)packet;
    pCursor->type = PACKET_TYPE_CURSOR;
    pCursor->state = (UCHAR)TRUE;
    pCursor->x = (UCHAR)wWindow[OUTPUT_WINDOW].usCurX;
    pCursor->y = (UCHAR)wWindow[OUTPUT_WINDOW].usCurY;

    p = AssemblePacket((PUCHAR)pCursor,sizeof(SERIAL_DATA_PACKET_CURSOR));
    SendPacket(p);

    LEAVE_FUNC();
}

//*************************************************************************
// HideCursorSerial()
//
// hide hardware cursor
//*************************************************************************
void HideCursorSerial(void)
{
    PSERIAL_DATA_PACKET_CURSOR pCursor;
    PSERIAL_PACKET p;

    ENTER_FUNC();

    bCursorEnabled = FALSE;

    pCursor = (PSERIAL_DATA_PACKET_CURSOR)packet;
    pCursor->type = PACKET_TYPE_CURSOR;
    pCursor->state = (UCHAR)TRUE;

    p = AssemblePacket((PUCHAR)pCursor,sizeof(SERIAL_DATA_PACKET_CURSOR));
    SendPacket(p);

    LEAVE_FUNC();
}

//*************************************************************************
// CopyLineToSerial()
//
// copy a line from src to dest
//*************************************************************************
void CopyLineToSerial(USHORT dest,USHORT src)
{
    NOT_IMPLEMENTED();
}

//*************************************************************************
// InvertLineSerial()
//
// invert a line on the screen
//*************************************************************************
void InvertLineSerial(ULONG line)
{
    PSERIAL_DATA_PACKET_INVERTLINE pInvertLine;
    PSERIAL_PACKET p;

    pInvertLine = (PSERIAL_DATA_PACKET_INVERTLINE)packet;
    pInvertLine->type = PACKET_TYPE_INVERTLINE;
    pInvertLine->line = line;

    p = AssemblePacket((PUCHAR)pInvertLine,sizeof(SERIAL_DATA_PACKET_INVERTLINE));
    SendPacket(p);
}

//*************************************************************************
// HatchLineSerial()
//
// hatches a line on the screen
//*************************************************************************
void HatchLineSerial(ULONG line)
{
    NOT_IMPLEMENTED();
}

//*************************************************************************
// ClrLineSerial()
//
// clear a line on the screen
//*************************************************************************
void ClrLineSerial(ULONG line)
{
    PSERIAL_DATA_PACKET_CLRLINE pClrLine;
    PSERIAL_PACKET p;

    pClrLine = (PSERIAL_DATA_PACKET_CLRLINE)packet;
    pClrLine->type = PACKET_TYPE_CLRLINE;
    pClrLine->fgcol = eForegroundColor;
    pClrLine->bkcol = eBackgroundColor;
    pClrLine->line = line;

    p = AssemblePacket((PUCHAR)pClrLine,sizeof(SERIAL_DATA_PACKET_CLRLINE));
    SendPacket(p);
}

//*************************************************************************
// PrintLogoSerial()
//
//*************************************************************************
void PrintLogoSerial(BOOLEAN bShow)
{
    NOT_IMPLEMENTED();
}

//*************************************************************************
// PrintCursorSerial()
//
// emulate a blinking cursor block
//*************************************************************************
void PrintCursorSerial(BOOLEAN bForce)
{
    NOT_IMPLEMENTED();
}

//*************************************************************************
// SaveGraphicsStateSerial()
//
//*************************************************************************
void SaveGraphicsStateSerial(void)
{
    // not implemented
}

//*************************************************************************
// RestoreGraphicsStateSerial()
//
//*************************************************************************
void RestoreGraphicsStateSerial(void)
{
    // not implemented
}

// INPUT handlers
//*************************************************************************
// GetKeyPolledSerial()
//
//*************************************************************************
UCHAR GetKeyPolledSerial(void)
{
    UCHAR ucResult;
    PSERIAL_DATA_PACKET_POLL pPoll;
    PSERIAL_PACKET p;

    pPoll =                 (PSERIAL_DATA_PACKET_POLL)packet;
    pPoll->type             = PACKET_TYPE_POLL;
    pPoll->major_version    = PICE_MAJOR_VERSION;
    pPoll->minor_version    = PICE_MINOR_VERSION;
    pPoll->build_number     = PICE_BUILD;

    p = AssemblePacket((PUCHAR)pPoll,sizeof(SERIAL_DATA_PACKET_POLL));
    SendPacket(p);

    ucResult = ucLastKeyRead;

    ucLastKeyRead = 0;

    return ucResult;
}

//*************************************************************************
// FlushKeyboardQueueSerial()
//
//*************************************************************************
void FlushKeyboardQueueSerial(void)
{
    // not implemented
}

//*************************************************************************
// Connect()
//
//*************************************************************************
BOOLEAN Connect(USHORT xSize,USHORT ySize)
{
    PSERIAL_DATA_PACKET_CONNECT pConnect;
    PSERIAL_PACKET p;

    pConnect = (PSERIAL_DATA_PACKET_CONNECT)packet;
    pConnect->type = PACKET_TYPE_CONNECT;
    pConnect->xsize = xSize;
    pConnect->ysize = ySize;

    p = AssemblePacket((PUCHAR)pConnect,sizeof(SERIAL_DATA_PACKET_CONNECT));
    return SendPacketTimeout(p);
}

//*************************************************************************
// ConsoleInitSerial()
//
// init terminal screen
//*************************************************************************
BOOLEAN ConsoleInitSerial(void)
{
	BOOLEAN bResult = FALSE;

    ENTER_FUNC();

    ohandlers.CopyLineTo            = CopyLineToSerial;
    ohandlers.PrintGraf             = PrintGrafSerial;
    ohandlers.Flush                 = FlushSerial;
    ohandlers.ClrLine               = ClrLineSerial;
    ohandlers.InvertLine            = InvertLineSerial;
    ohandlers.HatchLine             = HatchLineSerial;
    ohandlers.PrintLogo             = PrintLogoSerial;
    ohandlers.PrintCursor           = PrintCursorSerial;
    ohandlers.SaveGraphicsState     = SaveGraphicsStateSerial;
    ohandlers.RestoreGraphicsState  = RestoreGraphicsStateSerial;
    ohandlers.ShowCursor            = ShowCursorSerial;
    ohandlers.HideCursor            = HideCursorSerial;
    ohandlers.SetForegroundColor    = SetForegroundColorSerial;
    ohandlers.SetBackgroundColor    = SetBackgroundColorSerial;

    ihandlers.GetKeyPolled          = GetKeyPolledSerial;
    ihandlers.FlushKeyboardQueue    = FlushKeyboardQueueSerial;

    SetWindowGeometry(wWindowSerial);

    GLOBAL_SCREEN_WIDTH = 80;
    GLOBAL_SCREEN_HEIGHT = 60;

	pScreenBufferSerial = PICE_malloc(FRAMEBUFFER_SIZE, NONPAGEDPOOL);

    if(pScreenBufferSerial)
    {
	    bResult = TRUE;

        EmptyRingBuffer();

        SetupSerial(1,115200);

        // connect to terminal, if none's there, we give up
        bResult = Connect(GLOBAL_SCREEN_WIDTH,GLOBAL_SCREEN_HEIGHT);

        if(bResult)
        {
            GetKeyPolledSerial();
        }
    }

    LEAVE_FUNC();

	return bResult;
}


//*************************************************************************
// ConsoleShutdownSerial()
//
// exit terminal screen
//*************************************************************************
void ConsoleShutdownSerial(void)
{
    ENTER_FUNC();

    Connect(80,25);

    FlushSerialBuffer();

    if(pScreenBufferSerial)
        PICE_free(pScreenBufferSerial);

    LEAVE_FUNC();
}



