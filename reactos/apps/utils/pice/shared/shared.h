/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    shared.h

Abstract:

    shared stuff between module and loader

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher
	
	reactos port by:
 			Eugene Ingerman

Revision History:

    13-Nov-1999:	created
    15-Nov-2000:    general cleanup of source files
	
	10/20/2001:		porting to reactos begins

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

#include <ddk/ntddk.h>

// define custom device type
#define PICE_DEVICE_DEBUGGER	64787

#define PICE_IOCTL_LOAD     CTL_CODE(PICE_DEVICE_DEBUGGER, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PICE_IOCTL_UNLOAD   CTL_CODE(PICE_DEVICE_DEBUGGER, 2050, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PICE_IOCTL_RELOAD   CTL_CODE(PICE_DEVICE_DEBUGGER, 2051, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PICE_IOCTL_BREAK    CTL_CODE(PICE_DEVICE_DEBUGGER, 2052, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PICE_IOCTL_STATUS   CTL_CODE(PICE_DEVICE_DEBUGGER, 2053, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _DEBUGGER_STATUS_BLOCK
{
    ULONG Test;
}DEBUGGER_STATUS_BLOCK,*PDEBUGGER_STATUS_BLOCK;

#define MAGIC_ULONG( ch0, ch1, ch2, ch3 ) \
       ( (ULONG)(UCHAR)(ch0) |               \
         ( (ULONG)(UCHAR)(ch1) << 8 ) |      \
         ( (ULONG)(UCHAR)(ch2) << 16 ) |     \
         ( (ULONG)(UCHAR)(ch3) << 24 ) )

#define PICE_MAGIC     MAGIC_ULONG('P','I','C','E')

typedef struct _PICE_SYMBOLFILE_HEADER
{
	ULONG magic;
	char name[32];
	ULONG ulOffsetToHeaders,ulSizeOfHeader;
	ULONG ulOffsetToGlobals,ulSizeOfGlobals;
	ULONG ulOffsetToGlobalsStrings,ulSizeOfGlobalsStrings;
	ULONG ulOffsetToStabs,ulSizeOfStabs;
	ULONG ulOffsetToStabsStrings,ulSizeOfStabsStrings;
	ULONG ulOffsetToSrcFiles,ulNumberOfSrcFiles;
}PICE_SYMBOLFILE_HEADER;

typedef struct _STAB_ENTRY
{
    unsigned long n_strx;
    unsigned char n_type;
    unsigned char n_other;
    unsigned short n_desc;
    unsigned long n_value;
}STAB_ENTRY,*PSTAB_ENTRY;

typedef struct _PICE_SYMBOLFILE_SOURCE
{
    char filename[256];
    ULONG ulOffsetToNext;
}PICE_SYMBOLFILE_SOURCE;





///////////////////////////////////////////////////////////////////////////////////
// serial stuff
typedef struct _SERIAL_PACKET_HEADER
{
    ULONG packet_size;
    ULONG packet_header_chksum;
    ULONG packet_chksum;
}SERIAL_PACKET_HEADER,*PSERIAL_PACKET_HEADER;

typedef struct _SERIAL_PACKET
{
    SERIAL_PACKET_HEADER header;
    UCHAR data[1];
}SERIAL_PACKET,*PSERIAL_PACKET;

#define ACK (0)

typedef enum _ECOLORS
{
    BLACK = 0,
    BLUE,
    GREEN,
    TURK,
    RED,
    VIOLET,
    BROWN,
    LTGRAY,
    GRAY,
    LTBLUE,
    LT_GREEN,
    LTTURK,
    LTRED,
    LTVIOLET,
    YELLOW,
    WHITE
}ECOLORS;

typedef struct _SERIAL_DATA_PACKET
{
    UCHAR type;
    UCHAR data[1];
}SERIAL_DATA_PACKET,*PSERIAL_DATA_PACKET;

#define PACKET_TYPE_CLRLINE     (0)
typedef struct _SERIAL_DATA_PACKET_CLRLINE
{
    UCHAR   type;
    ECOLORS fgcol,bkcol;
    UCHAR   line;
}SERIAL_DATA_PACKET_CLRLINE,*PSERIAL_DATA_PACKET_CLRLINE;

#define PACKET_TYPE_PRINT       (1)
typedef struct _SERIAL_DATA_PACKET_PRINT
{
    UCHAR   type;
    UCHAR   x;
    UCHAR   y;
    ECOLORS fgcol,bkcol;
    UCHAR   string[1];
}SERIAL_DATA_PACKET_PRINT,*PSERIAL_DATA_PACKET_PRINT;

#define PACKET_TYPE_CONNECT (2)
typedef struct _SERIAL_DATA_PACKET_CONNECT
{
    UCHAR type;
    UCHAR xsize,ysize;
}SERIAL_DATA_PACKET_CONNECT,*PSERIAL_DATA_PACKET_CONNECT;

#define PACKET_TYPE_CURSOR (3)
typedef struct _SERIAL_DATA_PACKET_CURSOR
{
    UCHAR type;
    UCHAR state,x,y;
}SERIAL_DATA_PACKET_CURSOR,*PSERIAL_DATA_PACKET_CURSOR;

#define PACKET_TYPE_INVERTLINE (4)
typedef struct _SERIAL_DATA_PACKET_INVERTLINE
{
    UCHAR type;
    UCHAR line;
}SERIAL_DATA_PACKET_INVERTLINE,*PSERIAL_DATA_PACKET_INVERTLINE;

#define PACKET_TYPE_POLL (5)
typedef struct _SERIAL_DATA_PACKET_POLL
{
    UCHAR type;
    USHORT major_version,minor_version,build_number;
}SERIAL_DATA_PACKET_POLL,*PSERIAL_DATA_PACKET_POLL;

// END of serial stuff
///////////////////////////////////////////////////////////////////////////////////


// EOF
