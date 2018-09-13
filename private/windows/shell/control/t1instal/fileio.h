/***
**
**   Module: FileIO
**
**   Description:
**      This is a module of the T1 to TT font converter. The module
**      is the interface towards all low level I/O functions that are
**      are available on the current platform.
**
**   Author: Michael Jansson
**
**   Created: 5/26/93
**
***/


#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif

#ifndef FASTCALL
#  ifdef MSDOS
#     define FASTCALL   __fastcall
#  else
#     define FASTCALL
#  endif
#endif

#define READONLY  0
#define READWRITE 1

struct ioFile;



/***
** Function: io_Close
**
** Description:
**   This function closes an open file.
***/
errcode           io_CloseFile   _ARGS((INOUT   struct ioFile *fp));


/***
** Function: io_ReadOneByte
**
** Description:
**   This function reads one byte from the current position in 
**   the given file. 
***/
USHORT FASTCALL   io_ReadOneByte _ARGS((INOUT   struct ioFile *fp));


/***
** Function: io_FileError
**
** Description:
**   This function returns the current error status of the file.
***/
boolean           io_FileError   _ARGS((INOUT   struct ioFile *fp));


/***
** Function: io_FileTell
**
** Description:
**   This function returns the current position in the file.
***/
long FASTCALL     io_FileTell    _ARGS((INOUT   struct ioFile *fp));


/***
** Function: io_RemoveFile
**
** Description:
**   This function removes an already closed file.
***/
void FASTCALL     io_RemoveFile  _ARGS((IN      char *name));


/***
** Function: io_OpenFile
**
** Description:
**   This function opens a file.
***/
struct ioFile     *io_OpenFile   _ARGS((IN      char *name,
                                        IN      int mode));


/***
** Function: io_FileSeek
**
** Description:
**   This function moves the current position in the file,
**   relative the beginning of the file.
***/
long FASTCALL     io_FileSeek    _ARGS((INOUT   struct ioFile *fp,
                                        INOUT   long where));


/***
** Function: io_WriteBytes
**
** Description:
**   This function writes a number of bytes, starting at the 
**   current position in the file.
***/
USHORT FASTCALL   io_WriteBytes  _ARGS((IN      UBYTE *,
                                        INOUT   USHORT, struct ioFile *));


/***
** Function: io_ReadBytes
**
** Description:
**   This function reades a number of bytes, starting at the 
**   current position in the file.
***/
USHORT FASTCALL   io_ReadBytes   _ARGS((INOUT   UBYTE *buf,
                                        INOUT   USHORT len,
                                        INOUT   struct ioFile *fp));

