/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    retypes.h

Abstract:

    HEADER for type remapping (porting from NT code)

Environment:

    LINUX 2.2.X
    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
//typedef unsigned int ULONG,*PULONG;
//typedef unsigned short USHORT,*PUSHORT;
//typedef unsigned char UCHAR,*PUCHAR,BYTE,*PBYTE;

//typedef signed int LONG,*PLONG;
//typedef signed short SHORT,*PSHORT;
//typedef signed char CHAR,*PCHAR,*LPSTR,*PSTR;
//typedef unsigned short WCHAR;

//typedef void VOID,*PVOID;

//typedef char BOOLEAN,*PBOOLEAN;

//#define FALSE (0==1)
//#define TRUE (1==1)
#ifndef NULL
#define NULL ((void*)0)
#endif

// dimension macro
#define DIM(name) (sizeof(name)/sizeof(name[0]))

