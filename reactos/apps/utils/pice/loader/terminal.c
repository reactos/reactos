/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    termínal.c

Abstract:
	
    serial terminal for pICE headless mode

Environment:

    User mode only

Author:

    Klaus P. Gerlicher

Revision History:

    23-Jan-2001:	created
    
Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
#include "stdinc.h"
#include <curses.h>


#define CONSOLE_WIDTH (80)
#define CONSOLE_HEIGHT (25)

USHORT major_version=0xFFFF,minor_version=0xFFFF,build_number=0xFFFF;

USHORT g_attr = 0;

USHORT usCurX,usCurY,xSize,ySize;

USHORT foreground_color_map[]=
{
};

USHORT background_color_map[]=
{
};


int fd_comm;
struct termios oldtio;

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
// ReadByte()
//
///************************************************************************
BOOLEAN ReadByte(PUCHAR pc)
{
    return (read(fd_comm,pc,1) > 0);
}

///************************************************************************
// SendByte()
//
///************************************************************************
BOOLEAN SendByte(UCHAR c)
{
    return (write(fd_comm,&c,1) > 0);
}


///************************************************************************
// ReadPacket()
//
///************************************************************************
PSERIAL_PACKET ReadPacket(void)
{
    ULONG i;
    PSERIAL_PACKET p;
    SERIAL_PACKET_HEADER header;
    PUCHAR pHeaderRaw,pData;
    char temp[256];
    ULONG ulCheckSum;

    // read a packet header
    pHeaderRaw = (PUCHAR)&header;
    for(i=0;i<sizeof(SERIAL_PACKET_HEADER);i++)
    {
//        //printf("reading()\n");
        if(! ReadByte(pHeaderRaw))          
        {
  //          //printf("no header byte read!\n");
            return NULL;
        }

        pHeaderRaw++;
    }

    //printf("received header!\n");

    ulCheckSum = header.packet_header_chksum;
    header.packet_header_chksum = 0;

    if(ulCheckSum != CheckSum((PUCHAR)&header,sizeof(SERIAL_PACKET_HEADER)) )
    {
        //printf("header checksum mismatch!\n");
        tcflush(fd_comm, TCIFLUSH);
        return NULL;
    }

    p = malloc(sizeof(SERIAL_PACKET_HEADER) + header.packet_size);
    if(!p)
    {
        //printf("out of memory!\n");
        return NULL;
    }
    memcpy(p,&header,sizeof(SERIAL_PACKET_HEADER));

    sprintf(temp,"size %X chksum %x\n",header.packet_size,header.packet_chksum);
    //printf(temp);

    // read the attached data
    pData = (PUCHAR)p + sizeof(header);
    for(i=0;i<header.packet_size;i++)
    {
        if(! ReadByte(pData))          
        {
            //printf("no data byte read!\n");
            return NULL;
        }

        pData++;
    }

    //printf("received data!\n");

    pData = (PUCHAR)p + sizeof(header);
    if(header.packet_chksum != CheckSum(pData,header.packet_size))
    {
        free(p);
        p = NULL;
        //printf("data checksum mismatch!\n");
        return NULL;
    }

    while(!SendByte(ACK));

    return p;
}

///************************************************************************
// SendPacket()
//
///************************************************************************
BOOLEAN SendPacket(PSERIAL_PACKET p)
{
    return TRUE;
}

void DeletePacket(PSERIAL_PACKET p)
{
    free(p);
}

//************************************************************************
// SetupSerial()
//
//************************************************************************
BOOLEAN SetupSerial(ULONG port,ULONG baudrate)
{
    struct termios newtio;
    char* ports[]={"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2","/dev/ttyS3"};
    
    /* 
    Open modem device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */
    //printf("opening comm %s\n",ports[port-1]);
    fd_comm = open(ports[port-1], O_RDWR | O_NOCTTY);
    if (fd_comm <0)
    {
        perror(ports[port-1]); 
        exit(-1); 
    }
    
    //printf("tcgetattr()\n");
    tcgetattr(fd_comm,&oldtio); /* save current modem settings */
    
                           /* 
                           Set bps rate and hardware flow control and 8n1 (8bit,no parity,1 stopbit).
                           Also don't hangup automatically and ignore modem status.
                           Finally enable receiving characters.
    */
    newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
    
    /*
    Ignore bytes with parity errors and make terminal raw and dumb.
    */
    newtio.c_iflag = IGNPAR;
    
    /*
    Raw output.
    */
    newtio.c_oflag = 0;
    
    /*
    Don't echo characters because if you connect to a host it or your
    modem will echo characters for you. Don't generate signals.
    */
    newtio.c_lflag = 0;
    
    /* blocking read until 1 char arrives */
    newtio.c_cc[VMIN]=0;
    newtio.c_cc[VTIME]=0;
    
    /* now clean the modem line and activate the settings for modem */
    //printf("tcflush()\n");
    tcflush(fd_comm, TCIFLUSH);
    //printf("tcsetattr()\n");
    tcsetattr(fd_comm,TCSANOW,&newtio);

    // NCURSES
    initscr();
    refresh();

    return TRUE;
}

//************************************************************************
// CloseSerial()
//
//************************************************************************
void CloseSerial(void)
{
    // NCURSES
    endwin();

    tcsetattr(fd_comm,TCSANOW,&oldtio); /* save current modem settings */
    close(fd_comm);
}

//************************************************************************
// ClrLine()
//
//************************************************************************
void ClrLine(UCHAR line)
{
    move(line,0);
}

//************************************************************************
// InvertLine()
//
//************************************************************************
void InvertLine(UCHAR line)
{
    move(line,0);
}

//************************************************************************
// SetCursorPosition()
//
//************************************************************************
void SetCursorPosition(USHORT x, USHORT y)
{
    move(y,x);
}

//************************************************************************
// GetCursorPosition()
//
//************************************************************************
void GetCursorPosition(PUSHORT px,PUSHORT py)
{
}

//************************************************************************
// SetCursorState()
//
//************************************************************************
void SetCursorState(UCHAR c)
{
}


//************************************************************************
// Print()
//
//************************************************************************
void Print(LPSTR p,USHORT x,USHORT y)
{
    // save the cursor pos
    GetCursorPosition(&usCurX,&usCurY);

    if(y<25)
    {
        SetCursorPosition(x,y);
        refresh();
    
        addstr(p);
        refresh();
        SetCursorPosition(usCurX,usCurY);
    }
}

//************************************************************************
// ProcessPacket()
//
//************************************************************************
void ProcessPacket(PSERIAL_PACKET p)
{
    ULONG ulSize;
    PSERIAL_DATA_PACKET pData;

    pData = (PSERIAL_DATA_PACKET)((PUCHAR)p + sizeof(SERIAL_PACKET_HEADER));
    ulSize = p->header.packet_size;

    switch(pData->type)
    {
        case PACKET_TYPE_CONNECT:
            {
                PSERIAL_DATA_PACKET_CONNECT pDataConnect = (PSERIAL_DATA_PACKET_CONNECT)pData;
                UCHAR i;

                for(i=0;i<ySize;i++)
                    ClrLine(i);

                SetCursorState(0);
                SetCursorPosition(0,0);
//                ResizeConsole(hConsole,pDataConnect->xsize,pDataConnect->ysize);
                xSize = pDataConnect->xsize;
                ySize = pDataConnect->ysize;
            }
            break;
        case PACKET_TYPE_CLRLINE:
            {
                PSERIAL_DATA_PACKET_CLRLINE pDataClrLine = (PSERIAL_DATA_PACKET_CLRLINE)pData;

                ClrLine(pDataClrLine->line);
            }
            break;
        case PACKET_TYPE_INVERTLINE:
            {
                PSERIAL_DATA_PACKET_INVERTLINE pDataInvertLine = (PSERIAL_DATA_PACKET_INVERTLINE)pData;
                
                InvertLine(pDataInvertLine->line);
            }
            break;
        case PACKET_TYPE_PRINT:
            {
                PSERIAL_DATA_PACKET_PRINT pDataPrint = (PSERIAL_DATA_PACKET_PRINT)pData;
                
                Print(pDataPrint->string,pDataPrint->x,pDataPrint->y);
            }
            break;
        case PACKET_TYPE_CURSOR:
            {
                PSERIAL_DATA_PACKET_CURSOR pDataCursor = (PSERIAL_DATA_PACKET_CURSOR)pData;

                SetCursorPosition(pDataCursor->x,pDataCursor->y);
                SetCursorState(pDataCursor->state);
            }
            break;
        case PACKET_TYPE_POLL:
            {
                PSERIAL_DATA_PACKET_POLL pDataPoll= (PSERIAL_DATA_PACKET_POLL)pData;

                if( (major_version != pDataPoll->major_version) ||
                    (minor_version != pDataPoll->minor_version) ||
                    (build_number != pDataPoll->build_number) )
                {
                    major_version = pDataPoll->major_version;
                    minor_version = pDataPoll->minor_version;
                    build_number  = pDataPoll->build_number;

//                    SetAppTitle();
                }

            }
            break;
        default:
            //printf("UNHANDLED\n");
            break;
    }
}

//************************************************************************
// DebuggerShell()
//
//************************************************************************
void DebuggerShell(void)
{
    PSERIAL_PACKET p;

    //printf("DebuggerShell()\n");
    for(;;)
    {
         p = ReadPacket();
         if(p)
         {
             ProcessPacket(p);
             DeletePacket(p);
         }
         else
         {
             usleep(100*1000);
         }
    }
}
